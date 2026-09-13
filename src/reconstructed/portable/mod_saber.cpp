#include "jpb/mods.h"
#include "jpb/bmd.h"
#include <cstring>

extern "C" int jpb_ModSaberNodes(int player,unsigned *base,unsigned *tip,unsigned *second_base,unsigned *second_tip){
    auto mod=jpb_ModPlayer(player);
    if(!mod||!mod->isJedi||!base||!tip||!second_base||!second_tip)return 0;
    unsigned weapon2=0,weapon3=0,coll2=0,coll4=0;
    for(unsigned i=0;i<JPB_COLLISION_NODE_CAPACITY;++i){
        auto node=coll_GetNode(player,i);
        if(!node||!node->pGeomData)continue;
        auto name=node->pGeomData->name;
        if(std::strncmp(name,"v_weapon2",32)==0)weapon2=i;
        else if(std::strncmp(name,"v_weapon3",32)==0)weapon3=i;
        else if(std::strncmp(name,"v_coll2",32)==0)coll2=i;
        else if(std::strncmp(name,"v_coll4",32)==0)coll4=i;
    }
    if(!weapon2||!coll2)return 0;
    *base=weapon2;*tip=coll2;*second_base=weapon3&&coll4?weapon3:0;*second_tip=weapon3&&coll4?coll4:0;
    return 1;
}
