#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "item_menu.h"
#include "lilycove_lady.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "pokeblock.h"
#include "pokemon.h"
#include "safari_zone.h"
#include "scanline_effect.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/rgb.h"

#define MAX_MENU_ITEMS 9
#define MENU_MIDPOINT (MAX_MENU_ITEMS / 2)

#define TILE_HIGHLIGHT_NONE 0x0005 // Tile number for the bg of an unselected menu item
#define TILE_HIGHLIGHT_BLUE 0x1005 // Tile number for the bg of a selected menu item
#define TILE_HIGHLIGHT_RED  0x2005 // Tile number for the bg of a menu item to swap

#define TAG_POKEBLOCK_CASE  14800
#define TAG_SCROLL_ARROW    1110

#define POKEBLOCK_MAX_FEEL 99

enum {
    WIN_TITLE,
    WIN_LIST,
    WIN_SPICY,
    WIN_DRY,
    WIN_SWEET,
    WIN_BITTER,
    WIN_SOUR,
    WIN_FEEL,
    WIN_ACTIONS_TALL,
    WIN_ACTIONS,
    WIN_TOSS_MSG,
};

struct PokeblockMenuStruct
{
    u8 tilemap[BG_SCREEN_SIZE];
    void (*callbackOnUse)(void);
    const u8 *pokeblockActionIds;
    u8 numActions;
    u8 caseId;
    u8 itemsNo;
    u8 maxShowed;
    struct ListMenuItem items[POKEBLOCKS_COUNT + 1];
    u8 menuItemsStrings[POKEBLOCKS_COUNT + 1][32]; // + 1 because of STOW CASE item
    u8 pokeblockCaseSpriteId;
    u8 swapLineSpriteIds[7];
    u8 arrowTaskId;
    bool8 isSwapping;
    s16 gfxState;
    u8 unused[8];
};

struct PokeblockSavedData
{
    void (*callback)(void);
    u16 selectedRow;
    u16 scrollOffset;
};

enum
{
    PKBL_USE_ON_FIELD,
    PKBL_TOSS,
    PKBL_CANCEL,
    PKBL_USE_IN_BATTLE,
    PKBL_USE_ON_FEEDER,
    PKBL_GIVE_TO_LADY
};

static void CB2_InitPokeblockMenu(void);
static bool8 InitPokeblockMenu(void);
static bool8 LoadPokeblockMenuGfx(void);
static void HandleInitBackgrounds(void);
static void HandleInitWindows(void);
static void SetMenuItemsCountAndMaxShowed(void);
static void LimitMenuScrollAndRow(void);
static void SetInitialScroll(void);
static void UpdatePokeblockList(void);
static void CreateScrollArrows(void);
static void MovePokeblockMenuCursor(s32, bool8, struct ListMenu *);
static void DrawPokeblockMenuTitleText(void);
static void DrawPokeblockMenuHighlight(u16, u16);
static void PutPokeblockListMenuString(u8 *, u16);
static void Task_HandlePokeblockMenuInput(u8);
static void PokeblockAction_UseOnField(u8);
static void PokeblockAction_Toss(u8);
static void PokeblockAction_Cancel(u8);
static void PokeblockAction_UseInBattle(u8);
static void PokeblockAction_UseOnPokeblockFeeder(u8);
static void PokeblockAction_GiveToContestLady(u8);
static void TossedPokeblockMessage(u8);
static void CloseTossPokeblockWindow(u8);
static void Task_FreeDataAndExitPokeblockCase(u8);
static void Task_HandlePokeblockActionsInput(u8);
static void ShowPokeblockActionsWindow(u8);
static void Task_HandlePokeblocksSwapInput(u8);
static void SpriteCB_ShakePokeblockCase(struct Sprite *);
static void DrawPokeblockInfo(s32);
static void UpdatePokeblockSwapMenu(u8, bool8);
static void UsePokeblockOnField(void);
static void ReturnToPokeblockCaseOnField(void);
static void CreateTossPokeblockYesNoMenu(u8);
static void TossPokeblock(u8);

EWRAM_DATA static struct PokeblockSavedData sSavedPokeblockData = {0};
EWRAM_DATA static struct PokeblockMenuStruct *sPokeblockMenu = NULL;

const s8 gPokeblockFlavorCompatibilityTable[NUM_NATURES * FLAVOR_COUNT] =
{
     // Spicy,  Dry, Sweet, Bitter, Sour
          0,      0,    0,     0,     0, // Hardy
          1,      0,    0,     0,    -1, // Lonely
          1,      0,   -1,     0,     0, // Brave
          1,     -1,    0,     0,     0, // Adamant
          1,      0,    0,    -1,     0, // Naughty
         -1,      0,    0,     0,     1, // Bold
          0,      0,    0,     0,     0, // Docile
          0,      0,   -1,     0,     1, // Relaxed
          0,     -1,    0,     0,     1, // Impish
          0,      0,    0,    -1,     1, // Lax
         -1,      0,    1,     0,     0, // Timid
          0,      0,    1,     0,    -1, // Hasty
          0,      0,    0,     0,     0, // Serious
          0,     -1,    1,     0,     0, // Jolly
          0,      0,    1,    -1,     0, // Naive
         -1,      1,    0,     0,     0, // Modest
          0,      1,    0,     0,    -1, // Mild
          0,      1,   -1,     0,     0, // Quiet
          0,      0,    0,     0,     0, // Bashful
          0,      1,    0,    -1,     0, // Rash
         -1,      0,    0,     1,     0, // Calm
          0,      0,    0,     1,    -1, // Gentle
          0,      0,   -1,     1,     0, // Sassy
          0,     -1,    0,     1,     0, // Careful
          0,      0,    0,     0,     0  // Quirky
};

static const struct BgTemplate sBgTemplatesForPokeblockMenu[] =
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
    }
};

const u8 *const gPokeblockNames[] =
{
    [PBLOCK_CLR_NONE]      = NULL,
    [PBLOCK_CLR_RED]       = gText_RedPokeblock,
    [PBLOCK_CLR_BLUE]      = gText_BluePokeblock,
    [PBLOCK_CLR_PINK]      = gText_PinkPokeblock,
    [PBLOCK_CLR_GREEN]     = gText_GreenPokeblock,
    [PBLOCK_CLR_YELLOW]    = gText_YellowPokeblock,
    [PBLOCK_CLR_PURPLE]    = gText_PurplePokeblock,
    [PBLOCK_CLR_INDIGO]    = gText_IndigoPokeblock,
    [PBLOCK_CLR_BROWN]     = gText_BrownPokeblock,
    [PBLOCK_CLR_LITE_BLUE] = gText_LiteBluePokeblock,
    [PBLOCK_CLR_OLIVE]     = gText_OlivePokeblock,
    [PBLOCK_CLR_GRAY]      = gText_GrayPokeblock,
    [PBLOCK_CLR_BLACK]     = gText_BlackPokeblock,
    [PBLOCK_CLR_WHITE]     = gText_WhitePokeblock,
    [PBLOCK_CLR_GOLD]      = gText_GoldPokeblock
};

static const struct MenuAction sPokeblockMenuActions[] =
{
    [PKBL_USE_ON_FIELD]  = {gMenuText_Use, {PokeblockAction_UseOnField}},
    [PKBL_TOSS]          = {gMenuText_Toss, {PokeblockAction_Toss}},
    [PKBL_CANCEL]        = {gText_Cancel2, {PokeblockAction_Cancel}},
    [PKBL_USE_IN_BATTLE] = {gMenuText_Use, {PokeblockAction_UseInBattle}},
    [PKBL_USE_ON_FEEDER] = {gMenuText_Use, {PokeblockAction_UseOnPokeblockFeeder}},
    [PKBL_GIVE_TO_LADY]  = {gMenuText_Give2, {PokeblockAction_GiveToContestLady}},
};

static const u8 sActionsOnField[] = {PKBL_USE_ON_FIELD, PKBL_TOSS, PKBL_CANCEL};
static const u8 sActionsInBattle[] = {PKBL_USE_IN_BATTLE, PKBL_CANCEL};
static const u8 sActionsOnPokeblockFeeder[] = {PKBL_USE_ON_FEEDER, PKBL_CANCEL};
static const u8 sActionsWhenGivingToLady[] = {PKBL_GIVE_TO_LADY, PKBL_CANCEL};

static const struct YesNoFuncTable sTossYesNoFuncTable = {TossedPokeblockMessage, CloseTossPokeblockWindow};

static const u8 sContestStatsMonData[] = {MON_DATA_COOL, MON_DATA_BEAUTY, MON_DATA_CUTE, MON_DATA_SMART, MON_DATA_TOUGH};

static const struct OamData sOamData_PokeblockCase =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_PokeblockCase[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_PokeblockCase[] =
{
    sSpriteAnim_PokeblockCase
};

static const union AffineAnimCmd sAffineAnim_PokeblockCaseShake[] =
{
    AFFINEANIMCMD_FRAME(0, 0, -2,  2),
    AFFINEANIMCMD_FRAME(0, 0,  2,  4),
    AFFINEANIMCMD_FRAME(0, 0, -2,  4),
    AFFINEANIMCMD_FRAME(0, 0,  2,  2),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_PokeblockCaseShake[] =
{
    sAffineAnim_PokeblockCaseShake
};

const struct CompressedSpriteSheet gPokeblockCase_SpriteSheet =
{
    gMenuPokeblockDevice_Gfx, 0x800, TAG_POKEBLOCK_CASE
};

const struct CompressedSpritePalette gPokeblockCase_SpritePal =
{
    gMenuPokeblockDevice_Pal, TAG_POKEBLOCK_CASE
};

static const struct SpriteTemplate sSpriteTemplate_PokeblockCase =
{
    .tileTag = TAG_POKEBLOCK_CASE,
    .paletteTag = TAG_POKEBLOCK_CASE,
    .oam = &sOamData_PokeblockCase,
    .anims = sSpriteAnimTable_PokeblockCase,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const u8 sTextColor[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

static const struct Pokeblock sFavoritePokeblocksTable[FLAVOR_COUNT] =
{
    [FLAVOR_SPICY]  = { PBLOCK_CLR_RED,    20,  0,  0,  0,  0, 20},
    [FLAVOR_DRY]    = { PBLOCK_CLR_BLUE,    0, 20,  0,  0,  0, 20},
    [FLAVOR_SWEET]  = { PBLOCK_CLR_PINK,    0,  0, 20,  0,  0, 20},
    [FLAVOR_BITTER] = { PBLOCK_CLR_GREEN,   0,  0,  0, 20,  0, 20},
    [FLAVOR_SOUR]   = { PBLOCK_CLR_YELLOW,  0,  0,  0,  0, 20, 20}
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 9,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x1E
    },
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 15,
        .tilemapTop = 1,
        .width = 14,
        .height = 18,
        .paletteNum = 15,
        .baseBlock = 0x30
    },
    [WIN_SPICY] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 13,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x12C
    },
    [WIN_DRY] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x136
    },
    [WIN_SWEET] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 17,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x140
    },
    [WIN_BITTER] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 13,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x14A
    },
    [WIN_SOUR] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 15,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x154
    },
    [WIN_FEEL] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 17,
        .width = 2,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x15E
    },
    [WIN_ACTIONS_TALL] = {
        .bg = 1,
        .tilemapLeft = 7,
        .tilemapTop = 5,
        .width = 6,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 0x162
    },
    [WIN_ACTIONS] = {
        .bg = 1,
        .tilemapLeft = 7,
        .tilemapTop = 7,
        .width = 6,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x186
    },
    [WIN_TOSS_MSG] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x19E
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sTossPkblockWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x20A
};

static const struct ListMenuTemplate sPokeblockListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = MovePokeblockMenuCursor,
    .itemPrintFunc = NULL,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = WIN_LIST,
    .header_X = 0,
    .item_X = 1,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_MULTIPLE_SCROLL_DPAD,
    .fontId = FONT_NORMAL,
    .cursorKind = CURSOR_INVISIBLE
};

void OpenPokeblockCase(u8 caseId, void (*callback)(void))
{
    sPokeblockMenu = Alloc(sizeof(*sPokeblockMenu));
    sPokeblockMenu->caseId = caseId;
    sPokeblockMenu->callbackOnUse = NULL;
    sPokeblockMenu->arrowTaskId = TASK_NONE;
    sPokeblockMenu->isSwapping = FALSE;
    sSavedPokeblockData.callback = callback;

    switch (sPokeblockMenu->caseId)
    {
    case PBLOCK_CASE_BATTLE:
        sPokeblockMenu->pokeblockActionIds = sActionsInBattle;
        sPokeblockMenu->numActions = ARRAY_COUNT(sActionsInBattle);
        break;
    case PBLOCK_CASE_FEEDER:
        sPokeblockMenu->pokeblockActionIds = sActionsOnPokeblockFeeder;
        sPokeblockMenu->numActions = ARRAY_COUNT(sActionsOnPokeblockFeeder);
        break;
    case PBLOCK_CASE_GIVE:
        sPokeblockMenu->pokeblockActionIds = sActionsWhenGivingToLady;
        sPokeblockMenu->numActions = ARRAY_COUNT(sActionsWhenGivingToLady);
        break;
    default: // PBLOCK_CASE_FIELD
        sPokeblockMenu->pokeblockActionIds = sActionsOnField;
        sPokeblockMenu->numActions = ARRAY_COUNT(sActionsOnField);
        break;
    }

    SetMainCallback2(CB2_InitPokeblockMenu);
}

void OpenPokeblockCaseInBattle(void)
{
    OpenPokeblockCase(PBLOCK_CASE_BATTLE, CB2_SetUpReshowBattleScreenAfterMenu2);
}

void OpenPokeblockCaseOnFeeder(void)
{
    OpenPokeblockCase(PBLOCK_CASE_FEEDER, CB2_ReturnToField);
}

static void CB2_PokeblockMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_PokeblockMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_InitPokeblockMenu(void)
{
    while (1)
    {
        if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE)
            break;
        if (InitPokeblockMenu() == TRUE)
            break;
        if (MenuHelpers_IsLinkActive() == TRUE)
            break;
    }
}

#define tListTaskId data[0]
#define tWindowId   data[1]
#define tToSwapId   data[2]

static bool8 InitPokeblockMenu(void)
{
    u8 taskId;

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
        if (sPokeblockMenu->caseId != PBLOCK_CASE_BATTLE)
            ResetTasks();
        gMain.state++;
        break;
    case 6:
        HandleInitBackgrounds();
        sPokeblockMenu->gfxState = 0;
        gMain.state++;
        break;
    case 7:
        if (!LoadPokeblockMenuGfx())
            return FALSE;
        gMain.state++;
        break;
    case 8:
        SetMenuItemsCountAndMaxShowed();
        LimitMenuScrollAndRow();
        SetInitialScroll();
        gMain.state++;
        break;
    case 9:
        sPokeblockMenu->pokeblockCaseSpriteId = CreatePokeblockCaseSprite(56, 64, 0);
        gMain.state++;
        break;
    case 10:
        CreateSwapLineSprites(sPokeblockMenu->swapLineSpriteIds, ARRAY_COUNT(sPokeblockMenu->swapLineSpriteIds));
        gMain.state++;
        break;
    case 11:
        DrawPokeblockMenuHighlight(sSavedPokeblockData.selectedRow, TILE_HIGHLIGHT_BLUE);
        gMain.state++;
        break;
    case 12:
        HandleInitWindows();
        gMain.state++;
        break;
    case 13:
        UpdatePokeblockList();
        gMain.state++;
        break;
    case 14:
        CreateScrollArrows();
        gMain.state++;
        break;
    case 15:
        taskId = CreateTask(Task_HandlePokeblockMenuInput, 0);
        gTasks[taskId].tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, sSavedPokeblockData.scrollOffset, sSavedPokeblockData.selectedRow);
        gMain.state++;
        break;
    case 16:
        DrawPokeblockMenuTitleText();
        gMain.state++;
        break;
    case 17:
        BlendPalettes(PALETTES_ALL, 16, 0);
        gMain.state++;
        break;
    case 18:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = FALSE;
        gMain.state++;
        break;
    default:
        SetVBlankCallback(VBlankCB_PokeblockMenu);
        SetMainCallback2(CB2_PokeblockMenu);
        return TRUE;
    }

    return FALSE;
}

static void HandleInitBackgrounds(void)
{
    ResetVramOamAndBgCntRegs();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplatesForPokeblockMenu, ARRAY_COUNT(sBgTemplatesForPokeblockMenu));
    SetBgTilemapBuffer(2, sPokeblockMenu->tilemap);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(2);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);

    SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

static bool8 LoadPokeblockMenuGfx(void)
{
    switch (sPokeblockMenu->gfxState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(2, gMenuPokeblock_Gfx, 0, 0, 0);
        sPokeblockMenu->gfxState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(gMenuPokeblock_Tilemap, sPokeblockMenu->tilemap);
            sPokeblockMenu->gfxState++;
        }
        break;
    case 2:
        LoadCompressedPalette(gMenuPokeblock_Pal, BG_PLTT_ID(0), 6 * PLTT_SIZE_4BPP);
        sPokeblockMenu->gfxState++;
        break;
    case 3:
        LoadCompressedSpriteSheet(&gPokeblockCase_SpriteSheet);
        sPokeblockMenu->gfxState++;
        break;
    case 4:
        LoadCompressedSpritePalette(&gPokeblockCase_SpritePal);
        sPokeblockMenu->gfxState++;
        break;
    case 5:
        LoadListMenuSwapLineGfx();
        sPokeblockMenu->gfxState = 0;
        return TRUE;
    }

    return FALSE;
}

static void HandleInitWindows(void)
{
    u8 i;

    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 1, BG_PLTT_ID(14));
    LoadMessageBoxGfx(0, 0xA, BG_PLTT_ID(13));
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);

    for (i = 0; i < ARRAY_COUNT(sWindowTemplates) - 1; i++)
        FillWindowPixelBuffer(i, PIXEL_FILL(0));

    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
}

static void PrintOnPokeblockWindow(u8 windowId, const u8 *string, s32 x)
{
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, 1, 0, 0, sTextColor, 0, string);
}

static void DrawPokeblockMenuTitleText(void)
{
    u8 i;

    const u8 *itemName = ItemId_GetName(ITEM_POKEBLOCK_CASE);
    PrintOnPokeblockWindow(WIN_TITLE, itemName, GetStringCenterAlignXOffset(FONT_NORMAL, itemName, 0x48));

    PrintOnPokeblockWindow(WIN_SPICY,  gText_Spicy, 0);
    PrintOnPokeblockWindow(WIN_DRY,    gText_Dry, 0);
    PrintOnPokeblockWindow(WIN_SWEET,  gText_Sweet, 0);
    PrintOnPokeblockWindow(WIN_BITTER, gText_Bitter, 0);
    PrintOnPokeblockWindow(WIN_SOUR,   gText_Sour, 0);

    for (i = 0; i < WIN_ACTIONS_TALL; i++)
        PutWindowTilemap(i);
}

static void UpdatePokeblockList(void)
{
    u16 i;

    for (i = 0; i < sPokeblockMenu->itemsNo - 1; i++)
    {
        PutPokeblockListMenuString(sPokeblockMenu->menuItemsStrings[i], i);
        sPokeblockMenu->items[i].name = sPokeblockMenu->menuItemsStrings[i];
        sPokeblockMenu->items[i].id = i;
    }

    StringCopy(sPokeblockMenu->menuItemsStrings[i], gText_StowCase);
    sPokeblockMenu->items[i].name = sPokeblockMenu->menuItemsStrings[i];
    sPokeblockMenu->items[i].id = LIST_CANCEL;

    gMultiuseListMenuTemplate = sPokeblockListMenuTemplate;
    gMultiuseListMenuTemplate.fontId = FONT_NARROW;
    gMultiuseListMenuTemplate.totalItems = sPokeblockMenu->itemsNo;
    gMultiuseListMenuTemplate.items = sPokeblockMenu->items;
    gMultiuseListMenuTemplate.maxShowed = sPokeblockMenu->maxShowed;
}

static void PutPokeblockListMenuString(u8 *dst, u16 pkblId)
{
    struct Pokeblock *pkblock = &gSaveBlock1Ptr->pokeblocks[pkblId];
    u8 *txtPtr = StringCopy(dst, gPokeblockNames[pkblock->color]);

    *(txtPtr++) = EXT_CTRL_CODE_BEGIN;
    *(txtPtr++) = EXT_CTRL_CODE_SKIP;
    *(txtPtr++) = CHAR_BLOCK_1;

    ConvertIntToDecimalStringN(gStringVar1, GetHighestPokeblocksFlavorLevel(pkblock), STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(txtPtr, gText_LvVar1);
}

static void MovePokeblockMenuCursor(s32 pkblId, bool8 onInit, struct ListMenu *list)
{
    if (onInit != TRUE)
    {
        PlaySE(SE_SELECT);
        gSprites[sPokeblockMenu->pokeblockCaseSpriteId].callback = SpriteCB_ShakePokeblockCase;
    }

    if (!sPokeblockMenu->isSwapping)
        DrawPokeblockInfo(pkblId);
}

static void DrawPokeblockInfo(s32 pkblId)
{
    u8 i;
    struct Pokeblock *pokeblock;
    u16 rectTilemapSrc[2];

    FillWindowPixelBuffer(WIN_FEEL, PIXEL_FILL(0));

    if (pkblId != LIST_CANCEL)
    {
        pokeblock = &gSaveBlock1Ptr->pokeblocks[pkblId];
        rectTilemapSrc[0] = 0x17;
        rectTilemapSrc[1] = 0x18;
        for (i = 0; i < FLAVOR_COUNT; i++)
        {
            if (GetPokeblockData(pokeblock, PBLOCK_SPICY + i) > 0)
            {
                // Pokéblock has this flavor, draw Pokéblock icon next to it
                rectTilemapSrc[0] = (i << 12) + 0x17;
                rectTilemapSrc[1] = (i << 12) + 0x18;
            }
            else
            {
                // Pokéblock doesn't have this flavor, draw regular tiles
                rectTilemapSrc[0] = 0xF;
                rectTilemapSrc[1] = 0xF;
            }
            CopyToBgTilemapBufferRect(2, rectTilemapSrc, (i / 3 * 6) + 1, (i % 3 * 2) + 13, 1, 2);
        }

        // Print the Pokéblock's feel
        ConvertIntToDecimalStringN(gStringVar1, GetPokeblocksFeel(pokeblock), STR_CONV_MODE_RIGHT_ALIGN, 2);
        PrintOnPokeblockWindow(WIN_FEEL, gStringVar1, 4);
    }
    else
    {
        // Selected cancel, erase info
        rectTilemapSrc[0] = 0xF;
        rectTilemapSrc[1] = 0xF;

        for (i = 0; i < FLAVOR_COUNT; i++)
            CopyToBgTilemapBufferRect(2, rectTilemapSrc, (i / 3 * 6) + 1, (i % 3 * 2) + 13, 1, 2);

        CopyWindowToVram(WIN_FEEL, COPYWIN_GFX);
    }

    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(2);
}

static void DrawPokeblockMenuHighlight(u16 cursorPos, u16 tileNum)
{
    FillBgTilemapBufferRect_Palette0(2, tileNum, 0xF, (cursorPos * 2) + 1, 0xE, 2);
    ScheduleBgCopyTilemapToVram(2);
}

static void CompactPokeblockSlots(void)
{
    u16 i, j;

    for (i = 0; i < POKEBLOCKS_COUNT - 1; i++)
    {
        for (j = i + 1; j < POKEBLOCKS_COUNT; j++)
        {
            if (gSaveBlock1Ptr->pokeblocks[i].color == PBLOCK_CLR_NONE)
            {
                struct Pokeblock temp = gSaveBlock1Ptr->pokeblocks[i];
                gSaveBlock1Ptr->pokeblocks[i] = gSaveBlock1Ptr->pokeblocks[j];
                gSaveBlock1Ptr->pokeblocks[j] = temp;
            }
        }
    }
}

static void SwapPokeblockMenuItems(u32 id1, u32 id2)
{
    s16 i, count;
    struct Pokeblock *pokeblocks = gSaveBlock1Ptr->pokeblocks;
    struct Pokeblock *copyPokeblock1;

    if (id1 == id2)
        return;

    copyPokeblock1 = Alloc(sizeof(struct Pokeblock));
    *copyPokeblock1 = pokeblocks[id1];

    if (id2 > id1)
    {
        id2--;
        for (count = id2, i = id1; i < count; i++)
            pokeblocks[i] = pokeblocks[i + 1];
    }
    else
    {
        for (count = id2, i = id1; i > count; i--)
            pokeblocks[i] = pokeblocks[i - 1];
    }

    pokeblocks[id2] = *copyPokeblock1;
    Free(copyPokeblock1);
}

void ResetPokeblockScrollPositions(void)
{
    sSavedPokeblockData.selectedRow = 0;
    sSavedPokeblockData.scrollOffset = 0;
}

static void SetMenuItemsCountAndMaxShowed(void)
{
    u16 i;

    CompactPokeblockSlots();

    for (sPokeblockMenu->itemsNo = 0, i = 0; i < POKEBLOCKS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->pokeblocks[i].color != PBLOCK_CLR_NONE)
            sPokeblockMenu->itemsNo++;
    }

    sPokeblockMenu->itemsNo++; // STOW CASE menu item

    if (sPokeblockMenu->itemsNo > MAX_MENU_ITEMS)
        sPokeblockMenu->maxShowed = MAX_MENU_ITEMS;
    else
        sPokeblockMenu->maxShowed = sPokeblockMenu->itemsNo;
}

static void LimitMenuScrollAndRow(void)
{
    if (sSavedPokeblockData.scrollOffset != 0)
    {
        if (sSavedPokeblockData.scrollOffset + sPokeblockMenu->maxShowed > sPokeblockMenu->itemsNo)
            sSavedPokeblockData.scrollOffset = sPokeblockMenu->itemsNo - sPokeblockMenu->maxShowed;
    }

    if (sSavedPokeblockData.scrollOffset + sSavedPokeblockData.selectedRow >= sPokeblockMenu->itemsNo)
    {
        if (sPokeblockMenu->itemsNo == 0)
            sSavedPokeblockData.selectedRow = 0;
        else
            sSavedPokeblockData.selectedRow = sPokeblockMenu->itemsNo - 1;
    }
}

static void SetInitialScroll(void)
{
    if (sSavedPokeblockData.selectedRow > MENU_MIDPOINT)
    {
        u8 i;

        for (i = 0;
             (i < sSavedPokeblockData.selectedRow - MENU_MIDPOINT) && (sSavedPokeblockData.scrollOffset + sPokeblockMenu->maxShowed != sPokeblockMenu->itemsNo);
             sSavedPokeblockData.selectedRow--, sSavedPokeblockData.scrollOffset++, i++);
    }
}

static void CreateScrollArrows(void)
{
    if (sPokeblockMenu->arrowTaskId == TASK_NONE)
    {
        sPokeblockMenu->arrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 0xB0, 8, 0x98, sPokeblockMenu->itemsNo - sPokeblockMenu->maxShowed,
                                                                               TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &sSavedPokeblockData.scrollOffset);
    }
}

static void DestroyScrollArrows(void)
{
    if (sPokeblockMenu->arrowTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(sPokeblockMenu->arrowTaskId);
        sPokeblockMenu->arrowTaskId = TASK_NONE;
    }
}

u8 CreatePokeblockCaseSprite(s16 x, s16 y, u8 subpriority)
{
    return CreateSprite(&sSpriteTemplate_PokeblockCase, x, y, subpriority);
}

#define sState data[0]
#define sTimer data[1]

static void SpriteCB_ShakePokeblockCase(struct Sprite *sprite)
{
    if (sprite->sState > 1)
        sprite->sState = 0;

    switch (sprite->sState)
    {
    case 0:
        sprite->oam.affineMode = ST_OAM_AFFINE_NORMAL;
        sprite->affineAnims = sAffineAnims_PokeblockCaseShake;
        InitSpriteAffineAnim(sprite);
        sprite->sState = 1;
        sprite->sTimer = 0;
        break;
    case 1:
        if (++sprite->sTimer > 11)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            sprite->sState = 0;
            sprite->sTimer = 0;
            FreeOamMatrix(sprite->oam.matrixNum);
            sprite->callback = SpriteCallbackDummy;
        }
        break;
    }
}

static void FadePaletteAndSetTaskToClosePokeblockCase(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_FreeDataAndExitPokeblockCase;
}

static void Task_FreeDataAndExitPokeblockCase(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        if (sPokeblockMenu->caseId == PBLOCK_CASE_FEEDER || sPokeblockMenu->caseId == PBLOCK_CASE_GIVE)
            gFieldCallback = FieldCB_ContinueScriptHandleMusic;

        DestroyListMenuTask(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);
        DestroyScrollArrows();
        ResetSpriteData();
        FreeAllSpritePalettes();

        if (sPokeblockMenu->callbackOnUse != NULL)
            SetMainCallback2(sPokeblockMenu->callbackOnUse);
        else
            SetMainCallback2(sSavedPokeblockData.callback);

        FreeAllWindowBuffers();
        Free(sPokeblockMenu);
        DestroyTask(taskId);
    }
}

static void Task_HandlePokeblockMenuInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active && MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (JOY_NEW(SELECT_BUTTON))
        {
            ListMenuGetScrollAndRow(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);
            if (sSavedPokeblockData.scrollOffset + sSavedPokeblockData.selectedRow != sPokeblockMenu->itemsNo - 1)
            {
                // Chose menu item to swap
                PlaySE(SE_SELECT);
                DrawPokeblockMenuHighlight(sSavedPokeblockData.selectedRow, TILE_HIGHLIGHT_RED);
                tToSwapId = sSavedPokeblockData.scrollOffset + sSavedPokeblockData.selectedRow;
                sPokeblockMenu->isSwapping = TRUE;
                gTasks[taskId].func = Task_HandlePokeblocksSwapInput;
            }
        }
        else
        {
            u16 oldPosition = sSavedPokeblockData.selectedRow;
            s32 input = ListMenu_ProcessInput(tListTaskId);
            ListMenuGetScrollAndRow(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);

            if (oldPosition != sSavedPokeblockData.selectedRow)
            {
                // Moved cursor
                DrawPokeblockMenuHighlight(oldPosition, TILE_HIGHLIGHT_NONE);
                DrawPokeblockMenuHighlight(sSavedPokeblockData.selectedRow, TILE_HIGHLIGHT_BLUE);
            }

            switch (input)
            {
            case LIST_NOTHING_CHOSEN:
                break;
            case LIST_CANCEL:
                PlaySE(SE_SELECT);
                gSpecialVar_Result = 0xFFFF;
                gSpecialVar_ItemId = 0;
                FadePaletteAndSetTaskToClosePokeblockCase(taskId);
                break;
            default:
                // Selected Pokéblock
                PlaySE(SE_SELECT);
                gSpecialVar_ItemId = input;
                ShowPokeblockActionsWindow(taskId);
                break;
            }
        }
    }
}

static void Task_HandlePokeblocksSwapInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE)
        return;

    if (JOY_NEW(SELECT_BUTTON))
    {
        // Swap items
        PlaySE(SE_SELECT);
        ListMenuGetScrollAndRow(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);
        UpdatePokeblockSwapMenu(taskId, FALSE);
    }
    else
    {
        u16 i = sSavedPokeblockData.scrollOffset;
        u16 row = sSavedPokeblockData.selectedRow;
        s32 input = ListMenu_ProcessInput(tListTaskId);
        ListMenuGetScrollAndRow(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);

        if (i != sSavedPokeblockData.scrollOffset || row != sSavedPokeblockData.selectedRow)
        {
            for (i = 0; i < MAX_MENU_ITEMS; i++)
            {
                row = i + sSavedPokeblockData.scrollOffset;
                if (row == tToSwapId)
                    DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_RED);
                else
                    DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_NONE);
            }
        }

        SetSwapLineSpritesInvisibility(sPokeblockMenu->swapLineSpriteIds, ARRAY_COUNT(sPokeblockMenu->swapLineSpriteIds), FALSE);
        UpdateSwapLineSpritesPos(sPokeblockMenu->swapLineSpriteIds, ARRAY_COUNT(sPokeblockMenu->swapLineSpriteIds), 128, (sSavedPokeblockData.selectedRow * 16) + 8);

        switch (input)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            if (JOY_NEW(A_BUTTON)) // Pointless check, B Button has been pressed here
                UpdatePokeblockSwapMenu(taskId, FALSE);
            else
                UpdatePokeblockSwapMenu(taskId, TRUE); // Canceled swapping
            break;
        default:
            // Swap items
            PlaySE(SE_SELECT);
            UpdatePokeblockSwapMenu(taskId, FALSE);
            break;
        }
    }
}

static void UpdatePokeblockSwapMenu(u8 taskId, bool8 noSwap)
{
    u8 i;
    s16 *data = gTasks[taskId].data;
    u16 swappedFromId = sSavedPokeblockData.scrollOffset + sSavedPokeblockData.selectedRow;

    sPokeblockMenu->isSwapping = FALSE;
    DestroyListMenuTask(tListTaskId, &sSavedPokeblockData.scrollOffset, &sSavedPokeblockData.selectedRow);

    if (!noSwap && tToSwapId != swappedFromId && tToSwapId != swappedFromId - 1)
    {
        SwapPokeblockMenuItems(tToSwapId, swappedFromId);
        UpdatePokeblockList();
    }

    if (tToSwapId < swappedFromId)
        sSavedPokeblockData.selectedRow--;

    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, sSavedPokeblockData.scrollOffset, sSavedPokeblockData.selectedRow);
    ScheduleBgCopyTilemapToVram(0);
    SetSwapLineSpritesInvisibility(sPokeblockMenu->swapLineSpriteIds, ARRAY_COUNT(sPokeblockMenu->swapLineSpriteIds), TRUE);

    for (i = 0; i < MAX_MENU_ITEMS; i++)
        DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_NONE);

    DrawPokeblockMenuHighlight(sSavedPokeblockData.selectedRow, TILE_HIGHLIGHT_BLUE);
    gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}

static void ShowPokeblockActionsWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (sPokeblockMenu->numActions == 3)
        tWindowId = WIN_ACTIONS_TALL;
    else
        tWindowId = WIN_ACTIONS;

    DestroyScrollArrows();
    DrawStdFrameWithCustomTileAndPalette(tWindowId, FALSE, 1, 0xE);
    PrintMenuActionTextsInUpperLeftCorner(tWindowId, sPokeblockMenu->numActions, sPokeblockMenuActions, sPokeblockMenu->pokeblockActionIds);
    InitMenuInUpperLeftCornerNormal(tWindowId, sPokeblockMenu->numActions, 0);
    PutWindowTilemap(tWindowId);
    ScheduleBgCopyTilemapToVram(1);

    gTasks[taskId].func = Task_HandlePokeblockActionsInput;
}

static void Task_HandlePokeblockActionsInput(u8 taskId)
{
    s8 itemId;

    if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE)
        return;

    itemId = Menu_ProcessInputNoWrap();
    if (itemId == MENU_NOTHING_CHOSEN)
    {
        return;
    }
    else if (itemId == MENU_B_PRESSED)
    {
        PlaySE(SE_SELECT);
        PokeblockAction_Cancel(taskId);
    }
    else
    {
        PlaySE(SE_SELECT);
        sPokeblockMenuActions[sPokeblockMenu->pokeblockActionIds[itemId]].func.void_u8(taskId);
    }
}

static void PokeblockAction_UseOnField(u8 taskId)
{
    sPokeblockMenu->callbackOnUse = UsePokeblockOnField;
    FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

static void UsePokeblockOnField(void)
{
    ChooseMonToGivePokeblock(&gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId], ReturnToPokeblockCaseOnField);
}

static void ReturnToPokeblockCaseOnField(void)
{
    OpenPokeblockCase(PBLOCK_CASE_FIELD, sSavedPokeblockData.callback);
}

static void PokeblockAction_Toss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
    StringCopy(gStringVar1, gPokeblockNames[gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId].color]);
    StringExpandPlaceholders(gStringVarBuffer, gText_ThrowAwayVar1);
    DisplayMessageAndContinueTask(taskId, WIN_TOSS_MSG, 10, 13, FONT_NORMAL, GetPlayerTextSpeedDelay(), gStringVarBuffer, CreateTossPokeblockYesNoMenu);
}

static void CreateTossPokeblockYesNoMenu(u8 taskId)
{
    CreateYesNoMenuWithCallbacks(taskId, &sTossPkblockWindowTemplate, 1, 0, 2, 1, 0xE, &sTossYesNoFuncTable);
}

static void TossedPokeblockMessage(u8 taskId)
{
    StringExpandPlaceholders(gStringVarBuffer, gText_Var1ThrownAway);
    DisplayMessageAndContinueTask(taskId, WIN_TOSS_MSG, 10, 13, FONT_NORMAL, GetPlayerTextSpeedDelay(), gStringVarBuffer, TossPokeblock);
}

static void TossPokeblock(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        s16 *data;
        u16 *scrollOffset, *selectedRow;

        TryClearPokeblock(gSpecialVar_ItemId);
        PlaySE(SE_SELECT);

        scrollOffset = &sSavedPokeblockData.scrollOffset;
        selectedRow = &sSavedPokeblockData.selectedRow;
        data = gTasks[taskId].data;

        DestroyListMenuTask(tListTaskId, scrollOffset, selectedRow);
        DrawPokeblockMenuHighlight(*selectedRow, TILE_HIGHLIGHT_NONE);
        SetMenuItemsCountAndMaxShowed();
        LimitMenuScrollAndRow();
        UpdatePokeblockList();
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollOffset, *selectedRow);
        DrawPokeblockMenuHighlight(*selectedRow, TILE_HIGHLIGHT_BLUE);
        ScheduleBgCopyTilemapToVram(0);
        ScheduleBgCopyTilemapToVram(1);
        CloseTossPokeblockWindow(taskId);
    }
}

static void CloseTossPokeblockWindow(u8 taskId)
{
    ClearDialogWindowAndFrameToTransparent(WIN_TOSS_MSG, FALSE);
    ScheduleBgCopyTilemapToVram(1);
    CreateScrollArrows();
    gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}

static void PokeblockAction_UseInBattle(u8 taskId)
{
    u8 nature = GetNature(&gEnemyParty[0]);
    s16 gain = PokeblockGetGain(nature, &gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId]);
    StringCopy(gBattleTextBuff1, gPokeblockNames[gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId].color]);
    TryClearPokeblock(gSpecialVar_ItemId);

    gSpecialVar_ItemId = gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId].color << 8;
    if (gain == 0)
        gSpecialVar_ItemId += 1;
    else if (gain > 0)
        gSpecialVar_ItemId += 2;
    else
        gSpecialVar_ItemId += 3;

    FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

static void PokeblockAction_UseOnPokeblockFeeder(u8 taskId)
{
    SafariZoneActivatePokeblockFeeder(gSpecialVar_ItemId);
    StringCopy(gStringVar1, gPokeblockNames[gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId].color]);
    gSpecialVar_Result = gSpecialVar_ItemId;
    TryClearPokeblock(gSpecialVar_ItemId);
    gSpecialVar_ItemId = 0;
    FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

static void PokeblockAction_GiveToContestLady(u8 taskId)
{
    gSpecialVar_0x8004 = GivePokeblockToContestLady(&gSaveBlock1Ptr->pokeblocks[gSpecialVar_ItemId]);
    gSpecialVar_Result = gSpecialVar_ItemId;
    TryClearPokeblock(gSpecialVar_ItemId);
    gSpecialVar_ItemId = 0;
    FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

static void PokeblockAction_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
    ScheduleBgCopyTilemapToVram(1);
    CreateScrollArrows();
    gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}

static void ClearPokeblock(u8 pkblId)
{
    gSaveBlock1Ptr->pokeblocks[pkblId].color = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].spicy = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].dry = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].sweet = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].bitter = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].sour = 0;
    gSaveBlock1Ptr->pokeblocks[pkblId].feel = 0;
}

void ClearPokeblocks(void)
{
    u8 i;

    for (i = 0; i < POKEBLOCKS_COUNT; i++)
        ClearPokeblock(i);
}

u8 GetHighestPokeblocksFlavorLevel(const struct Pokeblock *pokeblock)
{
    u8 i;
    u8 maxFlavor = GetPokeblockData(pokeblock, PBLOCK_SPICY);

    for (i = PBLOCK_SPICY; i < FLAVOR_COUNT; i++)
    {
        u8 currFlavor = GetPokeblockData(pokeblock, PBLOCK_SPICY + i);
        if (maxFlavor < currFlavor)
            maxFlavor = currFlavor;
    }

    return maxFlavor;
}

u8 GetPokeblocksFeel(const struct Pokeblock *pokeblock)
{
    u8 feel = GetPokeblockData(pokeblock, PBLOCK_FEEL);
    if (feel > POKEBLOCK_MAX_FEEL)
        feel = POKEBLOCK_MAX_FEEL;

    return feel;
}

s8 GetFirstFreePokeblockSlot(void)
{
    u8 i;

    for (i = 0; i < POKEBLOCKS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->pokeblocks[i].color == PBLOCK_CLR_NONE)
            return i;
    }

    return -1;
}

bool32 AddPokeblock(const struct Pokeblock *pokeblock)
{
    s8 slot = GetFirstFreePokeblockSlot();

    if (slot == -1)
    {
        return FALSE;
    }
    else
    {
        gSaveBlock1Ptr->pokeblocks[slot] = *pokeblock;
        return TRUE;
    }
}

bool32 TryClearPokeblock(u8 pkblId)
{
    if (gSaveBlock1Ptr->pokeblocks[pkblId].color == PBLOCK_CLR_NONE)
    {
        return FALSE;
    }
    else
    {
        ClearPokeblock(pkblId);
        return TRUE;
    }
}

s16 GetPokeblockData(const struct Pokeblock *pokeblock, u8 field)
{
    if (field == PBLOCK_COLOR)
        return pokeblock->color;
    if (field == PBLOCK_SPICY)
        return pokeblock->spicy;
    if (field == PBLOCK_DRY)
        return pokeblock->dry;
    if (field == PBLOCK_SWEET)
        return pokeblock->sweet;
    if (field == PBLOCK_BITTER)
        return pokeblock->bitter;
    if (field == PBLOCK_SOUR)
        return pokeblock->sour;
    if (field == PBLOCK_FEEL)
        return pokeblock->feel;

    return 0;
}

s16 PokeblockGetGain(u8 nature, const struct Pokeblock *pokeblock)
{
    u8 flavor;
    s16 curGain, totalGain = 0;

    for (flavor = 0; flavor < FLAVOR_COUNT; flavor++)
    {
        curGain = GetPokeblockData(pokeblock, flavor + PBLOCK_SPICY);
        if (curGain > 0)
            totalGain += curGain * gPokeblockFlavorCompatibilityTable[FLAVOR_COUNT * nature + flavor];
    }

    return totalGain;
}

void PokeblockCopyName(const struct Pokeblock *pokeblock, u8 *dest)
{
    u8 color = GetPokeblockData(pokeblock, PBLOCK_COLOR);
    StringCopy(dest, gPokeblockNames[color]);
}

bool8 CopyMonFavoritePokeblockName(u8 nature, u8 *dest)
{
    u8 i;

    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (PokeblockGetGain(nature, &sFavoritePokeblocksTable[i]) > 0)
        {
            StringCopy(dest, gPokeblockNames[i + 1]);
            return TRUE;
        }
    }

    return FALSE;
}

u8 GetPokeblocksFlavor(const struct Pokeblock *pokeblock)
{
    s16 bestFlavor = 0;
    s16 i;

    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (GetPokeblockData(pokeblock, bestFlavor + 1) < GetPokeblockData(pokeblock, i + 1))
            bestFlavor = i;
    }

    return bestFlavor;
}
