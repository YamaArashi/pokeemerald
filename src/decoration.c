#include "global.h"
#include "malloc.h"
#include "decompress.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_camera.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item_icon.h"
#include "item_menu.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "player_pc.h"
#include "script.h"
#include "secret_base.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "tilesets.h"
#include "trader.h"
#include "tv.h"
#include "constants/decorations.h"
#include "constants/event_objects.h"
#include "constants/songs.h"
#include "constants/region_map_sections.h"
#include "constants/metatile_labels.h"

#define PLACE_DECORATION_SELECTOR_TAG 0xbe5
#define PLACE_DECORATION_PLAYER_TAG   0x008
#define NUM_DECORATION_FLAGS (FLAG_DECORATION_14 - FLAG_DECORATION_1 + 1)

#define tCursorX data[0]
#define tCursorY data[1]
#define tState data[2]
#define tInitialX data[3]
#define tInitialY data[4]
#define tDecorWidth data[5]
#define tDecorHeight data[6]
#define tButton data[10]
#define tDecorationMenuCommand data[11]
#define tDecorationItemsMenuCommand data[12]
#define tMenuTaskId data[13]

#define DECOR_MENU_PLACE 0
#define DECOR_MENU_TOSS 1
#define DECOR_MENU_TRADE 2

#define DECOR_ITEMS_MENU_PLACE 0
#define DECOR_ITEMS_MENU_PUT_AWAY 1

struct DecorationItemsMenu
{
    struct ListMenuItem items[41];
    u8 names[41][24];
    u8 numMenuItems;
    u8 maxShownItems;
    u8 scrollIndicatorsTaskId;
};

struct PlaceDecorationGraphicsDataBuffer
{
    const struct Decoration *decoration;
    u16 tiles[0x40];
    u8 image[0x800];
    u16 palette[16];
};

struct DecorRearrangementDataBuffer
{
    u8 idx;
    u8 width;
    u8 height;
    u16 flagId;
};

struct DecorationPCContext
{
    u8 *items;
    u8 *pos;
    u8 size;
    u8 isPlayerRoom;
};

enum Windows
{
    WINDOW_MAIN_MENU,
    WINDOW_DECORATION_CATEGORIES,
    WINDOW_DECORATION_CATEGORY_SUMMARY,
    WINDOW_DECORATION_CATEGORY_ITEMS,
    WINDOW_COUNT
};

EWRAM_DATA u8 *gCurDecorationItems = NULL;
EWRAM_DATA static u8 sDecorationActionsCursorPos = 0;
EWRAM_DATA static u8 sNumOwnedDecorationsInCurCategory = 0;
EWRAM_DATA static u8 sSecretBaseItemsIndicesBuffer[DECOR_MAX_SECRET_BASE] = {};
EWRAM_DATA static u8 sPlayerRoomItemsIndicesBuffer[DECOR_MAX_PLAYERS_HOUSE] = {};
EWRAM_DATA static u16 sDecorationsCursorPos = 0;
EWRAM_DATA static u16 sDecorationsScrollOffset = 0;
EWRAM_DATA u8 gCurDecorationIndex = 0;
EWRAM_DATA static u8 sCurDecorationCategory = DECORCAT_DESK;
EWRAM_DATA static u32 UNUSED sFiller[2] = {};
EWRAM_DATA static struct DecorationPCContext sDecorationContext = {};
EWRAM_DATA static u8 sDecorMenuWindowIds[WINDOW_COUNT] = {};
EWRAM_DATA static struct DecorationItemsMenu *sDecorationItemsMenu = NULL;
EWRAM_DATA static struct PlaceDecorationGraphicsDataBuffer sPlaceDecorationGraphicsDataBuffer = {};
EWRAM_DATA static u16 sCurDecorMapX = 0;
EWRAM_DATA static u16 sCurDecorMapY = 0;
EWRAM_DATA static u8 sDecor_CameraSpriteObjectIdx1 = 0;
EWRAM_DATA static u8 sDecor_CameraSpriteObjectIdx2 = 0;
EWRAM_DATA static u8 sDecorationLastDirectionMoved = 0;
EWRAM_DATA static struct OamData sDecorSelectorOam = {};
EWRAM_DATA static struct DecorRearrangementDataBuffer sDecorRearrangementDataBuffer[DECOR_MAX_SECRET_BASE] = {};
EWRAM_DATA static u8 sCurDecorSelectedInRearrangement = 0;

static void HandleDecorationActionsMenuInput(u8 taskId);
static void PrintCurMainMenuDescription(void);
static void DecorationMenuAction_Decorate(u8 taskId);
static void DecorationMenuAction_PutAway(u8 taskId);
static void DecorationMenuAction_Toss(u8 taskId);
static void DecorationMenuAction_Cancel(u8 taskId);
static void ReturnToDecorationActionsAfterInvalidSelection(u8 taskId);
static void SecretBasePC_PrepMenuForSelectingStoredDecors(u8 taskId);
static void InitDecorationCategoriesWindow(u8 taskId);
static void PrintDecorationCategoryMenuItems(u8 taskId);
static void PrintDecorationCategoryMenuItem(u8 winid, u8 category, u8 x, u8 y, bool8 disabled, u8 speed);
static void ColorMenuItemString(u8 *str, bool8 disabled);
static void HandleDecorationCategoriesMenuInput(u8 taskId);
static void SelectDecorationCategory(u8 taskId);
static void ReturnToDecorationCategoriesAfterInvalidSelection(u8 taskId);
static void ExitDecorationCategoriesMenu(u8 taskId);
static void ReturnToActionsMenuFromCategories(u8 taskId);
static void ExitTraderDecorationMenu(u8 taskId);
static void CopyDecorationMenuItemName(u8 *dest, u16 decoration);
static void DecorationItemsMenu_OnCursorMove(s32 itemIndex, bool8 flag, struct ListMenu *menu);
static void DecorationItemsMenu_PrintDecorationInUse(u8 windowId, u32 itemIndex, u8 y);
static void ShowDecorationItemsWindow(u8 taskId);
static void HandleDecorationItemsMenuInput(u8 taskId);
static void PrintDecorationItemDescription(s32 itemIndex);
static void RemoveDecorationItemsOtherWindows(void);
static bool8 IsDecorationIndexInSecretBase(u8 idx);
static bool8 IsDecorationIndexInPlayersRoom(u8 idx);
static void IdentifyOwnedDecorationsCurrentlyInUse(u8 taskId);
static void InitDecorationItemsWindow(u8 taskId);
static void ShowDecorationCategorySummaryWindow(u8 category);
static void DontTossDecoration(u8 taskId);
static void DecorationItemsMenuAction_Cancel(u8 taskId);
static void DecorationItemsMenuAction_AttemptPlace(u8 taskId);
static void Task_PlaceDecoration(u8 taskId);
static void ConfigureCameraObjectForPlacingDecoration(struct PlaceDecorationGraphicsDataBuffer *data, u8 decor);
static void SetUpPlacingDecorationPlayerAvatar(u8 taskId, struct PlaceDecorationGraphicsDataBuffer *data);
static void SetUpDecorationShape(u8 taskId);
static void AttemptPlaceDecoration(u8 taskId);
static void AttemptCancelPlaceDecoration(u8 taskId);
static void AttemptPlaceDecoration_(u8 taskId);
static void PlaceDecorationPrompt(u8 taskId);
static void PlaceDecoration(u8 taskId);
static void PlaceDecoration_(u8 taskId);
static void CancelDecoratingPrompt(u8 taskId);
static void CancelDecorating(u8 taskId);
static void CancelDecorating_(u8 taskId);
static void c1_overworld_prev_quest(u8 taskId);
static void FieldCB_InitDecorationItemsWindow(void);
static void ResetCursorMovement(void);
static void ContinueDecorating(u8 taskId);
static void CantPlaceDecorationPrompt(u8 taskId);
static void InitializePuttingAwayCursorSprite(struct Sprite *sprite);
static void InitializePuttingAwayCursorSprite2(struct Sprite *sprite);
static u8 gpu_pal_decompress_alloc_tag_and_upload(struct PlaceDecorationGraphicsDataBuffer *data, u8 decor);
static const u32 *GetDecorationIconPicOrPalette(u16 decor, u8 mode);
static bool8 HasDecorationsInUse(u8 taskId);
static void Task_ContinuePuttingAwayDecorations(u8 taskId);
static void ContinuePuttingAwayDecorations(u8 taskId);
static void AttemptPutAwayDecoration(u8 taskId);
static void AttemptCancelPutAwayDecoration(u8 taskId);
static void AttemptPutAwayDecoration_(u8 taskId);
static void ContinuePuttingAwayDecorationsPrompt(u8 taskId);
static void AttemptMarkDecorUnderCursorForRemoval(u8 taskId);
static void ReturnDecorationPrompt(u8 taskId);
static void PutAwayDecoration(u8 taskId);
static void StopPuttingAwayDecorationsPrompt(u8 taskId);
static void StopPuttingAwayDecorations(u8 taskId);
static void StopPuttingAwayDecorations_(u8 taskId);
static void Task_StopPuttingAwayDecorations(u8 taskId);
static void FieldCB_StopPuttingAwayDecorations(void);
static void InitializeCameraSprite1(struct Sprite *sprite);
static void LoadPlayerSpritePalette(void);
static void FreePlayerSpritePalette(void);
static void DecorationItemsMenuAction_AttemptToss(u8 taskId);
static void TossDecorationPrompt(u8 taskId);
static void TossDecoration(u8 taskId);

#include "data/decoration/tiles.h"
#include "data/decoration/description.h"
#include "data/decoration/header.h"

static const u8 *const sDecorationCategoryNames[] =
{
    gText_Desk,
    gText_Chair,
    gText_Plant,
    gText_Ornament,
    gText_Mat,
    gText_Poster,
    gText_Doll,
    gText_Cushion
};

static const struct MenuAction sDecorationMainMenuActions[] =
{
    {
        .text = gText_Decorate,
        .func = { .void_u8 = DecorationMenuAction_Decorate },
    },
    {
        .text = gText_PutAway,
        .func = { .void_u8 = DecorationMenuAction_PutAway },
    },
    {
        .text = gText_Toss2,
        .func = { .void_u8 = DecorationMenuAction_Toss },
    },
    {
        .text = gText_Cancel,
        .func = { .void_u8 = DecorationMenuAction_Cancel },
    },
};

static const u8 *const sSecretBasePCMenuItemDescriptions[] =
{
    gText_PutOutSelectedDecorItem,
    gText_StoreChosenDecorInPC,
    gText_ThrowAwayUnwantedDecors,
    gText_GoBackPrevMenu
};

static const TaskFunc sSecretBasePC_SelectedDecorationActions[][2] =
{
   { DecorationItemsMenuAction_AttemptPlace, DecorationItemsMenuAction_Cancel },
   { DecorationItemsMenuAction_AttemptToss,  DecorationItemsMenuAction_Cancel },
   { DecorationItemsMenuAction_Trade,        DecorationItemsMenuAction_Cancel },
};

static const struct WindowTemplate sDecorationWindowTemplates[WINDOW_COUNT] =
{
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 18,
        .height = 2 * ARRAY_COUNT(sDecorationMainMenuActions),
        .paletteNum = 15,
        .baseBlock = 0x0001
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 13,
        .height = 18,
        .paletteNum = 13,
        .baseBlock = 0x0091
    },
    {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 1,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x017b
    },
    {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 13,
        .width = 13,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 0x0193
    }
};

static const u16 sDecorationMenuPalette[] = INCBIN_U16("graphics/decorations/decoration_menu.gbapal");

static const struct ListMenuTemplate sDecorationItemsListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = DecorationItemsMenu_OnCursorMove,
    .itemPrintFunc = DecorationItemsMenu_PrintDecorationInUse,
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
    .lettersSpacing = FALSE,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_BLACK_ARROW,
};

#include "data/decoration/icon.h"
#include "data/decoration/tilemaps.h"

static const struct {
    u8 shape;
    u8 size;
    u8 cameraX;
    u8 cameraY;
} sDecorationMovementInfo[] =
{
    [DECORSHAPE_1x1] = {SPRITE_SHAPE(16x16), SPRITE_SIZE(16x16), 120, 78},
    [DECORSHAPE_2x1] = {SPRITE_SHAPE(32x16), SPRITE_SIZE(32x16), 128, 78},
    [DECORSHAPE_3x1] = {SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), 144, 86},
    [DECORSHAPE_4x2] = {SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), 144, 70},
    [DECORSHAPE_2x2] = {SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), 128, 70},
    [DECORSHAPE_1x2] = {SPRITE_SHAPE(16x32), SPRITE_SIZE(16x32), 120, 70},
    [DECORSHAPE_1x3] = {SPRITE_SHAPE(32x64), SPRITE_SIZE(32x64), 128, 86},
    [DECORSHAPE_2x4] = {SPRITE_SHAPE(32x64), SPRITE_SIZE(32x64), 128, 54},
    [DECORSHAPE_3x3] = {SPRITE_SHAPE(64x64), SPRITE_SIZE(64x64), 144, 70},
    [DECORSHAPE_3x2] = {SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), 144, 70},
};

static const union AnimCmd sDecorSelectorAnimCmd0[] =
{
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sDecorSelectorAnimCmds[] = { sDecorSelectorAnimCmd0 };

static const struct SpriteFrameImage sDecorSelectorSpriteFrameImages =
{
    .data = (const u8 *)&sPlaceDecorationGraphicsDataBuffer.image,
    .size = 0x800,
};

static const struct SpriteTemplate sDecorationSelectorSpriteTemplate =
{
    TAG_NONE,
    PLACE_DECORATION_SELECTOR_TAG,
    &sDecorSelectorOam,
    sDecorSelectorAnimCmds,
    &sDecorSelectorSpriteFrameImages,
    gDummySpriteAffineAnimTable,
    SpriteCallbackDummy
};

static const struct SpriteTemplate sDecorWhilePlacingSpriteTemplate =
{
    0x0000,
    0x0000,
    &sDecorSelectorOam,
    sDecorSelectorAnimCmds,
    NULL,
    gDummySpriteAffineAnimTable,
    SpriteCallbackDummy
};

static const struct SpritePalette sSpritePal_PlaceDecoration =
{
    .data = (const u16 *)&sPlaceDecorationGraphicsDataBuffer.palette,
    .tag = PLACE_DECORATION_SELECTOR_TAG,
};

static const struct YesNoFuncTable sPlaceDecorationYesNoFunctions =
{
    .yesFunc = PlaceDecoration,
    .noFunc = ContinueDecorating,
};

static const struct YesNoFuncTable sCancelDecoratingYesNoFunctions =
{
    .yesFunc = CancelDecorating,
    .noFunc = ContinueDecorating,
};

static const struct YesNoFuncTable sPlacePutAwayYesNoFunctions[] =
{
    {
        .yesFunc = AttemptPlaceDecoration,
        .noFunc = AttemptCancelPlaceDecoration,
    },
    {
        .yesFunc = AttemptPutAwayDecoration,
        .noFunc = AttemptCancelPutAwayDecoration,
    }
};

static const u8 sDecorationStandElevations[] =
{
    4, 4, 4, 4,
    0, 3, 3, 0
};

static const u8 sDecorationSlideElevation[] =
{
    4, 4,
    4, 4,
    0, 4,
    3, 0,
};

static const u16 sDecorShapeSizes[] = {
    [DECORSHAPE_1x1] = 4,
    [DECORSHAPE_2x1] = 8,
    [DECORSHAPE_3x1] = 16,
    [DECORSHAPE_4x2] = 32,
    [DECORSHAPE_2x2] = 16,
    [DECORSHAPE_1x2] = 8,
    [DECORSHAPE_1x3] = 16,
    [DECORSHAPE_2x4] = 32,
    [DECORSHAPE_3x3] = 64,
    [DECORSHAPE_3x2] = 32,
};

static const u16 sBrendanPalette[] = INCBIN_U16("graphics/decorations/brendan.gbapal");

static const u16 sMayPalette[] = INCBIN_U16("graphics/decorations/may.gbapal");

static const struct YesNoFuncTable sReturnDecorationYesNoFunctions =
{
    .yesFunc = PutAwayDecoration,
    .noFunc = ContinuePuttingAwayDecorations,
};

static const struct YesNoFuncTable sStopPuttingAwayDecorationsYesNoFunctions =
{
    .yesFunc = StopPuttingAwayDecorations,
    .noFunc = ContinuePuttingAwayDecorations,
};

static const u8 sDecorationPuttingAwayCursor[] = INCBIN_U8("graphics/decorations/put_away_cursor.4bpp");

static const struct SpritePalette sSpritePal_PuttingAwayCursorBrendan =
{
    .data = sBrendanPalette,
    .tag = PLACE_DECORATION_PLAYER_TAG,
};

static const struct SpritePalette sSpritePal_PuttingAwayCursorMay =
{
    .data = sMayPalette,
    .tag = PLACE_DECORATION_PLAYER_TAG,
};

static const struct OamData sPuttingAwayCursorOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sPuttingAwayCursorAnimCmd0[] =
{
    ANIMCMD_FRAME(0, 0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sPuttingAwayCursorAnimCmds[] =
{
    sPuttingAwayCursorAnimCmd0,
};

static const struct SpriteFrameImage sPuttingAwayCursorPicTable =
{
    .data = sDecorationPuttingAwayCursor,
    .size = sizeof(sDecorationPuttingAwayCursor),
};

static const struct SpriteTemplate sPuttingAwayCursorSpriteTemplate =
{
    TAG_NONE,
    PLACE_DECORATION_PLAYER_TAG,
    &sPuttingAwayCursorOamData,
    sPuttingAwayCursorAnimCmds,
    &sPuttingAwayCursorPicTable,
    gDummySpriteAffineAnimTable,
    InitializeCameraSprite1
};

static const struct YesNoFuncTable sTossDecorationYesNoFunctions =
{
    .yesFunc = TossDecoration,
    .noFunc = DontTossDecoration,
};

void InitDecorationContextItems(void)
{
    if (sCurDecorationCategory < DECORCAT_COUNT)
        gCurDecorationItems = gDecorationInventories[sCurDecorationCategory].items;

    if (sDecorationContext.isPlayerRoom == FALSE)
    {
        sDecorationContext.items = gSaveBlock1Ptr->secretBases[0].decorations;
        sDecorationContext.pos = gSaveBlock1Ptr->secretBases[0].decorationPositions;
    }

    if (sDecorationContext.isPlayerRoom == TRUE)
    {
        sDecorationContext.items = gSaveBlock1Ptr->playerRoomDecorations;
        sDecorationContext.pos = gSaveBlock1Ptr->playerRoomDecorationPositions;
    }
}

static u8 AddDecorationWindow(u8 windowIndex)
{
    u8 *windowId;
    struct WindowTemplate template;

    windowId = &sDecorMenuWindowIds[windowIndex];
    if (windowIndex == WINDOW_MAIN_MENU)
    {
        template = sDecorationWindowTemplates[WINDOW_MAIN_MENU];
        template.width = GetMaxWidthInMenuTable(sDecorationMainMenuActions, ARRAY_COUNT(sDecorationMainMenuActions));
        if (template.width > 18)
            template.width = 18;

        *windowId = AddWindow(&template);
    }
    else
    {
        *windowId = AddWindow(&sDecorationWindowTemplates[windowIndex]);
    }

    DrawStdFrameWithCustomTileAndPalette(*windowId, FALSE, 0x214, 14);
    ScheduleBgCopyTilemapToVram(0);
    return *windowId;
}

static void RemoveDecorationWindow(u8 windowIndex)
{
    ClearStdWindowAndFrameToTransparent(sDecorMenuWindowIds[windowIndex], FALSE);
    ClearWindowTilemap(sDecorMenuWindowIds[windowIndex]);
    RemoveWindow(sDecorMenuWindowIds[windowIndex]);
    ScheduleBgCopyTilemapToVram(0);
}

static void AddDecorationActionsWindow(void)
{
    u8 windowId = AddDecorationWindow(WINDOW_MAIN_MENU);
    PrintMenuTable(windowId, ARRAY_COUNT(sDecorationMainMenuActions), sDecorationMainMenuActions);
    InitMenuInUpperLeftCornerNormal(windowId, ARRAY_COUNT(sDecorationMainMenuActions), sDecorationActionsCursorPos);
}

static void InitDecorationActionsWindow(void)
{
    sDecorationActionsCursorPos = 0;
    LockPlayerFieldControls();
    AddDecorationActionsWindow();
    PrintCurMainMenuDescription();
}

void DoSecretBaseDecorationMenu(u8 taskId)
{
    InitDecorationActionsWindow();
    sDecorationContext.items = gSaveBlock1Ptr->secretBases[0].decorations;
    sDecorationContext.pos = gSaveBlock1Ptr->secretBases[0].decorationPositions;
    sDecorationContext.size = DECOR_MAX_SECRET_BASE;
    sDecorationContext.isPlayerRoom = FALSE;
    gTasks[taskId].func = HandleDecorationActionsMenuInput;
}

void DoPlayerRoomDecorationMenu(u8 taskId)
{
    InitDecorationActionsWindow();
    sDecorationContext.items = gSaveBlock1Ptr->playerRoomDecorations;
    sDecorationContext.pos = gSaveBlock1Ptr->playerRoomDecorationPositions;
    sDecorationContext.size = DECOR_MAX_PLAYERS_HOUSE;
    sDecorationContext.isPlayerRoom = TRUE;
    gTasks[taskId].func = HandleDecorationActionsMenuInput;
}

static void HandleDecorationActionsMenuInput(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        s8 menuPos = Menu_GetCursorPos();
        switch (Menu_ProcessInput())
        {
        default:
            PlaySE(SE_SELECT);
            sDecorationMainMenuActions[sDecorationActionsCursorPos].func.void_u8(taskId);
            break;
        case MENU_NOTHING_CHOSEN:
            sDecorationActionsCursorPos = Menu_GetCursorPos();
            if (menuPos != sDecorationActionsCursorPos)
                PrintCurMainMenuDescription();
            break;
        case MENU_B_PRESSED:
            PlaySE(SE_SELECT);
            DecorationMenuAction_Cancel(taskId);
            break;
        }
    }
}

static void PrintCurMainMenuDescription(void)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized2(0, FONT_NORMAL, sSecretBasePCMenuItemDescriptions[sDecorationActionsCursorPos], 0, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

static void DecorationMenuAction_Decorate(u8 taskId)
{
    if (GetNumOwnedDecorations() == 0)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_NoDecorations);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationActionsAfterInvalidSelection);
    }
    else
    {
        gTasks[taskId].tDecorationMenuCommand = DECOR_MENU_PLACE;
        sCurDecorationCategory = DECORCAT_DESK;
        SecretBasePC_PrepMenuForSelectingStoredDecors(taskId);
    }
}

static void DecorationMenuAction_PutAway(u8 taskId)
{
    if (!HasDecorationsInUse(taskId))
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_NoDecorationsInUse);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationActionsAfterInvalidSelection);
    }
    else
    {
        RemoveDecorationWindow(WINDOW_MAIN_MENU);
        ClearDialogWindowAndFrame(0, FALSE);
        FadeScreen(FADE_TO_BLACK, 0);
        gTasks[taskId].tState = 0;
        gTasks[taskId].func = Task_ContinuePuttingAwayDecorations;
    }
}

static void DecorationMenuAction_Toss(u8 taskId)
{
    if (GetNumOwnedDecorations() == 0)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_NoDecorations);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationActionsAfterInvalidSelection);
    }
    else
    {
        gTasks[taskId].tDecorationMenuCommand = DECOR_MENU_TOSS;
        sCurDecorationCategory = DECORCAT_DESK;
        SecretBasePC_PrepMenuForSelectingStoredDecors(taskId);
    }
}

static void DecorationMenuAction_Cancel(u8 taskId)
{
    RemoveDecorationWindow(WINDOW_MAIN_MENU);
    if (!sDecorationContext.isPlayerRoom)
    {
        ScriptContext_SetupScript(SecretBase_EventScript_PCCancel);
        DestroyTask(taskId);
    }
    else
    {
        ReshowPlayerPC(taskId);
    }
}

static void ReturnToDecorationActionsAfterInvalidSelection(u8 taskId)
{
    PrintCurMainMenuDescription();
    gTasks[taskId].func = HandleDecorationActionsMenuInput;
}

static void SecretBasePC_PrepMenuForSelectingStoredDecors(u8 taskId)
{
    LoadPalette(sDecorationMenuPalette, BG_PLTT_ID(13), sizeof(sDecorationMenuPalette));
    ClearDialogWindowAndFrame(0, FALSE);
    RemoveDecorationWindow(WINDOW_MAIN_MENU);
    InitDecorationCategoriesWindow(taskId);
}

static void InitDecorationCategoriesWindow(u8 taskId)
{
    u8 windowId = AddDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    PrintDecorationCategoryMenuItems(taskId);
    InitMenuInUpperLeftCornerNormal(windowId, DECORCAT_COUNT + 1, sCurDecorationCategory);
    gTasks[taskId].func = HandleDecorationCategoriesMenuInput;
}

static void ReinitDecorationCategoriesWindow(u8 taskId)
{
    FillWindowPixelBuffer(sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES], PIXEL_FILL(1));
    PrintDecorationCategoryMenuItems(taskId);
    InitMenuInUpperLeftCornerNormal(sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES], DECORCAT_COUNT + 1, sCurDecorationCategory);
    gTasks[taskId].func = HandleDecorationCategoriesMenuInput;
}

static void PrintDecorationCategoryMenuItems(u8 taskId)
{
    u8 i;
    s16 *data = gTasks[taskId].data;
    u8 windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES];
    bool8 isPlayerRoom = sDecorationContext.isPlayerRoom;
    bool8 shouldDisable = FALSE;
    if (isPlayerRoom == TRUE && tDecorationMenuCommand == DECOR_MENU_PLACE)
        shouldDisable = TRUE;

    for (i = 0; i < DECORCAT_COUNT; i++)
    {
        // Only DOLL and CUSHION decorations are enabled when decorating the player's room.
        if (shouldDisable == TRUE && i != DECORCAT_DOLL && i != DECORCAT_CUSHION)
            PrintDecorationCategoryMenuItem(windowId, i, 8, i * 16, TRUE, TEXT_SKIP_DRAW);
        else
            PrintDecorationCategoryMenuItem(windowId, i, 8, i * 16, FALSE, TEXT_SKIP_DRAW);
    }

    AddTextPrinterParameterized(windowId, FONT_NORMAL, gTasks[taskId].tDecorationMenuCommand == DECOR_MENU_TRADE ? gText_Exit : gText_Cancel, 8, i * 16 + 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintDecorationCategoryMenuItem(u8 winid, u8 category, u8 x, u8 y, bool8 disabled, u8 speed)
{
    u8 width;
    u8 *str;

    width = x == 8 ? 104 : 96;
    y++;
    ColorMenuItemString(gStringVarBuffer, disabled);
    str = StringLength(gStringVarBuffer) + gStringVarBuffer;
    StringCopy(str, sDecorationCategoryNames[category]);
    AddTextPrinterParameterized(winid, FONT_NORMAL, gStringVarBuffer, x, y, speed, NULL);
    str = ConvertIntToDecimalStringN(str, GetNumOwnedDecorationsInCategory(category), STR_CONV_MODE_RIGHT_ALIGN, 2);
    *(str++) = CHAR_SLASH;
    ConvertIntToDecimalStringN(str, gDecorationInventories[category].size, STR_CONV_MODE_RIGHT_ALIGN, 2);
    x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVarBuffer, width);
    AddTextPrinterParameterized(winid, FONT_NORMAL, gStringVarBuffer, x, y, speed, NULL);
}

static void ColorMenuItemString(u8 *str, bool8 disabled)
{
    StringCopy(str, gText_Color161Shadow161);
    if (disabled == TRUE)
    {
        str[2] = 4;
        str[5] = 5;
    }
    else
    {
        str[2] = 2;
        str[5] = 3;
    }
}

static void HandleDecorationCategoriesMenuInput(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        s8 input = Menu_ProcessInput();
        switch (input)
        {
        case MENU_B_PRESSED:
        case DECORCAT_COUNT: // CANCEL
            PlaySE(SE_SELECT);
            ExitDecorationCategoriesMenu(taskId);
            break;
        case MENU_NOTHING_CHOSEN:
            break;
        default:
            PlaySE(SE_SELECT);
            sCurDecorationCategory = input;
            SelectDecorationCategory(taskId);
            break;
        }
    }
}

static void SelectDecorationCategory(u8 taskId)
{
    sNumOwnedDecorationsInCurCategory = GetNumOwnedDecorationsInCategory(sCurDecorationCategory);
    if (sNumOwnedDecorationsInCurCategory != 0)
    {
        CondenseDecorationsInCategory(sCurDecorationCategory);
        gCurDecorationItems = gDecorationInventories[sCurDecorationCategory].items;
        IdentifyOwnedDecorationsCurrentlyInUse(taskId);
        sDecorationsScrollOffset = 0;
        sDecorationsCursorPos = 0;
        gTasks[taskId].func = ShowDecorationItemsWindow;
    }
    else
    {
        RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
        StringExpandPlaceholders(gStringVarBuffer, gText_NoDecorations);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationCategoriesAfterInvalidSelection);
    }
}

static void ReturnToDecorationCategoriesAfterInvalidSelection(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    InitDecorationCategoriesWindow(taskId);
}

static void ExitDecorationCategoriesMenu(u8 taskId)
{
    if (gTasks[taskId].tDecorationMenuCommand != DECOR_MENU_TRADE)
        ReturnToActionsMenuFromCategories(taskId);
    else
        ExitTraderDecorationMenu(taskId);
}

static void ReturnToActionsMenuFromCategories(u8 taskId)
{
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    AddDecorationActionsWindow();
    DrawDialogueFrame(0, FALSE);
    PrintCurMainMenuDescription();
    gTasks[taskId].func = HandleDecorationActionsMenuInput;
}

void ShowDecorationCategoriesWindow(u8 taskId)
{
    LoadPalette(sDecorationMenuPalette, BG_PLTT_ID(13), sizeof(sDecorationMenuPalette));
    ClearDialogWindowAndFrame(0, FALSE);
    gTasks[taskId].tDecorationMenuCommand = DECOR_MENU_TRADE;
    sCurDecorationCategory = DECORCAT_DESK;
    InitDecorationCategoriesWindow(taskId);
}

void CopyDecorationCategoryName(u8 *dest, u8 category)
{
    StringCopy(dest, sDecorationCategoryNames[category]);
}

static void ExitTraderDecorationMenu(u8 taskId)
{
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    ExitTraderMenu(taskId);
}

static void InitDecorationItemsMenuLimits(void)
{
    sDecorationItemsMenu->numMenuItems = sNumOwnedDecorationsInCurCategory + 1;
    if (sDecorationItemsMenu->numMenuItems > 8)
        sDecorationItemsMenu->maxShownItems = 8;
    else
        sDecorationItemsMenu->maxShownItems = sDecorationItemsMenu->numMenuItems;
}

static void InitDecorationItemsMenuScrollAndCursor(void)
{
    SetCursorWithinListBounds(&sDecorationsScrollOffset, &sDecorationsCursorPos, sDecorationItemsMenu->maxShownItems, sDecorationItemsMenu->numMenuItems);
}

static void InitDecorationItemsMenuScrollAndCursor2(void)
{
    SetCursorScrollWithinListBounds(&sDecorationsScrollOffset, &sDecorationsCursorPos, sDecorationItemsMenu->maxShownItems, sDecorationItemsMenu->numMenuItems, 8);
}

static void PrintDecorationItemMenuItems(u8 taskId)
{
    s16 *data;
    u16 i;

    data = gTasks[taskId].data;
    if ((sCurDecorationCategory < DECORCAT_DOLL || sCurDecorationCategory > DECORCAT_CUSHION) && sDecorationContext.isPlayerRoom == TRUE && tDecorationMenuCommand == DECOR_MENU_PLACE)
        ColorMenuItemString(gStringVar1, TRUE);
    else
        ColorMenuItemString(gStringVar1, FALSE);

    for (i = 0; i < sDecorationItemsMenu->numMenuItems - 1; i++)
    {
        CopyDecorationMenuItemName(sDecorationItemsMenu->names[i], gCurDecorationItems[i]);
        sDecorationItemsMenu->items[i].name = sDecorationItemsMenu->names[i];
        sDecorationItemsMenu->items[i].id = i;
    }

    StringCopy(sDecorationItemsMenu->names[i], gText_Cancel);
    sDecorationItemsMenu->items[i].name = sDecorationItemsMenu->names[i];
    sDecorationItemsMenu->items[i].id = LIST_CANCEL;
    gMultiuseListMenuTemplate = sDecorationItemsListMenuTemplate;
    gMultiuseListMenuTemplate.windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES];
    gMultiuseListMenuTemplate.totalItems = sDecorationItemsMenu->numMenuItems;
    gMultiuseListMenuTemplate.items = sDecorationItemsMenu->items;
    gMultiuseListMenuTemplate.maxShowed = sDecorationItemsMenu->maxShownItems;
}

static void CopyDecorationMenuItemName(u8 *dest, u16 decoration)
{
    StringCopy(dest, gStringVar1);
    StringAppend(dest, gDecorations[decoration].name);
}

static void DecorationItemsMenu_OnCursorMove(s32 itemIndex, bool8 flag, struct ListMenu *menu)
{
    if (flag != TRUE)
        PlaySE(SE_SELECT);

    PrintDecorationItemDescription(itemIndex);
}

static void DecorationItemsMenu_PrintDecorationInUse(u8 windowId, u32 itemIndex, u8 y)
{
    if (itemIndex != LIST_CANCEL)
    {
        if (IsDecorationIndexInSecretBase(itemIndex + 1) == TRUE)
            BlitMenuInfoIcon(windowId, MENU_INFO_ICON_BALL_RED, 92, y + 2);
        else if (IsDecorationIndexInPlayersRoom(itemIndex + 1) == TRUE)
            BlitMenuInfoIcon(windowId, MENU_INFO_ICON_BALL_BLUE, 92, y + 2);
    }
}

static void AddDecorationItemsScrollIndicators(void)
{
    if (sDecorationItemsMenu->scrollIndicatorsTaskId == TASK_NONE)
    {
        sDecorationItemsMenu->scrollIndicatorsTaskId = AddScrollIndicatorArrowPairParameterized(
            SCROLL_ARROW_UP,
            0x3c,
            0x0c,
            0x94,
            sDecorationItemsMenu->numMenuItems - sDecorationItemsMenu->maxShownItems,
            0x6e,
            0x6e,
            &sDecorationsScrollOffset);
    }
}

static void RemoveDecorationItemsScrollIndicators(void)
{
    if (sDecorationItemsMenu->scrollIndicatorsTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(sDecorationItemsMenu->scrollIndicatorsTaskId);
        sDecorationItemsMenu->scrollIndicatorsTaskId = TASK_NONE;
    }
}

static void AddDecorationItemsWindow(u8 taskId)
{
    AddDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    InitDecorationItemsWindow(taskId);
}

static void InitDecorationItemsWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    AddDecorationWindow(WINDOW_DECORATION_CATEGORY_ITEMS);
    ShowDecorationCategorySummaryWindow(sCurDecorationCategory);
    sDecorationItemsMenu = AllocZeroed(sizeof(*sDecorationItemsMenu));
    sDecorationItemsMenu->scrollIndicatorsTaskId = TASK_NONE;
    InitDecorationItemsMenuLimits();
    InitDecorationItemsMenuScrollAndCursor();
    InitDecorationItemsMenuScrollAndCursor2();
    PrintDecorationItemMenuItems(taskId);
    tMenuTaskId = ListMenuInit(&gMultiuseListMenuTemplate, sDecorationsScrollOffset, sDecorationsCursorPos);
    AddDecorationItemsScrollIndicators();
}

static void ShowDecorationItemsWindow(u8 taskId)
{
    InitDecorationItemsWindow(taskId);
    gTasks[taskId].func = HandleDecorationItemsMenuInput;
}

static void HandleDecorationItemsMenuInput(u8 taskId)
{
    s16 *data;
    s32 input;

    data = gTasks[taskId].data;
    if (!gPaletteFade.active)
    {
        input = ListMenu_ProcessInput(tMenuTaskId);
        ListMenuGetScrollAndRow(tMenuTaskId, &sDecorationsScrollOffset, &sDecorationsCursorPos);
        switch (input)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            sSecretBasePC_SelectedDecorationActions[tDecorationMenuCommand][1](taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            gCurDecorationIndex = input;
            RemoveDecorationItemsScrollIndicators();
            DestroyListMenuTask(tMenuTaskId, &sDecorationsScrollOffset, &sDecorationsCursorPos);
            RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
            RemoveDecorationItemsOtherWindows();
            Free(sDecorationItemsMenu);
            sSecretBasePC_SelectedDecorationActions[tDecorationMenuCommand][0](taskId);
            break;
        }
    }
}

static void ShowDecorationCategorySummaryWindow(u8 category)
{
    PrintDecorationCategoryMenuItem(AddDecorationWindow(WINDOW_DECORATION_CATEGORY_SUMMARY), category, 0, 0, 0, 0);
}

static void PrintDecorationItemDescription(s32 itemIndex)
{
    u8 windowId;
    const u8 *str;

    windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORY_ITEMS];
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    if ((u32)itemIndex >= sNumOwnedDecorationsInCurCategory)
        str = gText_GoBackPrevMenu;
    else
        str = gDecorations[gCurDecorationItems[itemIndex]].description;

    AddTextPrinterParameterized(windowId, FONT_NORMAL, str, 0, 1, 0, 0);
}

static void RemoveDecorationItemsOtherWindows(void)
{
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORY_ITEMS);
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORY_SUMMARY);
}

static bool8 IsDecorationIndexInSecretBase(u8 idx)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (sSecretBaseItemsIndicesBuffer[i] == idx)
            return TRUE;
    }

    return FALSE;
}

static bool8 IsDecorationIndexInPlayersRoom(u8 idx)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer); i++)
    {
        if (sPlayerRoomItemsIndicesBuffer[i] == idx)
            return TRUE;
    }

    return FALSE;
}

static void IdentifyOwnedDecorationsCurrentlyInUseInternal(u8 taskId)
{
    u16 i, j, k;
    u16 count;

    count = 0;
    memset(sSecretBaseItemsIndicesBuffer, 0, sizeof(sSecretBaseItemsIndicesBuffer));
    memset(sPlayerRoomItemsIndicesBuffer, 0, sizeof(sPlayerRoomItemsIndicesBuffer));

    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (gSaveBlock1Ptr->secretBases[0].decorations[i] != DECOR_NONE)
        {
            for (j = 0; j < gDecorationInventories[sCurDecorationCategory].size; j++)
            {
                if (gCurDecorationItems[j] == gSaveBlock1Ptr->secretBases[0].decorations[i])
                {
                    for (k = 0; k < count && sSecretBaseItemsIndicesBuffer[k] != j + 1; k++)
                        ;

                    if (k == count)
                    {
                        sSecretBaseItemsIndicesBuffer[count] = j + 1;
                        count++;
                        break;
                    }
                }
            }
        }
    }

    count = 0;
    for (i = 0; i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer); i++)
    {
        if (gSaveBlock1Ptr->playerRoomDecorations[i] != DECOR_NONE)
        {
            for (j = 0; j < gDecorationInventories[sCurDecorationCategory].size; j++)
            {
                if (gCurDecorationItems[j] == gSaveBlock1Ptr->playerRoomDecorations[i] && IsDecorationIndexInSecretBase(j + 1) != TRUE)
                {
                    for (k = 0; k < count && sPlayerRoomItemsIndicesBuffer[k] != j + 1; k++);
                    if (k == count)
                    {
                        sPlayerRoomItemsIndicesBuffer[count] = j + 1;
                        count++;
                        break;
                    }
                }
            }
        }
    }
}

static void IdentifyOwnedDecorationsCurrentlyInUse(u8 taskId)
{
    IdentifyOwnedDecorationsCurrentlyInUseInternal(taskId);
}

bool8 IsSelectedDecorInThePC(void)
{
    u16 i;
    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (sSecretBaseItemsIndicesBuffer[i] == sDecorationsScrollOffset + sDecorationsCursorPos + 1)
            return FALSE;

        if (i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer)
         && sPlayerRoomItemsIndicesBuffer[i] == sDecorationsScrollOffset + sDecorationsCursorPos + 1)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void Task_ShowDecorationItemsWindow(u8 taskId)
{
    AddDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    ShowDecorationItemsWindow(taskId);
}

static void DontTossDecoration(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    gTasks[taskId].func = Task_ShowDecorationItemsWindow;
}

static void ReturnToDecorationItemsAfterInvalidSelection(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        ClearDialogWindowAndFrame(0, FALSE);
        AddDecorationWindow(WINDOW_DECORATION_CATEGORIES);
        ShowDecorationItemsWindow(taskId);
    }
}

static void DecorationItemsMenuAction_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    RemoveDecorationItemsScrollIndicators();
    RemoveDecorationItemsOtherWindows();
    DestroyListMenuTask(tMenuTaskId, NULL, NULL);
    Free(sDecorationItemsMenu);
    ReinitDecorationCategoriesWindow(taskId);
}

static void SetInitialPositions(u8 taskId)
{
    gTasks[taskId].tInitialX = gSaveBlock1Ptr->pos.x;
    gTasks[taskId].tInitialY = gSaveBlock1Ptr->pos.y;
    PlayerGetDestCoords(&gTasks[taskId].tCursorX, &gTasks[taskId].tCursorY);
}

static void WarpToInitialPosition(u8 taskId)
{
    DrawWholeMapView();
    SetWarpDestination(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE, gTasks[taskId].tInitialX, gTasks[taskId].tInitialY);
    WarpIntoMap();
}

static u16 GetDecorationElevation(u8 decoration, u8 tileIndex)
{
    u16 elevation = -1;
    switch (decoration)
    {
    case DECOR_STAND:
        elevation = sDecorationStandElevations[tileIndex] << MAPGRID_ELEVATION_SHIFT;
        return elevation;
    case DECOR_SLIDE:
        elevation = sDecorationSlideElevation[tileIndex] << MAPGRID_ELEVATION_SHIFT;
        return elevation;
    default:
        return elevation;
    }
}

static void ShowDecorationOnMap_(u16 mapX, u16 mapY, u8 decWidth, u8 decHeight, u16 decoration)
{
    u16 i, j;
    s16 x, y;
    u16 attributes;
    u16 impassableFlag;
    u16 overlapsWall;
    u16 elevation;

    for (j = 0; j < decHeight; j++)
    {
        y = mapY - decHeight + 1 + j;
        for (i = 0; i < decWidth; i++)
        {
            x = mapX + i;
            attributes = GetMetatileAttributesById(NUM_TILES_IN_PRIMARY + gDecorations[decoration].tiles[j * decWidth + i]);
            if (MetatileBehavior_IsSecretBaseImpassable(attributes & METATILE_ATTR_BEHAVIOR_MASK) == TRUE
             || (gDecorations[decoration].permission != DECORPERM_PASS_FLOOR && (attributes >> METATILE_ATTR_LAYER_SHIFT) != METATILE_LAYER_TYPE_NORMAL))
                impassableFlag = MAPGRID_COLLISION_MASK;
            else
                impassableFlag = 0;

            // Choose the metatile that has the wall background instead of the floor if overlapping a wall.
            if (gDecorations[decoration].permission != DECORPERM_NA_WALL && MetatileBehavior_IsSecretBaseNorthWall(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
                overlapsWall = 1;
            else
                overlapsWall = 0;

            elevation = GetDecorationElevation(gDecorations[decoration].id, j * decWidth + i);
            if (elevation != 0xFFFF)
                MapGridSetMetatileEntryAt(x, y, (gDecorations[decoration].tiles[j * decWidth + i] + (NUM_TILES_IN_PRIMARY | overlapsWall)) | impassableFlag | elevation);
            else
                MapGridSetMetatileIdAt(x, y, (gDecorations[decoration].tiles[j * decWidth + i] + (NUM_TILES_IN_PRIMARY | overlapsWall)) | impassableFlag);
        }
    }
}

void ShowDecorationOnMap(u16 mapX, u16 mapY, u16 decoration)
{
    switch (gDecorations[decoration].shape)
    {
    case DECORSHAPE_1x1:
        ShowDecorationOnMap_(mapX, mapY, 1, 1, decoration);
        break;
    case DECORSHAPE_2x1:
        ShowDecorationOnMap_(mapX, mapY, 2, 1, decoration);
        break;
    case DECORSHAPE_3x1: // unused
        ShowDecorationOnMap_(mapX, mapY, 3, 1, decoration);
        break;
    case DECORSHAPE_4x2:
        ShowDecorationOnMap_(mapX, mapY, 4, 2, decoration);
        break;
    case DECORSHAPE_2x2:
        ShowDecorationOnMap_(mapX, mapY, 2, 2, decoration);
        break;
    case DECORSHAPE_1x2:
        ShowDecorationOnMap_(mapX, mapY, 1, 2, decoration);
        break;
    case DECORSHAPE_1x3: // unused
        ShowDecorationOnMap_(mapX, mapY, 1, 3, decoration);
        break;
    case DECORSHAPE_2x4:
        ShowDecorationOnMap_(mapX, mapY, 2, 4, decoration);
        break;
    case DECORSHAPE_3x3:
        ShowDecorationOnMap_(mapX, mapY, 3, 3, decoration);
        break;
    case DECORSHAPE_3x2:
        ShowDecorationOnMap_(mapX, mapY, 3, 2, decoration);
        break;
    }
}

void SetDecoration(void)
{
    u8 i;
    u8 j;

    for (i = 0; i < NUM_DECORATION_FLAGS; i++)
    {
        if (FlagGet(FLAG_DECORATION_1 + i) == TRUE)
        {
            FlagClear(FLAG_DECORATION_1 + i);
            for (j = 0; j < gMapHeader.events->objectEventCount; j++)
            {
                if (gMapHeader.events->objectEvents[j].flagId == FLAG_DECORATION_1 + i)
                    break;
            }

            VarSet(
                VAR_OBJ_GFX_ID_0 + (gMapHeader.events->objectEvents[j].graphicsId - OBJ_EVENT_GFX_VAR_0),
                sPlaceDecorationGraphicsDataBuffer.decoration->tiles[0]);

            gSpecialVar_0x8005 = gMapHeader.events->objectEvents[j].localId;
            gSpecialVar_0x8006 = sCurDecorMapX;
            gSpecialVar_0x8007 = sCurDecorMapY;
            TrySpawnObjectEvent(gSpecialVar_0x8005, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
            TryMoveObjectEventToMapCoords(gSpecialVar_0x8005, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, gSpecialVar_0x8006, gSpecialVar_0x8007);
            TryOverrideObjectEventTemplateCoords(gSpecialVar_0x8005, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
            break;
        }
    }
}

static bool8 HasDecorationSpace(void)
{
    u16 i;
    for (i = 0; i < sDecorationContext.size; i++)
    {
        if (sDecorationContext.items[i] == DECOR_NONE)
            return TRUE;
    }

    return FALSE;
}

static void DecorationItemsMenuAction_AttemptPlace(u8 taskId)
{
    if (sDecorationContext.isPlayerRoom == TRUE && sCurDecorationCategory != DECORCAT_DOLL && sCurDecorationCategory != DECORCAT_CUSHION)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_CantPlaceInRoom);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationItemsAfterInvalidSelection);
    }
    else if (IsSelectedDecorInThePC() == TRUE)
    {
        if (HasDecorationSpace() == TRUE)
        {
            FadeScreen(FADE_TO_BLACK, 0);
            gTasks[taskId].tState = 0;
            gTasks[taskId].func = Task_PlaceDecoration;
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, sDecorationContext.size, STR_CONV_MODE_RIGHT_ALIGN, 2);
            if (sDecorationContext.isPlayerRoom == FALSE)
            {
                StringExpandPlaceholders(gStringVarBuffer, gText_NoMoreDecorations);
            }
            else
            {
                StringExpandPlaceholders(gStringVarBuffer, gText_NoMoreDecorations2);
            }
            DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationItemsAfterInvalidSelection);
        }
    }
    else
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_InUseAlready);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationItemsAfterInvalidSelection);
    }
}

static void Task_PlaceDecoration(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
        case 0:
            if (!gPaletteFade.active)
            {
                SetInitialPositions(taskId);
                gTasks[taskId].tState = 1;
            }
            break;
        case 1:
            gPaletteFade.bufferTransferDisabled = TRUE;
            ConfigureCameraObjectForPlacingDecoration(&sPlaceDecorationGraphicsDataBuffer, gCurDecorationItems[gCurDecorationIndex]);
            SetUpDecorationShape(taskId);
            SetUpPlacingDecorationPlayerAvatar(taskId, &sPlaceDecorationGraphicsDataBuffer);
            FadeInFromBlack();
            gPaletteFade.bufferTransferDisabled = FALSE;
            gTasks[taskId].tState = 2;
            break;
        case 2:
            if (IsWeatherNotFadingIn() == TRUE)
            {
                gTasks[taskId].tDecorationItemsMenuCommand = DECOR_ITEMS_MENU_PLACE;
                ContinueDecorating(taskId);
            }
            break;
    }
}

static void ConfigureCameraObjectForPlacingDecoration(struct PlaceDecorationGraphicsDataBuffer *data, u8 decor)
{
    sDecor_CameraSpriteObjectIdx1 = gSprites[gFieldCamera.spriteId].data[0];
    gFieldCamera.spriteId = gpu_pal_decompress_alloc_tag_and_upload(data, decor);
    gSprites[gFieldCamera.spriteId].oam.priority = 1;
    gSprites[gFieldCamera.spriteId].callback = InitializePuttingAwayCursorSprite;
    gSprites[gFieldCamera.spriteId].x = sDecorationMovementInfo[data->decoration->shape].cameraX;
    gSprites[gFieldCamera.spriteId].y = sDecorationMovementInfo[data->decoration->shape].cameraY;
}

static void SetUpPlacingDecorationPlayerAvatar(u8 taskId, struct PlaceDecorationGraphicsDataBuffer *data)
{
    u8 x;

    x = 16 * (u8)gTasks[taskId].tDecorWidth + sDecorationMovementInfo[data->decoration->shape].cameraX - 8 * ((u8)gTasks[taskId].tDecorWidth - 1);
    if (data->decoration->shape == DECORSHAPE_3x1 || data->decoration->shape == DECORSHAPE_3x3 || data->decoration->shape == DECORSHAPE_3x2)
        x -= 8;

    if (gSaveBlock2Ptr->playerGender == MALE)
        sDecor_CameraSpriteObjectIdx2 = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_BRENDAN_DECORATING, SpriteCallbackDummy, x, 72, 0);
    else
        sDecor_CameraSpriteObjectIdx2 = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_MAY_DECORATING, SpriteCallbackDummy, x, 72, 0);

    gSprites[sDecor_CameraSpriteObjectIdx2].oam.priority = 1;
    DestroySprite(&gSprites[sDecor_CameraSpriteObjectIdx1]);
    sDecor_CameraSpriteObjectIdx1 = gFieldCamera.spriteId;
}

static void SetUpDecorationShape(u8 taskId)
{
    switch (gDecorations[gCurDecorationItems[gCurDecorationIndex]].shape)
    {
        case DECORSHAPE_1x1:
            gTasks[taskId].tDecorWidth = 1;
            gTasks[taskId].tDecorHeight = 1;
            break;
        case DECORSHAPE_2x1:
            gTasks[taskId].tDecorWidth = 2;
            gTasks[taskId].tDecorHeight = 1;
            break;
        case DECORSHAPE_3x1:
            gTasks[taskId].tDecorWidth = 3;
            gTasks[taskId].tDecorHeight = 1;
            break;
        case DECORSHAPE_4x2:
            gTasks[taskId].tDecorWidth = 4;
            gTasks[taskId].tDecorHeight = 2;
            break;
        case DECORSHAPE_2x2:
            gTasks[taskId].tDecorWidth = 2;
            gTasks[taskId].tDecorHeight = 2;
            break;
        case DECORSHAPE_1x2:
            gTasks[taskId].tDecorWidth = 1;
            gTasks[taskId].tDecorHeight = 2;
            break;
        case DECORSHAPE_1x3:
            gTasks[taskId].tDecorWidth = 1;
            gTasks[taskId].tDecorHeight = 3;
            gTasks[taskId].tCursorY++;
            break;
        case DECORSHAPE_2x4:
            gTasks[taskId].tDecorWidth = 2;
            gTasks[taskId].tDecorHeight = 4;
            break;
        case DECORSHAPE_3x3:
            gTasks[taskId].tDecorWidth = 3;
            gTasks[taskId].tDecorHeight = 3;
            break;
        case DECORSHAPE_3x2:
            gTasks[taskId].tDecorWidth = 3;
            gTasks[taskId].tDecorHeight = 2;
            break;
    }
}

static void AttemptPlaceDecoration(u8 taskId)
{
    gTasks[taskId].tButton = 0;
    gSprites[sDecor_CameraSpriteObjectIdx1].data[7] = 1;
    gSprites[sDecor_CameraSpriteObjectIdx2].data[7] = 1;
    ResetCursorMovement();
    AttemptPlaceDecoration_(taskId);
}

static void AttemptCancelPlaceDecoration(u8 taskId)
{
    gTasks[taskId].tButton = 0;
    gSprites[sDecor_CameraSpriteObjectIdx1].data[7] = 1;
    gSprites[sDecor_CameraSpriteObjectIdx2].data[7] = 1;
    ResetCursorMovement();
    StringExpandPlaceholders(gStringVarBuffer, gText_CancelDecorating);
    DisplayItemMessageOnField(taskId, gStringVarBuffer, CancelDecoratingPrompt);
}

static bool8 IsSecretBaseTrainerSpot(u8 behaviorAt, u16 layerType)
{
    if (!(MetatileBehavior_IsSecretBaseTrainerSpot(behaviorAt) == TRUE && layerType == METATILE_LAYER_TYPE_NORMAL))
        return FALSE;
    return TRUE;
}

// Can't place decoration where the player was standing when they interacted with the PC
static bool8 IsntInitialPosition(u8 taskId, s16 x, s16 y, u16 layerType)
{
    if (x == gTasks[taskId].tInitialX + MAP_OFFSET
     && y == gTasks[taskId].tInitialY + MAP_OFFSET
     && layerType != METATILE_LAYER_TYPE_NORMAL)
        return FALSE;
    return TRUE;
}

static bool8 IsFloorOrBoardAndHole(u16 behaviorAt, const struct Decoration *decoration)
{
    if (MetatileBehavior_IsSecretBaseTrainerSpot(behaviorAt) != TRUE)
    {
        if (decoration->id == DECOR_SOLID_BOARD && MetatileBehavior_IsSecretBaseHole(behaviorAt) == TRUE)
            return TRUE;

        if (MetatileBehavior_IsNormal(behaviorAt))
            return TRUE;
    }

    return FALSE;
}

static bool8 CanPlaceDecoration(u8 taskId, const struct Decoration *decoration)
{
    u8 i;
    u8 j;
    u8 behaviorAt;
    u16 layerType;
    u8 mapY;
    u8 mapX;
    s16 curY;
    s16 curX;
    mapY = gTasks[taskId].tDecorHeight;
    mapX = gTasks[taskId].tDecorWidth;

    switch (decoration->permission)
    {
    case DECORPERM_SOLID_FLOOR:
    case DECORPERM_PASS_FLOOR:
        for (i = 0; i < mapY; i++)
        {
            curY = gTasks[taskId].tCursorY - i;
            for (j = 0; j < mapX; j++)
            {
                curX = gTasks[taskId].tCursorX + j;
                behaviorAt = MapGridGetMetatileBehaviorAt(curX, curY);
                layerType = GetMetatileAttributesById(NUM_TILES_IN_PRIMARY + decoration->tiles[(mapY - 1 - i) * mapX + j]) & METATILE_ATTR_LAYER_MASK;
                if (!IsFloorOrBoardAndHole(behaviorAt, decoration))
                    return FALSE;

                if (!IsntInitialPosition(taskId, curX, curY, layerType))
                    return FALSE;

                behaviorAt = GetObjectEventIdByPosition(curX, curY, 0);
                if (behaviorAt != 0 && behaviorAt != OBJECT_EVENTS_COUNT)
                    return FALSE;
            }
        }
        break;
    case DECORPERM_BEHIND_FLOOR:
        for (i = 0; i < mapY - 1; i++)
        {
            curY = gTasks[taskId].tCursorY - i;
            for (j = 0; j < mapX; j++)
            {
                curX = gTasks[taskId].tCursorX + j;
                behaviorAt = MapGridGetMetatileBehaviorAt(curX, curY);
                layerType = GetMetatileAttributesById(NUM_TILES_IN_PRIMARY + decoration->tiles[(mapY - 1 - i) * mapX + j]) & METATILE_ATTR_LAYER_MASK;
                if (!MetatileBehavior_IsNormal(behaviorAt) && !IsSecretBaseTrainerSpot(behaviorAt, layerType))
                    return FALSE;

                if (!IsntInitialPosition(taskId, curX, curY, layerType))
                    return FALSE;

                if (GetObjectEventIdByPosition(curX, curY, 0) != OBJECT_EVENTS_COUNT)
                    return FALSE;
            }
        }

        curY = gTasks[taskId].tCursorY - mapY + 1;
        for (j = 0; j < mapX; j++)
        {
            curX = gTasks[taskId].tCursorX + j;
            behaviorAt = MapGridGetMetatileBehaviorAt(curX, curY);
            layerType = GetMetatileAttributesById(NUM_TILES_IN_PRIMARY + decoration->tiles[j]) & METATILE_ATTR_LAYER_MASK;
            if (!MetatileBehavior_IsNormal(behaviorAt) && !MetatileBehavior_IsSecretBaseNorthWall(behaviorAt))
                return FALSE;

            if (!IsntInitialPosition(taskId, curX, curY, layerType))
                return FALSE;

            behaviorAt = GetObjectEventIdByPosition(curX, curY, 0);
            if (behaviorAt != 0 && behaviorAt != OBJECT_EVENTS_COUNT)
                return FALSE;
        }
        break;
    case DECORPERM_NA_WALL:
        for (i = 0; i < mapY; i++)
        {
            curY = gTasks[taskId].tCursorY - i;
            for (j = 0; j < mapX; j++)
            {
                curX = gTasks[taskId].tCursorX + j;
                if (!MetatileBehavior_IsSecretBaseNorthWall(MapGridGetMetatileBehaviorAt(curX, curY)))
                    return FALSE;

                if (MapGridGetMetatileIdAt(curX, curY + 1) == METATILE_SecretBase_SandOrnament_BrokenBase)
                    return FALSE;
            }
        }
        break;
    case DECORPERM_SPRITE:
        curY = gTasks[taskId].tCursorY;
        for (j = 0; j < mapX; j++)
        {
            curX = gTasks[taskId].tCursorX + j;
            behaviorAt = MapGridGetMetatileBehaviorAt(curX, curY);
            if (decoration->shape == DECORSHAPE_1x2)
            {
                if (!MetatileBehavior_HoldsLargeDecoration(behaviorAt))
                    return FALSE;
            }
            else if (!MetatileBehavior_HoldsSmallDecoration(behaviorAt))
            {
                if (!MetatileBehavior_HoldsLargeDecoration(behaviorAt))
                    return FALSE;
            }

            if (GetObjectEventIdByPosition(curX, curY, 0) != OBJECT_EVENTS_COUNT)
                return FALSE;
        }
        break;
    }
    return TRUE;
}

static void AttemptPlaceDecoration_(u8 taskId)
{
    if (CanPlaceDecoration(taskId, &gDecorations[gCurDecorationItems[gCurDecorationIndex]]) == TRUE)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_PlaceItHere);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, PlaceDecorationPrompt);
    }
    else
    {
        PlaySE(SE_FAILURE);
        StringExpandPlaceholders(gStringVarBuffer, gText_CantBePlacedHere);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, CantPlaceDecorationPrompt);
    }
}

static void PlaceDecorationPrompt(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sPlaceDecorationYesNoFunctions);
}

static void PlaceDecoration(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    PlaceDecoration_(taskId);
    if (gDecorations[gCurDecorationItems[gCurDecorationIndex]].permission != DECORPERM_SPRITE)
    {
        ShowDecorationOnMap(gTasks[taskId].tCursorX, gTasks[taskId].tCursorY, gCurDecorationItems[gCurDecorationIndex]);
    }
    else
    {
        sCurDecorMapX = gTasks[taskId].tCursorX - MAP_OFFSET;
        sCurDecorMapY = gTasks[taskId].tCursorY - MAP_OFFSET;
        ScriptContext_SetupScript(SecretBase_EventScript_SetDecoration);
    }

    gSprites[sDecor_CameraSpriteObjectIdx1].y += 2;
    if (gMapHeader.regionMapSectionId == MAPSEC_SECRET_BASE)
        TryPutSecretBaseVisitOnAir();

    CancelDecorating_(taskId);
}

static void PlaceDecoration_(u8 taskId)
{
    u16 i;

    for (i = 0; i < sDecorationContext.size; i++)
    {
        if (sDecorationContext.items[i] == DECOR_NONE)
        {
            sDecorationContext.items[i] = gCurDecorationItems[gCurDecorationIndex];
            sDecorationContext.pos[i] = ((gTasks[taskId].tCursorX - MAP_OFFSET) << 4) + (gTasks[taskId].tCursorY - MAP_OFFSET);
            break;
        }
    }

    if (!sDecorationContext.isPlayerRoom)
    {
        for (i = 0; i < DECOR_MAX_SECRET_BASE; i++)
        {
            if (sSecretBaseItemsIndicesBuffer[i] == DECOR_NONE)
            {
                sSecretBaseItemsIndicesBuffer[i] = gCurDecorationIndex + 1;
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < DECOR_MAX_PLAYERS_HOUSE; i++)
        {
            if (sPlayerRoomItemsIndicesBuffer[i] == DECOR_NONE)
            {
                sPlayerRoomItemsIndicesBuffer[i] = gCurDecorationIndex + 1;
                break;
            }
        }
    }
}

static void CancelDecoratingPrompt(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sCancelDecoratingYesNoFunctions);
}

static void CancelDecorating(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    CancelDecorating_(taskId);
}

static void CancelDecorating_(u8 taskId)
{
    FadeScreen(FADE_TO_BLACK, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].func = c1_overworld_prev_quest;
}

static void c1_overworld_prev_quest(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        LockPlayerFieldControls();
        if (!gPaletteFade.active)
        {
            WarpToInitialPosition(taskId);
            gTasks[taskId].tState = 1;
        }
        break;
    case 1:
        FreePlayerSpritePalette();
        FreeSpritePaletteByTag(PLACE_DECORATION_SELECTOR_TAG);
        gFieldCallback = FieldCB_InitDecorationItemsWindow;
        SetMainCallback2(CB2_ReturnToField);
        DestroyTask(taskId);
        break;
    }
}

static void Task_InitDecorationItemsWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        HideSecretBaseDecorationSprites();
        tState++;
        break;
    case 1:
        ScriptContext_SetupScript(SecretBase_EventScript_InitDecorations);
        tState++;
        break;
    case 2:
        LockPlayerFieldControls();
        tState++;
        break;
    case 3:
        if (IsWeatherNotFadingIn() == TRUE)
            gTasks[taskId].func = HandleDecorationItemsMenuInput;
        break;
    }
}

static void FieldCB_InitDecorationItemsWindow(void)
{
    u8 taskId;

    LockPlayerFieldControls();
    FadeInFromBlack();
    taskId = CreateTask(Task_InitDecorationItemsWindow, 8);
    AddDecorationItemsWindow(taskId);
    gTasks[taskId].tState = 0;
}

static bool8 ApplyCursorMovement_IsInvalid(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (sDecorationLastDirectionMoved == DIR_SOUTH && tCursorY - tDecorHeight - 6 < 0)
    {
        tCursorY++;
        return FALSE;
    }

    if (sDecorationLastDirectionMoved == DIR_NORTH && tCursorY - 7 >= gMapHeader.mapLayout->height)
    {
        tCursorY--;
        return FALSE;
    }

    if (sDecorationLastDirectionMoved == DIR_WEST && tCursorX - 7 < 0)
    {
        tCursorX++;
        return FALSE;
    }

    if (sDecorationLastDirectionMoved == DIR_EAST && tCursorX + tDecorWidth - 8 >= gMapHeader.mapLayout->width)
    {
        tCursorX--;
        return FALSE;
    }

    return TRUE;
}

static bool8 IsHoldingDirection(void)
{
    u16 heldKeys = JOY_HELD(DPAD_ANY);
    if (heldKeys != DPAD_UP && heldKeys != DPAD_DOWN && heldKeys != DPAD_LEFT && heldKeys != DPAD_RIGHT)
        return FALSE;

    return TRUE;
}

static void ResetCursorMovement(void)
{
    sDecorationLastDirectionMoved = 0;
    gSprites[sDecor_CameraSpriteObjectIdx1].data[2] = 0;
    gSprites[sDecor_CameraSpriteObjectIdx1].data[3] = 0;
}

static void Task_SelectLocation(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!gSprites[sDecor_CameraSpriteObjectIdx1].data[4])
    {
        if (tButton == A_BUTTON)
        {
            sPlacePutAwayYesNoFunctions[tDecorationItemsMenuCommand].yesFunc(taskId);
            return;
        }

        if (tButton == B_BUTTON)
        {
            sPlacePutAwayYesNoFunctions[tDecorationItemsMenuCommand].noFunc(taskId);
            return;
        }

        if ((JOY_HELD(DPAD_ANY)) == DPAD_UP)
        {
            sDecorationLastDirectionMoved = DIR_SOUTH;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[2] =  0;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[3] = -2;
            tCursorY--;
        }

        if ((JOY_HELD(DPAD_ANY)) == DPAD_DOWN)
        {
            sDecorationLastDirectionMoved = DIR_NORTH;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[2] =  0;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[3] =  2;
            tCursorY++;
        }

        if ((JOY_HELD(DPAD_ANY)) == DPAD_LEFT)
        {
            sDecorationLastDirectionMoved = DIR_WEST;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[2] = -2;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[3] =  0;
            tCursorX--;
        }

        if ((JOY_HELD(DPAD_ANY)) == DPAD_RIGHT)
        {
            sDecorationLastDirectionMoved = DIR_EAST;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[2] =  2;
            gSprites[sDecor_CameraSpriteObjectIdx1].data[3] =  0;
            tCursorX++;
        }

        if (!IsHoldingDirection() || !ApplyCursorMovement_IsInvalid(taskId))
            ResetCursorMovement();
    }

    if (sDecorationLastDirectionMoved)
    {
        gSprites[sDecor_CameraSpriteObjectIdx1].data[4]++;
        gSprites[sDecor_CameraSpriteObjectIdx1].data[4] &= 7;
    }

    if (!tButton)
    {
        if (JOY_NEW(A_BUTTON))
            tButton = A_BUTTON;

        if (JOY_NEW(B_BUTTON))
            tButton = B_BUTTON;
    }
}

static void ContinueDecorating(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    gSprites[sDecor_CameraSpriteObjectIdx1].data[7] = 0;
    gTasks[taskId].tButton = 0;
    gTasks[taskId].func = Task_SelectLocation;
}

static void CantPlaceDecorationPrompt(u8 taskId)
{
    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
        ContinueDecorating(taskId);
}

static void ClearPlaceDecorationGraphicsDataBuffer(struct PlaceDecorationGraphicsDataBuffer *data)
{
    CpuFill16(0, data, sizeof(*data));
}

static void CopyPalette(u16 *dest, u16 pal)
{
    CpuFastCopy(&((u16 *)gTilesetPointer_SecretBase->palettes)[pal * 16], dest, sizeof(u16) * 16);
}

static void CopyTile(u8 *dest, u16 tile)
{
    u8 ALIGNED(4) buffer[TILE_SIZE_4BPP];
    u16 mode;
    u16 i;

    mode = tile >> 10;
    if (tile != 0)
        tile &= 0x03FF;

    CpuFastCopy(&((u8 *)gTilesetPointer_SecretBase->tiles)[tile * TILE_SIZE_4BPP], buffer, TILE_SIZE_4BPP);
    switch (mode)
    {
    case 0:
        CpuFastCopy(buffer, dest, TILE_SIZE_4BPP);
        break;
    case BG_TILE_H_FLIP(0) >> 10:
        for (i = 0; i < 8; i++)
        {
            dest[4 * i + 0] = (buffer[4 * (i + 1) - 1] >> 4) + ((buffer[4 * (i + 1) - 1] & 0x0F) << 4);
            dest[4 * i + 1] = (buffer[4 * (i + 1) - 2] >> 4) + ((buffer[4 * (i + 1) - 2] & 0x0F) << 4);
            dest[4 * i + 2] = (buffer[4 * (i + 1) - 3] >> 4) + ((buffer[4 * (i + 1) - 3] & 0x0F) << 4);
            dest[4 * i + 3] = (buffer[4 * (i + 1) - 4] >> 4) + ((buffer[4 * (i + 1) - 4] & 0x0F) << 4);
        }
        break;
    case BG_TILE_V_FLIP(0) >> 10:
        for (i = 0; i < 8; i++)
        {
            dest[4 * i + 0] = buffer[4 * (7 - i) + 0];
            dest[4 * i + 1] = buffer[4 * (7 - i) + 1];
            dest[4 * i + 2] = buffer[4 * (7 - i) + 2];
            dest[4 * i + 3] = buffer[4 * (7 - i) + 3];
        }
        break;
    case BG_TILE_H_FLIP(BG_TILE_V_FLIP(0)) >> 10:
        for (i = 0; i < 32; i++)
        {
            dest[i] = (buffer[31 - i] >> 4) + ((buffer[31 - i] & 0x0F) << 4);
        }
        break;
    }
}

static void SetDecorSelectionBoxTiles(struct PlaceDecorationGraphicsDataBuffer *data)
{
    u16 i;
    for (i = 0; i < 64; i++)
        CopyTile(&data->image[i * TILE_SIZE_4BPP], data->tiles[i]);
}

static u16 GetMetatile(u16 tile)
{
    return ((u16 *)gTilesetPointer_SecretBaseRedCave->metatiles)[tile] & 0xFFF;
}

static void SetDecorSelectionMetatiles(struct PlaceDecorationGraphicsDataBuffer *data)
{
    u8 i;
    u8 shape;

    shape = data->decoration->shape;
    for (i = 0; i < sDecorTilemaps[shape].size; i++)
    {
        data->tiles[sDecorTilemaps[shape].tiles[i]] = GetMetatile(data->decoration->tiles[sDecorTilemaps[shape].y[i]] * NUM_TILES_PER_METATILE + sDecorTilemaps[shape].x[i]);
    }
}

static void SetDecorSelectionBoxOamAttributes(u8 decorShape)
{
    sDecorSelectorOam.y = 0;
    sDecorSelectorOam.affineMode = ST_OAM_AFFINE_OFF;
    sDecorSelectorOam.objMode = ST_OAM_OBJ_NORMAL;
    sDecorSelectorOam.mosaic = FALSE;
    sDecorSelectorOam.bpp = ST_OAM_4BPP;
    sDecorSelectorOam.shape = sDecorationMovementInfo[decorShape].shape;
    sDecorSelectorOam.x = 0;
    sDecorSelectorOam.matrixNum = 0;
    sDecorSelectorOam.size = sDecorationMovementInfo[decorShape].size;
    sDecorSelectorOam.tileNum = 0;
    sDecorSelectorOam.priority = 0;
    sDecorSelectorOam.paletteNum = 0;
}

static void InitializePuttingAwayCursorSprite(struct Sprite *sprite)
{
    sprite->data[2] = 0;
    sprite->data[3] = 0;
    sprite->data[4] = 0;
    sprite->data[5] = 0;
    sprite->data[6] = 0;
    sprite->data[7] = 0;
    sprite->callback = InitializePuttingAwayCursorSprite2;
}

static void InitializePuttingAwayCursorSprite2(struct Sprite *sprite)
{
    if (sprite->data[7] == 0)
    {
        if (sprite->data[6] < 15)
            sprite->invisible = 0;
        else
            sprite->invisible = 1;

        sprite->data[6]++;
        sprite->data[6] &= 0x1F;
    }
    else
    {
        sprite->invisible = FALSE;
    }
}

static u8 gpu_pal_decompress_alloc_tag_and_upload(struct PlaceDecorationGraphicsDataBuffer *data, u8 decor)
{
    ClearPlaceDecorationGraphicsDataBuffer(data);
    data->decoration = &gDecorations[decor];
    if (data->decoration->permission == DECORPERM_SPRITE)
        return CreateObjectGraphicsSprite(data->decoration->tiles[0], SpriteCallbackDummy, 0, 0, 1);

    FreeSpritePaletteByTag(PLACE_DECORATION_SELECTOR_TAG);
    SetDecorSelectionMetatiles(data);
    SetDecorSelectionBoxOamAttributes(data->decoration->shape);
    SetDecorSelectionBoxTiles(data);
    CopyPalette(data->palette, ((u16 *)gTilesetPointer_SecretBaseRedCave->metatiles)[(data->decoration->tiles[0] * NUM_TILES_PER_METATILE) + 7] >> 12);
    LoadSpritePalette(&sSpritePal_PlaceDecoration);
    return CreateSprite(&sDecorationSelectorSpriteTemplate, 0, 0, 0);
}

static u8 AddDecorationIconObjectFromIconTable(u16 tilesTag, u16 paletteTag, u8 decor)
{
    struct SpriteSheet sheet;
    struct CompressedSpritePalette palette;
    struct SpriteTemplate *template;
    u8 spriteId;

    if (!AllocItemIconTemporaryBuffers())
        return MAX_SPRITES;

    LZDecompressWram(GetDecorationIconPicOrPalette(decor, 0), gItemIconDecompressionBuffer);
    CopyItemIconPicTo4x4Buffer(gItemIconDecompressionBuffer, gItemIcon4x4Buffer);
    sheet.data = gItemIcon4x4Buffer;
    sheet.size = 0x200;
    sheet.tag = tilesTag;
    LoadSpriteSheet(&sheet);
    palette.data = GetDecorationIconPicOrPalette(decor, 1);
    palette.tag = paletteTag;
    LoadCompressedSpritePalette(&palette);
    template = Alloc(sizeof(struct SpriteTemplate));
    *template = gItemIconSpriteTemplate;
    template->tileTag = tilesTag;
    template->paletteTag = paletteTag;
    spriteId = CreateSprite(template, 0, 0, 0);
    FreeItemIconTemporaryBuffers();
    Free(template);
    return spriteId;
}

static const u32 *GetDecorationIconPicOrPalette(u16 decor, u8 mode)
{
    if (decor > NUM_DECORATIONS)
        decor = DECOR_NONE;

    return gDecorIconTable[decor][mode];
}

static u8 AddDecorationIconObjectFromObjectEvent(u16 tilesTag, u16 paletteTag, u8 decor)
{
    u8 spriteId;
    struct SpriteSheet sheet;
    struct SpritePalette palette;
    struct SpriteTemplate *template;

    ClearPlaceDecorationGraphicsDataBuffer(&sPlaceDecorationGraphicsDataBuffer);
    sPlaceDecorationGraphicsDataBuffer.decoration = &gDecorations[decor];
    if (sPlaceDecorationGraphicsDataBuffer.decoration->permission != DECORPERM_SPRITE)
    {
        SetDecorSelectionMetatiles(&sPlaceDecorationGraphicsDataBuffer);
        SetDecorSelectionBoxOamAttributes(sPlaceDecorationGraphicsDataBuffer.decoration->shape);
        SetDecorSelectionBoxTiles(&sPlaceDecorationGraphicsDataBuffer);
        CopyPalette(sPlaceDecorationGraphicsDataBuffer.palette, ((u16 *)gTilesetPointer_SecretBaseRedCave->metatiles)[(sPlaceDecorationGraphicsDataBuffer.decoration->tiles[0] * NUM_TILES_PER_METATILE) + 7] >> 12);
        sheet.data = sPlaceDecorationGraphicsDataBuffer.image;
        sheet.size = sDecorShapeSizes[sPlaceDecorationGraphicsDataBuffer.decoration->shape] * TILE_SIZE_4BPP;
        sheet.tag = tilesTag;
        LoadSpriteSheet(&sheet);
        palette.data = sPlaceDecorationGraphicsDataBuffer.palette;
        palette.tag = paletteTag;
        LoadSpritePalette(&palette);
        template = Alloc(sizeof(struct SpriteTemplate));
        *template = sDecorWhilePlacingSpriteTemplate;
        template->tileTag = tilesTag;
        template->paletteTag = paletteTag;
        spriteId = CreateSprite(template, 0, 0, 0);
        Free(template);
    }
    else
    {
        spriteId = CreateObjectGraphicsSprite(sPlaceDecorationGraphicsDataBuffer.decoration->tiles[0], SpriteCallbackDummy, 0, 0, 1);
    }
    return spriteId;
}

u8 AddDecorationIconObject(u8 decor, s16 x, s16 y, u8 priority, u16 tilesTag, u16 paletteTag)
{
    u8 spriteId;

    if (decor > NUM_DECORATIONS)
    {
        spriteId = AddDecorationIconObjectFromIconTable(tilesTag, paletteTag, DECOR_NONE);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x + 4;
        gSprites[spriteId].y2 = y + 4;
    }
    else if (gDecorIconTable[decor][0] == NULL)
    {
        spriteId = AddDecorationIconObjectFromObjectEvent(tilesTag, paletteTag, decor);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x;
        if (decor == DECOR_SILVER_SHIELD || decor == DECOR_GOLD_SHIELD)
            gSprites[spriteId].y2 = y - 4;
        else
            gSprites[spriteId].y2 = y;
    }
    else
    {
        spriteId = AddDecorationIconObjectFromIconTable(tilesTag, paletteTag, decor);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x + 4;
        gSprites[spriteId].y2 = y + 4;
    }

    gSprites[spriteId].oam.priority = priority;
    return spriteId;
}

static void ClearDecorationContextIndex(u8 idx)
{
    sDecorationContext.items[idx] = DECOR_NONE;
    sDecorationContext.pos[idx] = 0;
}

// Input
// gSpecialVar_0x8004: Current iteration.
//
// Output
// gSpecialVar_Result: TRUE if all iterations complete.
// gSpecialVar_0x8005: flagId of decoration (if any).
// gSpecialVar_0x8006: localId of decoration object event (if any).
void PutAwayDecorationIteration(void)
{
    u16 i;

    gSpecialVar_0x8005 = 0;
    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 == sCurDecorSelectedInRearrangement)
    {
        gSpecialVar_Result = TRUE;
    }
    else if (gDecorations[sDecorationContext.items[sDecorRearrangementDataBuffer[gSpecialVar_0x8004].idx]].permission == DECORPERM_SPRITE)
    {
        gSpecialVar_0x8005 = sDecorRearrangementDataBuffer[gSpecialVar_0x8004].flagId;
        ClearDecorationContextIndex(sDecorRearrangementDataBuffer[gSpecialVar_0x8004].idx);
        for (i = 0; i < gMapHeader.events->objectEventCount; i++)
        {
            if (gMapHeader.events->objectEvents[i].flagId == gSpecialVar_0x8005)
            {
                gSpecialVar_0x8006 = gMapHeader.events->objectEvents[i].localId;
                break;
            }
        }
    }
}

// Unused
void GetObjectEventLocalIdByFlag(void)
{
    u8 i;

    for (i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        if (gMapHeader.events->objectEvents[i].flagId == gSpecialVar_0x8004)
        {
            gSpecialVar_0x8005 = gMapHeader.events->objectEvents[i].localId;
            break;
        }
    }
}

static void ClearRearrangementNonSprites(void)
{
    u8 i;
    u8 y;
    u8 x;
    int posX;
    int posY;
    u8 perm;

    for (i = 0; i < sCurDecorSelectedInRearrangement; i++)
    {
        perm = gDecorations[sDecorationContext.items[sDecorRearrangementDataBuffer[i].idx]].permission;
        posX = sDecorationContext.pos[sDecorRearrangementDataBuffer[i].idx] >> 4;
        posY = sDecorationContext.pos[sDecorRearrangementDataBuffer[i].idx] & 0x0F;
        if (perm != DECORPERM_SPRITE)
        {
            for (y = 0; y < sDecorRearrangementDataBuffer[i].height; y++)
            {
                for (x = 0; x < sDecorRearrangementDataBuffer[i].width; x++)
                {
                    MapGridSetMetatileEntryAt(posX + MAP_OFFSET + x, posY + MAP_OFFSET - y, gMapHeader.mapLayout->map[posX + x + gMapHeader.mapLayout->width * (posY - y)] | 0x3000);
                }
            }

            ClearDecorationContextIndex(sDecorRearrangementDataBuffer[i].idx);
        }
    }
}

static void Task_PutAwayDecoration(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        ClearRearrangementNonSprites();
        gTasks[taskId].tState = 1;
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            DrawWholeMapView();
            ScriptContext_SetupScript(SecretBase_EventScript_PutAwayDecoration);
            ClearDialogWindowAndFrame(0, TRUE);
            gTasks[taskId].tState = 2;
        }
        break;
    case 2:
        LockPlayerFieldControls();
        IdentifyOwnedDecorationsCurrentlyInUseInternal(taskId);
        FadeInFromBlack();
        gTasks[taskId].tState = 3;
        break;
    case 3:
        if (IsWeatherNotFadingIn() == TRUE)
        {
            StringExpandPlaceholders(gStringVarBuffer, gText_DecorationReturnedToPC);
            DisplayItemMessageOnField(taskId, gStringVarBuffer, ContinuePuttingAwayDecorationsPrompt);
            if (gMapHeader.regionMapSectionId == MAPSEC_SECRET_BASE)
                TryPutSecretBaseVisitOnAir();
        }
        break;
    }
}

static bool8 HasDecorationsInUse(u8 taskId)
{
    u16 i;
    for (i = 0; i < sDecorationContext.size; i++)
    {
        if (sDecorationContext.items[i] != DECOR_NONE)
            return TRUE;
    }

    return FALSE;
}

static void SetUpPuttingAwayDecorationPlayerAvatar(void)
{
    GetPlayerFacingDirection();
    sDecor_CameraSpriteObjectIdx1 = gSprites[gFieldCamera.spriteId].data[0];
    LoadPlayerSpritePalette();
    gFieldCamera.spriteId = CreateSprite(&sPuttingAwayCursorSpriteTemplate, 120, 80, 0);
    if (gSaveBlock2Ptr->playerGender == MALE)
        sDecor_CameraSpriteObjectIdx2 = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_BRENDAN_DECORATING, SpriteCallbackDummy, 136, 72, 0);
    else
        sDecor_CameraSpriteObjectIdx2 = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_MAY_DECORATING, SpriteCallbackDummy, 136, 72, 0);

    gSprites[sDecor_CameraSpriteObjectIdx2].oam.priority = 1;
    DestroySprite(&gSprites[sDecor_CameraSpriteObjectIdx1]);
    sDecor_CameraSpriteObjectIdx1 = gFieldCamera.spriteId;
    gSprites[sDecor_CameraSpriteObjectIdx1].oam.priority = 1;
}

static void Task_ContinuePuttingAwayDecorations(u8 taskId)
{
    s16 *data;

    data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        if (!gPaletteFade.active)
        {
            SetInitialPositions(taskId);
            tState = 1;
            tDecorHeight = 1;
            tDecorWidth = 1;
        }
        break;
    case 1:
        SetUpPuttingAwayDecorationPlayerAvatar();
        FadeInFromBlack();
        tState = 2;
        break;
    case 2:
        if (IsWeatherNotFadingIn() == TRUE)
        {
            tDecorationItemsMenuCommand = DECOR_ITEMS_MENU_PUT_AWAY;
            ContinuePuttingAwayDecorations(taskId);
        }
        break;
    }
}

static void ContinuePuttingAwayDecorations(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    gSprites[sDecor_CameraSpriteObjectIdx1].data[7] = 0;
    gSprites[sDecor_CameraSpriteObjectIdx1].invisible = FALSE;
    gSprites[sDecor_CameraSpriteObjectIdx1].callback = InitializeCameraSprite1;
    gSprites[sDecor_CameraSpriteObjectIdx2].x = 136;
    gSprites[sDecor_CameraSpriteObjectIdx2].y = 72;
    gTasks[taskId].tButton = 0;
    gTasks[taskId].func = Task_SelectLocation;
}

static void AttemptPutAwayDecoration(u8 taskId)
{
    gTasks[taskId].tButton = 0;
    ResetCursorMovement();
    AttemptPutAwayDecoration_(taskId);
}

static void AttemptCancelPutAwayDecoration(u8 taskId)
{
    gTasks[taskId].tButton = 0;
    ResetCursorMovement();
    gSprites[sDecor_CameraSpriteObjectIdx1].invisible = FALSE;
    gSprites[sDecor_CameraSpriteObjectIdx1].callback = SpriteCallbackDummy;
    StringExpandPlaceholders(gStringVarBuffer, gText_StopPuttingAwayDecorations);
    DisplayItemMessageOnField(taskId, gStringVarBuffer, StopPuttingAwayDecorationsPrompt);
}

static void AttemptPutAwayDecoration_(u8 taskId)
{
    s16 *data;
    u8 behavior;

    AttemptMarkDecorUnderCursorForRemoval(taskId);
    if (sCurDecorSelectedInRearrangement != 0)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_ReturnDecorationToPC);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnDecorationPrompt);
    }
    else
    {
        data = gTasks[taskId].data;
        behavior = MapGridGetMetatileBehaviorAt(tCursorX, tCursorY);
        if (MetatileBehavior_IsSecretBasePC(behavior) == TRUE || MetatileBehavior_IsPlayerRoomPCOn(behavior) == TRUE)
        {
            gSprites[sDecor_CameraSpriteObjectIdx1].invisible = FALSE;
            gSprites[sDecor_CameraSpriteObjectIdx1].callback = SpriteCallbackDummy;
            StringExpandPlaceholders(gStringVarBuffer, gText_StopPuttingAwayDecorations);
            DisplayItemMessageOnField(taskId, gStringVarBuffer, StopPuttingAwayDecorationsPrompt);
        }
        else
        {
            StringExpandPlaceholders(gStringVarBuffer, gText_NoDecorationHere);
            DisplayItemMessageOnField(taskId, gStringVarBuffer, ContinuePuttingAwayDecorationsPrompt);
        }
    }
}

static void ContinuePuttingAwayDecorationsPrompt(u8 taskId)
{
    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
        ContinuePuttingAwayDecorations(taskId);
}

static void SetDecorRearrangementShape(u8 decor, struct DecorRearrangementDataBuffer *data)
{
    if (gDecorations[decor].shape == DECORSHAPE_1x1)
    {
        data->width = 1;
        data->height = 1;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_2x1)
    {
        data->width = 2;
        data->height = 1;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_3x1)
    {
        data->width = 3;
        data->height = 1;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_4x2)
    {
        data->width = 4;
        data->height = 2;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_2x2)
    {
        data->width = 2;
        data->height = 2;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_1x2)
    {
        data->width = 1;
        data->height = 2;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_1x3)
    {
        data->width = 1;
        data->height = 3;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_2x4)
    {
        data->width = 2;
        data->height = 4;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_3x3)
    {
        data->width = 3;
        data->height = 3;
    }
    else if (gDecorations[decor].shape == DECORSHAPE_3x2)
    {
        data->width = 3;
        data->height = 2;
    }
}

static void SetCameraSpritePosition(u8 x, u8 y)
{
    gSprites[sDecor_CameraSpriteObjectIdx1].invisible = TRUE;
    gSprites[sDecor_CameraSpriteObjectIdx1].callback = SpriteCallbackDummy;
    gSprites[sDecor_CameraSpriteObjectIdx2].x = x * 16 + 136;
    gSprites[sDecor_CameraSpriteObjectIdx2].y = y * 16 + 72;
}

static bool8 DecorationIsUnderCursor(u8 taskId, u8 idx, struct DecorRearrangementDataBuffer *data)
{
    u8 x;
    u8 y;
    u8 xOff;
    u8 yOff;
    u8 ht;

    x = gTasks[taskId].tCursorX - MAP_OFFSET;
    y = gTasks[taskId].tCursorY - MAP_OFFSET;
    xOff = sDecorationContext.pos[idx] >> 4;
    yOff = sDecorationContext.pos[idx] & 0x0F;
    ht = data->height;
    if (sDecorationContext.items[idx] == DECOR_SAND_ORNAMENT && MapGridGetMetatileIdAt(xOff + MAP_OFFSET, yOff + MAP_OFFSET) == METATILE_SecretBase_SandOrnament_BrokenBase)
        ht--;

    if (x >= xOff && x < xOff + data->width && y > yOff - ht && y <= yOff)
    {
        SetCameraSpritePosition(data->width - (x - xOff + 1), yOff - y);
        return TRUE;
    }

    return FALSE;
}

static void SetDecorRearrangementFlagIdIfFlagUnset(void)
{
    u8 xOff;
    u8 yOff;
    u16 i;

    xOff = sDecorationContext.pos[sDecorRearrangementDataBuffer[sCurDecorSelectedInRearrangement].idx] >> 4;
    yOff = sDecorationContext.pos[sDecorRearrangementDataBuffer[sCurDecorSelectedInRearrangement].idx] & 0x0F;
    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->objectEventTemplates[i].x == xOff && gSaveBlock1Ptr->objectEventTemplates[i].y == yOff && !FlagGet(gSaveBlock1Ptr->objectEventTemplates[i].flagId))
        {
            sDecorRearrangementDataBuffer[sCurDecorSelectedInRearrangement].flagId = gSaveBlock1Ptr->objectEventTemplates[i].flagId;
            break;
        }
    }
}

static bool8 AttemptMarkSpriteDecorUnderCursorForRemoval(u8 taskId)
{
    u16 i;

    for (i = 0; i < sDecorationContext.size; i++)
    {
        if (sDecorationContext.items[i] != DECOR_NONE)
        {
            if (gDecorations[sDecorationContext.items[i]].permission == DECORPERM_SPRITE)
            {
                SetDecorRearrangementShape(sDecorationContext.items[i], sDecorRearrangementDataBuffer);
                if (DecorationIsUnderCursor(taskId, i, sDecorRearrangementDataBuffer) == TRUE)
                {
                    sDecorRearrangementDataBuffer->idx = i;
                    SetDecorRearrangementFlagIdIfFlagUnset();
                    sCurDecorSelectedInRearrangement = 1;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static void MarkSpriteDecorsInBoundsForRemoval(u8 left, u8 top, u8 right, u8 bottom)
{
    u8 i;
    u8 xOff;
    u8 yOff;
    u8 decor;

    for (i = 0; i < sDecorationContext.size; i++)
    {
        decor = sDecorationContext.items[i];
        xOff = sDecorationContext.pos[i] >> 4;
        yOff = sDecorationContext.pos[i] & 0x0F;
        if (decor != DECOR_NONE && gDecorations[decor].permission == DECORPERM_SPRITE && left <= xOff && top <= yOff && right >= xOff && bottom >= yOff)
        {
            sDecorRearrangementDataBuffer[sCurDecorSelectedInRearrangement].idx = i;
            SetDecorRearrangementFlagIdIfFlagUnset();
            sCurDecorSelectedInRearrangement++;
        }
    }
}

static void AttemptMarkDecorUnderCursorForRemoval(u8 taskId)
{
    u8 i;
    u8 xOff;
    u8 yOff;
    u8 var1;
    u32 var2;

    sCurDecorSelectedInRearrangement = 0;
    if (AttemptMarkSpriteDecorUnderCursorForRemoval(taskId) != TRUE)
    {
        // Not a sprite.
        for (i = 0; i < sDecorationContext.size; i++)
        {
            var1 = sDecorationContext.items[i];
            if (var1 != DECOR_NONE)
            {
                SetDecorRearrangementShape(var1, &sDecorRearrangementDataBuffer[0]);
                if (DecorationIsUnderCursor(taskId, i, &sDecorRearrangementDataBuffer[0]) == TRUE)
                {
                    sDecorRearrangementDataBuffer[0].idx = i;
                    sCurDecorSelectedInRearrangement++;
                    break;
                }
            }
        }
        if (sCurDecorSelectedInRearrangement != 0)
        {
            xOff = sDecorationContext.pos[sDecorRearrangementDataBuffer[0].idx] >> 4;
            yOff = sDecorationContext.pos[sDecorRearrangementDataBuffer[0].idx] & 0x0F;
            var1 = yOff - sDecorRearrangementDataBuffer[0].height + 1;
            var2 = sDecorRearrangementDataBuffer[0].width + xOff - 1;

            // Remove any dolls/cushions on this decoration.
            MarkSpriteDecorsInBoundsForRemoval(xOff, var1, var2, yOff);
        }
    }
}

static void ReturnDecorationPrompt(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sReturnDecorationYesNoFunctions);
}

static void PutAwayDecoration(u8 taskId)
{
    FadeScreen(FADE_TO_BLACK, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].func = Task_PutAwayDecoration;
}

static void StopPuttingAwayDecorationsPrompt(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sStopPuttingAwayDecorationsYesNoFunctions);
}

static void StopPuttingAwayDecorations(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    StopPuttingAwayDecorations_(taskId);
}

static void StopPuttingAwayDecorations_(u8 taskId)
{
    FadeScreen(FADE_TO_BLACK, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].func = Task_StopPuttingAwayDecorations;
}

static void Task_StopPuttingAwayDecorations(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        if (!gPaletteFade.active)
        {
            WarpToInitialPosition(taskId);
            gTasks[taskId].tState = 1;
        }
        break;
    case 1:
        FreePlayerSpritePalette();
        gFieldCallback = FieldCB_StopPuttingAwayDecorations;
        SetMainCallback2(CB2_ReturnToField);
        DestroyTask(taskId);
        break;
    }
}

static void Task_ReinitializeDecorationMenuHandler(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        HideSecretBaseDecorationSprites();
        tState++;
        break;
    case 1:
        ScriptContext_SetupScript(SecretBase_EventScript_InitDecorations);
        tState++;
        break;
    case 2:
        LockPlayerFieldControls();
        tState++;
        break;
    case 3:
        if (IsWeatherNotFadingIn() == TRUE)
            gTasks[taskId].func = HandleDecorationActionsMenuInput;
        break;
    }
}

static void FieldCB_StopPuttingAwayDecorations(void)
{
    u8 taskId;

    FadeInFromBlack();
    DrawDialogueFrame(0, TRUE);
    InitDecorationActionsWindow();
    taskId = CreateTask(Task_ReinitializeDecorationMenuHandler, 8);
    gTasks[taskId].tState = 0;
}

static void InitializeCameraSprite1(struct Sprite *sprite)
{
    sprite->data[0]++;
    sprite->data[0] &= 0x1F;
    if (sprite->data[0] > 15)
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

static void LoadPlayerSpritePalette(void)
{
    if (gSaveBlock2Ptr->playerGender == MALE)
        LoadSpritePalette(&sSpritePal_PuttingAwayCursorBrendan);
    else
        LoadSpritePalette(&sSpritePal_PuttingAwayCursorMay);
}

static void FreePlayerSpritePalette(void)
{
    FreeSpritePaletteByTag(PLACE_DECORATION_PLAYER_TAG);
}

static void DecorationItemsMenuAction_AttemptToss(u8 taskId)
{
    if (IsSelectedDecorInThePC() == TRUE)
    {
        StringCopy(gStringVar1, gDecorations[gCurDecorationItems[gCurDecorationIndex]].name);
        StringExpandPlaceholders(gStringVarBuffer, gText_DecorationWillBeDiscarded);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, TossDecorationPrompt);
    }
    else
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_CantThrowAwayInUse);
        DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationItemsAfterInvalidSelection);
    }
}

static void TossDecorationPrompt(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sTossDecorationYesNoFunctions);
}

static void TossDecoration(u8 taskId)
{
    gCurDecorationItems[gCurDecorationIndex] = DECOR_NONE;
    sNumOwnedDecorationsInCurCategory = GetNumOwnedDecorationsInCategory(sCurDecorationCategory);
    CondenseDecorationsInCategory(sCurDecorationCategory);
    IdentifyOwnedDecorationsCurrentlyInUseInternal(taskId);
    StringExpandPlaceholders(gStringVarBuffer, gText_DecorationThrownAway);
    DisplayItemMessageOnField(taskId, gStringVarBuffer, ReturnToDecorationItemsAfterInvalidSelection);
}
