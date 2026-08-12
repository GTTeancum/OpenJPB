#include "jpb/braindmg.h"
#include "jpb/anim.h"
#include "jpb/effects.h"
#include "jpb/game.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf( \
                stderr, \
                "FAIL %s:%d: %s\n", \
                __FILE__, \
                __LINE__, \
                #condition); \
            ++failures; \
        } \
    } while (0)

static int trajectory_callback(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
    return 0;
}

static int test_damage_control(void)
{
    static WorldData world;
    playerObject player;
    playerObject attacker;
    sceneObject scene;
    physicsObject physics;
    Motion hit_motion;
    Motion current_motion;
    Motion *current_motion_pointer = &current_motion;
    ProjType projectile;

    memset(&world, 0, sizeof(world));
    memset(&player, 0, sizeof(player));
    memset(&attacker, 0, sizeof(attacker));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&hit_motion, 0, sizeof(hit_motion));
    memset(&current_motion, 0, sizeof(current_motion));
    memset(&projectile, 0, sizeof(projectile));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(damageTracking, 0, sizeof(damageTracking));

    gpWorld = &world;
    world.currentDolly = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 0;
    player.target = &attacker;
    player.whohitme = &attacker;
    player.hitMotion = &hit_motion;
    player.pMotion = &current_motion_pointer;
    player.pFlags = UINT32_C(0x21);
    player.forceFlags = UINT32_C(0x20);
    attacker.playerRoot.objectID = 2;
    attacker.playernum = 2;
    attacker.playerID = 10;
    hit_motion.Damage = 10;
    hit_motion.Delay = 3;
    hit_motion.fx1 = -1;
    player.hitNumber = 1;
    gGlobalTimer = 1000;
    GameStruct.NumPlayers = 2;
    GameStruct.AIDamage = 8;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;

    CHECK(braindmg_DamageControl(&player) == 1);
    CHECK(GameStruct.aCharacterData[0].Energy == 90);
    CHECK(player.fStun == 160);
    CHECK(player.hitDelay == UINT32_C(0x09e8));
    CHECK(player.hitNumber == 0);
    CHECK(player.hitMask == UINT32_C(4));
    CHECK(damageTracking[0].current == 10);
    CHECK(attacker.fStun == 0);

    player.hitNumber = 1;
    player.hitDelay = 0;
    player.hitMask = 0;
    player.fStun = 0;
    player.projectile = &projectile;
    projectile.damage = 8;
    projectile.hitReact = 7;
    projectile.hitEffect = -1;
    GameStruct.aCharacterData[0].Energy = 100;
    memset(damageTracking, 0, sizeof(damageTracking));

    CHECK(braindmg_DamageControl(&player) == 1);
    CHECK(player.projectile == NULL);
    CHECK(projType == &projectile);
    CHECK(GameStruct.aCharacterData[0].Energy == 92);
    CHECK(player.hitDelay == UINT32_C(0x23e8));
    CHECK(damageTracking[0].current == 4);

    player.projectile = &projectile;
    player.hitNumber = 3;
    GameStruct.aCharacterData[0].Energy = 100;
    world.player0 = &player;
    world.player1 = &attacker;
    GameStruct.versusModeFlag = 0;

    CHECK(braindmg_DamageControl(&player) == 1);
    CHECK(player.projectile == NULL);
    CHECK(player.hitNumber == 0);
    CHECK(GameStruct.aCharacterData[0].Energy == 100);

    world.aDolly[0].flags = UINT32_C(0x400);
    CHECK(braindmg_DamageControl(&player) == 0x400);
    world.aDolly[0].flags = 0;
    return 0;
}

static int test_player_block_reaction(void)
{
    static WorldData world;
    playerObject player;
    playerObject attacker;
    playerObject lock_target;
    sceneObject scene;
    physicsObject physics;
    Motion motions[50];
    Motion hit_motion;
    Motion *current_motion;
    _animTemplate templates[50];
    animObject *animation;
    animListNode *queued;
    int reaction_motion;
    int i;

    memset(&world, 0, sizeof(world));
    memset(&player, 0, sizeof(player));
    memset(&attacker, 0, sizeof(attacker));
    memset(&lock_target, 0, sizeof(lock_target));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(motions, 0, sizeof(motions));
    memset(&hit_motion, 0, sizeof(hit_motion));
    memset(templates, 0, sizeof(templates));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(damageTracking, 0, sizeof(damageTracking));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));

    gpWorld = &world;
    world.currentDolly = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 0;
    player.paMotions = motions;
    player.maxMotions = 50;
    player.hitMotion = &hit_motion;
    current_motion = &motions[0];
    player.pMotion = &current_motion;
    scene.pPhysics = &physics.physicsRoot;
    scene.pPlayer = &player.playerRoot;
    physics.physicsRoot.pParent = &scene.sceneRoot;
    attacker.playerRoot.objectID = 2;
    attacker.playerID = 10;
    hit_motion.Damage = 10;
    hit_motion.Delay = 3;
    hit_motion.fx1 = -1;
    for (i = 0; i < 50; ++i) {
        motions[i].Seq = (uint16_t)i;
        motions[i].Lock = 25;
        motions[i].Speed = -1;
        templates[i].Lframe = 10;
    }

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    gGlobalTimer = 1000;
    leveldata = NULL;

    /* Prime the exact file-local mpMotion owner without processing a hit,
     * then exercise braindmg_Blocking directly so the visual effect does not
     * obscure the control-state assertions. */
    CHECK(braindmg_DamageControl(&player) == 0);
    player.pFlags = UINT32_C(0x20);
    player.hitNumber = 2;
    srand(1);
    CHECK(braindmg_Blocking(&player, &attacker, 10) == 1);
    reaction_motion = player.currentMotion;
    CHECK(reaction_motion >= 16 && reaction_motion <= 18);
    CHECK(animation->pMotion == &motions[reaction_motion]);
    queued = (animListNode *)animation->animList.head;
    CHECK(queued != NULL);
    CHECK(queued == NULL || queued->pMotion == &motions[21]);
    CHECK(player.fStun == 160);
    CHECK(player.fForce == 10);
    CHECK(player.hitNumber == 0);
    CHECK(player.hitDelay == UINT32_C(0x07e8));

    /* The PDB-backed accumulated-damage threshold rejects a normal block
     * above 128, while one defense upgrade raises that boundary to 133. */
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    player.currentMotion = 0;
    current_motion = &motions[0];
    player.pFlags = UINT32_C(0x20);
    player.hitNumber = 1;
    player.fStun = 0;
    player.fForce = 0;
    damageTracking[0].total = 129.0f;
    CHECK(braindmg_Blocking(&player, &attacker, 10) == 0);
    CHECK(player.currentMotion == 0);
    CHECK(animation->animList.head == NULL);

    jediUpgrades[0].attackDefendUpgrades = UINT8_C(0x10);
    srand(1);
    CHECK(braindmg_Blocking(&player, &attacker, 10) == 1);
    CHECK(player.currentMotion >= 16 && player.currentMotion <= 18);

    /*
     * The matched special-block owner reads player->target at +0x18 for the
     * Obi-Wan/Mace condition, not the hit source passed as target. Keep the
     * two relationships deliberately different so a parameter substitution
     * cannot silently regress this branch. Invalid reaction sequence IDs
     * deliberately stop after the condition's fStun mutation, isolating
     * relationship ownership from animation activation.
     */
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    current_motion = &motions[0];
    player.pMotion = &current_motion;
    player.currentMotion = 0;
    player.playerID = 9;
    player.target = &lock_target;
    player.pFlags = 0;
    player.fStun = 91;
    player.fForce = 0;
    player.hitNumber = 1;
    lock_target.playerID = 2;
    lock_target.currentMotion = 43;
    attacker.playerID = 10;
    attacker.currentMotion = 0;
    motions[16].Seq = 50;
    motions[17].Seq = 50;
    motions[18].Seq = 50;
    srand(1);
    CHECK(braindmg_Blocking(&player, &attacker, 10) == 0);
    CHECK(player.currentMotion == 0);
    CHECK(player.fStun == 0);
    CHECK(player.fForce == 0);
    motions[16].Seq = 16;
    motions[17].Seq = 17;
    motions[18].Seq = 18;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    current_motion = &motions[0];
    player.pMotion = &current_motion;
    player.currentMotion = 0;
    player.pFlags = 0;
    player.fStun = 91;
    player.fForce = 0;
    player.hitNumber = 1;
    lock_target.playerID = 10;
    lock_target.currentMotion = 0;
    attacker.playerID = 2;
    attacker.currentMotion = 43;
    CHECK(braindmg_Blocking(&player, &attacker, 10) == 0);
    CHECK(player.currentMotion == 0);
    CHECK(player.fStun == 91);
    return 0;
}

static int test_forced_death_effect(void)
{
    static WorldData world;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[52];
    Motion hit_motion;
    Motion *current_motion;
    _animTemplate templates[52];
    animObject *animation;
    EffectHeader effect;
    EffectHeader *saved_effect;
    int i;

    memset(&world, 0, sizeof(world));
    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(motions, 0, sizeof(motions));
    memset(&hit_motion, 0, sizeof(hit_motion));
    memset(templates, 0, sizeof(templates));
    memset(&effect, 0, sizeof(effect));
    memset(&GameStruct, 0, sizeof(GameStruct));

    gpWorld = &world;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 35;
    player.paMotions = motions;
    player.maxMotions = 52;
    player.oldmaxCMotions = 52;
    player.hitMotion = &hit_motion;
    current_motion = &motions[0];
    player.pMotion = &current_motion;
    scene.pPhysics = &physics.physicsRoot;
    scene.pPlayer = &player.playerRoot;
    physics.physicsRoot.pParent = &scene.sceneRoot;
    for (i = 0; i < 52; ++i) {
        motions[i].Seq = (uint16_t)i;
        motions[i].Speed = -1;
        templates[i].Lframe = 10;
    }
    motions[23].fx1 = -1;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    LevelSelect = 0;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    CHECK(braindmg_DamageControl(&player) == 0);

    /*
     * Player ID 35 ignores Motion.fx1 and forces retail effect 18. Keeping
     * fx1 at -1 makes an incorrect motion-owned lookup fail immediately.
     */
    saved_effect = paEffects[18];
    paEffects[18] = &effect;
    CHECK(braindmg_DeathReaction(&player, NULL) == 1);
    CHECK(player.currentMotion == 23);
    CHECK((motions[23].motionFlags &
           UINT32_C(0x04000000)) != 0);
    CHECK((player.pFlags & UINT32_C(0x400)) != 0);
    paEffects[18] = saved_effect;
    return 0;
}

int main(void)
{
    playerObject player;
    playerObject attacker;
    playerObject target;
    sceneObject player_scene;
    sceneObject target_scene;
    physicsObject player_physics;
    physicsObject target_physics;
    Motion motions[50];
    _animTemplate templates[50];
    animObject *animation;
    int32_t levels[4] = {0};
    int32_t poly = 2;

    CHECK(test_damage_control() == 0);
    CHECK(test_player_block_reaction() == 0);
    CHECK(test_forced_death_effect() == 0);

    memset(&player, 0, sizeof(player));
    memset(&attacker, 0, sizeof(attacker));
    memset(&target, 0, sizeof(target));
    memset(&player_scene, 0, sizeof(player_scene));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&player_physics, 0, sizeof(player_physics));
    memset(&target_physics, 0, sizeof(target_physics));
    memset(motions, 0, sizeof(motions));
    memset(templates, 0, sizeof(templates));
    memset(damageTracking, 0, sizeof(damageTracking));

    player.playerRoot.objectID = 0;
    player.playerID = 0;
    gGlobalTimer = 1234;
    braindmg_DamageTracker(&player, 10);
    CHECK(damageTracking[0].timer == 1234);
    CHECK(damageTracking[0].hits == 1);
    CHECK(damageTracking[0].current == 10);
    CHECK(damageTracking[0].total == 30.0f);

    player.playerID = 9;
    braindmg_DamageTracker(&player, 50);
    CHECK(damageTracking[0].hits == 2);
    CHECK(damageTracking[0].total == 192.0f);

    attacker.playernum = 5;
    braindmg_LogHits(&player, &attacker);
    CHECK(player.hitMask == UINT32_C(0x20));
    braindmg_LogHits(&player, &attacker);
    CHECK(player.hitMask == UINT32_C(0x20));

    player.playerRoot.pParent = &player_scene.sceneRoot;
    player_scene.pPhysics = &player_physics.physicsRoot;
    target.playerRoot.pParent = &target_scene.sceneRoot;
    target_scene.pPhysics = &target_physics.physicsRoot;
    target_physics.vpos.vz = -100;
    player.target = &target;
    player.paMotions = motions;
    player.playerID = 0;
    player.currentMotion = 13;
    CHECK(braindmg_FindHitReaction(
              &player, &target, 0) == -42);
    player.currentMotion = 37;
    CHECK(braindmg_FindHitReaction(
              &player, &target, 0) == -38);
    player.currentMotion = 10;
    CHECK(braindmg_FindHitReaction(
              &player, &target, 0) == -37);

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    player_scene.pAnim = &animation->animRoot;
    player_scene.pPlayer = &player.playerRoot;
    animation->animRoot.pParent =
        &player_scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    player.paMotions = motions;
    player.maxMotions = 50;
    player.playerRoot.objectID = -1;
    player.currentMotion = 0;
    player.playernum = 0;
    motions[49].Seq = 49;
    motions[49].Speed = -1;
    motions[49].Charge = 100;
    motions[49].Recoil = 200;
    motions[49].RecoilAcc = 300;
    player.hitMask = UINT32_MAX;
    player.hitNumber = 7;
    player.fStun = 9;
    player.delayedMotion = 8;
    player_physics.flags = 0;
    gGlobalTimer = 1000;
    mDrawingSurfaceId = 0;
    jpb_TrajectoryCallbackSlot =
        trajectory_callback;
    CHECK(braindmg_AirHitReaction(
              &player, &target) == 1);
    CHECK(player.currentMotion == 49);
    CHECK(player.pMotionCallBack ==
          trajectory_callback);
    CHECK(player.fStun == 0);
    CHECK(player.hitMask == 0);
    CHECK(player.hitNumber == 0);
    CHECK(player.hitDelay == UINT32_C(0x0fe8));
    CHECK(player.groundDelay == UINT32_C(0x21e8));
    CHECK(player.delayedMotion == 0);
    CHECK(player_physics.airspeed == 200);
    CHECK(player_physics.trajectory == 300);

    player.playerRoot.objectID = 0;
    player.playernum = 1;
    player.playerID = 17;
    player.pFlags = 0;
    player_physics.mapinfo.poly = &poly;
    player_physics.lastpolyhit = NULL;
    levels[2] = INT32_C(0x100000);
    leveldata = levels;
    LevelSelect = 0;
    GameStruct.aCharacterData[1].Energy = 100;
    GameStruct.aCharacterData[1].MaxEnergy = 100;
    CHECK(braindmg_LevelDamage(&player) == 1);

    levels[2] = 0;
    CHECK(braindmg_LevelDamage(&player) == 0);
    player_physics.mapinfo.poly = NULL;
    player_physics.lastpolyhit = &poly;
    levels[2] = INT32_C(0x100000);
    CHECK(braindmg_LevelDamage(&player) == 1);

    player_physics.mapinfo.poly = &poly;
    player_physics.lastpolyhit = NULL;
    LevelSelect = 5;
    CHECK(braindmg_LevelDamage(&player) == 1);
    CHECK(GameStruct.aCharacterData[1].Energy == 75);

    player.pFlags = UINT32_C(1);
    CHECK(braindmg_LevelDamage(&player) == 0);
    player.pFlags = 0;
    GameStruct.aCharacterData[1].Energy = 255;
    CHECK(braindmg_LevelDamage(&player) == 0);

    gGlobalTimer = 5000;
    braindmg_ResetDamageTracker(0);
    CHECK(damageTracking[0].timer == 5000);
    CHECK(damageTracking[0].hits == 0);
    CHECK(damageTracking[0].current == 0);
    CHECK(damageTracking[0].total == 0.0f);

    damageTracking[0].timer = 11;
    damageTracking[1].timer = 22;
    braindmg_ResetDamageTracker(-1);
    braindmg_ResetDamageTracker(2);
    CHECK(damageTracking[0].timer == 11);
    CHECK(damageTracking[1].timer == 22);

    if (failures != 0) {
        fprintf(
            stderr,
            "%d brain damage test(s) failed\n",
            failures);
        return 1;
    }
    puts("brain damage tests passed");
    return 0;
}
