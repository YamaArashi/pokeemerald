#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_pyramid_bag.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "field_effect.h"
#include "field_weather.h"
#include "graphics.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item.h"
#include "item_icon.h"
#include "item_menu.h"
#include "item_use.h"
#include "list_menu.h"
#include "mail.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "task.h"
#include "text_window.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define TAG_SCROLL_ARROW  2910
#define TAG_PYRAMID_BAG   4132
#define TAG_ITEM_ICON     4133
#define TAG_ITEM_ICON_ALT 4134

#define POS_NONE ((u8)-1)

enum {
    WIN_LIST,
    WIN_INFO,
    WIN_MSG,
    WIN_TOSS_NUM,
};

EWRAM_DATA struct PyramidBagMenu *gPyramidBagMenu = NULL;
EWRAM_DATA struct PyramidBagMenuState gPyramidBagMenuState = {0};

static void Task_HandlePyramidBagInput(u8);
static void Task_ChooseItemsToTossFromPyramidBag(u8);
static void Task_ClosePyramidBag(u8);
static void Task_BeginItemSwap(u8);
static void OpenContextMenu(u8);
static void TryCloseBagToGiveItem(u8);
static void HandleMenuActionInput_2x2(u8);
static void HandleMenuActionInput_SingleRow(u8);
static void Task_WaitCloseErrorMessage(u8);
static void SetTaskToMainPyramidBagInputHandler(u8);
static void AskConfirmToss(u8);
static void Task_ChooseHowManyToToss(u8);
static void Task_TossItem(u8);
static void ShowCantHoldMessage(u8);
static void PerformItemSwap(u8);
static void Task_ItemSwapHandleInput(u8);
static void CancelItemSwap(u8);
static void SetBagItemsListTemplate(void);
static void CB2_LoadPyramidBagMenu(void);
static void InitPyramidBagBgs(void);
static void AddScrollArrows(void);
static void CreatePyramidBagInputTask(void);
static void InitPyramidBagScroll(void);
static void InitPyramidBagWindows(void);
static void CreatePyramidBagSprite(void);
static void CreateSwapLine(void);
static void LoadPyramidBagPalette(void);
static void ShakePyramidBag(void);
static void ShowNumToToss(void);
static void CloseBattlePyramidBagTextWindow(void);
static bool8 LoadPyramidBagGfx(void);
static bool8 LoadPyramidBagMenu(void);
static void ShowItemIcon(u16, u8);
static void CopyBagItemName(u8 *, u16);
static void FreeItemIconSpriteByAltId(u8);
static void PrintItemDescription(s32);
static void PrintSelectorArrowAtPos(u8, u8);
static void PyramidBagPrint(u8, const u8 *, u8, u8, u8, u8, u8, u8);
static void PyramidBagPrint_Quantity(u8, const u8 *, u8, u8, u8, u8, u8, u8);
static u8 OpenMenuActionWindowById(u8);
static void CloseMenuActionWindowById(u8);
static void PrintMenuActionText_SingleRow(u8);
static void PrintMenuActionText_MultiRow(u8, u8, u8);
static bool8 IsValidMenuAction(s8);
static void CreatePyramidBagYesNo(u8, const struct YesNoFuncTable *);
static void DrawTossNumberWindow(u8);
static void UpdateSwapLinePos(u8);
static void SetSwapLineInvisibility(bool8);
static void SpriteCB_BagWaitForShake(struct Sprite *);
static void BagAction_UseOnField(u8);
static void BagAction_Toss(u8);
static void BagAction_Give(u8);
static void BagAction_Cancel(u8);
static void BagAction_UseInBattle(u8);
static void BagCursorMoved(s32, bool8, struct ListMenu *);
static void PrintItemQuantity(u8 windowId, u32 itemId, u8 y);
static void TossItem(u8);
static void DontTossItem(u8);

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct ListMenuTemplate sListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = BagCursorMoved,
    .itemPrintFunc = PrintItemQuantity,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = WIN_LIST,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_BLACK_ARROW
};

enum {
    ACTION_USE_FIELD,
    ACTION_TOSS,
    ACTION_GIVE,
    ACTION_CANCEL,
    ACTION_USE_BATTLE,
    ACTION_DUMMY,
};

static const struct MenuAction sMenuActions[] =
{
    [ACTION_USE_FIELD] =    { gMenuText_Use, {BagAction_UseOnField} },
    [ACTION_TOSS] =         { gMenuText_Toss, {BagAction_Toss} },
    [ACTION_GIVE] =         { gMenuText_Give, {BagAction_Give} },
    [ACTION_CANCEL] =       { gText_Cancel2, {BagAction_Cancel} },
    [ACTION_USE_BATTLE] =   { gMenuText_Use, {BagAction_UseInBattle} },
    [ACTION_DUMMY] =        { gText_EmptyString2, {NULL} },
};

static const u8 sMenuActionIds_Field[] = {ACTION_USE_FIELD, ACTION_GIVE, ACTION_TOSS, ACTION_CANCEL};
static const u8 sMenuActionIds_ChooseToss[] = {ACTION_TOSS, ACTION_CANCEL};
static const u8 sMenuActionIds_Battle[] = {ACTION_USE_BATTLE, ACTION_CANCEL};
static const u8 sMenuActionIds_BattleCannotUse[] = {ACTION_CANCEL};

static const struct YesNoFuncTable sYesNoTossFuncions =
{
    TossItem, DontTossItem
};

enum {
    COLORID_DARK_GRAY,
    COLORID_LIGHT_GRAY,
    COLORID_WHITE_BG,
    COLORID_NONE = 0xFF
};

static const u8 sTextColors[][3] =
{
    [COLORID_DARK_GRAY]  = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY,  TEXT_COLOR_LIGHT_GRAY},
    [COLORID_LIGHT_GRAY] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_WHITE},
    [COLORID_WHITE_BG]   = {TEXT_COLOR_WHITE,       TEXT_COLOR_DARK_GRAY,  TEXT_COLOR_LIGHT_GRAY}, // Unused
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 14,
        .tilemapTop = 2,
        .width = 15,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 30
    },
    [WIN_INFO] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 13,
        .width = 14,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 270
    },
    [WIN_MSG] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 354
    },
    [WIN_TOSS_NUM] = {
        .bg = 1,
        .tilemapLeft = 24,
        .tilemapTop = 17,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 462
    },
    DUMMY_WIN_TEMPLATE,
};

enum {
    MENU_WIN_1x1,
    MENU_WIN_1x2,
    MENU_WIN_2x2,
    MENU_WIN_2x3,
    MENU_WIN_YESNO,
};

static const struct WindowTemplate sWindowTemplates_MenuActions[] =
{
    [MENU_WIN_1x1] = {
        .bg = 1,
        .tilemapLeft = 22,
        .tilemapTop = 17,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 472
    },
    [MENU_WIN_1x2] = {
        .bg = 1,
        .tilemapLeft = 22,
        .tilemapTop = 15,
        .width = 7,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 472
    },
    [MENU_WIN_2x2] = {
        .bg = 1,
        .tilemapLeft = 15,
        .tilemapTop = 15,
        .width = 14,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 472
    },
    [MENU_WIN_2x3] = { // Unused
        .bg = 1,
        .tilemapLeft = 15,
        .tilemapTop = 13,
        .width = 14,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 472
    },
    [MENU_WIN_YESNO] = {
        .bg = 1,
        .tilemapLeft = 24,
        .tilemapTop = 15,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 472
    },
};

static const struct OamData sOamData_PyramidBag =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_PyramidBag[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_END,
};

static const union AnimCmd * const sAnims_PyramidBag[] =
{
    sAnim_PyramidBag,
};

static const union AffineAnimCmd sAffineAnim_PyramidBag_Still[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_PyramidBag_Shake[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 254, 2),
    AFFINEANIMCMD_FRAME(0, 0, 2, 4),
    AFFINEANIMCMD_FRAME(0, 0, 254, 4),
    AFFINEANIMCMD_FRAME(0, 0, 2, 2),
    AFFINEANIMCMD_END,
};

enum {
    ANIM_BAG_STILL,
    ANIM_BAG_SHAKE,
};

static const union AffineAnimCmd * const sAffineAnims_PyramidBag[] =
{
    [ANIM_BAG_STILL] = sAffineAnim_PyramidBag_Still,
    [ANIM_BAG_SHAKE] = sAffineAnim_PyramidBag_Shake,
};

static const struct CompressedSpriteSheet sSpriteSheet_PyramidBag = {gBattlePyramidBag_Gfx, 0x0800, TAG_PYRAMID_BAG};

static const struct SpriteTemplate sSpriteTemplate_PyramidBag =
{
    .tileTag = TAG_PYRAMID_BAG,
    .paletteTag = TAG_PYRAMID_BAG,
    .oam = &sOamData_PyramidBag,
    .anims = sAnims_PyramidBag,
    .images = NULL,
    .affineAnims = sAffineAnims_PyramidBag,
    .callback = SpriteCallbackDummy
};

void InitBattlePyramidBagCursorPosition(void)
{
    gPyramidBagMenuState.cursorPosition = 0;
    gPyramidBagMenuState.scrollPosition = 0;
}

void CB2_PyramidBagMenuFromStartMenu(void)
{
    GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_FIELD, CB2_ReturnToFieldWithOpenMenu);
}

// CB2_BagMenuFromBattle is used instead
static void UNUSED OpenBattlePyramidBagInBattle(void)
{
    GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_BATTLE, CB2_SetUpReshowBattleScreenAfterMenu2);
}

// If the player finishes a round at the Battle Pyramid with insufficient space in their
// Pyramid Bag to store the party's held items, they may choose items to toss in order to
// make room.
void ChooseItemsToTossFromPyramidBag(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_ChooseItemsToTossFromPyramidBag, 10);
}

static void Task_ChooseItemsToTossFromPyramidBag(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        gFieldCallback2 = CB2_FadeFromPartyMenu;
        GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_CHOOSE_TOSS, CB2_ReturnToField);
        DestroyTask(taskId);
    }
}

void CB2_ReturnToPyramidBagMenu(void)
{
    GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_PREV, gPyramidBagMenuState.exitCallback);
}

void GoToBattlePyramidBagMenu(u8 location, void (*exitCallback)(void))
{
    gPyramidBagMenu = AllocZeroed(sizeof(*gPyramidBagMenu));

    if (location != PYRAMIDBAG_LOC_PREV)
        gPyramidBagMenuState.location = location;

    if (exitCallback != NULL)
        gPyramidBagMenuState.exitCallback = exitCallback;

    gPyramidBagMenu->newScreenCallback = NULL;
    gPyramidBagMenu->toSwapPos = POS_NONE;
    gPyramidBagMenu->scrollIndicatorsTaskId = TASK_NONE;

    memset(gPyramidBagMenu->spriteIds, SPRITE_NONE, sizeof(gPyramidBagMenu->spriteIds));
    memset(gPyramidBagMenu->windowIds, WINDOW_NONE, sizeof(gPyramidBagMenu->windowIds));

    SetMainCallback2(CB2_LoadPyramidBagMenu);
}

static void CB2_PyramidBag(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_PyramidBag(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_LoadPyramidBagMenu(void)
{
    while (MenuHelpers_ShouldWaitForLinkRecv() != TRUE
        && LoadPyramidBagMenu() != TRUE
        && MenuHelpers_IsLinkActive() != TRUE);
}

static bool8 LoadPyramidBagMenu(void)
{
    switch (gMain.state)
    {
        case 0:
            SetVBlankHBlankCallbacksToNull();
            ClearScheduledBgCopiesToVram();
            gMain.state++;
            break;
        case 1:
            ScanlineEffect_Stop();
            gMain.state++;
            break;
        case 2:
            FreeAllSpritePalettes();
            gMain.state++;
            break;
        case 3:
            ResetPaletteFade();
            gPaletteFade.bufferTransferDisabled = TRUE;
            gMain.state++;
            break;
        case 4:
            ResetSpriteData();
            gMain.state++;
            break;
        case 5:
            if (!MenuHelpers_IsLinkActive())
                ResetTasks();
            gMain.state++;
            break;
        case 6:
            InitPyramidBagBgs();
            gPyramidBagMenu->state = 0;
            gMain.state++;
            break;
        case 7:
            if (LoadPyramidBagGfx())
                gMain.state++;
            break;
        case 8:
            InitPyramidBagWindows();
            gMain.state++;
            break;
        case 9:
            UpdatePyramidBagList();
            UpdatePyramidBagCursorPos();
            InitPyramidBagScroll();
            gMain.state++;
            break;
        case 10:
            SetBagItemsListTemplate();
            gMain.state++;
            break;
        case 11:
            CreatePyramidBagInputTask();
            gMain.state++;
            break;
        case 12:
            CreatePyramidBagSprite();
            gMain.state++;
            break;
        case 13:
            AddScrollArrows();
            gMain.state++;
            break;
        case 14:
            CreateSwapLine();
            gMain.state++;
            break;
        case 15:
            BlendPalettes(PALETTES_ALL, 16, 0);
            gMain.state++;
            break;
        case 16:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
            gPaletteFade.bufferTransferDisabled = FALSE;
            gMain.state++;
            break;
        default:
            SetVBlankCallback(VBlankCB_PyramidBag);
            SetMainCallback2(CB2_PyramidBag);
            return TRUE;
    }
    return FALSE;
}

static void InitPyramidBagBgs(void)
{
    ResetVramOamAndBgCntRegs();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    SetBgTilemapBuffer(2, gPyramidBagMenu->tilemapBuffer);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(2);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                  DISPCNT_OBJ_1D_MAP |
                                  DISPCNT_OBJ_ON);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

static bool8 LoadPyramidBagGfx(void)
{
    switch (gPyramidBagMenu->state)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(2, gBagScreen_Gfx, 0, 0, 0);
        gPyramidBagMenu->state++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(gBattlePyramidBagTilemap, gPyramidBagMenu->tilemapBuffer);
            gPyramidBagMenu->state++;
        }
        break;
    case 2:
        LoadCompressedPalette(gBattlePyramidBagInterface_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        gPyramidBagMenu->state++;
        break;
    case 3:
        LoadCompressedSpriteSheet(&sSpriteSheet_PyramidBag);
        gPyramidBagMenu->state++;
        break;
    case 4:
        LoadPyramidBagPalette();
        gPyramidBagMenu->state++;
        break;
    default:
        LoadListMenuSwapLineGfx();
        gPyramidBagMenu->state = 0;
        return TRUE;
    }

    return FALSE;
}

static void SetBagItemsListTemplate(void)
{
    u16 i;
    u16 *itemIds = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];

    for (i = 0; i < gPyramidBagMenu->listMenuCount - 1; i++)
    {
        CopyBagItemName(gPyramidBagMenu->itemStrings[i], itemIds[i]);
        gPyramidBagMenu->bagListItems[i].name = gPyramidBagMenu->itemStrings[i];
        gPyramidBagMenu->bagListItems[i].id = i;
    }
    StringCopy(gPyramidBagMenu->itemStrings[i], gText_CloseBag);
    gPyramidBagMenu->bagListItems[i].name = gPyramidBagMenu->itemStrings[i];
    gPyramidBagMenu->bagListItems[i].id = LIST_CANCEL;
    gMultiuseListMenuTemplate = sListMenuTemplate;
    gMultiuseListMenuTemplate.totalItems = gPyramidBagMenu->listMenuCount;
    gMultiuseListMenuTemplate.items = gPyramidBagMenu->bagListItems;
    gMultiuseListMenuTemplate.maxShowed = gPyramidBagMenu->listMenuMaxShown;
}

static void CopyBagItemName(u8 *dst, u16 itemId)
{
    if (ItemId_GetPocket(itemId) == POCKET_BERRIES)
    {
        ConvertIntToDecimalStringN(gStringVar1, ITEM_TO_BERRY(itemId), STR_CONV_MODE_LEADING_ZEROS, 2);
        CopyItemName(itemId, gStringVar2);
        StringExpandPlaceholders(dst, gText_NumberItem_TMBerry);
    }
    else
    {
        CopyItemName(itemId, dst);
    }
}

static void BagCursorMoved(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (onInit != TRUE)
    {
        PlaySE(SE_SELECT);
        ShakePyramidBag();
    }
    if (gPyramidBagMenu->toSwapPos == POS_NONE)
    {
        FreeItemIconSpriteByAltId(gPyramidBagMenu->isAltIcon ^ 1);
        if (itemIndex != LIST_CANCEL)
            ShowItemIcon(gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode][itemIndex], gPyramidBagMenu->isAltIcon);
        else
            ShowItemIcon(ITEM_LIST_END, gPyramidBagMenu->isAltIcon); // Show exit arrow if on Cancel
        gPyramidBagMenu->isAltIcon ^= 1;
        PrintItemDescription(itemIndex);
    }
}

static void PrintItemQuantity(u8 windowId, u32 itemIndex, u8 y)
{
    s32 xAlign;
    if (itemIndex == LIST_CANCEL)
        return;

    if (gPyramidBagMenu->toSwapPos != POS_NONE)
    {
        // Performing a swap. Keep a gray selector arrow on the position to swap to
        // and erase the selector arrow anywhere else
        if (gPyramidBagMenu->toSwapPos == (u8)(itemIndex))
            PrintSelectorArrowAtPos(y, COLORID_LIGHT_GRAY);
        else
            PrintSelectorArrowAtPos(y, COLORID_NONE);
    }

    ConvertIntToDecimalStringN(gStringVar1,
                               gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode][itemIndex],
                               STR_CONV_MODE_RIGHT_ALIGN,
                               2);
    StringExpandPlaceholders(gStringVarBuffer, gText_xVar1);
    xAlign = GetStringRightAlignXOffset(FONT_NARROW, gStringVarBuffer, 119);
    PyramidBagPrint_Quantity(windowId, gStringVarBuffer, xAlign, y, 0, 0, TEXT_SKIP_DRAW, COLORID_DARK_GRAY);
}

static void PrintItemDescription(s32 listMenuId)
{
    const u8 *desc;
    if (listMenuId != LIST_CANCEL)
    {
        desc = ItemId_GetDescription(gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode][listMenuId]);
    }
    else
    {
        StringCopy(gStringVar1, gPyramidBagMenu_ReturnToStrings[gPyramidBagMenuState.location]);
        StringExpandPlaceholders(gStringVarBuffer, gText_ReturnToVar1);
        desc = gStringVarBuffer;
    }
    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    PyramidBagPrint(WIN_INFO, desc, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
}

static void AddScrollArrows(void)
{
    if (gPyramidBagMenu->scrollIndicatorsTaskId == TASK_NONE)
        gPyramidBagMenu->scrollIndicatorsTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 172, 12, 148,
                                                                                            gPyramidBagMenu->listMenuCount - gPyramidBagMenu->listMenuMaxShown,
                                                                                            TAG_SCROLL_ARROW, TAG_SCROLL_ARROW,
                                                                                            &gPyramidBagMenuState.scrollPosition);
}

static void RemoveScrollArrow(void)
{
    if (gPyramidBagMenu->scrollIndicatorsTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(gPyramidBagMenu->scrollIndicatorsTaskId);
        gPyramidBagMenu->scrollIndicatorsTaskId = TASK_NONE;
    }
}

#define tListTaskId data[0]
#define tListPos    data[1]
#define tQuantity   data[2]
#define tNumToToss  data[8]

static void CreatePyramidBagInputTask(void)
{
    u8 taskId = CreateTask(Task_HandlePyramidBagInput, 0);
    s16 *data = gTasks[taskId].data;
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gPyramidBagMenuState.scrollPosition, gPyramidBagMenuState.cursorPosition);
}

static void SwapItems(u8 id1, u8 id2)
{
    u16 temp;
    u16 *itemIds = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    SWAP(itemIds[id1], itemIds[id2], temp);
    SWAP(quantities[id1], quantities[id2], temp);
}

static void MovePyramidBagItemSlotInList(u8 from, u8 to)
{
    u16 *itemIds = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    if (from != to)
    {
        s16 i;
        u16 firstSlotItemId = itemIds[from];
        u8 firstSlotQuantity = quantities[from];

        if (to > from)
        {
            to--;
            for (i = from; i < to; i++)
            {
                itemIds[i] = itemIds[i + 1];
                quantities[i] = quantities[i + 1];
            }
        }
        else
        {
            for (i = from; i > to; i--)
            {
                itemIds[i] = itemIds[i - 1];
                quantities[i] = quantities[i - 1];
            }
        }
        itemIds[to] = firstSlotItemId;
        quantities[to] = firstSlotQuantity;
    }
}

static void CompactItems(void)
{
    u8 i, j;
    u16 *itemIds = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        if (itemIds[i] == ITEM_NONE || quantities[i] == 0)
        {
            itemIds[i] = ITEM_NONE;
            quantities[i] = 0;
        }
    }
    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT - 1; i++)
    {
        for (j = i + 1; j < PYRAMID_BAG_ITEMS_COUNT; j++)
        {
            if (itemIds[i] == ITEM_NONE || quantities[i] == 0)
                SwapItems(i, j);
        }
    }
}

void UpdatePyramidBagList(void)
{
    u16 i;
    u16 *itemIds = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];

    CompactItems();
    gPyramidBagMenu->listMenuCount = 0;
    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        if (itemIds[i] != ITEM_NONE)
            gPyramidBagMenu->listMenuCount++;
    }
    gPyramidBagMenu->listMenuCount++;
    if (gPyramidBagMenu->listMenuCount > 8)
        gPyramidBagMenu->listMenuMaxShown = 8;
    else
        gPyramidBagMenu->listMenuMaxShown = gPyramidBagMenu->listMenuCount;
}

void UpdatePyramidBagCursorPos(void)
{
    if (gPyramidBagMenuState.scrollPosition != 0 && gPyramidBagMenuState.scrollPosition + gPyramidBagMenu->listMenuMaxShown > gPyramidBagMenu->listMenuCount)
        gPyramidBagMenuState.scrollPosition = gPyramidBagMenu->listMenuCount - gPyramidBagMenu->listMenuMaxShown;

    if (gPyramidBagMenuState.scrollPosition + gPyramidBagMenuState.cursorPosition >= gPyramidBagMenu->listMenuCount)
    {
        if (gPyramidBagMenu->listMenuCount == 0)
            gPyramidBagMenuState.cursorPosition = 0;
        else
            gPyramidBagMenuState.cursorPosition = gPyramidBagMenu->listMenuCount - 1;
    }
}

static void InitPyramidBagScroll(void)
{
    u8 i;

    if (gPyramidBagMenuState.cursorPosition > 4)
    {
        for (i = 0; i <= gPyramidBagMenuState.cursorPosition - 4; i++)
        {
            if (gPyramidBagMenuState.scrollPosition + gPyramidBagMenu->listMenuMaxShown == gPyramidBagMenu->listMenuCount)
                break;

            gPyramidBagMenuState.cursorPosition--;
            gPyramidBagMenuState.scrollPosition++;
        }
    }
}

static void PrintSelectorArrow(u8 listMenuTaskId, u8 colorId)
{
    u8 y = ListMenuGetYCoordForPrintingArrowCursor(listMenuTaskId);
    PrintSelectorArrowAtPos(y, colorId);
}

static void PrintSelectorArrowAtPos(u8 y, u8 colorId)
{
    if (colorId == COLORID_NONE) // If 'no color', erase arrow
        FillWindowPixelRect(WIN_LIST, PIXEL_FILL(0), 0, y, GetMenuCursorDimensionByFont(FONT_NORMAL, 0), GetMenuCursorDimensionByFont(FONT_NORMAL, 1));
    else
        PyramidBagPrint(WIN_LIST, gText_SelectorArrow2, 0, y, 0, 0, 0, colorId);
}

void CloseBattlePyramidBag(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ClosePyramidBag;
}

static void Task_ClosePyramidBag(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(tListTaskId, &gPyramidBagMenuState.scrollPosition, &gPyramidBagMenuState.cursorPosition);

        // If ready for a new screen (e.g. party menu for giving an item) go to that screen
        // Otherwise exit the bag and use callback set up when the bag was first opened
        if (gPyramidBagMenu->newScreenCallback != NULL)
            SetMainCallback2(gPyramidBagMenu->newScreenCallback);
        else
            SetMainCallback2(gPyramidBagMenuState.exitCallback);
        RemoveScrollArrow();
        ResetSpriteData();
        FreeAllSpritePalettes();
        FreeAllWindowBuffers();
        Free(gPyramidBagMenu);
        DestroyTask(taskId);
    }
}

static void Task_HandlePyramidBagInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE || gPaletteFade.active)
        return;

    if (JOY_NEW(SELECT_BUTTON))
    {
        if (gPyramidBagMenuState.location != PYRAMIDBAG_LOC_PARTY)
        {
            ListMenuGetScrollAndRow(tListTaskId, &gPyramidBagMenuState.scrollPosition, &gPyramidBagMenuState.cursorPosition);
            if (gPyramidBagMenuState.scrollPosition + gPyramidBagMenuState.cursorPosition != gPyramidBagMenu->listMenuCount - 1)
            {
                PlaySE(SE_SELECT);
                Task_BeginItemSwap(taskId);
            }
        }
    }
    else
    {
        s32 listId = ListMenu_ProcessInput(tListTaskId);
        ListMenuGetScrollAndRow(tListTaskId, &gPyramidBagMenuState.scrollPosition, &gPyramidBagMenuState.cursorPosition);
        switch (listId)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            gSpecialVar_ItemId = ITEM_NONE;
            CloseBattlePyramidBag(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            gSpecialVar_ItemId = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode][listId];
            tListPos = listId;
            tQuantity = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode][listId];
            if (gPyramidBagMenuState.location == PYRAMIDBAG_LOC_PARTY)
                TryCloseBagToGiveItem(taskId);
            else
                OpenContextMenu(taskId);
            break;
        }
    }
}

static void OpenContextMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    RemoveScrollArrow();
    PrintSelectorArrow(tListTaskId, COLORID_LIGHT_GRAY);
    switch (gPyramidBagMenuState.location)
    {
    default:
//  case PYRAMIDBAG_LOC_FIELD:
//  case PYRAMIDBAG_LOC_PARTY:
        gPyramidBagMenu->menuActionIds = sMenuActionIds_Field;
        gPyramidBagMenu->menuActionsCount = ARRAY_COUNT(sMenuActionIds_Field);
        break;
    case PYRAMIDBAG_LOC_BATTLE:
        if (ItemId_GetBattleUsage(gSpecialVar_ItemId))
        {
            gPyramidBagMenu->menuActionIds = sMenuActionIds_Battle;
            gPyramidBagMenu->menuActionsCount = ARRAY_COUNT(sMenuActionIds_Battle);
        }
        else
        {
            gPyramidBagMenu->menuActionIds = sMenuActionIds_BattleCannotUse;
            gPyramidBagMenu->menuActionsCount = ARRAY_COUNT(sMenuActionIds_BattleCannotUse);
        }
        break;
    case PYRAMIDBAG_LOC_CHOOSE_TOSS:
        gPyramidBagMenu->menuActionIds = sMenuActionIds_ChooseToss;
        gPyramidBagMenu->menuActionsCount = ARRAY_COUNT(sMenuActionIds_ChooseToss);
        break;
    }

    CopyItemName(gSpecialVar_ItemId, gStringVar1);
    StringExpandPlaceholders(gStringVarBuffer, gText_Var1IsSelected);
    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    PyramidBagPrint(WIN_INFO, gStringVarBuffer, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
    if (gPyramidBagMenu->menuActionsCount == 1)
        PrintMenuActionText_SingleRow(OpenMenuActionWindowById(MENU_WIN_1x1));
    else if (gPyramidBagMenu->menuActionsCount == 2)
        PrintMenuActionText_SingleRow(OpenMenuActionWindowById(MENU_WIN_1x2));
    else
        PrintMenuActionText_MultiRow(OpenMenuActionWindowById(MENU_WIN_2x2), 2, 2);

    if (gPyramidBagMenu->menuActionsCount == 2 * 2) // Assumes any non 2x2 menu is single-row
        gTasks[taskId].func = HandleMenuActionInput_2x2;
    else
        gTasks[taskId].func = HandleMenuActionInput_SingleRow;
}

static void PrintMenuActionText_SingleRow(u8 windowId)
{
    PrintMenuActionTexts(windowId, FONT_NARROW, 8, 1, 0, 0x10, gPyramidBagMenu->menuActionsCount, sMenuActions, gPyramidBagMenu->menuActionIds);
    InitMenuInUpperLeftCornerNormal(windowId, gPyramidBagMenu->menuActionsCount, 0);
}

static void PrintMenuActionText_MultiRow(u8 windowId, u8 horizontalCount, u8 verticalCount)
{
    PrintMenuActionGrid(windowId, FONT_NARROW, 8, 1, 56, horizontalCount, verticalCount, sMenuActions, gPyramidBagMenu->menuActionIds);
    InitMenuActionGrid(windowId, 56, horizontalCount, verticalCount, 0);
}

static void HandleMenuActionInput_SingleRow(u8 taskId)
{
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s32 id = Menu_ProcessInputNoWrap();
        switch (id)
        {
        case MENU_NOTHING_CHOSEN:
            break;
        case MENU_B_PRESSED:
            PlaySE(SE_SELECT);
            sMenuActions[ACTION_CANCEL].func.void_u8(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            if (sMenuActions[gPyramidBagMenu->menuActionIds[id]].func.void_u8 != NULL)
                sMenuActions[gPyramidBagMenu->menuActionIds[id]].func.void_u8(taskId);
            break;
        }
    }
}

static void HandleMenuActionInput_2x2(u8 taskId)
{
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s8 id = Menu_GetCursorPos();
        if (JOY_NEW(DPAD_UP))
        {
            if (id > 0 && IsValidMenuAction(id - 2))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_UP);
            }
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (id < gPyramidBagMenu->menuActionsCount - 2 && IsValidMenuAction(id + 2))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_DOWN);
            }
        }
        else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
        {
            if (id & 1 && IsValidMenuAction(id - 1))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_LEFT, MENU_CURSOR_DELTA_NONE);
            }
        }
        else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
        {
            if (!(id & 1) && IsValidMenuAction(id + 1))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_RIGHT, MENU_CURSOR_DELTA_NONE);
            }
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (sMenuActions[gPyramidBagMenu->menuActionIds[id]].func.void_u8 != NULL)
                sMenuActions[gPyramidBagMenu->menuActionIds[id]].func.void_u8(taskId);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sMenuActions[ACTION_CANCEL].func.void_u8(taskId);
        }
    }
}

static bool8 IsValidMenuAction(s8 actionTableId)
{
    if (actionTableId < 0)
        return FALSE;
    else if (actionTableId > gPyramidBagMenu->menuActionsCount)
        return FALSE;
    else if (gPyramidBagMenu->menuActionIds[actionTableId] == ACTION_DUMMY)
        return FALSE;
    else
        return TRUE;
}

static void CloseMenuActionWindow(void)
{
    if (gPyramidBagMenu->menuActionsCount == 1)
        CloseMenuActionWindowById(MENU_WIN_1x1);
    else if (gPyramidBagMenu->menuActionsCount == 2)
        CloseMenuActionWindowById(MENU_WIN_1x2);
    else
        CloseMenuActionWindowById(MENU_WIN_2x2);
}

static void BagAction_UseOnField(u8 taskId)
{
    u8 pocketId = ItemId_GetPocket(gSpecialVar_ItemId);

    if (pocketId == POCKET_KEY_ITEMS
        || pocketId == POCKET_POKE_BALLS
        || pocketId == POCKET_TM_HM
        || ItemIsMail(gSpecialVar_ItemId) == TRUE)
    {
        CloseMenuActionWindow();
        DisplayItemMessageInBattlePyramid(taskId, gText_DadsAdvice, Task_CloseBattlePyramidBagMessage);
    }
    else if (ItemId_GetFieldFunc(gSpecialVar_ItemId) != NULL)
    {
        CloseMenuActionWindow();
        FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
        ScheduleBgCopyTilemapToVram(0);
        ItemId_GetFieldFunc(gSpecialVar_ItemId)(taskId);
    }
}

static void BagAction_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CloseMenuActionWindow();
    PrintItemDescription(tListPos);
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
    PrintSelectorArrow(tListTaskId, COLORID_DARK_GRAY);
    SetTaskToMainPyramidBagInputHandler(taskId);
}

static void SetTaskToMainPyramidBagInputHandler(u8 taskId)
{
    AddScrollArrows();
    gTasks[taskId].func = Task_HandlePyramidBagInput;
}

static void BagAction_Toss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CloseMenuActionWindow();
    tNumToToss = 1;
    if (tQuantity == 1)
    {
        AskConfirmToss(taskId);
    }
    else
    {
        CopyItemName(gSpecialVar_ItemId, gStringVar1);
        StringExpandPlaceholders(gStringVarBuffer, gText_TossHowManyVar1s);
        FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
        PyramidBagPrint(WIN_INFO, gStringVarBuffer, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
        ShowNumToToss();
        gTasks[taskId].func = Task_ChooseHowManyToToss;
    }
}

static void AskConfirmToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CopyItemName(gSpecialVar_ItemId, gStringVar1);
    ConvertIntToDecimalStringN(gStringVar2, tNumToToss, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_ConfirmTossItems);
    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    PyramidBagPrint(WIN_INFO, gStringVarBuffer, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
    CreatePyramidBagYesNo(taskId, &sYesNoTossFuncions);
}

static void DontTossItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    PrintItemDescription(tListPos);
    PrintSelectorArrow(tListTaskId, COLORID_DARK_GRAY);
    SetTaskToMainPyramidBagInputHandler(taskId);
}

static void ShowNumToToss(void)
{
    s32 x;
    ConvertIntToDecimalStringN(gStringVar1, 1, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_xVar1);
    DrawTossNumberWindow(WIN_TOSS_NUM);
    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 0x28);
    AddTextPrinterParameterized(WIN_TOSS_NUM, FONT_NORMAL, gStringVarBuffer, x, 2, 0, NULL);
}

static void UpdateNumToToss(s16 num)
{
    s32 x;
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_xVar1);
    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 0x28);
    AddTextPrinterParameterized(WIN_TOSS_NUM, FONT_NORMAL, gStringVarBuffer, x, 2, 0, NULL);
}

static void Task_ChooseHowManyToToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (AdjustQuantityAccordingToDPadInput(&tNumToToss, tQuantity) == TRUE)
    {
        UpdateNumToToss(tNumToToss);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        // Toss
        PlaySE(SE_SELECT);
        ClearStdWindowAndFrameToTransparent(WIN_TOSS_NUM, FALSE);
        ClearWindowTilemap(WIN_TOSS_NUM);
        ScheduleBgCopyTilemapToVram(1);
        AskConfirmToss(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // Cancel tossing
        PlaySE(SE_SELECT);
        ClearStdWindowAndFrameToTransparent(WIN_TOSS_NUM, FALSE);
        ClearWindowTilemap(WIN_TOSS_NUM);
        ScheduleBgCopyTilemapToVram(1);
        DontTossItem(taskId);
    }
}

static void TossItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CopyItemName(gSpecialVar_ItemId, gStringVar1);
    ConvertIntToDecimalStringN(gStringVar2, tNumToToss, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_ThrewAwayVar2Var1s);
    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    PyramidBagPrint(WIN_INFO, gStringVarBuffer, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
    gTasks[taskId].func = Task_TossItem;
}

static void Task_TossItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollOffset = &gPyramidBagMenuState.scrollPosition;
    u16 *selectedRow = &gPyramidBagMenuState.cursorPosition;

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RemovePyramidBagItem(gSpecialVar_ItemId, tNumToToss);
        DestroyListMenuTask(tListTaskId, scrollOffset, selectedRow);
        UpdatePyramidBagList();
        UpdatePyramidBagCursorPos();
        SetBagItemsListTemplate();
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollOffset, *selectedRow);
        ScheduleBgCopyTilemapToVram(0);
        SetTaskToMainPyramidBagInputHandler(taskId);
    }
}

static void BagAction_Give(u8 taskId)
{
    CloseMenuActionWindow();
    if (ItemIsMail(gSpecialVar_ItemId) == TRUE)
    {
        DisplayItemMessageInBattlePyramid(taskId, gText_CantWriteMail, Task_WaitCloseErrorMessage);
    }
    else if (!ItemId_GetImportance(gSpecialVar_ItemId))
    {
        gPyramidBagMenu->newScreenCallback = CB2_ChooseMonToGiveItem;
        CloseBattlePyramidBag(taskId);
    }
    else
    {
        ShowCantHoldMessage(taskId);
    }
}

static void ShowCantHoldMessage(u8 taskId)
{
    CopyItemName(gSpecialVar_ItemId, gStringVar1);
    StringExpandPlaceholders(gStringVarBuffer, gText_Var1CantBeHeld);
    DisplayItemMessageInBattlePyramid(taskId, gStringVarBuffer, Task_WaitCloseErrorMessage);
}

static void Task_WaitCloseErrorMessage(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        Task_CloseBattlePyramidBagMessage(taskId);
    }
}

void Task_CloseBattlePyramidBagMessage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CloseBattlePyramidBagTextWindow();
    PrintItemDescription(tListPos);
    PrintSelectorArrow(tListTaskId, COLORID_DARK_GRAY);
    SetTaskToMainPyramidBagInputHandler(taskId);
}

static void TryCloseBagToGiveItem(u8 taskId)
{
    if (!IsWritingMailAllowed(gSpecialVar_ItemId))
        DisplayItemMessageInBattlePyramid(taskId, gText_CantWriteMail, Task_WaitCloseErrorMessage);
    else if (!ItemId_GetImportance(gSpecialVar_ItemId))
        CloseBattlePyramidBag(taskId);
    else
        ShowCantHoldMessage(taskId);
}

static void BagAction_UseInBattle(u8 taskId)
{
    if (ItemId_GetBattleFunc(gSpecialVar_ItemId) != NULL)
    {
        CloseMenuActionWindow();
        ItemId_GetBattleFunc(gSpecialVar_ItemId)(taskId);
    }
}

static void Task_BeginItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tListPos = gPyramidBagMenuState.scrollPosition + gPyramidBagMenuState.cursorPosition;
    gPyramidBagMenu->toSwapPos = tListPos;
    ListMenuSetUnkIndicatorsStructField(tListTaskId, 0x10, 1);
    CopyItemName(gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode][tListPos], gStringVar1);
    StringExpandPlaceholders(gStringVarBuffer, gText_MoveVar1Where);
    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    PyramidBagPrint(WIN_INFO, gStringVarBuffer, 3, 0, 0, 1, 0, COLORID_DARK_GRAY);
    PrintSelectorArrow(tListTaskId, COLORID_LIGHT_GRAY);
    UpdateSwapLinePos(tListPos);
    gTasks[taskId].func = Task_ItemSwapHandleInput;
}

static void Task_ItemSwapHandleInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (JOY_NEW(SELECT_BUTTON))
        {
            PlaySE(SE_SELECT);
            ListMenuGetScrollAndRow(tListTaskId, &gPyramidBagMenuState.scrollPosition, &gPyramidBagMenuState.cursorPosition);
            PerformItemSwap(taskId);
        }
        else
        {
            s32 id = ListMenu_ProcessInput(tListTaskId);
            ListMenuGetScrollAndRow(tListTaskId, &gPyramidBagMenuState.scrollPosition, &gPyramidBagMenuState.cursorPosition);
            SetSwapLineInvisibility(FALSE);
            UpdateSwapLinePos(gPyramidBagMenuState.cursorPosition);
            switch (id)
            {
            case LIST_NOTHING_CHOSEN:
                break;
            case LIST_CANCEL:
                PlaySE(SE_SELECT);
                if (JOY_NEW(A_BUTTON))
                    PerformItemSwap(taskId);
                else
                    CancelItemSwap(taskId);
                break;
            default:
                PlaySE(SE_SELECT);
                PerformItemSwap(taskId);
                break;
            }
        }
    }
}

static void PerformItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollOffset = &gPyramidBagMenuState.scrollPosition;
    u16 *selectedRow = &gPyramidBagMenuState.cursorPosition;
    u16 swapPos = *scrollOffset + *selectedRow;

    if (tListPos == swapPos || tListPos == swapPos - 1)
    {
        CancelItemSwap(taskId);
    }
    else
    {
        MovePyramidBagItemSlotInList(tListPos, swapPos);
        gPyramidBagMenu->toSwapPos = POS_NONE;
        SetSwapLineInvisibility(TRUE);
        DestroyListMenuTask(tListTaskId, scrollOffset, selectedRow);
        if (tListPos < swapPos)
            gPyramidBagMenuState.cursorPosition--;
        SetBagItemsListTemplate();
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollOffset, *selectedRow);
        SetTaskToMainPyramidBagInputHandler(taskId);
    }
}

static void CancelItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollOffset = &gPyramidBagMenuState.scrollPosition;
    u16 *selectedRow = &gPyramidBagMenuState.cursorPosition;

    gPyramidBagMenu->toSwapPos = POS_NONE;
    SetSwapLineInvisibility(TRUE);
    DestroyListMenuTask(tListTaskId, scrollOffset, selectedRow);
    if (tListPos < *scrollOffset + *selectedRow)
        gPyramidBagMenuState.cursorPosition--;
    SetBagItemsListTemplate();
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollOffset, *selectedRow);
    SetTaskToMainPyramidBagInputHandler(taskId);
}

void TryStoreHeldItemsInPyramidBag(void)
{
    u8 i;
    struct Pokemon *party = gPlayerParty;
    u16 *newItems = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
    u8 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
    u16 heldItem;

    memcpy(newItems, gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode], PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
    memcpy(newQuantities, gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode], PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        heldItem = GetMonData(&party[i], MON_DATA_HELD_ITEM);
        if (heldItem != ITEM_NONE && !AddBagItem(heldItem, 1))
        {
            // Cant store party held items in pyramid bag because bag is full
            memcpy(gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode], newItems, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
            memcpy(gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode], newQuantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
            Free(newItems);
            Free(newQuantities);
            gSpecialVar_Result = 1;
            return;
        }
    }

    heldItem = ITEM_NONE;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        SetMonData(&party[i], MON_DATA_HELD_ITEM, &heldItem);
    gSpecialVar_Result = 0;
    Free(newItems);
    Free(newQuantities);
}

static void InitPyramidBagWindows(void)
{
    u8 i;

    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x1, BG_PLTT_ID(14));
    LoadMessageBoxGfx(0, 0xA, BG_PLTT_ID(13));
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);

    for (i = 0; i < ARRAY_COUNT(sWindowTemplates); i++)
        FillWindowPixelBuffer(i, PIXEL_FILL(0));

    PutWindowTilemap(WIN_LIST);
    PutWindowTilemap(WIN_INFO);
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
}

static void PyramidBagPrint(u8 windowId, const u8 *src, u8 x, u8 y, u8 letterSpacing, u8 lineSpacing, u8 speed, u8 colorTableId)
{
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, letterSpacing, lineSpacing, sTextColors[colorTableId], speed, src);
}

static void PyramidBagPrint_Quantity(u8 windowId, const u8 *src, u8 x, u8 y, u8 letterSpacing, u8 lineSpacing, u8 speed, u8 colorTableId)
{
    AddTextPrinterParameterized4(windowId, FONT_NARROW, x, y, letterSpacing, lineSpacing, sTextColors[colorTableId], speed, src);
}

static void DrawTossNumberWindow(u8 windowId)
{
    DrawStdFrameWithCustomTileAndPalette(windowId, FALSE, 1, 0xE);
    ScheduleBgCopyTilemapToVram(1);
}

static u8 UNUSED GetMenuActionWindowId(u8 windowArrayId)
{
    return gPyramidBagMenu->windowIds[windowArrayId];
}

static u8 OpenMenuActionWindowById(u8 windowArrayId)
{
    u8 *windowId = &gPyramidBagMenu->windowIds[windowArrayId];
    if (*windowId == WINDOW_NONE)
    {
        *windowId = AddWindow(&sWindowTemplates_MenuActions[windowArrayId]);
        DrawStdFrameWithCustomTileAndPalette(*windowId, FALSE, 1, 0xE);
        ScheduleBgCopyTilemapToVram(1);
    }
    return *windowId;
}

static void CloseMenuActionWindowById(u8 windowArrayId)
{
    u8 *windowId = &gPyramidBagMenu->windowIds[windowArrayId];
    if (*windowId != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(*windowId, FALSE);
        ClearWindowTilemap(*windowId);
        RemoveWindow(*windowId);
        ScheduleBgCopyTilemapToVram(1);
        *windowId = WINDOW_NONE;
    }
}

static void CreatePyramidBagYesNo(u8 taskId, const struct YesNoFuncTable *yesNoTable)
{
    CreateYesNoMenuWithCallbacks(taskId, &sWindowTemplates_MenuActions[MENU_WIN_YESNO], 1, 0, 2, 1, 0xE, yesNoTable);
}

void DisplayItemMessageInBattlePyramid(u8 taskId, const u8 *str, void (*callback)(u8 taskId))
{
    FillWindowPixelBuffer(WIN_MSG, PIXEL_FILL(1));
    DisplayMessageAndContinueTask(taskId, WIN_MSG, 0xA, 0xD, FONT_NORMAL, GetPlayerTextSpeedDelay(), str, callback);
    ScheduleBgCopyTilemapToVram(1);
}

static void CloseBattlePyramidBagTextWindow(void)
{
    ClearDialogWindowAndFrameToTransparent(WIN_MSG, FALSE);
    // This ClearWindowTilemap call is redundant, since ClearDialogWindowAndFrameToTransparent already calls it.
    ClearWindowTilemap(WIN_MSG);
    ScheduleBgCopyTilemapToVram(1);
}

static void FreeItemIconSprite(u8 spriteArrId)
{
    u8 *spriteId = &gPyramidBagMenu->spriteIds[spriteArrId];
    if (*spriteId != SPRITE_NONE)
    {
        // spriteArrId is PBAG_SPRITE_ITEM_ICON / PBAG_SPRITE_ITEM_ICON_ALT here (1-2)
        // so tag will be TAG_ITEM_ICON / TAG_ITEM_ICON_ALT
        FreeSpriteTilesByTag(TAG_ITEM_ICON - 1 + spriteArrId);
        FreeSpritePaletteByTag(TAG_ITEM_ICON - 1 + spriteArrId);
        FreeSpriteOamMatrix(&gSprites[*spriteId]);
        DestroySprite(&gSprites[*spriteId]);
        *spriteId = SPRITE_NONE;
    }
}

static void LoadPyramidBagPalette(void)
{
    struct SpritePalette spritePalette;
    u16 *palPtr = Alloc(2 * PLTT_SIZE_4BPP);

    LZDecompressWram(gBattlePyramidBag_Pal, palPtr);
    spritePalette.data = palPtr + PLTT_ID(gSaveBlock2Ptr->frontier.lvlMode);
    spritePalette.tag = TAG_PYRAMID_BAG;
    LoadSpritePalette(&spritePalette);
    Free(palPtr);
}

static void CreatePyramidBagSprite(void)
{
    u8 *spriteId = &gPyramidBagMenu->spriteIds[PBAG_SPRITE_BAG];
    *spriteId = CreateSprite(&sSpriteTemplate_PyramidBag, 68, 56, 0);
}

static void ShakePyramidBag(void)
{
    struct Sprite *sprite = &gSprites[gPyramidBagMenu->spriteIds[PBAG_SPRITE_BAG]];
    if (sprite->affineAnimEnded)
    {
        StartSpriteAffineAnim(sprite, ANIM_BAG_SHAKE);
        sprite->callback = SpriteCB_BagWaitForShake;
    }
}

static void SpriteCB_BagWaitForShake(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
    {
        StartSpriteAffineAnim(sprite, ANIM_BAG_STILL);
        sprite->callback = SpriteCallbackDummy;
    }
}

static void ShowItemIcon(u16 itemId, bool8 isAlt)
{
    u8 itemSpriteId;
    u8 *spriteId = &gPyramidBagMenu->spriteIds[isAlt + PBAG_SPRITE_ITEM_ICON];
    if (*spriteId == SPRITE_NONE)
    {
        FreeSpriteTilesByTag(TAG_ITEM_ICON + isAlt);
        FreeSpritePaletteByTag(TAG_ITEM_ICON + isAlt);
        itemSpriteId = AddItemIconSprite(TAG_ITEM_ICON + isAlt, TAG_ITEM_ICON + isAlt, itemId);
        if (itemSpriteId != MAX_SPRITES)
        {
            *spriteId = itemSpriteId;
            gSprites[itemSpriteId].x2 = 24;
            gSprites[itemSpriteId].y2 = 88;
        }
    }
}

static void FreeItemIconSpriteByAltId(bool8 isAlt)
{
    FreeItemIconSprite(isAlt + PBAG_SPRITE_ITEM_ICON);
}

static void CreateSwapLine(void)
{
    CreateSwapLineSprites(&gPyramidBagMenu->spriteIds[PBAG_SPRITE_SWAP_LINE_START], NUM_SWAP_LINE_SPRITES);
}

static void SetSwapLineInvisibility(bool8 invisible)
{
    SetSwapLineSpritesInvisibility(&gPyramidBagMenu->spriteIds[PBAG_SPRITE_SWAP_LINE_START], NUM_SWAP_LINE_SPRITES, invisible);
}

static void UpdateSwapLinePos(u8 y)
{
    UpdateSwapLineSpritesPos(&gPyramidBagMenu->spriteIds[PBAG_SPRITE_SWAP_LINE_START], NUM_SWAP_LINE_SPRITES | SWAP_LINE_HAS_MARGIN, 120, (y + 1) * 16);
}
