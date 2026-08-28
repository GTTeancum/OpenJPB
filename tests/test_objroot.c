#include "jpb/objroot.h"
#include "jpb/scene.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                 \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_child_slot_mapping(void)
{
    sceneObject scene;
    objectRoot children[5];
    objectRoot prior_parent;
    int type;

    memset(&scene, 0, sizeof(scene));
    memset(children, 0, sizeof(children));
    memset(&prior_parent, 0, sizeof(prior_parent));
    children[4].pParent = &prior_parent;

    for (type = 0; type < 5; ++type) {
        CHECK(obj_gSetChildObject(&scene, &children[type], type) == NULL);
    }

    CHECK(scene.sceneRoot.pParent == &children[0]);
    for (type = 0; type < 5; ++type) {
        CHECK((&scene.pScene)[type] == &children[type]);
    }
    for (type = 0; type < 4; ++type) {
        CHECK(children[type].pParent == &scene.sceneRoot);
    }
    CHECK(children[4].pParent == &prior_parent);

    scene.pPlayer = NULL;
    CHECK(obj_gSetChildObject(&scene, &children[4], -1) == NULL);
    CHECK(scene.pPlayer == NULL);
    return 0;
}

static int test_flag_slot_indexing(void)
{
    sceneObject scene;
    objectRoot children[5];
    int type;

    memset(&scene, 0, sizeof(scene));
    memset(children, 0, sizeof(children));
    for (type = 0; type < 5; ++type) {
        children[type].pParent = &scene.sceneRoot;
        (&scene.pScene)[type] = &children[type];
        children[type].flags = UINT32_C(1) << type;
    }

    for (type = 0; type < 5; ++type) {
        CHECK(obj_gCheckObjectFlag(
                  &children[0], type, UINT32_C(1) << type) == 1);
        CHECK(obj_gCheckObjectFlag(
                  &children[0], type, UINT32_C(0x40000000)) == 0);
        obj_gSetObjectFlag(
            &children[0], type, UINT32_C(0x100) << type);
        CHECK(children[type].flags ==
              ((UINT32_C(1) << type) | (UINT32_C(0x100) << type)));
        obj_gSetObjectFlag(
            &children[0], type, UINT32_C(0x100) << type);
        obj_gClrObjectFlag(
            &children[0], type, UINT32_C(1) << type);
        CHECK(children[type].flags == (UINT32_C(0x100) << type));
    }

    children[0].pParent = NULL;
    CHECK(obj_gCheckObjectFlag(&children[0], 0, UINT32_MAX) == 0);
    return 0;
}

static int test_clear_object(void)
{
    sceneObject scene;
    objectRoot components[4];
    int index;

    memset(&scene, 0, sizeof(scene));
    memset(components, 0, sizeof(components));
    scene.sceneRoot.flags = UINT32_MAX;
    scene.sceneRoot.objectID = 7;
    scene.pModel = &components[0];
    scene.pPhysics = &components[1];
    scene.pAnim = &components[2];
    scene.pPlayer = &components[3];
    for (index = 0; index < 4; ++index) {
        components[index].objectID = 7;
    }
    components[3].pParent = &scene.sceneRoot;

    CHECK(obj_gClearObject(&components[3]) == NULL);
    CHECK(scene.sceneRoot.flags == 0);
    CHECK(scene.sceneRoot.objectID == -1);
    for (index = 0; index < 4; ++index) {
        CHECK(components[index].objectID == -1);
    }

    components[3].pParent = NULL;
    CHECK(obj_gClearObject(&components[3]) == NULL);
    return 0;
}

int main(void)
{
    if (test_child_slot_mapping() != 0) return 1;
    if (test_flag_slot_indexing() != 0) return 1;
    if (test_clear_object() != 0) return 1;
    puts("objroot tests passed");
    return 0;
}
