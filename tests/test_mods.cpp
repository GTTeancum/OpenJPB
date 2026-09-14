#include "jpb/mods.h"
#include "jpb/anim.h"
#include "jpb/force.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/jedi.h"
#include "jpb/menu.h"
#include "jpb/game.h"
#include "jpb/io.h"
#include "jpb/savegame.h"
#include <cstring>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>
namespace fs=std::filesystem;
static void check(bool b){if(!b)throw std::runtime_error("mod test failed");}
static void write(const fs::path &p,const std::string &s){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<s;}
static void check_mod_force(int donor, int count, int expectedMotion, int expectedCallback){
    playerObject player{};sceneObject scene{};physicsObject physics{};
    Motion motions[160]{};_animTemplate templates[160]{};
    static uint32_t pose[3]{};
    (anim_InitAnimations)(0);
    auto animation=&maAnimationData[0];
    animation->depack_context.huffdataorigin=pose;
    animation->depack_context3.huffdataorigin=pose;
    animation->depack_context.seqdata=templates;
    animation->animRoot.pParent=&scene.sceneRoot;
    scene.pAnim=&animation->animRoot;scene.pPhysics=&physics.physicsRoot;
    scene.pPlayer=&player.playerRoot;player.playerRoot.pParent=&scene.sceneRoot;
    player.playernum=0;player.playerID=(int16_t)donor;
    player.paMotions=motions;player.maxMotions=count;player.oldmaxCMotions=count;
    player.pMotion=&animation->pMotion;
    for(int i=0;i<count;++i){motions[i].Seq=(uint16_t)i;motions[i].globalID=(uint16_t)i;motions[i].Speed=-1;templates[i].Lframe=10;}
    GameStruct.aCharacterData[0].Force=100;GameStruct.aCharacterData[0].MaxForce=100;
    auto original=mapData[4];int32_t pad[2]={0x10,0};
    jpb_ModSetPlayer(0,115);
    check(force_gActivate(pad,&player)==1);
    check(motions[expectedMotion].FunctPtr==expectedCallback);
    check(std::memcmp(&original,&mapData[4],sizeof(original))==0);
    check(player.playerID==donor);
    /* A Maul body borrowing a Jedi power must obey its donor's Force gate,
     * including delayed animation callbacks after activation has returned. */
    GameStruct.aCharacterData[0].Force=0;
    check(force_RingCallBack(pad,&player)==1);
    jpb_ModSetPlayer(0,-1);
}
int main(){
    auto root=fs::temp_directory_path()/("jpb-mod-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    char error[1024],path[1024];
    try{
        auto pack=root/"mods/one";
        std::string manifest=R"({"version":1,"id":"one","type":"character","modelId":115,"name":"One","model":"one","animationDonor":"mace","forceDonor":"plo","isJedi":true,"soundBank":"mace","bmd":"res/model/one.bmd","cad":"res/animation/one.cad","cmb":"res/combo/one.cmb","portrait":"res/front/one.png","colors":["00000001","00000002","00000001"],"icons":["000000B2","000000B3","000000B2"]})";
        for(auto name:{"res/model/one.bmd","res/animation/one.cad","res/combo/one.cmb","res/front/one.png"})write(pack/name,"asset");
        write(pack/"mod.json",manifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error))==1);
        check(jpb_ModsCount()==1&&jpb_ModDonor(115)==2&&jpb_ModDonor(47)==47);
        check(jpb_ModCharacterById(115)->forceDonor==4);
        auto iconManifest=manifest;
        iconManifest.insert(iconManifest.size()-1,",\"saberIcons\":[\"res/front/default.png\",\"res/front/alternate.png\"]");
        write(pack/"res/front/default.png","default");write(pack/"res/front/alternate.png","alternate");
        write(pack/"mod.json",iconManifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(jedi_GetColorSprite(115)==JPB_MOD_SABER_ICON_BASE+115);
        check(fs::equivalent(jpb_ModSaberIconPath(115),pack/"res/front/default.png"));
        check(jpb_ModToggleColor(115));
        check(fs::equivalent(jpb_ModSaberIconPath(115),pack/"res/front/alternate.png"));
        auto staleIcon=iconManifest;
        auto currentColor=staleIcon.find("00000001\"]");
        staleIcon.replace(currentColor,8,"00000002");
        write(pack/"mod.json",staleIcon);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(fs::equivalent(jpb_ModSaberIconPath(115),pack/"res/front/alternate.png"));
        check(jpb_ModSelectColor(115,0));
        check(fs::equivalent(jpb_ModSaberIconPath(115),pack/"res/front/default.png"));
        fs::remove(pack/"res/front/alternate.png");
        check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(jpb_ModsCount()==1); // Invalid icon cannot replace the active registry.
        write(pack/"mod.json",manifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check_mod_force(2,160,137,9);
        auto maulManifest=manifest;auto donorPos=maulManifest.find("\"animationDonor\":\"mace\"");
        maulManifest.replace(donorPos,std::strlen("\"animationDonor\":\"mace\""),"\"animationDonor\":\"maul\"");
        write(pack/"mod.json",maulManifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error))==1);
        check_mod_force(9,92,67,9);
        menu_setPlayer(0,115);gaPlayerData[0].playerID=9;
        GameStruct.NumPlayers=1;GameStruct.CurrentLevel=3;
        GameStruct.jediLevelPlayed[9][3]=0;
        check(jedi_GetAwardFlags(0,10000)==0);
        check(jpb_ModLevelPlayed(115,3)&&jpb_ModLevelScore(115,3)==10000);
        check(GameStruct.jediLevelPlayed[9][3]==0);
        int skill=0,highest=0;jedi_CalcSkillLevels(115,&skill,&highest);check(highest==3);
        write(pack/"mod.json",manifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error))==1);
        check(jedi_CheckValidPlayer(115)&&jedi_CheckValidVersus(115));
        check(jedi_CheckValidPlayerWTabs(0,115)&&!jedi_CheckValidPlayerWTabs(1,115));
        check(jpb_ModLastSelectable(79)==115&&jedi_CanToggleSaber((model_id)115));
        check(jpb_ModToggleColor(115)&&jedi_GetColorSprite(115)==0xb3);
        check(jedi_GetColour32(115)==2);
        check(jpb_ModToggleColor(115)&&jedi_GetColorSprite(115)==0xb2);
        check(jedi_GetColour32(115)==1);
        menu_setPlayer(0,115);
        check(GameStruct.ModelSelect[0]==2&&jpb_ModPlayerModel(0,2)==115);
        GameStruct.ModelSelect[0]=115;GameStruct.NumPlayers=1;
        jediUpgrades[2].lifeUpgrades=3;
        check(jedi_GetLives()==0);
        game_getUpgrades(115)->lifeUpgrades=2;
        check(jedi_GetLives()==2);jedi_InitLives();check(GameStruct.mNumContinues==1002);
        check(jediUpgrades[2].lifeUpgrades==3);
        jediUpgrades[2].lifeUpgrades=0;GameStruct.ModelSelect[0]=2;
        check(jpb_ModPlayerModel(0,1)==1&&jpb_ModPlayer(1)==nullptr);
        jpb_ModSetPlayer(0,-1);check(jpb_ModPlayerModel(0,2)==2);

        check(jpb_ModsResolve("RES\\MODEL\\ONE.BMD",path,sizeof(path))==1);
        check(jpb_ModsResolve("../outside",path,sizeof(path))==0);
        auto virtual_model=(root/"res/model/one.bmd").string();
        check(jpb_ModsResolvePath(virtual_model.c_str(),path,sizeof(path))==1);
        check(fs::equivalent(path,pack/"res/model/one.bmd"));
        auto shared=(root/"res/animation/shared.tab");write(shared,"stock");
        auto package_shared=(pack/"res/animation/shared.tab").string();
        check(jpb_ModsResolvePath(package_shared.c_str(),path,sizeof(path))==1);
        check(fs::equivalent(path,shared));
        JPBFileHandle handle=0;char contents[6]={0};
        check(file_OPEN(package_shared.data(),&handle)!=0);
        check(file_READ(&handle,contents,5,0)==5);file_CLOSE(&handle);
        check(std::string(contents)=="stock");
        write(root/"SAVEDATA0/Game","save");
        check(!jpb_ModsResolvePath((root/"SAVEDATA0/Game").string().c_str(),path,sizeof(path)));
        check(jpb_ModsBasePath((pack/"res/model/one.bmd").string().c_str(),path,sizeof(path))==1);
        check(fs::path(path).lexically_normal()==fs::path(virtual_model).lexically_normal());

        auto secondManifest=manifest;
        secondManifest.replace(secondManifest.find("\"id\":\"one\""),10,"\"id\":\"two\"");
        secondManifest.replace(secondManifest.find("115"),3,"116");
        fs::create_directories(root/"mods/two");
        fs::copy(pack/"res",root/"mods/two/res",fs::copy_options::recursive);
        write(root/"mods/two/mod.json",secondManifest);
        check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(jpb_ModDonor(115)==jpb_ModDonor(116));
        write(root/"mods/legacy-progress.json",R"({"version":1,"packages":{"one":{"highestLevel":10,"skillPercent":100}}})");
        check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        jedi_CalcSkillLevels(115,&skill,&highest);check(skill==100&&highest==10);
        check(!jpb_ModLevelPlayed(115,10)&&jpb_ModLevelScore(115,10)==0);
        jedi_CalcSkillLevels(116,&skill,&highest);check(skill==0&&highest==0);
        write(root/"mods/legacy-progress.json",R"({"version":1,"packages":{"one":{"highestLevel":11,"skillPercent":100}}})");
        check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(jpb_ModCharacterById(115)->legacyHighestLevel==10);
        write(root/"mods/legacy-progress.json",R"({"version":1,"packages":{"one":{"highestLevel":10,"skillPercent":100}}})");

        menu_setPlayer(0,115);gaPlayerData[0].playerID=2;gaPlayerData[0].playernum=0;
        GameStruct.NumPlayers=1;GameStruct.CurrentLevel=1;gaPlayerData[0].maxCombos=0;
        check(jedi_GetAwardFlags(0,16000)==3);
        check(game_getUpgrades(115)->awardData[1]==2&&game_getUpgrades(116)->awardData[1]==0);
        check(jediUpgrades[2].awardData[1]==0);
        menuVars.scoreCurrentPlayer=0;menuVars.scoreMode=1;menuVars.scoreNextMode=6;
        auto stockCapacity=GameStruct.maxEnergyLevels[2];
        check(menu_handleMenuTriggers(0x7d)==0);
        check(game_getProgressCapacity(115,0)==120&&GameStruct.maxEnergyLevels[2]==stockCapacity);
        check(game_getUpgrades(116)->healthUpgrades==0&&jediUpgrades[2].healthUpgrades==0);
        jpb_ModSetPlayer(0,-1);
        game_getUpgrades(115)->healthUpgrades=3;game_getUpgrades(115)->forceUpgrades=2;
        game_getUpgrades(115)->forcePowers=0x6000;game_getUpgrades(115)->awardData[2]=3;
        check(game_getUpgrades(116)->healthUpgrades==0&&jediUpgrades[2].healthUpgrades==0);
        check(game_getProgressCapacity(115,0)==160&&game_getProgressCapacity(116,0)==100);
        check(game_getProgressCapacity(115,1)==140&&game_getProgressLineLength(115,1)==35);
        check(game_getUpgrades(116)->forcePowers==0&&jediUpgrades[2].forcePowers==0);

        game_disableCombo(115,47);game_disableCombo(116,47);game_disableCombo(2,47);
        game_enableCombo(115,47);
        check(game_getCombo(115,47)!=0&&!game_getCombo(116,47)&&!game_getCombo(2,47));
        check(!game_getCombo(115,48)&&!game_getCombo(255,1));
        check(!jpb_ModLevelPlayed(116,3)&&jpb_ModLevelPlayed(115,3));
        check(jpb_ModRecordLevel(116,3,99));
        check(jpb_ModLevelScore(115,3)==10000&&jpb_ModLevelScore(116,3)==99);
        write(root/"mods/two/mod.json",R"({"enabled":false})");
        check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        auto save=root/"SAVEDATA0/Game";int selected[2]={115,1},restored[2]={2,1};
        const std::string payload="native-save-one",nextPayload="native-save-two";
        check(jpb_ModRecordLevel(115,2,12345));
        check(jpb_ModRecordLevel(115,2,100));
        check(jpb_ModLevelPlayed(115,2)&&jpb_ModLevelScore(115,2)==12345);
        check(!jpb_ModLevelPlayed(2,2)&&!jpb_ModRecordLevel(2,2,99999));
        check(!jpb_ModRecordLevel(115,30,1)&&!jpb_ModRecordLevel(115,-1,1));
        check(jpb_ModToggleColor(115));
        check(jpb_ModSaveCommit(save.string().c_str(),payload.data(),payload.size(),selected)==1);
        check(fs::file_size(save)==payload.size());
        check(jpb_ModRecordLevel(115,2,50000));
        check(jpb_ModToggleColor(115));
        check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==1);
        check(restored[0]==115&&jpb_ModCharacterById(115)->colors[2]==2);
        check(jpb_ModLevelScore(115,2)==12345);
        check(game_getCombo(115,47)!=0);
        {
            auto metadata=fs::path(save.string()+".mods.json");
            std::ifstream stream(metadata,std::ios::binary);
            std::string good((std::istreambuf_iterator<char>(stream)),{});stream.close();
            auto invalid=good;auto at=invalid.find("\"score\":\"12345\"");check(at!=std::string::npos);
            invalid.replace(at,std::strlen("\"score\":\"12345\""),"\"score\":\"4294967296\"");
            write(metadata,invalid);restored[0]=2;
            check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==-1);
            check(restored[0]==2&&jpb_ModLevelScore(115,2)==12345);
            invalid=good;at=invalid.find("\"health\":\"3\"");check(at!=std::string::npos);
            invalid.replace(at,std::strlen("\"health\":\"3\""),"\"health\":\"6\"");write(metadata,invalid);
            check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==-1);
            check(restored[0]==2&&game_getUpgrades(115)->healthUpgrades==3);
            write(metadata,good);
        }
#if defined(_WIN32)
        auto locked=CreateFileW(save.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        check(locked!=INVALID_HANDLE_VALUE);
        check(!jpb_ModSaveCommit(save.string().c_str(),nextPayload.data(),nextPayload.size(),selected));
        CloseHandle(locked);
        restored[0]=2;check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==1&&restored[0]==115);
#endif
        auto renumbered=manifest;renumbered.replace(renumbered.find("115"),3,"150");write(pack/"mod.json",renumbered);
        check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        restored[0]=2;check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==1&&restored[0]==150);
        check(jpb_ModLevelPlayed(150,2)&&jpb_ModLevelScore(150,2)==12345);
        check(game_getCombo(150,47)!=0);
        check(game_getUpgrades(150)->healthUpgrades==3&&game_getUpgrades(150)->forceUpgrades==2);
        check(game_getUpgrades(150)->lifeUpgrades==2&&game_getUpgrades(150)->forcePowers==0x6000);
        check(game_getUpgrades(150)->awardData[2]==3);

        write(pack/"mod.json",R"({"enabled":false})");check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        restored[0]=2;check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==-2&&restored[0]==2);
        jpb_ModResetProgress();
        auto savedStamp=fs::last_write_time(save);fs::last_write_time(save,savedStamp+std::chrono::seconds(10));
        check(jpb_ModSaveRestore(save.string().c_str(),payload.data(),payload.size(),restored)==0);
        check(restored[0]==2);
        write(pack/"mod.json",manifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        check(game_getUpgrades(115)->healthUpgrades==3&&jpb_ModLevelScore(115,2)==12345);
        newGameGameInit();
        check(game_getUpgrades(115)->healthUpgrades==3);
        menu_handleMenuTriggers(9);
        check(!jpb_ModPlayer(0)&&game_getUpgrades(115)->healthUpgrades==0&&jpb_ModLevelScore(115,2)==0);
        jedi_CalcSkillLevels(115,&skill,&highest);check(skill==100&&highest==10);
        fs::last_write_time(save,savedStamp);
        write(pack/"mod.json",manifest);check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        std::memset(&GameStruct,0,sizeof(GameStruct));std::memset(&SaveGameStruct,0,sizeof(SaveGameStruct));
        GameStruct.NumPlayers=1;GameStruct.CurrentLevel=2;menu_setPlayer(0,115);menu_setPlayer(1,1);
        check(jpb_SaveGameWriteFile(save.string().c_str())==JPB_SAVE_OK);
        check(fs::file_size(save)==sizeof(saveGameStruct));
        jpb_ModSetPlayer(0,-1);GameStruct.ModelSelect[0]=0;
        check(jpb_SaveGameReadFile(save.string().c_str())==JPB_SAVE_OK);
        check(GameStruct.ModelSelect[0]==2&&jpb_ModPlayer(0)&&jpb_ModPlayer(0)->modelId==115);
        menuVars.menuModeSP=0;menuVars.menuMode[0]=0;
        menu_initNewMenu();
        check(GameStruct.ModelSelect[0]==0);
        menu_handleMenuTriggers(10);
        check(GameStruct.ModelSelect[0]==2&&jpb_ModPlayerModel(0,2)==115);
        auto meta=fs::path(save.string()+".mods.json");write(meta,"{");
        check(jpb_SaveGameReadFile(save.string().c_str())==JPB_SAVE_INVALID_DATA);
        check(fs::file_size(save)==sizeof(saveGameStruct));

        auto badIcon=manifest;badIcon.replace(badIcon.find("000000B2"),8,"FFFFFFFF");
        write(pack/"mod.json",badIcon);check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        write(pack/"mod.json",manifest+"junk");check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));check(jpb_ModsCount()==1);
        auto escape=manifest;escape.replace(escape.find("res/model/one.bmd"),17,"../outside");write(pack/"mod.json",escape);check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        write(pack/"mod.json",manifest);write(root/"mods/two/mod.json",R"({"version":1,"id":"two","type":"assets"})");write(root/"mods/two/res/model/one.bmd","different");check(!jpb_ModsLoad(root.string().c_str(),error,sizeof(error)));
        write(root/"mods/two/res/model/one.bmd","asset");check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error))==1);
        write(pack/"mod.json",R"({"enabled":false})");check(jpb_ModsLoad(root.string().c_str(),error,sizeof(error))==1&&jpb_ModsCount()==0);
        jpb_ModsReset();check(!jpb_ModsResolve("res/model/one.bmd",path,sizeof(path)));
        check(fs::weakly_canonical(root).parent_path()==fs::weakly_canonical(fs::temp_directory_path()));
        check(root.filename().string().rfind("jpb-mod-test-",0)==0);
        fs::remove_all(root);std::cout<<"mod tests passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<": "<<error<<"\n";return 1;}
}
