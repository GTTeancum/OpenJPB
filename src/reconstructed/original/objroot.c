/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\objroot.c.
 *
 * Provenance:
 *   direct     - name/signature and objectRoot/sceneObject types from PDB.
 *   decompiled - branch structure checked against the raw Ghidra export.
 *   assembly   - parent-if-null behavior, type-to-slot mapping, type-zero
 *                double link, and null return checked at RVA 0xDA640.
 *
 * PDB module: 0057
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\objroot.obj
 * Primary source: W:\SWJediPowerBattles\Work\objroot.c
 * Compiler language: c
 * Emitted procedures: 5
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/objroot.h"
#include "jpb/scene.h"

/* 0xDA5B0, 32 bytes, global, 3 named locals
 * obj_gCheckObjectFlag
 * PDB type: int (objectRoot*, int, long)
 * Source: W:\SWJediPowerBattles\Work\objroot.c
 */
int obj_gCheckObjectFlag(objectRoot *object, int type, uint32_t flag)
{
    sceneObject *scene;
    objectRoot *component;

    if (object->pParent == NULL) {
        return 0;
    }
    scene = (sceneObject *)object->pParent;
    switch (type) {
    case 0:
        component = scene->pScene;
        break;
    case 1:
        component = scene->pModel;
        break;
    case 2:
        component = scene->pPhysics;
        break;
    case 3:
        component = scene->pAnim;
        break;
    case 4:
        component = scene->pPlayer;
        break;
    default:
        return 0;
    }
    return (component->flags & flag) != 0;
}

/* 0xDA5D0, 69 bytes, global, 2 named locals
 * obj_gClearObject
 * PDB type: objectRoot* (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\objroot.c
 */
objectRoot *obj_gClearObject(objectRoot *object)
{
    sceneObject *scene =
        (sceneObject *)object->pParent;

    if (scene != NULL) {
        scene->sceneRoot.flags = 0;
        scene->sceneRoot.objectID = -1;
        scene->pModel->objectID = -1;
        scene->pPhysics->objectID = -1;
        scene->pAnim->objectID = -1;
        scene->pPlayer->objectID = -1;
    }
    return NULL;
}

/* 0xDA620, 19 bytes, global, 4 named locals
 * obj_gClrObjectFlag
 * PDB type: void (objectRoot*, int, long)
 * Source: W:\SWJediPowerBattles\Work\objroot.c
 */
void obj_gClrObjectFlag(objectRoot *child, int type, uint32_t mask)
{
    sceneObject *scene = (sceneObject *)child->pParent;
    objectRoot *target;

    switch (type) {
    case 0:
        target = scene->pScene;
        break;
    case 1:
        target = scene->pModel;
        break;
    case 2:
        target = scene->pPhysics;
        break;
    case 3:
        target = scene->pAnim;
        break;
    case 4:
        target = scene->pPlayer;
        break;
    default:
        return;
    }
    target->flags &= ~mask;
}

/* 0xDA640, 76 bytes, global, 3 named locals
 * obj_gSetChildObject
 * PDB type: objectRoot* (sceneObject*, objec...
 * Source: W:\SWJediPowerBattles\Work\objroot.c
 */
objectRoot *obj_gSetChildObject(
    sceneObject *parent, objectRoot *child, int type)
{
    if (child->pParent == NULL) {
        child->pParent = &parent->sceneRoot;
    }
    switch (type) {
    case 0:
        parent->sceneRoot.pParent = child;
        parent->pScene = child;
        break;
    case 1:
        parent->pModel = child;
        break;
    case 2:
        parent->pPhysics = child;
        break;
    case 3:
        parent->pAnim = child;
        break;
    case 4:
        parent->pPlayer = child;
        break;
    default:
        break;
    }
    return NULL;
}

/* 0xDA690, 26 bytes, global, 4 named locals
 * obj_gSetObjectFlag
 * PDB type: void (objectRoot*, int, long)
 * Source: W:\SWJediPowerBattles\Work\objroot.c
 */
void obj_gSetObjectFlag(objectRoot *child, int type, uint32_t mask)
{
    sceneObject *scene = (sceneObject *)child->pParent;
    objectRoot *target;

    /*
     * The reference indexes the five contiguous component slots directly.
     * A switch expresses the same valid type mapping without relying on
     * pointer arithmetic across separately declared structure members.
     */
    switch (type) {
    case 0:
        target = scene->pScene;
        break;
    case 1:
        target = scene->pModel;
        break;
    case 2:
        target = scene->pPhysics;
        break;
    case 3:
        target = scene->pAnim;
        break;
    case 4:
        target = scene->pPlayer;
        break;
    default:
        return;
    }
    if ((target->flags & mask) == 0) {
        target->flags |= mask;
    }
}
