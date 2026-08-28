/*
 * COMPLETE REVIEWED RECONSTRUCTION
 * PDB module: 0041
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\globalarrays.obj
 * Primary source: W:\SWJediPowerBattles\work\globalarrays.cpp
 * Compiler language: c++
 * Emitted procedures: 46
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/globalarrays.h"

#include <algorithm>
#include <climits>
#include <set>
#include <vector>

int vertexPointerCount;
int normalPointerCount;
int uvPointerCount;
int colorPointerCount;
int indexPointerCount;
int aiPArrayCount;
int wslEnemyPointerCount;

/* 0x1050, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'aiPArraySet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> aiPArraySet = {};

/* 0x1090, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'aiPArrayVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> aiPArrayVec = {};

/* 0x10A0, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'colorPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> colorPointerSet = {};

/* 0x10E0, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'colorPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> colorPointerVec = {};

/* 0x10F0, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'indexPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> indexPointerSet = {};

/* 0x1130, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'indexPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> indexPointerVec = {};

/* 0x1140, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'normalPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> normalPointerSet = {};

/* 0x1180, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'normalPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> normalPointerVec = {};

/* 0x1190, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'uvPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> uvPointerSet = {};

/* 0x11D0, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'uvPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> uvPointerVec = {};

/* 0x11E0, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'vertexPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> vertexPointerSet = {};

/* 0x1220, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'vertexPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> vertexPointerVec = {};

/* 0x1230, 54 bytes, local, 1 named locals
 * `dynamic initializer for 'wslEnemyPointerSet''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::set<void *> wslEnemyPointerSet = {};

/* 0x1270, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'wslEnemyPointerVec''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
std::vector<void *> wslEnemyPointerVec = {};

/* 0xAD4A0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<void * *,void * *>
 * PDB type: void** (void**, void**, void**)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0xAD4D0, 415 bytes, global, 23 named locals
 * std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
 * PDB type: void** std::vector<void *,std::a...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0xAD670, 85 bytes, global, 4 named locals
 * std::_Tree_val<std::_Tree_simple_types<void *> >::_Erase_tree<std::allocator<std::_Tree_node<void *,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xAD6D0, 5 bytes, global, 3 named locals
 * __std_find_trivial<void *,void *>
 * PDB type: void** (void**, void**, void* co...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0xAD6E0, 5 bytes, global, 3 named locals
 * __std_find_trivial<void *,unsigned __int64>
 * PDB type: void** (void**, void**, const un...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0xAD6F0, 252 bytes, global, 5 named locals
 * std::_Tree<std::_Tset_traits<void *,std::less<void *>,std::allocator<void *>,0> >::insert<0,0>
 * PDB type: std::pair<std::_Tree_const_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xAD7F0, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_Tree_node<void *,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_Tree_node<void *,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xAD810, 157 bytes, global, 7 named locals
 * std::set<void *,std::less<void *>,std::allocator<void *> >::operator=
 * PDB type: std::set<void *,std::less<void *...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\set
 */

/* 0xAD8B0, 154 bytes, global, 6 named locals
 * std::vector<void *,std::allocator<void *> >::operator=
 * PDB type: std::vector<void *,std::allocato...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0xAD950, 603 bytes, global, 13 named locals
 * std::_Tree_val<std::_Tree_simple_types<void *> >::_Insert_node
 * PDB type: std::_Tree_node<void *,void *>* ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xADBB0, 17 bytes, global, 0 named locals
 * std::_Throw_tree_length_error
 * PDB type: void ()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xADBD0, 17 bytes, global, 0 named locals
 * std::vector<void *,std::allocator<void *> >::_Xlength
 * PDB type: void std::vector<void *,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0xADBF0, 66 bytes, global, 7 named locals
 * std::allocator<void *>::deallocate
 * PDB type: void std::allocator<void *>::(vo...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xADC40, 1660 bytes, global, 37 named locals
 * addPtr
 * PDB type: int (void*, int)
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
extern "C" int addPtr(void *pointer, int type)
{
#define JPB_ADD_POINTER(pointer_set, pointer_vector)                         \
    do                                                                      \
    {                                                                       \
        if ((pointer_set).find(pointer) == (pointer_set).end())             \
        {                                                                   \
            (pointer_set).insert(pointer);                                  \
            (pointer_vector).push_back(pointer);                            \
            return static_cast<int>((pointer_vector).size()) - 1;           \
        }                                                                   \
        return static_cast<int>(                                            \
            std::find(                                                      \
                (pointer_vector).begin(),                                   \
                (pointer_vector).end(),                                     \
                pointer) -                                                  \
            (pointer_vector).begin());                                      \
    } while (false)

    switch (type)
    {
        case JPB_POINTER_ARRAY_VERTEX:
            JPB_ADD_POINTER(vertexPointerSet, vertexPointerVec);
        case JPB_POINTER_ARRAY_NORMAL:
            JPB_ADD_POINTER(normalPointerSet, normalPointerVec);
        case JPB_POINTER_ARRAY_UV:
            JPB_ADD_POINTER(uvPointerSet, uvPointerVec);
        case JPB_POINTER_ARRAY_COLOR:
            JPB_ADD_POINTER(colorPointerSet, colorPointerVec);
        case JPB_POINTER_ARRAY_INDEX:
            JPB_ADD_POINTER(indexPointerSet, indexPointerVec);
        case JPB_POINTER_ARRAY_AI:
            JPB_ADD_POINTER(aiPArraySet, aiPArrayVec);
        case JPB_POINTER_ARRAY_ENEMY:
            JPB_ADD_POINTER(wslEnemyPointerSet, wslEnemyPointerVec);
        default:
            return INT_MAX;
    }

#undef JPB_ADD_POINTER
}

/* 0xAE2C0, 324 bytes, global, 9 named locals
 * getPtr
 * PDB type: void* (int, int)
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
extern "C" void *getPtr(int index, int type)
{
#define JPB_GET_POINTER(pointer_vector)                                     \
    do                                                                      \
    {                                                                       \
        if (static_cast<std::size_t>(index) >= (pointer_vector).size())     \
        {                                                                   \
            return nullptr;                                                 \
        }                                                                   \
        return (pointer_vector)[static_cast<std::size_t>(index)];           \
    } while (false)

    if (index < 0)
    {
        return nullptr;
    }

    switch (type)
    {
        case JPB_POINTER_ARRAY_VERTEX:
            JPB_GET_POINTER(vertexPointerVec);
        case JPB_POINTER_ARRAY_NORMAL:
            JPB_GET_POINTER(normalPointerVec);
        case JPB_POINTER_ARRAY_UV:
            JPB_GET_POINTER(uvPointerVec);
        case JPB_POINTER_ARRAY_COLOR:
            JPB_GET_POINTER(colorPointerVec);
        case JPB_POINTER_ARRAY_INDEX:
            JPB_GET_POINTER(indexPointerVec);
        case JPB_POINTER_ARRAY_AI:
            JPB_GET_POINTER(aiPArrayVec);
        case JPB_POINTER_ARRAY_ENEMY:
            JPB_GET_POINTER(wslEnemyPointerVec);
        default:
            return nullptr;
    }

#undef JPB_GET_POINTER
}

/* 0xAE410, 1977 bytes, global, 58 named locals
 * initArrays
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\globalarrays.cpp
 */
extern "C" void initArrays(void)
{
    vertexPointerCount = 0;
    indexPointerCount = 0;
    normalPointerCount = 0;
    uvPointerCount = 0;
    colorPointerCount = 0;
    aiPArrayCount = 0;
    wslEnemyPointerCount = 0;

    vertexPointerSet = std::set<void *>();
    vertexPointerVec = std::vector<void *>();
    normalPointerSet = std::set<void *>();
    normalPointerVec = std::vector<void *>();
    uvPointerSet = std::set<void *>();
    uvPointerVec = std::vector<void *>();
    colorPointerSet = std::set<void *>();
    colorPointerVec = std::vector<void *>();
    indexPointerSet = std::set<void *>();
    indexPointerVec = std::vector<void *>();
    aiPArrayVec = std::vector<void *>();
    aiPArraySet = std::set<void *>();
    wslEnemyPointerSet = std::set<void *>();
    wslEnemyPointerVec = std::vector<void *>();
}

extern "C" void pointerRegistry_Reset(void)
{
    initArrays();
}

/* 0x270180, 40 bytes, local, 2 named locals
 * `std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2701B0, 12 bytes, local, 0 named locals
 * `std::_Tree<std::_Tset_traits<void *,std::less<void *>,std::allocator<void *>,0> >::insert<0,0>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2797E0, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'aiPArraySet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279850, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'aiPArrayVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x2798C0, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'colorPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279930, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'colorPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x2799A0, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'indexPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279A10, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'indexPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279A80, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'normalPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279AF0, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'normalPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279B60, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'uvPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279BD0, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'uvPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279C40, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'vertexPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279CB0, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'vertexPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279D20, 99 bytes, local, 3 named locals
 * `dynamic atexit destructor for 'wslEnemyPointerSet''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279D90, 101 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'wslEnemyPointerVec''
 * PDB type: void ()
 * Source: no line mapping
 */
