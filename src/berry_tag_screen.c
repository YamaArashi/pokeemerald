#include "global.h"
#include "berry_tag_screen.h"
#include "berry.h"
#include "decompress.h"
#include "event_object_movement.h"
#include "item_menu.h"
#include "item.h"
#include "item_use.h"
#include "main.h"
#include "menu.h"
#include "text.h"
#include "window.h"
#include "task.h"
#include "menu_helpers.h"
#include "palette.h"
#include "overworld.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "bg.h"
#include "malloc.h"
#include "scanline_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "item_menu_icons.h"
#include "decompress.h"
#include "international_string_util.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// There are 4 windows used in berry tag screen.
enum
{
    WIN_BERRY_NAME,
    WIN_SIZE_FIRM,
    WIN_DESC,
    WIN_BERRY_TAG
};

struct BerryTagScreenStruct
{
    u16 tilemapBuffers[3][0x400];
    u16 berryId;
    u8 berrySpriteId;
    u8 flavorCircleIds[FLAVOR_COUNT];
    u16 gfxState;
};

// EWRAM vars
static EWRAM_DATA struct BerryTagScreenStruct *sBerryTag = NULL;

// const rom data
static const struct BgTemplate sBackgroundTemplates[] =
{
  {
      .bg = 0,
      .charBaseIndex = 0,
      .mapBaseIndex = 31,
      .screenSize = 0,
      .paletteMode = 0,
      .priority = 0,
      .baseTile = 0
  },
  {
      .bg = 1,
      .charBaseIndex = 0,
      .mapBaseIndex = 30,
      .screenSize = 0,
      .paletteMode = 0,
      .priority = 1,
      .baseTile = 0
  },
  {
      .bg = 2,
      .charBaseIndex = 0,
      .mapBaseIndex = 29,
      .screenSize = 0,
      .paletteMode = 0,
      .priority = 2,
      .baseTile = 0
  },
  {
      .bg = 3,
      .charBaseIndex = 0,
      .mapBaseIndex = 28,
      .screenSize = 0,
      .paletteMode = 0,
      .priority = 3,
      .baseTile = 0
  }
};

static const u16 sFontPalette[] = INCBIN_U16("graphics/bag/berry_tag_screen.gbapal");

static const u8 sTextColors[2][3] =
{
    {0, 2, 3},
    {15, 14, 13}
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_BERRY_NAME] = {
        .bg = 1,
        .tilemapLeft = 11,
        .tilemapTop = 4,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 69,
    },
    [WIN_SIZE_FIRM] = {
        .bg = 1,
        .tilemapLeft = 11,
        .tilemapTop = 7,
        .width = 18,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 85,
    },
    [WIN_DESC] = {
        .bg = 1,
        .tilemapLeft = 4,
        .tilemapTop = 14,
        .width = 25,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 157,
    },
    [WIN_BERRY_TAG] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 257,
    },
    DUMMY_WIN_TEMPLATE
};

static const u8 *const sBerryFirmnessStrings[] =
{
    gBerryFirmnessString_VerySoft,
    gBerryFirmnessString_Soft,
    gBerryFirmnessString_Hard,
    gBerryFirmnessString_VeryHard,
    gBerryFirmnessString_SuperHard
};

// this file's functions
static void CB2_InitBerryTagScreen(void);
static void HandleInitBackgrounds(void);
static void HandleInitWindows(void);
static void AddBerryTagTextToBg0(void);
static void PrintAllBerryData(void);
static void CreateBerrySprite(void);
static void CreateFlavorCircleSprites(void);
static void SetFlavorCirclesVisiblity(void);
static void PrintBerryNumberAndName(void);
static void PrintBerrySize(void);
static void PrintBerryFirmness(void);
static void PrintBerryDescription1(void);
static void PrintBerryDescription2(void);
static bool8 InitBerryTagScreen(void);
static bool8 LoadBerryTagGfx(void);
static void Task_HandleInput(u8 taskId);
static void Task_CloseBerryTagScreen(u8 taskId);
static void Task_DisplayAnotherBerry(u8 taskId);
static void TryChangeDisplayedBerry(u8 taskId, s8 toMove);
static void HandleBagCursorPositionChange(s8 toMove);

// code
void DoBerryTagScreen(void)
{
    sBerryTag = AllocZeroed(sizeof(*sBerryTag));
    sBerryTag->berryId = ItemIdToBerryType(gSpecialVar_ItemId);
    SetMainCallback2(CB2_InitBerryTagScreen);
}

static void CB2_BerryTagScreen(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VblankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_InitBerryTagScreen(void)
{
    while (1)
    {
        if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE)
            break;
        if (InitBerryTagScreen() == TRUE)
            break;
        if (MenuHelpers_IsLinkActive() == TRUE)
            break;
    }
}

static bool8 InitBerryTagScreen(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ResetVramOamAndBgCntRegs();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        gPaletteFade.bufferTransferDisabled = 1;
        gMain.state++;
        break;
    case 3:
        ResetSpriteData();
        gMain.state++;
        break;
    case 4:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 5:
        if (!MenuHelpers_IsLinkActive())
            ResetTasks();
        gMain.state++;
        break;
    case 6:
        HandleInitBackgrounds();
        sBerryTag->gfxState = 0;
        gMain.state++;
        break;
    case 7:
        if (LoadBerryTagGfx())
            gMain.state++;
        break;
    case 8:
        HandleInitWindows();
        gMain.state++;
        break;
    case 9:
        AddBerryTagTextToBg0();
        gMain.state++;
        break;
    case 10:
        PrintAllBerryData();
        gMain.state++;
        break;
    case 11:
        CreateBerrySprite();
        gMain.state++;
        break;
    case 12:
        CreateFlavorCircleSprites();
        SetFlavorCirclesVisiblity();
        gMain.state++;
        break;
    case 13:
        CreateTask(Task_HandleInput, 0);
        gMain.state++;
        break;
    case 14:
        BlendPalettes(PALETTES_ALL, 0x10, RGB_BLACK);
        gMain.state++;
        break;
    case 15:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = 0;
        gMain.state++;
        break;
    default: // done
        SetVBlankCallback(VblankCB);
        SetMainCallback2(CB2_BerryTagScreen);
        return TRUE;
    }

    return FALSE;
}

static void HandleInitBackgrounds(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBackgroundTemplates, ARRAY_COUNT(sBackgroundTemplates));
    SetBgTilemapBuffer(2, sBerryTag->tilemapBuffers[0]);
    SetBgTilemapBuffer(3, sBerryTag->tilemapBuffers[1]);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

#define BG_TILE 0x42

static bool8 LoadBerryTagGfx(void)
{
    u16 i;

    switch (sBerryTag->gfxState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(2, gBerryCheck_Gfx, 0, 0, 0);
        sBerryTag->gfxState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(gBerryTag_Gfx, sBerryTag->tilemapBuffers[0]);
            sBerryTag->gfxState++;
        }
        break;
    case 2:
        LZDecompressWram(gBerryTag_Pal, sBerryTag->tilemapBuffers[2]);
        sBerryTag->gfxState++;
        break;
    case 3:
        // Palette of the bg tiles changes depending on the player's gender
        if (gSaveBlock2Ptr->playerGender == MALE)
        {
            for (i = 0; i < ARRAY_COUNT(sBerryTag->tilemapBuffers[1]); i++)
                sBerryTag->tilemapBuffers[1][i] = (4 << 12) | BG_TILE;
        }
        else
        {
            for (i = 0; i < ARRAY_COUNT(sBerryTag->tilemapBuffers[1]); i++)
                sBerryTag->tilemapBuffers[1][i] = (5 << 12) | BG_TILE;
        }
        sBerryTag->gfxState++;
        break;
    case 4:
        LoadCompressedPalette(gBerryCheck_Pal, BG_PLTT_ID(0), 6 * PLTT_SIZE_4BPP);
        sBerryTag->gfxState++;
        break;
    case 5:
        LoadCompressedSpriteSheet(&gBerryCheckCircleSpriteSheet);
        sBerryTag->gfxState++;
        break;
    default:
        LoadCompressedSpritePalette(&gBerryCheckCirclePaletteTable);
        return TRUE; // done
    }

    return FALSE;
}

static void HandleInitWindows(void)
{
    u16 i;

    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadPalette(sFontPalette, BG_PLTT_ID(15), sizeof(sFontPalette));
    for (i = 0; i < ARRAY_COUNT(sWindowTemplates) - 1; i++)
        PutWindowTilemap(i);
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
}

static void PrintTextInBerryTagScreen(u8 windowId, const u8 *text, u8 x, u8 y, s32 speed, u8 colorStructId)
{
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 0, 0, sTextColors[colorStructId], speed, text);
}

static void AddBerryTagTextToBg0(void)
{
    memcpy(GetBgTilemapBuffer(0), sBerryTag->tilemapBuffers[2], sizeof(sBerryTag->tilemapBuffers[2]));
    FillWindowPixelBuffer(WIN_BERRY_TAG, PIXEL_FILL(15));
    PrintTextInBerryTagScreen(WIN_BERRY_TAG, gText_BerryTag, GetStringCenterAlignXOffset(FONT_NORMAL, gText_BerryTag, 0x40), 1, 0, 1);
    PutWindowTilemap(WIN_BERRY_TAG);
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintAllBerryData(void)
{
    PrintBerryNumberAndName();
    PrintBerrySize();
    PrintBerryFirmness();
    PrintBerryDescription1();
    PrintBerryDescription2();
}

static void PrintBerryNumberAndName(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);
    ConvertIntToDecimalStringN(gStringVar1, sBerryTag->berryId, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringCopy(gStringVar2, berry->name);
    StringExpandPlaceholders(gStringVarBuffer, gText_NumberVar1Var2);
    PrintTextInBerryTagScreen(WIN_BERRY_NAME, gStringVarBuffer, 0, 1, 0, 0);
}

static void PrintBerrySize(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);
    AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, gText_SizeSlash, 0, 1, TEXT_SKIP_DRAW, NULL);
    if (berry->size != 0)
    {
        u32 inches, fraction;

        inches = 1000 * berry->size / 254;
        if (inches % 10 > 4)
            inches += 10;
        fraction = (inches % 100) / 10;
        inches /= 100;

        ConvertIntToDecimalStringN(gStringVar1, inches, STR_CONV_MODE_LEFT_ALIGN, 2);
        ConvertIntToDecimalStringN(gStringVar2, fraction, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(gStringVarBuffer, gText_Var1DotVar2);
        AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, gStringVarBuffer, 0x28, 1, 0, NULL);
    }
    else
    {
        AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, gText_ThreeMarks, 0x28, 1, 0, NULL);
    }
}

static void PrintBerryFirmness(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);
    AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, gText_FirmSlash, 0, 0x11, TEXT_SKIP_DRAW, NULL);
    if (berry->firmness != 0)
        AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, sBerryFirmnessStrings[berry->firmness - 1], 0x28, 0x11, 0, NULL);
    else
        AddTextPrinterParameterized(WIN_SIZE_FIRM, FONT_NORMAL, gText_ThreeMarks, 0x28, 0x11, 0, NULL);
}

static void PrintBerryDescription1(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);
    AddTextPrinterParameterized(WIN_DESC, FONT_NORMAL, berry->description1, 0, 1, 0, NULL);
}

static void PrintBerryDescription2(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);
    AddTextPrinterParameterized(WIN_DESC, FONT_NORMAL, berry->description2, 0, 0x11, 0, NULL);
}

static void CreateBerrySprite(void)
{
    sBerryTag->berrySpriteId = CreateBerryTagSprite(sBerryTag->berryId - 1, 56, 64);
}

static void DestroyBerrySprite(void)
{
    DestroySprite(&gSprites[sBerryTag->berrySpriteId]);
    FreeBerryTagSpritePalette();
}

static void CreateFlavorCircleSprites(void)
{
    sBerryTag->flavorCircleIds[FLAVOR_SPICY] = CreateBerryFlavorCircleSprite(64);
    sBerryTag->flavorCircleIds[FLAVOR_DRY] = CreateBerryFlavorCircleSprite(104);
    sBerryTag->flavorCircleIds[FLAVOR_SWEET] = CreateBerryFlavorCircleSprite(144);
    sBerryTag->flavorCircleIds[FLAVOR_BITTER] = CreateBerryFlavorCircleSprite(184);
    sBerryTag->flavorCircleIds[FLAVOR_SOUR] = CreateBerryFlavorCircleSprite(224);
}

static void SetFlavorCirclesVisiblity(void)
{
    const struct Berry *berry = GetBerryInfo(sBerryTag->berryId);

    if (berry->spicy)
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SPICY]].invisible = FALSE;
    else
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SPICY]].invisible = TRUE;

    if (berry->dry)
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_DRY]].invisible = FALSE;
    else
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_DRY]].invisible = TRUE;

    if (berry->sweet)
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SWEET]].invisible = FALSE;
    else
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SWEET]].invisible = TRUE;

    if (berry->bitter)
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_BITTER]].invisible = FALSE;
    else
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_BITTER]].invisible = TRUE;

    if (berry->sour)
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SOUR]].invisible = FALSE;
    else
        gSprites[sBerryTag->flavorCircleIds[FLAVOR_SOUR]].invisible = TRUE;
}

static void DestroyFlavorCircleSprites(void)
{
    u16 i;

    for (i = 0; i < FLAVOR_COUNT; i++)
        DestroySprite(&gSprites[sBerryTag->flavorCircleIds[i]]);
}

static void PrepareToCloseBerryTagScreen(u8 taskId)
{
    PlaySE(SE_SELECT);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gTasks[taskId].func = Task_CloseBerryTagScreen;
}

static void Task_CloseBerryTagScreen(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyBerrySprite();
        DestroyFlavorCircleSprites();
        Free(sBerryTag);
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_ReturnToBagMenuPocket);
        DestroyTask(taskId);
    }
}

static void Task_HandleInput(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        u16 arrowKeys = JOY_REPEAT(DPAD_ANY);
        if (arrowKeys == DPAD_UP)
            TryChangeDisplayedBerry(taskId, -1);
        else if (arrowKeys == DPAD_DOWN)
            TryChangeDisplayedBerry(taskId, 1);
        else if (JOY_NEW(A_BUTTON | B_BUTTON))
            PrepareToCloseBerryTagScreen(taskId);
    }
}

#define tBerryY data[0]
#define tBgOp   data[1]

static void TryChangeDisplayedBerry(u8 taskId, s8 toMove)
{
    s16 *data = gTasks[taskId].data;
    s16 currPocketPosition = gBagPosition.scrollPosition[BERRIES_POCKET] + gBagPosition.cursorPosition[BERRIES_POCKET];
    u32 newPocketPosition = currPocketPosition + toMove;
    if (newPocketPosition < ITEM_TO_BERRY(MAX_BERRY_INDEX) && BagGetItemIdByPocketPosition(POCKET_BERRIES, newPocketPosition) != ITEM_NONE)
    {
        if (toMove < 0)
            tBgOp = BG_COORD_SUB;
        else
            tBgOp = BG_COORD_ADD;

        tBerryY = 0;
        PlaySE(SE_SELECT);
        HandleBagCursorPositionChange(toMove);
        gTasks[taskId].func = Task_DisplayAnotherBerry;
    }
}

static void HandleBagCursorPositionChange(s8 toMove)
{
    u16 *scrollPos = &gBagPosition.scrollPosition[BERRIES_POCKET];
    u16 *cursorPos = &gBagPosition.cursorPosition[BERRIES_POCKET];
    if (toMove > 0)
    {
        if (*cursorPos < 4 || BagGetItemIdByPocketPosition(POCKET_BERRIES, *scrollPos + 8) == 0)
            *cursorPos += toMove;
        else
            *scrollPos += toMove;
    }
    else
    {
        if (*cursorPos > 3 || *scrollPos == 0)
            *cursorPos += toMove;
        else
            *scrollPos += toMove;
    }

    sBerryTag->berryId = ItemIdToBerryType(BagGetItemIdByPocketPosition(POCKET_BERRIES, *scrollPos + *cursorPos));
}

#define DISPLAY_SPEED 16

static void Task_DisplayAnotherBerry(u8 taskId)
{
    u16 i;
    s16 y;
    s16 *data = gTasks[taskId].data;
    tBerryY += DISPLAY_SPEED;
    tBerryY &= 0xFF;

    if (tBgOp == BG_COORD_ADD)
    {
        switch (tBerryY)
        {
        case 3 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_BERRY_NAME, PIXEL_FILL(0));
            break;
        case 4 * DISPLAY_SPEED:
            PrintBerryNumberAndName();
            break;
        case 5 * DISPLAY_SPEED:
            DestroyBerrySprite();
            CreateBerrySprite();
            break;
        case 6 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_SIZE_FIRM, PIXEL_FILL(0));
            break;
        case 7 * DISPLAY_SPEED:
            PrintBerrySize();
            break;
        case 8 * DISPLAY_SPEED:
            PrintBerryFirmness();
            break;
        case 9 * DISPLAY_SPEED:
            SetFlavorCirclesVisiblity();
            break;
        case 10 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
            break;
        case 11 * DISPLAY_SPEED:
            PrintBerryDescription1();
            break;
        case 12 * DISPLAY_SPEED:
            PrintBerryDescription2();
            break;
        }
    }
    else // BG_COORD_SUB
    {
        switch (tBerryY)
        {
        case 3 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
            break;
        case 4 * DISPLAY_SPEED:
            PrintBerryDescription2();
            break;
        case 5 * DISPLAY_SPEED:
            PrintBerryDescription1();
            break;
        case 6 * DISPLAY_SPEED:
            SetFlavorCirclesVisiblity();
            break;
        case 7 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_SIZE_FIRM, PIXEL_FILL(0));
            break;
        case 8 * DISPLAY_SPEED:
            PrintBerryFirmness();
            break;
        case 9 * DISPLAY_SPEED:
            PrintBerrySize();
            break;
        case 10 * DISPLAY_SPEED:
            DestroyBerrySprite();
            CreateBerrySprite();
            break;
        case 11 * DISPLAY_SPEED:
            FillWindowPixelBuffer(WIN_BERRY_NAME, PIXEL_FILL(0));
            break;
        case 12 * DISPLAY_SPEED:
            PrintBerryNumberAndName();
            break;
        }
    }

    if (tBgOp == BG_COORD_ADD)
        y = -tBerryY;
    else
        y = tBerryY;

    gSprites[sBerryTag->berrySpriteId].y2 = y;
    for (i = 0; i < FLAVOR_COUNT; i++)
        gSprites[sBerryTag->flavorCircleIds[i]].y2 = y;

    ChangeBgY(1, 0x1000, tBgOp);
    ChangeBgY(2, 0x1000, tBgOp);

    if (tBerryY == 0)
        gTasks[taskId].func = Task_HandleInput;
}
