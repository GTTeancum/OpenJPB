/* nxdk file access for the shared mod loader (mods.cpp). Xbox paths are
   always absolute drive paths ("D:\mods\..."); FATX and the disc have no
   links, so the canonical form is the normalized absolute form. */
#include "../../src/reconstructed/portable/mod_files.h"
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
const DWORD missing = INVALID_FILE_ATTRIBUTES;
DWORD attributes(const std::string &p){return GetFileAttributesA(jpb_mod_files::native(p).c_str());}
void listDirectory(const std::string &directory,std::vector<std::string> &names,bool wantDirectories){
    WIN32_FIND_DATAA data;
    HANDLE find=FindFirstFileA((jpb_mod_files::native(directory)+"\\*").c_str(),&data);
    if(find==INVALID_HANDLE_VALUE)return;
    do{
        if(!std::strcmp(data.cFileName,".")||!std::strcmp(data.cFileName,".."))continue;
        if(((data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)==wantDirectories)names.push_back(data.cFileName);
    }while(FindNextFileA(find,&data));
    FindClose(find);
}
void collect(const std::string &root,const std::string &prefix,std::vector<std::string> &out){
    std::vector<std::string> names;
    listDirectory(prefix.empty()?root:root+"/"+prefix,names,false);
    for(auto &name:names)out.push_back(prefix.empty()?name:prefix+"/"+name);
    names.clear();
    listDirectory(prefix.empty()?root:root+"/"+prefix,names,true);
    for(auto &name:names)collect(root,prefix.empty()?name:prefix+"/"+name,out);
}
}

namespace jpb_mod_files {
std::string absolute(const std::string &p){
    std::string path=p;std::replace(path.begin(),path.end(),'\\','/');
    std::string drive;
    if(path.size()>=2&&path[1]==':'){drive=path.substr(0,2);path.erase(0,2);}
    std::vector<std::string> parts;
    for(size_t start=0;start<=path.size();){
        size_t end=path.find('/',start);if(end==std::string::npos)end=path.size();
        std::string part=path.substr(start,end-start);start=end+1;
        if(part.empty()||part==".")continue;
        if(part==".."){if(!parts.empty())parts.pop_back();continue;}
        parts.push_back(part);
    }
    std::string out=drive.empty()?"D:":drive;
    for(auto &part:parts)out+="/"+part;
    /* A bare drive stays "D:" so callers can append "/res/...". */
    return out;
}
std::string canonical(const std::string &p){return absolute(p);}
std::string native(const std::string &p){std::string out=p;std::replace(out.begin(),out.end(),'/','\\');return out;}
bool exists(const std::string &p){return attributes(p)!=missing;}
bool isFile(const std::string &p){DWORD a=attributes(p);return a!=missing&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
bool isDirectory(const std::string &p){DWORD a=attributes(p);return a!=missing&&(a&FILE_ATTRIBUTE_DIRECTORY);}
/* Queried through an open handle: the disc driver does not answer
   NtQueryFullAttributesFile (GetFileAttributesExA). */
uint64_t size(const std::string &p){
    HANDLE file=CreateFileA(native(p).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    LARGE_INTEGER bytes;
    if(file==INVALID_HANDLE_VALUE)fail("cannot read "+p);
    BOOL ok=GetFileSizeEx(file,&bytes);CloseHandle(file);
    if(!ok)fail("cannot read "+p);
    return (uint64_t)bytes.QuadPart;
}
std::vector<std::string> directories(const std::string &p){
    std::vector<std::string> names;listDirectory(p,names,true);
    std::sort(names.begin(),names.end());
    for(auto &name:names)name=p+"/"+name;
    return names;
}
std::vector<std::string> files(const std::string &p){std::vector<std::string> out;collect(p,"",out);return out;}
/* Reads exactly the file's size: nxdk's fread flags an error, not just end
   of file, when a read runs past the end. */
std::string read(const std::string &p){
    uint64_t bytes=size(p);
    FILE *file=std::fopen(native(p).c_str(),"rb");if(!file)fail("cannot read "+p);
    std::string out((size_t)bytes,'\0');
    bool ok=bytes==0||std::fread(&out[0],1,out.size(),file)==out.size();
    std::fclose(file);
    if(!ok)fail("cannot read "+p);
    return out;
}
void write(const std::string &p,const void *data,size_t size){
    auto path=native(p);
    FILE *file=std::fopen(path.c_str(),"wb");if(!file)fail("cannot write "+p);
    bool ok=std::fwrite(data,1,size,file)==size;ok=std::fclose(file)==0&&ok;
    if(!ok)fail("cannot write "+p);
    /* FATX keeps fclose'd data in the kernel cache; flush it (save.c). */
    HANDLE handle=CreateFileA(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(handle!=INVALID_HANDLE_VALUE){IO_STATUS_BLOCK status;NtFlushBuffersFile(handle,&status);CloseHandle(handle);}
}
bool sameContents(const std::string &a,const std::string &b){
    uint64_t remaining=size(a);
    if(remaining!=size(b))return false;
    FILE *left=std::fopen(native(a).c_str(),"rb"),*right=std::fopen(native(b).c_str(),"rb");
    bool same=left&&right;
    static char x[8192],y[8192];
    while(same&&remaining){
        size_t n=remaining<sizeof(x)?(size_t)remaining:sizeof(x);
        same=std::fread(x,1,n,left)==n&&std::fread(y,1,n,right)==n&&!std::memcmp(x,y,n);
        remaining-=n;
    }
    if(left)std::fclose(left);
    if(right)std::fclose(right);
    return same;
}
std::string stamp(const std::string &p){
    HANDLE file=CreateFileA(native(p).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    FILETIME written;
    if(file==INVALID_HANDLE_VALUE)fail("cannot read "+p);
    BOOL ok=GetFileTime(file,NULL,NULL,&written);CloseHandle(file);
    if(!ok)fail("cannot read "+p);
    return std::to_string(((uint64_t)written.dwHighDateTime<<32)|written.dwLowDateTime);
}
std::string temporary(const std::string &p){
    static unsigned serial=0;
    return p+".tmp"+std::to_string((unsigned)GetTickCount())+"-"+std::to_string(++serial);
}
/* FATX keeps directory changes in the kernel cache; a console switched off
   before the lazy writer runs would lose a rename (and the replaced file
   with it). Flush the volume holding the path. */
void flushVolume(const std::string &path){
    if(path.size()<2||path[1]!=':')return;
    char drive[3]={path[0],':',0};
    ANSI_STRING name;OBJECT_ATTRIBUTES attributes;IO_STATUS_BLOCK status;HANDLE volume;
    RtlInitAnsiString(&name,drive);
    InitializeObjectAttributes(&attributes,&name,OBJ_CASE_INSENSITIVE,ObDosDevicesDirectory(),NULL);
    if(!NT_SUCCESS(NtOpenFile(&volume,GENERIC_WRITE|SYNCHRONIZE,&attributes,&status,
            FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_SYNCHRONOUS_IO_ALERT)))return;
    NtFlushBuffersFile(volume,&status);
    NtClose(volume);
}
void rename(const std::string &from,const std::string &to){
    auto source=native(from),target=native(to);
    if(GetFileAttributesA(target.c_str())!=missing&&!DeleteFileA(target.c_str()))fail("cannot replace "+to);
    if(!MoveFileA(source.c_str(),target.c_str()))fail("cannot rename "+from);
    flushVolume(target);
}
void remove(const std::string &p) noexcept{DeleteFileA(native(p).c_str());}
}
