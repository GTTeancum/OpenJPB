#include "mod_files.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace jpb_mod_files {
std::string absolute(const std::string &p){return fs::absolute(fs::u8path(p)).lexically_normal().generic_u8string();}
std::string canonical(const std::string &p){return fs::weakly_canonical(fs::u8path(p)).generic_u8string();}
std::string native(const std::string &p){return fs::u8path(p).make_preferred().u8string();}
bool exists(const std::string &p){return fs::exists(fs::u8path(p));}
bool isFile(const std::string &p){return fs::is_regular_file(fs::u8path(p));}
bool isDirectory(const std::string &p){return fs::is_directory(fs::u8path(p));}
uint64_t size(const std::string &p){return fs::file_size(fs::u8path(p));}
std::vector<std::string> directories(const std::string &p){
    std::vector<std::string> out;
    for(auto &e:fs::directory_iterator(fs::u8path(p)))if(e.is_directory())out.push_back(e.path().generic_u8string());
    std::sort(out.begin(),out.end());return out;
}
std::vector<std::string> files(const std::string &p){
    std::vector<std::string> out;auto root=fs::u8path(p);
    for(auto &e:fs::recursive_directory_iterator(root))if(e.is_regular_file())out.push_back(e.path().lexically_relative(root).generic_u8string());
    return out;
}
std::string read(const std::string &p){std::ifstream f(fs::u8path(p),std::ios::binary);if(!f)throw std::runtime_error("cannot read "+p);return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::string &p,const void *data,size_t size){
    std::ofstream f(fs::u8path(p),std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("cannot write "+p);
    f.write((const char *)data,(std::streamsize)size);f.flush();if(!f)throw std::runtime_error("cannot flush "+p);f.close();if(!f)throw std::runtime_error("cannot close "+p);
}
bool sameContents(const std::string &a,const std::string &b){
    if(size(a)!=size(b))return false;
    std::ifstream left(fs::u8path(a),std::ios::binary),right(fs::u8path(b),std::ios::binary);
    char x[8192],y[8192];
    while(left){left.read(x,sizeof(x));right.read(y,sizeof(y));if(left.gcount()!=right.gcount()||std::memcmp(x,y,(size_t)left.gcount()))return false;}
    return !left.bad()&&!right.bad();
}
std::string stamp(const std::string &p){return std::to_string(fs::last_write_time(fs::u8path(p)).time_since_epoch().count());}
std::string temporary(const std::string &p){
    static unsigned serial=0;
    return p+".native-tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial);
}
void rename(const std::string &from,const std::string &to){fs::rename(fs::u8path(from),fs::u8path(to));}
void remove(const std::string &p) noexcept{std::error_code ignored;fs::remove(fs::u8path(p),ignored);}
}
