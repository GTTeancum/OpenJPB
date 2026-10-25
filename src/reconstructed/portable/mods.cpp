#include "jpb/mods.h"
#include "jpb/game.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "mod_files.h"
#if !defined(__cpp_exceptions)
#include <setjmp.h>
#endif

namespace jpb_mod_files {
#if defined(__cpp_exceptions)
void fail(const std::string &message){throw std::runtime_error(message);}
#else
/* nxdk builds without C++ exceptions. A failure jumps back to the guarded
   API entry point; objects owned by the abandoned frames are not destroyed,
   a leak confined to rejecting a package set or a save. */
static jmp_buf *failTarget;
static std::string failMessage;
void fail(const std::string &message){failMessage=message;longjmp(*failTarget,1);}
#endif
}
namespace {
namespace files = jpb_mod_files;
using files::fail;
/* Runs body; a failure (or, on PC, any std::exception) returns failed(what). */
template<class R,class F,class E> R guarded(F body,E failed){
#if defined(__cpp_exceptions)
    try{return body();}catch(const std::exception &e){return failed(e.what());}
#else
    jmp_buf here;jmp_buf *outer=files::failTarget;files::failTarget=&here;
    if(setjmp(here)){files::failTarget=outer;return failed(files::failMessage.c_str());}
    R result=body();files::failTarget=outer;return result;
#endif
}
/* Decimal digits already validated by the caller; bounds std::stoull, which
   aborts on overflow when exceptions are disabled. */
unsigned long long digits(const std::string &value){if(value.size()>19)fail("number out of range");return std::stoull(value);}
struct Json {
    enum Kind { Null, String, Number, Bool, Object, Array } kind = Null;
    std::string text;
    std::map<std::string, Json> object;
    std::vector<Json> array;
    const Json &at(const std::string &key) const {
        auto i = object.find(key);
        if (kind != Object || i == object.end()) fail("missing field: " + key);
        return i->second;
    }
    std::string str() const { if (kind != String) fail("expected string"); return text; }
    int integer() const {
        if (kind != Number || text.find_first_of(".eE") != std::string::npos) fail("expected integer");
        if (text.size() > (text[0] == '-' ? 10u : 9u)) fail("integer out of range");
        return std::stoi(text);
    }
    bool boolean() const { if (kind != Bool) fail("expected boolean"); return text == "true"; }
};
class Parser {
    const std::string &s; size_t pos = 0;
    void ws() { while (pos < s.size() && (s[pos]==' ' || s[pos]=='\n' || s[pos]=='\r' || s[pos]=='\t')) ++pos; }
    char get() { if (pos >= s.size()) fail("truncated JSON"); return s[pos++]; }
    void expect(char c) { ws(); if (get()!=c) fail("invalid JSON delimiter"); }
    unsigned hex4() {
        unsigned n=0;
        for (int i=0;i<4;++i) { char c=get(); int v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
            if(v<0) fail("invalid unicode escape"); n=n*16+v; }
        return n;
    }
    std::string string() {
        expect('"'); std::string out;
        for (;;) {
            unsigned char c=get(); if(c=='"') return out;
            if(c<32) fail("control character in string");
            if(c!='\\') { out+=char(c); continue; }
            c=get();
            switch(c) {
            case '"': case '\\': case '/': out+=char(c); break;
            case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;
            case 'u': {
                unsigned u=hex4();
                if(u>=0xd800 && u<=0xdbff) { if(get()!='\\'||get()!='u') fail("unpaired surrogate"); unsigned low=hex4(); if(low<0xdc00||low>0xdfff) fail("unpaired surrogate"); u=0x10000+((u-0xd800)<<10)+low-0xdc00; }
                else if(u>=0xdc00&&u<=0xdfff) fail("unpaired surrogate");
                if(u==0) fail("NUL in string");
                if(u<0x80) out+=char(u);
                else if(u<0x800) {out+=char(0xc0|(u>>6));out+=char(0x80|(u&63));}
                else if(u<0x10000) {out+=char(0xe0|(u>>12));out+=char(0x80|((u>>6)&63));out+=char(0x80|(u&63));}
                else {out+=char(0xf0|(u>>18));out+=char(0x80|((u>>12)&63));out+=char(0x80|((u>>6)&63));out+=char(0x80|(u&63));}
                break;
            }
            default: fail("invalid escape");
            }
        }
    }
    Json value(int depth) {
        if(depth>32) fail("JSON nesting too deep"); ws(); Json j;
        if(pos==s.size()) fail("missing JSON value");
        char c=s[pos];
        if(c=='"') {j.kind=Json::String;j.text=string();return j;}
        if(c=='{' || c=='[') {
            ++pos; j.kind=c=='{'?Json::Object:Json::Array; char end=c=='{'?'}':']'; ws();
            if(pos<s.size()&&s[pos]==end) {++pos;return j;}
            for(;;) {
                if(c=='{') {auto key=string();expect(':');if(!j.object.emplace(key,value(depth+1)).second) fail("duplicate JSON key: "+key);}
                else j.array.push_back(value(depth+1));
                ws();char sep=get();if(sep==end)return j;if(sep!=',')fail("invalid JSON separator");
            }
        }
        for(const char *literal:{"true","false","null"}) {size_t n=std::strlen(literal);if(s.compare(pos,n,literal)==0){pos+=n;j.kind=*literal=='n'?Json::Null:Json::Bool;j.text=literal;return j;}}
        size_t begin=pos; if(s[pos]=='-')++pos;
        if(pos==s.size()||!std::isdigit((unsigned char)s[pos]))fail("invalid JSON value");
        if(s[pos]=='0')++pos;else while(pos<s.size()&&std::isdigit((unsigned char)s[pos]))++pos;
        if(pos<s.size()&&(s[pos]=='.'||s[pos]=='e'||s[pos]=='E'))fail("manifest numbers must be integers");
        j.kind=Json::Number;j.text=s.substr(begin,pos-begin);return j;
    }
public:
    explicit Parser(const std::string &input):s(input){}
    Json parse(){Json j=value(0);ws();if(pos!=s.size())fail("trailing JSON data");return j;}
};
std::vector<JPBModCharacter> characters;
struct CompletionRecord {uint8_t played[30]{};uint32_t scores[30]{};uint8_t combos[6]{};bool combosInitialized=false;Upgrades upgrades{};bool upgradesInitialized=false;};
std::map<std::string,CompletionRecord> completionRecords;
int playerModels[2]={-1,-1};
std::string assetRoot;
std::vector<std::string> packageRoots;
std::map<std::string,std::string> resources;
std::string lower(std::string v){for(char &c:v){if(c=='\\')c='/';else c=(char)std::tolower((unsigned char)c);}return v;}
template<size_t N> void copy(char (&dst)[N],const std::string &v){if(v.size()>=N)fail("manifest string too long");std::memcpy(dst,v.c_str(),v.size()+1);}
void output(char *dst,size_t n,const std::string &v){if(dst&&n){std::strncpy(dst,v.c_str(),n-1);dst[n-1]=0;}}
std::string read(const std::string &p){if(!files::isFile(p))fail("cannot read "+p);if(files::size(p)>1024*1024)fail("manifest exceeds 1 MiB");return files::read(p);}
bool identifier(const std::string &s){return !s.empty()&&s.size()<96&&std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_';});}
std::string contained(const std::string &root,const std::string &relative){
    if(relative.empty()||relative[0]=='/'||relative[0]=='\\'||relative.find(':')!=std::string::npos)fail("invalid package path");
    for(size_t start=0;start<=relative.size();){
        size_t end=relative.find_first_of("/\\",start);if(end==std::string::npos)end=relative.size();
        if(relative.compare(start,end-start,"..")==0)fail("package traversal");
        start=end+1;
    }
    auto full=files::canonical(root+"/"+relative);if(full.size()>=256)fail("package path exceeds native 255-byte limit");auto base=files::canonical(root);
    if(full.size()<=base.size()+1||full.compare(0,base.size(),base)!=0||full[base.size()]!='/')fail("path escapes package");
    return files::native(full);
}
int donor(const std::string &name){static const char *names[]={"obi_wan","qui_gon","mace","adi","plo","maul_p","amidala","panaka","ki_adi","maul"};for(int i=0;i<10;++i)if(name==names[i])return i;fail("unsupported donor: "+name);}
uint32_t color(const Json &j){auto s=j.str();if(s.size()!=8||s.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)fail("color/icon must be 8 hex digits");return (uint32_t)std::stoul(s,nullptr,16);}
}
extern "C" void jpb_ModResetProgress(){completionRecords.clear();playerModels[0]=playerModels[1]=-1;}
extern "C" void jpb_ModsReset(){completionRecords.clear();characters.clear();resources.clear();packageRoots.clear();assetRoot.clear();playerModels[0]=playerModels[1]=-1;}
extern "C" size_t jpb_ModsCount(){return characters.size();}
extern "C" const JPBModCharacter *jpb_ModCharacterById(int id){for(auto &c:characters)if(c.modelId==id)return &c;return nullptr;}
extern "C" const JPBModCharacter *jpb_ModCharacterAt(size_t i){return i<characters.size()?&characters[i]:nullptr;}
extern "C" uint8_t *jpb_ModComboMask(int id,const uint8_t initial[6]){
    auto c=jpb_ModCharacterById(id);if(!c||!initial)return nullptr;
    auto &r=completionRecords[c->id];if(!r.combosInitialized){std::memcpy(r.combos,initial,6);r.combosInitialized=true;}return r.combos;
}
extern "C" int jpb_ModRecordLevel(int id,int level,uint32_t score){
    auto c=jpb_ModCharacterById(id);if(!c||level<0||level>=30)return 0;
    auto &r=completionRecords[c->id];r.played[level]=1;r.scores[level]=std::max(r.scores[level],score);return 1;
}
extern "C" int jpb_ModLevelPlayed(int id,int level){
    auto c=jpb_ModCharacterById(id);if(!c||level<0||level>=30)return 0;
    auto r=completionRecords.find(c->id);return r!=completionRecords.end()?r->second.played[level]:0;
}
extern "C" uint32_t jpb_ModLevelScore(int id,int level){
    auto c=jpb_ModCharacterById(id);if(!c||level<0||level>=30)return 0;
    auto r=completionRecords.find(c->id);return r!=completionRecords.end()?r->second.scores[level]:0;
}
extern "C" int jpb_ModDonor(int id){auto c=jpb_ModCharacterById(id);return c?c->animationDonor:id;}
extern "C" int jpb_ModsResolve(const char *relative,char *dst,size_t n){if(!relative)return 0;auto i=resources.find(lower(relative));if(i==resources.end()||i->second.size()>=n)return 0;output(dst,n,i->second);return 1;}
extern "C" int jpb_ModsLoad(const char *game_root,char *error,size_t error_size){
    return guarded<int>([&]{
        if(!game_root)fail("missing game root");
        std::string nextRoot=files::absolute(game_root);
        std::vector<std::string> nextPackages;
        std::string root=std::string(game_root)+"/mods";std::vector<JPBModCharacter> next;std::map<std::string,std::string> resourceFiles;std::set<std::string> ids;std::set<int> model_ids;std::vector<std::string> manifests;
        if(files::exists(root))for(auto &directory:files::directories(root))if(files::isFile(directory+"/mod.json"))manifests.push_back(directory+"/mod.json");
        std::sort(manifests.begin(),manifests.end());
        for(auto &manifest:manifests){
            auto j=Parser(read(manifest)).parse();
            if(j.object.count("enabled")&&!j.at("enabled").boolean())continue;
            if(j.at("version").integer()!=1)fail("unsupported manifest version: "+manifest);
            auto id=j.at("id").str();if(!identifier(id)||!ids.insert(id).second)fail("invalid/duplicate package ID: "+id);
            auto package=manifest.substr(0,manifest.size()-sizeof("/mod.json")+1);
            nextPackages.push_back(files::canonical(package));
            nextPackages.push_back(files::absolute(package));
            auto type=j.at("type").str();
            if(type!="character"&&type!="assets")fail("unsupported package type: "+type);
            if(type=="character"){
                JPBModCharacter c{};copy(c.id,id);copy(c.name,j.at("name").str());copy(c.model,j.at("model").str());
                if(!identifier(c.model))fail("invalid model name");
                c.modelId=j.at("modelId").integer();if(c.modelId<JPB_MOD_FIRST_ID||c.modelId>JPB_MOD_LAST_ID||!model_ids.insert(c.modelId).second)fail("invalid/duplicate mod model ID");
                c.animationDonor=donor(j.at("animationDonor").str());c.forceDonor=donor(j.at("forceDonor").str());
                c.isJedi=j.at("isJedi").boolean();c.hidden=j.object.count("hidden")&&j.at("hidden").boolean();copy(c.soundBank,j.at("soundBank").str());
                if(!identifier(c.soundBank))fail("invalid sound bank");
                auto asset=[&](const char *key){auto path=contained(package,j.at(key).str());if(!files::isFile(path))fail("missing asset: "+path);return path;};
                copy(c.bmd,asset("bmd"));copy(c.cad,asset("cad"));copy(c.cmb,asset("cmb"));copy(c.portrait,asset("portrait"));
                if(j.object.count("saberIcons")){
                    const auto &paths=j.at("saberIcons");
                    if(paths.kind!=Json::Array||paths.array.size()!=2)fail("expected default and alternate saber icon paths");
                    for(int i=0;i<2;++i){auto path=contained(package,paths.array[i].str());if(!files::isFile(path))fail("missing saber icon: "+path);copy(c.saberIcons[i],path);}
                }
                const auto &colors=j.at("colors");const auto &icons=j.at("icons");if(colors.kind!=Json::Array||icons.kind!=Json::Array||colors.array.size()!=3||icons.array.size()!=3)fail("expected three colors/icons");
                for(int i=0;i<3;++i){c.colors[i]=color(colors.array[i]);c.icons[i]=color(icons.array[i]);if(c.icons[i]>=249)fail("saber icon outside menu texture table");}
                next.push_back(c);
            }
            auto res=package+"/res";
            if(files::exists(res))for(auto &file:files::files(res)){
                auto relative="res/"+file;auto full=contained(package,relative);auto key=lower(relative);
                auto inserted=resourceFiles.emplace(key,full);
                if(!inserted.second&&!files::sameContents(inserted.first->second,full))fail("conflicting mod resource: "+key);
            }
        }
        auto imported=root+"/legacy-progress.json";
        if(files::exists(imported)){
            auto j=Parser(read(contained(root,"legacy-progress.json"))).parse();
            if(j.at("version").integer()!=1)fail("unsupported legacy progress version");
            const auto &entries=j.at("packages");if(entries.kind!=Json::Object||entries.object.size()>256)fail("invalid legacy progress packages");
            for(const auto &entry:entries.object){
                if(!identifier(entry.first))fail("invalid legacy progress package ID");
                int level=entry.second.at("highestLevel").integer(),skill=entry.second.at("skillPercent").integer();
                if(level<0||level>10||skill<0||skill>100)fail("legacy progress out of range");
                for(auto &c:next)if(entry.first==c.id){c.legacyHighestLevel=level;c.legacySkillPercent=skill;break;}
            }
        }
        std::sort(next.begin(),next.end(),[](const auto&a,const auto&b){return a.modelId<b.modelId;});characters.swap(next);resources.swap(resourceFiles);assetRoot.swap(nextRoot);packageRoots.swap(nextPackages);output(error,error_size,"");return 1;
    },[&](const char *message){output(error,error_size,message);return 0;});
}

extern "C" void jpb_ModSetPlayer(int player,int id){if(player>=0&&player<2)playerModels[player]=jpb_ModCharacterById(id)?id:-1;}
extern "C" const JPBModCharacter *jpb_ModPlayer(int player){return player>=0&&player<2?jpb_ModCharacterById(playerModels[player]):nullptr;}
extern "C" int jpb_ModPlayerModel(int player,int fallback){auto c=jpb_ModPlayer(player);return c&&c->animationDonor==fallback?c->modelId:fallback;}


extern "C" int jpb_ModLastSelectable(int stock_last){return characters.empty()?stock_last:std::max(stock_last,characters.back().modelId);}
extern "C" int jpb_ModToggleColor(int id){
    for(auto &c:characters)if(c.modelId==id){if(!c.isJedi)return 0;int next=c.colors[2]==c.colors[0]?1:0;c.colors[2]=c.colors[next];c.icons[2]=c.icons[next];return 1;}return 0;
}
extern "C" int jpb_ModSelectColor(int id,int alternate){
    if(alternate<0||alternate>1)return 0;
    for(auto &c:characters)if(c.modelId==id){if(!c.isJedi)return 0;c.colors[2]=c.colors[alternate];c.icons[2]=c.icons[alternate];return 1;}return 0;
}
extern "C" const char *jpb_ModSaberIconPath(int id){
    auto c=jpb_ModCharacterById(id);if(!c||!c->isJedi)return nullptr;
    // Some legacy configs persisted the color but left the current icon stale.
    // Prefer the selected color; use the icon only for equal-color variants.
    int slot=c->colors[2]==c->colors[1]&&(c->colors[0]!=c->colors[1]||c->icons[2]==c->icons[1])?1:0;
    return c->saberIcons[slot][0]?c->saberIcons[slot]:nullptr;
}

namespace {
std::string resourceRelative(const char *path){
    if(!path||assetRoot.empty())return {};
    const auto normalized=files::absolute(path);
    const auto key=lower(normalized);
    auto relative=[&](const std::string &root)->std::string{
        const auto prefix=lower(root)+"/res/";
        return key.compare(0,prefix.size(),prefix)==0?normalized.substr(root.size()+1):std::string{};
    };
    auto result=relative(assetRoot);if(!result.empty())return result;
    for(auto &root:packageRoots){result=relative(root);if(!result.empty())return result;}
    return {};
}
int resolvePath(const char *path,char *dst,size_t n,bool baseOnly){
    return guarded<int>([&]{
        auto relative=resourceRelative(path);if(relative.empty())return 0;
        if(!baseOnly&&jpb_ModsResolve(relative.c_str(),dst,n))return 1;
        auto full=assetRoot+"/"+relative;
        if(full.size()>=n||!dst)return 0;output(dst,n,full);return 1;
    },[](const char *){return 0;});
}
}
extern "C" int jpb_ModsSetAssetRoot(const char *root){
    return guarded<int>([&]{if(!root||!*root)return 0;assetRoot=files::absolute(root);return 1;},[](const char *){return 0;});
}
extern "C" int jpb_ModsResolvePath(const char *path,char *dst,size_t n){return resolvePath(path,dst,n,false);}
extern "C" int jpb_ModsBasePath(const char *path,char *dst,size_t n){return resolvePath(path,dst,n,true);}

namespace {
std::string saveError;
Json jsonString(const std::string &s){Json j;j.kind=Json::String;j.text=s;return j;}
Json jsonObject(){Json j;j.kind=Json::Object;return j;}
Json jsonArray(){Json j;j.kind=Json::Array;return j;}
std::string encodeBytes(const void *data,size_t size){
    static const char digits[]="0123456789abcdef";auto bytes=(const unsigned char *)data;std::string s;s.reserve(size*2);
    for(size_t i=0;i<size;++i){s+=digits[bytes[i]>>4];s+=digits[bytes[i]&15];}return s;
}
std::string dumpJson(const Json &j){
    if(j.kind==Json::Null)return "null";
    if(j.kind==Json::Number||j.kind==Json::Bool)return j.text;
    if(j.kind==Json::String){
        std::string s="\"";for(unsigned char c:j.text){if(c=='"'||c=='\\'){s+='\\';s+=char(c);}else if(c<32){const char *h="0123456789abcdef";s+="\\u00";s+=h[c>>4];s+=h[c&15];}else s+=char(c);}return s+"\"";
    }
    std::string s=j.kind==Json::Array?"[":"{";bool first=true;
    if(j.kind==Json::Array)for(auto &v:j.array){if(!first)s+=',';first=false;s+=dumpJson(v);}
    else for(auto &v:j.object){if(!first)s+=',';first=false;s+=dumpJson(jsonString(v.first))+":"+dumpJson(v.second);}
    return s+(j.kind==Json::Array?"]":"}");
}
Json sidecarRead(const std::string &p){
    Json j=Parser(read(p)).parse();
    if(j.at("version").integer()!=1||j.at("snapshots").kind!=Json::Array||j.at("snapshots").array.size()>2)fail("invalid native mod save sidecar");return j;
}
}
extern "C" const char *jpb_ModSaveError(){return saveError.c_str();}
extern "C" int jpb_ModSaveCommit(const char *path,const void *payload,size_t size,const int models[2]){
    std::string rawTemp,metaTemp;
    return guarded<int>([&]{
        saveError.clear();if(!path||!*path||!payload||!models||size>65536)fail("invalid native save arguments");
        std::string raw=path,meta=std::string(path)+".mods.json";
        Json snapshot=jsonObject(),players=jsonArray();bool hasMods=false;
        for(int i=0;i<2;++i){
            auto c=jpb_ModCharacterById(models[i]);if(models[i]>=JPB_MOD_FIRST_ID&&!c)fail("cannot save an unavailable mod character");Json player=jsonObject();
            player.object["id"]=jsonString(c?c->id:"");
            if(c){hasMods=true;player.object["color"]=jsonString(std::to_string(c->colors[2]));player.object["icon"]=jsonString(std::to_string(c->icons[2]));}
            players.array.push_back(player);
        }
        rawTemp=files::temporary(raw);files::write(rawTemp,payload,size);
        if(hasMods||!completionRecords.empty()||files::exists(meta)){
            snapshot.object["game"]=jsonString(encodeBytes(payload,size));snapshot.object["stamp"]=jsonString(files::stamp(rawTemp));snapshot.object["players"]=players;
            Json progress=jsonObject();
            for(const auto &entry:completionRecords){
                Json values=jsonArray();for(int level=0;level<30;++level){
                    Json value=jsonObject();value.object["played"]=jsonString(std::to_string(entry.second.played[level]));
                    value.object["score"]=jsonString(std::to_string(entry.second.scores[level]));values.array.push_back(value);
                }progress.object[entry.first]=values;
            }
            snapshot.object["progress"]=progress;
            Json masks=jsonObject();for(const auto &entry:completionRecords)if(entry.second.combosInitialized){
                Json values=jsonArray();for(auto byte:entry.second.combos)values.array.push_back(jsonString(std::to_string(byte)));
                masks.object[entry.first]=values;
            }snapshot.object["comboMasks"]=masks;
            Json upgrades=jsonObject();for(const auto &entry:completionRecords)if(entry.second.upgradesInitialized){
                const auto &u=entry.second.upgrades;Json value=jsonObject();
                value.object["health"]=jsonString(std::to_string(u.healthUpgrades));
                value.object["force"]=jsonString(std::to_string(u.forceUpgrades));
                value.object["attackDefend"]=jsonString(std::to_string((uint8_t)u.attackDefendUpgrades));
                value.object["lives"]=jsonString(std::to_string(u.lifeUpgrades));
                value.object["powers"]=jsonString(std::to_string((uint16_t)u.forcePowers));
                Json awards=jsonArray();for(auto tier:u.awardData)awards.array.push_back(jsonString(std::to_string(tier)));
                value.object["awards"]=awards;upgrades.object[entry.first]=value;
            }snapshot.object["upgrades"]=upgrades;
            Json document=jsonObject(),version;version.kind=Json::Number;version.text="1";document.object["version"]=version;
            auto snapshots=jsonArray();snapshots.array.push_back(snapshot);
            if(files::exists(meta)&&files::exists(raw)){
                auto old=sidecarRead(meta);auto oldBytes=read(raw);auto oldKey=encodeBytes(oldBytes.data(),oldBytes.size());auto oldStamp=files::stamp(raw);
                for(auto &record:old.at("snapshots").array)if(record.at("game").str()==oldKey&&record.at("stamp").str()==oldStamp){snapshots.array.push_back(record);break;}
            }
            document.object["snapshots"]=snapshots;auto text=dumpJson(document)+"\n";
            metaTemp=files::temporary(meta);files::write(metaTemp,text.data(),text.size());files::rename(metaTemp,meta);metaTemp.clear();
        }
        files::rename(rawTemp,raw);rawTemp.clear();return 1;
    },[&](const char *message){saveError=message;if(!rawTemp.empty())files::remove(rawTemp);if(!metaTemp.empty())files::remove(metaTemp);return 0;});
}
extern "C" Upgrades *jpb_ModUpgrades(int model_id){
    const auto *c=jpb_ModCharacterById(model_id);if(!c)return nullptr;
    auto &r=completionRecords[c->id];
    if(!r.upgradesInitialized){r.upgrades={};if(c->animationDonor==5)r.upgrades.forcePowers=(int16_t)0xf800;r.upgradesInitialized=true;}
    return &r.upgrades;
}
extern "C" int jpb_ModSaveRestore(const char *path,const void *payload,size_t size,int models[2]){
    return guarded<int>([&]{
        saveError.clear();if(!path||!payload||!models||size>65536)fail("invalid native save arguments");
        std::string raw=path,meta=std::string(path)+".mods.json";if(!files::exists(meta))return 0;
        auto document=sidecarRead(meta);auto key=encodeBytes(payload,size);auto rawStamp=files::stamp(raw);
        const Json *selected=nullptr;bool matched=false;
        for(const auto &record:document.at("snapshots").array){
            if(!selected)selected=&record;
            if(record.at("game").str()==key&&record.at("stamp").str()==rawStamp){selected=&record;matched=true;break;}
        }
        if(selected){
            const auto &record=*selected;
            auto &players=record.at("players");if(players.kind!=Json::Array||players.array.size()!=2)fail("invalid native mod save players");
            JPBModCharacter *chosen[2]={};uint32_t colors[2]={},icons[2]={};
            for(int i=0;matched&&i<2;++i){auto id=players.array[i].at("id").str();if(id.empty())continue;if(!identifier(id))fail("invalid saved package ID");
                for(auto &c:characters)if(id==c.id){chosen[i]=&c;break;}
                if(!chosen[i]){saveError="Required mod package is unavailable: "+id;return -2;}
                auto unsignedValue=[&](const char *field){auto v=players.array[i].at(field).str();if(v.empty()||v.find_first_not_of("0123456789")!=std::string::npos)fail("invalid saved color/icon");auto n=digits(v);if(n>UINT32_MAX)fail("saved color/icon out of range");return (uint32_t)n;};
                colors[i]=unsignedValue("color");icons[i]=unsignedValue("icon");if(icons[i]>=249)fail("saved icon out of range");
            }
            std::map<std::string,CompletionRecord> restoredProgress;
            auto progress=record.object.find("progress");
            if(progress!=record.object.end()){
                if(progress->second.kind!=Json::Object||progress->second.object.size()>256)fail("invalid mod completion records");
                for(const auto &entry:progress->second.object){
                    if(!identifier(entry.first)||entry.second.kind!=Json::Array||entry.second.array.size()!=30)fail("invalid mod completion entry");
                    CompletionRecord value;
                    for(int level=0;level<30;++level){
                        const auto &item=entry.second.array[level];auto played=item.at("played").str(),score=item.at("score").str();
                        if((played!="0"&&played!="1")||score.empty()||score.find_first_not_of("0123456789")!=std::string::npos)fail("invalid mod completion value");
                        auto n=digits(score);if(n>UINT32_MAX)fail("mod completion score out of range");
                        value.played[level]=(uint8_t)(played=="1");value.scores[level]=(uint32_t)n;
                    }restoredProgress.emplace(entry.first,value);
                }
            }
            auto masks=record.object.find("comboMasks");
            if(masks!=record.object.end()){
                if(masks->second.kind!=Json::Object||masks->second.object.size()>256)fail("invalid mod combo masks");
                for(const auto &entry:masks->second.object){
                    if(!identifier(entry.first)||entry.second.kind!=Json::Array||entry.second.array.size()!=6)fail("invalid mod combo mask");
                    auto &value=restoredProgress[entry.first];
                    for(int i=0;i<6;++i){auto byte=entry.second.array[i].str();if(byte.empty()||byte.find_first_not_of("0123456789")!=std::string::npos)fail("invalid combo mask byte");auto n=digits(byte);if(n>255)fail("combo mask byte out of range");value.combos[i]=(uint8_t)n;}
                    value.combosInitialized=true;
                }
            }
            auto upgrades=record.object.find("upgrades");
            if(upgrades!=record.object.end()){
                if(upgrades->second.kind!=Json::Object||upgrades->second.object.size()>256)fail("invalid mod upgrades");
                for(const auto &entry:upgrades->second.object){
                    if(!identifier(entry.first))fail("invalid mod upgrade package");
                    auto number=[](const Json &value,unsigned max){auto text=value.str();if(text.empty()||text.find_first_not_of("0123456789")!=std::string::npos)fail("invalid mod upgrade value");auto n=digits(text);if(n>max)fail("mod upgrade out of range");return (unsigned)n;};
                    const auto &item=entry.second;auto &value=restoredProgress[entry.first];auto &u=value.upgrades;
                    u.healthUpgrades=(int8_t)number(item.at("health"),5);u.forceUpgrades=(int8_t)number(item.at("force"),5);
                    u.attackDefendUpgrades=(int8_t)number(item.at("attackDefend"),255);u.lifeUpgrades=(int8_t)number(item.at("lives"),3);
                    u.forcePowers=(int16_t)number(item.at("powers"),65535);
                    const auto &awards=item.at("awards");if(awards.kind!=Json::Array||awards.array.size()!=12)fail("invalid mod awards");
                    for(int i=0;i<12;++i)u.awardData[i]=(int8_t)number(awards.array[i],4);
                    value.upgradesInitialized=true;
                }
            }
            completionRecords.swap(restoredProgress);
            for(int i=0;i<2;++i)if(chosen[i]){models[i]=chosen[i]->modelId;chosen[i]->colors[2]=colors[i];chosen[i]->icons[2]=icons[i];}return matched?1:0;
        }
        return 0;
    },[](const char *message){saveError=message;return -1;});
}
