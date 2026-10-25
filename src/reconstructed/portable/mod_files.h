#ifndef JPB_MOD_FILES_H
#define JPB_MOD_FILES_H
/* File-system access used by mods.cpp. Paths are UTF-8. "Generic" paths use
   '/' separators; "native" paths use the platform's own. The PC build
   implements this with std::filesystem (mod_files_std.cpp); nxdk has no
   std::filesystem, so the Xbox build supplies xbox/src/mod_files.cpp.
   Functions that cannot complete call fail(), which does not return. */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace jpb_mod_files {
/* Defined by mods.cpp: throws on PC; on nxdk (no C++ exceptions) it returns
   to the guarded mod API entry point. */
[[noreturn]] void fail(const std::string &message);
/* Absolute, lexically normal, generic. */
std::string absolute(const std::string &path);
/* Absolute with existing links resolved (weakly canonical), generic. */
std::string canonical(const std::string &path);
std::string native(const std::string &path);
bool exists(const std::string &path);
bool isFile(const std::string &path);
bool isDirectory(const std::string &path);
uint64_t size(const std::string &path);
/* Immediate subdirectories as generic paths "<path>/<name>", sorted. */
std::vector<std::string> directories(const std::string &path);
/* Regular files beneath path, recursively, as generic relative paths. */
std::vector<std::string> files(const std::string &path);
std::string read(const std::string &path);
void write(const std::string &path, const void *data, size_t size);
bool sameContents(const std::string &left, const std::string &right);
/* Opaque modification stamp; equal stamps identify an unchanged file. */
std::string stamp(const std::string &path);
/* Unique sibling path for an atomic replacement of path. */
std::string temporary(const std::string &path);
/* Replaces an existing destination. */
void rename(const std::string &from, const std::string &to);
void remove(const std::string &path) noexcept;
}
#endif
