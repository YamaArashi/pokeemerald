#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_setup.h"
#include "decoration.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_camera.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_specials.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "international_string_util.h"
#include "item_menu.h"
#include "link.h"
#include "list_menu.h"
#include "main.h"
#include "map_name_popup.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "secret_base.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "tv.h"
#include "window.h"
#include "constants/event_bg.h"
#include "constants/decorations.h"
#include "constants/event_objects.h"
#include "constants/field_specials.h"
#include "constants/items.h"
#include "constants/map_types.h"
#include "constants/metatile_behaviors.h"
#include "constants/metatile_labels.h"
#include "constants/moves.h"
#include "constants/secret_bases.h"
#include "constants/songs.h"
#include "constants/trainers.h"

#define TAG_SCROLL_ARROW 5112

// Values for registryStatus
enum {
    UNREGISTERED,
    REGISTERED,
    NEW, // Temporary, so new secret bases mixed at the same time don't overwrite each other
};

struct SecretBaseRegistryMenu
{
    struct ListMenuItem items[11];
    u8 names[11][32];
};

struct SecretBaseRecordMixer
{
    struct SecretBase *secretBases;
    u32 version;
    u32 language;
};

struct SecretBaseEntranceMetatiles
{
    u16 closedMetatileId;
    u16 openMetatileId;
};

static EWRAM_DATA u8 sCurSecretBaseId = 0;
static EWRAM_DATA bool8 sInFriendSecretBase = FALSE;
static EWRAM_DATA struct SecretBaseRegistryMenu *sRegistryMenu = NULL;

static void Task_ShowSecretBaseRegistryMenu(u8);
static void BuildRegistryMenuItems(u8);
static void RegistryMenu_OnCursorMove(s32, bool8, struct ListMenu *);
static void FinalizeRegistryMenu(u8);
static void AddRegistryMenuScrollArrows(u8);
static void HandleRegistryMenuInput(u8);
static void ShowRegistryMenuActions(u8);
static void HandleRegistryMenuActionsInput(u8);
static void ShowRegistryMenuDeleteConfirmation(u8);
static void ShowRegistryMenuDeleteYesNo(u8);
static void DeleteRegistry_Yes(u8);
static void DeleteRegistry_No(u8);
static void ReturnToMainRegistryMenu(u8);
static void GoToSecretBasePCRegisterMenu(u8);
static u8 GetSecretBaseOwnerType(u8);

static const struct SecretBaseEntranceMetatiles sSecretBaseEntranceMetatiles[] =
{
    {.closedMetatileId = METATILE_General_SecretBase_TreeLeft,  .openMetatileId = METATILE_General_SecretBase_VineLeft},
    {.closedMetatileId = METATILE_General_SecretBase_TreeRight, .openMetatileId = METATILE_General_SecretBase_VineRight},
    {.closedMetatileId = METATILE_General_RedCaveIndent,        .openMetatileId = METATILE_General_RedCaveOpen},
    {.closedMetatileId = METATILE_General_YellowCaveIndent,     .openMetatileId = METATILE_General_YellowCaveOpen},
    {.closedMetatileId = METATILE_General_BlueCaveIndent,       .openMetatileId = METATILE_General_BlueCaveOpen},
    {.closedMetatileId = METATILE_Fallarbor_BrownCaveIndent,    .openMetatileId = METATILE_Fallarbor_BrownCaveOpen},
    {.closedMetatileId = METATILE_Fortree_SecretBase_Shrub,     .openMetatileId = METATILE_Fortree_SecretBase_ShrubOpen},
};

// mapNum, warpId, x, y
// x, y positions are for when the player warps in for the first time (in front of the computer)
static const u8 sSecretBaseEntrancePositions[NUM_SECRET_BASE_GROUPS * 4] =
{
    [SECRET_BASE_RED_CAVE1]    = MAP_NUM(SECRET_BASE_RED_CAVE1),    0,  1,  3,
    [SECRET_BASE_RED_CAVE2]    = MAP_NUM(SECRET_BASE_RED_CAVE2),    0,  5,  9,
    [SECRET_BASE_RED_CAVE3]    = MAP_NUM(SECRET_BASE_RED_CAVE3),    0,  1,  3,
    [SECRET_BASE_RED_CAVE4]    = MAP_NUM(SECRET_BASE_RED_CAVE4),    0,  7, 13,
    [SECRET_BASE_BROWN_CAVE1]  = MAP_NUM(SECRET_BASE_BROWN_CAVE1),  0,  2,  3,
    [SECRET_BASE_BROWN_CAVE2]  = MAP_NUM(SECRET_BASE_BROWN_CAVE2),  0,  9,  2,
    [SECRET_BASE_BROWN_CAVE3]  = MAP_NUM(SECRET_BASE_BROWN_CAVE3),  0, 13,  4,
    [SECRET_BASE_BROWN_CAVE4]  = MAP_NUM(SECRET_BASE_BROWN_CAVE4),  0,  1,  2,
    [SECRET_BASE_BLUE_CAVE1]   = MAP_NUM(SECRET_BASE_BLUE_CAVE1),   0,  1,  3,
    [SECRET_BASE_BLUE_CAVE2]   = MAP_NUM(SECRET_BASE_BLUE_CAVE2),   0,  1,  2,
    [SECRET_BASE_BLUE_CAVE3]   = MAP_NUM(SECRET_BASE_BLUE_CAVE3),   0,  3, 15,
    [SECRET_BASE_BLUE_CAVE4]   = MAP_NUM(SECRET_BASE_BLUE_CAVE4),   0,  3, 14,
    [SECRET_BASE_YELLOW_CAVE1] = MAP_NUM(SECRET_BASE_YELLOW_CAVE1), 0,  9,  3,
    [SECRET_BASE_YELLOW_CAVE2] = MAP_NUM(SECRET_BASE_YELLOW_CAVE2), 0,  8,  7,
    [SECRET_BASE_YELLOW_CAVE3] = MAP_NUM(SECRET_BASE_YELLOW_CAVE3), 0,  3,  6,
    [SECRET_BASE_YELLOW_CAVE4] = MAP_NUM(SECRET_BASE_YELLOW_CAVE4), 0,  5,  9,
    [SECRET_BASE_TREE1]        = MAP_NUM(SECRET_BASE_TREE1),        0,  2,  3,
    [SECRET_BASE_TREE2]        = MAP_NUM(SECRET_BASE_TREE2),        0,  5,  6,
    [SECRET_BASE_TREE3]        = MAP_NUM(SECRET_BASE_TREE3),        0, 15,  3,
    [SECRET_BASE_TREE4]        = MAP_NUM(SECRET_BASE_TREE4),        0,  4, 10,
    [SECRET_BASE_SHRUB1]       = MAP_NUM(SECRET_BASE_SHRUB1),       0,  3,  3,
    [SECRET_BASE_SHRUB2]       = MAP_NUM(SECRET_BASE_SHRUB2),       0,  1,  2,
    [SECRET_BASE_SHRUB3]       = MAP_NUM(SECRET_BASE_SHRUB3),       0,  7,  8,
    [SECRET_BASE_SHRUB4]       = MAP_NUM(SECRET_BASE_SHRUB4),       0,  9,  6,
};

#define GET_BASE_MAP_NUM(group)    (sSecretBaseEntrancePositions[(group) + 0])
#define GET_BASE_WARP_ID(group)    (sSecretBaseEntrancePositions[(group) + 1])
#define GET_BASE_COMPUTER_X(group) (sSecretBaseEntrancePositions[(group) + 2])
#define GET_BASE_COMPUTER_Y(group) (sSecretBaseEntrancePositions[(group) + 3])

static const struct MenuAction sRegistryMenuActions[] =
{
    {
        .text = gText_DelRegist,
        .func = { .void_u8 = ShowRegistryMenuDeleteConfirmation },
    },
    {
        .text = gText_Cancel,
        .func = { .void_u8 = ReturnToMainRegistryMenu },
    },
};

static const struct YesNoFuncTable sDeleteRegistryYesNoFuncs =
{
    .yesFunc = DeleteRegistry_Yes,
    .noFunc = DeleteRegistry_No,
};

static const u8 sSecretBaseOwnerGfxIds[10] =
{
    // Male
    OBJ_EVENT_GFX_YOUNGSTER,
    OBJ_EVENT_GFX_BUG_CATCHER,
    OBJ_EVENT_GFX_RICH_BOY,
    OBJ_EVENT_GFX_CAMPER,
    OBJ_EVENT_GFX_MAN_3,
    // Female
    OBJ_EVENT_GFX_LASS,
    OBJ_EVENT_GFX_GIRL_3,
    OBJ_EVENT_GFX_WOMAN_2,
    OBJ_EVENT_GFX_PICNICKER,
    OBJ_EVENT_GFX_WOMAN_5,
};

static const struct WindowTemplate sRegistryWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 1,
        .width = 11,
        .height = 18,
        .paletteNum = 15,
        .baseBlock = 0x01,
    },
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0xc7,
    }
};

static const struct ListMenuTemplate sRegistryListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = RegistryMenu_OnCursorMove,
    .itemPrintFunc = NULL,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 9,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NORMAL,
    .cursorKind = CURSOR_BLACK_ARROW,
};

static void ClearSecretBase(struct SecretBase *secretBase)
{
    u16 i;
    CpuFastFill16(0, secretBase, sizeof(struct SecretBase));
    for (i = 0; i < PLAYER_NAME_LENGTH; i++)
        secretBase->trainerName[i] = EOS;
}

void ClearSecretBases(void)
{
    u16 i;
    for (i = 0; i < SECRET_BASES_COUNT; i++)
        ClearSecretBase(&gSaveBlock1Ptr->secretBases[i]);
}

static void SetCurSecretBaseId(void)
{
    sCurSecretBaseId = gSpecialVar_0x8004;
}

void TrySetCurSecretBaseIndex(void)
{
    u16 i;

    gSpecialVar_Result = FALSE;
    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (sCurSecretBaseId == gSaveBlock1Ptr->secretBases[i].secretBaseId)
        {
            gSpecialVar_Result = TRUE;
            VarSet(VAR_CURRENT_SECRET_BASE, i);
            break;
        }
    }
}

void CheckPlayerHasSecretBase(void)
{
    // The player's secret base is always the first in the array.
    if (gSaveBlock1Ptr->secretBases[0].secretBaseId)
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

static u8 GetSecretBaseTypeInFrontOfPlayer_(void)
{
    s16 x, y;
    s16 behavior;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    behavior = MapGridGetMetatileBehaviorAt(x, y) & 0xFFF;
    if (behavior == MB_SECRET_BASE_SPOT_RED_CAVE || behavior == MB_SECRET_BASE_SPOT_RED_CAVE_OPEN)
        return SECRET_BASE_RED_CAVE;

    if (behavior == MB_SECRET_BASE_SPOT_BROWN_CAVE || behavior == MB_SECRET_BASE_SPOT_BROWN_CAVE_OPEN)
        return SECRET_BASE_BROWN_CAVE;

    if (behavior == MB_SECRET_BASE_SPOT_BLUE_CAVE || behavior == MB_SECRET_BASE_SPOT_BLUE_CAVE_OPEN)
        return SECRET_BASE_BLUE_CAVE;

    if (behavior == MB_SECRET_BASE_SPOT_YELLOW_CAVE || behavior == MB_SECRET_BASE_SPOT_YELLOW_CAVE_OPEN)
        return SECRET_BASE_YELLOW_CAVE;

    if (behavior == MB_SECRET_BASE_SPOT_TREE_LEFT  || behavior == MB_SECRET_BASE_SPOT_TREE_LEFT_OPEN
     || behavior == MB_SECRET_BASE_SPOT_TREE_RIGHT || behavior == MB_SECRET_BASE_SPOT_TREE_RIGHT_OPEN)
        return SECRET_BASE_TREE;

    if (behavior == MB_SECRET_BASE_SPOT_SHRUB || behavior == MB_SECRET_BASE_SPOT_SHRUB_OPEN)
        return SECRET_BASE_SHRUB;

    return 0;
}

void GetSecretBaseTypeInFrontOfPlayer(void)
{
    gSpecialVar_0x8007 = GetSecretBaseTypeInFrontOfPlayer_();
}

static void FindMetatileIdMapCoords(s16 *x, s16 *y, u16 metatileId)
{
    s16 i, j;
    const struct MapLayout *mapLayout = gMapHeader.mapLayout;

    for (j = 0; j < mapLayout->height; j++)
    {
        for (i = 0; i < mapLayout->width; i++)
        {
            if ((mapLayout->map[j * mapLayout->width + i] & MAPGRID_METATILE_ID_MASK) == metatileId)
            {
                *x = i;
                *y = j;
                return;
            }
        }
    }
}

// Opens or closes the secret base entrance metatile in front of the player.
void ToggleSecretBaseEntranceMetatile(void)
{
    u16 i;
    s16 x, y;
    s16 metatileId;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    metatileId = MapGridGetMetatileIdAt(x, y);

    // Look for entrance metatiles to open
    for (i = 0; i < ARRAY_COUNT(sSecretBaseEntranceMetatiles); i++)
    {
        if (sSecretBaseEntranceMetatiles[i].closedMetatileId == metatileId)
        {
            MapGridSetMetatileIdAt(x, y, sSecretBaseEntranceMetatiles[i].openMetatileId | MAPGRID_COLLISION_MASK);
            CurrentMapDrawMetatileAt(x, y);
            return;
        }
    }

    // Look for entrance metatiles to close
    for (i = 0; i < ARRAY_COUNT(sSecretBaseEntranceMetatiles); i++)
    {
        if (sSecretBaseEntranceMetatiles[i].openMetatileId == metatileId)
        {
            MapGridSetMetatileIdAt(x, y, sSecretBaseEntranceMetatiles[i].closedMetatileId | MAPGRID_COLLISION_MASK);
            CurrentMapDrawMetatileAt(x, y);
            return;
        }
    }
}

static u8 GetNameLength(const u8 *secretBaseOwnerName)
{
    u8 i;
    for (i = 0; i < PLAYER_NAME_LENGTH; i++)
    {
        if (secretBaseOwnerName[i] == EOS)
            return i;
    }

    return PLAYER_NAME_LENGTH;
}

void SetPlayerSecretBase(void)
{
    u16 i;

    gSaveBlock1Ptr->secretBases[0].secretBaseId = sCurSecretBaseId;
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        gSaveBlock1Ptr->secretBases[0].trainerId[i] = gSaveBlock2Ptr->playerTrainerId[i];

    VarSet(VAR_CURRENT_SECRET_BASE, 0);
    StringCopyN(gSaveBlock1Ptr->secretBases[0].trainerName, gSaveBlock2Ptr->playerName, GetNameLength(gSaveBlock2Ptr->playerName));
    gSaveBlock1Ptr->secretBases[0].gender = gSaveBlock2Ptr->playerGender;
    gSaveBlock1Ptr->secretBases[0].language = GAME_LANGUAGE;
    VarSet(VAR_SECRET_BASE_MAP, gMapHeader.regionMapSectionId);
}

// Set the 'open' entrance metatile for any occupied secret base on this map
void SetOccupiedSecretBaseEntranceMetatiles(struct MapEvents const *events)
{
    u16 bgId;
    u16 i, j;

    for (bgId = 0; bgId < events->bgEventCount; bgId++)
    {
        if (events->bgEvents[bgId].kind == BG_EVENT_SECRET_BASE)
        {
            for (j = 0; j < SECRET_BASES_COUNT; j++)
            {
                if (gSaveBlock1Ptr->secretBases[j].secretBaseId == events->bgEvents[bgId].bgUnion.secretBaseId)
                {
                    s16 x = events->bgEvents[bgId].x + MAP_OFFSET;
                    s16 y = events->bgEvents[bgId].y + MAP_OFFSET;
                    s16 tile_id = MapGridGetMetatileIdAt(x, y);
                    for (i = 0; i < ARRAY_COUNT(sSecretBaseEntranceMetatiles); i++)
                    {
                        if (sSecretBaseEntranceMetatiles[i].closedMetatileId == tile_id)
                        {
                            MapGridSetMetatileIdAt(x, y, sSecretBaseEntranceMetatiles[i].openMetatileId | MAPGRID_COLLISION_MASK);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

static void SetSecretBaseWarpDestination(void)
{
    s8 secretBaseGroup = SECRET_BASE_ID_TO_GROUP(sCurSecretBaseId);
    SetWarpDestinationToMapWarp(MAP_GROUP(SECRET_BASE_RED_CAVE1), GET_BASE_MAP_NUM(secretBaseGroup), GET_BASE_WARP_ID(secretBaseGroup));
}

#define tState data[0]

static void Task_EnterSecretBase(u8 taskId)
{
    u16 secretBaseIdx;

    switch (gTasks[taskId].tState)
    {
    case 0:
        if (!gPaletteFade.active)
            gTasks[taskId].tState = 1;
        break;
    case 1:
        secretBaseIdx = VarGet(VAR_CURRENT_SECRET_BASE);
        if (gSaveBlock1Ptr->secretBases[secretBaseIdx].numTimesEntered < 255)
            gSaveBlock1Ptr->secretBases[secretBaseIdx].numTimesEntered++;

        SetSecretBaseWarpDestination();
        WarpIntoMap();
        gFieldCallback = FieldCB_ContinueScriptHandleMusic;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
        break;
    }
}

#undef tState

void EnterSecretBase(void)
{
    CreateTask(Task_EnterSecretBase, 0);
    FadeScreen(FADE_TO_BLACK, 0);
    SetDynamicWarp(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE);
}

bool8 SecretBaseMapPopupEnabled(void)
{
    if (gMapHeader.mapType == MAP_TYPE_SECRET_BASE && VarGet(VAR_INIT_SECRET_BASE) == 0)
        return FALSE;

    return TRUE;
}

static void EnterNewlyCreatedSecretBase_WaitFadeIn(u8 taskId)
{
    ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], DIR_NORTH);
    if (IsWeatherNotFadingIn() == TRUE)
    {
        ScriptContext_Enable();
        DestroyTask(taskId);
    }
}

static void EnterNewlyCreatedSecretBase_StartFadeIn(void)
{
    s16 x, y;

    LockPlayerFieldControls();
    HideMapNamePopUpWindow();
    FindMetatileIdMapCoords(&x, &y, METATILE_SecretBase_PC);
    x += MAP_OFFSET;
    y += MAP_OFFSET;
    MapGridSetMetatileIdAt(x, y, METATILE_SecretBase_PC | MAPGRID_COLLISION_MASK);
    CurrentMapDrawMetatileAt(x, y);
    FadeInFromBlack();
    CreateTask(EnterNewlyCreatedSecretBase_WaitFadeIn, 0);
}

static void Task_EnterNewlyCreatedSecretBase(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        s8 secretBaseGroup = SECRET_BASE_ID_TO_GROUP(sCurSecretBaseId);
        SetWarpDestination(
            gSaveBlock1Ptr->location.mapGroup,
            gSaveBlock1Ptr->location.mapNum,
            WARP_ID_NONE,
            GET_BASE_COMPUTER_X(secretBaseGroup),
            GET_BASE_COMPUTER_Y(secretBaseGroup));
        WarpIntoMap();
        gFieldCallback = EnterNewlyCreatedSecretBase_StartFadeIn;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
    }
}

void EnterNewlyCreatedSecretBase(void)
{
    CreateTask(Task_EnterNewlyCreatedSecretBase, 0);
    FadeScreen(FADE_TO_BLACK, 0);
}

bool8 CurMapIsSecretBase(void)
{
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(SECRET_BASE_RED_CAVE1)
     && (u8)gSaveBlock1Ptr->location.mapNum <= MAP_NUM(SECRET_BASE_SHRUB4))
        return TRUE;
    else
        return FALSE;
}

void InitSecretBaseAppearance(bool8 hidePC)
{
    u16 secretBaseIdx;
    u16 x, y;
    u8 *decorations;
    u8 *decorPos;

    if (CurMapIsSecretBase())
    {
        secretBaseIdx = VarGet(VAR_CURRENT_SECRET_BASE);
        decorations = gSaveBlock1Ptr->secretBases[secretBaseIdx].decorations;
        decorPos = gSaveBlock1Ptr->secretBases[secretBaseIdx].decorationPositions;
        for (x = 0; x < DECOR_MAX_SECRET_BASE; x++)
        {
            if (decorations[x] > 0 && decorations[x] <= NUM_DECORATIONS && gDecorations[decorations[x]].permission != DECORPERM_SPRITE)
                ShowDecorationOnMap((decorPos[x] >> 4) + MAP_OFFSET, (decorPos[x] & 0xF) + MAP_OFFSET, decorations[x]);
        }

        if (secretBaseIdx != 0)
        {
            // Another player's secret base. Change PC type to the "Register" PC.
            FindMetatileIdMapCoords(&x, &y, METATILE_SecretBase_PC);
            MapGridSetMetatileIdAt(x + MAP_OFFSET, y + MAP_OFFSET, METATILE_SecretBase_RegisterPC | MAPGRID_COLLISION_MASK);
        }
        else if (hidePC == TRUE && VarGet(VAR_SECRET_BASE_INITIALIZED) == 1)
        {
            // Change PC to regular ground tile.
            FindMetatileIdMapCoords(&x, &y, METATILE_SecretBase_PC);
            MapGridSetMetatileIdAt(x + MAP_OFFSET, y + MAP_OFFSET, METATILE_SecretBase_Ground | MAPGRID_COLLISION_MASK);
        }
    }
}

void InitSecretBaseDecorationSprites(void)
{
    u8 i;
    u8 *decorations;
    u8 *decorationPositions;
    u8 objectEventId;
    u8 metatileBehavior;
    u8 category;
    u8 permission;
    u8 numDecorations;

    objectEventId = 0;
    if (!CurMapIsSecretBase())
    {
        decorations = gSaveBlock1Ptr->playerRoomDecorations;
        decorationPositions = gSaveBlock1Ptr->playerRoomDecorationPositions;
        numDecorations = DECOR_MAX_PLAYERS_HOUSE;
    }
    else
    {
        u16 secretBaseIdx = VarGet(VAR_CURRENT_SECRET_BASE);
        decorations = gSaveBlock1Ptr->secretBases[secretBaseIdx].decorations;
        decorationPositions = gSaveBlock1Ptr->secretBases[secretBaseIdx].decorationPositions;
        numDecorations = DECOR_MAX_SECRET_BASE;
    }

    for (i = 0; i < numDecorations; i++)
    {
        if (decorations[i] == DECOR_NONE)
            continue;

        permission = gDecorations[decorations[i]].permission;
        category = gDecorations[decorations[i]].category;
        if (permission == DECORPERM_SPRITE)
        {
            for (objectEventId = 0; objectEventId < gMapHeader.events->objectEventCount; objectEventId++)
            {
                if (gMapHeader.events->objectEvents[objectEventId].flagId == FLAG_DECORATION_1 + gSpecialVar_0x8004)
                    break;
            }

            if (objectEventId == gMapHeader.events->objectEventCount)
                continue;

            gSpecialVar_0x8006 = decorationPositions[i] >> 4;
            gSpecialVar_0x8007 = decorationPositions[i] & 0xF;
            metatileBehavior = MapGridGetMetatileBehaviorAt(gSpecialVar_0x8006 + MAP_OFFSET, gSpecialVar_0x8007 + MAP_OFFSET);
            if (MetatileBehavior_HoldsSmallDecoration(metatileBehavior) == TRUE
             || MetatileBehavior_HoldsLargeDecoration(metatileBehavior) == TRUE)
            {
                gSpecialVar_Result = VAR_OBJ_GFX_ID_0 + (gMapHeader.events->objectEvents[objectEventId].graphicsId - OBJ_EVENT_GFX_VAR_0);
                VarSet(gSpecialVar_Result, gDecorations[decorations[i]].tiles[0]);
                gSpecialVar_Result = gMapHeader.events->objectEvents[objectEventId].localId;
                FlagClear(FLAG_DECORATION_1 + gSpecialVar_0x8004);
                TrySpawnObjectEvent(gSpecialVar_Result, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
                TryMoveObjectEventToMapCoords(gSpecialVar_Result, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, gSpecialVar_0x8006, gSpecialVar_0x8007);
                TryOverrideObjectEventTemplateCoords(gSpecialVar_Result, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
                if (CurMapIsSecretBase() == TRUE && VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                {
                    if (category == DECORCAT_DOLL)
                    {
                        OverrideSecretBaseDecorationSpriteScript(
                            gSpecialVar_Result,
                            gSaveBlock1Ptr->location.mapNum,
                            gSaveBlock1Ptr->location.mapGroup,
                            DECORCAT_DOLL);
                    }
                    else if (category == DECORCAT_CUSHION)
                    {
                        OverrideSecretBaseDecorationSpriteScript(
                            gSpecialVar_Result,
                            gSaveBlock1Ptr->location.mapNum,
                            gSaveBlock1Ptr->location.mapGroup,
                            DECORCAT_CUSHION);
                    }
                }

                gSpecialVar_0x8004++;
            }
        }
    }
}

void HideSecretBaseDecorationSprites(void)
{
    u8 objectEventId;
    u16 flag;

    for (objectEventId = 0; objectEventId < gMapHeader.events->objectEventCount; objectEventId++)
    {
        flag = gMapHeader.events->objectEvents[objectEventId].flagId;
        if (flag >= FLAG_DECORATION_1 && flag <= FLAG_DECORATION_14)
        {
            RemoveObjectEventByLocalIdAndMap(
                gMapHeader.events->objectEvents[objectEventId].localId,
                gSaveBlock1Ptr->location.mapNum,
                gSaveBlock1Ptr->location.mapGroup);
            FlagSet(flag);
        }
    }
}

void SetSecretBaseOwnerGfxId(void)
{
    VarSet(VAR_OBJ_GFX_ID_F, sSecretBaseOwnerGfxIds[GetSecretBaseOwnerType(VarGet(VAR_CURRENT_SECRET_BASE))]);
}

void SetCurSecretBaseIdFromPosition(const struct MapPosition *position, const struct MapEvents *events)
{
    s16 i;
    for (i = 0; i < events->bgEventCount; i++)
    {
        if (events->bgEvents[i].kind == BG_EVENT_SECRET_BASE
          && position->x == events->bgEvents[i].x + MAP_OFFSET
          && position->y == events->bgEvents[i].y + MAP_OFFSET)
        {
            sCurSecretBaseId = events->bgEvents[i].bgUnion.secretBaseId;
            break;
        }
    }
}

void WarpIntoSecretBase(const struct MapPosition *position, const struct MapEvents *events)
{
    SetCurSecretBaseIdFromPosition(position, events);
    TrySetCurSecretBaseIndex();
    ScriptContext_SetupScript(SecretBase_EventScript_Enter);
}

bool8 TrySetCurSecretBase(void)
{
    SetCurSecretBaseId();
    TrySetCurSecretBaseIndex();
    if (gSpecialVar_Result == TRUE)
        return FALSE;

    return TRUE;
}

static void Task_WarpOutOfSecretBase(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        LockPlayerFieldControls();
        gTasks[taskId].data[0] = 1;
        break;
    case 1:
        if (!gPaletteFade.active)
            gTasks[taskId].data[0] = 2;
        break;
    case 2:
        SetWarpDestinationToDynamicWarp(WARP_ID_SECRET_BASE);
        WarpIntoMap();
        gFieldCallback = FieldCB_DefaultWarpExit;
        SetMainCallback2(CB2_LoadMap);
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
        break;
    }
}

static void WarpOutOfSecretBase(void)
{
    CreateTask(Task_WarpOutOfSecretBase, 0);
    FadeScreen(FADE_TO_BLACK, 0);
}

void IsCurSecretBaseOwnedByAnotherPlayer(void)
{
    if (gSaveBlock1Ptr->secretBases[0].secretBaseId != sCurSecretBaseId)
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

static u8 *GetSecretBaseName(u8 *dest, u8 secretBaseIdx)
{
    *StringCopyN(dest, gSaveBlock1Ptr->secretBases[secretBaseIdx].trainerName, GetNameLength(gSaveBlock1Ptr->secretBases[secretBaseIdx].trainerName)) = EOS;
    ConvertInternationalString(dest, gSaveBlock1Ptr->secretBases[secretBaseIdx].language);
    return StringAppend(dest, gText_ApostropheSBase);
}

u8 *GetSecretBaseMapName(u8 *dest)
{
    return GetSecretBaseName(dest, VarGet(VAR_CURRENT_SECRET_BASE));
}

void CopyCurSecretBaseOwnerName_StrVar1(void)
{
    u8 secretBaseIdx;
    const u8 *name;

    secretBaseIdx = VarGet(VAR_CURRENT_SECRET_BASE);
    name = gSaveBlock1Ptr->secretBases[secretBaseIdx].trainerName;
    *StringCopyN(gStringVar1, name, GetNameLength(name)) = EOS;
    ConvertInternationalString(gStringVar1, gSaveBlock1Ptr->secretBases[secretBaseIdx].language);
}

static bool8 IsSecretBaseRegistered(u8 secretBaseIdx)
{
    if (gSaveBlock1Ptr->secretBases[secretBaseIdx].registryStatus)
        return TRUE;

    return FALSE;
}

static u8 GetAverageEVs(struct Pokemon *pokemon)
{
    u16 evTotal;
    evTotal  = GetMonData(pokemon, MON_DATA_HP_EV);
    evTotal += GetMonData(pokemon, MON_DATA_ATK_EV);
    evTotal += GetMonData(pokemon, MON_DATA_DEF_EV);
    evTotal += GetMonData(pokemon, MON_DATA_SPEED_EV);
    evTotal += GetMonData(pokemon, MON_DATA_SPATK_EV);
    evTotal += GetMonData(pokemon, MON_DATA_SPDEF_EV);
    return evTotal / 6;
}

void SetPlayerSecretBaseParty(void)
{
    u16 i;
    u16 moveIdx;
    u16 partyId;
    struct SecretBaseParty *party;

    partyId = 0;
    party = &gSaveBlock1Ptr->secretBases[0].party;
    if (gSaveBlock1Ptr->secretBases[0].secretBaseId)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
                party->moves[i * MAX_MON_MOVES + moveIdx] = MOVE_NONE;

            party->species[i] = SPECIES_NONE;
            party->heldItems[i] = ITEM_NONE;
            party->levels[i] = 0;
            party->personality[i] = 0;
            party->EVs[i] = 0;

            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE
            && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            {
                for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
                    party->moves[partyId * MAX_MON_MOVES + moveIdx] = GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + moveIdx);

                party->species[partyId] = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES);
                party->heldItems[partyId] = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);
                party->levels[partyId] = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);
                party->personality[partyId] = GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY);
                party->EVs[partyId] = GetAverageEVs(&gPlayerParty[i]);
                partyId++;
            }
        }
    }
}

void ClearAndLeaveSecretBase(void)
{
    u16 temp = gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived;
    ClearSecretBase(&gSaveBlock1Ptr->secretBases[0]);
    gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived = temp;
    WarpOutOfSecretBase();
}

void MoveOutOfSecretBase(void)
{
    IncrementGameStat(GAME_STAT_MOVED_SECRET_BASE);
    ClearAndLeaveSecretBase();
}

static void ClosePlayerSecretBaseEntrance(void)
{
    u16 i;
    u16 j;
    s16 metatileId;
    const struct MapEvents *events = gMapHeader.events;

    for (i = 0; i < events->bgEventCount; i++)
    {
        if (events->bgEvents[i].kind == BG_EVENT_SECRET_BASE
         && gSaveBlock1Ptr->secretBases[0].secretBaseId == events->bgEvents[i].bgUnion.secretBaseId)
        {
            metatileId = MapGridGetMetatileIdAt(events->bgEvents[i].x + MAP_OFFSET, events->bgEvents[i].y + MAP_OFFSET);
            for (j = 0; j < ARRAY_COUNT(sSecretBaseEntranceMetatiles); j++)
            {
                if (sSecretBaseEntranceMetatiles[j].openMetatileId == metatileId)
                {
                    MapGridSetMetatileIdAt(events->bgEvents[i].x + MAP_OFFSET,
                                           events->bgEvents[i].y + MAP_OFFSET,
                                           sSecretBaseEntranceMetatiles[j].closedMetatileId | MAPGRID_COLLISION_MASK);
                    break;
                }
            }

            DrawWholeMapView();
            break;
        }
    }
}

// When the player moves to a new secret base by interacting with a new secret base
// entrance in the overworld.
void MoveOutOfSecretBaseFromOutside(void)
{
    u16 temp;

    ClosePlayerSecretBaseEntrance();
    IncrementGameStat(GAME_STAT_MOVED_SECRET_BASE);
    temp = gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived;
    ClearSecretBase(&gSaveBlock1Ptr->secretBases[0]);
    gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived = temp;
}

static u8 GetNumRegisteredSecretBases(void)
{
    s16 i;
    u8 count = 0;
    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (IsSecretBaseRegistered(i) == TRUE)
            count++;
    }

    return count;
}

void GetCurSecretBaseRegistrationValidity(void)
{
    if (IsSecretBaseRegistered(VarGet(VAR_CURRENT_SECRET_BASE)) == TRUE)
        gSpecialVar_Result = 1;
    else if (GetNumRegisteredSecretBases() >= 10)
        gSpecialVar_Result = 2;
    else
        gSpecialVar_Result = 0;
}

void ToggleCurSecretBaseRegistry(void)
{
    gSaveBlock1Ptr->secretBases[VarGet(VAR_CURRENT_SECRET_BASE)].registryStatus ^= 1;
    FlagSet(FLAG_SECRET_BASE_REGISTRY_ENABLED);
}

void ShowSecretBaseDecorationMenu(void)
{
    CreateTask(DoSecretBaseDecorationMenu, 0);
}

void ShowSecretBaseRegistryMenu(void)
{
    CreateTask(Task_ShowSecretBaseRegistryMenu, 0);
}

#define tNumBases       data[0]
#define tSelectedRow    data[1]
#define tScrollOffset   data[2]
#define tMaxShownItems  data[3]
#define tSelectedBaseId data[4]
#define tListTaskId     data[5]
#define tMainWindowId   data[6]
#define tActionWindowId data[7]
#define tArrowTaskId    data[8]

static void Task_ShowSecretBaseRegistryMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    LockPlayerFieldControls();
    tNumBases = GetNumRegisteredSecretBases();
    if (tNumBases != 0)
    {
        tSelectedRow = 0;
        tScrollOffset = 0;
        ClearDialogWindowAndFrame(0, FALSE);
        sRegistryMenu = AllocZeroed(sizeof(*sRegistryMenu));
        tMainWindowId = AddWindow(&sRegistryWindowTemplates[0]);
        BuildRegistryMenuItems(taskId);
        FinalizeRegistryMenu(taskId);
        gTasks[taskId].func = HandleRegistryMenuInput;
    }
    else
    {
        DisplayItemMessageOnField(taskId, gText_NoRegistry, GoToSecretBasePCRegisterMenu);
    }
}

static void BuildRegistryMenuItems(u8 taskId)
{
    s16 *data;
    u8 i;
    u8 count;

    data = gTasks[taskId].data;
    count = 0;
    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (IsSecretBaseRegistered(i))
        {
            GetSecretBaseName(sRegistryMenu->names[count], i);
            sRegistryMenu->items[count].name = sRegistryMenu->names[count];
            sRegistryMenu->items[count].id = i;
            count++;
        }
    }

    sRegistryMenu->items[count].name = gText_Cancel;
    sRegistryMenu->items[count].id = LIST_CANCEL;
    tNumBases = count + 1;
    if (tNumBases < 8)
        tMaxShownItems = tNumBases;
    else
        tMaxShownItems = 8;

    gMultiuseListMenuTemplate = sRegistryListMenuTemplate;
    gMultiuseListMenuTemplate.windowId = tMainWindowId;
    gMultiuseListMenuTemplate.totalItems = tNumBases;
    gMultiuseListMenuTemplate.items = sRegistryMenu->items;
    gMultiuseListMenuTemplate.maxShowed = tMaxShownItems;
}

static void RegistryMenu_OnCursorMove(s32 unused, bool8 flag, struct ListMenu *menu)
{
    if (flag != TRUE)
        PlaySE(SE_SELECT);
}

static void FinalizeRegistryMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    SetStandardWindowBorderStyle(tMainWindowId, FALSE);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, tScrollOffset, tSelectedRow);
    AddRegistryMenuScrollArrows(taskId);
    ScheduleBgCopyTilemapToVram(0);
}

static void AddRegistryMenuScrollArrows(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    tArrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 188, 12, 148, tNumBases - tMaxShownItems, TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &tScrollOffset);
}

static void HandleRegistryMenuInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    s32 input = ListMenu_ProcessInput(tListTaskId);
    ListMenuGetScrollAndRow(tListTaskId, &tScrollOffset, &tSelectedRow);

    switch (input)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        DestroyListMenuTask(tListTaskId, NULL, NULL);
        RemoveScrollIndicatorArrowPair(tArrowTaskId);
        ClearStdWindowAndFrame(tMainWindowId, FALSE);
        ClearWindowTilemap(tMainWindowId);
        RemoveWindow(tMainWindowId);
        ScheduleBgCopyTilemapToVram(0);
        Free(sRegistryMenu);
        GoToSecretBasePCRegisterMenu(taskId);
        break;
    default:
        PlaySE(SE_SELECT);
        tSelectedBaseId = input;
        ShowRegistryMenuActions(taskId);
        break;
    }
}

static void ShowRegistryMenuActions(u8 taskId)
{
    struct WindowTemplate template;
    s16 *data = gTasks[taskId].data;
    RemoveScrollIndicatorArrowPair(tArrowTaskId);
    template = sRegistryWindowTemplates[1];
    template.width = GetMaxWidthInMenuTable(sRegistryMenuActions, 2);
    tActionWindowId = AddWindow(&template);
    SetStandardWindowBorderStyle(tActionWindowId, FALSE);
    PrintMenuTable(tActionWindowId, ARRAY_COUNT(sRegistryMenuActions), sRegistryMenuActions);
    InitMenuInUpperLeftCornerNormal(tActionWindowId, ARRAY_COUNT(sRegistryMenuActions), 0);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = HandleRegistryMenuActionsInput;
}

static void HandleRegistryMenuActionsInput(u8 taskId)
{
    s8 input = Menu_ProcessInputNoWrap();
    switch (input)
    {
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        ReturnToMainRegistryMenu(taskId);
        break;
    case MENU_NOTHING_CHOSEN:
        break;
    default:
        PlaySE(SE_SELECT);
        sRegistryMenuActions[input].func.void_u8(taskId);
        break;
    }
}

static void ShowRegistryMenuDeleteConfirmation(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    ClearStdWindowAndFrame(tMainWindowId, FALSE);
    ClearStdWindowAndFrame(tActionWindowId, FALSE);
    ClearWindowTilemap(tMainWindowId);
    ClearWindowTilemap(tActionWindowId);
    RemoveWindow(tActionWindowId);
    ScheduleBgCopyTilemapToVram(0);
    GetSecretBaseName(gStringVar1, tSelectedBaseId);
    StringExpandPlaceholders(gStringVarBuffer, gText_OkayToDeleteFromRegistry);
    DisplayItemMessageOnField(taskId, gStringVarBuffer, ShowRegistryMenuDeleteYesNo);
}

static void ShowRegistryMenuDeleteYesNo(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sDeleteRegistryYesNoFuncs);
}

void DeleteRegistry_Yes_Callback(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    ClearDialogWindowAndFrame(0, FALSE);
    DestroyListMenuTask(tListTaskId, &tScrollOffset, &tSelectedRow);
    gSaveBlock1Ptr->secretBases[tSelectedBaseId].registryStatus = UNREGISTERED;
    BuildRegistryMenuItems(taskId);
    SetCursorWithinListBounds(&tScrollOffset, &tSelectedRow, tMaxShownItems, tNumBases);
    FinalizeRegistryMenu(taskId);
    gTasks[taskId].func = HandleRegistryMenuInput;
}

static void DeleteRegistry_Yes(u8 taskId)
{
    DisplayItemMessageOnField(taskId, gText_RegisteredDataDeleted, DeleteRegistry_Yes_Callback);
}

static void DeleteRegistry_No(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    ClearDialogWindowAndFrame(0, FALSE);
    DestroyListMenuTask(tListTaskId, &tScrollOffset, &tSelectedRow);
    FinalizeRegistryMenu(taskId);
    gTasks[taskId].func = HandleRegistryMenuInput;
}

static void ReturnToMainRegistryMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    AddRegistryMenuScrollArrows(taskId);
    ClearStdWindowAndFrame(tActionWindowId, FALSE);
    ClearWindowTilemap(tActionWindowId);
    RemoveWindow(tActionWindowId);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = HandleRegistryMenuInput;
}

static void GoToSecretBasePCRegisterMenu(u8 taskId)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) == 0)
        ScriptContext_SetupScript(SecretBase_EventScript_PCCancel);
    else
        ScriptContext_SetupScript(SecretBase_EventScript_ShowRegisterMenu);

    DestroyTask(taskId);
}

#undef tNumBases
#undef tSelectedRow
#undef tScrollOffset
#undef tMaxShownItems
#undef tSelectedBaseId
#undef tListTaskId
#undef tMainWindowId
#undef tActionWindowId
#undef tArrowTaskId

static u8 GetSecretBaseOwnerType(u8 secretBaseIdx)
{
    return (gSaveBlock1Ptr->secretBases[secretBaseIdx].trainerId[0] % 5)
         + (gSaveBlock1Ptr->secretBases[secretBaseIdx].gender * 5);
}

const u8 *GetSecretBaseTrainerLoseText(void)
{
    u8 ownerType = GetSecretBaseOwnerType(VarGet(VAR_CURRENT_SECRET_BASE));
    if (ownerType == 0)
        return SecretBase_Text_Trainer0Defeated;
    else if (ownerType == 1)
        return SecretBase_Text_Trainer1Defeated;
    else if (ownerType == 2)
        return SecretBase_Text_Trainer2Defeated;
    else if (ownerType == 3)
        return SecretBase_Text_Trainer3Defeated;
    else if (ownerType == 4)
        return SecretBase_Text_Trainer4Defeated;
    else if (ownerType == 5)
        return SecretBase_Text_Trainer5Defeated;
    else if (ownerType == 6)
        return SecretBase_Text_Trainer6Defeated;
    else if (ownerType == 7)
        return SecretBase_Text_Trainer7Defeated;
    else if (ownerType == 8)
        return SecretBase_Text_Trainer8Defeated;
    else
        return SecretBase_Text_Trainer9Defeated;
}

void PrepSecretBaseBattleFlags(void)
{
    TryGainNewFanFromCounter(FANCOUNTER_BATTLED_AT_BASE);
    gTrainerBattleOpponent_A = TRAINER_SECRET_BASE;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_SECRET_BASE;
}

void SetBattledOwnerFromResult(void)
{
    gSaveBlock1Ptr->secretBases[VarGet(VAR_CURRENT_SECRET_BASE)].battledOwnerToday = gSpecialVar_Result;
}

void GetSecretBaseOwnerAndState(void)
{
    u16 secretBaseIdx;
    u8 i;

    secretBaseIdx = VarGet(VAR_CURRENT_SECRET_BASE);
    if (!FlagGet(FLAG_DAILY_SECRET_BASE))
    {
        for (i = 0; i < SECRET_BASES_COUNT; i++)
            gSaveBlock1Ptr->secretBases[i].battledOwnerToday = FALSE;

        FlagSet(FLAG_DAILY_SECRET_BASE);
    }
    gSpecialVar_0x8004 = GetSecretBaseOwnerType(secretBaseIdx);
    gSpecialVar_Result = gSaveBlock1Ptr->secretBases[secretBaseIdx].battledOwnerToday;
}

#define tStepCb  data[0] // See Task_RunPerStepCallback
#define tState   data[1]
#define tPlayerX data[2]
#define tPlayerY data[3]
#define tFldEff  data[4]

void SecretBasePerStepCallback(u8 taskId)
{
    s16 x;
    s16 y;
    u8 behavior;
    u16 tileId;
    s16 *data;

    data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        if (VarGet(VAR_CURRENT_SECRET_BASE))
            sInFriendSecretBase = TRUE;
        else
            sInFriendSecretBase = FALSE;

        PlayerGetDestCoords(&tPlayerX, &tPlayerY);
        tState = 1;
        break;
    case 1:
        // End if player hasn't moved
        PlayerGetDestCoords(&x, &y);
        if (x == tPlayerX && y == tPlayerY)
            return;

        tPlayerX = x;
        tPlayerY = y;
        VarSet(VAR_SECRET_BASE_STEP_COUNTER, VarGet(VAR_SECRET_BASE_STEP_COUNTER) + 1);
        behavior = MapGridGetMetatileBehaviorAt(x, y);
        tileId = MapGridGetMetatileIdAt(x, y);
        if (tileId == METATILE_SecretBase_SolidBoard_Top || tileId == METATILE_SecretBase_SolidBoard_Bottom)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_SOLID_BOARD);
        }
        else if (tileId == METATILE_SecretBase_SmallChair
              || tileId == METATILE_SecretBase_PokemonChair
              || tileId == METATILE_SecretBase_HeavyChair
              || tileId == METATILE_SecretBase_PrettyChair
              || tileId == METATILE_SecretBase_ComfortChair
              || tileId == METATILE_SecretBase_RaggedChair
              || tileId == METATILE_SecretBase_BrickChair
              || tileId == METATILE_SecretBase_CampChair
              || tileId == METATILE_SecretBase_HardChair)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_CHAIR);
        }
        else if (tileId == METATILE_SecretBase_RedTent_DoorTop
              || tileId == METATILE_SecretBase_RedTent_Door
              || tileId == METATILE_SecretBase_BlueTent_DoorTop
              || tileId == METATILE_SecretBase_BlueTent_Door)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_TENT);
        }
        else if ((behavior == MB_IMPASSABLE_NORTHEAST && tileId == METATILE_SecretBase_Stand_CornerRight)
              || (behavior == MB_IMPASSABLE_NORTHWEST && MapGridGetMetatileIdAt(x, y) == METATILE_SecretBase_Stand_CornerLeft))
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_STAND);
        }
        else if (behavior == MB_IMPASSABLE_WEST_AND_EAST && tileId == METATILE_SecretBase_Slide_StairLanding)
        {
            if (sInFriendSecretBase == TRUE)
            {
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) ^ SECRET_BASE_USED_SLIDE);
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_DECLINED_SLIDE);
            }
        }
        else if (behavior == MB_SLIDE_SOUTH && tileId == METATILE_SecretBase_Slide_SlideTop)
        {
            if (sInFriendSecretBase == TRUE)
            {
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_SLIDE);
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) ^ SECRET_BASE_DECLINED_SLIDE);
            }
        }
        else if (MetatileBehavior_IsSecretBaseGlitterMat(behavior) == TRUE)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_GLITTER_MAT);
        }
        else if (MetatileBehavior_IsSecretBaseBalloon(behavior) == TRUE)
        {
            PopSecretBaseBalloon(MapGridGetMetatileIdAt(x, y), x, y);
            if (sInFriendSecretBase == TRUE)
            {
                switch ((int)MapGridGetMetatileIdAt(x, y))
                {
                case METATILE_SecretBase_RedBalloon:
                case METATILE_SecretBase_BlueBalloon:
                case METATILE_SecretBase_YellowBalloon:
                    VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_BALLOON);
                    break;
                case METATILE_SecretBase_MudBall:
                    VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_MUD_BALL);
                    break;
                }
            }
        }
        else if (MetatileBehavior_IsSecretBaseBreakableDoor(behavior) == TRUE)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_BREAKABLE_DOOR);

            ShatterSecretBaseBreakableDoor(x, y);
        }
        else if (MetatileBehavior_IsSecretBaseSoundMat(behavior) == TRUE){
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_NOTE_MAT);
        }
        else if (MetatileBehavior_IsSecretBaseJumpMat(behavior) == TRUE)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_JUMP_MAT);
        }
        else if (MetatileBehavior_IsSecretBaseSpinMat(behavior) == TRUE)
        {
            if (sInFriendSecretBase == TRUE)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_SPIN_MAT);
        }
        break;
    case 2:
        // This state is never reached, and tFldEff is never set
        if (!FieldEffectActiveListContains(tFldEff))
            tState = 1;
        break;
    }
}

#undef tStepCb
#undef tState
#undef tPlayerX
#undef tPlayerY
#undef tFldEff

static void SaveSecretBase(u8 secretBaseIdx, struct SecretBase *secretBase, u32 version, u32 language)
{
    int stringLength;
    u8 *name;

    gSaveBlock1Ptr->secretBases[secretBaseIdx] = *secretBase;
    gSaveBlock1Ptr->secretBases[secretBaseIdx].registryStatus = NEW;
    if (version == VERSION_SAPPHIRE || version == VERSION_RUBY)
        gSaveBlock1Ptr->secretBases[secretBaseIdx].language = GAME_LANGUAGE;

    if (version == VERSION_EMERALD && language == LANGUAGE_JAPANESE)
    {
        name = gSaveBlock1Ptr->secretBases[secretBaseIdx].trainerName;
        for (stringLength = 0; stringLength < PLAYER_NAME_LENGTH; stringLength++)
        {
            if (name[stringLength] == EOS)
                break;
        }

        if (stringLength > 5)
            gSaveBlock1Ptr->secretBases[secretBaseIdx].language = GAME_LANGUAGE;
    }
}

static bool8 SecretBasesHaveSameTrainerId(struct SecretBase *secretBase1, struct SecretBase *secretBase2)
{
    u8 i;
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
    {
        if (secretBase1->trainerId[i] != secretBase2->trainerId[i])
            return FALSE;
    }

    return TRUE;
}

static bool8 SecretBasesHaveSameTrainerName(struct SecretBase *sbr1, struct SecretBase *sbr2)
{
    u8 i;
    for (i = 0; i < PLAYER_NAME_LENGTH && (sbr1->trainerName[i] != EOS || sbr2->trainerName[i] != EOS); i++)
    {
        if (sbr1->trainerName[i] != sbr2->trainerName[i])
            return FALSE;
    }

    return TRUE;
}

static bool8 SecretBasesBelongToSamePlayer(struct SecretBase *secretBase1, struct SecretBase *secretBase2)
{
    if (secretBase1->gender == secretBase2->gender
     && SecretBasesHaveSameTrainerId(secretBase1, secretBase2)
     && SecretBasesHaveSameTrainerName(secretBase1, secretBase2))
    {
        return TRUE;
    }

    return FALSE;
}

static s16 GetSecretBaseIndexFromId(u8 secretBaseId)
{
    s16 i;
    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->secretBases[i].secretBaseId == secretBaseId)
            return i;
    }

    return -1;
}

static u8 FindAvailableSecretBaseIndex(void)
{
    s16 i;
    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->secretBases[i].secretBaseId == 0)
            return i;
    }

    return 0;
}

static u8 FindUnregisteredSecretBaseIndex(void)
{
    s16 i;
    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->secretBases[i].registryStatus == UNREGISTERED && gSaveBlock1Ptr->secretBases[i].toRegister == FALSE)
            return i;
    }

    return 0;
}

static u8 TrySaveFriendsSecretBase(struct SecretBase *secretBase, u32 version, u32 language)
{
    s16 index;

    // Secret base has no location
    if (!secretBase->secretBaseId)
        return 0;

    index = GetSecretBaseIndexFromId(secretBase->secretBaseId);
    if (index != 0)
    {
        // An existing secret base is using this location
        if (index != -1)
        {
            if (gSaveBlock1Ptr->secretBases[index].toRegister == TRUE)
                return 0;

            if (gSaveBlock1Ptr->secretBases[index].registryStatus != NEW || secretBase->toRegister == TRUE)
            {
                // Overwrite unregistered base at this location
                SaveSecretBase(index, secretBase, version, language);
                return index;
            }
        }
        // No secret base is using this location, find a spot to save it
        else
        {
            index = FindAvailableSecretBaseIndex();
            if (index != 0)
            {
                // Save in empty space
                SaveSecretBase(index, secretBase, version, language);
                return index;
            }

            index = FindUnregisteredSecretBaseIndex();
            if (index != 0)
            {
                // Overwrite unregistered base
                SaveSecretBase(index, secretBase, version, language);
                return index;
            }
        }
    }

    // Unable to save. Either...
    // - This was the player's base
    // - A registered base exists at this location
    // - The secret base limit has been filled with registered bases
    return 0;
}

// Moves the registered secret bases to the beginning of the array, so that
// they won't be forgotten during record mixing.
static void SortSecretBasesByRegistryStatus(void)
{
    u8 i;
    u8 j;
    struct SecretBase *secretBases;

    secretBases = gSaveBlock1Ptr->secretBases;
    for (i = 1; i < SECRET_BASES_COUNT - 1; i++)
    {
        for (j = i + 1; j < SECRET_BASES_COUNT; j++)
        {
            if ((secretBases[i].registryStatus == UNREGISTERED && secretBases[j].registryStatus == REGISTERED)
             || (secretBases[i].registryStatus == NEW && secretBases[j].registryStatus != NEW))
            {
                struct SecretBase temp;
                SWAP(secretBases[i], secretBases[j], temp)
            }
        }
    }
}

// Used to save a record mixing friends' bases other than their own
// registryStatus is so registered bases can be attempted first
static void TrySaveFriendsSecretBases(struct SecretBaseRecordMixer *mixer, u8 registryStatus)
{
    u16 i;
    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (mixer->secretBases[i].registryStatus == registryStatus)
            TrySaveFriendsSecretBase(&mixer->secretBases[i], mixer->version, mixer->language);
    }
}

static bool8 SecretBaseBelongsToPlayer(struct SecretBase *secretBase)
{
    u8 i;

    if (secretBase->secretBaseId == 0)
        return FALSE;

    if (secretBase->secretBaseId && secretBase->gender != gSaveBlock2Ptr->playerGender)
        return FALSE;

    // Check if the player's trainer Id matches the secret base's id.
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
    {
        if (secretBase->trainerId[i] != gSaveBlock2Ptr->playerTrainerId[i])
            return FALSE;
    }

    for (i = 0; i < PLAYER_NAME_LENGTH && (secretBase->trainerName[i] != EOS || gSaveBlock2Ptr->playerName[i] != EOS); i++)
    {
        if (secretBase->trainerName[i] != gSaveBlock2Ptr->playerName[i])
            return FALSE;
    }

    return TRUE;
}

#define DELETED_BASE_A  (1 << 0)
#define DELETED_BASE_B  (1 << 1)
#define DELETED_BASE_C  (1 << 2)

static void DeleteFirstOldBaseFromPlayerInRecordMixingFriendsRecords(struct SecretBase *basesA, struct SecretBase *basesB, struct SecretBase *basesC)
{
    u8 i;
    u8 sbFlags = 0;

    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (!(sbFlags & DELETED_BASE_A))
        {
            if (SecretBaseBelongsToPlayer(&basesA[i]) == TRUE)
            {
                ClearSecretBase(&basesA[i]);
                sbFlags |= DELETED_BASE_A;
            }
        }

        if (!(sbFlags & DELETED_BASE_B))
        {
            if (SecretBaseBelongsToPlayer(&basesB[i]) == TRUE)
            {
                ClearSecretBase(&basesB[i]);
                sbFlags |= DELETED_BASE_B;
            }
        }

        if (!(sbFlags & DELETED_BASE_C))
        {
            if (SecretBaseBelongsToPlayer(&basesC[i]) == TRUE)
            {
                ClearSecretBase(&basesC[i]);
                sbFlags |= DELETED_BASE_C;
            }
        }

        if (sbFlags == (DELETED_BASE_A | DELETED_BASE_B | DELETED_BASE_C))
        {
            break;
        }
    }
}

#undef DELETED_BASE_A
#undef DELETED_BASE_B
#undef DELETED_BASE_C

// returns TRUE if secretBase was deleted, FALSE otherwise
static bool8 ClearDuplicateOwnedSecretBase(struct SecretBase *secretBase, struct SecretBase *secretBases, u8 idx)
{
    u8 i;

    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (secretBases[i].secretBaseId != 0)
        {
            if (SecretBasesBelongToSamePlayer(secretBase, &secretBases[i]) == TRUE)
            {
                if (idx == 0)
                {
                    ClearSecretBase(&secretBases[i]);
                    return FALSE;
                }

                if (secretBase->numSecretBasesReceived > secretBases[i].numSecretBasesReceived)
                {
                    ClearSecretBase(&secretBases[i]);
                    return FALSE;
                }

                secretBases[i].toRegister = secretBase->toRegister;
                ClearSecretBase(secretBase);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void ClearDuplicateOwnedSecretBases(struct SecretBase *playersBases, struct SecretBase *friendsBasesA, struct SecretBase *friendsBasesB, struct SecretBase *friendsBasesC)
{
    u8 i;

    for (i = 1; i < SECRET_BASES_COUNT; i++)
    {
        if (playersBases[i].secretBaseId)
        {
            if (playersBases[i].registryStatus == REGISTERED)
            {
                // Mark registered bases, so if they're deleted as a duplicate they
                // will be re-registered later
                playersBases[i].toRegister = TRUE;
            }
            if (!ClearDuplicateOwnedSecretBase(&playersBases[i], friendsBasesA, i))
            {
                if (!ClearDuplicateOwnedSecretBase(&playersBases[i], friendsBasesB, i))
                {
                    ClearDuplicateOwnedSecretBase(&playersBases[i], friendsBasesC, i);
                }
            }
        }
    }
    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (friendsBasesA[i].secretBaseId)
        {
            friendsBasesA[i].battledOwnerToday = 0;
            if (!ClearDuplicateOwnedSecretBase(&friendsBasesA[i], friendsBasesB, i))
            {
                ClearDuplicateOwnedSecretBase(&friendsBasesA[i], friendsBasesC, i);
            }
        }
    }
    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (friendsBasesB[i].secretBaseId)
        {
            friendsBasesB[i].battledOwnerToday = 0;
            ClearDuplicateOwnedSecretBase(&friendsBasesB[i], friendsBasesC, i);
        }
        if (friendsBasesC[i].secretBaseId)
        {
            friendsBasesC[i].battledOwnerToday = 0;
        }
    }
}

static void TrySaveRegisteredDuplicate(struct SecretBase *base, u32 version, u32 language)
{
    if (base->toRegister == TRUE)
    {
        TrySaveFriendsSecretBase(base, version, language);
        ClearSecretBase(base);
    }
}

static void TrySaveRegisteredDuplicates(struct SecretBaseRecordMixer *mixers)
{
    u16 i;

    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        TrySaveRegisteredDuplicate(&mixers[0].secretBases[i], mixers[0].version, mixers[0].language);
        TrySaveRegisteredDuplicate(&mixers[1].secretBases[i], mixers[1].version, mixers[1].language);
        TrySaveRegisteredDuplicate(&mixers[2].secretBases[i], mixers[2].version, mixers[2].language);
    }
}

static void SaveRecordMixBases(struct SecretBaseRecordMixer *mixers)
{
    DeleteFirstOldBaseFromPlayerInRecordMixingFriendsRecords(mixers[0].secretBases, mixers[1].secretBases, mixers[2].secretBases);
    ClearDuplicateOwnedSecretBases(gSaveBlock1Ptr->secretBases, mixers[0].secretBases, mixers[1].secretBases, mixers[2].secretBases);

    // First, save any registered secret bases that were deleted as duplicates
    TrySaveRegisteredDuplicates(mixers);

    // Then try to save the record mixing friends' own bases
    TrySaveFriendsSecretBase(mixers[0].secretBases, mixers[0].version, mixers[0].language);
    TrySaveFriendsSecretBase(mixers[1].secretBases, mixers[1].version, mixers[1].language);
    TrySaveFriendsSecretBase(mixers[2].secretBases, mixers[2].version, mixers[2].language);

    // Then try to save as many of their registered bases as possible
    TrySaveFriendsSecretBases(&mixers[0], REGISTERED);
    TrySaveFriendsSecretBases(&mixers[1], REGISTERED);
    TrySaveFriendsSecretBases(&mixers[2], REGISTERED);

    // Lastly save as many of their unregistered bases as possible
    TrySaveFriendsSecretBases(&mixers[0], UNREGISTERED);
    TrySaveFriendsSecretBases(&mixers[1], UNREGISTERED);
    TrySaveFriendsSecretBases(&mixers[2], UNREGISTERED);
}

#define INIT_SECRET_BASE_RECORD_MIXER(linkId1, linkId2, linkId3)        \
            mixers[0].secretBases = secretBases + linkId1 * recordSize; \
            mixers[0].version = gLinkPlayers[linkId1].version & 0xFF;   \
            mixers[0].language = gLinkPlayers[linkId1].language;        \
            mixers[1].secretBases = secretBases + linkId2 * recordSize; \
            mixers[1].version = gLinkPlayers[linkId2].version & 0xFF;   \
            mixers[1].language = gLinkPlayers[linkId2].language;        \
            mixers[2].secretBases = secretBases + linkId3 * recordSize; \
            mixers[2].version = gLinkPlayers[linkId3].version & 0xFF;   \
            mixers[2].language = gLinkPlayers[linkId3].language;

void ReceiveSecretBasesData(void *secretBases, size_t recordSize, u8 linkIdx)
{
    struct SecretBaseRecordMixer mixers[3];
    u16 i;

    if (FlagGet(FLAG_RECEIVED_SECRET_POWER))
    {
        switch (GetLinkPlayerCount())
        {
        case 2:
            memset(secretBases + 2 * recordSize, 0, recordSize);
            memset(secretBases + 3 * recordSize, 0, recordSize);
            break;
        case 3:
            memset(secretBases + 3 * recordSize, 0, recordSize);
            break;
        }

        switch (linkIdx)
        {
        case 0:
            INIT_SECRET_BASE_RECORD_MIXER(1, 2, 3)
            break;
        case 1:
            INIT_SECRET_BASE_RECORD_MIXER(2, 3, 0)
            break;
        case 2:
            INIT_SECRET_BASE_RECORD_MIXER(3, 0, 1)
            break;
        case 3:
            INIT_SECRET_BASE_RECORD_MIXER(0, 1, 2)
            break;
        }

        SaveRecordMixBases(mixers);

        for (i = 1; i < SECRET_BASES_COUNT; i++)
        {
            // In the process of deleting duplicate bases, if a base the player has registered is deleted it is
            // flagged with the temporary toRegister flag, so it can be re-registered after it has been newly saved
            if (gSaveBlock1Ptr->secretBases[i].toRegister == TRUE)
            {
                gSaveBlock1Ptr->secretBases[i].registryStatus = REGISTERED;
                gSaveBlock1Ptr->secretBases[i].toRegister = FALSE;
            }
        }

        SortSecretBasesByRegistryStatus();
        for (i = 1; i < SECRET_BASES_COUNT; i++)
        {
            // Unmark "new" bases, they've been saved now and are no longer important
            if (gSaveBlock1Ptr->secretBases[i].registryStatus == NEW)
                gSaveBlock1Ptr->secretBases[i].registryStatus = UNREGISTERED;
        }

        if (gSaveBlock1Ptr->secretBases[0].secretBaseId != 0
         && gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived != 0xFFFF)
        {
            gSaveBlock1Ptr->secretBases[0].numSecretBasesReceived++;
        }
    }
}

void ClearJapaneseSecretBases(struct SecretBase *bases)
{
    u32 i;
    for (i = 0; i < SECRET_BASES_COUNT; i++)
    {
        if (bases[i].language == LANGUAGE_JAPANESE)
            ClearSecretBase(&bases[i]);
    }
}

void InitSecretBaseVars(void)
{
    VarSet(VAR_SECRET_BASE_STEP_COUNTER, 0);
    VarSet(VAR_SECRET_BASE_LAST_ITEM_USED, 0);
    VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, 0);
    VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, 0);
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
        VarSet(VAR_SECRET_BASE_IS_NOT_LOCAL, TRUE);
    else
        VarSet(VAR_SECRET_BASE_IS_NOT_LOCAL, FALSE);

    sInFriendSecretBase = FALSE;
}

void CheckLeftFriendsSecretBase(void)
{
    if (VarGet(VAR_SECRET_BASE_IS_NOT_LOCAL) && sInFriendSecretBase == TRUE && !CurMapIsSecretBase())
    {
        VarSet(VAR_SECRET_BASE_IS_NOT_LOCAL, FALSE);
        sInFriendSecretBase = FALSE;
        TryPutSecretBaseSecretsOnAir();
        VarSet(VAR_SECRET_BASE_STEP_COUNTER, 0);
        VarSet(VAR_SECRET_BASE_LAST_ITEM_USED, 0);
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, 0);
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, 0);
        VarSet(VAR_SECRET_BASE_IS_NOT_LOCAL, FALSE);
    }
}

void CheckInteractedWithFriendsDollDecor(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_DOLL);
}

void CheckInteractedWithFriendsCushionDecor(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_CUSHION);
}

void DeclinedSecretBaseBattle(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
    {
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) & ~(SECRET_BASE_BATTLED_WON | SECRET_BASE_BATTLED_LOST | SECRET_BASE_DECLINED_BATTLE));
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) & ~(SECRET_BASE_BATTLED_DRAW));
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_DECLINED_BATTLE);
    }
}

void WonSecretBaseBattle(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
    {
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) & ~(SECRET_BASE_BATTLED_WON | SECRET_BASE_BATTLED_LOST | SECRET_BASE_DECLINED_BATTLE));
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) & ~(SECRET_BASE_BATTLED_DRAW));
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_BATTLED_WON);
    }
}

void LostSecretBaseBattle(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
    {
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) & ~(SECRET_BASE_BATTLED_WON | SECRET_BASE_BATTLED_LOST | SECRET_BASE_DECLINED_BATTLE));
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) & ~(SECRET_BASE_BATTLED_DRAW));
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_BATTLED_LOST);
    }
}

void DrewSecretBaseBattle(void)
{
    if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
    {
        VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) & ~(SECRET_BASE_BATTLED_WON | SECRET_BASE_BATTLED_LOST | SECRET_BASE_DECLINED_BATTLE));
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) & ~(SECRET_BASE_BATTLED_DRAW));
        VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_BATTLED_DRAW);
    }
}

void CheckInteractedWithFriendsPosterDecor(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    switch (MapGridGetMetatileIdAt(x, y))
    {
        case METATILE_SecretBase_PikaPoster_Left:
        case METATILE_SecretBase_PikaPoster_Right:
        case METATILE_SecretBase_LongPoster_Left:
        case METATILE_SecretBase_LongPoster_Right:
        case METATILE_SecretBase_SeaPoster_Left:
        case METATILE_SecretBase_SeaPoster_Right:
        case METATILE_SecretBase_SkyPoster_Left:
        case METATILE_SecretBase_SkyPoster_Right:
        case METATILE_SecretBase_KissPoster_Left:
        case METATILE_SecretBase_KissPoster_Right:
        case METATILE_SecretBase_BallPoster:
        case METATILE_SecretBase_GreenPoster:
        case METATILE_SecretBase_RedPoster:
        case METATILE_SecretBase_BluePoster:
        case METATILE_SecretBase_CutePoster:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_POSTER);
            break;
    }
}

void CheckInteractedWithFriendsFurnitureBottom(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    switch (MapGridGetMetatileIdAt(x, y))
    {
        case METATILE_SecretBase_GlassOrnament_Base1:
        case METATILE_SecretBase_GlassOrnament_Base2:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_GLASS_ORNAMENT);
            break;
        case METATILE_SecretBase_RedPlant_Base1:
        case METATILE_SecretBase_RedPlant_Base2:
        case METATILE_SecretBase_TropicalPlant_Base1:
        case METATILE_SecretBase_TropicalPlant_Base2:
        case METATILE_SecretBase_PrettyFlowers_Base1:
        case METATILE_SecretBase_PrettyFlowers_Base2:
        case METATILE_SecretBase_ColorfulPlant_BaseLeft1:
        case METATILE_SecretBase_ColorfulPlant_BaseRight1:
        case METATILE_SecretBase_ColorfulPlant_BaseLeft2:
        case METATILE_SecretBase_ColorfulPlant_BaseRight2:
        case METATILE_SecretBase_BigPlant_BaseLeft1:
        case METATILE_SecretBase_BigPlant_BaseRight1:
        case METATILE_SecretBase_BigPlant_BaseLeft2:
        case METATILE_SecretBase_BigPlant_BaseRight2:
        case METATILE_SecretBase_GorgeousPlant_BaseLeft1:
        case METATILE_SecretBase_GorgeousPlant_BaseRight1:
        case METATILE_SecretBase_GorgeousPlant_BaseLeft2:
        case METATILE_SecretBase_GorgeousPlant_BaseRight2:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_PLANT);
            break;
        case METATILE_SecretBase_Fence_Horizontal:
        case METATILE_SecretBase_Fence_Vertical:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_FENCE);
            break;
        case METATILE_SecretBase_Tire_BottomLeft:
        case METATILE_SecretBase_Tire_BottomRight:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_TIRE);
            break;
        case METATILE_SecretBase_RedBrick_Bottom:
        case METATILE_SecretBase_YellowBrick_Bottom:
        case METATILE_SecretBase_BlueBrick_Bottom:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_BRICK);
            break;
        case METATILE_SecretBase_SmallDesk:
        case METATILE_SecretBase_PokemonDesk:
        case METATILE_SecretBase_HeavyDesk_BottomLeft:
        case METATILE_SecretBase_HeavyDesk_BottomMid:
        case METATILE_SecretBase_HeavyDesk_BottomRight:
        case METATILE_SecretBase_RaggedDesk_BottomLeft:
        case METATILE_SecretBase_RaggedDesk_BottomMid:
        case METATILE_SecretBase_RaggedDesk_BottomRight:
        case METATILE_SecretBase_ComfortDesk_BottomLeft:
        case METATILE_SecretBase_ComfortDesk_BottomMid:
        case METATILE_SecretBase_ComfortDesk_BottomRight:
        case METATILE_SecretBase_BrickDesk_BottomLeft:
        case METATILE_SecretBase_BrickDesk_BottomMid:
        case METATILE_SecretBase_BrickDesk_BottomRight:
        case METATILE_SecretBase_CampDesk_BottomLeft:
        case METATILE_SecretBase_CampDesk_BottomMid:
        case METATILE_SecretBase_CampDesk_BottomRight:
        case METATILE_SecretBase_HardDesk_BottomLeft:
        case METATILE_SecretBase_HardDesk_BottomMid:
        case METATILE_SecretBase_HardDesk_BottomRight:
        case METATILE_SecretBase_PrettyDesk_BottomLeft:
        case METATILE_SecretBase_PrettyDesk_BottomMid:
        case METATILE_SecretBase_PrettyDesk_BottomRight:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_DESK);
            break;
    }
}

void CheckInteractedWithFriendsFurnitureMiddle(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    switch (MapGridGetMetatileIdAt(x, y))
    {
        case METATILE_SecretBase_HeavyDesk_TopMid:
        case METATILE_SecretBase_RaggedDesk_TopMid:
        case METATILE_SecretBase_ComfortDesk_TopMid:
        case METATILE_SecretBase_BrickDesk_TopMid:
        case METATILE_SecretBase_BrickDesk_Center:
        case METATILE_SecretBase_CampDesk_TopMid:
        case METATILE_SecretBase_CampDesk_Center:
        case METATILE_SecretBase_HardDesk_TopMid:
        case METATILE_SecretBase_HardDesk_Center:
        case METATILE_SecretBase_PrettyDesk_TopMid:
        case METATILE_SecretBase_PrettyDesk_Center:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_DESK);
            break;
    }
}

void CheckInteractedWithFriendsFurnitureTop(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    switch (MapGridGetMetatileIdAt(x, y))
    {
        case METATILE_SecretBase_HeavyDesk_TopLeft:
        case METATILE_SecretBase_HeavyDesk_TopRight:
        case METATILE_SecretBase_RaggedDesk_TopLeft:
        case METATILE_SecretBase_RaggedDesk_TopRight:
        case METATILE_SecretBase_ComfortDesk_TopLeft:
        case METATILE_SecretBase_ComfortDesk_TopRight:
        case METATILE_SecretBase_BrickDesk_TopLeft:
        case METATILE_SecretBase_BrickDesk_TopRight:
        case METATILE_SecretBase_BrickDesk_MidLeft:
        case METATILE_SecretBase_BrickDesk_MidRight:
        case METATILE_SecretBase_CampDesk_TopLeft:
        case METATILE_SecretBase_CampDesk_TopRight:
        case METATILE_SecretBase_CampDesk_MidLeft:
        case METATILE_SecretBase_CampDesk_MidRight:
        case METATILE_SecretBase_HardDesk_TopLeft:
        case METATILE_SecretBase_HardDesk_TopRight:
        case METATILE_SecretBase_HardDesk_MidLeft:
        case METATILE_SecretBase_HardDesk_MidRight:
        case METATILE_SecretBase_PrettyDesk_TopLeft:
        case METATILE_SecretBase_PrettyDesk_TopRight:
        case METATILE_SecretBase_PrettyDesk_MidLeft:
        case METATILE_SecretBase_PrettyDesk_MidRight:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_DESK);
            break;
        case METATILE_SecretBase_Tire_TopLeft:
        case METATILE_SecretBase_Tire_TopRight:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_TIRE);
            break;
        case METATILE_SecretBase_RedBrick_Top:
        case METATILE_SecretBase_YellowBrick_Top:
        case METATILE_SecretBase_BlueBrick_Top:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_BRICK);
            break;
    }
}

void CheckInteractedWithFriendsSandOrnament(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    switch ((int)MapGridGetMetatileIdAt(x, y))
    {
        case METATILE_SecretBase_SandOrnament_Base1:
        case METATILE_SecretBase_SandOrnament_Base2:
            if (VarGet(VAR_CURRENT_SECRET_BASE) != 0)
                VarSet(VAR_SECRET_BASE_HIGH_TV_FLAGS, VarGet(VAR_SECRET_BASE_HIGH_TV_FLAGS) | SECRET_BASE_USED_SAND_ORNAMENT);
            break;
    }
}
