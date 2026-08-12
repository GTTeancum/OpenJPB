#!/usr/bin/env python3
"""Generate the portable-foundation reconstruction queue from PDB evidence."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


FOUNDATIONS = [
    {
        "module": "list",
        "status": "reviewed",
        "risk": "low",
        "depends_on": [],
        "required_globals": [],
        "reason": "Intrusive container used across gameplay systems.",
    },
    {
        "module": "timer",
        "status": "reviewed",
        "risk": "low",
        "depends_on": [],
        "required_globals": [
            "mRoundTimer",
            "timerbucket",
            "buckettime",
            "gputiming",
            "vbls",
        ],
        "reason": "Portable state transitions; VBlank source is a port adapter.",
    },
    {
        "module": "alloc",
        "status": "reviewed",
        "risk": "medium",
        "depends_on": [],
        "required_globals": [],
        "reason": "Legacy heap contract needed before memory pools.",
    },
    {
        "module": "memory",
        "status": "reviewed",
        "risk": "medium",
        "depends_on": ["alloc"],
        "required_globals": [],
        "reason": "Game memory pools and allocation-domain behavior.",
    },
    {
        "module": "fmath",
        "status": "reviewed",
        "risk": "high",
        "depends_on": [],
        "required_globals": [
            {
                "name": "gte_matrix",
                "module": "fmath",
                "rva": "0x537CB8",
            }
        ],
        "reason": (
            "All PDB-named fixed-point, packed-vector, matrix, camera-space, "
            "and projection procedures are reviewed."
        ),
    },
    {
        "module": "flex",
        "status": "reviewed",
        "risk": "medium",
        "depends_on": [],
        "required_globals": [],
        "reason": (
            "All PDB-named fixed-point scalar, cross/dot, projection, packed-"
            "normal, and short-vector procedures are reviewed."
        ),
    },
    {
        "module": "vectors",
        "status": "reviewed",
        "risk": "medium",
        "depends_on": ["fmath"],
        "required_globals": [],
        "reason": "Geometry primitives needed by collision and rendering.",
    },
    {
        "module": "IO",
        "status": "reviewed",
        "risk": "medium",
        "depends_on": [],
        "required_globals": [],
        "reason": "Game-owned standard-C file boundary for PC and Xbox.",
    },
    {
        "module": "filesys",
        "status": "reviewed",
        "risk": "high",
        "depends_on": ["alloc", "memory", "IO"],
        "required_globals": [
            {
                "name": "gFileNotFound",
                "rva": "0x10D7E20",
            }
        ],
        "reason": "Archive parsing and relocation above the portable I/O seam.",
    },
    {
        "module": "jonny",
        "status": "partial",
        "risk": "high",
        "depends_on": [
            "IO",
            "filesys",
            "fmath",
            "vectors",
            "player",
        ],
        "required_globals": [
            {
                "name": "WorldmeshData",
                "rva": "0x10DE658",
            },
            {
                "name": "level_materials",
                "rva": "0x10D7E80",
            },
            {
                "name": "leveldata",
                "rva": "0x537C88",
            },
            {
                "name": "eventlist_start",
                "rva": "0x4AFDA8",
            },
            {
                "name": "eventlist_next",
                "rva": "0x4AFEE8",
            },
            {
                "name": "eventlist_end",
                "rva": "0x4B0028",
            },
        ],
        "reviewed_symbols": [
            "ExtraCharacterEnvironmentEffectExceptions",
            "HitsHit",
            "InitJPX",
            "MTV",
            "clear_eventlist",
        ],
        "reason": "JPX relocation, spatial queries, and exact map-contact event propagation.",
    },
    {
        "module": "intersec",
        "status": "reviewed",
        "risk": "high",
        "depends_on": ["fmath", "flex", "jonny", "physics", "player"],
        "required_globals": [
            {
                "name": "leveldata",
                "rva": "0x537C88",
            },
        ],
        "reason": (
            "Complete PDB-named map and dynamic-solid raycasts, movement "
            "wrappers, frustum clipping, walk height, and zap collision."
        ),
    },
    {
        "module": "camera",
        "status": "partial",
        "risk": "high",
        "depends_on": ["fmath", "vectors"],
        "required_globals": [
            {
                "name": "gCamera",
                "rva": "0x10DE6A0",
            },
            {
                "name": "screenshake",
                "rva": "0x4F1A9C",
            },
            {
                "name": "screenshakeamplitude",
                "rva": "0x4F1AA0",
            },
            {
                "name": "gGlobalFrameRate",
                "rva": "0x4CC0A0",
            },
        ],
        "reviewed_symbols": [
            "camLerp",
            "camera_Camera2ViewVector",
            "camera_CameraSlide",
            "camera_GetCamera",
            "camera_GetLocation",
            "camera_GetViewType",
            "camera_SetShake",
            "camera_SetViewType",
            "camera_SnapCamera",
            "camera_gGetLocation",
        ],
        "reason": "Gameplay camera state and renderer-neutral view-transform inputs.",
    },
    {
        "module": "cube",
        "status": "partial",
        "risk": "high",
        "depends_on": ["camera"],
        "required_globals": [
            {
                "name": "cullmesh",
                "rva": "0x4B0BD0",
            },
        ],
        "reviewed_symbols": [
            "cube_HideMesh",
            "cube_ShowMesh",
            "twatcameramatrix",
        ],
        "reason": (
            "Exact mesh-visibility leaves and the camera-to-world-frustum "
            "conversion used by scene rendering and collision scheduling."
        ),
    },
    {
        "module": "scene",
        "status": "partial",
        "risk": "high",
        "depends_on": ["camera", "fmath", "vectors"],
        "required_globals": [
            {
                "name": "maSceneData",
                "rva": "0x9460C0",
            },
            {
                "name": "v3Translate",
                "rva": "0x946CB0",
            },
            {
                "name": "gGTEMATRIX",
                "rva": "0x10D7E40",
            },
        ],
        "reviewed_symbols": [
            "hurtplayer",
            "scene_AspectCorrectMatrix",
            "scene_DimScreen",
            "scene_GetRawSceneMatrix",
            "scene_GetSceneMatrix",
            "scene_GetViewPos",
            "scene_UpdateWorld2ScreenMatrix",
            "scene_gCreateObject",
            "scene_gGetNewSceneObject",
            "scene_gGetSceneModelMatrix",
            "scene_gGetSceneModelMatrixFV",
            "scene_gGetSnapShotPosition",
            "scene_gInitRoot",
            "scene_gInitScenes",
            "scene_gProject2Screen",
            "scene_gSetSceneModelKeyFrame",
            "scene_gSetSceneModelMatrix",
            "scene_gSetSceneModelMatrixFV",
            "scene_gSetSceneModelMatrixLV",
            "scene_gSetStrobe",
            "scene_gSetWorldPosition",
            "scene_postRender",
            "scene_preRender",
        ],
        "reason": (
            "Exact root/model ownership, command registration, view matrices, "
            "model transforms, projection, player death, and post-render "
            "state publication."
        ),
    },
    {
        "module": "input",
        "status": "partial",
        "risk": "medium",
        "depends_on": [],
        "required_globals": [
            {
                "name": "padMaskBits",
                "rva": "0x4BACA0",
            },
            {
                "name": "padCurrentBits",
                "rva": "0x10D8EA0",
            },
            {
                "name": "padExist",
                "rva": "0x538051",
            },
            {
                "name": "nShockers",
                "rva": "0x10D8E80",
            },
        ],
        "reviewed_symbols": [
            "ClearInput",
            "feedback_startEffect",
            "initPSXPad",
            "input_ReadControlPad",
            "maskPadBits",
        ],
        "reason": "Game-owned pad masks and edge semantics above a replaceable platform reader.",
    },
    {
        "module": "collisn",
        "status": "partial",
        "risk": "high",
        "depends_on": ["vectors"],
        "required_globals": [
            {
                "name": "maNodes",
                "module": "collisn",
                "rva": "0x4F1AF0",
            },
            {
                "name": "mNodeIndex",
                "module": "collisn",
                "rva": "0x4F2EF0",
            },
        ],
        "reviewed_symbols": [
            "coll_4DCollision",
            "coll_CheckForEventNode",
            "coll_CheckForHotNode",
            "coll_CheckForSabreNode",
            "coll_ChkNodeFlags",
            "coll_ClrNodeFlags",
            "coll_GetNode",
            "coll_GetNodeCenter",
            "coll_GetNodeRotation",
            "coll_GetNodeRotationAbs",
            "coll_GetNodeRotationDelta",
            "coll_GetNodeTranslation",
            "coll_GetNodeVelocity",
            "coll_IncNodeRotationAbs",
            "coll_IncNodeRotationDelta",
            "coll_ResetCollisionSystem",
            "coll_ResetPlayerCollision",
            "coll_SetNodeFlags",
            "coll_SetNodeRotationAbs",
            "coll_SetNodeRotationDelta",
            "coll_SetNodeTranslation",
            "coll_SetNodeZBufferOffset",
            "coll_gRegisterNode",
            "old_coll_ZeroNodeTranslation",
        ],
        "reason": "Player/world collision-node state and movement-facing accessors.",
    },
    {
        "module": "unpack",
        "status": "reviewed",
        "risk": "high",
        "depends_on": ["fmath"],
        "required_globals": [],
        "reviewed_symbols": [
            "flushbits",
            "huffgetword",
            "unpack_grabsvectors_raw",
            "unpack_grabsvectors_s",
            "unpack_init",
            "unpack_initcontext",
            "unpack_seekcontext",
        ],
        "reason": "Complete CAD bit reservoir, Huffman lookup/tree decode, raw/compressed vectors, table binding, context initialization, and seek state.",
    },
    {
        "module": "anim",
        "status": "partial",
        "risk": "high",
        "depends_on": ["list", "scene", "unpack"],
        "required_globals": [
            {
                "name": "maAnimationData",
                "rva": "0x4DC680",
            },
        ],
        "reviewed_symbols": [
            "anim_AddNextAnimSeq",
            "anim_CreateObject",
            "anim_InitAnimations",
        ],
        "reason": "Exact animation object pool, fixed motion queue, and motion/frame runtime layouts.",
    },
    {
        "module": "animutil",
        "status": "partial",
        "risk": "high",
        "depends_on": ["anim", "scene"],
        "required_globals": [],
        "reviewed_symbols": [
            "anim_GetSeqFrameRange",
            "anim_GetTargetContext",
            "anim_GetTargetPartNum",
            "anim_GetTargetSeqPtr",
            "anim_CheckFreeze",
            "anim_gDumpSeq",
            "animutl_FlushSeqQueue",
            "animutl_GetCurrentLock",
            "animutl_GetLockLevel",
            "animutl_GetPercentPlayed",
            "animutl_GetTweeningFramesLeft",
            "animutl_GetWindow",
            "animutl_SetCurrentLock",
            "animutl_gGetCurrentAnimLength",
            "animutl_gGetCurrentFrameIndex",
            "animutl_gPauseAnim",
            "animutl_gRestartAnim",
            "animutl_gScaleAnimFrameRate",
            "animutl_gSetAnimFrameRate",
            "animutl_gSetCurrentFrameIndex",
            "animutl_gUnPauseAnim",
        ],
        "reason": "Exact animation query/control layer consumed by player motion selection.",
    },
    {
        "module": "animctrl",
        "status": "partial",
        "risk": "high",
        "depends_on": ["anim", "animutil", "scene"],
        "required_globals": [],
        "reviewed_symbols": [
            "animctrl_MotionChain",
            "animctrl_MotionComboChain",
            "animctrl_MotionEqualLock",
            "animctrl_MotionLock",
            "animctrl_MotionLockLevel",
            "animctrl_MotionNoLock",
        ],
        "reason": "Exact motion queue and lock predicates over bounded forced activation.",
    },
    {
        "module": "combo",
        "status": "partial",
        "risk": "medium",
        "depends_on": ["anim", "brainutl", "input", "scene"],
        "required_globals": [],
        "reviewed_symbols": [
            "combo_CheckCombo",
            "combo_CheckHeldPad",
            "combo_CheckPreCombo",
            "combo_GetHeldTime",
            "combo_ReadCombo",
            "combo_ResetComboEngine",
        ],
        "reason": (
            "Exact combo timing, pad-to-motion buffering, held/released "
            "button masks, and motion-state reset used by player control."
        ),
    },
    {
        "module": "player",
        "status": "partial",
        "risk": "high",
        "depends_on": [
            "brainutl",
            "collisn",
            "combo",
            "input",
            "model",
            "scene",
            "vectors",
        ],
        "required_globals": [
            {
                "name": "gaPlayerData",
                "rva": "0x53A600",
            },
            {
                "name": "gpWorld",
                "rva": "0x4AFDA0",
            },
        ],
        "reviewed_symbols": [
            "player_AfterLife",
            "player_GetPlayerPad",
            "player_ResetJedi",
            "player_gGetNewPlayerObject",
            "player_gGetPlayerPtr",
            "player_gInitPlayers",
        ],
        "reason": (
            "Exact player storage, embedded pad state, pool lifecycle, "
            "afterlife cleanup, and Jedi reset state."
        ),
    },
    {
        "module": "objroot",
        "status": "partial",
        "risk": "medium",
        "depends_on": ["scene"],
        "required_globals": [],
        "reviewed_symbols": [
            "obj_gCheckObjectFlag",
            "obj_gSetChildObject",
            "obj_gSetObjectFlag",
        ],
        "reason": "Exact scene-to-subsystem parent/child ownership links.",
    },
    {
        "module": "sprite",
        "status": "partial",
        "risk": "high",
        "depends_on": ["alloc", "fmath", "flex", "list"],
        "required_globals": [
            {"name": "aCircle", "rva": "0x4BA510"},
            {"name": "framerate", "rva": "0x4CC0A8"},
            {"name": "mDrawingSurfaceId", "rva": "0x53D270"},
            {"name": "effects1Handle", "rva": "0x547EC0"},
            {"name": "mSCBDraw", "rva": "0x548050"},
            {
                "name": "mSpriteWork",
                "module": "sprite",
                "rva": "0x548070",
            },
            {
                "name": "mCurSpriteList",
                "module": "sprite",
                "rva": "0x548090",
            },
            {
                "name": "shot",
                "module": "sprite",
                "rva": "0x5480A0",
            },
            {"name": "gGlobalTimer", "rva": "0x581FC0"},
            {"name": "OptionStruct", "rva": "0x10DA100"},
        ],
        "reviewed_symbols": [
            "sprite_AddCallBack",
            "sprite_AddSpriteEffect",
            "sprite_AddSpriteEffectAtNode",
            "sprite_AllocRing",
            "sprite_CommentsCallBack",
            "sprite_FireRing",
            "sprite_GetCommentsSprite",
            "sprite_GetPointsSprite",
            "sprite_MainCallBack",
            "sprite_PointsCallBack",
            "sprite_SmallPointsCallBack",
            "sprite_gAllocSCB",
            "sprite_gAllocSprite",
            "sprite_gFreeSCB",
            "sprite_gFreeSprite",
            "sprite_gHideSCB",
            "sprite_gHideSprite",
            "sprite_gInitSprites",
            "sprite_gMoveSpritePosition",
            "sprite_gSetSpritePosition",
            "sprite_gUnHideSprite",
        ],
        "reason": (
            "Exact dependency-light effect allocation, node forwarding, ring "
            "construction, callback motion, and sprite/SCB lifecycle."
        ),
    },
    {
        "module": "sound",
        "status": "partial",
        "risk": "medium",
        "depends_on": [],
        "required_globals": [],
        "reviewed_symbols": [
            "sound_Play",
            "sound_PlayFV",
            "sound_StopSound",
        ],
        "reason": "Exact bank fallback and float-position conversion over a dependency-light platform audio boundary.",
    },
    {
        "module": "achievement",
        "status": "partial",
        "risk": "low",
        "depends_on": ["platform"],
        "required_globals": [],
        "reviewed_symbols": [
            "achievement_complete",
            "achievement_getcount",
            "achievement_update",
        ],
        "reason": "Exact achievement completion and platinum scan over a portable platform service boundary.",
    },
    {
        "module": "brainutl",
        "status": "partial",
        "risk": "high",
        "depends_on": [
            "achievement",
            "animctrl",
            "game",
            "physics",
            "scene",
        ],
        "required_globals": [
            {
                "name": "gDeathCount",
                "rva": "0x5381D4",
            },
            {
                "name": "totalframes",
                "rva": "0x547B48",
            },
        ],
        "reviewed_symbols": [
            "brainutl_DeltaTime",
            "brainutl_ElapsedTime",
            "brainutl_FindLSB_LV",
            "brainutl_HeldPad",
            "brainutl_gGetNearestTarget",
            "brainutl_Land",
            "brainutl_MultiPad",
            "brainutl_PlayMotionSound",
            "brainutil_ReverseCheck",
            "brainutil_limitRange",
        ],
        "reason": (
            "Exact elapsed-time and bit predicates, pad-to-motion text "
            "construction, nearest-target selection, motion sound, landing "
            "motion, fall penalty, achievement, and enemy-owner side effects."
        ),
    },
    {
        "module": "physics",
        "status": "partial",
        "risk": "high",
        "depends_on": [
            "player",
            "scene",
            "anim",
            "animutil",
            "camera",
            "collision",
            "fmath",
            "game",
            "intersec",
            "vectors",
            "jonny",
            "cube",
            "sprite",
            "sound",
            "vehicle",
            "wRender",
        ],
        "required_globals": [
            {
                "name": "maPhysicsData",
                "rva": "0x951BA0",
            },
            {
                "name": "numsolids",
                "rva": "0x951B90",
            },
            {
                "name": "maRange",
                "rva": "0x9543E0",
            },
            {
                "name": "maDesert_BNodeSizes",
                "rva": "0x4CC0D0",
            },
            {
                "name": "maWormNodeSizes",
                "rva": "0x4CC0E0",
            },
            {
                "name": "gSCENE_READY",
                "rva": "0x946CA0",
            },
            {
                "name": "collisionfrustrum",
                "rva": "0x946000",
            },
            {
                "name": "clippingfrustrum",
                "rva": "0x946060",
            },
            {
                "name": "initialLevelPauseDelay",
                "rva": "0x547B3C",
            },
            {
                "name": "streetsending",
                "module": "physics",
                "rva": "0x53A53C",
            },
            {
                "name": "eventarray",
                "rva": "0x4CB490",
            },
            {
                "name": "maphitsounds",
                "rva": "0x4CB820",
            },
            {
                "name": "splasheffects",
                "rva": "0x4CB8B0",
            },
            {
                "name": "LevelSelect",
                "rva": "0x537DEA",
            },
            {
                "name": "paEffects",
                "rva": "0x10DE900",
            },
            {
                "name": "stapbikeindex",
                "rva": "0x582420",
            },
            {
                "name": "stapsound",
                "rva": "0x5381EC",
            },
            {
                "name": "totalframes",
                "rva": "0x547B48",
            },
        ],
        "reviewed_symbols": [
            "BuildNodeVertexList",
            "BuildSolids",
            "CalcNewBox",
            "CalcRelativePosFromWorld",
            "CalcSolidRelativePos",
            "CalcWorldPosFromRelative",
            "CalcWorldRelativePos",
            "CharBlocking",
            "CheckCubeBlocking",
            "UpdatePublicVars",
            "UpdateSceneObject",
            "LaunchMapAnimEffects",
            "MovePlayer",
            "ProcessPhysicsObjects",
            "WorldBlocking",
            "buildfrustrum",
            "buildplane",
            "checkdriving",
            "generalCollide",
            "newclosestPoly",
            "physics_ResetJedi",
            "physics_gClrConstantVector",
            "physics_gFaceTarget",
            "physics_gForceFaceTarget",
            "physics_gGetConstantVector",
            "physics_gGetFaceTargetDelta",
            "physics_gGetFacing",
            "physics_gGetRange",
            "physics_gGetNewObject",
            "physics_gGetPosition",
            "physics_gInitObjects",
            "physics_gModFacing",
            "physics_gSetCharge",
            "physics_gSetConstantVector",
            "physics_gSetFacing",
            "physics_gSetRecoil",
            "physics_gSnapShotPosition",
            "physics_gSwapVel",
            "physics_gTurnToAttack",
            "physics_gTurnToFace",
            "planecheck",
            "polycollidecheck",
            "sphereAndPoly",
        ],
        "reason": (
            "Exact actor physics state, recursive standee scheduling, "
            "all six CalcMovement modes, slope/conveyor movement, collision "
            "contact, moving-solid geometry construction and ownership, "
            "complete MovePlayer scheduling, scene publication, driver "
            "synchronization, the complete frame scheduler and both the "
            "street-ending collision trigger and terminal sequence, the full "
            "Jedi physics reset including its level-ten vehicle exception, "
            "landing, splash, and map-animation dispatch."
        ),
    },
    {
        "module": "braindmg",
        "status": "reviewed",
        "risk": "high",
        "depends_on": [
            "achievement",
            "ai",
            "animctrl",
            "animutil",
            "brain",
            "brainutl",
            "collisn",
            "combo",
            "enemy",
            "extracharacters",
            "game",
            "input",
            "jedi",
            "physics",
            "sound",
            "sprite",
            "world",
        ],
        "required_globals": [
            {"name": "mDamageTotal", "rva": "0x4F13B0"},
            {"name": "mCurrentSFXP", "rva": "0x4F13B4"},
            {"name": "mProjectileAttack", "rva": "0x4F13B8"},
            {"name": "mProjMotion", "rva": "0x4F13C0"},
            {"name": "damageTracking", "rva": "0x4F1430"},
            {"name": "mpMotion", "rva": "0x4F1520"},
            {"name": "knob", "rva": "0x4F1528"},
            {"name": "zeroBSSCheck", "rva": "0x4F152C"},
            {"name": "gaPoints", "rva": "0x4BD900"},
            {"name": "gpWorld", "rva": "0x4AFDA0"},
            {"name": "leveldata", "rva": "0x537C88"},
            {"name": "LevelSelect", "rva": "0x537DEA"},
            {"name": "projType", "rva": "0x10DEBA0"},
        ],
        "reviewed_symbols": [
            "braindmg_AirHitReaction",
            "braindmg_BlockEffects",
            "braindmg_Blocking",
            "braindmg_DamageControl",
            "braindmg_DamageEffects",
            "braindmg_DamageTracker",
            "braindmg_DeathReaction",
            "braindmg_FindHitReaction",
            "braindmg_HitReaction",
            "braindmg_LevelDamage",
            "braindmg_LogHits",
            "braindmg_ResetDamageTracker",
        ],
        "reason": (
            "All twelve exact damage procedures cover projectile conversion, "
            "blocking, difficulty scaling, scoring, hit/death reactions, "
            "achievements, feedback, and hazardous-surface damage."
        ),
    },
    {
        "module": "brain",
        "status": "reviewed",
        "risk": "high",
        "depends_on": [
            "animctrl",
            "braindmg",
            "brainutl",
            "camera",
            "collisn",
            "combo",
            "enemy",
            "extracharacters",
            "fmath",
            "force",
            "game",
            "input",
            "objroot",
            "physics",
            "player",
            "sound",
            "sprite",
        ],
        "required_globals": [
            {
                "name": "OMNIDIRECTIONAL_MOVEMENT",
                "rva": "0x4A65A4",
            },
            {"name": "gaButtonMap", "rva": "0x4BAA00"},
            {"name": "player1InputType", "rva": "0x4D49A4"},
            {
                "name": "wait",
                "module": "brain",
                "rva": "0x4F13A0",
            },
            {"name": "mCameraAngleDest", "rva": "0x4F1AA4"},
            {"name": "leveldata", "rva": "0x537C88"},
            {"name": "LevelSelect", "rva": "0x537DEA"},
            {"name": "gGlobalTimer", "rva": "0x581FC0"},
            {"name": "g_p2Y", "rva": "0x932C80"},
            {"name": "g_p2X", "rva": "0x932C84"},
            {"name": "g_p1Y", "rva": "0x932C88"},
            {"name": "g_p1X", "rva": "0x932C8C"},
            {"name": "OptionStruct", "rva": "0x10DA100"},
            {"name": "GameStruct", "rva": "0x10DA140"},
            {"name": "paEffects", "rva": "0x10DE900"},
        ],
        "reviewed_symbols": [
            "brain_CheckForEffects",
            "brain_ControlPlayer",
            "brain_DoRingOffEffect",
            "brain_DoRingOnEffect",
            "brain_GroundControl",
            "brain_HangCallback",
            "brain_LockOn",
            "brain_SetFallTrajectory",
            "brain_SetJumpTrajectory",
            "brain_SetTrajectory",
            "brain_SkidCallBack",
            "brain_SwapVelDirCallBack",
            "brain_TakeOff",
            "brain_ThrowEnder",
            "brain_ValidateLockOn",
        ],
        "reason": (
            "All fifteen exact procedures cover the complete player input "
            "and callback controller, motion-event sound, lock-on/ring "
            "lifecycle, knockdown, recovery, player/NPC death, animation-end "
            "callbacks, and jump/fall trajectory setup."
        ),
    },
]


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def build_inventory(
    functions: list[dict[str, Any]],
    globals_: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    result = []
    globals_by_name: dict[str, list[dict[str, Any]]] = {}
    for symbol in globals_:
        globals_by_name.setdefault(symbol["name"], []).append(symbol)

    for priority, foundation in enumerate(FOUNDATIONS, start=1):
        module_functions = [
            function
            for function in functions
            if function["module"] == foundation["module"]
        ]
        if foundation["status"] == "reviewed":
            reviewed_functions = module_functions
        else:
            reviewed_names = set(foundation.get("reviewed_symbols", []))
            reviewed_functions = [
                function
                for function in module_functions
                if function["name"] in reviewed_names
            ]
        required = foundation["required_globals"]
        matched_globals = []
        for requirement in required:
            if isinstance(requirement, str):
                constraints = {"name": requirement}
            else:
                constraints = requirement
            name = constraints["name"]
            matches = globals_by_name.get(name, [])
            for key in ("module", "rva"):
                if key in constraints:
                    matches = [
                        symbol
                        for symbol in matches
                        if symbol.get(key) == constraints[key]
                    ]
            matched_globals.append(
                {
                    **constraints,
                    "matches": matches,
                }
            )
        result.append(
            {
                **foundation,
                "priority": priority,
                "procedure_count": len(module_functions),
                "procedure_bytes": sum(
                    int(function["size"]) for function in module_functions
                ),
                "reviewed_procedure_count": len(reviewed_functions),
                "reviewed_procedure_bytes": sum(
                    int(function["size"]) for function in reviewed_functions
                ),
                "named_parameters_and_locals": sum(
                    len(function["locals"]) for function in module_functions
                ),
                "procedures_with_direct_lines": sum(
                    bool(function["line_ranges"])
                    for function in module_functions
                ),
                "symbols": [function["name"] for function in module_functions],
                "global_evidence": matched_globals,
            }
        )
    return result


def write_report(path: Path, items: list[dict[str, Any]]) -> None:
    lines = [
        "# Portable foundation queue",
        "",
        "| # | Module | Status | Risk | Reviewed | Bytes reviewed | Direct lines |",
        "|---:|---|---|---|---:|---:|---:|",
    ]
    for item in items:
        lines.append(
            f"| {item['priority']} | `{item['module']}` | "
            f"{item['status']} | {item['risk']} | "
            f"{item['reviewed_procedure_count']}/{item['procedure_count']} | "
            f"{item['reviewed_procedure_bytes']:,}/"
            f"{item['procedure_bytes']:,} | "
            f"{item['procedures_with_direct_lines']} |"
        )
    lines.extend(
        [
            "",
            "The order is a reconstruction dependency order, not an assertion",
            "about the original link order. Status changes are versioned in",
            "`tools/foundation_inventory.py` and regenerated from the symbol",
            "inventory.",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--functions", type=Path, default=Path("inventory/functions.json")
    )
    parser.add_argument(
        "--globals", type=Path, default=Path("inventory/globals.json")
    )
    parser.add_argument(
        "--output", type=Path, default=Path("inventory/foundations.json")
    )
    parser.add_argument(
        "--report", type=Path, default=Path("inventory/FOUNDATIONS.md")
    )
    args = parser.parse_args()

    items = build_inventory(
        load_json(args.functions),
        load_json(args.globals),
    )
    args.output.write_text(
        json.dumps(
            {
                "schema_version": 1,
                "generated_from": [
                    str(args.functions),
                    str(args.globals),
                ],
                "foundations": items,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    write_report(args.report, items)
    print(
        f"Wrote {len(items)} foundation modules to {args.output} "
        f"and {args.report}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
