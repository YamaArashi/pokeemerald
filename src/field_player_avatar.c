#include "global.h"
#include "main.h"
#include "bike.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "menu.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "party_menu.h"
#include "random.h"
#include "rotating_gate.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "tv.h"
#include "wild_encounter.h"
#include "constants/abilities.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/field_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/trainer_types.h"

#define NUM_FORCED_MOVEMENTS 18
#define NUM_ACRO_BIKE_COLLISIONS 5

static EWRAM_DATA u8 sSpinStartFacingDir = 0;
EWRAM_DATA struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT] = {};
EWRAM_DATA struct PlayerAvatar gPlayerAvatar = {};

// static declarations

static u8 ObjectEventCB2_NoMovement2();
static bool8 TryInterruptObjectEventSpecialAnim(struct ObjectEvent *, u8);
static void npc_clear_strange_bits(struct ObjectEvent *);
static void MovePlayerAvatarUsingKeypadInput(u8, u16, u16);
static void PlayerAllowForcedMovementIfMovingSameDirection();
static bool8 TryDoMetatileBehaviorForcedMovement();
static u8 GetForcedMovementByMetatileBehavior();

static bool8 ForcedMovement_None(void);
static bool8 ForcedMovement_Slip(void);
static bool8 ForcedMovement_WalkSouth(void);
static bool8 ForcedMovement_WalkNorth(void);
static bool8 ForcedMovement_WalkWest(void);
static bool8 ForcedMovement_WalkEast(void);
static bool8 ForcedMovement_PushedSouthByCurrent(void);
static bool8 ForcedMovement_PushedNorthByCurrent(void);
static bool8 ForcedMovement_PushedWestByCurrent(void);
static bool8 ForcedMovement_PushedEastByCurrent(void);
static bool8 ForcedMovement_SlideSouth(void);
static bool8 ForcedMovement_SlideNorth(void);
static bool8 ForcedMovement_SlideWest(void);
static bool8 ForcedMovement_SlideEast(void);
static bool8 ForcedMovement_MatJump(void);
static bool8 ForcedMovement_MatSpin(void);
static bool8 ForcedMovement_MuddySlope(void);

static void MovePlayerNotOnBike(u8, u16);
static u8 CheckMovementInputNotOnBike(u8);
static void PlayerNotOnBikeNotMoving(u8, u16);
static void PlayerNotOnBikeTurningInPlace(u8, u16);
static void PlayerNotOnBikeMoving(u8, u16);
static u8 CheckForPlayerAvatarCollision(u8);
static u8 CheckForPlayerAvatarStaticCollision(u8);
static u8 CheckForObjectEventStaticCollision(struct ObjectEvent *, s16, s16, u8, u8);
static bool8 CanStopSurfing(s16, s16, u8);
static bool8 ShouldJumpLedge(s16, s16, u8);
static bool8 TryPushBoulder(s16, s16, u8);
static void CheckAcroBikeCollision(s16, s16, u8, u8 *);

static void DoPlayerAvatarTransition(void);
static void PlayerAvatarTransition_Dummy(struct ObjectEvent *);
static void PlayerAvatarTransition_Normal(struct ObjectEvent *);
static void PlayerAvatarTransition_MachBike(struct ObjectEvent *);
static void PlayerAvatarTransition_AcroBike(struct ObjectEvent *);
static void PlayerAvatarTransition_Surfing(struct ObjectEvent *);
static void PlayerAvatarTransition_Underwater(struct ObjectEvent *);
static void PlayerAvatarTransition_ReturnToField(struct ObjectEvent *);

static bool8 PlayerAnimIsMultiFrameStationary(void);
static bool8 PlayerAnimIsMultiFrameStationaryAndStateNotTurning(void);
static bool8 PlayerIsAnimActive(void);
static bool8 PlayerCheckIfAnimFinishedOrInactive(void);

static void PlayerRun(u8);
static void PlayerNotOnBikeCollide(u8);
static void PlayerNotOnBikeCollideWithFarawayIslandMew(u8);

static void PlayCollisionSoundIfNotFacingWarp(u8);

static void HideShowWarpArrow(struct ObjectEvent *);

static void StartStrengthAnim(u8, u8);
static void Task_PushBoulder(u8);
static bool8 PushBoulder_Start(struct Task *, struct ObjectEvent *, struct ObjectEvent *);
static bool8 PushBoulder_Move(struct Task *, struct ObjectEvent *, struct ObjectEvent *);
static bool8 PushBoulder_End(struct Task *, struct ObjectEvent *, struct ObjectEvent *);

static void DoPlayerMatJump(void);
static void DoPlayerAvatarSecretBaseMatJump(u8);
static u8 PlayerAvatar_DoSecretBaseMatJump(struct Task *, struct ObjectEvent *);

static void DoPlayerMatSpin(void);
static void PlayerAvatar_DoSecretBaseMatSpin(u8);
static bool8 PlayerAvatar_SecretBaseMatSpinStep0(struct Task *, struct ObjectEvent *);
static bool8 PlayerAvatar_SecretBaseMatSpinStep1(struct Task *, struct ObjectEvent *);
static bool8 PlayerAvatar_SecretBaseMatSpinStep2(struct Task *, struct ObjectEvent *);
static bool8 PlayerAvatar_SecretBaseMatSpinStep3(struct Task *, struct ObjectEvent *);

static void CreateStopSurfingTask(u8);
static void Task_StopSurfingInit(u8);
static void Task_WaitStopSurfing(u8);

static void Task_Fishing(u8);
static u8 Fishing_Init(struct Task *);
static u8 Fishing_GetRodOut(struct Task *);
static u8 Fishing_WaitBeforeDots(struct Task *);
static u8 Fishing_InitDots(struct Task *);
static u8 Fishing_ShowDots(struct Task *);
static u8 Fishing_CheckForBite(struct Task *);
static u8 Fishing_GotBite(struct Task *);
static u8 Fishing_WaitForA(struct Task *);
static u8 Fishing_CheckMoreDots(struct Task *);
static u8 Fishing_MonOnHook(struct Task *);
static u8 Fishing_StartEncounter(struct Task *);
static u8 Fishing_NotEvenNibble(struct Task *);
static u8 Fishing_GotAway(struct Task *);
static u8 Fishing_NoMon(struct Task *);
static u8 Fishing_PutRodAway(struct Task *);
static u8 Fishing_EndNoMon(struct Task *);
static void AlignFishingAnimationFrames(void);

static u8 TrySpinPlayerForWarp(struct ObjectEvent *, s16 *);

static bool8 (*const sForcedMovementTestFuncs[NUM_FORCED_MOVEMENTS])(u8) =
{
    MetatileBehavior_IsTrickHouseSlipperyFloor,
    MetatileBehavior_IsIce_2,
    MetatileBehavior_IsWalkSouth,
    MetatileBehavior_IsWalkNorth,
    MetatileBehavior_IsWalkWest,
    MetatileBehavior_IsWalkEast,
    MetatileBehavior_IsSouthwardCurrent,
    MetatileBehavior_IsNorthwardCurrent,
    MetatileBehavior_IsWestwardCurrent,
    MetatileBehavior_IsEastwardCurrent,
    MetatileBehavior_IsSlideSouth,
    MetatileBehavior_IsSlideNorth,
    MetatileBehavior_IsSlideWest,
    MetatileBehavior_IsSlideEast,
    MetatileBehavior_IsWaterfall,
    MetatileBehavior_IsSecretBaseJumpMat,
    MetatileBehavior_IsSecretBaseSpinMat,
    MetatileBehavior_IsMuddySlope,
};

// + 1 for ForcedMovement_None, which is excluded above
static bool8 (*const sForcedMovementFuncs[NUM_FORCED_MOVEMENTS + 1])(void) =
{
    ForcedMovement_None,
    ForcedMovement_Slip,
    ForcedMovement_Slip,
    ForcedMovement_WalkSouth,
    ForcedMovement_WalkNorth,
    ForcedMovement_WalkWest,
    ForcedMovement_WalkEast,
    ForcedMovement_PushedSouthByCurrent,
    ForcedMovement_PushedNorthByCurrent,
    ForcedMovement_PushedWestByCurrent,
    ForcedMovement_PushedEastByCurrent,
    ForcedMovement_SlideSouth,
    ForcedMovement_SlideNorth,
    ForcedMovement_SlideWest,
    ForcedMovement_SlideEast,
    ForcedMovement_PushedSouthByCurrent,
    ForcedMovement_MatJump,
    ForcedMovement_MatSpin,
    ForcedMovement_MuddySlope,
};

static void (*const sPlayerNotOnBikeFuncs[])(u8, u16) =
{
    [NOT_MOVING]     = PlayerNotOnBikeNotMoving,
    [TURN_DIRECTION] = PlayerNotOnBikeTurningInPlace,
    [MOVING]         = PlayerNotOnBikeMoving,
};

static bool8 (*const sAcroBikeTrickMetatiles[NUM_ACRO_BIKE_COLLISIONS])(u8) =
{
    MetatileBehavior_IsBumpySlope,
    MetatileBehavior_IsIsolatedVerticalRail,
    MetatileBehavior_IsIsolatedHorizontalRail,
    MetatileBehavior_IsVerticalRail,
    MetatileBehavior_IsHorizontalRail,
};

static const u8 sAcroBikeTrickCollisionTypes[NUM_ACRO_BIKE_COLLISIONS] = {
    COLLISION_WHEELIE_HOP,
    COLLISION_ISOLATED_VERTICAL_RAIL,
    COLLISION_ISOLATED_HORIZONTAL_RAIL,
    COLLISION_VERTICAL_RAIL,
    COLLISION_HORIZONTAL_RAIL,
};

static void (*const sPlayerAvatarTransitionFuncs[])(struct ObjectEvent *) =
{
    [PLAYER_AVATAR_STATE_NORMAL]     = PlayerAvatarTransition_Normal,
    [PLAYER_AVATAR_STATE_MACH_BIKE]  = PlayerAvatarTransition_MachBike,
    [PLAYER_AVATAR_STATE_ACRO_BIKE]  = PlayerAvatarTransition_AcroBike,
    [PLAYER_AVATAR_STATE_SURFING]    = PlayerAvatarTransition_Surfing,
    [PLAYER_AVATAR_STATE_UNDERWATER] = PlayerAvatarTransition_Underwater,
    [PLAYER_AVATAR_STATE_FIELD_MOVE] = PlayerAvatarTransition_ReturnToField,
    [PLAYER_AVATAR_STATE_FISHING]    = PlayerAvatarTransition_Dummy,
    [PLAYER_AVATAR_STATE_WATERING]   = PlayerAvatarTransition_Dummy,
};

static bool8 (*const sArrowWarpMetatileBehaviorChecks[])(u8) =
{
    [DIR_SOUTH - 1] = MetatileBehavior_IsSouthArrowWarp,
    [DIR_NORTH - 1] = MetatileBehavior_IsNorthArrowWarp,
    [DIR_WEST - 1]  = MetatileBehavior_IsWestArrowWarp,
    [DIR_EAST - 1]  = MetatileBehavior_IsEastArrowWarp,
};

static const u8 sRivalAvatarGfxIds[][2] =
{
    [PLAYER_AVATAR_STATE_NORMAL]     = {OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL,     OBJ_EVENT_GFX_RIVAL_MAY_NORMAL},
    [PLAYER_AVATAR_STATE_MACH_BIKE]  = {OBJ_EVENT_GFX_RIVAL_BRENDAN_MACH_BIKE,  OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE},
    [PLAYER_AVATAR_STATE_ACRO_BIKE]  = {OBJ_EVENT_GFX_RIVAL_BRENDAN_ACRO_BIKE,  OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE},
    [PLAYER_AVATAR_STATE_SURFING]    = {OBJ_EVENT_GFX_RIVAL_BRENDAN_SURFING,    OBJ_EVENT_GFX_RIVAL_MAY_SURFING},
    [PLAYER_AVATAR_STATE_UNDERWATER] = {OBJ_EVENT_GFX_BRENDAN_UNDERWATER,       OBJ_EVENT_GFX_MAY_UNDERWATER},
    [PLAYER_AVATAR_STATE_FIELD_MOVE] = {OBJ_EVENT_GFX_RIVAL_BRENDAN_FIELD_MOVE, OBJ_EVENT_GFX_RIVAL_MAY_FIELD_MOVE},
    [PLAYER_AVATAR_STATE_FISHING]    = {OBJ_EVENT_GFX_BRENDAN_FISHING,          OBJ_EVENT_GFX_MAY_FISHING},
    [PLAYER_AVATAR_STATE_WATERING]   = {OBJ_EVENT_GFX_BRENDAN_WATERING,         OBJ_EVENT_GFX_MAY_WATERING}
};

static const u8 sPlayerAvatarGfxIds[][2] =
{
    [PLAYER_AVATAR_STATE_NORMAL]     = {OBJ_EVENT_GFX_BRENDAN_NORMAL,     OBJ_EVENT_GFX_MAY_NORMAL},
    [PLAYER_AVATAR_STATE_MACH_BIKE]  = {OBJ_EVENT_GFX_BRENDAN_MACH_BIKE,  OBJ_EVENT_GFX_MAY_MACH_BIKE},
    [PLAYER_AVATAR_STATE_ACRO_BIKE]  = {OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE,  OBJ_EVENT_GFX_MAY_ACRO_BIKE},
    [PLAYER_AVATAR_STATE_SURFING]    = {OBJ_EVENT_GFX_BRENDAN_SURFING,    OBJ_EVENT_GFX_MAY_SURFING},
    [PLAYER_AVATAR_STATE_UNDERWATER] = {OBJ_EVENT_GFX_BRENDAN_UNDERWATER, OBJ_EVENT_GFX_MAY_UNDERWATER},
    [PLAYER_AVATAR_STATE_FIELD_MOVE] = {OBJ_EVENT_GFX_BRENDAN_FIELD_MOVE, OBJ_EVENT_GFX_MAY_FIELD_MOVE},
    [PLAYER_AVATAR_STATE_FISHING]    = {OBJ_EVENT_GFX_BRENDAN_FISHING,    OBJ_EVENT_GFX_MAY_FISHING},
    [PLAYER_AVATAR_STATE_WATERING]   = {OBJ_EVENT_GFX_BRENDAN_WATERING,   OBJ_EVENT_GFX_MAY_WATERING},
};

static const u8 sFRLGAvatarGfxIds[GENDER_COUNT] =
{
    [MALE]   = OBJ_EVENT_GFX_RED,
    [FEMALE] = OBJ_EVENT_GFX_LEAF
};

static const u8 sRSAvatarGfxIds[GENDER_COUNT] =
{
    [MALE]   = OBJ_EVENT_GFX_LINK_RS_BRENDAN,
    [FEMALE] = OBJ_EVENT_GFX_LINK_RS_MAY
};

static const u8 sPlayerAvatarGfxToStateFlag[GENDER_COUNT][5][2] =
{
    [MALE] =
    {
        {OBJ_EVENT_GFX_BRENDAN_NORMAL,     PLAYER_AVATAR_FLAG_ON_FOOT},
        {OBJ_EVENT_GFX_BRENDAN_MACH_BIKE,  PLAYER_AVATAR_FLAG_MACH_BIKE},
        {OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE,  PLAYER_AVATAR_FLAG_ACRO_BIKE},
        {OBJ_EVENT_GFX_BRENDAN_SURFING,    PLAYER_AVATAR_FLAG_SURFING},
        {OBJ_EVENT_GFX_BRENDAN_UNDERWATER, PLAYER_AVATAR_FLAG_UNDERWATER},
    },
    [FEMALE] =
    {
        {OBJ_EVENT_GFX_MAY_NORMAL,         PLAYER_AVATAR_FLAG_ON_FOOT},
        {OBJ_EVENT_GFX_MAY_MACH_BIKE,      PLAYER_AVATAR_FLAG_MACH_BIKE},
        {OBJ_EVENT_GFX_MAY_ACRO_BIKE,      PLAYER_AVATAR_FLAG_ACRO_BIKE},
        {OBJ_EVENT_GFX_MAY_SURFING,        PLAYER_AVATAR_FLAG_SURFING},
        {OBJ_EVENT_GFX_MAY_UNDERWATER,     PLAYER_AVATAR_FLAG_UNDERWATER},
    }
};

static bool8 (*const sArrowWarpMetatileBehaviorChecks2[])(u8) =  //Duplicate of sArrowWarpMetatileBehaviorChecks
{
    [DIR_SOUTH - 1] = MetatileBehavior_IsSouthArrowWarp,
    [DIR_NORTH - 1] = MetatileBehavior_IsNorthArrowWarp,
    [DIR_WEST - 1]  = MetatileBehavior_IsWestArrowWarp,
    [DIR_EAST - 1]  = MetatileBehavior_IsEastArrowWarp,
};

static bool8 (*const sPushBoulderFuncs[])(struct Task *, struct ObjectEvent *, struct ObjectEvent *) =
{
    PushBoulder_Start,
    PushBoulder_Move,
    PushBoulder_End,
};

static bool8 (*const sPlayerAvatarSecretBaseMatJump[])(struct Task *, struct ObjectEvent *) =
{
    PlayerAvatar_DoSecretBaseMatJump,
};

static bool8 (*const sPlayerAvatarSecretBaseMatSpin[])(struct Task *, struct ObjectEvent *) =
{
    PlayerAvatar_SecretBaseMatSpinStep0,
    PlayerAvatar_SecretBaseMatSpinStep1,
    PlayerAvatar_SecretBaseMatSpinStep2,
    PlayerAvatar_SecretBaseMatSpinStep3,
};

void MovementType_Player(struct Sprite *sprite)
{
    UpdateObjectEventCurrentMovement(&gObjectEvents[sprite->data[0]], sprite, ObjectEventCB2_NoMovement2);
}

static u8 ObjectEventCB2_NoMovement2(void)
{
    return 0;
}

void PlayerStep(u8 direction, u16 newKeys, u16 heldKeys)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    HideShowWarpArrow(playerObjEvent);
    if (gPlayerAvatar.preventStep == FALSE)
    {
        Bike_TryAcroBikeHistoryUpdate(newKeys, heldKeys);
        if (TryInterruptObjectEventSpecialAnim(playerObjEvent, direction) == 0)
        {
            npc_clear_strange_bits(playerObjEvent);
            DoPlayerAvatarTransition();
            if (TryDoMetatileBehaviorForcedMovement() == 0)
            {
                MovePlayerAvatarUsingKeypadInput(direction, newKeys, heldKeys);
                PlayerAllowForcedMovementIfMovingSameDirection();
            }
        }
    }
}

static bool8 TryInterruptObjectEventSpecialAnim(struct ObjectEvent *playerObjEvent, u8 direction)
{
    if (ObjectEventIsMovementOverridden(playerObjEvent)
     && !ObjectEventClearHeldMovementIfFinished(playerObjEvent))
    {
        u8 heldMovementActionId = ObjectEventGetHeldMovementActionId(playerObjEvent);
        if (heldMovementActionId > MOVEMENT_ACTION_WALK_FAST_RIGHT && heldMovementActionId < MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_DOWN)
        {
            if (direction == DIR_NONE)
            {
                return TRUE;
            }

            if (playerObjEvent->movementDirection != direction)
            {
                ObjectEventClearHeldMovement(playerObjEvent);
                return FALSE;
            }

            if (CheckForPlayerAvatarStaticCollision(direction) == COLLISION_NONE)
            {
                ObjectEventClearHeldMovement(playerObjEvent);
                return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

static void npc_clear_strange_bits(struct ObjectEvent *objEvent)
{
    objEvent->inanimate = FALSE;
    objEvent->disableAnim = FALSE;
    objEvent->facingDirectionLocked = FALSE;
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_DASH;
}

static void MovePlayerAvatarUsingKeypadInput(u8 direction, u16 newKeys, u16 heldKeys)
{
    if (gPlayerAvatar.flags & (PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
        MovePlayerOnBike(direction, newKeys, heldKeys);
    else
        MovePlayerNotOnBike(direction, heldKeys);
}

static void PlayerAllowForcedMovementIfMovingSameDirection(void)
{
    if (gPlayerAvatar.runningState == MOVING)
        gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_CONTROLLABLE;
}

static bool8 TryDoMetatileBehaviorForcedMovement(void)
{
    return sForcedMovementFuncs[GetForcedMovementByMetatileBehavior()]();
}

static u8 GetForcedMovementByMetatileBehavior(void)
{
    u8 i;

    if (!(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_CONTROLLABLE))
    {
        u8 metatileBehavior = gObjectEvents[gPlayerAvatar.objectEventId].currentMetatileBehavior;

        for (i = 0; i < NUM_FORCED_MOVEMENTS; i++)
        {
            if (sForcedMovementTestFuncs[i](metatileBehavior))
                return i + 1;
        }
    }
    return 0;
}

static bool8 ForcedMovement_None(void)
{
    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED_MOVE)
    {
        struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

        playerObjEvent->facingDirectionLocked = FALSE;
        playerObjEvent->enableAnim = TRUE;
        SetObjectEventDirection(playerObjEvent, playerObjEvent->facingDirection);
        gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED_MOVE;
    }
    return FALSE;
}

static bool8 DoForcedMovement(u8 direction, void (*moveFunc)(u8))
{
    struct PlayerAvatar *playerAvatar = &gPlayerAvatar;
    u8 collision = CheckForPlayerAvatarCollision(direction);

    playerAvatar->flags |= PLAYER_AVATAR_FLAG_FORCED_MOVE;
    if (collision)
    {
        ForcedMovement_None();
        if (collision < COLLISION_STOP_SURFING)
        {
            return FALSE;
        }
        else
        {
            if (collision == COLLISION_LEDGE_JUMP)
                PlayerJumpLedge(direction);
            playerAvatar->flags |= PLAYER_AVATAR_FLAG_FORCED_MOVE;
            playerAvatar->runningState = MOVING;
            return TRUE;
        }
    }
    else
    {
        playerAvatar->runningState = MOVING;
        moveFunc(direction);
        return TRUE;
    }
}

static bool8 DoForcedMovementInCurrentDirection(void (*moveFunc)(u8))
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    playerObjEvent->disableAnim = TRUE;
    return DoForcedMovement(playerObjEvent->movementDirection, moveFunc);
}

static bool8 ForcedMovement_Slip(void)
{
    return DoForcedMovementInCurrentDirection(PlayerWalkFast);
}

static bool8 ForcedMovement_WalkSouth(void)
{
    return DoForcedMovement(DIR_SOUTH, PlayerWalkNormal);
}

static bool8 ForcedMovement_WalkNorth(void)
{
    return DoForcedMovement(DIR_NORTH, PlayerWalkNormal);
}

static bool8 ForcedMovement_WalkWest(void)
{
    return DoForcedMovement(DIR_WEST, PlayerWalkNormal);
}

static bool8 ForcedMovement_WalkEast(void)
{
    return DoForcedMovement(DIR_EAST, PlayerWalkNormal);
}

static bool8 ForcedMovement_PushedSouthByCurrent(void)
{
    return DoForcedMovement(DIR_SOUTH, PlayerRideWaterCurrent);
}

static bool8 ForcedMovement_PushedNorthByCurrent(void)
{
    return DoForcedMovement(DIR_NORTH, PlayerRideWaterCurrent);
}

static bool8 ForcedMovement_PushedWestByCurrent(void)
{
    return DoForcedMovement(DIR_WEST, PlayerRideWaterCurrent);
}

static bool8 ForcedMovement_PushedEastByCurrent(void)
{
    return DoForcedMovement(DIR_EAST, PlayerRideWaterCurrent);
}

static bool8 ForcedMovement_Slide(u8 direction, void (*moveFunc)(u8))
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    playerObjEvent->disableAnim = TRUE;
    playerObjEvent->facingDirectionLocked = TRUE;
    return DoForcedMovement(direction, moveFunc);
}

static bool8 ForcedMovement_SlideSouth(void)
{
    return ForcedMovement_Slide(DIR_SOUTH, PlayerWalkFast);
}

static bool8 ForcedMovement_SlideNorth(void)
{
    return ForcedMovement_Slide(DIR_NORTH, PlayerWalkFast);
}

static bool8 ForcedMovement_SlideWest(void)
{
    return ForcedMovement_Slide(DIR_WEST, PlayerWalkFast);
}

static bool8 ForcedMovement_SlideEast(void)
{
    return ForcedMovement_Slide(DIR_EAST, PlayerWalkFast);
}

static bool8 ForcedMovement_MatJump(void)
{
    DoPlayerMatJump();
    return TRUE;
}

static bool8 ForcedMovement_MatSpin(void)
{
    DoPlayerMatSpin();
    return TRUE;
}

static bool8 ForcedMovement_MuddySlope(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (playerObjEvent->movementDirection != DIR_NORTH || GetPlayerSpeed() < PLAYER_SPEED_FASTEST)
    {
        Bike_UpdateBikeCounterSpeed(0);
        playerObjEvent->facingDirectionLocked = TRUE;
        return DoForcedMovement(DIR_SOUTH, PlayerWalkFast);
    }
    else
    {
        return FALSE;
    }
}

static void MovePlayerNotOnBike(u8 direction, u16 heldKeys)
{
    sPlayerNotOnBikeFuncs[CheckMovementInputNotOnBike(direction)](direction, heldKeys);
}

static u8 CheckMovementInputNotOnBike(u8 direction)
{
    if (direction == DIR_NONE)
        return gPlayerAvatar.runningState = NOT_MOVING;
    else if (direction != GetPlayerMovementDirection() && gPlayerAvatar.runningState != MOVING)
        return gPlayerAvatar.runningState = TURN_DIRECTION;
    else
        return gPlayerAvatar.runningState = MOVING;
}

static void PlayerNotOnBikeNotMoving(u8 direction, u16 heldKeys)
{
    PlayerFaceDirection(GetPlayerFacingDirection());
}

static void PlayerNotOnBikeTurningInPlace(u8 direction, u16 heldKeys)
{
    PlayerTurnInPlace(direction);
}

static void PlayerNotOnBikeMoving(u8 direction, u16 heldKeys)
{
    u8 collision = CheckForPlayerAvatarCollision(direction);

    if (collision)
    {
        if (collision == COLLISION_LEDGE_JUMP)
        {
            PlayerJumpLedge(direction);
            return;
        }
        else if (collision == COLLISION_OBJECT_EVENT && IsPlayerCollidingWithFarawayIslandMew(direction))
        {
            PlayerNotOnBikeCollideWithFarawayIslandMew(direction);
            return;
        }
        else
        {
            // Player collided with something. Certain collisions have special handling that precludes the normal collision effect.
            // COLLISION_STOP_SURFING and COLLISION_PUSHED_BOULDER's effects are started by CheckForObjectEventCollision.
            // COLLISION_LEDGE_JUMP's effect is handled further up in this function, so it will never reach this point.
            // COLLISION_ROTATING_GATE is unusual however, this was probably included by mistake. When the player walks into a
            // rotating gate that cannot rotate there is no additional handling, it's just a regular collision. Its exclusion here
            // means that the player avatar won't update if they encounter this kind of collision. This has two noticeable effects:
            // - Colliding with it head-on stops the player dead, rather than playing the walking animation and playing a bump sound effect
            // - Colliding with it by changing direction won't turn the player avatar, their walking animation will just speed up.
#ifdef BUGFIX
            if (collision != COLLISION_STOP_SURFING
             && collision != COLLISION_LEDGE_JUMP
             && collision != COLLISION_PUSHED_BOULDER)
#else
            if (collision != COLLISION_STOP_SURFING
             && collision != COLLISION_LEDGE_JUMP
             && collision != COLLISION_PUSHED_BOULDER
             && collision != COLLISION_ROTATING_GATE)
#endif
            {
                PlayerNotOnBikeCollide(direction);
            }
            return;
        }
    }

    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
    {
        // same speed as running
        PlayerWalkFast(direction);
        return;
    }

    if (!(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_UNDERWATER) && (heldKeys & B_BUTTON) && FlagGet(FLAG_SYS_B_DASH)
     && IsRunningDisallowed(gObjectEvents[gPlayerAvatar.objectEventId].currentMetatileBehavior) == 0)
    {
        PlayerRun(direction);
        gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_DASH;
        return;
    }
    else
    {
        PlayerWalkNormal(direction);
    }
}

static u8 CheckForPlayerAvatarCollision(u8 direction)
{
    s16 x, y;
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    x = playerObjEvent->currentCoords.x;
    y = playerObjEvent->currentCoords.y;
    MoveCoords(direction, &x, &y);
    return CheckForObjectEventCollision(playerObjEvent, x, y, direction, MapGridGetMetatileBehaviorAt(x, y));
}

static u8 CheckForPlayerAvatarStaticCollision(u8 direction)
{
    s16 x, y;
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    x = playerObjEvent->currentCoords.x;
    y = playerObjEvent->currentCoords.y;
    MoveCoords(direction, &x, &y);
    return CheckForObjectEventStaticCollision(playerObjEvent, x, y, direction, MapGridGetMetatileBehaviorAt(x, y));
}

u8 CheckForObjectEventCollision(struct ObjectEvent *objectEvent, s16 x, s16 y, u8 direction, u8 metatileBehavior)
{
    u8 collision = GetCollisionAtCoords(objectEvent, x, y, direction);
    if (collision == COLLISION_ELEVATION_MISMATCH && CanStopSurfing(x, y, direction))
        return COLLISION_STOP_SURFING;

    if (ShouldJumpLedge(x, y, direction))
    {
        IncrementGameStat(GAME_STAT_JUMPED_DOWN_LEDGES);
        return COLLISION_LEDGE_JUMP;
    }
    if (collision == COLLISION_OBJECT_EVENT && TryPushBoulder(x, y, direction))
        return COLLISION_PUSHED_BOULDER;

    if (collision == COLLISION_NONE)
    {
        if (CheckForRotatingGatePuzzleCollision(direction, x, y))
            return COLLISION_ROTATING_GATE;
        CheckAcroBikeCollision(x, y, metatileBehavior, &collision);
    }
    return collision;
}

static u8 CheckForObjectEventStaticCollision(struct ObjectEvent *objectEvent, s16 x, s16 y, u8 direction, u8 metatileBehavior)
{
    u8 collision = GetCollisionAtCoords(objectEvent, x, y, direction);

    if (collision == COLLISION_NONE)
    {
        if (CheckForRotatingGatePuzzleCollisionWithoutAnimation(direction, x, y))
            return COLLISION_ROTATING_GATE;
        CheckAcroBikeCollision(x, y, metatileBehavior, &collision);
    }
    return collision;
}

static bool8 CanStopSurfing(s16 x, s16 y, u8 direction)
{
    if ((gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
     && MapGridGetElevationAt(x, y) == 3
     && GetObjectEventIdByPosition(x, y, 3) == OBJECT_EVENTS_COUNT)
    {
        CreateStopSurfingTask(direction);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static bool8 ShouldJumpLedge(s16 x, s16 y, u8 direction)
{
    if (GetLedgeJumpDirection(x, y, direction) != DIR_NONE)
        return TRUE;
    else
        return FALSE;
}

static bool8 TryPushBoulder(s16 x, s16 y, u8 direction)
{
    if (FlagGet(FLAG_SYS_USE_STRENGTH))
    {
        u8 objectEventId = GetObjectEventIdByXY(x, y);

        if (objectEventId != OBJECT_EVENTS_COUNT && gObjectEvents[objectEventId].graphicsId == OBJ_EVENT_GFX_PUSHABLE_BOULDER)
        {
            x = gObjectEvents[objectEventId].currentCoords.x;
            y = gObjectEvents[objectEventId].currentCoords.y;
            MoveCoords(direction, &x, &y);
            if (GetCollisionAtCoords(&gObjectEvents[objectEventId], x, y, direction) == COLLISION_NONE
             && MetatileBehavior_IsNonAnimDoor(MapGridGetMetatileBehaviorAt(x, y)) == FALSE)
            {
                StartStrengthAnim(objectEventId, direction);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void CheckAcroBikeCollision(s16 x, s16 y, u8 metatileBehavior, u8 *collision)
{
    u8 i;

    for (i = 0; i < NUM_ACRO_BIKE_COLLISIONS; i++)
    {
        if (sAcroBikeTrickMetatiles[i](metatileBehavior))
        {
            *collision = sAcroBikeTrickCollisionTypes[i];
            return;
        }
    }
}

bool8 IsPlayerCollidingWithFarawayIslandMew(u8 direction)
{
    u8 mewObjectId;
    struct ObjectEvent *object;
    s16 playerX;
    s16 playerY;
    s16 mewPrevX;

    object = &gObjectEvents[gPlayerAvatar.objectEventId];
    playerX = object->currentCoords.x;
    playerY = object->currentCoords.y;

    MoveCoords(direction, &playerX, &playerY);
    mewObjectId = GetObjectEventIdByLocalIdAndMap(1, MAP_NUM(FARAWAY_ISLAND_INTERIOR), MAP_GROUP(FARAWAY_ISLAND_INTERIOR));
    if (mewObjectId == OBJECT_EVENTS_COUNT)
        return FALSE;

    object = &gObjectEvents[mewObjectId];
    mewPrevX = object->previousCoords.x;

    if (mewPrevX == playerX)
    {
        if (object->previousCoords.y != playerY
            || object->currentCoords.x != mewPrevX
            || object->currentCoords.y != object->previousCoords.y)
        {
            if (object->previousCoords.x == playerX &&
                object->previousCoords.y == playerY)
                return TRUE;
        }
    }
    return FALSE;
}

void SetPlayerAvatarTransitionFlags(u16 transitionFlags)
{
    gPlayerAvatar.transitionFlags |= transitionFlags;
    DoPlayerAvatarTransition();
}

static void DoPlayerAvatarTransition(void)
{
    u8 i;
    u8 flags = gPlayerAvatar.transitionFlags;

    if (flags != 0)
    {
        for (i = 0; i < ARRAY_COUNT(sPlayerAvatarTransitionFuncs); i++, flags >>= 1)
        {
            if (flags & 1)
                sPlayerAvatarTransitionFuncs[i](&gObjectEvents[gPlayerAvatar.objectEventId]);
        }
        gPlayerAvatar.transitionFlags = 0;
    }
}

static void PlayerAvatarTransition_Dummy(struct ObjectEvent *objEvent)
{

}

static void PlayerAvatarTransition_Normal(struct ObjectEvent *objEvent)
{
    ObjectEventSetGraphicsId(objEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_NORMAL));
    ObjectEventTurn(objEvent, objEvent->movementDirection);
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT);
}

static void PlayerAvatarTransition_MachBike(struct ObjectEvent *objEvent)
{
    ObjectEventSetGraphicsId(objEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_MACH_BIKE));
    ObjectEventTurn(objEvent, objEvent->movementDirection);
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_MACH_BIKE);
    BikeClearState(0, 0);
}

static void PlayerAvatarTransition_AcroBike(struct ObjectEvent *objEvent)
{
    ObjectEventSetGraphicsId(objEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_ACRO_BIKE));
    ObjectEventTurn(objEvent, objEvent->movementDirection);
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ACRO_BIKE);
    BikeClearState(0, 0);
    Bike_HandleBumpySlopeJump();
}

static void PlayerAvatarTransition_Surfing(struct ObjectEvent *objEvent)
{
    u8 spriteId;

    ObjectEventSetGraphicsId(objEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_SURFING));
    ObjectEventTurn(objEvent, objEvent->movementDirection);
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_SURFING);
    gFieldEffectArguments[0] = objEvent->currentCoords.x;
    gFieldEffectArguments[1] = objEvent->currentCoords.y;
    gFieldEffectArguments[2] = gPlayerAvatar.objectEventId;
    spriteId = FieldEffectStart(FLDEFF_SURF_BLOB);
    objEvent->fieldEffectSpriteId = spriteId;
    SetSurfBlob_BobState(spriteId, BOB_PLAYER_AND_MON);
}

static void PlayerAvatarTransition_Underwater(struct ObjectEvent *objEvent)
{
    ObjectEventSetGraphicsId(objEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_UNDERWATER));
    ObjectEventTurn(objEvent, objEvent->movementDirection);
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_UNDERWATER);
    objEvent->fieldEffectSpriteId = StartUnderwaterSurfBlobBobbing(objEvent->spriteId);
}

static void PlayerAvatarTransition_ReturnToField(struct ObjectEvent *objEvent)
{
    gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_CONTROLLABLE;
}

void UpdatePlayerAvatarTransitionState(void)
{
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    if (PlayerIsAnimActive())
    {
        if (!PlayerCheckIfAnimFinishedOrInactive())
        {
            if (!PlayerAnimIsMultiFrameStationary())
                gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        }
        else
        {
            if (!PlayerAnimIsMultiFrameStationaryAndStateNotTurning())
                gPlayerAvatar.tileTransitionState = T_TILE_CENTER;
        }
    }
}

static bool8 PlayerAnimIsMultiFrameStationary(void)
{
    u8 movementActionId = gObjectEvents[gPlayerAvatar.objectEventId].movementActionId;

    if (movementActionId <= MOVEMENT_ACTION_FACE_RIGHT
     || (movementActionId >= MOVEMENT_ACTION_DELAY_1 && movementActionId <= MOVEMENT_ACTION_DELAY_16)
     || (movementActionId >= MOVEMENT_ACTION_WALK_IN_PLACE_SLOW_DOWN && movementActionId <= MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_RIGHT)
     || (movementActionId >= MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN && movementActionId <= MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_RIGHT)
     || (movementActionId >= MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_DOWN && movementActionId <= MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_RIGHT))
        return TRUE;
    else
        return FALSE;
}

static bool8 PlayerAnimIsMultiFrameStationaryAndStateNotTurning(void)
{
    if (PlayerAnimIsMultiFrameStationary() && gPlayerAvatar.runningState != TURN_DIRECTION)
        return TRUE;
    else
        return FALSE;
}

static bool8 PlayerIsAnimActive(void)
{
    return ObjectEventIsMovementOverridden(&gObjectEvents[gPlayerAvatar.objectEventId]);
}

static bool8 PlayerCheckIfAnimFinishedOrInactive(void)
{
    return ObjectEventCheckHeldMovementStatus(&gObjectEvents[gPlayerAvatar.objectEventId]);
}

static void PlayerSetCopyableMovement(u8 movement)
{
    gObjectEvents[gPlayerAvatar.objectEventId].playerCopyableMovement = movement;
}

u8 PlayerGetCopyableMovement(void)
{
    return gObjectEvents[gPlayerAvatar.objectEventId].playerCopyableMovement;
}

static void PlayerForceSetHeldMovement(u8 movementActionId)
{
    ObjectEventForceSetHeldMovement(&gObjectEvents[gPlayerAvatar.objectEventId], movementActionId);
}

void PlayerSetAnimId(u8 movementActionId, u8 copyableMovement)
{
    if (!PlayerIsAnimActive())
    {
        PlayerSetCopyableMovement(copyableMovement);
        ObjectEventSetHeldMovement(&gObjectEvents[gPlayerAvatar.objectEventId], movementActionId);
    }
}

void PlayerWalkNormal(u8 direction)
{
    PlayerSetAnimId(GetWalkNormalMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerWalkFast(u8 direction)
{
    PlayerSetAnimId(GetWalkFastMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerRideWaterCurrent(u8 direction)
{
    PlayerSetAnimId(GetRideWaterCurrentMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerWalkFaster(u8 direction)
{
    PlayerSetAnimId(GetWalkFasterMovementAction(direction), COPY_MOVE_WALK);
}

static void PlayerRun(u8 direction)
{
    PlayerSetAnimId(GetPlayerRunMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerOnBikeCollide(u8 direction)
{
    PlayCollisionSoundIfNotFacingWarp(direction);
    PlayerSetAnimId(GetWalkInPlaceNormalMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerOnBikeCollideWithFarawayIslandMew(u8 direction)
{
    PlayerSetAnimId(GetWalkInPlaceNormalMovementAction(direction), COPY_MOVE_WALK);
}

static void PlayerNotOnBikeCollide(u8 direction)
{
    PlayCollisionSoundIfNotFacingWarp(direction);
    PlayerSetAnimId(GetWalkInPlaceSlowMovementAction(direction), COPY_MOVE_WALK);
}

static void PlayerNotOnBikeCollideWithFarawayIslandMew(u8 direction)
{
    PlayerSetAnimId(GetWalkInPlaceSlowMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerFaceDirection(u8 direction)
{
    PlayerSetAnimId(GetFaceDirectionMovementAction(direction), COPY_MOVE_FACE);
}

void PlayerTurnInPlace(u8 direction)
{
    PlayerSetAnimId(GetWalkInPlaceFastMovementAction(direction), COPY_MOVE_FACE);
}

void PlayerJumpLedge(u8 direction)
{
    PlaySE(SE_LEDGE);
    PlayerSetAnimId(GetJump2MovementAction(direction), COPY_MOVE_JUMP2);
}

// Stop player on current facing direction once they're done moving and if they're not currently Acro Biking on bumpy slope
void PlayerFreeze(void)
{
    if (gPlayerAvatar.tileTransitionState == T_TILE_CENTER || gPlayerAvatar.tileTransitionState == T_NOT_MOVING)
    {
        if (IsPlayerNotUsingAcroBikeOnBumpySlope())
            PlayerForceSetHeldMovement(GetFaceDirectionMovementAction(gObjectEvents[gPlayerAvatar.objectEventId].facingDirection));
    }
}

// wheelie idle
void PlayerIdleWheelie(u8 direction)
{
    PlayerSetAnimId(GetAcroWheelieFaceDirectionMovementAction(direction), COPY_MOVE_FACE);
}

// normal to wheelie
void PlayerStartWheelie(u8 direction)
{
    PlayerSetAnimId(GetAcroPopWheelieFaceDirectionMovementAction(direction), COPY_MOVE_FACE);
}

// wheelie to normal
void PlayerEndWheelie(u8 direction)
{
    PlayerSetAnimId(GetAcroEndWheelieFaceDirectionMovementAction(direction), COPY_MOVE_FACE);
}

// wheelie hopping standing
void PlayerStandingHoppingWheelie(u8 direction)
{
    PlaySE(SE_BIKE_HOP);
    PlayerSetAnimId(GetAcroWheelieHopFaceDirectionMovementAction(direction), COPY_MOVE_FACE);
}

// wheelie hopping moving
void PlayerMovingHoppingWheelie(u8 direction)
{
    PlaySE(SE_BIKE_HOP);
    PlayerSetAnimId(GetAcroWheelieHopDirectionMovementAction(direction), COPY_MOVE_WALK);
}

// wheelie hopping ledge
void PlayerLedgeHoppingWheelie(u8 direction)
{
    PlaySE(SE_BIKE_HOP);
    PlayerSetAnimId(GetAcroWheelieJumpDirectionMovementAction(direction), COPY_MOVE_JUMP2);
}

// acro turn jump
void PlayerAcroTurnJump(u8 direction)
{
    PlaySE(SE_BIKE_HOP);
    PlayerSetAnimId(GetJumpInPlaceTurnAroundMovementAction(direction), COPY_MOVE_FACE);
}

void PlayerWheelieInPlace(u8 direction)
{
    PlaySE(SE_WALL_HIT);
    PlayerSetAnimId(GetAcroWheelieInPlaceDirectionMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerPopWheelieWhileMoving(u8 direction)
{
    PlayerSetAnimId(GetAcroPopWheelieMoveDirectionMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerWheelieMove(u8 direction)
{
    PlayerSetAnimId(GetAcroWheelieMoveDirectionMovementAction(direction), COPY_MOVE_WALK);
}

void PlayerEndWheelieWhileMoving(u8 direction)
{
    PlayerSetAnimId(GetAcroEndWheelieMoveDirectionMovementAction(direction), COPY_MOVE_WALK);
}

static void PlayCollisionSoundIfNotFacingWarp(u8 direction)
{
    s16 x, y;
    u8 metatileBehavior = gObjectEvents[gPlayerAvatar.objectEventId].currentMetatileBehavior;

    if (!sArrowWarpMetatileBehaviorChecks[direction - 1](metatileBehavior))
    {
        // Check if walking up into a door
        if (direction == DIR_NORTH)
        {
            PlayerGetDestCoords(&x, &y);
            MoveCoords(direction, &x, &y);
            if (MetatileBehavior_IsWarpDoor(MapGridGetMetatileBehaviorAt(x, y)))
                return;
        }
        PlaySE(SE_WALL_HIT);
    }
}

void GetXYCoordsOneStepInFrontOfPlayer(s16 *x, s16 *y)
{
    *x = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
    *y = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
    MoveCoords(GetPlayerFacingDirection(), x, y);
}

void PlayerGetDestCoords(s16 *x, s16 *y)
{
    *x = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
    *y = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
}

u8 player_get_pos_including_state_based_drift(s16 *x, s16 *y)
{
    struct ObjectEvent *object = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (object->heldMovementActive && !object->heldMovementFinished && !gSprites[object->spriteId].data[2])
    {
        *x = object->currentCoords.x;
        *y = object->currentCoords.y;

        switch (object->movementActionId)
        {
        case MOVEMENT_ACTION_WALK_NORMAL_DOWN:
        case MOVEMENT_ACTION_PLAYER_RUN_DOWN:
            (*y)++;
            return TRUE;
        case MOVEMENT_ACTION_WALK_NORMAL_UP:
        case MOVEMENT_ACTION_PLAYER_RUN_UP:
            (*y)--;
            return TRUE;
        case MOVEMENT_ACTION_WALK_NORMAL_LEFT:
        case MOVEMENT_ACTION_PLAYER_RUN_LEFT:
            (*x)--;
            return TRUE;
        case MOVEMENT_ACTION_WALK_NORMAL_RIGHT:
        case MOVEMENT_ACTION_PLAYER_RUN_RIGHT:
            (*x)++;
            return TRUE;
        }
    }

    *x = -1;
    *y = -1;
    return FALSE;
}

u8 GetPlayerFacingDirection(void)
{
    return gObjectEvents[gPlayerAvatar.objectEventId].facingDirection;
}

u8 GetPlayerMovementDirection(void)
{
    return gObjectEvents[gPlayerAvatar.objectEventId].movementDirection;
}

u8 PlayerGetElevation(void)
{
    return gObjectEvents[gPlayerAvatar.objectEventId].previousElevation;
}

// unused
void MovePlayerToMapCoords(s16 x, s16 y)
{
    MoveObjectEventToMapCoords(&gObjectEvents[gPlayerAvatar.objectEventId], x, y);
}

u8 TestPlayerAvatarFlags(u8 flag)
{
    return gPlayerAvatar.flags & flag;
}

u8 GetPlayerAvatarFlags(void)
{
    return gPlayerAvatar.flags;
}

u8 GetPlayerAvatarSpriteId(void)
{
    return gPlayerAvatar.spriteId;
}

void CancelPlayerForcedMovement(void)
{
    ForcedMovement_None();
}

void StopPlayerAvatar(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    npc_clear_strange_bits(playerObjEvent);
    SetObjectEventDirection(playerObjEvent, playerObjEvent->facingDirection);
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
    {
        Bike_HandleBumpySlopeJump();
        Bike_UpdateBikeCounterSpeed(0);
    }
}

u8 GetRivalAvatarGraphicsIdByStateIdAndGender(u8 state, u8 gender)
{
    return sRivalAvatarGfxIds[state][gender];
}

u8 GetPlayerAvatarGraphicsIdByStateIdAndGender(u8 state, u8 gender)
{
    return sPlayerAvatarGfxIds[state][gender];
}

u8 GetFRLGAvatarGraphicsIdByGender(u8 gender)
{
    return sFRLGAvatarGfxIds[gender];
}

u8 GetRSAvatarGraphicsIdByGender(u8 gender)
{
    return sRSAvatarGfxIds[gender];
}

u8 GetPlayerAvatarGraphicsIdByStateId(u8 state)
{
    return GetPlayerAvatarGraphicsIdByStateIdAndGender(state, gPlayerAvatar.gender);
}

u8 unref_GetRivalAvatarGenderByGraphicsId(u8 gfxId)
{
    switch (gfxId)
    {
    case OBJ_EVENT_GFX_RIVAL_MAY_NORMAL:
    case OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE:
    case OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE:
    case OBJ_EVENT_GFX_RIVAL_MAY_SURFING:
    case OBJ_EVENT_GFX_RIVAL_MAY_FIELD_MOVE:
    case OBJ_EVENT_GFX_MAY_UNDERWATER:
    case OBJ_EVENT_GFX_MAY_FISHING:
    case OBJ_EVENT_GFX_MAY_WATERING:
        return FEMALE;
    default:
        return MALE;
    }
}

u8 GetPlayerAvatarGenderByGraphicsId(u8 gfxId)
{
    switch (gfxId)
    {
    case OBJ_EVENT_GFX_MAY_NORMAL:
    case OBJ_EVENT_GFX_MAY_MACH_BIKE:
    case OBJ_EVENT_GFX_MAY_ACRO_BIKE:
    case OBJ_EVENT_GFX_MAY_SURFING:
    case OBJ_EVENT_GFX_MAY_FIELD_MOVE:
    case OBJ_EVENT_GFX_MAY_UNDERWATER:
    case OBJ_EVENT_GFX_MAY_FISHING:
    case OBJ_EVENT_GFX_MAY_WATERING:
        return FEMALE;
    default:
        return MALE;
    }
}

bool8 PartyHasMonWithSurf(void)
{
    u8 i;

    if (!TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE)
                break;
            if (MonKnowsMove(&gPlayerParty[i], MOVE_SURF))
                return TRUE;
        }
    }
    return FALSE;
}

bool8 IsPlayerSurfingNorth(void)
{
    if (GetPlayerMovementDirection() == DIR_NORTH && TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
        return TRUE;
    else
        return FALSE;
}

bool8 IsPlayerFacingSurfableFishableWater(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 x = playerObjEvent->currentCoords.x;
    s16 y = playerObjEvent->currentCoords.y;

    MoveCoords(playerObjEvent->facingDirection, &x, &y);
    if (GetCollisionAtCoords(playerObjEvent, x, y, playerObjEvent->facingDirection) == COLLISION_ELEVATION_MISMATCH
     && PlayerGetElevation() == 3
     && MetatileBehavior_IsSurfableFishableWater(MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;
    else
        return FALSE;
}

void ClearPlayerAvatarInfo(void)
{
    memset(&gPlayerAvatar, 0, sizeof(struct PlayerAvatar));
}

void SetPlayerAvatarStateMask(u8 flags)
{
    gPlayerAvatar.flags &= (PLAYER_AVATAR_FLAG_DASH | PLAYER_AVATAR_FLAG_FORCED_MOVE | PLAYER_AVATAR_FLAG_CONTROLLABLE);
    gPlayerAvatar.flags |= flags;
}

static u8 GetPlayerAvatarStateTransitionByGraphicsId(u8 graphicsId, u8 gender)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sPlayerAvatarGfxToStateFlag[0]); i++)
    {
        if (sPlayerAvatarGfxToStateFlag[gender][i][0] == graphicsId)
            return sPlayerAvatarGfxToStateFlag[gender][i][1];
    }
    return PLAYER_AVATAR_FLAG_ON_FOOT;
}

u8 GetPlayerAvatarGraphicsIdByCurrentState(void)
{
    u8 i;
    u8 flags = gPlayerAvatar.flags;

    for (i = 0; i < ARRAY_COUNT(sPlayerAvatarGfxToStateFlag[0]); i++)
    {
        if (sPlayerAvatarGfxToStateFlag[gPlayerAvatar.gender][i][1] & flags)
            return sPlayerAvatarGfxToStateFlag[gPlayerAvatar.gender][i][0];
    }
    return 0;
}

void SetPlayerAvatarExtraStateTransition(u8 graphicsId, u8 transitionFlag)
{
    u8 stateFlag = GetPlayerAvatarStateTransitionByGraphicsId(graphicsId, gPlayerAvatar.gender);

    gPlayerAvatar.transitionFlags |= stateFlag | transitionFlag;
    DoPlayerAvatarTransition();
}

void InitPlayerAvatar(s16 x, s16 y, u8 direction, u8 gender)
{
    struct ObjectEventTemplate playerObjEventTemplate;
    u8 objectEventId;
    struct ObjectEvent *objectEvent;

    playerObjEventTemplate.localId = OBJ_EVENT_ID_PLAYER;
    playerObjEventTemplate.graphicsId = GetPlayerAvatarGraphicsIdByStateIdAndGender(PLAYER_AVATAR_STATE_NORMAL, gender);
    playerObjEventTemplate.x = x - MAP_OFFSET;
    playerObjEventTemplate.y = y - MAP_OFFSET;
    playerObjEventTemplate.elevation = 0;
    playerObjEventTemplate.movementType = MOVEMENT_TYPE_PLAYER;
    playerObjEventTemplate.movementRangeX = 0;
    playerObjEventTemplate.movementRangeY = 0;
    playerObjEventTemplate.trainerType = TRAINER_TYPE_NONE;
    playerObjEventTemplate.trainerRange_berryTreeId = 0;
    playerObjEventTemplate.script = NULL;
    playerObjEventTemplate.flagId = 0;
    objectEventId = SpawnSpecialObjectEvent(&playerObjEventTemplate);
    objectEvent = &gObjectEvents[objectEventId];
    objectEvent->isPlayer = TRUE;
    objectEvent->warpArrowSpriteId = CreateWarpArrowSprite();
    ObjectEventTurn(objectEvent, direction);
    ClearPlayerAvatarInfo();
    gPlayerAvatar.runningState = NOT_MOVING;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    gPlayerAvatar.objectEventId = objectEventId;
    gPlayerAvatar.spriteId = objectEvent->spriteId;
    gPlayerAvatar.gender = gender;
    SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_CONTROLLABLE | PLAYER_AVATAR_FLAG_ON_FOOT);
}

void SetPlayerInvisibility(bool8 invisible)
{
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = invisible;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
        gSprites[gObjectEvents[gPlayerAvatar.objectEventId].fieldEffectSpriteId].invisible = invisible;
}

void SetPlayerAvatarFieldMove(void)
{
    ObjectEventSetGraphicsId(&gObjectEvents[gPlayerAvatar.objectEventId], GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_FIELD_MOVE));
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], ANIM_FIELD_MOVE);
}

static void SetPlayerAvatarFishing(u8 direction)
{
    ObjectEventSetGraphicsId(&gObjectEvents[gPlayerAvatar.objectEventId], GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_FISHING));
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetFishingDirectionAnimNum(direction));
}

void PlayerUseAcroBikeOnBumpySlope(u8 direction)
{
    ObjectEventSetGraphicsId(&gObjectEvents[gPlayerAvatar.objectEventId], GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_ACRO_BIKE));
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetAcroWheelieDirectionAnimNum(direction));
    SeekSpriteAnim(&gSprites[gPlayerAvatar.spriteId], 1);
}

void SetPlayerAvatarWatering(u8 direction)
{
    ObjectEventSetGraphicsId(&gObjectEvents[gPlayerAvatar.objectEventId], GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_WATERING));
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetFaceDirectionAnimNum(direction));
}

static void HideShowWarpArrow(struct ObjectEvent *objectEvent)
{
    s16 x;
    s16 y;
    u8 direction;
    u8 metatileBehavior = objectEvent->currentMetatileBehavior;

    for (x = 0, direction = DIR_SOUTH; x < 4; x++, direction++)
    {
        if (sArrowWarpMetatileBehaviorChecks2[x](metatileBehavior) && direction == objectEvent->movementDirection)
        {
            // Show warp arrow if applicable
            x = objectEvent->currentCoords.x;
            y = objectEvent->currentCoords.y;
            MoveCoords(direction, &x, &y);
            ShowWarpArrowSprite(objectEvent->warpArrowSpriteId, direction, x, y);
            return;
        }
    }
    SetSpriteInvisible(objectEvent->warpArrowSpriteId);
}

/* Strength */

#define tState        data[0]
#define tBoulderObjId data[1]
#define tDirection    data[2]

static void StartStrengthAnim(u8 objectEventId, u8 direction)
{
    u8 taskId = CreateTask(Task_PushBoulder, 0xFF);

    gTasks[taskId].tBoulderObjId = objectEventId;
    gTasks[taskId].tDirection = direction;
    Task_PushBoulder(taskId);
}

static void Task_PushBoulder(u8 taskId)
{
    while (sPushBoulderFuncs[gTasks[taskId].tState](&gTasks[taskId],
                                                     &gObjectEvents[gPlayerAvatar.objectEventId],
                                                     &gObjectEvents[gTasks[taskId].tBoulderObjId]))
        ;
}

static bool8 PushBoulder_Start(struct Task *task, struct ObjectEvent *player, struct ObjectEvent *boulder)
{
    LockPlayerFieldControls();
    gPlayerAvatar.preventStep = TRUE;
    task->tState++;
    return FALSE;
}

static bool8 PushBoulder_Move(struct Task *task, struct ObjectEvent *player, struct ObjectEvent *boulder)
{
    if (ObjectEventIsHeldMovementActive(player))
        ObjectEventClearHeldMovementIfFinished(player);

    if (ObjectEventIsHeldMovementActive(boulder))
        ObjectEventClearHeldMovementIfFinished(boulder);

    if (!ObjectEventIsMovementOverridden(player)
     && !ObjectEventIsMovementOverridden(boulder))
    {
        ObjectEventClearHeldMovementIfFinished(player);
        ObjectEventClearHeldMovementIfFinished(boulder);
        ObjectEventSetHeldMovement(player, GetWalkInPlaceNormalMovementAction((u8)task->tDirection));
        ObjectEventSetHeldMovement(boulder, GetWalkSlowMovementAction((u8)task->tDirection));
        gFieldEffectArguments[0] = boulder->currentCoords.x;
        gFieldEffectArguments[1] = boulder->currentCoords.y;
        gFieldEffectArguments[2] = boulder->previousElevation;
        gFieldEffectArguments[3] = gSprites[boulder->spriteId].oam.priority;
        FieldEffectStart(FLDEFF_DUST);
        PlaySE(SE_M_STRENGTH);
        task->tState++;
    }
    return FALSE;
}

static bool8 PushBoulder_End(struct Task *task, struct ObjectEvent *player, struct ObjectEvent *boulder)
{
    if (ObjectEventCheckHeldMovementStatus(player)
     && ObjectEventCheckHeldMovementStatus(boulder))
    {
        ObjectEventClearHeldMovementIfFinished(player);
        ObjectEventClearHeldMovementIfFinished(boulder);
        gPlayerAvatar.preventStep = FALSE;
        UnlockPlayerFieldControls();
        DestroyTask(FindTaskIdByFunc(Task_PushBoulder));
    }
    return FALSE;
}

#undef tState
#undef tBoulderObjId
#undef tDirection

/* Some field effect */

static void DoPlayerMatJump(void)
{
    DoPlayerAvatarSecretBaseMatJump(CreateTask(DoPlayerAvatarSecretBaseMatJump, 0xFF));
}

static void DoPlayerAvatarSecretBaseMatJump(u8 taskId)
{
    while (sPlayerAvatarSecretBaseMatJump[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId]))
        ;
}

// because data[0] is used to call this, it can be inferred that there may have been multiple mat jump functions at one point, so the name for these groups of functions is appropriate in assuming the sole use of mat jump.
static u8 PlayerAvatar_DoSecretBaseMatJump(struct Task *task, struct ObjectEvent *objectEvent)
{
    gPlayerAvatar.preventStep = TRUE;
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        PlaySE(SE_LEDGE);
        ObjectEventSetHeldMovement(objectEvent, GetJumpInPlaceMovementAction(objectEvent->facingDirection));
        task->data[1]++;
        if (task->data[1] > 1)
        {
            gPlayerAvatar.preventStep = FALSE;
            gPlayerAvatar.transitionFlags |= PLAYER_AVATAR_FLAG_CONTROLLABLE;
            DestroyTask(FindTaskIdByFunc(DoPlayerAvatarSecretBaseMatJump));
        }
    }
    return FALSE;
}

/* Some field effect */

static void DoPlayerMatSpin(void)
{
    u8 taskId = CreateTask(PlayerAvatar_DoSecretBaseMatSpin, 0xFF);

    PlayerAvatar_DoSecretBaseMatSpin(taskId);
}

static void PlayerAvatar_DoSecretBaseMatSpin(u8 taskId)
{
    while (sPlayerAvatarSecretBaseMatSpin[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId]))
        ;
}

static bool8 PlayerAvatar_SecretBaseMatSpinStep0(struct Task *task, struct ObjectEvent *objectEvent)
{
    task->data[0]++;
    task->data[1] = objectEvent->movementDirection;
    gPlayerAvatar.preventStep = TRUE;
    LockPlayerFieldControls();
    PlaySE(SE_WARP_IN);
    return TRUE;
}

static bool8 PlayerAvatar_SecretBaseMatSpinStep1(struct Task *task, struct ObjectEvent *objectEvent)
{
    u8 directions[] = {DIR_WEST, DIR_EAST, DIR_NORTH, DIR_SOUTH};

    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        u8 direction;

        ObjectEventSetHeldMovement(objectEvent, GetFaceDirectionMovementAction(direction = directions[objectEvent->movementDirection - 1]));
        if (direction == (u8)task->data[1])
            task->data[2]++;
        task->data[0]++;
        if (task->data[2] > 3 && direction == GetOppositeDirection(task->data[1]))
            task->data[0]++;
    }
    return FALSE;
}

static bool8 PlayerAvatar_SecretBaseMatSpinStep2(struct Task *task, struct ObjectEvent *objectEvent)
{
    const u8 actions[] = {
        MOVEMENT_ACTION_DELAY_1,
        MOVEMENT_ACTION_DELAY_1,
        MOVEMENT_ACTION_DELAY_2,
        MOVEMENT_ACTION_DELAY_4,
        MOVEMENT_ACTION_DELAY_8,
    };

    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        ObjectEventSetHeldMovement(objectEvent, actions[task->data[2]]);
        task->data[0] = 1;
    }
    return FALSE;
}

static bool8 PlayerAvatar_SecretBaseMatSpinStep3(struct Task *task, struct ObjectEvent *objectEvent)
{
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        ObjectEventSetHeldMovement(objectEvent, GetWalkSlowMovementAction(GetOppositeDirection(task->data[1])));
        UnlockPlayerFieldControls();
        gPlayerAvatar.preventStep = FALSE;
        DestroyTask(FindTaskIdByFunc(PlayerAvatar_DoSecretBaseMatSpin));
    }
    return FALSE;
}

static void CreateStopSurfingTask(u8 direction)
{
    u8 taskId;

    LockPlayerFieldControls();
    Overworld_ClearSavedMusic();
    Overworld_ChangeMusicToDefault();
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_SURFING;
    gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_ON_FOOT;
    gPlayerAvatar.preventStep = TRUE;
    taskId = CreateTask(Task_StopSurfingInit, 0xFF);
    gTasks[taskId].data[0] = direction;
    Task_StopSurfingInit(taskId);
}

static void Task_StopSurfingInit(u8 taskId)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (ObjectEventIsMovementOverridden(playerObjEvent))
    {
        if (!ObjectEventClearHeldMovementIfFinished(playerObjEvent))
            return;
    }
    SetSurfBlob_BobState(playerObjEvent->fieldEffectSpriteId, BOB_JUST_MON);
    ObjectEventSetHeldMovement(playerObjEvent, GetJumpSpecialMovementAction((u8)gTasks[taskId].data[0]));
    gTasks[taskId].func = Task_WaitStopSurfing;
}

static void Task_WaitStopSurfing(u8 taskId)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (ObjectEventClearHeldMovementIfFinished(playerObjEvent))
    {
        ObjectEventSetGraphicsId(playerObjEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_NORMAL));
        ObjectEventSetHeldMovement(playerObjEvent, GetFaceDirectionMovementAction(playerObjEvent->facingDirection));
        gPlayerAvatar.preventStep = FALSE;
        UnlockPlayerFieldControls();
        DestroySprite(&gSprites[playerObjEvent->fieldEffectSpriteId]);
#ifdef BUGFIX
        // If this is not defined but the player steps into grass from surfing, they will appear over the grass instead of in the grass.
        playerObjEvent->triggerGroundEffectsOnMove = TRUE;
#endif
        DestroyTask(taskId);
    }
}

#define tStep              data[0]
#define tFrameCounter      data[1]
#define tNumDots           data[2]
#define tDotsRequired      data[3]
#define tRoundsPlayed      data[12]
#define tMinRoundsRequired data[13]
#define tPlayerGfxId       data[14]
#define tFishingRod        data[15]

// Some states are jumped to directly, labeled below
#define FISHING_START_ROUND 3
#define FISHING_GOT_BITE 6
#define FISHING_ON_HOOK 9
#define FISHING_NO_BITE 11
#define FISHING_GOT_AWAY 12
#define FISHING_SHOW_RESULT 13

static bool8 (*const sFishingStateFuncs[])(struct Task *) =
{
    Fishing_Init,
    Fishing_GetRodOut,
    Fishing_WaitBeforeDots,
    Fishing_InitDots,       // FISHING_START_ROUND
    Fishing_ShowDots,
    Fishing_CheckForBite,
    Fishing_GotBite,        // FISHING_GOT_BITE
    Fishing_WaitForA,
    Fishing_CheckMoreDots,
    Fishing_MonOnHook,      // FISHING_ON_HOOK
    Fishing_StartEncounter,
    Fishing_NotEvenNibble,  // FISHING_NO_BITE
    Fishing_GotAway,        // FISHING_GOT_AWAY
    Fishing_NoMon,          // FISHING_SHOW_RESULT
    Fishing_PutRodAway,
    Fishing_EndNoMon,
};

void StartFishing(u8 rod)
{
    u8 taskId = CreateTask(Task_Fishing, 0xFF);

    gTasks[taskId].tFishingRod = rod;
    Task_Fishing(taskId);
}

static void Task_Fishing(u8 taskId)
{
    while (sFishingStateFuncs[gTasks[taskId].tStep](&gTasks[taskId]))
        ;
}

static bool8 Fishing_Init(struct Task *task)
{
    LockPlayerFieldControls();
    gPlayerAvatar.preventStep = TRUE;
    task->tStep++;
    return FALSE;
}

static bool8 Fishing_GetRodOut(struct Task *task)
{
    struct ObjectEvent *playerObjEvent;
    const s16 minRounds1[] = {
        [OLD_ROD]   = 1,
        [GOOD_ROD]  = 1,
        [SUPER_ROD] = 1
    };
    const s16 minRounds2[] = {
        [OLD_ROD]   = 1,
        [GOOD_ROD]  = 3,
        [SUPER_ROD] = 6
    };

    task->tRoundsPlayed = 0;
    task->tMinRoundsRequired = minRounds1[task->tFishingRod] + (Random() % minRounds2[task->tFishingRod]);
    task->tPlayerGfxId = gObjectEvents[gPlayerAvatar.objectEventId].graphicsId;
    playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    ObjectEventClearHeldMovementIfActive(playerObjEvent);
    playerObjEvent->enableAnim = TRUE;
    SetPlayerAvatarFishing(playerObjEvent->facingDirection);
    task->tStep++;
    return FALSE;
}

static bool8 Fishing_WaitBeforeDots(struct Task *task)
{
    AlignFishingAnimationFrames();

    // Wait one second
    task->tFrameCounter++;
    if (task->tFrameCounter >= 60)
        task->tStep++;
    return FALSE;
}

static bool8 Fishing_InitDots(struct Task *task)
{
    u32 randVal;

    LoadMessageBoxAndFrameGfx(0, TRUE);
    task->tStep++;
    task->tFrameCounter = 0;
    task->tNumDots = 0;
    randVal = Random();
    randVal %= 10;
    task->tDotsRequired = randVal + 1;
    if (task->tRoundsPlayed == 0)
        task->tDotsRequired = randVal + 4;
    if (task->tDotsRequired >= 10)
        task->tDotsRequired = 10;
    return TRUE;
}

static bool8 Fishing_ShowDots(struct Task *task)
{
    const u8 dot[] = _("·");

    AlignFishingAnimationFrames();
    task->tFrameCounter++;
    if (JOY_NEW(A_BUTTON))
    {
        task->tStep = FISHING_NO_BITE;
        if (task->tRoundsPlayed != 0)
            task->tStep = FISHING_GOT_AWAY;
        return TRUE;
    }
    else
    {
        if (task->tFrameCounter >= 20)
        {
            task->tFrameCounter = 0;
            if (task->tNumDots >= task->tDotsRequired)
            {
                task->tStep++;
                if (task->tRoundsPlayed != 0)
                    task->tStep++;
                task->tRoundsPlayed++;
            }
            else
            {
                AddTextPrinterParameterized(0, FONT_NORMAL, dot, task->tNumDots * 8, 1, 0, NULL);
                task->tNumDots++;
            }
        }
        return FALSE;
    }
}

static bool8 Fishing_CheckForBite(struct Task *task)
{
    bool8 bite;

    AlignFishingAnimationFrames();
    task->tStep++;
    bite = FALSE;

    if (!DoesCurrentMapHaveFishingMons())
    {
        task->tStep = FISHING_NO_BITE;
    }
    else
    {
        if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        {
            u8 ability = GetMonAbility(&gPlayerParty[0]);
            if (ability == ABILITY_SUCTION_CUPS || ability  == ABILITY_STICKY_HOLD)
            {
                if (Random() % 100 > 14)
                    bite = TRUE;
            }
        }

        if (!bite)
        {
            if (Random() & 1)
                task->tStep = FISHING_NO_BITE;
            else
                bite = TRUE;
        }

        if (bite == TRUE)
            StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetFishingBiteDirectionAnimNum(GetPlayerFacingDirection()));
    }
    return TRUE;
}

static bool8 Fishing_GotBite(struct Task *task)
{
    AlignFishingAnimationFrames();
    AddTextPrinterParameterized(0, FONT_NORMAL, gText_OhABite, 0, 17, 0, NULL);
    task->tStep++;
    task->tFrameCounter = 0;
    return FALSE;
}

// We have a bite. Now, wait for the player to press A, or the timer to expire.
static bool8 Fishing_WaitForA(struct Task *task)
{
    const s16 reelTimeouts[3] = {
        [OLD_ROD]   = 36,
        [GOOD_ROD]  = 33,
        [SUPER_ROD] = 30
    };

    AlignFishingAnimationFrames();
    task->tFrameCounter++;
    if (task->tFrameCounter >= reelTimeouts[task->tFishingRod])
        task->tStep = FISHING_GOT_AWAY;
    else if (JOY_NEW(A_BUTTON))
        task->tStep++;
    return FALSE;
}

// Determine if we're going to play the dot game again
static bool8 Fishing_CheckMoreDots(struct Task *task)
{
    const s16 moreDotsChance[][2] =
    {
        [OLD_ROD]   = {0, 0},
        [GOOD_ROD]  = {40, 10},
        [SUPER_ROD] = {70, 30}
    };

    AlignFishingAnimationFrames();
    task->tStep++;
    if (task->tRoundsPlayed < task->tMinRoundsRequired)
    {
        task->tStep = FISHING_START_ROUND;
    }
    else if (task->tRoundsPlayed < 2)
    {
        // probability of having to play another round
        s16 probability = Random() % 100;

        if (moreDotsChance[task->tFishingRod][task->tRoundsPlayed] > probability)
            task->tStep = FISHING_START_ROUND;
    }
    return FALSE;
}

static bool8 Fishing_MonOnHook(struct Task *task)
{
    AlignFishingAnimationFrames();
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized2(0, FONT_NORMAL, gText_PokemonOnHook, 1, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    task->tStep++;
    task->tFrameCounter = 0;
    return FALSE;
}

static bool8 Fishing_StartEncounter(struct Task *task)
{
    if (task->tFrameCounter == 0)
        AlignFishingAnimationFrames();

    RunTextPrinters();

    if (task->tFrameCounter == 0)
    {
        if (!IsTextPrinterActive(0))
        {
            struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

            ObjectEventSetGraphicsId(playerObjEvent, task->tPlayerGfxId);
            ObjectEventTurn(playerObjEvent, playerObjEvent->movementDirection);
            if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
                SetSurfBlob_PlayerOffset(gObjectEvents[gPlayerAvatar.objectEventId].fieldEffectSpriteId, FALSE, 0);
            gSprites[gPlayerAvatar.spriteId].x2 = 0;
            gSprites[gPlayerAvatar.spriteId].y2 = 0;
            ClearDialogWindowAndFrame(0, TRUE);
            task->tFrameCounter++;
            return FALSE;
        }
    }

    if (task->tFrameCounter != 0)
    {
        gPlayerAvatar.preventStep = FALSE;
        UnlockPlayerFieldControls();
        FishingWildEncounter(task->tFishingRod);
        RecordFishingAttemptForTV(TRUE);
        DestroyTask(FindTaskIdByFunc(Task_Fishing));
    }
    return FALSE;
}

static bool8 Fishing_NotEvenNibble(struct Task *task)
{
    AlignFishingAnimationFrames();
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetFishingNoCatchDirectionAnimNum(GetPlayerFacingDirection()));
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized2(0, FONT_NORMAL, gText_NotEvenANibble, 1, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    task->tStep = FISHING_SHOW_RESULT;
    return TRUE;
}

static bool8 Fishing_GotAway(struct Task *task)
{
    AlignFishingAnimationFrames();
    StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], GetFishingNoCatchDirectionAnimNum(GetPlayerFacingDirection()));
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized2(0, FONT_NORMAL, gText_ItGotAway, 1, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    task->tStep++;
    return TRUE;
}

static bool8 Fishing_NoMon(struct Task *task)
{
    AlignFishingAnimationFrames();
    task->tStep++;
    return FALSE;
}

static bool8 Fishing_PutRodAway(struct Task *task)
{
    AlignFishingAnimationFrames();
    if (gSprites[gPlayerAvatar.spriteId].animEnded)
    {
        struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

        ObjectEventSetGraphicsId(playerObjEvent, task->tPlayerGfxId);
        ObjectEventTurn(playerObjEvent, playerObjEvent->movementDirection);
        if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
            SetSurfBlob_PlayerOffset(gObjectEvents[gPlayerAvatar.objectEventId].fieldEffectSpriteId, FALSE, 0);
        gSprites[gPlayerAvatar.spriteId].x2 = 0;
        gSprites[gPlayerAvatar.spriteId].y2 = 0;
        task->tStep++;
    }
    return FALSE;
}

static bool8 Fishing_EndNoMon(struct Task *task)
{
    RunTextPrinters();
    if (!IsTextPrinterActive(0))
    {
        gPlayerAvatar.preventStep = FALSE;
        UnlockPlayerFieldControls();
        UnfreezeObjectEvents();
        ClearDialogWindowAndFrame(0, TRUE);
        RecordFishingAttemptForTV(FALSE);
        DestroyTask(FindTaskIdByFunc(Task_Fishing));
    }
    return FALSE;
}

#undef tStep
#undef tFrameCounter
#undef tFishingRod

static void AlignFishingAnimationFrames(void)
{
    struct Sprite *playerSprite = &gSprites[gPlayerAvatar.spriteId];
    u8 animCmdIndex;
    u8 animType;

    AnimateSprite(playerSprite);
    playerSprite->x2 = 0;
    playerSprite->y2 = 0;
    animCmdIndex = playerSprite->animCmdIndex;
    if (playerSprite->anims[playerSprite->animNum][animCmdIndex].type == -1)
    {
        animCmdIndex--;
    }
    else
    {
        playerSprite->animDelayCounter++;
        if (playerSprite->anims[playerSprite->animNum][animCmdIndex].type == -1)
            animCmdIndex--;
    }
    animType = playerSprite->anims[playerSprite->animNum][animCmdIndex].type;
    if (animType == 1 || animType == 2 || animType == 3)
    {
        playerSprite->x2 = 8;
        if (GetPlayerFacingDirection() == 3)
            playerSprite->x2 = -8;
    }
    if (animType == 5)
        playerSprite->y2 = -8;
    if (animType == 10 || animType == 11)
        playerSprite->y2 = 8;
    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
        SetSurfBlob_PlayerOffset(gObjectEvents[gPlayerAvatar.objectEventId].fieldEffectSpriteId, TRUE, playerSprite->y2);
}

void SetSpinStartFacingDir(u8 direction)
{
    sSpinStartFacingDir = direction;
}

static u8 GetSpinStartFacingDir(void)
{
    if (sSpinStartFacingDir == DIR_NONE)
        return DIR_SOUTH;

    return sSpinStartFacingDir;
}

// Task data for Task_DoPlayerSpinEntrance and Task_DoPlayerSpinExit
#define tState          data[0]
#define tSpinDelayTimer data[1]
#define tSpeed          data[2]
#define tCurY           data[3]
#define tDestY          data[4]
#define tStartDir       data[5]
#define tPriority       data[6]
#define tSubpriority    data[7]
#define tGroundTimer    data[8]

static void Task_DoPlayerSpinExit(u8 taskId)
{
    struct ObjectEvent *object = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite *sprite = &gSprites[object->spriteId];
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
        case 0: // Init
            if (!ObjectEventClearHeldMovementIfFinished(object))
                return;

            SetSpinStartFacingDir(object->facingDirection);
            tSpinDelayTimer = 0;
            tSpeed = 1;
            tCurY = (u16)(sprite->y + sprite->y2) << 4;
            sprite->y2 = 0;
            CameraObjectFreeze();
            object->fixedPriority = TRUE;
            sprite->oam.priority = 0;
            sprite->subpriority = 0;
            sprite->subspriteMode = SUBSPRITES_OFF;
            tState++;
        case 1: // Spin while rising
            TrySpinPlayerForWarp(object, &tSpinDelayTimer);

            // Rise and accelerate
            tCurY -= tSpeed;
            tSpeed += 3;
            sprite->y = tCurY >> 4;

            // Check if offscreen
            if (sprite->y + (s16)gTotalCameraPixelOffsetY < -32)
                tState++;
            break;
        case 2:
            DestroyTask(taskId);
            break;
    }
}

static void Task_DoPlayerSpinEntrance(u8 taskId);

void DoPlayerSpinEntrance(void)
{
    Task_DoPlayerSpinEntrance(CreateTask(Task_DoPlayerSpinEntrance, 0));
}

bool32 IsPlayerSpinEntranceActive(void)
{
    return FuncIsActiveTask(Task_DoPlayerSpinEntrance);
}

void DoPlayerSpinExit(void)
{
    Task_DoPlayerSpinExit(CreateTask(Task_DoPlayerSpinExit, 0));
}

bool32 IsPlayerSpinExitActive(void)
{
    return FuncIsActiveTask(Task_DoPlayerSpinExit);
}

static const u8 sSpinDirections[] = {DIR_SOUTH, DIR_WEST, DIR_EAST, DIR_NORTH, DIR_SOUTH};

static void Task_DoPlayerSpinEntrance(u8 taskId)
{
    struct ObjectEvent *object = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite *sprite = &gSprites[object->spriteId];
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
        case 0:
            // Because the spin start facing direction is never set for this
            // warp type, the player will always exit the warp facing South.
            // This may have been intentional, unclear
            tStartDir = GetSpinStartFacingDir();
            ObjectEventForceSetHeldMovement(object, GetFaceDirectionMovementAction(sSpinDirections[tStartDir]));
            tSpinDelayTimer = 0;
            tSpeed = 116;
            tDestY = sprite->y;
            tPriority = sprite->oam.priority;
            tSubpriority = sprite->subpriority;
            tCurY = -((u16)sprite->y2 + 32) * 16;
            sprite->y2 = 0;
            CameraObjectFreeze();
            object->fixedPriority = TRUE;
            sprite->oam.priority = 1;
            sprite->subpriority = 0;
            sprite->subspriteMode = SUBSPRITES_OFF;
            tState++;
        case 1: // Spin while descending
            TrySpinPlayerForWarp(object, &tSpinDelayTimer);

            // Fall and decelerate
            tCurY += tSpeed;
            tSpeed -= 3;
            if (tSpeed < 4)
                tSpeed = 4;
            sprite->y = tCurY >> 4;

            // Check if reached dest
            if (sprite->y >= tDestY)
            {
                sprite->y = tDestY;
                tGroundTimer = 0;
                tState++;
            }
            break;
        case 2: // Spin on ground
            TrySpinPlayerForWarp(object, &tSpinDelayTimer);
            if (++tGroundTimer > 8)
                tState++;
            break;
        case 3: // Spin until facing original direction
            if (tStartDir == TrySpinPlayerForWarp(object, &tSpinDelayTimer))
            {
                object->fixedPriority = 0;
                sprite->oam.priority = tPriority;
                sprite->subpriority = tSubpriority;
                CameraObjectReset();
                DestroyTask(taskId);
            }
            break;
    }
}

static u8 TrySpinPlayerForWarp(struct ObjectEvent *object, s16 *delayTimer)
{
    if (*delayTimer < 8 && ++(*delayTimer) < 8)
        return object->facingDirection;

    if (!ObjectEventCheckHeldMovementStatus(object))
        return object->facingDirection;

    ObjectEventForceSetHeldMovement(object, GetFaceDirectionMovementAction(sSpinDirections[object->facingDirection]));
    *delayTimer = 0;
    return sSpinDirections[object->facingDirection];
}
