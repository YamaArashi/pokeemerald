#include "global.h"
#include "malloc.h"
#include "battle_pyramid.h"
#include "berry.h"
#include "decoration.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "faraway_island.h"
#include "field_camera.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "mauville_old_man.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "random.h"
#include "sprite.h"
#include "task.h"
#include "trainer_see.h"
#include "trainer_hill.h"
#include "util.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/field_effects.h"
#include "constants/items.h"
#include "constants/mauville_old_man.h"
#include "constants/trainer_types.h"
#include "constants/union_room.h"

// this file was known as evobjmv.c in Game Freak's original source

enum {
    MOVE_SPEED_NORMAL, // walking
    MOVE_SPEED_FAST_1, // running / surfing / sliding (ice tile)
    MOVE_SPEED_FAST_2, // water current / acro bike
    MOVE_SPEED_FASTER, // mach bike's max speed
    MOVE_SPEED_FASTEST,
};

enum {
    JUMP_DISTANCE_IN_PLACE,
    JUMP_DISTANCE_NORMAL,
    JUMP_DISTANCE_FAR,
};

// Sprite data used throughout
#define sObjEventId   data[0]
#define sTypeFuncId   data[1] // Index into corresponding gMovementTypeFuncs_* table
#define sActionFuncId data[2] // Index into corresponding gMovementActionFuncs_* table
#define sDirection    data[3]


#define movement_type_def(setup, table) \
static u8 setup##_callback(struct ObjectEvent *, struct Sprite *);\
void setup(struct Sprite *sprite)\
{\
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->sObjEventId], sprite, setup##_callback);\
}\
static u8 setup##_callback(struct ObjectEvent *objectEvent, struct Sprite *sprite)\
{\
    return table[sprite->sTypeFuncId](objectEvent, sprite);\
}

#define movement_type_empty_callback(setup) \
static u8 setup##_callback(struct ObjectEvent *, struct Sprite *);\
void setup(struct Sprite *sprite)\
{\
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->sObjEventId], sprite, setup##_callback);\
}\
static u8 setup##_callback(struct ObjectEvent *objectEvent, struct Sprite *sprite)\
{\
    return 0;\
}

static EWRAM_DATA u8 sCurrentReflectionType = 0;
static EWRAM_DATA u16 sCurrentSpecialObjectPaletteTag = 0;
static EWRAM_DATA struct LockedAnimObjectEvents *sLockedAnimObjectEvents = {0};

static void MoveCoordsInDirection(u32, s16 *, s16 *, s16, s16);
static bool8 ObjectEventExecSingleMovementAction(struct ObjectEvent *, struct Sprite *);
static void SetMovementDelay(struct Sprite *, s16);
static bool8 WaitForMovementDelay(struct Sprite *);
static u8 GetCollisionInDirection(struct ObjectEvent *, u8);
static u32 GetCopyDirection(u8, u32, u32);
static void TryEnableObjectEventAnim(struct ObjectEvent *, struct Sprite *);
static void ObjectEventExecHeldMovementAction(struct ObjectEvent *, struct Sprite *);
static void UpdateObjectEventSpriteAnimPause(struct ObjectEvent *, struct Sprite *);
static bool8 IsCoordOutsideObjectEventMovementRange(struct ObjectEvent *, s16, s16);
static bool8 IsMetatileDirectionallyImpassable(struct ObjectEvent *, s16, s16, u8);
static bool8 DoesObjectCollideWithObjectAt(struct ObjectEvent *, s16, s16);
static void UpdateObjectEventOffscreen(struct ObjectEvent *, struct Sprite *);
static void UpdateObjectEventSpriteVisibility(struct ObjectEvent *, struct Sprite *);
static void ObjectEventUpdateMetatileBehaviors(struct ObjectEvent *);
static void GetGroundEffectFlags_Reflection(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_TallGrassOnSpawn(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_LongGrassOnSpawn(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_SandHeap(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_ShallowFlowingWater(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_ShortGrass(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_HotSprings(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_TallGrassOnBeginStep(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_LongGrassOnBeginStep(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_Tracks(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_Puddle(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_Ripple(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_Seaweed(struct ObjectEvent *, u32 *);
static void GetGroundEffectFlags_JumpLanding(struct ObjectEvent *, u32 *);
static u8 ObjectEventGetNearbyReflectionType(struct ObjectEvent *);
static u8 GetReflectionTypeByMetatileBehavior(u32);
static void InitObjectPriorityByElevation(struct Sprite *, u8);
static void ObjectEventUpdateSubpriority(struct ObjectEvent *, struct Sprite *);
static void DoTracksGroundEffect_None(struct ObjectEvent *, struct Sprite *, u8);
static void DoTracksGroundEffect_Footprints(struct ObjectEvent *, struct Sprite *, u8);
static void DoTracksGroundEffect_BikeTireTracks(struct ObjectEvent *, struct Sprite *, u8);
static void DoRippleFieldEffect(struct ObjectEvent *, struct Sprite *);
static void DoGroundEffects_OnSpawn(struct ObjectEvent *, struct Sprite *);
static void DoGroundEffects_OnBeginStep(struct ObjectEvent *, struct Sprite *);
static void DoGroundEffects_OnFinishStep(struct ObjectEvent *, struct Sprite *);
static void VirtualObject_UpdateAnim(struct Sprite *);
static void ApplyLevitateMovement(u8);
static bool8 MovementType_Disguise_Callback(struct ObjectEvent *, struct Sprite *);
static bool8 MovementType_Buried_Callback(struct ObjectEvent *, struct Sprite *);
static void CreateReflectionEffectSprites(void);
static u8 GetObjectEventIdByLocalId(u8);
static u8 GetObjectEventIdByLocalIdAndMapInternal(u8, u8, u8);
static bool8 GetAvailableObjectEventId(u16, u8, u8, u8 *);
static void SetObjectEventDynamicGraphicsId(struct ObjectEvent *);
static void RemoveObjectEventInternal(struct ObjectEvent *);
static u16 GetObjectEventFlagIdByObjectEventId(u8);
static void UpdateObjectEventVisibility(struct ObjectEvent *, struct Sprite *);
static void MakeSpriteTemplateFromObjectEventTemplate(const struct ObjectEventTemplate *, struct SpriteTemplate *, const struct SubspriteTable **);
static void GetObjectEventMovingCameraOffset(s16 *, s16 *);
static const struct ObjectEventTemplate *GetObjectEventTemplateByLocalIdAndMap(u8, u8, u8);
static void LoadObjectEventPalette(u16);
static void RemoveObjectEventIfOutsideView(struct ObjectEvent *);
static void SpawnObjectEventOnReturnToField(u8, s16, s16);
static void SetPlayerAvatarObjectEventIdAndObjectId(u8, u8);
static void ResetObjectEventFldEffData(struct ObjectEvent *);
static u8 LoadSpritePaletteIfTagExists(const struct SpritePalette *);
static u8 FindObjectEventPaletteIndexByTag(u16);
static void _PatchObjectPalette(u16, u8);
static bool8 ObjectEventDoesElevationMatch(struct ObjectEvent *, u8);
static void SpriteCB_CameraObject(struct Sprite *);
static void CameraObject_Init(struct Sprite *);
static void CameraObject_UpdateMove(struct Sprite *);
static void CameraObject_UpdateFrozen(struct Sprite *);
static const struct ObjectEventTemplate *FindObjectEventTemplateByLocalId(u8, const struct ObjectEventTemplate *, u8);
static void ClearObjectEventMovement(struct ObjectEvent *, struct Sprite *);
static void ObjectEventSetSingleMovement(struct ObjectEvent *, struct Sprite *, u8);
static void SetSpriteDataForNormalStep(struct Sprite *, u8, u8);
static void InitSpriteForFigure8Anim(struct Sprite *);
static bool8 AnimateSpriteInFigure8(struct Sprite *);
static void SpriteCB_VirtualObject(struct Sprite *);
static void DoShadowFieldEffect(struct ObjectEvent *);
static void SetJumpSpriteData(struct Sprite *, u8, u8, u8);
static void SetWalkSlowSpriteData(struct Sprite *, u8);
static bool8 UpdateWalkSlowAnim(struct Sprite *);
static u8 DoJumpSpriteMovement(struct Sprite *);
static u8 DoJumpSpecialSpriteMovement(struct Sprite *);
static void CreateLevitateMovementTask(struct ObjectEvent *);
static void DestroyLevitateMovementTask(u8);
static bool8 NpcTakeStep(struct Sprite *);
static bool8 IsElevationMismatchAt(u8, s16, s16);
static bool8 AreElevationsCompatible(u8, u8);

static const struct SpriteFrameImage sPicTable_PechaBerryTree[];

const u8 gReflectionEffectPaletteMap[16] = {
        [PALSLOT_PLAYER]                 = PALSLOT_PLAYER_REFLECTION,
        [PALSLOT_PLAYER_REFLECTION]      = PALSLOT_PLAYER_REFLECTION,
        [PALSLOT_NPC_1]                  = PALSLOT_NPC_1_REFLECTION,
        [PALSLOT_NPC_2]                  = PALSLOT_NPC_2_REFLECTION,
        [PALSLOT_NPC_3]                  = PALSLOT_NPC_3_REFLECTION,
        [PALSLOT_NPC_4]                  = PALSLOT_NPC_4_REFLECTION,
        [PALSLOT_NPC_1_REFLECTION]       = PALSLOT_NPC_1_REFLECTION,
        [PALSLOT_NPC_2_REFLECTION]       = PALSLOT_NPC_2_REFLECTION,
        [PALSLOT_NPC_3_REFLECTION]       = PALSLOT_NPC_3_REFLECTION,
        [PALSLOT_NPC_4_REFLECTION]       = PALSLOT_NPC_4_REFLECTION,
        [PALSLOT_NPC_SPECIAL]            = PALSLOT_NPC_SPECIAL_REFLECTION,
        [PALSLOT_NPC_SPECIAL_REFLECTION] = PALSLOT_NPC_SPECIAL_REFLECTION
};

static const struct SpriteTemplate sCameraSpriteTemplate = {
    .tileTag = 0,
    .paletteTag = TAG_NONE,
    .oam = &gDummyOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_CameraObject
};

enum {
    CAMERA_STATE_INIT,
    CAMERA_STATE_MOVE,
    CAMERA_STATE_FROZEN,
};

static void (*const sCameraObjectFuncs[])(struct Sprite *) = {
    [CAMERA_STATE_INIT]   = CameraObject_Init,
    [CAMERA_STATE_MOVE]   = CameraObject_UpdateMove,
    [CAMERA_STATE_FROZEN] = CameraObject_UpdateFrozen,
};

#include "data/object_events/object_event_graphics.h"

// movement type callbacks
static void (*const sMovementTypeCallbacks[])(struct Sprite *) =
{
    [MOVEMENT_TYPE_NONE] = MovementType_None,
    [MOVEMENT_TYPE_LOOK_AROUND] = MovementType_LookAround,
    [MOVEMENT_TYPE_WANDER_AROUND] = MovementType_WanderAround,
    [MOVEMENT_TYPE_WANDER_UP_AND_DOWN] = MovementType_WanderUpAndDown,
    [MOVEMENT_TYPE_WANDER_DOWN_AND_UP] = MovementType_WanderUpAndDown,
    [MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT] = MovementType_WanderLeftAndRight,
    [MOVEMENT_TYPE_WANDER_RIGHT_AND_LEFT] = MovementType_WanderLeftAndRight,
    [MOVEMENT_TYPE_FACE_UP] = MovementType_FaceDirection,
    [MOVEMENT_TYPE_FACE_DOWN] = MovementType_FaceDirection,
    [MOVEMENT_TYPE_FACE_LEFT] = MovementType_FaceDirection,
    [MOVEMENT_TYPE_FACE_RIGHT] = MovementType_FaceDirection,
    [MOVEMENT_TYPE_PLAYER] = MovementType_Player,
    [MOVEMENT_TYPE_BERRY_TREE_GROWTH] = MovementType_BerryTreeGrowth,
    [MOVEMENT_TYPE_FACE_DOWN_AND_UP] = MovementType_FaceDownAndUp,
    [MOVEMENT_TYPE_FACE_LEFT_AND_RIGHT] = MovementType_FaceLeftAndRight,
    [MOVEMENT_TYPE_FACE_UP_AND_LEFT] = MovementType_FaceUpAndLeft,
    [MOVEMENT_TYPE_FACE_UP_AND_RIGHT] = MovementType_FaceUpAndRight,
    [MOVEMENT_TYPE_FACE_DOWN_AND_LEFT] = MovementType_FaceDownAndLeft,
    [MOVEMENT_TYPE_FACE_DOWN_AND_RIGHT] = MovementType_FaceDownAndRight,
    [MOVEMENT_TYPE_FACE_DOWN_UP_AND_LEFT] = MovementType_FaceDownUpAndLeft,
    [MOVEMENT_TYPE_FACE_DOWN_UP_AND_RIGHT] = MovementType_FaceDownUpAndRight,
    [MOVEMENT_TYPE_FACE_UP_LEFT_AND_RIGHT] = MovementType_FaceUpRightAndLeft,
    [MOVEMENT_TYPE_FACE_DOWN_LEFT_AND_RIGHT] = MovementType_FaceDownRightAndLeft,
    [MOVEMENT_TYPE_ROTATE_COUNTERCLOCKWISE] = MovementType_RotateCounterclockwise,
    [MOVEMENT_TYPE_ROTATE_CLOCKWISE] = MovementType_RotateClockwise,
    [MOVEMENT_TYPE_WALK_UP_AND_DOWN] = MovementType_WalkBackAndForth,
    [MOVEMENT_TYPE_WALK_DOWN_AND_UP] = MovementType_WalkBackAndForth,
    [MOVEMENT_TYPE_WALK_LEFT_AND_RIGHT] = MovementType_WalkBackAndForth,
    [MOVEMENT_TYPE_WALK_RIGHT_AND_LEFT] = MovementType_WalkBackAndForth,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_LEFT_DOWN] = MovementType_WalkSequenceUpRightLeftDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_DOWN_UP] = MovementType_WalkSequenceRightLeftDownUp,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_RIGHT_LEFT] = MovementType_WalkSequenceDownUpRightLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_UP_RIGHT] = MovementType_WalkSequenceLeftDownUpRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_RIGHT_DOWN] = MovementType_WalkSequenceUpLeftRightDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_DOWN_UP] = MovementType_WalkSequenceLeftRightDownUp,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_LEFT_RIGHT] = MovementType_WalkSequenceDownUpLeftRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_UP_LEFT] = MovementType_WalkSequenceRightDownUpLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_DOWN_RIGHT] = MovementType_WalkSequenceLeftUpDownRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_RIGHT_LEFT] = MovementType_WalkSequenceUpDownRightLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_UP_DOWN] = MovementType_WalkSequenceRightLeftUpDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_LEFT_UP] = MovementType_WalkSequenceDownRightLeftUp,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_DOWN_LEFT] = MovementType_WalkSequenceRightUpDownLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_LEFT_RIGHT] = MovementType_WalkSequenceUpDownLeftRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_UP_DOWN] = MovementType_WalkSequenceLeftRightUpDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_RIGHT_UP] = MovementType_WalkSequenceDownLeftRightUp,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_DOWN_RIGHT] = MovementType_WalkSequenceUpLeftDownRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_UP_LEFT] = MovementType_WalkSequenceDownRightUpLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_RIGHT_UP] = MovementType_WalkSequenceLeftDownRightUp,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_LEFT_DOWN] = MovementType_WalkSequenceRightUpLeftDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_DOWN_LEFT] = MovementType_WalkSequenceUpRightDownLeft,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_UP_RIGHT] = MovementType_WalkSequenceDownLeftUpRight,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_RIGHT_DOWN] = MovementType_WalkSequenceLeftUpRightDown,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_LEFT_UP] = MovementType_WalkSequenceRightDownLeftUp,
    [MOVEMENT_TYPE_COPY_PLAYER] = MovementType_CopyPlayer,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE] = MovementType_CopyPlayer,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE] = MovementType_CopyPlayer,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE] = MovementType_CopyPlayer,
    [MOVEMENT_TYPE_TREE_DISGUISE] = MovementType_TreeDisguise,
    [MOVEMENT_TYPE_MOUNTAIN_DISGUISE] = MovementType_MountainDisguise,
    [MOVEMENT_TYPE_COPY_PLAYER_IN_GRASS] = MovementType_CopyPlayerInGrass,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE_IN_GRASS] = MovementType_CopyPlayerInGrass,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE_IN_GRASS] = MovementType_CopyPlayerInGrass,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE_IN_GRASS] = MovementType_CopyPlayerInGrass,
    [MOVEMENT_TYPE_BURIED] = MovementType_Buried,
    [MOVEMENT_TYPE_WALK_IN_PLACE_DOWN] = MovementType_WalkInPlace,
    [MOVEMENT_TYPE_WALK_IN_PLACE_UP] = MovementType_WalkInPlace,
    [MOVEMENT_TYPE_WALK_IN_PLACE_LEFT] = MovementType_WalkInPlace,
    [MOVEMENT_TYPE_WALK_IN_PLACE_RIGHT] = MovementType_WalkInPlace,
    [MOVEMENT_TYPE_JOG_IN_PLACE_DOWN] = MovementType_JogInPlace,
    [MOVEMENT_TYPE_JOG_IN_PLACE_UP] = MovementType_JogInPlace,
    [MOVEMENT_TYPE_JOG_IN_PLACE_LEFT] = MovementType_JogInPlace,
    [MOVEMENT_TYPE_JOG_IN_PLACE_RIGHT] = MovementType_JogInPlace,
    [MOVEMENT_TYPE_RUN_IN_PLACE_DOWN] = MovementType_RunInPlace,
    [MOVEMENT_TYPE_RUN_IN_PLACE_UP] = MovementType_RunInPlace,
    [MOVEMENT_TYPE_RUN_IN_PLACE_LEFT] = MovementType_RunInPlace,
    [MOVEMENT_TYPE_RUN_IN_PLACE_RIGHT] = MovementType_RunInPlace,
    [MOVEMENT_TYPE_INVISIBLE] = MovementType_Invisible,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_DOWN] = MovementType_WalkSlowlyInPlace,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_UP] = MovementType_WalkSlowlyInPlace,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_LEFT] = MovementType_WalkSlowlyInPlace,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_RIGHT] = MovementType_WalkSlowlyInPlace,
};

static const bool8 sMovementTypeHasRange[NUM_MOVEMENT_TYPES] = {
    [MOVEMENT_TYPE_WANDER_AROUND] = TRUE,
    [MOVEMENT_TYPE_WANDER_UP_AND_DOWN] = TRUE,
    [MOVEMENT_TYPE_WANDER_DOWN_AND_UP] = TRUE,
    [MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WANDER_RIGHT_AND_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_UP_AND_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_DOWN_AND_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_LEFT_AND_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_RIGHT_AND_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_LEFT_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_DOWN_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_RIGHT_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_UP_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_RIGHT_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_DOWN_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_LEFT_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_UP_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_DOWN_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_RIGHT_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_UP_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_LEFT_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_DOWN_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_LEFT_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_UP_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_RIGHT_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_DOWN_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_UP_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_RIGHT_UP] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_LEFT_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_DOWN_LEFT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_UP_RIGHT] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_RIGHT_DOWN] = TRUE,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_LEFT_UP] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_IN_GRASS] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE_IN_GRASS] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE_IN_GRASS] = TRUE,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE_IN_GRASS] = TRUE,
};

const u8 gInitialMovementTypeFacingDirections[] = {
    [MOVEMENT_TYPE_NONE] = DIR_SOUTH,
    [MOVEMENT_TYPE_LOOK_AROUND] = DIR_SOUTH,
    [MOVEMENT_TYPE_WANDER_AROUND] = DIR_SOUTH,
    [MOVEMENT_TYPE_WANDER_UP_AND_DOWN] = DIR_NORTH,
    [MOVEMENT_TYPE_WANDER_DOWN_AND_UP] = DIR_SOUTH,
    [MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT] = DIR_WEST,
    [MOVEMENT_TYPE_WANDER_RIGHT_AND_LEFT] = DIR_EAST,
    [MOVEMENT_TYPE_FACE_UP] = DIR_NORTH,
    [MOVEMENT_TYPE_FACE_DOWN] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_LEFT] = DIR_WEST,
    [MOVEMENT_TYPE_FACE_RIGHT] = DIR_EAST,
    [MOVEMENT_TYPE_PLAYER] = DIR_SOUTH,
    [MOVEMENT_TYPE_BERRY_TREE_GROWTH] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_DOWN_AND_UP] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_LEFT_AND_RIGHT] = DIR_WEST,
    [MOVEMENT_TYPE_FACE_UP_AND_LEFT] = DIR_NORTH,
    [MOVEMENT_TYPE_FACE_UP_AND_RIGHT] = DIR_NORTH,
    [MOVEMENT_TYPE_FACE_DOWN_AND_LEFT] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_DOWN_AND_RIGHT] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_DOWN_UP_AND_LEFT] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_DOWN_UP_AND_RIGHT] = DIR_SOUTH,
    [MOVEMENT_TYPE_FACE_UP_LEFT_AND_RIGHT] = DIR_NORTH,
    [MOVEMENT_TYPE_FACE_DOWN_LEFT_AND_RIGHT] = DIR_SOUTH,
    [MOVEMENT_TYPE_ROTATE_COUNTERCLOCKWISE] = DIR_SOUTH,
    [MOVEMENT_TYPE_ROTATE_CLOCKWISE] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_UP_AND_DOWN] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_DOWN_AND_UP] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_LEFT_AND_RIGHT] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_RIGHT_AND_LEFT] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_LEFT_DOWN] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_DOWN_UP] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_RIGHT_LEFT] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_UP_RIGHT] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_RIGHT_DOWN] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_DOWN_UP] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_UP_LEFT_RIGHT] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_UP_LEFT] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_DOWN_RIGHT] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_RIGHT_LEFT] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_LEFT_UP_DOWN] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_LEFT_UP] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_DOWN_LEFT] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_DOWN_LEFT_RIGHT] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_RIGHT_UP_DOWN] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_RIGHT_UP] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_LEFT_DOWN_RIGHT] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_UP_LEFT] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_DOWN_RIGHT_UP] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_UP_LEFT_DOWN] = DIR_EAST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_UP_RIGHT_DOWN_LEFT] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_UP_RIGHT] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_RIGHT_DOWN] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_LEFT_UP] = DIR_EAST,
    [MOVEMENT_TYPE_COPY_PLAYER] = DIR_NORTH,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE] = DIR_SOUTH,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE] = DIR_WEST,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE] = DIR_EAST,
    [MOVEMENT_TYPE_TREE_DISGUISE] = DIR_SOUTH,
    [MOVEMENT_TYPE_MOUNTAIN_DISGUISE] = DIR_SOUTH,
    [MOVEMENT_TYPE_COPY_PLAYER_IN_GRASS] = DIR_NORTH,
    [MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE_IN_GRASS] = DIR_SOUTH,
    [MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE_IN_GRASS] = DIR_WEST,
    [MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE_IN_GRASS] = DIR_EAST,
    [MOVEMENT_TYPE_BURIED] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_IN_PLACE_DOWN] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_IN_PLACE_UP] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_IN_PLACE_LEFT] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_IN_PLACE_RIGHT] = DIR_EAST,
    [MOVEMENT_TYPE_JOG_IN_PLACE_DOWN] = DIR_SOUTH,
    [MOVEMENT_TYPE_JOG_IN_PLACE_UP] = DIR_NORTH,
    [MOVEMENT_TYPE_JOG_IN_PLACE_LEFT] = DIR_WEST,
    [MOVEMENT_TYPE_JOG_IN_PLACE_RIGHT] = DIR_EAST,
    [MOVEMENT_TYPE_RUN_IN_PLACE_DOWN] = DIR_SOUTH,
    [MOVEMENT_TYPE_RUN_IN_PLACE_UP] = DIR_NORTH,
    [MOVEMENT_TYPE_RUN_IN_PLACE_LEFT] = DIR_WEST,
    [MOVEMENT_TYPE_RUN_IN_PLACE_RIGHT] = DIR_EAST,
    [MOVEMENT_TYPE_INVISIBLE] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_DOWN] = DIR_SOUTH,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_UP] = DIR_NORTH,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_LEFT] = DIR_WEST,
    [MOVEMENT_TYPE_WALK_SLOWLY_IN_PLACE_RIGHT] = DIR_EAST,
};

#define OBJ_EVENT_PAL_TAG_BRENDAN                 0x1100
#define OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION      0x1101
#define OBJ_EVENT_PAL_TAG_BRIDGE_REFLECTION       0x1102
#define OBJ_EVENT_PAL_TAG_NPC_1                   0x1103
#define OBJ_EVENT_PAL_TAG_NPC_2                   0x1104
#define OBJ_EVENT_PAL_TAG_NPC_3                   0x1105
#define OBJ_EVENT_PAL_TAG_NPC_4                   0x1106
#define OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION        0x1107
#define OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION        0x1108
#define OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION        0x1109
#define OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION        0x110A
#define OBJ_EVENT_PAL_TAG_QUINTY_PLUMP            0x110B
#define OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION 0x110C
#define OBJ_EVENT_PAL_TAG_TRUCK                   0x110D
#define OBJ_EVENT_PAL_TAG_VIGOROTH                0x110E
#define OBJ_EVENT_PAL_TAG_ZIGZAGOON               0x110F
#define OBJ_EVENT_PAL_TAG_MAY                     0x1110
#define OBJ_EVENT_PAL_TAG_MAY_REFLECTION          0x1111
#define OBJ_EVENT_PAL_TAG_MOVING_BOX              0x1112
#define OBJ_EVENT_PAL_TAG_CABLE_CAR               0x1113
#define OBJ_EVENT_PAL_TAG_SSTIDAL                 0x1114
#define OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER       0x1115
#define OBJ_EVENT_PAL_TAG_KYOGRE                  0x1116
#define OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION       0x1117
#define OBJ_EVENT_PAL_TAG_GROUDON                 0x1118
#define OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION      0x1119
#define OBJ_EVENT_PAL_TAG_UNUSED                  0x111A
#define OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW        0x111B
#define OBJ_EVENT_PAL_TAG_POOCHYENA               0x111C
#define OBJ_EVENT_PAL_TAG_RED_LEAF                0x111D
#define OBJ_EVENT_PAL_TAG_DEOXYS                  0x111E
#define OBJ_EVENT_PAL_TAG_BIRTH_ISLAND_STONE      0x111F
#define OBJ_EVENT_PAL_TAG_HO_OH                   0x1120
#define OBJ_EVENT_PAL_TAG_LUGIA                   0x1121
#define OBJ_EVENT_PAL_TAG_RS_BRENDAN              0x1122
#define OBJ_EVENT_PAL_TAG_RS_MAY                  0x1123
#define OBJ_EVENT_PAL_TAG_NONE                    0x11FF

#include "data/object_events/object_event_graphics_info_pointers.h"
#include "data/field_effects/field_effect_object_template_pointers.h"
#include "data/object_events/object_event_pic_tables.h"
#include "data/object_events/object_event_anims.h"
#include "data/object_events/base_oam.h"
#include "data/object_events/object_event_subsprites.h"
#include "data/object_events/object_event_graphics_info.h"

static const struct SpritePalette sObjectEventSpritePalettes[] = {
    {gObjectEventPal_Npc1,                  OBJ_EVENT_PAL_TAG_NPC_1},
    {gObjectEventPal_Npc2,                  OBJ_EVENT_PAL_TAG_NPC_2},
    {gObjectEventPal_Npc3,                  OBJ_EVENT_PAL_TAG_NPC_3},
    {gObjectEventPal_Npc4,                  OBJ_EVENT_PAL_TAG_NPC_4},
    {gObjectEventPal_Npc1Reflection,        OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION},
    {gObjectEventPal_Npc2Reflection,        OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION},
    {gObjectEventPal_Npc3Reflection,        OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION},
    {gObjectEventPal_Npc4Reflection,        OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION},
    {gObjectEventPal_Brendan,               OBJ_EVENT_PAL_TAG_BRENDAN},
    {gObjectEventPal_BrendanReflection,     OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION},
    {gObjectEventPal_BridgeReflection,      OBJ_EVENT_PAL_TAG_BRIDGE_REFLECTION},
    {gObjectEventPal_PlayerUnderwater,      OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER},
    {gObjectEventPal_QuintyPlump,           OBJ_EVENT_PAL_TAG_QUINTY_PLUMP},
    {gObjectEventPal_QuintyPlumpReflection, OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION},
    {gObjectEventPal_Truck,                 OBJ_EVENT_PAL_TAG_TRUCK},
    {gObjectEventPal_Vigoroth,              OBJ_EVENT_PAL_TAG_VIGOROTH},
    {gObjectEventPal_EnemyZigzagoon,        OBJ_EVENT_PAL_TAG_ZIGZAGOON},
    {gObjectEventPal_May,                   OBJ_EVENT_PAL_TAG_MAY},
    {gObjectEventPal_MayReflection,         OBJ_EVENT_PAL_TAG_MAY_REFLECTION},
    {gObjectEventPal_MovingBox,             OBJ_EVENT_PAL_TAG_MOVING_BOX},
    {gObjectEventPal_CableCar,              OBJ_EVENT_PAL_TAG_CABLE_CAR},
    {gObjectEventPal_SSTidal,               OBJ_EVENT_PAL_TAG_SSTIDAL},
    {gObjectEventPal_Kyogre,                OBJ_EVENT_PAL_TAG_KYOGRE},
    {gObjectEventPal_KyogreReflection,      OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION},
    {gObjectEventPal_Groudon,               OBJ_EVENT_PAL_TAG_GROUDON},
    {gObjectEventPal_GroudonReflection,     OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION},
    {gObjectEventPal_SubmarineShadow,       OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW},
    {gObjectEventPal_Poochyena,             OBJ_EVENT_PAL_TAG_POOCHYENA},
    {gObjectEventPal_RedLeaf,               OBJ_EVENT_PAL_TAG_RED_LEAF},
    {gObjectEventPal_Deoxys,                OBJ_EVENT_PAL_TAG_DEOXYS},
    {gObjectEventPal_BirthIslandStone,      OBJ_EVENT_PAL_TAG_BIRTH_ISLAND_STONE},
    {gObjectEventPal_HoOh,                  OBJ_EVENT_PAL_TAG_HO_OH},
    {gObjectEventPal_Lugia,                 OBJ_EVENT_PAL_TAG_LUGIA},
    {gObjectEventPal_RubySapphireBrendan,   OBJ_EVENT_PAL_TAG_RS_BRENDAN},
    {gObjectEventPal_RubySapphireMay,       OBJ_EVENT_PAL_TAG_RS_MAY},
#ifdef BUGFIX
    {NULL,                                  OBJ_EVENT_PAL_TAG_NONE},
#else
    {}, // BUG: FindObjectEventPaletteIndexByTag looks for OBJ_EVENT_PAL_TAG_NONE and not 0x0.
        // If it's looking for a tag that isn't in this table, the game locks in an infinite loop.
#endif
};

static const u16 sReflectionPaletteTags_Brendan[] = {
    OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
};

static const u16 sReflectionPaletteTags_May[] = {
    OBJ_EVENT_PAL_TAG_MAY_REFLECTION,
    OBJ_EVENT_PAL_TAG_MAY_REFLECTION,
    OBJ_EVENT_PAL_TAG_MAY_REFLECTION,
    OBJ_EVENT_PAL_TAG_MAY_REFLECTION,
};

static const u16 sReflectionPaletteTags_PlayerUnderwater[] = {
    OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER,
    OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER,
    OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER,
    OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER,
};

static const struct PairedPalettes sPlayerReflectionPaletteSets[] = {
    {OBJ_EVENT_PAL_TAG_BRENDAN,           sReflectionPaletteTags_Brendan},
    {OBJ_EVENT_PAL_TAG_MAY,               sReflectionPaletteTags_May},
    {OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER, sReflectionPaletteTags_PlayerUnderwater},
    {OBJ_EVENT_PAL_TAG_NONE,              NULL},
};

static const u16 sReflectionPaletteTags_QuintyPlump[] = {
    OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION,
    OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION,
    OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION,
    OBJ_EVENT_PAL_TAG_QUINTY_PLUMP_REFLECTION,
};

static const u16 sReflectionPaletteTags_Truck[] = {
    OBJ_EVENT_PAL_TAG_TRUCK,
    OBJ_EVENT_PAL_TAG_TRUCK,
    OBJ_EVENT_PAL_TAG_TRUCK,
    OBJ_EVENT_PAL_TAG_TRUCK,
};

static const u16 sReflectionPaletteTags_VigorothMover[] = {
    OBJ_EVENT_PAL_TAG_VIGOROTH,
    OBJ_EVENT_PAL_TAG_VIGOROTH,
    OBJ_EVENT_PAL_TAG_VIGOROTH,
    OBJ_EVENT_PAL_TAG_VIGOROTH,
};

static const u16 sReflectionPaletteTags_MovingBox[] = {
    OBJ_EVENT_PAL_TAG_MOVING_BOX,
    OBJ_EVENT_PAL_TAG_MOVING_BOX,
    OBJ_EVENT_PAL_TAG_MOVING_BOX,
    OBJ_EVENT_PAL_TAG_MOVING_BOX,
};

static const u16 sReflectionPaletteTags_CableCar[] = {
    OBJ_EVENT_PAL_TAG_CABLE_CAR,
    OBJ_EVENT_PAL_TAG_CABLE_CAR,
    OBJ_EVENT_PAL_TAG_CABLE_CAR,
    OBJ_EVENT_PAL_TAG_CABLE_CAR,
};

static const u16 sReflectionPaletteTags_SSTidal[] = {
    OBJ_EVENT_PAL_TAG_SSTIDAL,
    OBJ_EVENT_PAL_TAG_SSTIDAL,
    OBJ_EVENT_PAL_TAG_SSTIDAL,
    OBJ_EVENT_PAL_TAG_SSTIDAL,
};

static const u16 sReflectionPaletteTags_SubmarineShadow[] = {
    OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW,
    OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW,
    OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW,
    OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW,
};

static const u16 sReflectionPaletteTags_Kyogre[] = {
    OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION,
    OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION,
    OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION,
    OBJ_EVENT_PAL_TAG_KYOGRE_REFLECTION,
};

static const u16 sReflectionPaletteTags_Groudon[] = {
    OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION,
    OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION,
    OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION,
    OBJ_EVENT_PAL_TAG_GROUDON_REFLECTION,
};

static const u16 sReflectionPaletteTags_Npc3[] = { // Only used by the Route 120 bridge Kecleon
    OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
};

static const u16 sReflectionPaletteTags_RedLeaf[] = {
    OBJ_EVENT_PAL_TAG_RED_LEAF,
    OBJ_EVENT_PAL_TAG_RED_LEAF,
    OBJ_EVENT_PAL_TAG_RED_LEAF,
    OBJ_EVENT_PAL_TAG_RED_LEAF,
};

static const struct PairedPalettes sSpecialObjectReflectionPaletteSets[] = {
    {OBJ_EVENT_PAL_TAG_BRENDAN,          sReflectionPaletteTags_Brendan},
    {OBJ_EVENT_PAL_TAG_MAY,              sReflectionPaletteTags_May},
    {OBJ_EVENT_PAL_TAG_QUINTY_PLUMP,     sReflectionPaletteTags_QuintyPlump},
    {OBJ_EVENT_PAL_TAG_TRUCK,            sReflectionPaletteTags_Truck},
    {OBJ_EVENT_PAL_TAG_VIGOROTH,         sReflectionPaletteTags_VigorothMover},
    {OBJ_EVENT_PAL_TAG_MOVING_BOX,       sReflectionPaletteTags_MovingBox},
    {OBJ_EVENT_PAL_TAG_CABLE_CAR,        sReflectionPaletteTags_CableCar},
    {OBJ_EVENT_PAL_TAG_SSTIDAL,          sReflectionPaletteTags_SSTidal},
    {OBJ_EVENT_PAL_TAG_KYOGRE,           sReflectionPaletteTags_Kyogre},
    {OBJ_EVENT_PAL_TAG_GROUDON,          sReflectionPaletteTags_Groudon},
    {OBJ_EVENT_PAL_TAG_NPC_3,            sReflectionPaletteTags_Npc3},
    {OBJ_EVENT_PAL_TAG_SUBMARINE_SHADOW, sReflectionPaletteTags_SubmarineShadow},
    {OBJ_EVENT_PAL_TAG_RED_LEAF,         sReflectionPaletteTags_RedLeaf},
    {OBJ_EVENT_PAL_TAG_NONE,             NULL},
};

static const u16 sObjectPaletteTags0[] = {
    [PALSLOT_PLAYER]            = OBJ_EVENT_PAL_TAG_BRENDAN,
    [PALSLOT_PLAYER_REFLECTION] = OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    [PALSLOT_NPC_1]             = OBJ_EVENT_PAL_TAG_NPC_1,
    [PALSLOT_NPC_2]             = OBJ_EVENT_PAL_TAG_NPC_2,
    [PALSLOT_NPC_3]             = OBJ_EVENT_PAL_TAG_NPC_3,
    [PALSLOT_NPC_4]             = OBJ_EVENT_PAL_TAG_NPC_4,
    [PALSLOT_NPC_1_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION,
    [PALSLOT_NPC_2_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION,
    [PALSLOT_NPC_3_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    [PALSLOT_NPC_4_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION,
};

static const u16 sObjectPaletteTags1[] = {
    [PALSLOT_PLAYER]            = OBJ_EVENT_PAL_TAG_BRENDAN,
    [PALSLOT_PLAYER_REFLECTION] = OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    [PALSLOT_NPC_1]             = OBJ_EVENT_PAL_TAG_NPC_1,
    [PALSLOT_NPC_2]             = OBJ_EVENT_PAL_TAG_NPC_2,
    [PALSLOT_NPC_3]             = OBJ_EVENT_PAL_TAG_NPC_3,
    [PALSLOT_NPC_4]             = OBJ_EVENT_PAL_TAG_NPC_4,
    [PALSLOT_NPC_1_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION,
    [PALSLOT_NPC_2_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION,
    [PALSLOT_NPC_3_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    [PALSLOT_NPC_4_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION,
};

static const u16 sObjectPaletteTags2[] = {
    [PALSLOT_PLAYER]            = OBJ_EVENT_PAL_TAG_BRENDAN,
    [PALSLOT_PLAYER_REFLECTION] = OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    [PALSLOT_NPC_1]             = OBJ_EVENT_PAL_TAG_NPC_1,
    [PALSLOT_NPC_2]             = OBJ_EVENT_PAL_TAG_NPC_2,
    [PALSLOT_NPC_3]             = OBJ_EVENT_PAL_TAG_NPC_3,
    [PALSLOT_NPC_4]             = OBJ_EVENT_PAL_TAG_NPC_4,
    [PALSLOT_NPC_1_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION,
    [PALSLOT_NPC_2_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION,
    [PALSLOT_NPC_3_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    [PALSLOT_NPC_4_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION,
};

static const u16 sObjectPaletteTags3[] = {
    [PALSLOT_PLAYER]            = OBJ_EVENT_PAL_TAG_BRENDAN,
    [PALSLOT_PLAYER_REFLECTION] = OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION,
    [PALSLOT_NPC_1]             = OBJ_EVENT_PAL_TAG_NPC_1,
    [PALSLOT_NPC_2]             = OBJ_EVENT_PAL_TAG_NPC_2,
    [PALSLOT_NPC_3]             = OBJ_EVENT_PAL_TAG_NPC_3,
    [PALSLOT_NPC_4]             = OBJ_EVENT_PAL_TAG_NPC_4,
    [PALSLOT_NPC_1_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_1_REFLECTION,
    [PALSLOT_NPC_2_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_2_REFLECTION,
    [PALSLOT_NPC_3_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_3_REFLECTION,
    [PALSLOT_NPC_4_REFLECTION]  = OBJ_EVENT_PAL_TAG_NPC_4_REFLECTION,
};

static const u16 *const sObjectPaletteTagSets[] = {
    sObjectPaletteTags0,
    sObjectPaletteTags1,
    sObjectPaletteTags2,
    sObjectPaletteTags3,
};

#include "data/object_events/berry_tree_graphics_tables.h"
#include "data/field_effects/field_effect_objects.h"

static const s16 sMovementDelaysMedium[] = {32, 64,  96, 128};
static const s16 sMovementDelaysLong[] =   {32, 64, 128, 192}; // Unused
static const s16 sMovementDelaysShort[] =  {32, 48,  64,  80};

#include "data/object_events/movement_type_func_tables.h"

static const u8 sFaceDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_STD_FACE_SOUTH,
    [DIR_SOUTH] = ANIM_STD_FACE_SOUTH,
    [DIR_NORTH] = ANIM_STD_FACE_NORTH,
    [DIR_WEST] = ANIM_STD_FACE_WEST,
    [DIR_EAST] = ANIM_STD_FACE_EAST,
    [DIR_SOUTHWEST] = ANIM_STD_FACE_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STD_FACE_SOUTH,
    [DIR_NORTHWEST] = ANIM_STD_FACE_NORTH,
    [DIR_NORTHEAST] = ANIM_STD_FACE_NORTH,
};
static const u8 sMoveDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_STD_GO_SOUTH,
    [DIR_SOUTH] = ANIM_STD_GO_SOUTH,
    [DIR_NORTH] = ANIM_STD_GO_NORTH,
    [DIR_WEST] = ANIM_STD_GO_WEST,
    [DIR_EAST] = ANIM_STD_GO_EAST,
    [DIR_SOUTHWEST] = ANIM_STD_GO_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STD_GO_SOUTH,
    [DIR_NORTHWEST] = ANIM_STD_GO_NORTH,
    [DIR_NORTHEAST] = ANIM_STD_GO_NORTH,
};
static const u8 sMoveDirectionFastAnimNums[] = {
    [DIR_NONE] = ANIM_STD_GO_FAST_SOUTH,
    [DIR_SOUTH] = ANIM_STD_GO_FAST_SOUTH,
    [DIR_NORTH] = ANIM_STD_GO_FAST_NORTH,
    [DIR_WEST] = ANIM_STD_GO_FAST_WEST,
    [DIR_EAST] = ANIM_STD_GO_FAST_EAST,
    [DIR_SOUTHWEST] = ANIM_STD_GO_FAST_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STD_GO_FAST_SOUTH,
    [DIR_NORTHWEST] = ANIM_STD_GO_FAST_NORTH,
    [DIR_NORTHEAST] = ANIM_STD_GO_FAST_NORTH,
};
static const u8 sMoveDirectionFasterAnimNums[] = {
    [DIR_NONE] = ANIM_STD_GO_FASTER_SOUTH,
    [DIR_SOUTH] = ANIM_STD_GO_FASTER_SOUTH,
    [DIR_NORTH] = ANIM_STD_GO_FASTER_NORTH,
    [DIR_WEST] = ANIM_STD_GO_FASTER_WEST,
    [DIR_EAST] = ANIM_STD_GO_FASTER_EAST,
    [DIR_SOUTHWEST] = ANIM_STD_GO_FASTER_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STD_GO_FASTER_SOUTH,
    [DIR_NORTHWEST] = ANIM_STD_GO_FASTER_NORTH,
    [DIR_NORTHEAST] = ANIM_STD_GO_FASTER_NORTH,
};
static const u8 sMoveDirectionFastestAnimNums[] = {
    [DIR_NONE] = ANIM_STD_GO_FASTEST_SOUTH,
    [DIR_SOUTH] = ANIM_STD_GO_FASTEST_SOUTH,
    [DIR_NORTH] = ANIM_STD_GO_FASTEST_NORTH,
    [DIR_WEST] = ANIM_STD_GO_FASTEST_WEST,
    [DIR_EAST] = ANIM_STD_GO_FASTEST_EAST,
    [DIR_SOUTHWEST] = ANIM_STD_GO_FASTEST_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STD_GO_FASTEST_SOUTH,
    [DIR_NORTHWEST] = ANIM_STD_GO_FASTEST_NORTH,
    [DIR_NORTHEAST] = ANIM_STD_GO_FASTEST_NORTH,
};
static const u8 sJumpSpecialDirectionAnimNums[] = { // used for jumping onto surf mon
    [DIR_NONE] = ANIM_GET_ON_OFF_POKEMON_SOUTH,
    [DIR_SOUTH] = ANIM_GET_ON_OFF_POKEMON_SOUTH,
    [DIR_NORTH] = ANIM_GET_ON_OFF_POKEMON_NORTH,
    [DIR_WEST] = ANIM_GET_ON_OFF_POKEMON_WEST,
    [DIR_EAST] = ANIM_GET_ON_OFF_POKEMON_EAST,
    [DIR_SOUTHWEST] = ANIM_GET_ON_OFF_POKEMON_SOUTH,
    [DIR_SOUTHEAST] = ANIM_GET_ON_OFF_POKEMON_SOUTH,
    [DIR_NORTHWEST] = ANIM_GET_ON_OFF_POKEMON_NORTH,
    [DIR_NORTHEAST] = ANIM_GET_ON_OFF_POKEMON_NORTH,
};
static const u8 sAcroWheelieDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_BUNNY_HOP_BACK_WHEEL_SOUTH,
    [DIR_SOUTH] = ANIM_BUNNY_HOP_BACK_WHEEL_SOUTH,
    [DIR_NORTH] = ANIM_BUNNY_HOP_BACK_WHEEL_NORTH,
    [DIR_WEST] = ANIM_BUNNY_HOP_BACK_WHEEL_WEST,
    [DIR_EAST] = ANIM_BUNNY_HOP_BACK_WHEEL_EAST,
    [DIR_SOUTHWEST] = ANIM_BUNNY_HOP_BACK_WHEEL_SOUTH,
    [DIR_SOUTHEAST] = ANIM_BUNNY_HOP_BACK_WHEEL_SOUTH,
    [DIR_NORTHWEST] = ANIM_BUNNY_HOP_BACK_WHEEL_NORTH,
    [DIR_NORTHEAST] = ANIM_BUNNY_HOP_BACK_WHEEL_NORTH,
};
static const u8 sAcroUnusedDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_BUNNY_HOP_FRONT_WHEEL_SOUTH,
    [DIR_SOUTH] = ANIM_BUNNY_HOP_FRONT_WHEEL_SOUTH,
    [DIR_NORTH] = ANIM_BUNNY_HOP_FRONT_WHEEL_NORTH,
    [DIR_WEST] = ANIM_BUNNY_HOP_FRONT_WHEEL_WEST,
    [DIR_EAST] = ANIM_BUNNY_HOP_FRONT_WHEEL_EAST,
    [DIR_SOUTHWEST] = ANIM_BUNNY_HOP_FRONT_WHEEL_SOUTH,
    [DIR_SOUTHEAST] = ANIM_BUNNY_HOP_FRONT_WHEEL_SOUTH,
    [DIR_NORTHWEST] = ANIM_BUNNY_HOP_FRONT_WHEEL_NORTH,
    [DIR_NORTHEAST] = ANIM_BUNNY_HOP_FRONT_WHEEL_NORTH,
};
static const u8 sAcroEndWheelieDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_STANDING_WHEELIE_BACK_WHEEL_SOUTH,
    [DIR_SOUTH] = ANIM_STANDING_WHEELIE_BACK_WHEEL_SOUTH,
    [DIR_NORTH] = ANIM_STANDING_WHEELIE_BACK_WHEEL_NORTH,
    [DIR_WEST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_WEST,
    [DIR_EAST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_EAST,
    [DIR_SOUTHWEST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_SOUTH,
    [DIR_NORTHWEST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_NORTH,
    [DIR_NORTHEAST] = ANIM_STANDING_WHEELIE_BACK_WHEEL_NORTH,
};
static const u8 sAcroUnusedActionDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_SOUTH,
    [DIR_SOUTH] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_SOUTH,
    [DIR_NORTH] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_NORTH,
    [DIR_WEST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_WEST,
    [DIR_EAST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_EAST,
    [DIR_SOUTHWEST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_SOUTH,
    [DIR_SOUTHEAST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_SOUTH,
    [DIR_NORTHWEST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_NORTH,
    [DIR_NORTHEAST] = ANIM_STANDING_WHEELIE_FRONT_WHEEL_NORTH,
};
static const u8 sAcroWheeliePedalDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_MOVING_WHEELIE_SOUTH,
    [DIR_SOUTH] = ANIM_MOVING_WHEELIE_SOUTH,
    [DIR_NORTH] = ANIM_MOVING_WHEELIE_NORTH,
    [DIR_WEST] = ANIM_MOVING_WHEELIE_WEST,
    [DIR_EAST] = ANIM_MOVING_WHEELIE_EAST,
    [DIR_SOUTHWEST] = ANIM_MOVING_WHEELIE_SOUTH,
    [DIR_SOUTHEAST] = ANIM_MOVING_WHEELIE_SOUTH,
    [DIR_NORTHWEST] = ANIM_MOVING_WHEELIE_NORTH,
    [DIR_NORTHEAST] = ANIM_MOVING_WHEELIE_NORTH,
};
static const u8 sFishingDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_TAKE_OUT_ROD_SOUTH,
    [DIR_SOUTH] = ANIM_TAKE_OUT_ROD_SOUTH,
    [DIR_NORTH] = ANIM_TAKE_OUT_ROD_NORTH,
    [DIR_WEST] = ANIM_TAKE_OUT_ROD_WEST,
    [DIR_EAST] = ANIM_TAKE_OUT_ROD_EAST,
    [DIR_SOUTHWEST] = ANIM_TAKE_OUT_ROD_SOUTH,
    [DIR_SOUTHEAST] = ANIM_TAKE_OUT_ROD_SOUTH,
    [DIR_NORTHWEST] = ANIM_TAKE_OUT_ROD_NORTH,
    [DIR_NORTHEAST] = ANIM_TAKE_OUT_ROD_NORTH,
};
static const u8 sFishingNoCatchDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_PUT_AWAY_ROD_SOUTH,
    [DIR_SOUTH] = ANIM_PUT_AWAY_ROD_SOUTH,
    [DIR_NORTH] = ANIM_PUT_AWAY_ROD_NORTH,
    [DIR_WEST] = ANIM_PUT_AWAY_ROD_WEST,
    [DIR_EAST] = ANIM_PUT_AWAY_ROD_EAST,
    [DIR_SOUTHWEST] = ANIM_PUT_AWAY_ROD_SOUTH,
    [DIR_SOUTHEAST] = ANIM_PUT_AWAY_ROD_SOUTH,
    [DIR_NORTHWEST] = ANIM_PUT_AWAY_ROD_NORTH,
    [DIR_NORTHEAST] = ANIM_PUT_AWAY_ROD_NORTH,
};
static const u8 sFishingBiteDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_HOOKED_POKEMON_SOUTH,
    [DIR_SOUTH] = ANIM_HOOKED_POKEMON_SOUTH,
    [DIR_NORTH] = ANIM_HOOKED_POKEMON_NORTH,
    [DIR_WEST] = ANIM_HOOKED_POKEMON_WEST,
    [DIR_EAST] = ANIM_HOOKED_POKEMON_EAST,
    [DIR_SOUTHWEST] = ANIM_HOOKED_POKEMON_SOUTH,
    [DIR_SOUTHEAST] = ANIM_HOOKED_POKEMON_SOUTH,
    [DIR_NORTHWEST] = ANIM_HOOKED_POKEMON_NORTH,
    [DIR_NORTHEAST] = ANIM_HOOKED_POKEMON_NORTH,
};
static const u8 sRunningDirectionAnimNums[] = {
    [DIR_NONE] = ANIM_RUN_SOUTH,
    [DIR_SOUTH] = ANIM_RUN_SOUTH,
    [DIR_NORTH] = ANIM_RUN_NORTH,
    [DIR_WEST] = ANIM_RUN_WEST,
    [DIR_EAST] = ANIM_RUN_EAST,
    [DIR_SOUTHWEST] = ANIM_RUN_SOUTH,
    [DIR_SOUTHEAST] = ANIM_RUN_SOUTH,
    [DIR_NORTHWEST] = ANIM_RUN_NORTH,
    [DIR_NORTHEAST] = ANIM_RUN_NORTH,
};

const u8 gTrainerFacingDirectionMovementTypes[] = {
    [DIR_NONE] = MOVEMENT_TYPE_FACE_DOWN,
    [DIR_SOUTH] = MOVEMENT_TYPE_FACE_DOWN,
    [DIR_NORTH] = MOVEMENT_TYPE_FACE_UP,
    [DIR_WEST] = MOVEMENT_TYPE_FACE_LEFT,
    [DIR_EAST] = MOVEMENT_TYPE_FACE_RIGHT,
    [DIR_SOUTHWEST] = MOVEMENT_TYPE_FACE_DOWN,
    [DIR_SOUTHEAST] = MOVEMENT_TYPE_FACE_DOWN,
    [DIR_NORTHWEST] = MOVEMENT_TYPE_FACE_UP,
    [DIR_NORTHEAST] = MOVEMENT_TYPE_FACE_UP,
};

bool8 (*const gOppositeDirectionBlockedMetatileFuncs[])(u8) = {
    MetatileBehavior_IsSouthBlocked,
    MetatileBehavior_IsNorthBlocked,
    MetatileBehavior_IsWestBlocked,
    MetatileBehavior_IsEastBlocked
};

bool8 (*const gDirectionBlockedMetatileFuncs[])(u8) = {
    MetatileBehavior_IsNorthBlocked,
    MetatileBehavior_IsSouthBlocked,
    MetatileBehavior_IsEastBlocked,
    MetatileBehavior_IsWestBlocked
};

static const struct Coords16 sDirectionToVectors[] = {
    { 0,  0},
    { 0,  1},
    { 0, -1},
    {-1,  0},
    { 1,  0},
    {-1,  1},
    { 1,  1},
    {-1, -1},
    { 1, -1}
};

const u8 gFaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_FACE_DOWN,
    MOVEMENT_ACTION_FACE_DOWN,
    MOVEMENT_ACTION_FACE_UP,
    MOVEMENT_ACTION_FACE_LEFT,
    MOVEMENT_ACTION_FACE_RIGHT,
};
const u8 gWalkSlowMovementActions[] = {
    MOVEMENT_ACTION_WALK_SLOW_DOWN,
    MOVEMENT_ACTION_WALK_SLOW_DOWN,
    MOVEMENT_ACTION_WALK_SLOW_UP,
    MOVEMENT_ACTION_WALK_SLOW_LEFT,
    MOVEMENT_ACTION_WALK_SLOW_RIGHT,
};
const u8 gWalkNormalMovementActions[] = {
    MOVEMENT_ACTION_WALK_NORMAL_DOWN,
    MOVEMENT_ACTION_WALK_NORMAL_DOWN,
    MOVEMENT_ACTION_WALK_NORMAL_UP,
    MOVEMENT_ACTION_WALK_NORMAL_LEFT,
    MOVEMENT_ACTION_WALK_NORMAL_RIGHT,
};
const u8 gWalkFastMovementActions[] = {
    MOVEMENT_ACTION_WALK_FAST_DOWN,
    MOVEMENT_ACTION_WALK_FAST_DOWN,
    MOVEMENT_ACTION_WALK_FAST_UP,
    MOVEMENT_ACTION_WALK_FAST_LEFT,
    MOVEMENT_ACTION_WALK_FAST_RIGHT,
};
const u8 gRideWaterCurrentMovementActions[] = {
    MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN,
    MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN,
    MOVEMENT_ACTION_RIDE_WATER_CURRENT_UP,
    MOVEMENT_ACTION_RIDE_WATER_CURRENT_LEFT,
    MOVEMENT_ACTION_RIDE_WATER_CURRENT_RIGHT,
};
const u8 gWalkFasterMovementActions[] = {
    MOVEMENT_ACTION_WALK_FASTER_DOWN,
    MOVEMENT_ACTION_WALK_FASTER_DOWN,
    MOVEMENT_ACTION_WALK_FASTER_UP,
    MOVEMENT_ACTION_WALK_FASTER_LEFT,
    MOVEMENT_ACTION_WALK_FASTER_RIGHT,
};
const u8 gSlideMovementActions[] = {
    MOVEMENT_ACTION_SLIDE_DOWN,
    MOVEMENT_ACTION_SLIDE_DOWN,
    MOVEMENT_ACTION_SLIDE_UP,
    MOVEMENT_ACTION_SLIDE_LEFT,
    MOVEMENT_ACTION_SLIDE_RIGHT,
};
const u8 gPlayerRunMovementActions[] = {
    MOVEMENT_ACTION_PLAYER_RUN_DOWN,
    MOVEMENT_ACTION_PLAYER_RUN_DOWN,
    MOVEMENT_ACTION_PLAYER_RUN_UP,
    MOVEMENT_ACTION_PLAYER_RUN_LEFT,
    MOVEMENT_ACTION_PLAYER_RUN_RIGHT,
};
const u8 gJump2MovementActions[] = {
    MOVEMENT_ACTION_JUMP_2_DOWN,
    MOVEMENT_ACTION_JUMP_2_DOWN,
    MOVEMENT_ACTION_JUMP_2_UP,
    MOVEMENT_ACTION_JUMP_2_LEFT,
    MOVEMENT_ACTION_JUMP_2_RIGHT,
};
const u8 gJumpInPlaceMovementActions[] = {
    MOVEMENT_ACTION_JUMP_IN_PLACE_DOWN,
    MOVEMENT_ACTION_JUMP_IN_PLACE_DOWN,
    MOVEMENT_ACTION_JUMP_IN_PLACE_UP,
    MOVEMENT_ACTION_JUMP_IN_PLACE_LEFT,
    MOVEMENT_ACTION_JUMP_IN_PLACE_RIGHT,
};
const u8 gJumpInPlaceTurnAroundMovementActions[] = {
    MOVEMENT_ACTION_JUMP_IN_PLACE_UP_DOWN,
    MOVEMENT_ACTION_JUMP_IN_PLACE_UP_DOWN,
    MOVEMENT_ACTION_JUMP_IN_PLACE_DOWN_UP,
    MOVEMENT_ACTION_JUMP_IN_PLACE_RIGHT_LEFT,
    MOVEMENT_ACTION_JUMP_IN_PLACE_LEFT_RIGHT,
};
const u8 gJumpMovementActions[] = {
    MOVEMENT_ACTION_JUMP_DOWN,
    MOVEMENT_ACTION_JUMP_DOWN,
    MOVEMENT_ACTION_JUMP_UP,
    MOVEMENT_ACTION_JUMP_LEFT,
    MOVEMENT_ACTION_JUMP_RIGHT,
};
const u8 gJumpSpecialMovementActions[] = {
    MOVEMENT_ACTION_JUMP_SPECIAL_DOWN,
    MOVEMENT_ACTION_JUMP_SPECIAL_DOWN,
    MOVEMENT_ACTION_JUMP_SPECIAL_UP,
    MOVEMENT_ACTION_JUMP_SPECIAL_LEFT,
    MOVEMENT_ACTION_JUMP_SPECIAL_RIGHT,
};
const u8 gWalkInPlaceSlowMovementActions[] = {
    MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_UP,
    MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_LEFT,
    MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_RIGHT,
};
const u8 gWalkInPlaceNormalMovementActions[] = {
    MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_UP,
    MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_LEFT,
    MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_RIGHT,
};
const u8 gWalkInPlaceFastMovementActions[] = {
    MOVEMENT_ACTION_WALK_IN_PLACE_FAST_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_FAST_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_FAST_UP,
    MOVEMENT_ACTION_WALK_IN_PLACE_FAST_LEFT,
    MOVEMENT_ACTION_WALK_IN_PLACE_FAST_RIGHT,
};
const u8 gWalkInPlaceFasterMovementActions[] = {
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_DOWN,
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_UP,
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_LEFT,
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_RIGHT,
};
const u8 gAcroWheelieFaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_FACE_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_FACE_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_FACE_RIGHT,
};
const u8 gAcroPopWheelieFaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_DOWN,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_DOWN,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_UP,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_LEFT,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_RIGHT,
};
const u8 gAcroEndWheelieFaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_UP,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_LEFT,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_RIGHT,
};
const u8 gAcroWheelieHopFaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_RIGHT,
};
const u8 gAcroWheelieHopDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_HOP_RIGHT,
};
const u8 gAcroWheelieJumpDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_RIGHT,
};
const u8 gAcroWheelieInPlaceDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_RIGHT,
};
const u8 gAcroPopWheelieMoveDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_UP,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_LEFT,
    MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_RIGHT,
};
const u8 gAcroWheelieMoveDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_UP,
    MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_LEFT,
    MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_RIGHT,
};
const u8 gAcroEndWheelieMoveDirectionMovementActions[] = {
    MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_DOWN,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_UP,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_LEFT,
    MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_RIGHT,
};

static const u8 sOppositeDirections[] = {
    DIR_NORTH,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_NORTHEAST,
    DIR_NORTHWEST,
    DIR_SOUTHEAST,
    DIR_SOUTHWEST,
};

// Takes the player's original and current facing direction to get the direction that should be considered to copy.
// Note that this means an NPC who copies the player's movement changes how they copy them based on how
// the player entered the area. For instance an NPC who does the same movements as the player when they
// entered the area facing South will do the opposite movements as the player if they enter facing North.
static const u8 sPlayerDirectionsForCopy[][4] = {
    [DIR_SOUTH - 1] = {
        [DIR_SOUTH - 1] = DIR_NORTH,
        [DIR_NORTH - 1] = DIR_SOUTH,
        [DIR_WEST - 1]  = DIR_EAST,
        [DIR_EAST - 1]  = DIR_WEST
    },
    [DIR_NORTH - 1] = {
        [DIR_SOUTH - 1] = DIR_SOUTH,
        [DIR_NORTH - 1] = DIR_NORTH,
        [DIR_WEST - 1]  = DIR_WEST,
        [DIR_EAST - 1]  = DIR_EAST
    },
    [DIR_WEST - 1] = {
        [DIR_SOUTH - 1] = DIR_WEST,
        [DIR_NORTH - 1] = DIR_EAST,
        [DIR_WEST - 1]  = DIR_NORTH,
        [DIR_EAST - 1]  = DIR_SOUTH
    },
    [DIR_EAST - 1] = {
        [DIR_SOUTH - 1] = DIR_EAST,
        [DIR_NORTH - 1] = DIR_WEST,
        [DIR_WEST - 1]  = DIR_SOUTH,
        [DIR_EAST - 1]  = DIR_NORTH
    }
};

// Indexed first with the NPC's initial facing direction based on movement type, and secondly with the player direction to copy.
// Returns the direction the copy NPC should travel in.
static const u8 sPlayerDirectionToCopyDirection[][4] = {
    [DIR_SOUTH - 1] = { // MOVEMENT_TYPE_COPY_PLAYER_OPPOSITE(_IN_GRASS)
        [DIR_SOUTH - 1] = DIR_NORTH,
        [DIR_NORTH - 1] = DIR_SOUTH,
        [DIR_WEST - 1]  = DIR_EAST,
        [DIR_EAST - 1]  = DIR_WEST
    },
    [DIR_NORTH - 1] = { // MOVEMENT_TYPE_COPY_PLAYER(_IN_GRASS)
        [DIR_SOUTH - 1] = DIR_SOUTH,
        [DIR_NORTH - 1] = DIR_NORTH,
        [DIR_WEST - 1]  = DIR_WEST,
        [DIR_EAST - 1]  = DIR_EAST
    },
    [DIR_WEST - 1] = { // MOVEMENT_TYPE_COPY_PLAYER_COUNTERCLOCKWISE(_IN_GRASS)
        [DIR_SOUTH - 1] = DIR_EAST,
        [DIR_NORTH - 1] = DIR_WEST,
        [DIR_WEST - 1]  = DIR_SOUTH,
        [DIR_EAST - 1]  = DIR_NORTH
    },
    [DIR_EAST - 1] = { // MOVEMENT_TYPE_COPY_PLAYER_CLOCKWISE(_IN_GRASS)
        [DIR_SOUTH - 1] = DIR_WEST,
        [DIR_NORTH - 1] = DIR_EAST,
        [DIR_WEST - 1]  = DIR_NORTH,
        [DIR_EAST - 1]  = DIR_SOUTH
    }
};

#include "data/object_events/movement_action_func_tables.h"

static void ClearObjectEvent(struct ObjectEvent *objectEvent)
{
    *objectEvent = (struct ObjectEvent){};
    objectEvent->localId = OBJ_EVENT_ID_PLAYER;
    objectEvent->mapNum = MAP_NUM(UNDEFINED);
    objectEvent->mapGroup = MAP_GROUP(UNDEFINED);
    objectEvent->movementActionId = MOVEMENT_ACTION_NONE;
}

static void ClearAllObjectEvents(void)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        ClearObjectEvent(&gObjectEvents[i]);
}

void ResetObjectEvents(void)
{
    ClearLinkPlayerObjectEvents();
    ClearAllObjectEvents();
    ClearPlayerAvatarInfo();
    CreateReflectionEffectSprites();
}

static void CreateReflectionEffectSprites(void)
{
    u8 spriteId = CreateSpriteAtEnd(gFieldEffectObjectTemplatePointers[FLDEFFOBJ_REFLECTION_DISTORTION], 0, 0, 31);
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
    InitSpriteAffineAnim(&gSprites[spriteId]);
    StartSpriteAffineAnim(&gSprites[spriteId], 0);
    gSprites[spriteId].invisible = TRUE;

    spriteId = CreateSpriteAtEnd(gFieldEffectObjectTemplatePointers[FLDEFFOBJ_REFLECTION_DISTORTION], 0, 0, 31);
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
    InitSpriteAffineAnim(&gSprites[spriteId]);
    StartSpriteAffineAnim(&gSprites[spriteId], 1);
    gSprites[spriteId].invisible = TRUE;
}

u8 GetFirstInactiveObjectEventId(void)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (!gObjectEvents[i].active)
            break;
    }

    return i;
}

u8 GetObjectEventIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroupId)
{
    if (localId < OBJ_EVENT_ID_PLAYER)
        return GetObjectEventIdByLocalIdAndMapInternal(localId, mapNum, mapGroupId);

    return GetObjectEventIdByLocalId(localId);
}

bool8 TryGetObjectEventIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroupId, u8 *objectEventId)
{
    *objectEventId = GetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroupId);
    if (*objectEventId == OBJECT_EVENTS_COUNT)
        return TRUE;
    else
        return FALSE;
}

u8 GetObjectEventIdByXY(s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].currentCoords.x == x && gObjectEvents[i].currentCoords.y == y)
            break;
    }

    return i;
}

static u8 GetObjectEventIdByLocalIdAndMapInternal(u8 localId, u8 mapNum, u8 mapGroupId)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == localId && gObjectEvents[i].mapNum == mapNum && gObjectEvents[i].mapGroup == mapGroupId)
            return i;
    }

    return OBJECT_EVENTS_COUNT;
}

static u8 GetObjectEventIdByLocalId(u8 localId)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == localId)
            return i;
    }

    return OBJECT_EVENTS_COUNT;
}

static u8 InitObjectEventStateFromTemplate(const struct ObjectEventTemplate *template, u8 mapNum, u8 mapGroup)
{
    struct ObjectEvent *objectEvent;
    u8 objectEventId;
    s16 x;
    s16 y;

    if (GetAvailableObjectEventId(template->localId, mapNum, mapGroup, &objectEventId))
        return OBJECT_EVENTS_COUNT;
    objectEvent = &gObjectEvents[objectEventId];
    ClearObjectEvent(objectEvent);
    x = template->x + MAP_OFFSET;
    y = template->y + MAP_OFFSET;
    objectEvent->active = TRUE;
    objectEvent->triggerGroundEffectsOnMove = TRUE;
    objectEvent->graphicsId = template->graphicsId;
    objectEvent->movementType = template->movementType;
    objectEvent->localId = template->localId;
    objectEvent->mapNum = mapNum;
    objectEvent->mapGroup = mapGroup;
    objectEvent->initialCoords.x = x;
    objectEvent->initialCoords.y = y;
    objectEvent->currentCoords.x = x;
    objectEvent->currentCoords.y = y;
    objectEvent->previousCoords.x = x;
    objectEvent->previousCoords.y = y;
    objectEvent->currentElevation = template->elevation;
    objectEvent->previousElevation = template->elevation;
    objectEvent->rangeX = template->movementRangeX;
    objectEvent->rangeY = template->movementRangeY;
    objectEvent->trainerType = template->trainerType;
    objectEvent->mapNum = mapNum;
    objectEvent->trainerRange_berryTreeId = template->trainerRange_berryTreeId;
    objectEvent->previousMovementDirection = gInitialMovementTypeFacingDirections[template->movementType];
    SetObjectEventDirection(objectEvent, objectEvent->previousMovementDirection);
    SetObjectEventDynamicGraphicsId(objectEvent);
    if (sMovementTypeHasRange[objectEvent->movementType])
    {
        if (objectEvent->rangeX == 0)
            objectEvent->rangeX++;
        if (objectEvent->rangeY == 0)
            objectEvent->rangeY++;
    }
    return objectEventId;
}

u8 Unref_TryInitLocalObjectEvent(u8 localId)
{
    u8 i;
    u8 objectEventCount;
    struct ObjectEventTemplate *template;

    if (gMapHeader.events != NULL)
    {
        if (InBattlePyramid())
            objectEventCount = GetNumBattlePyramidObjectEvents();
        else if (InTrainerHill())
            objectEventCount = HILL_TRAINERS_PER_FLOOR;
        else
            objectEventCount = gMapHeader.events->objectEventCount;

        for (i = 0; i < objectEventCount; i++)
        {
            template = &gSaveBlock1Ptr->objectEventTemplates[i];
            if (template->localId == localId && !FlagGet(template->flagId))
                return InitObjectEventStateFromTemplate(template, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
        }
    }
    return OBJECT_EVENTS_COUNT;
}

static bool8 GetAvailableObjectEventId(u16 localId, u8 mapNum, u8 mapGroup, u8 *objectEventId)
// Looks for an empty slot.
// Returns FALSE and the location of the available slot
// in *objectEventId.
// If no slots are available, or if the object is already
// loaded, returns TRUE.
{
    u8 i = 0;

    for (i = 0; i < OBJECT_EVENTS_COUNT && gObjectEvents[i].active; i++)
    {
        if (gObjectEvents[i].localId == localId && gObjectEvents[i].mapNum == mapNum && gObjectEvents[i].mapGroup == mapGroup)
            return TRUE;
    }
    if (i >= OBJECT_EVENTS_COUNT)
        return TRUE;
    *objectEventId = i;
    for (; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == localId && gObjectEvents[i].mapNum == mapNum && gObjectEvents[i].mapGroup == mapGroup)
            return TRUE;
    }
    return FALSE;
}

static void RemoveObjectEvent(struct ObjectEvent *objectEvent)
{
    objectEvent->active = FALSE;
    RemoveObjectEventInternal(objectEvent);
}

void RemoveObjectEventByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        FlagSet(GetObjectEventFlagIdByObjectEventId(objectEventId));
        RemoveObjectEvent(&gObjectEvents[objectEventId]);
    }
}

static void RemoveObjectEventInternal(struct ObjectEvent *objectEvent)
{
    struct SpriteFrameImage image;
    image.size = GetObjectEventGraphicsInfo(objectEvent->graphicsId)->size;
    gSprites[objectEvent->spriteId].images = &image;
    DestroySprite(&gSprites[objectEvent->spriteId]);
}

void RemoveAllObjectEventsExceptPlayer(void)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (i != gPlayerAvatar.objectEventId)
            RemoveObjectEvent(&gObjectEvents[i]);
    }
}

static u8 TrySetupObjectEventSprite(const struct ObjectEventTemplate *objectEventTemplate, struct SpriteTemplate *spriteTemplate, u8 mapNum, u8 mapGroup, s16 cameraX, s16 cameraY)
{
    u8 spriteId;
    u8 paletteSlot;
    u8 objectEventId;
    struct Sprite *sprite;
    struct ObjectEvent *objectEvent;
    const struct ObjectEventGraphicsInfo *graphicsInfo;

    objectEventId = InitObjectEventStateFromTemplate(objectEventTemplate, mapNum, mapGroup);
    if (objectEventId == OBJECT_EVENTS_COUNT)
        return OBJECT_EVENTS_COUNT;

    objectEvent = &gObjectEvents[objectEventId];
    graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);
    paletteSlot = graphicsInfo->paletteSlot;
    if (paletteSlot == PALSLOT_PLAYER)
    {
        LoadPlayerObjectReflectionPalette(graphicsInfo->paletteTag, paletteSlot);
    }
    else if (paletteSlot == PALSLOT_NPC_SPECIAL)
    {
        LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, paletteSlot);
    }
    else if (paletteSlot >= 16)
    {
        paletteSlot -= 16;
        _PatchObjectPalette(graphicsInfo->paletteTag, paletteSlot);
    }

    if (objectEvent->movementType == MOVEMENT_TYPE_INVISIBLE)
        objectEvent->invisible = TRUE;

    *(u16 *)&spriteTemplate->paletteTag = TAG_NONE;
    spriteId = CreateSprite(spriteTemplate, 0, 0, 0);
    if (spriteId == MAX_SPRITES)
    {
        gObjectEvents[objectEventId].active = FALSE;
        return OBJECT_EVENTS_COUNT;
    }

    sprite = &gSprites[spriteId];
    GetMapCoordsFromSpritePos(objectEvent->currentCoords.x + cameraX, objectEvent->currentCoords.y + cameraY, &sprite->x, &sprite->y);
    sprite->centerToCornerVecX = -(graphicsInfo->width >> 1);
    sprite->centerToCornerVecY = -(graphicsInfo->height >> 1);
    sprite->x += 8;
    sprite->y += 16 + sprite->centerToCornerVecY;
    sprite->oam.paletteNum = paletteSlot;
    sprite->coordOffsetEnabled = TRUE;
    sprite->sObjEventId = objectEventId;
    objectEvent->spriteId = spriteId;
    objectEvent->inanimate = graphicsInfo->inanimate;
    if (!objectEvent->inanimate)
        StartSpriteAnim(sprite, GetFaceDirectionAnimNum(objectEvent->facingDirection));

    SetObjectSubpriorityByElevation(objectEvent->previousElevation, sprite, 1);
    UpdateObjectEventVisibility(objectEvent, sprite);
    return objectEventId;
}

static u8 TrySpawnObjectEventTemplate(const struct ObjectEventTemplate *objectEventTemplate, u8 mapNum, u8 mapGroup, s16 cameraX, s16 cameraY)
{
    u8 objectEventId;
    struct SpriteTemplate spriteTemplate;
    struct SpriteFrameImage spriteFrameImage;
    const struct ObjectEventGraphicsInfo *graphicsInfo;
    const struct SubspriteTable *subspriteTables = NULL;

    graphicsInfo = GetObjectEventGraphicsInfo(objectEventTemplate->graphicsId);
    MakeSpriteTemplateFromObjectEventTemplate(objectEventTemplate, &spriteTemplate, &subspriteTables);
    spriteFrameImage.size = graphicsInfo->size;
    spriteTemplate.images = &spriteFrameImage;
    objectEventId = TrySetupObjectEventSprite(objectEventTemplate, &spriteTemplate, mapNum, mapGroup, cameraX, cameraY);
    if (objectEventId == OBJECT_EVENTS_COUNT)
        return OBJECT_EVENTS_COUNT;

    gSprites[gObjectEvents[objectEventId].spriteId].images = graphicsInfo->images;
    if (subspriteTables)
        SetSubspriteTables(&gSprites[gObjectEvents[objectEventId].spriteId], subspriteTables);

    return objectEventId;
}

u8 SpawnSpecialObjectEvent(struct ObjectEventTemplate *objectEventTemplate)
{
    s16 cameraX;
    s16 cameraY;

    GetObjectEventMovingCameraOffset(&cameraX, &cameraY);
    return TrySpawnObjectEventTemplate(objectEventTemplate, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, cameraX, cameraY);
}

u8 SpawnSpecialObjectEventParameterized(u8 graphicsId, u8 movementBehavior, u8 localId, s16 x, s16 y, u8 elevation)
{
    struct ObjectEventTemplate objectEventTemplate;

    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    objectEventTemplate.localId = localId;
    objectEventTemplate.graphicsId = graphicsId;
    objectEventTemplate.kind = OBJ_KIND_NORMAL;
    objectEventTemplate.x = x;
    objectEventTemplate.y = y;
    objectEventTemplate.elevation = elevation;
    objectEventTemplate.movementType = movementBehavior;
    objectEventTemplate.movementRangeX = 0;
    objectEventTemplate.movementRangeY = 0;
    objectEventTemplate.trainerType = TRAINER_TYPE_NONE;
    objectEventTemplate.trainerRange_berryTreeId = 0;
    return SpawnSpecialObjectEvent(&objectEventTemplate);
}

u8 TrySpawnObjectEvent(u8 localId, u8 mapNum, u8 mapGroup)
{
    const struct ObjectEventTemplate *objectEventTemplate;
    s16 cameraX, cameraY;

    objectEventTemplate = GetObjectEventTemplateByLocalIdAndMap(localId, mapNum, mapGroup);
    if (!objectEventTemplate)
        return OBJECT_EVENTS_COUNT;

    GetObjectEventMovingCameraOffset(&cameraX, &cameraY);
    return TrySpawnObjectEventTemplate(objectEventTemplate, mapNum, mapGroup, cameraX, cameraY);
}

static void CopyObjectGraphicsInfoToSpriteTemplate(u16 graphicsId, void (*callback)(struct Sprite *), struct SpriteTemplate *spriteTemplate, const struct SubspriteTable **subspriteTables)
{
    const struct ObjectEventGraphicsInfo *graphicsInfo = GetObjectEventGraphicsInfo(graphicsId);

    spriteTemplate->tileTag = graphicsInfo->tileTag;
    spriteTemplate->paletteTag = graphicsInfo->paletteTag;
    spriteTemplate->oam = graphicsInfo->oam;
    spriteTemplate->anims = graphicsInfo->anims;
    spriteTemplate->images = graphicsInfo->images;
    spriteTemplate->affineAnims = graphicsInfo->affineAnims;
    spriteTemplate->callback = callback;
    *subspriteTables = graphicsInfo->subspriteTables;
}

static void CopyObjectGraphicsInfoToSpriteTemplate_WithMovementType(u16 graphicsId, u16 movementType, struct SpriteTemplate *spriteTemplate, const struct SubspriteTable **subspriteTables)
{
    CopyObjectGraphicsInfoToSpriteTemplate(graphicsId, sMovementTypeCallbacks[movementType], spriteTemplate, subspriteTables);
}

static void MakeSpriteTemplateFromObjectEventTemplate(const struct ObjectEventTemplate *objectEventTemplate, struct SpriteTemplate *spriteTemplate, const struct SubspriteTable **subspriteTables)
{
    CopyObjectGraphicsInfoToSpriteTemplate_WithMovementType(objectEventTemplate->graphicsId, objectEventTemplate->movementType, spriteTemplate, subspriteTables);
}

// Used to create a sprite using a graphicsId associated with object events.
u8 CreateObjectGraphicsSprite(u16 graphicsId, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority)
{
    struct SpriteTemplate *spriteTemplate;
    const struct SubspriteTable *subspriteTables;
    struct Sprite *sprite;
    u8 spriteId;

    spriteTemplate = Alloc(sizeof(struct SpriteTemplate));
    CopyObjectGraphicsInfoToSpriteTemplate(graphicsId, callback, spriteTemplate, &subspriteTables);
    if (spriteTemplate->paletteTag != TAG_NONE)
        LoadObjectEventPalette(spriteTemplate->paletteTag);

    spriteId = CreateSprite(spriteTemplate, x, y, subpriority);
    Free(spriteTemplate);

    if (spriteId != MAX_SPRITES && subspriteTables != NULL)
    {
        sprite = &gSprites[spriteId];
        SetSubspriteTables(sprite, subspriteTables);
        sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
    }
    return spriteId;
}

#define sVirtualObjId   data[0]
#define sVirtualObjElev data[1]

// "Virtual Objects" are a class of sprites used instead of a full object event.
// Used when more objects are needed than the object event limit (for Contest / Battle Dome audiences and group members in Union Room).
// A unique id is given as an argument and stored in the sprite data to allow referring back to the same virtual object.
// They can be turned (and, in the case of the Union Room, animated teleporting in and out) but do not have movement types
// or any of the other data normally associated with object events.
u8 CreateVirtualObject(u8 graphicsId, u8 virtualObjId, s16 x, s16 y, u8 elevation, u8 direction)
{
    u8 spriteId;
    struct Sprite *sprite;
    struct SpriteTemplate spriteTemplate;
    const struct SubspriteTable *subspriteTables;
    const struct ObjectEventGraphicsInfo *graphicsInfo;

    graphicsInfo = GetObjectEventGraphicsInfo(graphicsId);
    CopyObjectGraphicsInfoToSpriteTemplate(graphicsId, SpriteCB_VirtualObject, &spriteTemplate, &subspriteTables);
    *(u16 *)&spriteTemplate.paletteTag = TAG_NONE;
    x += MAP_OFFSET;
    y += MAP_OFFSET;
    SetSpritePosToOffsetMapCoords(&x, &y, 8, 16);
    spriteId = CreateSpriteAtEnd(&spriteTemplate, x, y, 0);
    if (spriteId != MAX_SPRITES)
    {
        sprite = &gSprites[spriteId];
        sprite->centerToCornerVecX = -(graphicsInfo->width >> 1);
        sprite->centerToCornerVecY = -(graphicsInfo->height >> 1);
        sprite->y += sprite->centerToCornerVecY;
        sprite->oam.paletteNum = graphicsInfo->paletteSlot;
        if (sprite->oam.paletteNum >= 16)
            sprite->oam.paletteNum -= 16;

        sprite->coordOffsetEnabled = TRUE;
        sprite->sVirtualObjId = virtualObjId;
        sprite->sVirtualObjElev = elevation;
        if (graphicsInfo->paletteSlot == PALSLOT_NPC_SPECIAL)
            LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
        else if (graphicsInfo->paletteSlot >= 16)
            _PatchObjectPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot | 0xf0);

        if (subspriteTables != NULL)
        {
            SetSubspriteTables(sprite, subspriteTables);
            sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
        }
        InitObjectPriorityByElevation(sprite, elevation);
        SetObjectSubpriorityByElevation(elevation, sprite, 1);
        StartSpriteAnim(sprite, GetFaceDirectionAnimNum(direction));
    }
    return spriteId;
}

void TrySpawnObjectEvents(s16 cameraX, s16 cameraY)
{
    u8 i;
    u8 objectCount;

    if (gMapHeader.events != NULL)
    {
        s16 left = gSaveBlock1Ptr->pos.x - 2;
        s16 right = gSaveBlock1Ptr->pos.x + MAP_OFFSET_W + 2;
        s16 top = gSaveBlock1Ptr->pos.y;
        s16 bottom = gSaveBlock1Ptr->pos.y + MAP_OFFSET_H + 2;

        if (InBattlePyramid())
            objectCount = GetNumBattlePyramidObjectEvents();
        else if (InTrainerHill())
            objectCount = HILL_TRAINERS_PER_FLOOR;
        else
            objectCount = gMapHeader.events->objectEventCount;

        for (i = 0; i < objectCount; i++)
        {
            struct ObjectEventTemplate *template = &gSaveBlock1Ptr->objectEventTemplates[i];
            s16 npcX = template->x + MAP_OFFSET;
            s16 npcY = template->y + MAP_OFFSET;

            if (top <= npcY && bottom >= npcY && left <= npcX && right >= npcX
                && !FlagGet(template->flagId))
                TrySpawnObjectEventTemplate(template, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, cameraX, cameraY);
        }
    }
}

void RemoveObjectEventsOutsideView(void)
{
    u8 i, j;
    bool8 isActiveLinkPlayer;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        for (j = 0, isActiveLinkPlayer = FALSE; j < ARRAY_COUNT(gLinkPlayerObjectEvents); j++)
        {
            if (gLinkPlayerObjectEvents[j].active && i == gLinkPlayerObjectEvents[j].objEventId)
                isActiveLinkPlayer = TRUE;
        }
        if (!isActiveLinkPlayer)
        {
            struct ObjectEvent *objectEvent = &gObjectEvents[i];

            if (objectEvent->active && !objectEvent->isPlayer)
                RemoveObjectEventIfOutsideView(objectEvent);
        }
    }
}

static void RemoveObjectEventIfOutsideView(struct ObjectEvent *objectEvent)
{
    s16 left =   gSaveBlock1Ptr->pos.x - 2;
    s16 right =  gSaveBlock1Ptr->pos.x + 17;
    s16 top =    gSaveBlock1Ptr->pos.y;
    s16 bottom = gSaveBlock1Ptr->pos.y + 16;

    if (objectEvent->currentCoords.x >= left && objectEvent->currentCoords.x <= right
     && objectEvent->currentCoords.y >= top && objectEvent->currentCoords.y <= bottom)
        return;
    if (objectEvent->initialCoords.x >= left && objectEvent->initialCoords.x <= right
     && objectEvent->initialCoords.y >= top && objectEvent->initialCoords.y <= bottom)
        return;
    RemoveObjectEvent(objectEvent);
}

void SpawnObjectEventsOnReturnToField(s16 x, s16 y)
{
    u8 i;

    ClearPlayerAvatarInfo();
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active)
            SpawnObjectEventOnReturnToField(i, x, y);
    }
    CreateReflectionEffectSprites();
}

static void SpawnObjectEventOnReturnToField(u8 objectEventId, s16 x, s16 y)
{
    u8 i;
    u8 paletteSlot;
    struct Sprite *sprite;
    struct ObjectEvent *objectEvent;
    struct SpriteTemplate spriteTemplate;
    struct SpriteFrameImage spriteFrameImage;
    const struct SubspriteTable *subspriteTables;
    const struct ObjectEventGraphicsInfo *graphicsInfo;

    for (i = 0; i < ARRAY_COUNT(gLinkPlayerObjectEvents); i++)
    {
        if (gLinkPlayerObjectEvents[i].active && objectEventId == gLinkPlayerObjectEvents[i].objEventId)
            return;
    }

    objectEvent = &gObjectEvents[objectEventId];
    subspriteTables = NULL;
    graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);
    spriteFrameImage.size = graphicsInfo->size;
    CopyObjectGraphicsInfoToSpriteTemplate_WithMovementType(objectEvent->graphicsId, objectEvent->movementType, &spriteTemplate, &subspriteTables);
    spriteTemplate.images = &spriteFrameImage;

    *(u16 *)&spriteTemplate.paletteTag = TAG_NONE;
    paletteSlot = graphicsInfo->paletteSlot;
    if (paletteSlot == PALSLOT_PLAYER)
    {
        LoadPlayerObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
    }
    else if (paletteSlot == PALSLOT_NPC_SPECIAL)
    {
        LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
    }
    else if (paletteSlot >= 16)
    {
        paletteSlot -= 16;
        _PatchObjectPalette(graphicsInfo->paletteTag, paletteSlot);
    }
    *(u16 *)&spriteTemplate.paletteTag = TAG_NONE;

    i = CreateSprite(&spriteTemplate, 0, 0, 0);
    if (i != MAX_SPRITES)
    {
        sprite = &gSprites[i];
        GetMapCoordsFromSpritePos(x + objectEvent->currentCoords.x, y + objectEvent->currentCoords.y, &sprite->x, &sprite->y);
        sprite->centerToCornerVecX = -(graphicsInfo->width >> 1);
        sprite->centerToCornerVecY = -(graphicsInfo->height >> 1);
        sprite->x += 8;
        sprite->y += 16 + sprite->centerToCornerVecY;
        sprite->images = graphicsInfo->images;
        if (objectEvent->movementType == MOVEMENT_TYPE_PLAYER)
        {
            SetPlayerAvatarObjectEventIdAndObjectId(objectEventId, i);
            objectEvent->warpArrowSpriteId = CreateWarpArrowSprite();
        }
        if (subspriteTables != NULL)
            SetSubspriteTables(sprite, subspriteTables);

        sprite->oam.paletteNum = paletteSlot;
        sprite->coordOffsetEnabled = TRUE;
        sprite->sObjEventId = objectEventId;
        objectEvent->spriteId = i;
        if (!objectEvent->inanimate && objectEvent->movementType != MOVEMENT_TYPE_PLAYER)
            StartSpriteAnim(sprite, GetFaceDirectionAnimNum(objectEvent->facingDirection));

        ResetObjectEventFldEffData(objectEvent);
        SetObjectSubpriorityByElevation(objectEvent->previousElevation, sprite, 1);
    }
}

static void ResetObjectEventFldEffData(struct ObjectEvent *objectEvent)
{
    objectEvent->singleMovementActive = FALSE;
    objectEvent->triggerGroundEffectsOnMove = TRUE;
    objectEvent->hasShadow = FALSE;
    objectEvent->hasReflection = FALSE;
    objectEvent->inShortGrass = FALSE;
    objectEvent->inShallowFlowingWater = FALSE;
    objectEvent->inSandPile = FALSE;
    objectEvent->inHotSprings = FALSE;
    ObjectEventClearHeldMovement(objectEvent);
}

static void SetPlayerAvatarObjectEventIdAndObjectId(u8 objectEventId, u8 spriteId)
{
    gPlayerAvatar.objectEventId = objectEventId;
    gPlayerAvatar.spriteId = spriteId;
    gPlayerAvatar.gender = GetPlayerAvatarGenderByGraphicsId(gObjectEvents[objectEventId].graphicsId);
    SetPlayerAvatarExtraStateTransition(gObjectEvents[objectEventId].graphicsId, PLAYER_AVATAR_FLAG_CONTROLLABLE);
}

void ObjectEventSetGraphicsId(struct ObjectEvent *objectEvent, u8 graphicsId)
{
    const struct ObjectEventGraphicsInfo *graphicsInfo;
    struct Sprite *sprite;
    u8 paletteSlot;

    graphicsInfo = GetObjectEventGraphicsInfo(graphicsId);
    sprite = &gSprites[objectEvent->spriteId];
    paletteSlot = graphicsInfo->paletteSlot;
    if (paletteSlot == PALSLOT_PLAYER)
    {
        PatchObjectPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
    }
    else if (paletteSlot == PALSLOT_NPC_SPECIAL)
    {
        LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
    }
    else if (paletteSlot >= 16)
    {
        paletteSlot -= 16;
        _PatchObjectPalette(graphicsInfo->paletteTag, paletteSlot);
    }
    sprite->oam.shape = graphicsInfo->oam->shape;
    sprite->oam.size = graphicsInfo->oam->size;
    sprite->images = graphicsInfo->images;
    sprite->anims = graphicsInfo->anims;
    sprite->subspriteTables = graphicsInfo->subspriteTables;
    sprite->oam.paletteNum = paletteSlot;
    objectEvent->inanimate = graphicsInfo->inanimate;
    objectEvent->graphicsId = graphicsId;
    SetSpritePosToMapCoords(objectEvent->currentCoords.x, objectEvent->currentCoords.y, &sprite->x, &sprite->y);
    sprite->centerToCornerVecX = -(graphicsInfo->width >> 1);
    sprite->centerToCornerVecY = -(graphicsInfo->height >> 1);
    sprite->x += 8;
    sprite->y += 16 + sprite->centerToCornerVecY;
    if (objectEvent->trackedByCamera)
        CameraObjectReset();
}

void ObjectEventSetGraphicsIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u8 graphicsId)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        ObjectEventSetGraphicsId(&gObjectEvents[objectEventId], graphicsId);
}

void ObjectEventTurn(struct ObjectEvent *objectEvent, u8 direction)
{
    SetObjectEventDirection(objectEvent, direction);
    if (!objectEvent->inanimate)
    {
        StartSpriteAnim(&gSprites[objectEvent->spriteId], GetFaceDirectionAnimNum(objectEvent->facingDirection));
        SeekSpriteAnim(&gSprites[objectEvent->spriteId], 0);
    }
}

void ObjectEventTurnByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u8 direction)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        ObjectEventTurn(&gObjectEvents[objectEventId], direction);
}

void PlayerObjectTurn(struct PlayerAvatar *playerAvatar, u8 direction)
{
    ObjectEventTurn(&gObjectEvents[playerAvatar->objectEventId], direction);
}

static void SetBerryTreeGraphics(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 berryStage;
    u8 berryId;

    objectEvent->invisible = TRUE;
    sprite->invisible = TRUE;
    berryStage = GetStageByBerryTreeId(objectEvent->trainerRange_berryTreeId);
    if (berryStage != BERRY_STAGE_NO_BERRY)
    {
        objectEvent->invisible = FALSE;
        sprite->invisible = FALSE;
        berryId = GetBerryTypeByBerryTreeId(objectEvent->trainerRange_berryTreeId) - 1;
        berryStage--;
        if (berryId > ITEM_TO_BERRY(LAST_BERRY_INDEX))
            berryId = 0;

        ObjectEventSetGraphicsId(objectEvent, gBerryTreeObjectEventGraphicsIdTablePointers[berryId][berryStage]);
        sprite->images = gBerryTreePicTablePointers[berryId];
        sprite->oam.paletteNum = gBerryTreePaletteSlotTablePointers[berryId][berryStage];
        StartSpriteAnim(sprite, berryStage);
    }
}

const struct ObjectEventGraphicsInfo *GetObjectEventGraphicsInfo(u8 graphicsId)
{
    u8 bard;

    if (graphicsId >= OBJ_EVENT_GFX_VARS)
        graphicsId = VarGetObjectEventGraphicsId(graphicsId - OBJ_EVENT_GFX_VARS);

    if (graphicsId == OBJ_EVENT_GFX_BARD)
    {
        bard = GetCurrentMauvilleOldMan();
        return gMauvilleOldManGraphicsInfoPointers[bard];
    }

    if (graphicsId >= NUM_OBJ_EVENT_GFX)
        graphicsId = OBJ_EVENT_GFX_NINJA_BOY;

    return gObjectEventGraphicsInfoPointers[graphicsId];
}

static void SetObjectEventDynamicGraphicsId(struct ObjectEvent *objectEvent)
{
    if (objectEvent->graphicsId >= OBJ_EVENT_GFX_VARS)
        objectEvent->graphicsId = VarGetObjectEventGraphicsId(objectEvent->graphicsId - OBJ_EVENT_GFX_VARS);
}

void SetObjectInvisibility(u8 localId, u8 mapNum, u8 mapGroup, bool8 invisible)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        gObjectEvents[objectEventId].invisible = invisible;
}

void ObjectEventGetLocalIdAndMap(struct ObjectEvent *objectEvent, void *localId, void *mapNum, void *mapGroup)
{
    *(u8 *)(localId) = objectEvent->localId;
    *(u8 *)(mapNum) = objectEvent->mapNum;
    *(u8 *)(mapGroup) = objectEvent->mapGroup;
}

void AllowObjectAtPosTriggerGroundEffects(s16 x, s16 y)
{
    u8 objectEventId;
    struct ObjectEvent *objectEvent;

    objectEventId = GetObjectEventIdByXY(x, y);
    if (objectEventId != OBJECT_EVENTS_COUNT)
    {
        objectEvent = &gObjectEvents[objectEventId];
        objectEvent->triggerGroundEffectsOnMove = TRUE;
    }
}

void SetObjectSubpriority(u8 localId, u8 mapNum, u8 mapGroup, u8 subpriority)
{
    u8 objectEventId;
    struct ObjectEvent *objectEvent;
    struct Sprite *sprite;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        objectEvent = &gObjectEvents[objectEventId];
        sprite = &gSprites[objectEvent->spriteId];
        objectEvent->fixedPriority = TRUE;
        sprite->subpriority = subpriority;
    }
}

void ResetObjectSubpriority(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;
    struct ObjectEvent *objectEvent;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        objectEvent = &gObjectEvents[objectEventId];
        objectEvent->fixedPriority = FALSE;
        objectEvent->triggerGroundEffectsOnMove = TRUE;
    }
}

void SetObjectEventSpritePosByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, s16 x, s16 y)
{
    u8 objectEventId;
    struct Sprite *sprite;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        sprite = &gSprites[gObjectEvents[objectEventId].spriteId];
        sprite->x2 = x;
        sprite->y2 = y;
    }
}

void FreeAndReserveObjectSpritePalettes(void)
{
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = OBJ_PALSLOT_COUNT;
}

static void LoadObjectEventPalette(u16 paletteTag)
{
    u16 i = FindObjectEventPaletteIndexByTag(paletteTag);

// FindObjectEventPaletteIndexByTag returns 0xFF on failure, not OBJ_EVENT_PAL_TAG_NONE.
#ifdef BUGFIX
    if (i != 0xFF)
#else
    if (i != OBJ_EVENT_PAL_TAG_NONE)
#endif
        LoadSpritePaletteIfTagExists(&sObjectEventSpritePalettes[i]);
}

static void UNUSED LoadObjectEventPaletteSet(u16 *paletteTags)
{
    u8 i;

    for (i = 0; paletteTags[i] != OBJ_EVENT_PAL_TAG_NONE; i++)
        LoadObjectEventPalette(paletteTags[i]);
}

static u8 LoadSpritePaletteIfTagExists(const struct SpritePalette *spritePalette)
{
    if (IndexOfSpritePaletteTag(spritePalette->tag) != 0xFF)
        return 0xFF;

    return LoadSpritePalette(spritePalette);
}

void PatchObjectPalette(u16 paletteTag, u8 paletteSlot)
{
    // paletteTag is assumed to exist in sObjectEventSpritePalettes
    u8 paletteIndex = FindObjectEventPaletteIndexByTag(paletteTag);

    LoadPalette(sObjectEventSpritePalettes[paletteIndex].data, OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
}

void PatchObjectPaletteRange(const u16 *paletteTags, u8 minSlot, u8 maxSlot)
{
    while (minSlot < maxSlot)
    {
        PatchObjectPalette(*paletteTags, minSlot);
        paletteTags++;
        minSlot++;
    }
}

static u8 FindObjectEventPaletteIndexByTag(u16 tag)
{
    u8 i;

    for (i = 0; sObjectEventSpritePalettes[i].tag != OBJ_EVENT_PAL_TAG_NONE; i++)
    {
        if (sObjectEventSpritePalettes[i].tag == tag)
            return i;
    }
    return 0xFF;
}

void LoadPlayerObjectReflectionPalette(u16 tag, u8 slot)
{
    u8 i;

    PatchObjectPalette(tag, slot);
    for (i = 0; sPlayerReflectionPaletteSets[i].tag != OBJ_EVENT_PAL_TAG_NONE; i++)
    {
        if (sPlayerReflectionPaletteSets[i].tag == tag)
        {
            PatchObjectPalette(sPlayerReflectionPaletteSets[i].data[sCurrentReflectionType], gReflectionEffectPaletteMap[slot]);
            return;
        }
    }
}

void LoadSpecialObjectReflectionPalette(u16 tag, u8 slot)
{
    u8 i;

    sCurrentSpecialObjectPaletteTag = tag;
    PatchObjectPalette(tag, slot);
    for (i = 0; sSpecialObjectReflectionPaletteSets[i].tag != OBJ_EVENT_PAL_TAG_NONE; i++)
    {
        if (sSpecialObjectReflectionPaletteSets[i].tag == tag)
        {
            PatchObjectPalette(sSpecialObjectReflectionPaletteSets[i].data[sCurrentReflectionType], gReflectionEffectPaletteMap[slot]);
            return;
        }
    }
}

static void _PatchObjectPalette(u16 tag, u8 slot)
{
    PatchObjectPalette(tag, slot);
}

static void UNUSED IncrementObjectEventCoords(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    objectEvent->previousCoords.x = objectEvent->currentCoords.x;
    objectEvent->previousCoords.y = objectEvent->currentCoords.y;
    objectEvent->currentCoords.x += x;
    objectEvent->currentCoords.y += y;
}

void ShiftObjectEventCoords(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    objectEvent->previousCoords.x = objectEvent->currentCoords.x;
    objectEvent->previousCoords.y = objectEvent->currentCoords.y;
    objectEvent->currentCoords.x = x;
    objectEvent->currentCoords.y = y;
}

static void SetObjectEventCoords(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    objectEvent->previousCoords.x = x;
    objectEvent->previousCoords.y = y;
    objectEvent->currentCoords.x = x;
    objectEvent->currentCoords.y = y;
}

void MoveObjectEventToMapCoords(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    struct Sprite *sprite;
    const struct ObjectEventGraphicsInfo *graphicsInfo;

    sprite = &gSprites[objectEvent->spriteId];
    graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);
    SetObjectEventCoords(objectEvent, x, y);
    SetSpritePosToMapCoords(objectEvent->currentCoords.x, objectEvent->currentCoords.y, &sprite->x, &sprite->y);
    sprite->centerToCornerVecX = -(graphicsInfo->width >> 1);
    sprite->centerToCornerVecY = -(graphicsInfo->height >> 1);
    sprite->x += 8;
    sprite->y += 16 + sprite->centerToCornerVecY;
    ResetObjectEventFldEffData(objectEvent);
    if (objectEvent->trackedByCamera)
        CameraObjectReset();
}

void TryMoveObjectEventToMapCoords(u8 localId, u8 mapNum, u8 mapGroup, s16 x, s16 y)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        x += MAP_OFFSET;
        y += MAP_OFFSET;
        MoveObjectEventToMapCoords(&gObjectEvents[objectEventId], x, y);
    }
}

void ShiftStillObjectEventCoords(struct ObjectEvent *objectEvent)
{
    ShiftObjectEventCoords(objectEvent, objectEvent->currentCoords.x, objectEvent->currentCoords.y);
}

void UpdateObjectEventCoordsForCameraUpdate(void)
{
    u8 i;
    s16 dx;
    s16 dy;

    if (gCamera.active)
    {
        dx = gCamera.x;
        dy = gCamera.y;
        for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        {
            if (gObjectEvents[i].active)
            {
                gObjectEvents[i].initialCoords.x -= dx;
                gObjectEvents[i].initialCoords.y -= dy;
                gObjectEvents[i].currentCoords.x -= dx;
                gObjectEvents[i].currentCoords.y -= dy;
                gObjectEvents[i].previousCoords.x -= dx;
                gObjectEvents[i].previousCoords.y -= dy;
            }
        }
    }
}

u8 GetObjectEventIdByPosition(u16 x, u16 y, u8 elevation)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active)
        {
            if (gObjectEvents[i].currentCoords.x == x
             && gObjectEvents[i].currentCoords.y == y
             && ObjectEventDoesElevationMatch(&gObjectEvents[i], elevation))
                return i;
        }
    }
    return OBJECT_EVENTS_COUNT;
}

static bool8 ObjectEventDoesElevationMatch(struct ObjectEvent *objectEvent, u8 elevation)
{
    if (objectEvent->currentElevation != 0 && elevation != 0 && objectEvent->currentElevation != elevation)
        return FALSE;

    return TRUE;
}

void UpdateObjectEventsForCameraUpdate(s16 x, s16 y)
{
    UpdateObjectEventCoordsForCameraUpdate();
    TrySpawnObjectEvents(x, y);
    RemoveObjectEventsOutsideView();
}

// The "CameraObject" functions below are responsible for an invisible sprite
// that follows the movements of a different sprite (normally the player's sprite)
// and tracks x/y movement distances for the camera so it knows where to move.
u8 AddCameraObject(u8 followSpriteId)
{
    u8 spriteId = CreateSprite(&sCameraSpriteTemplate, 0, 0, 4);

    gSprites[spriteId].invisible = TRUE;
    gSprites[spriteId].sCamera_FollowSpriteId = followSpriteId;
    return spriteId;
}

static void SpriteCB_CameraObject(struct Sprite *sprite)
{
    void (*callbacks[ARRAY_COUNT(sCameraObjectFuncs)])(struct Sprite *);

    memcpy(callbacks, sCameraObjectFuncs, sizeof sCameraObjectFuncs);
    callbacks[sprite->sCamera_State](sprite);
}

static void CameraObject_Init(struct Sprite *sprite)
{
    sprite->x = gSprites[sprite->sCamera_FollowSpriteId].x;
    sprite->y = gSprites[sprite->sCamera_FollowSpriteId].y;
    sprite->invisible = TRUE;
    sprite->sCamera_State = CAMERA_STATE_MOVE;
    CameraObject_UpdateMove(sprite);
}

static void CameraObject_UpdateMove(struct Sprite *sprite)
{
    s16 x = gSprites[sprite->sCamera_FollowSpriteId].x;
    s16 y = gSprites[sprite->sCamera_FollowSpriteId].y;

    sprite->sCamera_MoveX = x - sprite->x;
    sprite->sCamera_MoveY = y - sprite->y;
    sprite->x = x;
    sprite->y = y;
}

// Invisible sprite will continue to follow the parent sprite,
// but no corresponding camera movement will be shown.
static void CameraObject_UpdateFrozen(struct Sprite *sprite)
{
    sprite->x = gSprites[sprite->sCamera_FollowSpriteId].x;
    sprite->y = gSprites[sprite->sCamera_FollowSpriteId].y;
    sprite->sCamera_MoveX = 0;
    sprite->sCamera_MoveY = 0;
}

static struct Sprite *FindCameraSprite(void)
{
    u8 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_CameraObject)
            return &gSprites[i];
    }
    return NULL;
}

void CameraObjectReset(void)
{
    struct Sprite *camera = FindCameraSprite();
    if (camera != NULL)
    {
        camera->sCamera_State = CAMERA_STATE_INIT;
        camera->callback(camera);
    }
}

void CameraObjectSetFollowedSpriteId(u8 spriteId)
{
    struct Sprite *camera = FindCameraSprite();
    if (camera != NULL)
    {
        camera->sCamera_FollowSpriteId = spriteId;
        CameraObjectReset();
    }
}

static u8 UNUSED CameraObjectGetFollowedSpriteId(void)
{
    struct Sprite *camera = FindCameraSprite();
    if (camera == NULL)
        return MAX_SPRITES;

    return camera->sCamera_FollowSpriteId;
}

void CameraObjectFreeze(void)
{
    struct Sprite *camera = FindCameraSprite();
#ifdef UBFIX // Possible null dereference
    if (camera == NULL)
        return;
#endif
    camera->sCamera_State = CAMERA_STATE_FROZEN;
}

u8 CopySprite(struct Sprite *sprite, s16 x, s16 y, u8 subpriority)
{
    u8 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (!gSprites[i].inUse)
        {
            gSprites[i] = *sprite;
            gSprites[i].x = x;
            gSprites[i].y = y;
            gSprites[i].subpriority = subpriority;
            break;
        }
    }
    return i;
}

u8 CreateCopySpriteAt(struct Sprite *sprite, s16 x, s16 y, u8 subpriority)
{
    s16 i;

    for (i = MAX_SPRITES - 1; i > -1; i--)
    {
        if (!gSprites[i].inUse)
        {
            gSprites[i] = *sprite;
            gSprites[i].x = x;
            gSprites[i].y = y;
            gSprites[i].subpriority = subpriority;
            return i;
        }
    }
    return MAX_SPRITES;
}

void SetObjectEventDirection(struct ObjectEvent *objectEvent, u8 direction)
{
    s8 d2;
    objectEvent->previousMovementDirection = objectEvent->facingDirection;
    if (!objectEvent->facingDirectionLocked)
    {
        d2 = direction;
        objectEvent->facingDirection = d2;
    }
    objectEvent->movementDirection = direction;
}

static const u8 *GetObjectEventScriptPointerByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    return GetObjectEventTemplateByLocalIdAndMap(localId, mapNum, mapGroup)->script;
}

const u8 *GetObjectEventScriptPointerByObjectEventId(u8 objectEventId)
{
    return GetObjectEventScriptPointerByLocalIdAndMap(gObjectEvents[objectEventId].localId, gObjectEvents[objectEventId].mapNum, gObjectEvents[objectEventId].mapGroup);
}

static u16 GetObjectEventFlagIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    const struct ObjectEventTemplate *obj = GetObjectEventTemplateByLocalIdAndMap(localId, mapNum, mapGroup);
#ifdef UBFIX
    // BUG: The function may return NULL, and attempting to read from NULL may freeze the game using modern compilers.
    if (obj == NULL)
        return 0;
#endif // UBFIX
    return obj->flagId;
}

static u16 GetObjectEventFlagIdByObjectEventId(u8 objectEventId)
{
    return GetObjectEventFlagIdByLocalIdAndMap(gObjectEvents[objectEventId].localId, gObjectEvents[objectEventId].mapNum, gObjectEvents[objectEventId].mapGroup);
}

static u8 UNUSED GetObjectTrainerTypeByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;

    if (TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        return 0xFF;

    return gObjectEvents[objectEventId].trainerType;
}

static u8 UNUSED GetObjectTrainerTypeByObjectEventId(u8 objectEventId)
{
    return gObjectEvents[objectEventId].trainerType;
}

// Unused
u8 GetObjectEventBerryTreeIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;

    if (TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        return 0xFF;

    return gObjectEvents[objectEventId].trainerRange_berryTreeId;
}

u8 GetObjectEventBerryTreeId(u8 objectEventId)
{
    return gObjectEvents[objectEventId].trainerRange_berryTreeId;
}

static const struct ObjectEventTemplate *GetObjectEventTemplateByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup)
{
    const struct ObjectEventTemplate *templates;
    const struct MapHeader *mapHeader;
    u8 count;

    if (gSaveBlock1Ptr->location.mapNum == mapNum && gSaveBlock1Ptr->location.mapGroup == mapGroup)
    {
        templates = gSaveBlock1Ptr->objectEventTemplates;
        count = gMapHeader.events->objectEventCount;
    }
    else
    {
        mapHeader = Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum);
        templates = mapHeader->events->objectEvents;
        count = mapHeader->events->objectEventCount;
    }
    return FindObjectEventTemplateByLocalId(localId, templates, count);
}

static const struct ObjectEventTemplate *FindObjectEventTemplateByLocalId(u8 localId, const struct ObjectEventTemplate *templates, u8 count)
{
    u8 i;

    for (i = 0; i < count; i++)
    {
        if (templates[i].localId == localId)
            return &templates[i];
    }
    return NULL;
}

struct ObjectEventTemplate *GetBaseTemplateForObjectEvent(const struct ObjectEvent *objectEvent)
{
    int i;

    if (objectEvent->mapNum != gSaveBlock1Ptr->location.mapNum
     || objectEvent->mapGroup != gSaveBlock1Ptr->location.mapGroup)
        return NULL;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (objectEvent->localId == gSaveBlock1Ptr->objectEventTemplates[i].localId)
            return &gSaveBlock1Ptr->objectEventTemplates[i];
    }
    return NULL;
}

void OverrideTemplateCoordsForObjectEvent(const struct ObjectEvent *objectEvent)
{
    struct ObjectEventTemplate *objectEventTemplate;

    objectEventTemplate = GetBaseTemplateForObjectEvent(objectEvent);
    if (objectEventTemplate != NULL)
    {
        objectEventTemplate->x = objectEvent->currentCoords.x - MAP_OFFSET;
        objectEventTemplate->y = objectEvent->currentCoords.y - MAP_OFFSET;
    }
}

static void OverrideObjectEventTemplateScript(const struct ObjectEvent *objectEvent, const u8 *script)
{
    struct ObjectEventTemplate *objectEventTemplate;

    objectEventTemplate = GetBaseTemplateForObjectEvent(objectEvent);
    if (objectEventTemplate)
        objectEventTemplate->script = script;
}

void TryOverrideTemplateCoordsForObjectEvent(const struct ObjectEvent *objectEvent, u8 movementType)
{
    struct ObjectEventTemplate *objectEventTemplate;

    objectEventTemplate = GetBaseTemplateForObjectEvent(objectEvent);
    if (objectEventTemplate != NULL)
        objectEventTemplate->movementType = movementType;
}

void TryOverrideObjectEventTemplateCoords(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        OverrideTemplateCoordsForObjectEvent(&gObjectEvents[objectEventId]);
}

void OverrideSecretBaseDecorationSpriteScript(u8 localId, u8 mapNum, u8 mapGroup, u8 decorationCategory)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
    {
        switch (decorationCategory)
        {
        case DECORCAT_DOLL:
            OverrideObjectEventTemplateScript(&gObjectEvents[objectEventId], SecretBase_EventScript_DollInteract);
            break;
        case DECORCAT_CUSHION:
            OverrideObjectEventTemplateScript(&gObjectEvents[objectEventId], SecretBase_EventScript_CushionInteract);
            break;
        }
    }
}

void InitObjectEventPalettes(u8 reflectionType)
{
    FreeAndReserveObjectSpritePalettes();
    sCurrentSpecialObjectPaletteTag = OBJ_EVENT_PAL_TAG_NONE;
    sCurrentReflectionType = reflectionType;
    if (reflectionType == 1)
    {
        PatchObjectPaletteRange(sObjectPaletteTagSets[sCurrentReflectionType], PALSLOT_PLAYER, PALSLOT_NPC_4 + 1);
        gReservedSpritePaletteCount = 8;
    }
    else
    {
        PatchObjectPaletteRange(sObjectPaletteTagSets[sCurrentReflectionType], PALSLOT_PLAYER, PALSLOT_NPC_4_REFLECTION + 1);
    }
}

u16 GetObjectPaletteTag(u8 palSlot)
{
    u8 i;

    if (palSlot < PALSLOT_NPC_SPECIAL)
        return sObjectPaletteTagSets[sCurrentReflectionType][palSlot];

    for (i = 0; sSpecialObjectReflectionPaletteSets[i].tag != OBJ_EVENT_PAL_TAG_NONE; i++)
    {
        if (sSpecialObjectReflectionPaletteSets[i].tag == sCurrentSpecialObjectPaletteTag)
            return sSpecialObjectReflectionPaletteSets[i].data[sCurrentReflectionType];
    }
    return OBJ_EVENT_PAL_TAG_NONE;
}

movement_type_empty_callback(MovementType_None)
movement_type_def(MovementType_WanderAround, gMovementTypeFuncs_WanderAround)

bool8 MovementType_WanderAround_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_WanderAround_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_WanderAround_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (!ObjectEventExecSingleMovementAction(objectEvent, sprite))
        return FALSE;
    SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
    sprite->sTypeFuncId = 3;
    return TRUE;
}

bool8 MovementType_WanderAround_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_WanderAround_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[4];
    u8 chosenDirection;

    memcpy(directions, gStandardDirections, sizeof directions);
    chosenDirection = directions[Random() & 3];
    SetObjectEventDirection(objectEvent, chosenDirection);
    sprite->sTypeFuncId = 5;
    if (GetCollisionInDirection(objectEvent, chosenDirection))
        sprite->sTypeFuncId = 1;

    return TRUE;
}

bool8 MovementType_WanderAround_Step5(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkNormalMovementAction(objectEvent->movementDirection));
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 6;
    return TRUE;
}

bool8 MovementType_WanderAround_Step6(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

bool8 ObjectEventIsTrainerAndCloseToPlayer(struct ObjectEvent *objectEvent)
{
    s16 playerX;
    s16 playerY;
    s16 objX;
    s16 objY;
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;

    if (!TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH))
        return FALSE;

    if (objectEvent->trainerType != TRAINER_TYPE_NORMAL && objectEvent->trainerType != TRAINER_TYPE_BURIED)
        return FALSE;

    PlayerGetDestCoords(&playerX, &playerY);
    objX = objectEvent->currentCoords.x;
    objY = objectEvent->currentCoords.y;
    minX = objX - objectEvent->trainerRange_berryTreeId;
    minY = objY - objectEvent->trainerRange_berryTreeId;
    maxX = objX + objectEvent->trainerRange_berryTreeId;
    maxY = objY + objectEvent->trainerRange_berryTreeId;
    if (minX > playerX || maxX < playerX
     || minY > playerY || maxY < playerY)
        return FALSE;

    return TRUE;
}

u8 GetVectorDirection(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    if (absdx > absdy)
    {
        direction = DIR_EAST;
        if (dx < 0)
            direction = DIR_WEST;
    }
    else
    {
        direction = DIR_SOUTH;
        if (dy < 0)
            direction = DIR_NORTH;
    }
    return direction;
}

u8 GetLimitedVectorDirection_SouthNorth(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = DIR_SOUTH;
    if (dy < 0)
        direction = DIR_NORTH;
    return direction;
}

u8 GetLimitedVectorDirection_WestEast(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = DIR_EAST;
    if (dx < 0)
        direction = DIR_WEST;
    return direction;
}

u8 GetLimitedVectorDirection_WestNorth(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_SOUTH)
    {
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
        if (direction == DIR_EAST)
            direction = DIR_NORTH;
    }
    else if (direction == DIR_EAST)
    {
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
        if (direction == DIR_SOUTH)
            direction = DIR_NORTH;
    }
    return direction;
}

u8 GetLimitedVectorDirection_EastNorth(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_SOUTH)
    {
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
        if (direction == DIR_WEST)
            direction = DIR_NORTH;
    }
    else if (direction == DIR_WEST)
    {
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
        if (direction == DIR_SOUTH)
            direction = DIR_NORTH;
    }
    return direction;
}

u8 GetLimitedVectorDirection_WestSouth(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_NORTH)
    {
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
        if (direction == DIR_EAST)
            direction = DIR_SOUTH;
    }
    else if (direction == DIR_EAST)
    {
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
        if (direction == DIR_NORTH)
            direction = DIR_SOUTH;
    }
    return direction;
}

u8 GetLimitedVectorDirection_EastSouth(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_NORTH)
    {
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
        if (direction == DIR_WEST)
            direction = DIR_SOUTH;
    }
    else if (direction == DIR_WEST)
    {
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
        if (direction == DIR_NORTH)
            direction = DIR_SOUTH;
    }
    return direction;
}

u8 GetLimitedVectorDirection_SouthNorthWest(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_EAST)
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
    return direction;
}

u8 GetLimitedVectorDirection_SouthNorthEast(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_WEST)
        direction = GetLimitedVectorDirection_SouthNorth(dx, dy, absdx, absdy);
    return direction;
}

u8 GetLimitedVectorDirection_NorthWestEast(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_SOUTH)
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
    return direction;
}

u8 GetLimitedVectorDirection_SouthWestEast(s16 dx, s16 dy, s16 absdx, s16 absdy)
{
    u8 direction;

    direction = GetVectorDirection(dx, dy, absdx, absdy);
    if (direction == DIR_NORTH)
        direction = GetLimitedVectorDirection_WestEast(dx, dy, absdx, absdy);
    return direction;
}

u8 TryGetTrainerEncounterDirection(struct ObjectEvent *objectEvent, u8 movementType)
{
    s16 dx, dy;
    s16 absdx, absdy;

    if (!ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
        return DIR_NONE;

    PlayerGetDestCoords(&dx, &dy);
    dx -= objectEvent->currentCoords.x;
    dy -= objectEvent->currentCoords.y;
    absdx = dx;
    absdy = dy;

    if (absdx < 0)
        absdx = -absdx;
    if (absdy < 0)
        absdy = -absdy;

    return gGetVectorDirectionFuncs[movementType](dx, dy, absdx, absdy);
}

movement_type_def(MovementType_LookAround, gMovementTypeFuncs_LookAround)

bool8 MovementType_LookAround_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_LookAround_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_LookAround_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_LookAround_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_LookAround_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[4];
    memcpy(directions, gStandardDirections, sizeof directions);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_ANY);
    if (direction == DIR_NONE)
        direction = directions[Random() & 3];

    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_WanderUpAndDown, gMovementTypeFuncs_WanderUpAndDown)

bool8 MovementType_WanderUpAndDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_WanderUpAndDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_WanderUpAndDown_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (!ObjectEventExecSingleMovementAction(objectEvent, sprite))
        return FALSE;

    SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
    sprite->sTypeFuncId = 3;
    return TRUE;
}

bool8 MovementType_WanderUpAndDown_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_WanderUpAndDown_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gUpAndDownDirections, sizeof directions);
    direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 5;
    if (GetCollisionInDirection(objectEvent, direction))
        sprite->sTypeFuncId = 1;

    return TRUE;
}

bool8 MovementType_WanderUpAndDown_Step5(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkNormalMovementAction(objectEvent->movementDirection));
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 6;
    return TRUE;
}

bool8 MovementType_WanderUpAndDown_Step6(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

movement_type_def(MovementType_WanderLeftAndRight, gMovementTypeFuncs_WanderLeftAndRight)

bool8 MovementType_WanderLeftAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_WanderLeftAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_WanderLeftAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (!ObjectEventExecSingleMovementAction(objectEvent, sprite))
        return FALSE;

    SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
    sprite->sTypeFuncId = 3;
    return TRUE;
}

bool8 MovementType_WanderLeftAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_WanderLeftAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gLeftAndRightDirections, sizeof directions);
    direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 5;
    if (GetCollisionInDirection(objectEvent, direction))
        sprite->sTypeFuncId = 1;

    return TRUE;
}

bool8 MovementType_WanderLeftAndRight_Step5(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkNormalMovementAction(objectEvent->movementDirection));
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 6;
    return TRUE;
}

bool8 MovementType_WanderLeftAndRight_Step6(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

movement_type_def(MovementType_FaceDirection, gMovementTypeFuncs_FaceDirection)

bool8 MovementType_FaceDirection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDirection_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        sprite->sTypeFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDirection_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->singleMovementActive = FALSE;
    return FALSE;
}

static bool8 ObjectEventCB2_BerryTree(struct ObjectEvent *objectEvent, struct Sprite *sprite);
extern bool8 (*const gMovementTypeFuncs_BerryTreeGrowth[])(struct ObjectEvent *objectEvent, struct Sprite *sprite);

enum {
    BERRYTREEFUNC_NORMAL,
    BERRYTREEFUNC_MOVE,
    BERRYTREEFUNC_SPARKLE_START,
    BERRYTREEFUNC_SPARKLE,
    BERRYTREEFUNC_SPARKLE_END,
};

#define sTimer          data[2]
#define sBerryTreeFlags data[7]

#define BERRY_FLAG_SET_GFX     (1 << 0)
#define BERRY_FLAG_SPARKLING   (1 << 1)
#define BERRY_FLAG_JUST_PICKED (1 << 2)

void MovementType_BerryTreeGrowth(struct Sprite *sprite)
{
    struct ObjectEvent *objectEvent;

    objectEvent = &gObjectEvents[sprite->sObjEventId];
    if (!(sprite->sBerryTreeFlags & BERRY_FLAG_SET_GFX))
    {
        SetBerryTreeGraphics(objectEvent, sprite);
        sprite->sBerryTreeFlags |= BERRY_FLAG_SET_GFX;
    }
    UpdateObjectEventCurrentMovement(objectEvent, sprite, ObjectEventCB2_BerryTree);
}
static bool8 ObjectEventCB2_BerryTree(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return gMovementTypeFuncs_BerryTreeGrowth[sprite->sTypeFuncId](objectEvent, sprite);
}

// BERRYTREEFUNC_NORMAL
bool8 MovementType_BerryTreeGrowth_Normal(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 berryStage;
    ClearObjectEventMovement(objectEvent, sprite);
    objectEvent->invisible = TRUE;
    sprite->invisible = TRUE;
    berryStage = GetStageByBerryTreeId(objectEvent->trainerRange_berryTreeId);
    if (berryStage == BERRY_STAGE_NO_BERRY)
    {
        if (!(sprite->sBerryTreeFlags & BERRY_FLAG_JUST_PICKED) && sprite->animNum == BERRY_STAGE_FLOWERING)
        {
            gFieldEffectArguments[0] = objectEvent->currentCoords.x;
            gFieldEffectArguments[1] = objectEvent->currentCoords.y;
            gFieldEffectArguments[2] = sprite->subpriority - 1;
            gFieldEffectArguments[3] = sprite->oam.priority;
            FieldEffectStart(FLDEFF_BERRY_TREE_GROWTH_SPARKLE);
            sprite->animNum = berryStage;
        }
        return FALSE;
    }
    objectEvent->invisible = FALSE;
    sprite->invisible = FALSE;
    berryStage--;
    if (sprite->animNum != berryStage)
    {
        sprite->sTypeFuncId = BERRYTREEFUNC_SPARKLE_START;
        return TRUE;
    }
    SetBerryTreeGraphics(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
    sprite->sTypeFuncId = BERRYTREEFUNC_MOVE;
    return TRUE;
}

// BERRYTREEFUNC_MOVE
bool8 MovementType_BerryTreeGrowth_Move(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        sprite->sTypeFuncId = BERRYTREEFUNC_NORMAL;
        return TRUE;
    }
    return FALSE;
}

// BERRYTREEFUNC_SPARKLE_START
bool8 MovementType_BerryTreeGrowth_SparkleStart(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = BERRYTREEFUNC_SPARKLE;
    sprite->sTimer = 0;
    sprite->sBerryTreeFlags |= BERRY_FLAG_SPARKLING;
    gFieldEffectArguments[0] = objectEvent->currentCoords.x;
    gFieldEffectArguments[1] = objectEvent->currentCoords.y;
    gFieldEffectArguments[2] = sprite->subpriority - 1;
    gFieldEffectArguments[3] = sprite->oam.priority;
    FieldEffectStart(FLDEFF_BERRY_TREE_GROWTH_SPARKLE);
    return TRUE;
}

// BERRYTREEFUNC_SPARKLE
bool8 MovementType_BerryTreeGrowth_Sparkle(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->sTimer++;
    objectEvent->invisible = (sprite->sTimer & 2) >> 1;
    sprite->animPaused = TRUE;
    if (sprite->sTimer > 64)
    {
        SetBerryTreeGraphics(objectEvent, sprite);
        sprite->sTypeFuncId = BERRYTREEFUNC_SPARKLE_END;
        sprite->sTimer = 0;
        return TRUE;
    }
    return FALSE;
}

// BERRYTREEFUNC_SPARKLE_END
bool8 MovementType_BerryTreeGrowth_SparkleEnd(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->sTimer++;
    objectEvent->invisible = (sprite->sTimer & 2) >> 1;
    sprite->animPaused = TRUE;
    if (sprite->sTimer > 64)
    {
        sprite->sTypeFuncId = BERRYTREEFUNC_NORMAL;
        sprite->sBerryTreeFlags &= ~BERRY_FLAG_SPARKLING;
        return TRUE;
    }
    return FALSE;
}

movement_type_def(MovementType_FaceDownAndUp, gMovementTypeFuncs_FaceDownAndUp)

bool8 MovementType_FaceDownAndUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownAndUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownAndUp_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndUp_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndUp_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gUpAndDownDirections, sizeof gUpAndDownDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_SOUTH);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceLeftAndRight, gMovementTypeFuncs_FaceLeftAndRight)

bool8 MovementType_FaceLeftAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceLeftAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceLeftAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysMedium[Random() % ARRAY_COUNT(sMovementDelaysMedium)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceLeftAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceLeftAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gLeftAndRightDirections, sizeof gLeftAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_EAST_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceUpAndLeft, gMovementTypeFuncs_FaceUpAndLeft)

bool8 MovementType_FaceUpAndLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceUpAndLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceUpAndLeft_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceUpAndLeft_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceUpAndLeft_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gUpAndLeftDirections, sizeof gUpAndLeftDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceUpAndRight, gMovementTypeFuncs_FaceUpAndRight)

bool8 MovementType_FaceUpAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceUpAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceUpAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceUpAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceUpAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gUpAndRightDirections, sizeof gUpAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_EAST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceDownAndLeft, gMovementTypeFuncs_FaceDownAndLeft)

bool8 MovementType_FaceDownAndLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownAndLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownAndLeft_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndLeft_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndLeft_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gDownAndLeftDirections, sizeof gDownAndLeftDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_SOUTH_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceDownAndRight, gMovementTypeFuncs_FaceDownAndRight)

bool8 MovementType_FaceDownAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[2];
    memcpy(directions, gDownAndRightDirections, sizeof gDownAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_SOUTH_EAST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 1];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceDownUpAndLeft, gMovementTypeFuncs_FaceDownUpAndLeft)

bool8 MovementType_FaceDownUpAndLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownUpAndLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownUpAndLeft_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownUpAndLeft_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownUpAndLeft_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[4];
    memcpy(directions, gDownUpAndLeftDirections, sizeof gDownUpAndLeftDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_SOUTH_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 3];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceDownUpAndRight, gMovementTypeFuncs_FaceDownUpAndRight)

bool8 MovementType_FaceDownUpAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownUpAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownUpAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownUpAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownUpAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[4];
    memcpy(directions, gDownUpAndRightDirections, sizeof gDownUpAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_SOUTH_EAST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 3];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceUpRightAndLeft, gMovementTypeFuncs_FaceUpLeftAndRight)

bool8 MovementType_FaceUpLeftAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceUpLeftAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceUpLeftAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceUpLeftAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceUpLeftAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[4];
    memcpy(directions, gUpLeftAndRightDirections, sizeof gUpLeftAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_NORTH_EAST_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 3];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_FaceDownRightAndLeft, gMovementTypeFuncs_FaceDownLeftAndRight)

bool8 MovementType_FaceDownLeftAndRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_FaceDownLeftAndRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_FaceDownLeftAndRight_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, sMovementDelaysShort[Random() % ARRAY_COUNT(sMovementDelaysShort)]);
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 3;
    }
    return FALSE;
}

bool8 MovementType_FaceDownLeftAndRight_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
    {
        sprite->sTypeFuncId = 4;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_FaceDownLeftAndRight_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[4];
    memcpy(directions, gDownLeftAndRightDirections, sizeof gDownLeftAndRightDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_SOUTH_EAST_WEST);
    if (direction == DIR_NONE)
        direction = directions[Random() & 3];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_RotateCounterclockwise, gMovementTypeFuncs_RotateCounterclockwise)

bool8 MovementType_RotateCounterclockwise_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_RotateCounterclockwise_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, 48);
        sprite->sTypeFuncId = 2;
    }
    return FALSE;
}

bool8 MovementType_RotateCounterclockwise_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
        sprite->sTypeFuncId = 3;
    return FALSE;
}

bool8 MovementType_RotateCounterclockwise_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[5];
    memcpy(directions, gCounterclockwiseDirections, sizeof gCounterclockwiseDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_ANY);
    if (direction == DIR_NONE)
        direction = directions[objectEvent->facingDirection];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 0;
    return TRUE;
}

movement_type_def(MovementType_RotateClockwise, gMovementTypeFuncs_RotateClockwise)

bool8 MovementType_RotateClockwise_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_RotateClockwise_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        SetMovementDelay(sprite, 48);
        sprite->sTypeFuncId = 2;
    }
    return FALSE;
}

bool8 MovementType_RotateClockwise_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (WaitForMovementDelay(sprite) || ObjectEventIsTrainerAndCloseToPlayer(objectEvent))
        sprite->sTypeFuncId = 3;
    return FALSE;
}

bool8 MovementType_RotateClockwise_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;
    u8 directions[5];
    memcpy(directions, gClockwiseDirections, sizeof gClockwiseDirections);
    direction = TryGetTrainerEncounterDirection(objectEvent, RUNFOLLOW_ANY);
    if (direction == DIR_NONE)
        direction = directions[objectEvent->facingDirection];
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 0;
    return TRUE;
}

movement_type_def(MovementType_WalkBackAndForth, gMovementTypeFuncs_WalkBackAndForth)

bool8 MovementType_WalkBackAndForth_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_WalkBackAndForth_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 direction;

    direction = gInitialMovementTypeFacingDirections[objectEvent->movementType];
    if (objectEvent->directionSequenceIndex)
        direction = GetOppositeDirection(direction);
    SetObjectEventDirection(objectEvent, direction);
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_WalkBackAndForth_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    bool8 collision;
    u8 movementActionId;

    if (objectEvent->directionSequenceIndex && objectEvent->initialCoords.x == objectEvent->currentCoords.x && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
    {
        objectEvent->directionSequenceIndex = 0;
        SetObjectEventDirection(objectEvent, GetOppositeDirection(objectEvent->movementDirection));
    }
    collision = GetCollisionInDirection(objectEvent, objectEvent->movementDirection);
    movementActionId = GetWalkNormalMovementAction(objectEvent->movementDirection);
    if (collision == COLLISION_OUTSIDE_RANGE)
    {
        objectEvent->directionSequenceIndex++;
        SetObjectEventDirection(objectEvent, GetOppositeDirection(objectEvent->movementDirection));
        movementActionId = GetWalkNormalMovementAction(objectEvent->movementDirection);
        collision = GetCollisionInDirection(objectEvent, objectEvent->movementDirection);
    }

    if (collision)
        movementActionId = GetWalkInPlaceNormalMovementAction(objectEvent->facingDirection);

    ObjectEventSetSingleMovement(objectEvent, sprite, movementActionId);
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 3;
    return TRUE;
}

bool8 MovementType_WalkBackAndForth_Step3(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

bool8 MovementType_WalkSequence_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MoveNextDirectionInSequence(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 *route)
{
    u8 collision;
    u8 movementActionId;

    if (objectEvent->directionSequenceIndex == 3 && objectEvent->initialCoords.x == objectEvent->currentCoords.x && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 0;

    SetObjectEventDirection(objectEvent, route[objectEvent->directionSequenceIndex]);
    movementActionId = GetWalkNormalMovementAction(objectEvent->movementDirection);
    collision = GetCollisionInDirection(objectEvent, objectEvent->movementDirection);
    if (collision == COLLISION_OUTSIDE_RANGE)
    {
        objectEvent->directionSequenceIndex++;
        SetObjectEventDirection(objectEvent, route[objectEvent->directionSequenceIndex]);
        movementActionId = GetWalkNormalMovementAction(objectEvent->movementDirection);
        collision = GetCollisionInDirection(objectEvent, objectEvent->movementDirection);
    }

    if (collision)
        movementActionId = GetWalkInPlaceNormalMovementAction(objectEvent->facingDirection);

    ObjectEventSetSingleMovement(objectEvent, sprite, movementActionId);
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 MovementType_WalkSequence_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

movement_type_def(MovementType_WalkSequenceUpRightLeftDown, gMovementTypeFuncs_WalkSequenceUpRightLeftDown)

u8 MovementType_WalkSequenceUpRightLeftDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpRightLeftDownDirections)];
    memcpy(directions, gUpRightLeftDownDirections, sizeof(gUpRightLeftDownDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightLeftDownUp, gMovementTypeFuncs_WalkSequenceRightLeftDownUp)

u8 MovementType_WalkSequenceRightLeftDownUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightLeftDownUpDirections)];
    memcpy(directions, gRightLeftDownUpDirections, sizeof(gRightLeftDownUpDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownUpRightLeft, gMovementTypeFuncs_WalkSequenceDownUpRightLeft)

u8 MovementType_WalkSequenceDownUpRightLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gDownUpRightLeftDirections)];
    memcpy(directions, gDownUpRightLeftDirections, sizeof(gDownUpRightLeftDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftDownUpRight, gMovementTypeFuncs_WalkSequenceLeftDownUpRight)

u8 MovementType_WalkSequenceLeftDownUpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftDownUpRightDirections)];
    memcpy(directions, gLeftDownUpRightDirections, sizeof(gLeftDownUpRightDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceUpLeftRightDown, gMovementTypeFuncs_WalkSequenceUpLeftRightDown)

u8 MovementType_WalkSequenceUpLeftRightDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpLeftRightDownDirections)];
    memcpy(directions, gUpLeftRightDownDirections, sizeof(gUpLeftRightDownDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftRightDownUp, gMovementTypeFuncs_WalkSequenceLeftRightDownUp)

u8 MovementType_WalkSequenceLeftRightDownUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftRightDownUpDirections)];
    memcpy(directions, gLeftRightDownUpDirections, sizeof(gLeftRightDownUpDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownUpLeftRight, gMovementTypeFuncs_WalkSequenceDownUpLeftRight)

u8 MovementType_WalkSequenceDownUpLeftRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gStandardDirections)];
    memcpy(directions, gStandardDirections, sizeof(gStandardDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightDownUpLeft, gMovementTypeFuncs_WalkSequenceRightDownUpLeft)

u8 MovementType_WalkSequenceRightDownUpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightDownUpLeftDirections)];
    memcpy(directions, gRightDownUpLeftDirections, sizeof(gRightDownUpLeftDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftUpDownRight, gMovementTypeFuncs_WalkSequenceLeftUpDownRight)

u8 MovementType_WalkSequenceLeftUpDownRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftUpDownRightDirections)];
    memcpy(directions, gLeftUpDownRightDirections, sizeof(gLeftUpDownRightDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceUpDownRightLeft, gMovementTypeFuncs_WalkSequenceUpDownRightLeft)

u8 MovementType_WalkSequenceUpDownRightLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpDownRightLeftDirections)];
    memcpy(directions, gUpDownRightLeftDirections, sizeof(gUpDownRightLeftDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightLeftUpDown, gMovementTypeFuncs_WalkSequenceRightLeftUpDown)

u8 MovementType_WalkSequenceRightLeftUpDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightLeftUpDownDirections)];
    memcpy(directions, gRightLeftUpDownDirections, sizeof(gRightLeftUpDownDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownRightLeftUp, gMovementTypeFuncs_WalkSequenceDownRightLeftUp)

u8 MovementType_WalkSequenceDownRightLeftUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gDownRightLeftUpDirections)];
    memcpy(directions, gDownRightLeftUpDirections, sizeof(gDownRightLeftUpDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightUpDownLeft, gMovementTypeFuncs_WalkSequenceRightUpDownLeft)

u8 MovementType_WalkSequenceRightUpDownLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightUpDownLeftDirections)];
    memcpy(directions, gRightUpDownLeftDirections, sizeof(gRightUpDownLeftDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceUpDownLeftRight, gMovementTypeFuncs_WalkSequenceUpDownLeftRight)

u8 MovementType_WalkSequenceUpDownLeftRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpDownLeftRightDirections)];
    memcpy(directions, gUpDownLeftRightDirections, sizeof(gUpDownLeftRightDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftRightUpDown, gMovementTypeFuncs_WalkSequenceLeftRightUpDown)

u8 MovementType_WalkSequenceLeftRightUpDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftRightUpDownDirections)];
    memcpy(directions, gLeftRightUpDownDirections, sizeof(gLeftRightUpDownDirections));
    if (objectEvent->directionSequenceIndex == 1 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 2;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownLeftRightUp, gMovementTypeFuncs_WalkSequenceDownLeftRightUp)

u8 MovementType_WalkSequenceDownLeftRightUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gDownLeftRightUpDirections)];
    memcpy(directions, gDownLeftRightUpDirections, sizeof(gDownLeftRightUpDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceUpLeftDownRight, gMovementTypeFuncs_WalkSequenceUpLeftDownRight)

u8 MovementType_WalkSequenceUpLeftDownRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpLeftDownRightDirections)];
    memcpy(directions, gUpLeftDownRightDirections, sizeof(gUpLeftDownRightDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownRightUpLeft, gMovementTypeFuncs_WalkSequenceDownRightUpLeft)

u8 MovementType_WalkSequenceDownRightUpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gDownRightUpLeftDirections)];
    memcpy(directions, gDownRightUpLeftDirections, sizeof(gDownRightUpLeftDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftDownRightUp, gMovementTypeFuncs_WalkSequenceLeftDownRightUp)

u8 MovementType_WalkSequenceLeftDownRightUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftDownRightUpDirections)];
    memcpy(directions, gLeftDownRightUpDirections, sizeof(gLeftDownRightUpDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightUpLeftDown, gMovementTypeFuncs_WalkSequenceRightUpLeftDown)

u8 MovementType_WalkSequenceRightUpLeftDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightUpLeftDownDirections)];
    memcpy(directions, gRightUpLeftDownDirections, sizeof(gRightUpLeftDownDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceUpRightDownLeft, gMovementTypeFuncs_WalkSequenceUpRightDownLeft)

u8 MovementType_WalkSequenceUpRightDownLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gUpRightDownLeftDirections)];
    memcpy(directions, gUpRightDownLeftDirections, sizeof(gUpRightDownLeftDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceDownLeftUpRight, gMovementTypeFuncs_WalkSequenceDownLeftUpRight)

u8 MovementType_WalkSequenceDownLeftUpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gDownLeftUpRightDirections)];
    memcpy(directions, gDownLeftUpRightDirections, sizeof(gDownLeftUpRightDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.y == objectEvent->currentCoords.y)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceLeftUpRightDown, gMovementTypeFuncs_WalkSequenceLeftUpRightDown)

u8 MovementType_WalkSequenceLeftUpRightDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gLeftUpRightDownDirections)];
    memcpy(directions, gLeftUpRightDownDirections, sizeof(gLeftUpRightDownDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_WalkSequenceRightDownLeftUp, gMovementTypeFuncs_WalkSequenceRightDownLeftUp)

u8 MovementType_WalkSequenceRightDownLeftUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 directions[sizeof(gRightDownLeftUpDirections)];
    memcpy(directions, gRightDownLeftUpDirections, sizeof(gRightDownLeftUpDirections));
    if (objectEvent->directionSequenceIndex == 2 && objectEvent->initialCoords.x == objectEvent->currentCoords.x)
        objectEvent->directionSequenceIndex = 3;

    return MoveNextDirectionInSequence(objectEvent, sprite, directions);
}

movement_type_def(MovementType_CopyPlayer, gMovementTypeFuncs_CopyPlayer)

bool8 MovementType_CopyPlayer_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    if (objectEvent->directionSequenceIndex == 0)
        objectEvent->directionSequenceIndex = GetPlayerFacingDirection();
    sprite->sTypeFuncId = 1;
    return TRUE;
}

bool8 MovementType_CopyPlayer_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (gObjectEvents[gPlayerAvatar.objectEventId].movementActionId == MOVEMENT_ACTION_NONE || gPlayerAvatar.tileTransitionState == T_TILE_CENTER)
        return FALSE;

    return gCopyPlayerMovementFuncs[PlayerGetCopyableMovement()](objectEvent, sprite, GetPlayerMovementDirection(), NULL);
}

bool8 MovementType_CopyPlayer_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        objectEvent->singleMovementActive = FALSE;
        sprite->sTypeFuncId = 1;
    }
    return FALSE;
}

bool8 CopyablePlayerMovement_None(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    return FALSE;
}

bool8 CopyablePlayerMovement_FaceDirection(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, playerDirection)));
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_WalkNormal(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    if (ObjectEventIsFarawayIslandMew(objectEvent))
    {
        direction = GetMewMoveDirection();
        if (direction == DIR_NONE)
        {
            direction = playerDirection;
            direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
            ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
            ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));
            objectEvent->singleMovementActive = TRUE;
            sprite->sTypeFuncId = 2;
            return TRUE;
        }
    }
    else
    {
        direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    }
    ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkNormalMovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_WalkFast(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkFastMovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_WalkFaster(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkFasterMovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_Slide(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetSlideMovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_JumpInPlace(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetJumpInPlaceMovementAction(direction));
    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_Jump(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    ObjectEventMoveDestCoords(objectEvent, direction, &x, &y);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetJumpMovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

bool8 CopyablePlayerMovement_Jump2(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 playerDirection, bool8 tileCallback(u8))
{
    u32 direction;
    s16 x;
    s16 y;

    direction = playerDirection;
    direction = GetCopyDirection(gInitialMovementTypeFacingDirections[objectEvent->movementType], objectEvent->directionSequenceIndex, direction);
    x = objectEvent->currentCoords.x;
    y = objectEvent->currentCoords.y;
    MoveCoordsInDirection(direction, &x, &y, 2, 2);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetJump2MovementAction(direction));

    if (GetCollisionAtCoords(objectEvent, x, y, direction) || (tileCallback != NULL && !tileCallback(MapGridGetMetatileBehaviorAt(x, y))))
        ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(direction));

    objectEvent->singleMovementActive = TRUE;
    sprite->sTypeFuncId = 2;
    return TRUE;
}

movement_type_def(MovementType_CopyPlayerInGrass, gMovementTypeFuncs_CopyPlayerInGrass)

bool8 MovementType_CopyPlayerInGrass_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (gObjectEvents[gPlayerAvatar.objectEventId].movementActionId == MOVEMENT_ACTION_NONE || gPlayerAvatar.tileTransitionState == T_TILE_CENTER)
        return FALSE;

    return gCopyPlayerMovementFuncs[PlayerGetCopyableMovement()](objectEvent, sprite, GetPlayerMovementDirection(), MetatileBehavior_IsPokeGrass);
}

void MovementType_TreeDisguise(struct Sprite *sprite)
{
    struct ObjectEvent *objectEvent;

    objectEvent = &gObjectEvents[sprite->sObjEventId];
    if (objectEvent->directionSequenceIndex == 0 || (objectEvent->directionSequenceIndex == 1 && !sprite->data[7]))
    {
        ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
        objectEvent->fieldEffectSpriteId = FieldEffectStart(FLDEFF_TREE_DISGUISE);
        objectEvent->directionSequenceIndex = 1;
        sprite->data[7]++;
    }
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->sObjEventId], sprite, MovementType_Disguise_Callback);
}

static bool8 MovementType_Disguise_Callback(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    return FALSE;
}

void MovementType_MountainDisguise(struct Sprite *sprite)
{
    struct ObjectEvent *objectEvent;

    objectEvent = &gObjectEvents[sprite->sObjEventId];
    if (objectEvent->directionSequenceIndex == 0 || (objectEvent->directionSequenceIndex == 1 && !sprite->data[7]))
    {
        ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
        objectEvent->fieldEffectSpriteId = FieldEffectStart(FLDEFF_MOUNTAIN_DISGUISE);
        objectEvent->directionSequenceIndex = 1;
        sprite->data[7]++;
    }
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->sObjEventId], sprite, MovementType_Disguise_Callback);
}

void MovementType_Buried(struct Sprite *sprite)
{
    if (!sprite->data[7])
    {
        gObjectEvents[sprite->sObjEventId].fixedPriority = TRUE;
        sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
        sprite->oam.priority = 3;
        sprite->data[7]++;
    }
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->sObjEventId], sprite, MovementType_Buried_Callback);
}

static bool8 MovementType_Buried_Callback(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return gMovementTypeFuncs_Buried[sprite->sTypeFuncId](objectEvent, sprite);
}

bool8 MovementType_Buried_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    return FALSE;
}

bool8 MovementType_MoveInPlace_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
        sprite->sTypeFuncId = 0;
    return FALSE;
}

movement_type_def(MovementType_WalkInPlace, gMovementTypeFuncs_WalkInPlace)

bool8 MovementType_WalkInPlace_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkInPlaceNormalMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_WalkSlowlyInPlace, gMovementTypeFuncs_WalkSlowlyInPlace)

bool8 MovementType_WalkSlowlyInPlace_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkInPlaceSlowMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_JogInPlace, gMovementTypeFuncs_JogInPlace)

bool8 MovementType_JogInPlace_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkInPlaceFastMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_RunInPlace, gMovementTypeFuncs_RunInPlace)

bool8 MovementType_RunInPlace_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetWalkInPlaceFasterMovementAction(objectEvent->facingDirection));
    sprite->sTypeFuncId = 1;
    return TRUE;
}

movement_type_def(MovementType_Invisible, gMovementTypeFuncs_Invisible)

bool8 MovementType_Invisible_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ClearObjectEventMovement(objectEvent, sprite);
    ObjectEventSetSingleMovement(objectEvent, sprite, GetFaceDirectionMovementAction(objectEvent->facingDirection));
    objectEvent->invisible = TRUE;
    sprite->sTypeFuncId = 1;
    return TRUE;
}
bool8 MovementType_Invisible_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (ObjectEventExecSingleMovementAction(objectEvent, sprite))
    {
        sprite->sTypeFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementType_Invisible_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->singleMovementActive = FALSE;
    return FALSE;
}

static void ClearObjectEventMovement(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->singleMovementActive = FALSE;
    objectEvent->heldMovementActive = FALSE;
    objectEvent->heldMovementFinished = FALSE;
    objectEvent->movementActionId = MOVEMENT_ACTION_NONE;
    sprite->sTypeFuncId = 0;
}

u8 GetFaceDirectionAnimNum(u8 direction)
{
    return sFaceDirectionAnimNums[direction];
}

u8 GetMoveDirectionAnimNum(u8 direction)
{
    return sMoveDirectionAnimNums[direction];
}

u8 GetMoveDirectionFastAnimNum(u8 direction)
{
    return sMoveDirectionFastAnimNums[direction];
}

u8 GetMoveDirectionFasterAnimNum(u8 direction)
{
    return sMoveDirectionFasterAnimNums[direction];
}

u8 GetMoveDirectionFastestAnimNum(u8 direction)
{
    return sMoveDirectionFastestAnimNums[direction];
}

u8 GetJumpSpecialDirectionAnimNum(u8 direction)
{
    return sJumpSpecialDirectionAnimNums[direction];
}

u8 GetAcroWheelieDirectionAnimNum(u8 direction)
{
    return sAcroWheelieDirectionAnimNums[direction];
}

u8 GetAcroUnusedDirectionAnimNum(u8 direction)
{
    return sAcroUnusedDirectionAnimNums[direction];
}

u8 GetAcroEndWheelieDirectionAnimNum(u8 direction)
{
    return sAcroEndWheelieDirectionAnimNums[direction];
}

u8 GetAcroUnusedActionDirectionAnimNum(u8 direction)
{
    return sAcroUnusedActionDirectionAnimNums[direction];
}

u8 GetAcroWheeliePedalDirectionAnimNum(u8 direction)
{
    return sAcroWheeliePedalDirectionAnimNums[direction];
}

u8 GetFishingDirectionAnimNum(u8 direction)
{
    return sFishingDirectionAnimNums[direction];
}

u8 GetFishingNoCatchDirectionAnimNum(u8 direction)
{
    return sFishingNoCatchDirectionAnimNums[direction];
}

u8 GetFishingBiteDirectionAnimNum(u8 direction)
{
    return sFishingBiteDirectionAnimNums[direction];
}

u8 GetRunningDirectionAnimNum(u8 direction)
{
    return sRunningDirectionAnimNums[direction];
}

static const struct StepAnimTable *GetStepAnimTable(const union AnimCmd *const *anims)
{
    const struct StepAnimTable *stepTable;

    for (stepTable = sStepAnimTables; stepTable->anims != NULL; stepTable++)
    {
        if (stepTable->anims == anims)
            return stepTable;
    }
    return NULL;
}

void SetStepAnimHandleAlternation(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 animNum)
{
    const struct StepAnimTable *stepTable;

    if (!objectEvent->inanimate)
    {
        sprite->animNum = animNum;
        stepTable = GetStepAnimTable(sprite->anims);
        if (stepTable != NULL)
        {
            if (sprite->animCmdIndex == stepTable->animPos[0])
                sprite->animCmdIndex  = stepTable->animPos[3];
            else if (sprite->animCmdIndex == stepTable->animPos[1])
                sprite->animCmdIndex = stepTable->animPos[2];
        }
        SeekSpriteAnim(sprite, sprite->animCmdIndex);
    }
}

void SetStepAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 animNum)
{
    const struct StepAnimTable *stepTable;

    if (!objectEvent->inanimate)
    {
        u8 animPos;

        sprite->animNum = animNum;
        stepTable = GetStepAnimTable(sprite->anims);
        if (stepTable != NULL)
        {
            animPos = stepTable->animPos[1];
            if (sprite->animCmdIndex <= stepTable->animPos[0])
                animPos = stepTable->animPos[0];

            SeekSpriteAnim(sprite, animPos);
        }
    }
}

u8 GetDirectionToFace(s16 x, s16 y, s16 targetX, s16 targetY)
{
    if (x > targetX)
        return DIR_WEST;

    if (x < targetX)
        return DIR_EAST;

    if (y > targetY)
        return DIR_NORTH;

    return DIR_SOUTH;
}

void SetTrainerMovementType(struct ObjectEvent *objectEvent, u8 movementType)
{
    objectEvent->movementType = movementType;
    objectEvent->directionSequenceIndex = 0;
    objectEvent->playerCopyableMovement = 0;
    gSprites[objectEvent->spriteId].callback = sMovementTypeCallbacks[movementType];
    gSprites[objectEvent->spriteId].sTypeFuncId = 0;
}

u8 GetTrainerFacingDirectionMovementType(u8 direction)
{
    return gTrainerFacingDirectionMovementTypes[direction];
}

static u8 GetCollisionInDirection(struct ObjectEvent *objectEvent, u8 direction)
{
    s16 x = objectEvent->currentCoords.x;
    s16 y = objectEvent->currentCoords.y;
    MoveCoords(direction, &x, &y);
    return GetCollisionAtCoords(objectEvent, x, y, direction);
}

u8 GetCollisionAtCoords(struct ObjectEvent *objectEvent, s16 x, s16 y, u32 dir)
{
    u8 direction = dir;
    if (IsCoordOutsideObjectEventMovementRange(objectEvent, x, y))
        return COLLISION_OUTSIDE_RANGE;
    else if (MapGridGetCollisionAt(x, y) || GetMapBorderIdAt(x, y) == CONNECTION_INVALID || IsMetatileDirectionallyImpassable(objectEvent, x, y, direction))
        return COLLISION_IMPASSABLE;
    else if (objectEvent->trackedByCamera && !CanCameraMoveInDirection(direction))
        return COLLISION_IMPASSABLE;
    else if (IsElevationMismatchAt(objectEvent->currentElevation, x, y))
        return COLLISION_ELEVATION_MISMATCH;
    else if (DoesObjectCollideWithObjectAt(objectEvent, x, y))
        return COLLISION_OBJECT_EVENT;
    return COLLISION_NONE;
}

u8 GetCollisionFlagsAtCoords(struct ObjectEvent *objectEvent, s16 x, s16 y, u8 direction)
{
    u8 flags = 0;

    if (IsCoordOutsideObjectEventMovementRange(objectEvent, x, y))
        flags |= 1 << (COLLISION_OUTSIDE_RANGE - 1);
    if (MapGridGetCollisionAt(x, y) || GetMapBorderIdAt(x, y) == CONNECTION_INVALID || IsMetatileDirectionallyImpassable(objectEvent, x, y, direction) || (objectEvent->trackedByCamera && !CanCameraMoveInDirection(direction)))
        flags |= 1 << (COLLISION_IMPASSABLE - 1);
    if (IsElevationMismatchAt(objectEvent->currentElevation, x, y))
        flags |= 1 << (COLLISION_ELEVATION_MISMATCH - 1);
    if (DoesObjectCollideWithObjectAt(objectEvent, x, y))
        flags |= 1 << (COLLISION_OBJECT_EVENT - 1);
    return flags;
}

static bool8 IsCoordOutsideObjectEventMovementRange(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    s16 left;
    s16 right;
    s16 top;
    s16 bottom;

    if (objectEvent->rangeX != 0)
    {
        left = objectEvent->initialCoords.x - objectEvent->rangeX;
        right = objectEvent->initialCoords.x + objectEvent->rangeX;

        if (left > x || right < x)
            return TRUE;
    }
    if (objectEvent->rangeY != 0)
    {
        top = objectEvent->initialCoords.y - objectEvent->rangeY;
        bottom = objectEvent->initialCoords.y + objectEvent->rangeY;

        if (top > y || bottom < y)
            return TRUE;
    }
    return FALSE;
}

static bool8 IsMetatileDirectionallyImpassable(struct ObjectEvent *objectEvent, s16 x, s16 y, u8 direction)
{
    if (gOppositeDirectionBlockedMetatileFuncs[direction - 1](objectEvent->currentMetatileBehavior)
        || gDirectionBlockedMetatileFuncs[direction - 1](MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;

    return FALSE;
}

static bool8 DoesObjectCollideWithObjectAt(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    u8 i;
    struct ObjectEvent *curObject;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        curObject = &gObjectEvents[i];
        if (curObject->active && curObject != objectEvent)
        {
            if ((curObject->currentCoords.x == x && curObject->currentCoords.y == y) || (curObject->previousCoords.x == x && curObject->previousCoords.y == y))
            {
                if (AreElevationsCompatible(objectEvent->currentElevation, curObject->currentElevation))
                    return TRUE;
            }
        }
    }
    return FALSE;
}

bool8 IsBerryTreeSparkling(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId)
        && gSprites[gObjectEvents[objectEventId].spriteId].sBerryTreeFlags & BERRY_FLAG_SPARKLING)
        return TRUE;

    return FALSE;
}

void SetBerryTreeJustPicked(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        gSprites[gObjectEvents[objectEventId].spriteId].sBerryTreeFlags |= BERRY_FLAG_JUST_PICKED;
}

#undef sTimer
#undef sBerryTreeFlags

void MoveCoords(u8 direction, s16 *x, s16 *y)
{
    *x += sDirectionToVectors[direction].x;
    *y += sDirectionToVectors[direction].y;
}

static void UNUSED MoveCoordsInMapCoordIncrement(u8 direction, s16 *x, s16 *y)
{
    *x += sDirectionToVectors[direction].x << 4;
    *y += sDirectionToVectors[direction].y << 4;
}

static void MoveCoordsInDirection(u32 dir, s16 *x, s16 *y, s16 deltaX, s16 deltaY)
{
    u8 direction = dir;
    s16 dx2 = (u16)deltaX;
    s16 dy2 = (u16)deltaY;
    if (sDirectionToVectors[direction].x > 0)
        *x += dx2;
    if (sDirectionToVectors[direction].x < 0)
        *x -= dx2;
    if (sDirectionToVectors[direction].y > 0)
        *y += dy2;
    if (sDirectionToVectors[direction].y < 0)
        *y -= dy2;
}

void GetMapCoordsFromSpritePos(s16 x, s16 y, s16 *destX, s16 *destY)
{
    *destX = (x - gSaveBlock1Ptr->pos.x) << 4;
    *destY = (y - gSaveBlock1Ptr->pos.y) << 4;
    *destX -= gTotalCameraPixelOffsetX;
    *destY -= gTotalCameraPixelOffsetY;
}

void SetSpritePosToMapCoords(s16 mapX, s16 mapY, s16 *destX, s16 *destY)
{
    s16 dx = -gTotalCameraPixelOffsetX - gFieldCamera.x;
    s16 dy = -gTotalCameraPixelOffsetY - gFieldCamera.y;
    if (gFieldCamera.x > 0)
        dx += 16;

    if (gFieldCamera.x < 0)
        dx -= 16;

    if (gFieldCamera.y > 0)
        dy += 16;

    if (gFieldCamera.y < 0)
        dy -= 16;

    *destX = ((mapX - gSaveBlock1Ptr->pos.x) << 4) + dx;
    *destY = ((mapY - gSaveBlock1Ptr->pos.y) << 4) + dy;
}

void SetSpritePosToOffsetMapCoords(s16 *x, s16 *y, s16 dx, s16 dy)
{
    SetSpritePosToMapCoords(*x, *y, x, y);
    *x += dx;
    *y += dy;
}

static void GetObjectEventMovingCameraOffset(s16 *x, s16 *y)
{
    *x = 0;
    *y = 0;

    if (gFieldCamera.x > 0)
        (*x)++;

    if (gFieldCamera.x < 0)
        (*x)--;

    if (gFieldCamera.y > 0)
        (*y)++;

    if (gFieldCamera.y < 0)
        (*y)--;
}

void ObjectEventMoveDestCoords(struct ObjectEvent *objectEvent, u32 direction, s16 *x, s16 *y)
{
    u8 newDirn = direction;
    *x = objectEvent->currentCoords.x;
    *y = objectEvent->currentCoords.y;
    MoveCoords(newDirn, x, y);
}

bool8 ObjectEventIsMovementOverridden(struct ObjectEvent *objectEvent)
{
    if (objectEvent->singleMovementActive || objectEvent->heldMovementActive)
        return TRUE;

    return FALSE;
}

bool8 ObjectEventIsHeldMovementActive(struct ObjectEvent *objectEvent)
{
    if (objectEvent->heldMovementActive && objectEvent->movementActionId != MOVEMENT_ACTION_NONE)
        return TRUE;

    return FALSE;
}

bool8 ObjectEventSetHeldMovement(struct ObjectEvent *objectEvent, u8 movementActionId)
{
    if (ObjectEventIsMovementOverridden(objectEvent))
        return TRUE;

    UnfreezeObjectEvent(objectEvent);
    objectEvent->movementActionId = movementActionId;
    objectEvent->heldMovementActive = TRUE;
    objectEvent->heldMovementFinished = FALSE;
    gSprites[objectEvent->spriteId].sActionFuncId = 0;
    return FALSE;
}

void ObjectEventForceSetHeldMovement(struct ObjectEvent *objectEvent, u8 movementActionId)
{
    ObjectEventClearHeldMovementIfActive(objectEvent);
    ObjectEventSetHeldMovement(objectEvent, movementActionId);
}

void ObjectEventClearHeldMovementIfActive(struct ObjectEvent *objectEvent)
{
    if (objectEvent->heldMovementActive)
        ObjectEventClearHeldMovement(objectEvent);
}

void ObjectEventClearHeldMovement(struct ObjectEvent *objectEvent)
{
    objectEvent->movementActionId = MOVEMENT_ACTION_NONE;
    objectEvent->heldMovementActive = FALSE;
    objectEvent->heldMovementFinished = FALSE;
    gSprites[objectEvent->spriteId].sTypeFuncId = 0;
    gSprites[objectEvent->spriteId].sActionFuncId = 0;
}

u8 ObjectEventCheckHeldMovementStatus(struct ObjectEvent *objectEvent)
{
    if (objectEvent->heldMovementActive)
        return objectEvent->heldMovementFinished;

    return 16;
}

u8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *objectEvent)
{
    u8 heldMovementStatus = ObjectEventCheckHeldMovementStatus(objectEvent);
    if (heldMovementStatus != 0 && heldMovementStatus != 16)
        ObjectEventClearHeldMovementIfActive(objectEvent);

    return heldMovementStatus;
}

u8 ObjectEventGetHeldMovementActionId(struct ObjectEvent *objectEvent)
{
    if (objectEvent->heldMovementActive)
        return objectEvent->movementActionId;

    return MOVEMENT_ACTION_NONE;
}

void UpdateObjectEventCurrentMovement(struct ObjectEvent *objectEvent, struct Sprite *sprite, bool8 (*callback)(struct ObjectEvent *, struct Sprite *))
{
    DoGroundEffects_OnSpawn(objectEvent, sprite);
    TryEnableObjectEventAnim(objectEvent, sprite);

    if (ObjectEventIsHeldMovementActive(objectEvent))
        ObjectEventExecHeldMovementAction(objectEvent, sprite);
    else if (!objectEvent->frozen)
        while (callback(objectEvent, sprite));

    DoGroundEffects_OnBeginStep(objectEvent, sprite);
    DoGroundEffects_OnFinishStep(objectEvent, sprite);
    UpdateObjectEventSpriteAnimPause(objectEvent, sprite);
    UpdateObjectEventVisibility(objectEvent, sprite);
    ObjectEventUpdateSubpriority(objectEvent, sprite);
}

#define dirn_to_anim(name, table)\
u8 name(u32 idx)\
{\
    u8 direction;\
    u8 animIds[sizeof(table)];\
    direction = idx;\
    memcpy(animIds, (table), sizeof(table));\
    if (direction > DIR_EAST) direction = 0;\
    return animIds[direction];\
}

dirn_to_anim(GetFaceDirectionMovementAction, gFaceDirectionMovementActions);
dirn_to_anim(GetWalkSlowMovementAction, gWalkSlowMovementActions);
dirn_to_anim(GetWalkNormalMovementAction, gWalkNormalMovementActions);
dirn_to_anim(GetWalkFastMovementAction, gWalkFastMovementActions);
dirn_to_anim(GetRideWaterCurrentMovementAction, gRideWaterCurrentMovementActions);
dirn_to_anim(GetWalkFasterMovementAction, gWalkFasterMovementActions);
dirn_to_anim(GetSlideMovementAction, gSlideMovementActions);
dirn_to_anim(GetPlayerRunMovementAction, gPlayerRunMovementActions);
dirn_to_anim(GetJump2MovementAction, gJump2MovementActions);
dirn_to_anim(GetJumpInPlaceMovementAction, gJumpInPlaceMovementActions);
dirn_to_anim(GetJumpInPlaceTurnAroundMovementAction, gJumpInPlaceTurnAroundMovementActions);
dirn_to_anim(GetJumpMovementAction, gJumpMovementActions);
dirn_to_anim(GetJumpSpecialMovementAction, gJumpSpecialMovementActions);
dirn_to_anim(GetWalkInPlaceSlowMovementAction, gWalkInPlaceSlowMovementActions);
dirn_to_anim(GetWalkInPlaceNormalMovementAction, gWalkInPlaceNormalMovementActions);
dirn_to_anim(GetWalkInPlaceFastMovementAction, gWalkInPlaceFastMovementActions);
dirn_to_anim(GetWalkInPlaceFasterMovementAction, gWalkInPlaceFasterMovementActions);

bool8 ObjectEventFaceOppositeDirection(struct ObjectEvent *objectEvent, u8 direction)
{
    return ObjectEventSetHeldMovement(objectEvent, GetFaceDirectionMovementAction(GetOppositeDirection(direction)));
}

dirn_to_anim(GetAcroWheelieFaceDirectionMovementAction, gAcroWheelieFaceDirectionMovementActions);
dirn_to_anim(GetAcroPopWheelieFaceDirectionMovementAction, gAcroPopWheelieFaceDirectionMovementActions);
dirn_to_anim(GetAcroEndWheelieFaceDirectionMovementAction, gAcroEndWheelieFaceDirectionMovementActions);
dirn_to_anim(GetAcroWheelieHopFaceDirectionMovementAction, gAcroWheelieHopFaceDirectionMovementActions);
dirn_to_anim(GetAcroWheelieHopDirectionMovementAction, gAcroWheelieHopDirectionMovementActions);
dirn_to_anim(GetAcroWheelieJumpDirectionMovementAction, gAcroWheelieJumpDirectionMovementActions);
dirn_to_anim(GetAcroWheelieInPlaceDirectionMovementAction, gAcroWheelieInPlaceDirectionMovementActions);
dirn_to_anim(GetAcroPopWheelieMoveDirectionMovementAction, gAcroPopWheelieMoveDirectionMovementActions);
dirn_to_anim(GetAcroWheelieMoveDirectionMovementAction, gAcroWheelieMoveDirectionMovementActions);
dirn_to_anim(GetAcroEndWheelieMoveDirectionMovementAction, gAcroEndWheelieMoveDirectionMovementActions);

u8 GetOppositeDirection(u8 direction)
{
    u8 directions[sizeof sOppositeDirections];

    memcpy(directions, sOppositeDirections, sizeof sOppositeDirections);
    if (direction <= DIR_NONE || direction > (sizeof sOppositeDirections))
        return direction;

    return directions[direction - 1];
}

// Takes the player's original and current direction and gives a direction the copy NPC should consider as the player's direction.
// See comments at the table's definition.
static u32 GetPlayerDirectionForCopy(u8 initDir, u8 moveDir)
{
    return sPlayerDirectionsForCopy[initDir - 1][moveDir - 1];
}

// copyInitDir is the initial facing direction of the copying NPC.
// playerInitDir is the direction the player was facing when the copying NPC was spawned, as set by MovementType_CopyPlayer_Step0.
// playerMoveDir is the direction the player is currently moving.
static u32 GetCopyDirection(u8 copyInitDir, u32 playerInitDir, u32 playerMoveDir)
{
    u32 dir;
    u8 _playerInitDir = playerInitDir;
    u8 _playerMoveDir = playerMoveDir;
    if (_playerInitDir == DIR_NONE || _playerMoveDir == DIR_NONE
      || _playerInitDir > DIR_EAST || _playerMoveDir > DIR_EAST)
        return DIR_NONE;

    dir = GetPlayerDirectionForCopy(_playerInitDir, playerMoveDir);
    return sPlayerDirectionToCopyDirection[copyInitDir - 1][dir - 1];
}

static void ObjectEventExecHeldMovementAction(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (gMovementActionFuncs[objectEvent->movementActionId][sprite->sActionFuncId](objectEvent, sprite))
        objectEvent->heldMovementFinished = TRUE;
}

static bool8 ObjectEventExecSingleMovementAction(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (gMovementActionFuncs[objectEvent->movementActionId][sprite->sActionFuncId](objectEvent, sprite))
    {
        objectEvent->movementActionId = MOVEMENT_ACTION_NONE;
        sprite->sActionFuncId = 0;
        return TRUE;
    }
    return FALSE;
}

static void ObjectEventSetSingleMovement(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 animId)
{
    objectEvent->movementActionId = animId;
    sprite->sActionFuncId = 0;
}

static void FaceDirection(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    SetObjectEventDirection(objectEvent, direction);
    ShiftStillObjectEventCoords(objectEvent);
    SetStepAnim(objectEvent, sprite, GetMoveDirectionAnimNum(objectEvent->facingDirection));
    sprite->animPaused = TRUE;
    sprite->sActionFuncId = 1;
}

bool8 MovementAction_FaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FaceDirection(objectEvent, sprite, DIR_SOUTH);
    return TRUE;
}

bool8 MovementAction_FaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FaceDirection(objectEvent, sprite, DIR_NORTH);
    return TRUE;
}

bool8 MovementAction_FaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FaceDirection(objectEvent, sprite, DIR_WEST);
    return TRUE;
}

bool8 MovementAction_FaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FaceDirection(objectEvent, sprite, DIR_EAST);
    return TRUE;
}

void InitNpcForMovement(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 speed)
{
    s16 x;
    s16 y;

    x = objectEvent->currentCoords.x;
    y = objectEvent->currentCoords.y;
    SetObjectEventDirection(objectEvent, direction);
    MoveCoords(direction, &x, &y);
    ShiftObjectEventCoords(objectEvent, x, y);
    SetSpriteDataForNormalStep(sprite, direction, speed);
    sprite->animPaused = FALSE;

    if (sLockedAnimObjectEvents != NULL && FindLockedObjectEventIndex(objectEvent) != OBJECT_EVENTS_COUNT)
        sprite->animPaused = TRUE;

    objectEvent->triggerGroundEffectsOnMove = TRUE;
    sprite->sActionFuncId = 1;
}

static void InitMovementNormal(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 speed)
{
    u8 (*functions[ARRAY_COUNT(sDirectionAnimFuncsBySpeed)])(u8);

    memcpy(functions, sDirectionAnimFuncsBySpeed, sizeof sDirectionAnimFuncsBySpeed);
    InitNpcForMovement(objectEvent, sprite, direction, speed);
    SetStepAnimHandleAlternation(objectEvent, sprite, functions[speed](objectEvent->facingDirection));
}

static void StartRunningAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    InitNpcForMovement(objectEvent, sprite, direction, MOVE_SPEED_FAST_1);
    SetStepAnimHandleAlternation(objectEvent, sprite, GetRunningDirectionAnimNum(objectEvent->facingDirection));
}

static bool8 UpdateMovementNormal(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (NpcTakeStep(sprite))
    {
        ShiftStillObjectEventCoords(objectEvent);
        objectEvent->triggerGroundEffectsOnStop = TRUE;
        sprite->animPaused = TRUE;
        return TRUE;
    }
    return FALSE;
}

static void InitNpcForWalkSlow(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    s16 x;
    s16 y;

    x = objectEvent->currentCoords.x;
    y = objectEvent->currentCoords.y;
    SetObjectEventDirection(objectEvent, direction);
    MoveCoords(direction, &x, &y);
    ShiftObjectEventCoords(objectEvent, x, y);
    SetWalkSlowSpriteData(sprite, direction);
    sprite->animPaused = FALSE;
    objectEvent->triggerGroundEffectsOnMove = TRUE;
    sprite->sActionFuncId = 1;
}

static void InitWalkSlow(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    InitNpcForWalkSlow(objectEvent, sprite, direction);
    SetStepAnimHandleAlternation(objectEvent, sprite, GetMoveDirectionAnimNum(objectEvent->facingDirection));
}

static bool8 UpdateWalkSlow(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlowAnim(sprite))
    {
        ShiftStillObjectEventCoords(objectEvent);
        objectEvent->triggerGroundEffectsOnStop = TRUE;
        sprite->animPaused = TRUE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowDiagonalUpLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_NORTHWEST);
    return MovementAction_WalkSlowDiagonalUpLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowDiagonalUpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowDiagonalUpRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_NORTHEAST);
    return MovementAction_WalkSlowDiagonalUpRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowDiagonalUpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowDiagonalDownLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_SOUTHWEST);
    return MovementAction_WalkSlowDiagonalDownLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowDiagonalDownLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowDiagonalDownRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_SOUTHEAST);
    return MovementAction_WalkSlowDiagonalDownRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowDiagonalDownRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_SOUTH);
    return MovementAction_WalkSlowDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_NORTH);
    return MovementAction_WalkSlowUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_WEST);
    return MovementAction_WalkSlowLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkSlowRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_EAST);
    return MovementAction_WalkSlowRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkSlowRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalDiagonalUpLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTHWEST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalDiagonalUpLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalDiagonalUpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalDiagonalUpRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTHEAST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalDiagonalUpRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalDiagonalUpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalDiagonalDownLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTHWEST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalDiagonalDownLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalDiagonalDownLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalDiagonalDownRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTHEAST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalDiagonalDownRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalDiagonalDownRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTH, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTH, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkNormalRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_NORMAL);
    return MovementAction_WalkNormalRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkNormalRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

#define JUMP_HALFWAY  1
#define JUMP_FINISHED ((u8)-1)

enum {
    JUMP_TYPE_HIGH,
    JUMP_TYPE_LOW,
    JUMP_TYPE_NORMAL,
};

static void InitJump(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 distance, u8 type)
{
    s16 displacements[ARRAY_COUNT(sJumpInitDisplacements)];
    s16 x;
    s16 y;

    memcpy(displacements, sJumpInitDisplacements, sizeof sJumpInitDisplacements);
    x = 0;
    y = 0;
    SetObjectEventDirection(objectEvent, direction);
    MoveCoordsInDirection(direction, &x, &y, displacements[distance], displacements[distance]);
    ShiftObjectEventCoords(objectEvent, objectEvent->currentCoords.x + x, objectEvent->currentCoords.y + y);
    SetJumpSpriteData(sprite, direction, distance, type);
    sprite->sActionFuncId = 1;
    sprite->animPaused = FALSE;
    objectEvent->triggerGroundEffectsOnMove = TRUE;
    objectEvent->disableCoveringGroundEffects = TRUE;
}

static void InitJumpRegular(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 distance, u8 type)
{
    InitJump(objectEvent, sprite, direction, distance, type);
    SetStepAnimHandleAlternation(objectEvent, sprite, GetMoveDirectionAnimNum(objectEvent->facingDirection));
    DoShadowFieldEffect(objectEvent);
}

#define sDistance data[4]

static u8 UpdateJumpAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 callback(struct Sprite *))
{
    s16 displacements[ARRAY_COUNT(sJumpDisplacements)];
    s16 x;
    s16 y;
    u8 result;

    memcpy(displacements, sJumpDisplacements, sizeof sJumpDisplacements);
    result = callback(sprite);
    if (result == JUMP_HALFWAY && displacements[sprite->sDistance] != 0)
    {
        x = 0;
        y = 0;
        MoveCoordsInDirection(objectEvent->movementDirection, &x, &y, displacements[sprite->sDistance], displacements[sprite->sDistance]);
        ShiftObjectEventCoords(objectEvent, objectEvent->currentCoords.x + x, objectEvent->currentCoords.y + y);
        objectEvent->triggerGroundEffectsOnMove = TRUE;
        objectEvent->disableCoveringGroundEffects = TRUE;
    }
    else if (result == JUMP_FINISHED)
    {
        ShiftStillObjectEventCoords(objectEvent);
        objectEvent->triggerGroundEffectsOnStop = TRUE;
        objectEvent->landingJump = TRUE;
        sprite->animPaused = TRUE;
    }
    return result;
}

#undef sDistance

static u8 DoJumpAnimStep(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return UpdateJumpAnim(objectEvent, sprite, DoJumpSpriteMovement);
}

static u8 DoJumpSpecialAnimStep(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return UpdateJumpAnim(objectEvent, sprite, DoJumpSpecialSpriteMovement);
}

static bool8 DoJumpAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnimStep(objectEvent, sprite) == JUMP_FINISHED)
        return TRUE;

    return FALSE;
}

static bool8 DoJumpSpecialAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpSpecialAnimStep(objectEvent, sprite) == JUMP_FINISHED)
        return TRUE;

    return FALSE;
}

static bool8 DoJumpInPlaceAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    switch (DoJumpAnimStep(objectEvent, sprite))
    {
        case JUMP_FINISHED:
            return TRUE;
        case JUMP_HALFWAY:
            SetObjectEventDirection(objectEvent, GetOppositeDirection(objectEvent->movementDirection));
            SetStepAnim(objectEvent, sprite, GetMoveDirectionAnimNum(objectEvent->facingDirection));
        default:
            return FALSE;
    }
}

bool8 MovementAction_Jump2Down_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_Jump2Down_Step1(objectEvent, sprite);
}

bool8 MovementAction_Jump2Down_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Jump2Up_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_Jump2Up_Step1(objectEvent, sprite);
}

bool8 MovementAction_Jump2Up_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Jump2Left_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_Jump2Left_Step1(objectEvent, sprite);
}

bool8 MovementAction_Jump2Left_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Jump2Right_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_Jump2Right_Step1(objectEvent, sprite);
}

bool8 MovementAction_Jump2Right_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void InitMovementDelay(struct Sprite *sprite, u16 duration)
{
    sprite->sActionFuncId = 1;
    sprite->data[3] = duration;
}

bool8 MovementAction_Delay_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (--sprite->data[3] == 0)
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Delay1_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementDelay(sprite, 1);
    return MovementAction_Delay_Step1(objectEvent, sprite);
}

bool8 MovementAction_Delay2_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementDelay(sprite, 2);
    return MovementAction_Delay_Step1(objectEvent, sprite);
}

bool8 MovementAction_Delay4_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementDelay(sprite, 4);
    return MovementAction_Delay_Step1(objectEvent, sprite);
}

bool8 MovementAction_Delay8_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementDelay(sprite, 8);
    return MovementAction_Delay_Step1(objectEvent, sprite);
}

bool8 MovementAction_Delay16_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementDelay(sprite, 16);
    return MovementAction_Delay_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFastDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTH, MOVE_SPEED_FAST_1);
    return MovementAction_WalkFastDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFastDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFastUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTH, MOVE_SPEED_FAST_1);
    return MovementAction_WalkFastUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFastUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFastLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_FAST_1);
    return MovementAction_WalkFastLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFastLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFastRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_FAST_1);
    return MovementAction_WalkFastRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFastRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}


static void InitMoveInPlace(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 animNum, u16 duration)
{
    SetObjectEventDirection(objectEvent, direction);
    SetStepAnimHandleAlternation(objectEvent, sprite, animNum);
    sprite->animPaused = FALSE;
    sprite->sActionFuncId = 1;
    sprite->data[3] = duration;
}

bool8 MovementAction_WalkInPlace_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (-- sprite->data[3] == 0)
    {
        sprite->sActionFuncId = 2;
        sprite->animPaused = TRUE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkInPlaceSlow_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (sprite->data[3] & 1)
        sprite->animDelayCounter++;

    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceSlowDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_SOUTH, GetMoveDirectionAnimNum(DIR_SOUTH), 32);
    return MovementAction_WalkInPlaceSlow_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceSlowUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_NORTH, GetMoveDirectionAnimNum(DIR_NORTH), 32);
    return MovementAction_WalkInPlaceSlow_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceSlowLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_WEST, GetMoveDirectionAnimNum(DIR_WEST), 32);
    return MovementAction_WalkInPlaceSlow_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceSlowRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_EAST, GetMoveDirectionAnimNum(DIR_EAST), 32);
    return MovementAction_WalkInPlaceSlow_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceNormalDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_SOUTH, GetMoveDirectionAnimNum(DIR_SOUTH), 16);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceNormalUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_NORTH, GetMoveDirectionAnimNum(DIR_NORTH), 16);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceNormalLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_WEST, GetMoveDirectionAnimNum(DIR_WEST), 16);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceNormalRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_EAST, GetMoveDirectionAnimNum(DIR_EAST), 16);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFastDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_SOUTH, GetMoveDirectionFastAnimNum(DIR_SOUTH), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFastUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_NORTH, GetMoveDirectionFastAnimNum(DIR_NORTH), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFastLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_WEST, GetMoveDirectionFastAnimNum(DIR_WEST), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFastRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_EAST, GetMoveDirectionFastAnimNum(DIR_EAST), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFasterDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_SOUTH, GetMoveDirectionFasterAnimNum(DIR_SOUTH), 4);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFasterUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_NORTH, GetMoveDirectionFasterAnimNum(DIR_NORTH), 4);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFasterLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_WEST, GetMoveDirectionFasterAnimNum(DIR_WEST), 4);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkInPlaceFasterRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_EAST, GetMoveDirectionFasterAnimNum(DIR_EAST), 4);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_RideWaterCurrentDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTH, MOVE_SPEED_FAST_2);
    return MovementAction_RideWaterCurrentDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_RideWaterCurrentDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_RideWaterCurrentUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTH, MOVE_SPEED_FAST_2);
    return MovementAction_RideWaterCurrentUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_RideWaterCurrentUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_RideWaterCurrentLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_FAST_2);
    return MovementAction_RideWaterCurrentLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_RideWaterCurrentLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_RideWaterCurrentRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_FAST_2);
    return MovementAction_RideWaterCurrentRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_RideWaterCurrentRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFasterDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTH, MOVE_SPEED_FASTER);
    return MovementAction_WalkFasterDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFasterDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFasterUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTH, MOVE_SPEED_FASTER);
    return MovementAction_WalkFasterUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFasterUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFasterLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_FASTER);
    return MovementAction_WalkFasterLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFasterLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkFasterRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_FASTER);
    return MovementAction_WalkFasterRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkFasterRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_SlideDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_SOUTH, MOVE_SPEED_FASTEST);
    return MovementAction_SlideDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_SlideDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_SlideUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_NORTH, MOVE_SPEED_FASTEST);
    return MovementAction_SlideUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_SlideUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_SlideLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_FASTEST);
    return MovementAction_SlideLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_SlideLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_SlideRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_FASTEST);
    return MovementAction_SlideRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_SlideRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_PlayerRunDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartRunningAnim(objectEvent, sprite, DIR_SOUTH);
    return MovementAction_PlayerRunDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_PlayerRunDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_PlayerRunUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartRunningAnim(objectEvent, sprite, DIR_NORTH);
    return MovementAction_PlayerRunUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_PlayerRunUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_PlayerRunLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartRunningAnim(objectEvent, sprite, DIR_WEST);
    return MovementAction_PlayerRunLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_PlayerRunLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_PlayerRunRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartRunningAnim(objectEvent, sprite, DIR_EAST);
    return MovementAction_PlayerRunRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_PlayerRunRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

void StartSpriteAnimInDirection(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 animNum)
{
    SetAndStartSpriteAnim(sprite, animNum, 0);
    SetObjectEventDirection(objectEvent, direction);
    sprite->sActionFuncId = 1;
}

bool8 MovementAction_StartAnimInDirection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, objectEvent->movementDirection, sprite->animNum);
    return FALSE;
}

bool8 MovementAction_WaitSpriteAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (SpriteAnimEnded(sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void InitJumpSpecial(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    InitJump(objectEvent, sprite, direction, JUMP_DISTANCE_NORMAL, JUMP_TYPE_HIGH);
    StartSpriteAnim(sprite, GetJumpSpecialDirectionAnimNum(direction));
}

bool8 MovementAction_JumpSpecialDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpSpecial(objectEvent, sprite, DIR_SOUTH);
    return MovementAction_JumpSpecialDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpSpecialDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpSpecialAnim(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        objectEvent->landingJump = FALSE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpSpecialUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpSpecial(objectEvent, sprite, DIR_NORTH);
    return MovementAction_JumpSpecialUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpSpecialUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpSpecialAnim(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        objectEvent->landingJump = FALSE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpSpecialLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpSpecial(objectEvent, sprite, DIR_WEST);
    return MovementAction_JumpSpecialLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpSpecialLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpSpecialAnim(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        objectEvent->landingJump = FALSE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpSpecialRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpSpecial(objectEvent, sprite, DIR_EAST);
    return MovementAction_JumpSpecialRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpSpecialRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpSpecialAnim(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        objectEvent->landingJump = FALSE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_FacePlayer_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 playerObjectId;

    if (!TryGetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0, &playerObjectId))
        FaceDirection(objectEvent, sprite, GetDirectionToFace(objectEvent->currentCoords.x,
                                                              objectEvent->currentCoords.y,
                                                              gObjectEvents[playerObjectId].currentCoords.x,
                                                              gObjectEvents[playerObjectId].currentCoords.y));
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_FaceAwayPlayer_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u8 playerObjectId;

    if (!TryGetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0, &playerObjectId))
        FaceDirection(objectEvent, sprite, GetOppositeDirection(GetDirectionToFace(objectEvent->currentCoords.x,
                                                                                   objectEvent->currentCoords.y,
                                                                                   gObjectEvents[playerObjectId].currentCoords.x,
                                                                                   gObjectEvents[playerObjectId].currentCoords.y)));
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_LockFacingDirection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->facingDirectionLocked = TRUE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_UnlockFacingDirection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->facingDirectionLocked = FALSE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_JumpDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_NORMAL, JUMP_TYPE_NORMAL);
    return MovementAction_JumpDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_NORMAL, JUMP_TYPE_NORMAL);
    return MovementAction_JumpUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_NORMAL, JUMP_TYPE_NORMAL);
    return MovementAction_JumpLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_NORMAL, JUMP_TYPE_NORMAL);
    return MovementAction_JumpRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_HIGH);
    return MovementAction_JumpInPlaceDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_HIGH);
    return MovementAction_JumpInPlaceUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_HIGH);
    return MovementAction_JumpInPlaceLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_HIGH);
    return MovementAction_JumpInPlaceRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceDownUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_NORMAL);
    return MovementAction_JumpInPlaceDownUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceDownUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpInPlaceAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceUpDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_NORMAL);
    return MovementAction_JumpInPlaceUpDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceUpDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpInPlaceAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceLeftRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_NORMAL);
    return MovementAction_JumpInPlaceLeftRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceLeftRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpInPlaceAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_JumpInPlaceRightLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitJumpRegular(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_NORMAL);
    return MovementAction_JumpInPlaceRightLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_JumpInPlaceRightLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpInPlaceAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = 0;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_FaceOriginalDirection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FaceDirection(objectEvent, sprite, gInitialMovementTypeFacingDirections[objectEvent->movementType]);
    return TRUE;
}

bool8 MovementAction_NurseJoyBowDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_SOUTH, ANIM_NURSE_BOW);
    return FALSE;
}

bool8 MovementAction_EnableJumpLandingGroundEffect_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->disableJumpLandingGroundEffect = FALSE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_DisableJumpLandingGroundEffect_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->disableJumpLandingGroundEffect = TRUE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_DisableAnimation_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->inanimate = TRUE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_RestoreAnimation_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->inanimate = GetObjectEventGraphicsInfo(objectEvent->graphicsId)->inanimate;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_SetInvisible_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->invisible = TRUE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_SetVisible_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->invisible = FALSE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_EmoteExclamationMark_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
    FieldEffectStart(FLDEFF_EXCLAMATION_MARK_ICON);
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_EmoteQuestionMark_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
    FieldEffectStart(FLDEFF_QUESTION_MARK_ICON);
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_EmoteHeart_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
    FieldEffectStart(FLDEFF_HEART_ICON);
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_RevealTrainer_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (objectEvent->movementType == MOVEMENT_TYPE_BURIED)
    {
        SetBuriedTrainerMovement(objectEvent);
        return FALSE;
    }
    if (objectEvent->movementType != MOVEMENT_TYPE_TREE_DISGUISE && objectEvent->movementType != MOVEMENT_TYPE_MOUNTAIN_DISGUISE)
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    StartRevealDisguise(objectEvent);
    sprite->sActionFuncId = 1;
    return MovementAction_RevealTrainer_Step1(objectEvent, sprite);
}

bool8 MovementAction_RevealTrainer_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateRevealDisguise(objectEvent))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_RockSmashBreak_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    SetAndStartSpriteAnim(sprite, ANIM_REMOVE_OBSTACLE, 0);
    sprite->sActionFuncId = 1;
    return FALSE;
}

bool8 MovementAction_RockSmashBreak_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (SpriteAnimEnded(sprite))
    {
        SetMovementDelay(sprite, 32);
        sprite->sActionFuncId = 2;
    }
    return FALSE;
}

bool8 MovementAction_RockSmashBreak_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->invisible ^= TRUE;
    if (WaitForMovementDelay(sprite))
    {
        objectEvent->invisible = TRUE;
        sprite->sActionFuncId = 3;
    }
    return FALSE;
}

bool8 MovementAction_CutTree_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    SetAndStartSpriteAnim(sprite, ANIM_REMOVE_OBSTACLE, 0);
    sprite->sActionFuncId = 1;
    return FALSE;
}

bool8 MovementAction_CutTree_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (SpriteAnimEnded(sprite))
    {
        SetMovementDelay(sprite, 32);
        sprite->sActionFuncId = 2;
    }
    return FALSE;
}

bool8 MovementAction_CutTree_Step2(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->invisible ^= TRUE;
    if (WaitForMovementDelay(sprite))
    {
        objectEvent->invisible = TRUE;
        sprite->sActionFuncId = 3;
    }
    return FALSE;
}

bool8 MovementAction_SetFixedPriority_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->fixedPriority = TRUE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_ClearFixedPriority_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->fixedPriority = FALSE;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_InitAffineAnim_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    InitSpriteAffineAnim(sprite);
    sprite->affineAnimPaused = TRUE;
    sprite->subspriteMode = SUBSPRITES_OFF;
    return TRUE;
}

bool8 MovementAction_ClearAffineAnim_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    FreeOamMatrix(sprite->oam.matrixNum);
    sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
    return TRUE;
}

bool8 MovementAction_HideReflection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->hideReflection = TRUE;
    return TRUE;
}

bool8 MovementAction_ShowReflection_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    objectEvent->hideReflection = FALSE;
    return TRUE;
}

bool8 MovementAction_WalkDownStartAffine_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_SOUTH);
    sprite->affineAnimPaused = FALSE;
    StartSpriteAffineAnimIfDifferent(sprite, 0);
    return MovementAction_WalkDownStartAffine_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkDownStartAffine_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->affineAnimPaused = TRUE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkDownAffine_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitWalkSlow(objectEvent, sprite, DIR_SOUTH);
    sprite->affineAnimPaused = FALSE;
    ChangeSpriteAffineAnimIfDifferent(sprite, 1);
    return MovementAction_WalkDownAffine_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkDownAffine_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateWalkSlow(objectEvent, sprite))
    {
        sprite->affineAnimPaused = TRUE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkLeftAffine_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_WEST, MOVE_SPEED_FAST_1);
    sprite->affineAnimPaused = FALSE;
    ChangeSpriteAffineAnimIfDifferent(sprite, 2);
    return MovementAction_WalkLeftAffine_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkLeftAffine_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->affineAnimPaused = TRUE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_WalkRightAffine_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMovementNormal(objectEvent, sprite, DIR_EAST, MOVE_SPEED_FAST_1);
    sprite->affineAnimPaused = FALSE;
    ChangeSpriteAffineAnimIfDifferent(sprite, 3);
    return MovementAction_WalkRightAffine_Step1(objectEvent, sprite);
}

bool8 MovementAction_WalkRightAffine_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->affineAnimPaused = TRUE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void AcroWheelieFaceDirection(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction)
{
    SetObjectEventDirection(objectEvent, direction);
    ShiftStillObjectEventCoords(objectEvent);
    SetStepAnim(objectEvent, sprite, GetAcroWheeliePedalDirectionAnimNum(direction));
    sprite->animPaused = TRUE;
    sprite->sActionFuncId = 1;
}

bool8 MovementAction_AcroWheelieFaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    AcroWheelieFaceDirection(objectEvent, sprite, DIR_SOUTH);
    return TRUE;
}

bool8 MovementAction_AcroWheelieFaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    AcroWheelieFaceDirection(objectEvent, sprite, DIR_NORTH);
    return TRUE;
}

bool8 MovementAction_AcroWheelieFaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    AcroWheelieFaceDirection(objectEvent, sprite, DIR_WEST);
    return TRUE;
}

bool8 MovementAction_AcroWheelieFaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    AcroWheelieFaceDirection(objectEvent, sprite, DIR_EAST);
    return TRUE;
}

bool8 MovementAction_AcroPopWheelieDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_SOUTH, GetAcroWheelieDirectionAnimNum(DIR_SOUTH));
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_NORTH, GetAcroWheelieDirectionAnimNum(DIR_NORTH));
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_WEST, GetAcroWheelieDirectionAnimNum(DIR_WEST));
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_EAST, GetAcroWheelieDirectionAnimNum(DIR_EAST));
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieFaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_SOUTH, GetAcroEndWheelieDirectionAnimNum(DIR_SOUTH));
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieFaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_NORTH, GetAcroEndWheelieDirectionAnimNum(DIR_NORTH));
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieFaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_WEST, GetAcroEndWheelieDirectionAnimNum(DIR_WEST));
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieFaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_EAST, GetAcroEndWheelieDirectionAnimNum(DIR_EAST));
    return FALSE;
}

bool8 MovementAction_UnusedAcroActionDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_SOUTH, GetAcroUnusedActionDirectionAnimNum(DIR_SOUTH));
    return FALSE;
}

bool8 MovementAction_UnusedAcroActionUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_NORTH, GetAcroUnusedActionDirectionAnimNum(DIR_NORTH));
    return FALSE;
}

bool8 MovementAction_UnusedAcroActionLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_WEST, GetAcroUnusedActionDirectionAnimNum(DIR_WEST));
    return FALSE;
}

bool8 MovementAction_UnusedAcroActionRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    StartSpriteAnimInDirection(objectEvent, sprite, DIR_EAST, GetAcroUnusedActionDirectionAnimNum(DIR_EAST));
    return FALSE;
}

void InitFigure8Anim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitSpriteForFigure8Anim(sprite);
    sprite->animPaused = FALSE;
}

bool8 DoFigure8Anim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (AnimateSpriteInFigure8(sprite))
    {
        ShiftStillObjectEventCoords(objectEvent);
        objectEvent->triggerGroundEffectsOnStop = TRUE;
        sprite->animPaused = TRUE;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Figure8_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitFigure8Anim(objectEvent, sprite);
    sprite->sActionFuncId = 1;
    return MovementAction_Figure8_Step1(objectEvent, sprite);
}

bool8 MovementAction_Figure8_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoFigure8Anim(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void InitAcroWheelieJump(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 distance, u8 type)
{
    InitJump(objectEvent, sprite, direction, distance, type);
    StartSpriteAnimIfDifferent(sprite, GetAcroWheelieDirectionAnimNum(direction));
    DoShadowFieldEffect(objectEvent);
}

bool8 MovementAction_AcroWheelieHopFaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopFaceDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopFaceDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopFaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopFaceUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopFaceUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopFaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopFaceLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopFaceLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopFaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_IN_PLACE, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopFaceRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopFaceRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_NORMAL, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_NORMAL, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_NORMAL, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieHopRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_NORMAL, JUMP_TYPE_LOW);
    return MovementAction_AcroWheelieHopRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieHopRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieJumpDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_SOUTH, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_AcroWheelieJumpDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieJumpDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieJumpUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_NORTH, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_AcroWheelieJumpUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieJumpUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieJumpLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_WEST, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_AcroWheelieJumpLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieJumpLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieJumpRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieJump(objectEvent, sprite, DIR_EAST, JUMP_DISTANCE_FAR, JUMP_TYPE_HIGH);
    return MovementAction_AcroWheelieJumpRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieJumpRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (DoJumpAnim(objectEvent, sprite))
    {
        objectEvent->hasShadow = FALSE;
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieInPlaceDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_SOUTH, GetAcroWheeliePedalDirectionAnimNum(DIR_SOUTH), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieInPlaceUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_NORTH, GetAcroWheeliePedalDirectionAnimNum(DIR_NORTH), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieInPlaceLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_WEST, GetAcroWheeliePedalDirectionAnimNum(DIR_WEST), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieInPlaceRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitMoveInPlace(objectEvent, sprite, DIR_EAST, GetAcroWheeliePedalDirectionAnimNum(DIR_EAST), 8);
    return MovementAction_WalkInPlace_Step1(objectEvent, sprite);
}

static void InitAcroPopWheelie(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 speed)
{
    InitNpcForMovement(objectEvent, sprite, direction, speed);
    StartSpriteAnim(sprite, GetAcroWheelieDirectionAnimNum(objectEvent->facingDirection));
    SeekSpriteAnim(sprite, 0);
}

bool8 MovementAction_AcroPopWheelieMoveDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroPopWheelie(objectEvent, sprite, DIR_SOUTH, 1);
    return MovementAction_AcroPopWheelieMoveDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroPopWheelieMoveDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieMoveUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroPopWheelie(objectEvent, sprite, DIR_NORTH, 1);
    return MovementAction_AcroPopWheelieMoveUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroPopWheelieMoveUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieMoveLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroPopWheelie(objectEvent, sprite, DIR_WEST,  1);
    return MovementAction_AcroPopWheelieMoveLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroPopWheelieMoveLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroPopWheelieMoveRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroPopWheelie(objectEvent, sprite, DIR_EAST,  1);
    return MovementAction_AcroPopWheelieMoveRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroPopWheelieMoveRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void InitAcroWheelieMove(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 speed)
{
    InitNpcForMovement(objectEvent, sprite, direction, speed);
    SetStepAnimHandleAlternation(objectEvent, sprite, GetAcroWheeliePedalDirectionAnimNum(objectEvent->facingDirection));
}

bool8 MovementAction_AcroWheelieMoveDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieMove(objectEvent, sprite, DIR_SOUTH, 1);
    return MovementAction_AcroWheelieMoveDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieMoveDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieMoveUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieMove(objectEvent, sprite, DIR_NORTH, 1);
    return MovementAction_AcroWheelieMoveUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieMoveUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieMoveLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieMove(objectEvent, sprite, DIR_WEST,  1);
    return MovementAction_AcroWheelieMoveLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieMoveLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroWheelieMoveRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroWheelieMove(objectEvent, sprite, DIR_EAST, 1);
    return MovementAction_AcroWheelieMoveRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroWheelieMoveRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

static void InitAcroEndWheelie(struct ObjectEvent *objectEvent, struct Sprite *sprite, u8 direction, u8 speed)
{
    InitNpcForMovement(objectEvent, sprite, direction, speed);
    StartSpriteAnim(sprite, GetAcroEndWheelieDirectionAnimNum(objectEvent->facingDirection));
    SeekSpriteAnim(sprite, 0);
}

bool8 MovementAction_AcroEndWheelieMoveDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroEndWheelie(objectEvent, sprite, DIR_SOUTH, 1);
    return MovementAction_AcroEndWheelieMoveDown_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroEndWheelieMoveDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieMoveUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroEndWheelie(objectEvent, sprite, DIR_NORTH, 1);
    return MovementAction_AcroEndWheelieMoveUp_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroEndWheelieMoveUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieMoveLeft_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroEndWheelie(objectEvent, sprite, DIR_WEST, 1);
    return MovementAction_AcroEndWheelieMoveLeft_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroEndWheelieMoveLeft_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_AcroEndWheelieMoveRight_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    InitAcroEndWheelie(objectEvent, sprite, DIR_EAST, 1);
    return MovementAction_AcroEndWheelieMoveRight_Step1(objectEvent, sprite);
}

bool8 MovementAction_AcroEndWheelieMoveRight_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (UpdateMovementNormal(objectEvent, sprite))
    {
        sprite->sActionFuncId = 2;
        return TRUE;
    }
    return FALSE;
}

bool8 MovementAction_Levitate_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    CreateLevitateMovementTask(objectEvent);
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_StopLevitate_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    DestroyLevitateMovementTask(objectEvent->warpArrowSpriteId);
    sprite->y2 = 0;
    sprite->sActionFuncId = 1;
    return TRUE;
}

bool8 MovementAction_StopLevitateAtTop_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (sprite->y2 == 0)
    {
        DestroyLevitateMovementTask(objectEvent->warpArrowSpriteId);
        sprite->sActionFuncId = 1;
        return TRUE;
    }
    return FALSE;
}

u8 MovementAction_Finish(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return TRUE;
}

bool8 MovementAction_PauseSpriteAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->animPaused = TRUE;
    return TRUE;
}

static void UpdateObjectEventSpriteAnimPause(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (objectEvent->disableAnim)
        sprite->animPaused = TRUE;
}

static void TryEnableObjectEventAnim(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    if (objectEvent->enableAnim)
    {
        sprite->animPaused = FALSE;
        objectEvent->disableAnim = FALSE;
        objectEvent->enableAnim = FALSE;
    }
}

static void UpdateObjectEventVisibility(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    UpdateObjectEventOffscreen(objectEvent, sprite);
    UpdateObjectEventSpriteVisibility(objectEvent, sprite);
}

static void UpdateObjectEventOffscreen(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    u16 x, y;
    u16 x2, y2;
    const struct ObjectEventGraphicsInfo *graphicsInfo;

    objectEvent->offScreen = FALSE;

    graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);
    if (sprite->coordOffsetEnabled)
    {
        x = sprite->x + sprite->x2 + sprite->centerToCornerVecX + gSpriteCoordOffsetX;
        y = sprite->y + sprite->y2 + sprite->centerToCornerVecY + gSpriteCoordOffsetY;
    }
    else
    {
        x = sprite->x + sprite->x2 + sprite->centerToCornerVecX;
        y = sprite->y + sprite->y2 + sprite->centerToCornerVecY;
    }
    x2 = graphicsInfo->width;
    x2 += x;
    y2 = y;
    y2 += graphicsInfo->height;

    if ((s16)x >= DISPLAY_WIDTH + 16 || (s16)x2 < -16)
        objectEvent->offScreen = TRUE;

    if ((s16)y >= DISPLAY_HEIGHT + 16 || (s16)y2 < -16)
        objectEvent->offScreen = TRUE;
}

static void UpdateObjectEventSpriteVisibility(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->invisible = FALSE;
    if (objectEvent->invisible || objectEvent->offScreen)
        sprite->invisible = TRUE;
}

static void GetAllGroundEffectFlags_OnSpawn(struct ObjectEvent *objEvent, u32 *flags)
{
    ObjectEventUpdateMetatileBehaviors(objEvent);
    GetGroundEffectFlags_Reflection(objEvent, flags);
    GetGroundEffectFlags_TallGrassOnSpawn(objEvent, flags);
    GetGroundEffectFlags_LongGrassOnSpawn(objEvent, flags);
    GetGroundEffectFlags_SandHeap(objEvent, flags);
    GetGroundEffectFlags_ShallowFlowingWater(objEvent, flags);
    GetGroundEffectFlags_ShortGrass(objEvent, flags);
    GetGroundEffectFlags_HotSprings(objEvent, flags);
}

static void GetAllGroundEffectFlags_OnBeginStep(struct ObjectEvent *objEvent, u32 *flags)
{
    ObjectEventUpdateMetatileBehaviors(objEvent);
    GetGroundEffectFlags_Reflection(objEvent, flags);
    GetGroundEffectFlags_TallGrassOnBeginStep(objEvent, flags);
    GetGroundEffectFlags_LongGrassOnBeginStep(objEvent, flags);
    GetGroundEffectFlags_Tracks(objEvent, flags);
    GetGroundEffectFlags_SandHeap(objEvent, flags);
    GetGroundEffectFlags_ShallowFlowingWater(objEvent, flags);
    GetGroundEffectFlags_Puddle(objEvent, flags);
    GetGroundEffectFlags_ShortGrass(objEvent, flags);
    GetGroundEffectFlags_HotSprings(objEvent, flags);
}

static void GetAllGroundEffectFlags_OnFinishStep(struct ObjectEvent *objEvent, u32 *flags)
{
    ObjectEventUpdateMetatileBehaviors(objEvent);
    GetGroundEffectFlags_ShallowFlowingWater(objEvent, flags);
    GetGroundEffectFlags_SandHeap(objEvent, flags);
    GetGroundEffectFlags_Puddle(objEvent, flags);
    GetGroundEffectFlags_Ripple(objEvent, flags);
    GetGroundEffectFlags_ShortGrass(objEvent, flags);
    GetGroundEffectFlags_HotSprings(objEvent, flags);
    GetGroundEffectFlags_Seaweed(objEvent, flags);
    GetGroundEffectFlags_JumpLanding(objEvent, flags);
}

static void ObjectEventUpdateMetatileBehaviors(struct ObjectEvent *objEvent)
{
    objEvent->previousMetatileBehavior = MapGridGetMetatileBehaviorAt(objEvent->previousCoords.x, objEvent->previousCoords.y);
    objEvent->currentMetatileBehavior = MapGridGetMetatileBehaviorAt(objEvent->currentCoords.x, objEvent->currentCoords.y);
}

static void GetGroundEffectFlags_Reflection(struct ObjectEvent *objEvent, u32 *flags)
{
    u32 reflectionFlags[NUM_REFLECTION_TYPES - 1] = {
        [REFL_TYPE_ICE   - 1] = GROUND_EFFECT_FLAG_ICE_REFLECTION,
        [REFL_TYPE_WATER - 1] = GROUND_EFFECT_FLAG_WATER_REFLECTION
    };
    u8 reflType = ObjectEventGetNearbyReflectionType(objEvent);

    if (reflType)
    {
        if (objEvent->hasReflection == 0)
        {
            objEvent->hasReflection++;
            *flags |= reflectionFlags[reflType - 1];
        }
    }
    else
    {
        objEvent->hasReflection = FALSE;
    }
}

static void GetGroundEffectFlags_TallGrassOnSpawn(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsTallGrass(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_TALL_GRASS_ON_SPAWN;
}

static void GetGroundEffectFlags_TallGrassOnBeginStep(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsTallGrass(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_TALL_GRASS_ON_MOVE;
}

static void GetGroundEffectFlags_LongGrassOnSpawn(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsLongGrass(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_LONG_GRASS_ON_SPAWN;
}

static void GetGroundEffectFlags_LongGrassOnBeginStep(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsLongGrass(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_LONG_GRASS_ON_MOVE;
}

static void GetGroundEffectFlags_Tracks(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsDeepSand(objEvent->previousMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_DEEP_SAND;
    else if (MetatileBehavior_IsSandOrDeepSand(objEvent->previousMetatileBehavior)
             || MetatileBehavior_IsFootprints(objEvent->previousMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_SAND;
}

static void GetGroundEffectFlags_SandHeap(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsDeepSand(objEvent->currentMetatileBehavior)
        && MetatileBehavior_IsDeepSand(objEvent->previousMetatileBehavior))
    {
        if (!objEvent->inSandPile)
        {
            objEvent->inSandPile = FALSE;
            objEvent->inSandPile = TRUE;
            *flags |= GROUND_EFFECT_FLAG_SAND_PILE;
        }
    }
    else
    {
        objEvent->inSandPile = FALSE;
    }
}

static void GetGroundEffectFlags_ShallowFlowingWater(struct ObjectEvent *objEvent, u32 *flags)
{
    if ((MetatileBehavior_IsShallowFlowingWater(objEvent->currentMetatileBehavior)
         && MetatileBehavior_IsShallowFlowingWater(objEvent->previousMetatileBehavior))
        || (MetatileBehavior_IsPacifidlogLog(objEvent->currentMetatileBehavior)
            && MetatileBehavior_IsPacifidlogLog(objEvent->previousMetatileBehavior)))
    {
        if (!objEvent->inShallowFlowingWater)
        {
            objEvent->inShallowFlowingWater = FALSE;
            objEvent->inShallowFlowingWater = TRUE;
            *flags |= GROUND_EFFECT_FLAG_SHALLOW_FLOWING_WATER;
        }
    }
    else
    {
        objEvent->inShallowFlowingWater = FALSE;
    }
}

static void GetGroundEffectFlags_Puddle(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsPuddle(objEvent->currentMetatileBehavior)
        && MetatileBehavior_IsPuddle(objEvent->previousMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_PUDDLE;
}

static void GetGroundEffectFlags_Ripple(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_HasRipples(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_RIPPLES;
}

static void GetGroundEffectFlags_ShortGrass(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsShortGrass(objEvent->currentMetatileBehavior)
        && MetatileBehavior_IsShortGrass(objEvent->previousMetatileBehavior))
    {
        if (!objEvent->inShortGrass)
        {
            objEvent->inShortGrass = FALSE;
            objEvent->inShortGrass = TRUE;
            *flags |= GROUND_EFFECT_FLAG_SHORT_GRASS;
        }
    }
    else
    {
        objEvent->inShortGrass = FALSE;
    }
}

static void GetGroundEffectFlags_HotSprings(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsHotSprings(objEvent->currentMetatileBehavior)
        && MetatileBehavior_IsHotSprings(objEvent->previousMetatileBehavior))
    {
        if (!objEvent->inHotSprings)
        {
            objEvent->inHotSprings = FALSE;
            objEvent->inHotSprings = TRUE;
            *flags |= GROUND_EFFECT_FLAG_HOT_SPRINGS;
        }
    }
    else
    {
        objEvent->inHotSprings = FALSE;
    }
}

static void GetGroundEffectFlags_Seaweed(struct ObjectEvent *objEvent, u32 *flags)
{
    if (MetatileBehavior_IsSeaweed(objEvent->currentMetatileBehavior))
        *flags |= GROUND_EFFECT_FLAG_SEAWEED;
}

static void GetGroundEffectFlags_JumpLanding(struct ObjectEvent *objEvent, u32 *flags)
{
    typedef bool8 (*MetatileFunc)(u8);

    static const MetatileFunc metatileFuncs[] = {
        MetatileBehavior_IsTallGrass,
        MetatileBehavior_IsLongGrass,
        MetatileBehavior_IsPuddle,
        MetatileBehavior_IsSurfableWaterOrUnderwater,
        MetatileBehavior_IsShallowFlowingWater,
        MetatileBehavior_IsATile,
    };

    static const u32 jumpLandingFlags[] = {
        GROUND_EFFECT_FLAG_LAND_IN_TALL_GRASS,
        GROUND_EFFECT_FLAG_LAND_IN_LONG_GRASS,
        GROUND_EFFECT_FLAG_LAND_IN_SHALLOW_WATER,
        GROUND_EFFECT_FLAG_LAND_IN_DEEP_WATER,
        GROUND_EFFECT_FLAG_LAND_IN_SHALLOW_WATER,
        GROUND_EFFECT_FLAG_LAND_ON_NORMAL_GROUND,
    };

    if (objEvent->landingJump && !objEvent->disableJumpLandingGroundEffect)
    {
        u8 i;

        for (i = 0; i < ARRAY_COUNT(metatileFuncs); i++)
        {
            if (metatileFuncs[i](objEvent->currentMetatileBehavior))
            {
                *flags |= jumpLandingFlags[i];
                return;
            }
        }
    }
}

#define RETURN_REFLECTION_TYPE_AT(x, y)              \
    b = MapGridGetMetatileBehaviorAt(x, y);          \
    result = GetReflectionTypeByMetatileBehavior(b); \
    if (result != REFL_TYPE_NONE)                    \
        return result;

static u8 ObjectEventGetNearbyReflectionType(struct ObjectEvent *objEvent)
{
    const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(objEvent->graphicsId);

    // ceil div by tile width?
    s16 width = (info->width + 8) >> 4;
    s16 height = (info->height + 8) >> 4;
    s16 i, j;
    u8 result, b; // used by RETURN_REFLECTION_TYPE_AT
    s16 one = 1;

    for (i = 0; i < height; i++)
    {
        RETURN_REFLECTION_TYPE_AT(objEvent->currentCoords.x, objEvent->currentCoords.y + one + i)
        RETURN_REFLECTION_TYPE_AT(objEvent->previousCoords.x, objEvent->previousCoords.y + one + i)
        for (j = 1; j < width; j++)
        {
            RETURN_REFLECTION_TYPE_AT(objEvent->currentCoords.x + j, objEvent->currentCoords.y + one + i)
            RETURN_REFLECTION_TYPE_AT(objEvent->currentCoords.x - j, objEvent->currentCoords.y + one + i)
            RETURN_REFLECTION_TYPE_AT(objEvent->previousCoords.x + j, objEvent->previousCoords.y + one + i)
            RETURN_REFLECTION_TYPE_AT(objEvent->previousCoords.x - j, objEvent->previousCoords.y + one + i)
        }
    }

    return REFL_TYPE_NONE;
}

#undef RETURN_REFLECTION_TYPE_AT

static u8 GetReflectionTypeByMetatileBehavior(u32 behavior)
{
    if (MetatileBehavior_IsIce(behavior))
        return REFL_TYPE_ICE;
    else if (MetatileBehavior_IsReflective(behavior))
        return REFL_TYPE_WATER;
    else
        return REFL_TYPE_NONE;
}

u8 GetLedgeJumpDirection(s16 x, s16 y, u8 direction)
{
    static bool8 (*const ledgeBehaviorFuncs[])(u8) = {
        [DIR_SOUTH - 1] = MetatileBehavior_IsJumpSouth,
        [DIR_NORTH - 1] = MetatileBehavior_IsJumpNorth,
        [DIR_WEST - 1]  = MetatileBehavior_IsJumpWest,
        [DIR_EAST - 1]  = MetatileBehavior_IsJumpEast,
    };

    u8 behavior;
    u8 index = direction;

    if (index == DIR_NONE)
        return DIR_NONE;
    else if (index > DIR_EAST)
        index -= DIR_EAST;

    index--;
    behavior = MapGridGetMetatileBehaviorAt(x, y);

    if (ledgeBehaviorFuncs[index](behavior) == TRUE)
        return index + 1;

    return DIR_NONE;
}

static void SetObjectEventSpriteOamTableForLongGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    if (objEvent->disableCoveringGroundEffects)
        return;

    if (!MetatileBehavior_IsLongGrass(objEvent->currentMetatileBehavior))
        return;

    if (!MetatileBehavior_IsLongGrass(objEvent->previousMetatileBehavior))
        return;

    sprite->subspriteTableNum = 4;

    if (ElevationToPriority(objEvent->previousElevation) == 1)
        sprite->subspriteTableNum = 5;
}

static bool8 IsElevationMismatchAt(u8 elevation, s16 x, s16 y)
{
    u8 mapElevation;

    if (elevation == 0)
        return FALSE;

    mapElevation = MapGridGetElevationAt(x, y);

    if (mapElevation == 0 || mapElevation == 15)
        return FALSE;

    if (mapElevation != elevation)
        return TRUE;

    return FALSE;
}

static const u8 sElevationToSubpriority[] = {
    115, 115, 83, 115, 83, 115, 83, 115, 83, 115, 83, 115, 83, 0, 0, 115
};

static const u8 sElevationToPriority[] = {
    2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 0, 0, 2
};

static const u8 sElevationToSubspriteTableNum[] = {
    1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0, 0, 1,
};

static void UpdateObjectEventElevationAndPriority(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    if (objEvent->fixedPriority)
        return;

    ObjectEventUpdateElevation(objEvent);

    sprite->subspriteTableNum = sElevationToSubspriteTableNum[objEvent->previousElevation];
    sprite->oam.priority = sElevationToPriority[objEvent->previousElevation];
}

static void InitObjectPriorityByElevation(struct Sprite *sprite, u8 elevation)
{
    sprite->subspriteTableNum = sElevationToSubspriteTableNum[elevation];
    sprite->oam.priority = sElevationToPriority[elevation];
}

u8 ElevationToPriority(u8 elevation)
{
    return sElevationToPriority[elevation];
}

void ObjectEventUpdateElevation(struct ObjectEvent *objEvent)
{
    u8 curElevation = MapGridGetElevationAt(objEvent->currentCoords.x, objEvent->currentCoords.y);
    u8 prevElevation = MapGridGetElevationAt(objEvent->previousCoords.x, objEvent->previousCoords.y);

    if (curElevation == 15 || prevElevation == 15)
        return;

    objEvent->currentElevation = curElevation;

    if (curElevation != 0 && curElevation != 15)
        objEvent->previousElevation = curElevation;
}

void SetObjectSubpriorityByElevation(u8 elevation, struct Sprite *sprite, u8 subpriority)
{
    u16 y = (sprite->y - sprite->centerToCornerVecY + gSpriteCoordOffsetY + 8) & 0xFF;
    y = (16 - (y >> 4)) << 1;

    sprite->subpriority = sElevationToSubpriority[elevation] + y + subpriority;
}

static void ObjectEventUpdateSubpriority(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    if (objEvent->fixedPriority)
        return;

    SetObjectSubpriorityByElevation(objEvent->previousElevation, sprite, 1);
}

static bool8 AreElevationsCompatible(u8 a, u8 b)
{
    if (a == 0 || b == 0)
        return TRUE;

    if (a != b)
        return FALSE;

    return TRUE;
}

void GroundEffect_SpawnOnTallGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2; // priority
    gFieldEffectArguments[4] = objEvent->localId << 8 | objEvent->mapNum;
    gFieldEffectArguments[5] = objEvent->mapGroup;
    gFieldEffectArguments[6] = (u8)gSaveBlock1Ptr->location.mapNum << 8 | (u8)gSaveBlock1Ptr->location.mapGroup;
    gFieldEffectArguments[7] = TRUE; // skip to end of anim
    FieldEffectStart(FLDEFF_TALL_GRASS);
}

void GroundEffect_StepOnTallGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2; // priority
    gFieldEffectArguments[4] = objEvent->localId << 8 | objEvent->mapNum;
    gFieldEffectArguments[5] = objEvent->mapGroup;
    gFieldEffectArguments[6] = (u8)gSaveBlock1Ptr->location.mapNum << 8 | (u8)gSaveBlock1Ptr->location.mapGroup;
    gFieldEffectArguments[7] = FALSE; // don't skip to end of anim
    FieldEffectStart(FLDEFF_TALL_GRASS);
}

void GroundEffect_SpawnOnLongGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2;
    gFieldEffectArguments[4] = objEvent->localId << 8 | objEvent->mapNum;
    gFieldEffectArguments[5] = objEvent->mapGroup;
    gFieldEffectArguments[6] = (u8)gSaveBlock1Ptr->location.mapNum << 8 | (u8)gSaveBlock1Ptr->location.mapGroup;
    gFieldEffectArguments[7] = 1;
    FieldEffectStart(FLDEFF_LONG_GRASS);
}

void GroundEffect_StepOnLongGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2;
    gFieldEffectArguments[4] = (objEvent->localId << 8) | objEvent->mapNum;
    gFieldEffectArguments[5] = objEvent->mapGroup;
    gFieldEffectArguments[6] = (u8)gSaveBlock1Ptr->location.mapNum << 8 | (u8)gSaveBlock1Ptr->location.mapGroup;
    gFieldEffectArguments[7] = 0;
    FieldEffectStart(FLDEFF_LONG_GRASS);
}

void GroundEffect_WaterReflection(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    SetUpReflection(objEvent, sprite, FALSE);
}

void GroundEffect_IceReflection(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    SetUpReflection(objEvent, sprite, TRUE);
}

void GroundEffect_FlowingWater(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    StartFieldEffectForObjectEvent(FLDEFF_FEET_IN_FLOWING_WATER, objEvent);
}

static void (*const sGroundEffectTracksFuncs[])(struct ObjectEvent *objEvent, struct Sprite *sprite, bool8 isDeepSand) = {
    [TRACKS_NONE] = DoTracksGroundEffect_None,
    [TRACKS_FOOT] = DoTracksGroundEffect_Footprints,
    [TRACKS_BIKE_TIRE] = DoTracksGroundEffect_BikeTireTracks,
};

void GroundEffect_SandTracks(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(objEvent->graphicsId);
    sGroundEffectTracksFuncs[info->tracks](objEvent, sprite, FALSE);
}

void GroundEffect_DeepSandTracks(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(objEvent->graphicsId);
    sGroundEffectTracksFuncs[info->tracks](objEvent, sprite, TRUE);
}

static void DoTracksGroundEffect_None(struct ObjectEvent *objEvent, struct Sprite *sprite, bool8 isDeepSand)
{
}

static void DoTracksGroundEffect_Footprints(struct ObjectEvent *objEvent, struct Sprite *sprite, bool8 isDeepSand)
{
    // First half-word is a Field Effect script id. (gFieldEffectScriptPointers)
    u16 sandFootprints_FieldEffectData[2] = {
        FLDEFF_SAND_FOOTPRINTS,
        FLDEFF_DEEP_SAND_FOOTPRINTS
    };

    gFieldEffectArguments[0] = objEvent->previousCoords.x;
    gFieldEffectArguments[1] = objEvent->previousCoords.y;
    gFieldEffectArguments[2] = 149;
    gFieldEffectArguments[3] = 2;
    gFieldEffectArguments[4] = objEvent->facingDirection;
    FieldEffectStart(sandFootprints_FieldEffectData[isDeepSand]);
}

static void DoTracksGroundEffect_BikeTireTracks(struct ObjectEvent *objEvent, struct Sprite *sprite, bool8 isDeepSand)
{
    //  Specifies which bike track shape to show next.
    //  For example, when the bike turns from up to right, it will show
    //  a track that curves to the right.
    //  Each 4-byte row corresponds to the initial direction of the bike, and
    //  each byte in that row is for the next direction of the bike in the order
    //  of down, up, left, right.
    static const u8 bikeTireTracks_Transitions[4][4] = {
        {1, 2, 7, 8},
        {1, 2, 6, 5},
        {5, 8, 3, 4},
        {6, 7, 3, 4},
    };

    if (objEvent->currentCoords.x != objEvent->previousCoords.x || objEvent->currentCoords.y != objEvent->previousCoords.y)
    {
        gFieldEffectArguments[0] = objEvent->previousCoords.x;
        gFieldEffectArguments[1] = objEvent->previousCoords.y;
        gFieldEffectArguments[2] = 149;
        gFieldEffectArguments[3] = 2;
        gFieldEffectArguments[4] =
            bikeTireTracks_Transitions[objEvent->previousMovementDirection][objEvent->facingDirection - 5];
        FieldEffectStart(FLDEFF_BIKE_TIRE_TRACKS);
    }
}

void GroundEffect_Ripple(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    DoRippleFieldEffect(objEvent, sprite);
}

void GroundEffect_StepOnPuddle(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    StartFieldEffectForObjectEvent(FLDEFF_SPLASH, objEvent);
}

void GroundEffect_SandHeap(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    StartFieldEffectForObjectEvent(FLDEFF_SAND_PILE, objEvent);
}

void GroundEffect_JumpOnTallGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    u8 spriteId;

    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2;
    FieldEffectStart(FLDEFF_JUMP_TALL_GRASS);

    spriteId = FindTallGrassFieldEffectSpriteId(
        objEvent->localId,
        objEvent->mapNum,
        objEvent->mapGroup,
        objEvent->currentCoords.x,
        objEvent->currentCoords.y);

    if (spriteId == MAX_SPRITES)
        GroundEffect_SpawnOnTallGrass(objEvent, sprite);
}

void GroundEffect_JumpOnLongGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = 2;
    FieldEffectStart(FLDEFF_JUMP_LONG_GRASS);
}

void GroundEffect_JumpOnShallowWater(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = sprite->oam.priority;
    FieldEffectStart(FLDEFF_JUMP_SMALL_SPLASH);
}

void GroundEffect_JumpOnWater(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = sprite->oam.priority;
    FieldEffectStart(FLDEFF_JUMP_BIG_SPLASH);
}

void GroundEffect_JumpLandingDust(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = objEvent->previousElevation;
    gFieldEffectArguments[3] = sprite->oam.priority;
    FieldEffectStart(FLDEFF_DUST);
}

void GroundEffect_ShortGrass(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    StartFieldEffectForObjectEvent(FLDEFF_SHORT_GRASS, objEvent);
}

void GroundEffect_HotSprings(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    StartFieldEffectForObjectEvent(FLDEFF_HOT_SPRINGS_WATER, objEvent);
}

void GroundEffect_Seaweed(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    FieldEffectStart(FLDEFF_BUBBLES);
}

static void (*const sGroundEffectFuncs[])(struct ObjectEvent *objEvent, struct Sprite *sprite) = {
    GroundEffect_SpawnOnTallGrass,      // GROUND_EFFECT_FLAG_TALL_GRASS_ON_SPAWN
    GroundEffect_StepOnTallGrass,       // GROUND_EFFECT_FLAG_TALL_GRASS_ON_MOVE
    GroundEffect_SpawnOnLongGrass,      // GROUND_EFFECT_FLAG_LONG_GRASS_ON_SPAWN
    GroundEffect_StepOnLongGrass,       // GROUND_EFFECT_FLAG_LONG_GRASS_ON_MOVE
    GroundEffect_WaterReflection,       // GROUND_EFFECT_FLAG_WATER_REFLECTION
    GroundEffect_IceReflection,         // GROUND_EFFECT_FLAG_ICE_REFLECTION
    GroundEffect_FlowingWater,          // GROUND_EFFECT_FLAG_SHALLOW_FLOWING_WATER
    GroundEffect_SandTracks,            // GROUND_EFFECT_FLAG_SAND
    GroundEffect_DeepSandTracks,        // GROUND_EFFECT_FLAG_DEEP_SAND
    GroundEffect_Ripple,                // GROUND_EFFECT_FLAG_RIPPLES
    GroundEffect_StepOnPuddle,          // GROUND_EFFECT_FLAG_PUDDLE
    GroundEffect_SandHeap,              // GROUND_EFFECT_FLAG_SAND_PILE
    GroundEffect_JumpOnTallGrass,       // GROUND_EFFECT_FLAG_LAND_IN_TALL_GRASS
    GroundEffect_JumpOnLongGrass,       // GROUND_EFFECT_FLAG_LAND_IN_LONG_GRASS
    GroundEffect_JumpOnShallowWater,    // GROUND_EFFECT_FLAG_LAND_IN_SHALLOW_WATER
    GroundEffect_JumpOnWater,           // GROUND_EFFECT_FLAG_LAND_IN_DEEP_WATER
    GroundEffect_JumpLandingDust,       // GROUND_EFFECT_FLAG_LAND_ON_NORMAL_GROUND
    GroundEffect_ShortGrass,            // GROUND_EFFECT_FLAG_SHORT_GRASS
    GroundEffect_HotSprings,            // GROUND_EFFECT_FLAG_HOT_SPRINGS
    GroundEffect_Seaweed                // GROUND_EFFECT_FLAG_SEAWEED
};

static void DoFlaggedGroundEffects(struct ObjectEvent *objEvent, struct Sprite *sprite, u32 flags)
{
    u8 i;

    if (ObjectEventIsFarawayIslandMew(objEvent) == TRUE && !ShouldMewShakeGrass(objEvent))
        return;

    for (i = 0; i < ARRAY_COUNT(sGroundEffectFuncs); i++, flags >>= 1)
        if (flags & 1)
            sGroundEffectFuncs[i](objEvent, sprite);
}

void filters_out_some_ground_effects(struct ObjectEvent *objEvent, u32 *flags)
{
    if (objEvent->disableCoveringGroundEffects)
    {
        objEvent->inShortGrass = 0;
        objEvent->inSandPile = 0;
        objEvent->inShallowFlowingWater = 0;
        objEvent->inHotSprings = 0;
        *flags &= ~(GROUND_EFFECT_FLAG_HOT_SPRINGS
                  | GROUND_EFFECT_FLAG_SHORT_GRASS
                  | GROUND_EFFECT_FLAG_SAND_PILE
                  | GROUND_EFFECT_FLAG_SHALLOW_FLOWING_WATER
                  | GROUND_EFFECT_FLAG_TALL_GRASS_ON_MOVE);
    }
}

void FilterOutStepOnPuddleGroundEffectIfJumping(struct ObjectEvent *objEvent, u32 *flags)
{
    if (objEvent->landingJump)
        *flags &= ~GROUND_EFFECT_FLAG_PUDDLE;
}

static void DoGroundEffects_OnSpawn(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    u32 flags;

#ifdef BUGFIX
    if (objEvent->triggerGroundEffectsOnMove && objEvent->localId != OBJ_EVENT_ID_CAMERA)
#else
    if (objEvent->triggerGroundEffectsOnMove)
#endif
    {
        flags = 0;
        UpdateObjectEventElevationAndPriority(objEvent, sprite);
        GetAllGroundEffectFlags_OnSpawn(objEvent, &flags);
        SetObjectEventSpriteOamTableForLongGrass(objEvent, sprite);
        DoFlaggedGroundEffects(objEvent, sprite, flags);
        objEvent->triggerGroundEffectsOnMove = FALSE;
        objEvent->disableCoveringGroundEffects = 0;
    }
}

static void DoGroundEffects_OnBeginStep(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    u32 flags;

#ifdef BUGFIX
    if (objEvent->triggerGroundEffectsOnMove && objEvent->localId != OBJ_EVENT_ID_CAMERA)
#else
    if (objEvent->triggerGroundEffectsOnMove)
#endif
    {
        flags = 0;
        UpdateObjectEventElevationAndPriority(objEvent, sprite);
        GetAllGroundEffectFlags_OnBeginStep(objEvent, &flags);
        SetObjectEventSpriteOamTableForLongGrass(objEvent, sprite);
        filters_out_some_ground_effects(objEvent, &flags);
        DoFlaggedGroundEffects(objEvent, sprite, flags);
        objEvent->triggerGroundEffectsOnMove = FALSE;
        objEvent->disableCoveringGroundEffects = 0;
    }
}

static void DoGroundEffects_OnFinishStep(struct ObjectEvent *objEvent, struct Sprite *sprite)
{
    u32 flags;

#ifdef BUGFIX
    if (objEvent->triggerGroundEffectsOnStop && objEvent->localId != OBJ_EVENT_ID_CAMERA)
#else
    if (objEvent->triggerGroundEffectsOnStop)
#endif
    {
        flags = 0;
        UpdateObjectEventElevationAndPriority(objEvent, sprite);
        GetAllGroundEffectFlags_OnFinishStep(objEvent, &flags);
        SetObjectEventSpriteOamTableForLongGrass(objEvent, sprite);
        FilterOutStepOnPuddleGroundEffectIfJumping(objEvent, &flags);
        DoFlaggedGroundEffects(objEvent, sprite, flags);
        objEvent->triggerGroundEffectsOnStop = 0;
        objEvent->landingJump = 0;
    }
}

bool8 FreezeObjectEvent(struct ObjectEvent *objectEvent)
{
    if (objectEvent->heldMovementActive || objectEvent->frozen)
    {
        return TRUE;
    }
    else
    {
        objectEvent->frozen = TRUE;
        objectEvent->spriteAnimPausedBackup = gSprites[objectEvent->spriteId].animPaused;
        objectEvent->spriteAffineAnimPausedBackup = gSprites[objectEvent->spriteId].affineAnimPaused;
        gSprites[objectEvent->spriteId].animPaused = TRUE;
        gSprites[objectEvent->spriteId].affineAnimPaused = TRUE;
        return FALSE;
    }
}

void FreezeObjectEvents(void)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        if (gObjectEvents[i].active && i != gPlayerAvatar.objectEventId)
            FreezeObjectEvent(&gObjectEvents[i]);
}

void FreezeObjectEventsExceptOne(u8 objectEventId)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        if (i != objectEventId && gObjectEvents[i].active && i != gPlayerAvatar.objectEventId)
            FreezeObjectEvent(&gObjectEvents[i]);
}

void UnfreezeObjectEvent(struct ObjectEvent *objectEvent)
{
    if (objectEvent->active && objectEvent->frozen)
    {
        objectEvent->frozen = 0;
        gSprites[objectEvent->spriteId].animPaused = objectEvent->spriteAnimPausedBackup;
        gSprites[objectEvent->spriteId].affineAnimPaused = objectEvent->spriteAffineAnimPausedBackup;
    }
}

void UnfreezeObjectEvents(void)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        if (gObjectEvents[i].active)
            UnfreezeObjectEvent(&gObjectEvents[i]);
}

static void Step1(struct Sprite *sprite, u8 dir)
{
    sprite->x += sDirectionToVectors[dir].x;
    sprite->y += sDirectionToVectors[dir].y;
}

static void Step2(struct Sprite *sprite, u8 dir)
{
    sprite->x += 2 * (u16) sDirectionToVectors[dir].x;
    sprite->y += 2 * (u16) sDirectionToVectors[dir].y;
}

static void Step3(struct Sprite *sprite, u8 dir)
{
    sprite->x += 2 * (u16) sDirectionToVectors[dir].x + (u16) sDirectionToVectors[dir].x;
    sprite->y += 2 * (u16) sDirectionToVectors[dir].y + (u16) sDirectionToVectors[dir].y;
}

static void Step4(struct Sprite *sprite, u8 dir)
{
    sprite->x += 4 * (u16) sDirectionToVectors[dir].x;
    sprite->y += 4 * (u16) sDirectionToVectors[dir].y;
}

static void Step8(struct Sprite *sprite, u8 dir)
{
    sprite->x += 8 * (u16) sDirectionToVectors[dir].x;
    sprite->y += 8 * (u16) sDirectionToVectors[dir].y;
}

#define sSpeed data[4]
#define sTimer data[5]

static void SetSpriteDataForNormalStep(struct Sprite *sprite, u8 direction, u8 speed)
{
    sprite->sDirection = direction;
    sprite->sSpeed = speed;
    sprite->sTimer = 0;
}

typedef void (*SpriteStepFunc)(struct Sprite *sprite, u8 direction);

static const SpriteStepFunc sStep1Funcs[] = {
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
    Step1,
};

static const SpriteStepFunc sStep2Funcs[] = {
    Step2,
    Step2,
    Step2,
    Step2,
    Step2,
    Step2,
    Step2,
    Step2,
};

static const SpriteStepFunc sStep3Funcs[] = {
    Step2,
    Step3,
    Step3,
    Step2,
    Step3,
    Step3,
};

static const SpriteStepFunc sStep4Funcs[] = {
    Step4,
    Step4,
    Step4,
    Step4,
};

static const SpriteStepFunc sStep8Funcs[] = {
    Step8,
    Step8,
};

static const SpriteStepFunc *const sNpcStepFuncTables[] = {
    [MOVE_SPEED_NORMAL] = sStep1Funcs,
    [MOVE_SPEED_FAST_1] = sStep2Funcs,
    [MOVE_SPEED_FAST_2] = sStep3Funcs,
    [MOVE_SPEED_FASTER] = sStep4Funcs,
    [MOVE_SPEED_FASTEST] = sStep8Funcs,
};

static const s16 sStepTimes[] = {
    [MOVE_SPEED_NORMAL] = ARRAY_COUNT(sStep1Funcs),
    [MOVE_SPEED_FAST_1] = ARRAY_COUNT(sStep2Funcs),
    [MOVE_SPEED_FAST_2] = ARRAY_COUNT(sStep3Funcs),
    [MOVE_SPEED_FASTER] = ARRAY_COUNT(sStep4Funcs),
    [MOVE_SPEED_FASTEST] = ARRAY_COUNT(sStep8Funcs),
};

static bool8 NpcTakeStep(struct Sprite *sprite)
{
    if (sprite->sTimer >= sStepTimes[sprite->sSpeed])
        return FALSE;

    sNpcStepFuncTables[sprite->sSpeed][sprite->sTimer](sprite, sprite->sDirection);

    sprite->sTimer++;

    if (sprite->sTimer < sStepTimes[sprite->sSpeed])
        return FALSE;

    return TRUE;
}

#undef sSpeed
#undef sTimer

#define sTimer     data[4]
#define sNumSteps  data[5]

static void SetWalkSlowSpriteData(struct Sprite *sprite, u8 direction)
{
    sprite->sDirection = direction;
    sprite->sTimer = 0;
    sprite->sNumSteps = 0;
}

static bool8 UpdateWalkSlowAnim(struct Sprite *sprite)
{
    if (!(sprite->sTimer & 1))
    {
        Step1(sprite, sprite->sDirection);
        sprite->sNumSteps++;
    }

    sprite->sTimer++;

    if (sprite->sNumSteps > 15)
        return TRUE;
    else
        return FALSE;
}

#undef sTimer
#undef sNumSteps

static const s8 sFigure8XOffsets[FIGURE_8_LENGTH] = {
    1, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 2, 2, 1, 2,
    2, 1, 2, 2, 1, 2, 1, 1,
    2, 1, 1, 2, 1, 1, 2, 1,
    1, 2, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 0, 1, 1, 0,
    1, 0, 1, 0, 1, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
};

static const s8 sFigure8YOffsets[FIGURE_8_LENGTH] = {
     0,  0,  1,  0,  0,  1,  0,  0,
     1,  0,  1,  1,  0,  1,  1,  0,
     1,  1,  0,  1,  1,  0,  1,  1,
     0,  0,  1,  0,  0,  1,  0,  0,
     1,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0, -1,  0,  0, -1,  0,  0,
    -1,  0, -1, -1,  0, -1, -1,  0,
    -1, -1, -1, -1, -1, -1, -1, -2,
};

s16 GetFigure8YOffset(s16 idx)
{
    return sFigure8YOffsets[idx];
}

s16 GetFigure8XOffset(s16 idx)
{
    return sFigure8XOffsets[idx];
}

static void InitSpriteForFigure8Anim(struct Sprite *sprite)
{
    sprite->data[6] = 0;
    sprite->data[7] = 0;
}

static bool8 AnimateSpriteInFigure8(struct Sprite *sprite)
{
    bool8 finished = FALSE;

    switch(sprite->data[7])
    {
    case 0:
        sprite->x2 += GetFigure8XOffset(sprite->data[6]);
        sprite->y2 += GetFigure8YOffset(sprite->data[6]);
        break;
    case 1:
        sprite->x2 -= GetFigure8XOffset((FIGURE_8_LENGTH - 1) - sprite->data[6]);
        sprite->y2 += GetFigure8YOffset((FIGURE_8_LENGTH - 1) - sprite->data[6]);
        break;
    case 2:
        sprite->x2 -= GetFigure8XOffset(sprite->data[6]);
        sprite->y2 += GetFigure8YOffset(sprite->data[6]);
        break;
    case 3:
        sprite->x2 += GetFigure8XOffset((FIGURE_8_LENGTH - 1) - sprite->data[6]);
        sprite->y2 += GetFigure8YOffset((FIGURE_8_LENGTH - 1) - sprite->data[6]);
        break;
    }
    if (++sprite->data[6] == FIGURE_8_LENGTH)
    {
        sprite->data[6] = 0;
        sprite->data[7]++;
    }
    if (sprite->data[7] == 4)
    {
        sprite->y2 = 0;
        sprite->x2 = 0;
        finished = TRUE;
    }
    return finished;
}

static const s8 sJumpY_High[] = {
     -4,  -6,  -8, -10, -11, -12, -12, -12,
    -11, -10,  -9,  -8,  -6,  -4,   0,   0
};

static const s8 sJumpY_Low[] = {
    0,   -2,  -3,  -4,  -5,  -6,  -6,  -6,
    -5,  -5,  -4,  -3,  -2,   0,   0,   0
};

static const s8 sJumpY_Normal[] = {
    -2,  -4,  -6,  -8,  -9, -10, -10, -10,
    -9,  -8,  -6,  -5,  -3,  -2,   0,   0
};

static const s8 *const sJumpYTable[] = {
    [JUMP_TYPE_HIGH]   = sJumpY_High,
    [JUMP_TYPE_LOW]    = sJumpY_Low,
    [JUMP_TYPE_NORMAL] = sJumpY_Normal
};

static s16 GetJumpY(s16 i, u8 type)
{
    return sJumpYTable[type][i];
}

#define sDistance  data[4]
#define sJumpType  data[5]
#define sTimer     data[6]

static void SetJumpSpriteData(struct Sprite *sprite, u8 direction, u8 distance, u8 type)
{
    sprite->sDirection = direction;
    sprite->sDistance = distance;
    sprite->sJumpType = type;
    sprite->sTimer = 0;
}

static u8 DoJumpSpriteMovement(struct Sprite *sprite)
{
    s16 distanceToTime[] = {
        [JUMP_DISTANCE_IN_PLACE] = 16,
        [JUMP_DISTANCE_NORMAL] = 16,
        [JUMP_DISTANCE_FAR] = 32,
    };
    u8 distanceToShift[] = {
        [JUMP_DISTANCE_IN_PLACE] = 0,
        [JUMP_DISTANCE_NORMAL] = 0,
        [JUMP_DISTANCE_FAR] = 1,
    };
    u8 result = 0;

    if (sprite->sDistance != JUMP_DISTANCE_IN_PLACE)
        Step1(sprite, sprite->sDirection);

    sprite->y2 = GetJumpY(sprite->sTimer >> distanceToShift[sprite->sDistance], sprite->sJumpType);

    sprite->sTimer++;

    if (sprite->sTimer == distanceToTime[sprite->sDistance] >> 1)
        result = JUMP_HALFWAY;

    if (sprite->sTimer >= distanceToTime[sprite->sDistance])
    {
        sprite->y2 = 0;
        result = JUMP_FINISHED;
    }

    return result;
}

static u8 DoJumpSpecialSpriteMovement(struct Sprite *sprite)
{
    s16 distanceToTime[] = {
        [JUMP_DISTANCE_IN_PLACE] = 32,
        [JUMP_DISTANCE_NORMAL] = 32,
        [JUMP_DISTANCE_FAR] = 64,
    };
    u8 distanceToShift[] = {
        [JUMP_DISTANCE_IN_PLACE] = 1,
        [JUMP_DISTANCE_NORMAL] = 1,
        [JUMP_DISTANCE_FAR] = 2,
    };
    u8 result = 0;

    if (sprite->sDistance != JUMP_DISTANCE_IN_PLACE && !(sprite->sTimer & 1))
        Step1(sprite, sprite->sDirection);

    sprite->y2 = GetJumpY(sprite->sTimer >> distanceToShift[sprite->sDistance], sprite->sJumpType);

    sprite->sTimer++;

    if (sprite->sTimer == distanceToTime[sprite->sDistance] >> 1)
        result = JUMP_HALFWAY;

    if (sprite->sTimer >= distanceToTime[sprite->sDistance])
    {
        sprite->y2 = 0;
        result = JUMP_FINISHED;
    }

    return result;
}

#undef sDistance
#undef sJumpType
#undef sTimer

static void SetMovementDelay(struct Sprite *sprite, s16 timer)
{
    sprite->data[3] = timer;
}

static bool8 WaitForMovementDelay(struct Sprite *sprite)
{
    if (--sprite->data[3] == 0)
        return TRUE;
    else
        return FALSE;
}

void SetAndStartSpriteAnim(struct Sprite *sprite, u8 animNum, u8 animCmdIndex)
{
    sprite->animNum = animNum;
    sprite->animPaused = FALSE;
    SeekSpriteAnim(sprite, animCmdIndex);
}

bool8 SpriteAnimEnded(struct Sprite *sprite)
{
    if (sprite->animEnded)
        return TRUE;
    else
        return FALSE;
}

void UpdateObjectEventSpriteInvisibility(struct Sprite *sprite, bool8 invisible)
{
    u16 x, y;
    s16 x2, y2;

    sprite->invisible = invisible;

    if (sprite->coordOffsetEnabled)
    {
        x = sprite->x + sprite->x2 + sprite->centerToCornerVecX + gSpriteCoordOffsetX;
        y = sprite->y + sprite->y2 + sprite->centerToCornerVecY + gSpriteCoordOffsetY;
    }
    else
    {
        x = sprite->x + sprite->x2 + sprite->centerToCornerVecX;
        y = sprite->y + sprite->y2 + sprite->centerToCornerVecY;
    }

    x2 = x - (sprite->centerToCornerVecX >> 1);
    y2 = y - (sprite->centerToCornerVecY >> 1);

    if ((s16)x >= DISPLAY_WIDTH + 16 || x2 < -16)
        sprite->invisible = TRUE;
    if ((s16)y >= DISPLAY_HEIGHT + 16 || y2 < -16)
        sprite->invisible = TRUE;
}

#define sInvisible     data[2]
#define sAnimNum       data[3]
#define sAnimState     data[4]

static void SpriteCB_VirtualObject(struct Sprite *sprite)
{
    VirtualObject_UpdateAnim(sprite);
    SetObjectSubpriorityByElevation(sprite->sVirtualObjElev, sprite, 1);
    UpdateObjectEventSpriteInvisibility(sprite, sprite->sInvisible);
}

static void UNUSED DestroyVirtualObjects(void)
{
    int i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        struct Sprite *sprite = &gSprites[i];
        if(sprite->inUse && sprite->callback == SpriteCB_VirtualObject)
            DestroySprite(sprite);
    }
}

static int GetVirtualObjectSpriteId(u8 virtualObjId)
{
    int i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        struct Sprite *sprite = &gSprites[i];
        if (sprite->inUse && sprite->callback == SpriteCB_VirtualObject && (u8)sprite->sVirtualObjId == virtualObjId)
            return i;
    }
    return MAX_SPRITES;
}

void TurnVirtualObject(u8 virtualObjId, u8 direction)
{
    u8 spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId != MAX_SPRITES)
        StartSpriteAnim(&gSprites[spriteId], GetFaceDirectionAnimNum(direction));
}

void SetVirtualObjectGraphics(u8 virtualObjId, u8 graphicsId)
{
    int spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[spriteId];
        const struct ObjectEventGraphicsInfo *graphicsInfo = GetObjectEventGraphicsInfo(graphicsId);
        u16 tileNum = sprite->oam.tileNum;

        sprite->oam = *graphicsInfo->oam;
        sprite->oam.tileNum = tileNum;
        sprite->oam.paletteNum = graphicsInfo->paletteSlot;
        sprite->images = graphicsInfo->images;

        if (graphicsInfo->subspriteTables == NULL)
        {
            sprite->subspriteTables = NULL;
            sprite->subspriteTableNum = 0;
            sprite->subspriteMode = SUBSPRITES_OFF;
        }
        else
        {
            SetSubspriteTables(sprite, graphicsInfo->subspriteTables);
            sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
        }
        StartSpriteAnim(sprite, 0);
    }
}

void SetVirtualObjectInvisibility(u8 virtualObjId, bool32 invisible)
{
    u8 spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId == MAX_SPRITES)
        return;

    if (invisible)
        gSprites[spriteId].sInvisible = TRUE;
    else
        gSprites[spriteId].sInvisible = FALSE;
}

bool32 IsVirtualObjectInvisible(u8 virtualObjId)
{
    u8 spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId == MAX_SPRITES)
        return FALSE;

    return (gSprites[spriteId].sInvisible == TRUE);
}

void SetVirtualObjectSpriteAnim(u8 virtualObjId, u8 animNum)
{
    u8 spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].sAnimNum = animNum;
        gSprites[spriteId].sAnimState = 0;
    }
}

static void MoveUnionRoomObjectUp(struct Sprite *sprite)
{
    switch(sprite->sAnimState)
    {
    case 0:
        sprite->y2 = 0;
        sprite->sAnimState++;
    case 1:
        sprite->y2 -= 8;
        if (sprite->y2 == -DISPLAY_HEIGHT)
        {
            sprite->y2 = 0;
            sprite->sInvisible = TRUE;
            sprite->sAnimNum = 0;
            sprite->sAnimState = 0;
        }
    }
}

static void MoveUnionRoomObjectDown(struct Sprite *sprite)
{
    switch(sprite->sAnimState)
    {
    case 0:
        sprite->y2 = -DISPLAY_HEIGHT;
        sprite->sAnimState++;
    case 1:
        sprite->y2 += 8;
        if(sprite->y2 == 0)
        {
            sprite->sAnimNum = 0;
            sprite->sAnimState = 0;
        }
    }
}

static void VirtualObject_UpdateAnim(struct Sprite *sprite)
{
    switch(sprite->sAnimNum)
    {
    case UNION_ROOM_SPAWN_IN:
        MoveUnionRoomObjectDown(sprite);
        break;
    case UNION_ROOM_SPAWN_OUT:
        MoveUnionRoomObjectUp(sprite);
        break;
    case 0:
        break;
    default:
        sprite->sAnimNum = 0;
        break;
    }
}

bool32 IsVirtualObjectAnimating(u8 virtualObjId)
{
    u8 spriteId = GetVirtualObjectSpriteId(virtualObjId);

    if (spriteId == MAX_SPRITES)
        return FALSE;

    if (gSprites[spriteId].sAnimNum != 0)
        return TRUE;

    return FALSE;
}

u32 StartFieldEffectForObjectEvent(u8 fieldEffectId, struct ObjectEvent *objectEvent)
{
    ObjectEventGetLocalIdAndMap(objectEvent, &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
    return FieldEffectStart(fieldEffectId);
}

static void DoShadowFieldEffect(struct ObjectEvent *objectEvent)
{
    if (!objectEvent->hasShadow)
    {
        objectEvent->hasShadow = TRUE;
        StartFieldEffectForObjectEvent(FLDEFF_SHADOW, objectEvent);
    }
}

static void DoRippleFieldEffect(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    const struct ObjectEventGraphicsInfo *graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);
    gFieldEffectArguments[0] = sprite->x;
    gFieldEffectArguments[1] = sprite->y + (graphicsInfo->height >> 1) - 2;
    gFieldEffectArguments[2] = 151;
    gFieldEffectArguments[3] = 3;
    FieldEffectStart(FLDEFF_RIPPLE);
}

u8 (*const gMovementActionFuncs_LockAnim[])(struct ObjectEvent *, struct Sprite *) = {
    MovementAction_LockAnim_Step0,
    MovementAction_Finish,
};

u8 (*const gMovementActionFuncs_UnlockAnim[])(struct ObjectEvent *, struct Sprite *) = {
    MovementAction_UnlockAnim_Step0,
    MovementAction_Finish,
};

u8 (*const gMovementActionFuncs_FlyUp[])(struct ObjectEvent *, struct Sprite *) = {
    MovementAction_FlyUp_Step0,
    MovementAction_FlyUp_Step1,
    MovementAction_Fly_Finish,
};

u8 (*const gMovementActionFuncs_FlyDown[])(struct ObjectEvent *, struct Sprite *) = {
    MovementAction_FlyDown_Step0,
    MovementAction_FlyDown_Step1,
    MovementAction_Fly_Finish,
};

u8 MovementAction_LockAnim_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    bool32 ableToStore = FALSE;
    if (sLockedAnimObjectEvents == NULL)
    {
        sLockedAnimObjectEvents = AllocZeroed(sizeof(struct LockedAnimObjectEvents));
        sLockedAnimObjectEvents->localIds[0] = objectEvent->localId;
        sLockedAnimObjectEvents->count = 1;
        ableToStore = TRUE;
    }
    else
    {
        u8 i;
        u8 firstFreeSlot = OBJECT_EVENTS_COUNT;
        bool32 found = FALSE;
        for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        {
            if (firstFreeSlot == OBJECT_EVENTS_COUNT && sLockedAnimObjectEvents->localIds[i] == 0)
                firstFreeSlot = i;

            if (sLockedAnimObjectEvents->localIds[i] == objectEvent->localId)
            {
                found = TRUE;
                break;
            }
        }

        if (!found && firstFreeSlot != OBJECT_EVENTS_COUNT)
        {
            sLockedAnimObjectEvents->localIds[firstFreeSlot] = objectEvent->localId;
            sLockedAnimObjectEvents->count++;
            ableToStore = TRUE;
        }
    }

    if (ableToStore == TRUE)
    {
        objectEvent->inanimate = TRUE;
        objectEvent->facingDirectionLocked = TRUE;
    }

    sprite->sActionFuncId = 1;
    return TRUE;
}

u8 MovementAction_UnlockAnim_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    bool32 ableToStore;
    u8 index;

    sprite->sActionFuncId = 1;
    if (sLockedAnimObjectEvents != NULL)
    {
        ableToStore = FALSE;
        index = FindLockedObjectEventIndex(objectEvent);
        if (index != OBJECT_EVENTS_COUNT)
        {
            sLockedAnimObjectEvents->localIds[index] = 0;
            sLockedAnimObjectEvents->count--;
            ableToStore = TRUE;
        }
        if (sLockedAnimObjectEvents->count == 0)
            FREE_AND_SET_NULL(sLockedAnimObjectEvents);
        if (ableToStore == TRUE)
        {
            objectEvent->inanimate = GetObjectEventGraphicsInfo(objectEvent->graphicsId)->inanimate;
            objectEvent->facingDirectionLocked = FALSE;
            sprite->animPaused = 0;
        }
    }

    return TRUE;
}

u8 FindLockedObjectEventIndex(struct ObjectEvent *objectEvent)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (sLockedAnimObjectEvents->localIds[i] == objectEvent->localId)
            return i;
    }
    return OBJECT_EVENTS_COUNT;
}

static void CreateLevitateMovementTask(struct ObjectEvent *objectEvent)
{
    u8 taskId = CreateTask(ApplyLevitateMovement, 0xFF);
    struct Task *task = &gTasks[taskId];

    StoreWordInTwoHalfwords(&task->data[0], (u32)objectEvent);
    objectEvent->warpArrowSpriteId = taskId;
    task->data[3] = 0xFFFF;
}

static void ApplyLevitateMovement(u8 taskId)
{
    struct ObjectEvent *objectEvent;
    struct Sprite *sprite;
    struct Task *task = &gTasks[taskId];

    LoadWordFromTwoHalfwords(&task->data[0], (u32 *)&objectEvent); // load the map object pointer.
    sprite = &gSprites[objectEvent->spriteId];

    if(!(task->data[2] & 3))
        sprite->y2 += task->data[3];

    if(!(task->data[2] & 15))
        task->data[3] = -task->data[3];

    task->data[2]++;
}

static void DestroyLevitateMovementTask(u8 taskId)
{
    struct ObjectEvent *objectEvent;
    struct Task *task = &gTasks[taskId];

    LoadWordFromTwoHalfwords(&task->data[0], (u32 *)&objectEvent); // unused objectEvent
    DestroyTask(taskId);
}

// Used to freeze other objects except two trainers approaching for battle
void FreezeObjectEventsExceptTwo(u8 objectEventId1, u8 objectEventId2)
{
    u8 i;

    for(i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if(i != objectEventId1 && i != objectEventId2 &&
            gObjectEvents[i].active && i != gPlayerAvatar.objectEventId)
                FreezeObjectEvent(&gObjectEvents[i]);
    }
}

u8 MovementAction_FlyUp_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->y2 = 0;
    sprite->sActionFuncId++;
    return FALSE;
}

u8 MovementAction_FlyUp_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->y2 -= 8;

    if(sprite->y2 == -DISPLAY_HEIGHT)
        sprite->sActionFuncId++;
    return FALSE;
}

u8 MovementAction_FlyDown_Step0(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->y2 = -DISPLAY_HEIGHT;
    sprite->sActionFuncId++;
    return FALSE;
}

u8 MovementAction_FlyDown_Step1(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    sprite->y2 += 8;

    if(!sprite->y2)
        sprite->sActionFuncId++;
    return FALSE;
}

// though this function returns TRUE without doing anything, this header is required due to being in an array of functions which needs it.
u8 MovementAction_Fly_Finish(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    return TRUE;
}
