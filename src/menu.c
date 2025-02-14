#include "global.h"
#include "malloc.h"
#include "bg.h"
#include "blit.h"
#include "dma3.h"
#include "event_data.h"
#include "graphics.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon_icon.h"
#include "region_map.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text_window.h"
#include "window.h"
#include "constants/songs.h"

#define DLG_WINDOW_PALETTE_NUM 15
#define DLG_WINDOW_BASE_TILE_NUM 0x200
#define STD_WINDOW_PALETTE_NUM 14
#define STD_WINDOW_PALETTE_SIZE PLTT_SIZEOF(10)
#define STD_WINDOW_BASE_TILE_NUM 0x214

struct MenuInfoIcon
{
    u8 width;
    u8 height;
    u16 offset;
};

struct Menu
{
    u8 left;
    u8 top;
    s8 cursorPos;
    s8 minCursorPos;
    s8 maxCursorPos;
    u8 windowId;
    u8 fontId;
    u8 optionWidth;
    u8 optionHeight;
    u8 columns;
    u8 rows;
    bool8 APressMuted;
};

static u16 AddWindowParameterized(u8, u8, u8, u8, u8, u8, u16);
static void WindowFunc_DrawStandardFrame(u8, u8, u8, u8, u8, u8);
static void WindowFunc_DrawDialogueFrame(u8, u8, u8, u8, u8, u8);
static void WindowFunc_ClearStdWindowAndFrame(u8, u8, u8, u8, u8, u8);
static void WindowFunc_ClearDialogWindowAndFrame(u8, u8, u8, u8, u8, u8);
static void WindowFunc_DrawDialogFrameWithCustomTileAndPalette(u8, u8, u8, u8, u8, u8);
static void WindowFunc_ClearDialogWindowAndFrameNullPalette(u8, u8, u8, u8, u8, u8);
static void WindowFunc_DrawStdFrameWithCustomTileAndPalette(u8, u8, u8, u8, u8, u8);
static void WindowFunc_ClearStdWindowAndFrameToTransparent(u8, u8, u8, u8, u8, u8);
static void task_free_buf_after_copying_tile_data_to_vram(u8 taskId);

static EWRAM_DATA u8 sStartMenuWindowId = 0;
static EWRAM_DATA u8 sMapNamePopupWindowId = 0;
static EWRAM_DATA struct Menu sMenu = {0};
static EWRAM_DATA u16 sTileNum = 0;
static EWRAM_DATA u8 sPaletteNum = 0;
static EWRAM_DATA u8 sYesNoWindowId = 0;
static EWRAM_DATA u8 sHofPCTopBarWindowId = 0;
static EWRAM_DATA u16 sFiller = 0;  // needed to align
static EWRAM_DATA bool8 sScheduledBgCopiesToVram[4] = {FALSE};
static EWRAM_DATA u16 sTempTileDataBufferIdx = 0;
static EWRAM_DATA void *sTempTileDataBuffer[0x20] = {NULL};

const u16 gStandardMenuPalette[] = INCBIN_U16("graphics/interface/std_menu.gbapal");

static const u8 sTextSpeedFrameDelays[] =
{
    [OPTIONS_TEXT_SPEED_SLOW] = 8,
    [OPTIONS_TEXT_SPEED_MID]  = 4,
    [OPTIONS_TEXT_SPEED_FAST] = 1
};

static const struct WindowTemplate sStandardTextBox_WindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x194
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sYesNo_WindowTemplates =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x125
};

static const u16 sHofPC_TopBar_Pal[] = INCBIN_U16("graphics/interface/hof_pc_topbar.gbapal");
static const u8 sTextColors[] = { TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };

// Table of move info icon offsets in graphics/interface/menu_info.png
static const struct MenuInfoIcon sMenuInfoIcons[] =
{   // { width, height, offset }
    { 12, 12, 0x00 },  // Unused
    [TYPE_NORMAL + 1]   = { 32, 12, 0x20 },
    [TYPE_FIGHTING + 1] = { 32, 12, 0x64 },
    [TYPE_FLYING + 1]   = { 32, 12, 0x60 },
    [TYPE_POISON + 1]   = { 32, 12, 0x80 },
    [TYPE_GROUND + 1]   = { 32, 12, 0x48 },
    [TYPE_ROCK + 1]     = { 32, 12, 0x44 },
    [TYPE_BUG + 1]      = { 32, 12, 0x6C },
    [TYPE_GHOST + 1]    = { 32, 12, 0x68 },
    [TYPE_STEEL + 1]    = { 32, 12, 0x88 },
    [TYPE_MYSTERY + 1]  = { 32, 12, 0xA4 },
    [TYPE_FIRE + 1]     = { 32, 12, 0x24 },
    [TYPE_WATER + 1]    = { 32, 12, 0x28 },
    [TYPE_GRASS + 1]    = { 32, 12, 0x2C },
    [TYPE_ELECTRIC + 1] = { 32, 12, 0x40 },
    [TYPE_PSYCHIC + 1]  = { 32, 12, 0x84 },
    [TYPE_ICE + 1]      = { 32, 12, 0x4C },
    [TYPE_DRAGON + 1]   = { 32, 12, 0xA0 },
    [TYPE_DARK + 1]     = { 32, 12, 0x8C },
    [MENU_INFO_ICON_TYPE]      = { 42, 12, 0xA8 },
    [MENU_INFO_ICON_POWER]     = { 42, 12, 0xC0 },
    [MENU_INFO_ICON_ACCURACY]  = { 42, 12, 0xC8 },
    [MENU_INFO_ICON_PP]        = { 42, 12, 0xE0 },
    [MENU_INFO_ICON_EFFECT]    = { 42, 12, 0xE8 }, // Unused
    [MENU_INFO_ICON_BALL_RED]  = {  8,  8, 0xAE }, // For placed decorations in Secret Base
    [MENU_INFO_ICON_BALL_BLUE] = {  8,  8, 0xAF }, // For placed decorations in player's room
};

void InitStandardTextBoxWindows(void)
{
    InitWindows(sStandardTextBox_WindowTemplates);
    sStartMenuWindowId = WINDOW_NONE;
    sMapNamePopupWindowId = WINDOW_NONE;
}

void FreeAllOverworldWindowBuffers(void)
{
    FreeAllWindowBuffers();
}

void InitTextBoxGfxAndPrinters(void)
{
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    DeactivateAllTextPrinters();
    LoadMessageBoxAndBorderGfx();
}

u16 RunTextPrintersAndIsPrinter0Active(void)
{
    RunTextPrinters();
    return IsTextPrinterActive(0);
}

u16 AddTextPrinterParameterized2(u8 windowId, u8 fontId, const u8 *str, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16), u8 fgColor, u8 bgColor, u8 shadowColor)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = 0;
    printer.y = 1;
    printer.currentX = 0;
    printer.currentY = 1;
    printer.letterSpacing = 0;
    printer.lineSpacing = 0;
    printer.unk = 0;
    printer.fgColor = fgColor;
    printer.bgColor = bgColor;
    printer.shadowColor = shadowColor;

    gTextFlags.useAlternateDownArrow = 0;
    return AddTextPrinter(&printer, speed, callback);
}

void AddTextPrinterForMessage(bool8 allowSkippingDelayWithButtonPress)
{
    void (*callback)(struct TextPrinterTemplate *, u16) = NULL;
    gTextFlags.canABSpeedUpPrint = allowSkippingDelayWithButtonPress;
    AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVarBuffer, GetPlayerTextSpeedDelay(), callback, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

void AddTextPrinterForMessage_2(bool8 allowSkippingDelayWithButtonPress)
{
    gTextFlags.canABSpeedUpPrint = allowSkippingDelayWithButtonPress;
    AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVarBuffer, GetPlayerTextSpeedDelay(), NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

void AddTextPrinterWithCustomSpeedForMessage(bool8 allowSkippingDelayWithButtonPress, u8 speed)
{
    gTextFlags.canABSpeedUpPrint = allowSkippingDelayWithButtonPress;
    AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVarBuffer, speed, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

void LoadMessageBoxAndBorderGfx(void)
{
    LoadMessageBoxGfx(0, DLG_WINDOW_BASE_TILE_NUM, BG_PLTT_ID(DLG_WINDOW_PALETTE_NUM));
    LoadUserWindowBorderGfx(0, STD_WINDOW_BASE_TILE_NUM, BG_PLTT_ID(STD_WINDOW_PALETTE_NUM));
}

void DrawDialogueFrame(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, WindowFunc_DrawDialogueFrame);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

void DrawStdWindowFrame(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, WindowFunc_DrawStandardFrame);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

void ClearDialogWindowAndFrame(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, WindowFunc_ClearDialogWindowAndFrame);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

void ClearStdWindowAndFrame(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, WindowFunc_ClearStdWindowAndFrame);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void WindowFunc_DrawStandardFrame(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    int i;

    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 0,
                            tilemapLeft - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            STD_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 1,
                            tilemapLeft,
                            tilemapTop - 1,
                            width,
                            1,
                            STD_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 2,
                            tilemapLeft + width,
                            tilemapTop - 1,
                            1,
                            1,
                            STD_WINDOW_PALETTE_NUM);

    for (i = tilemapTop; i < tilemapTop + height; i++)
    {
        FillBgTilemapBufferRect(bg,
                                STD_WINDOW_BASE_TILE_NUM + 3,
                                tilemapLeft - 1,
                                i,
                                1,
                                1,
                                STD_WINDOW_PALETTE_NUM);
        FillBgTilemapBufferRect(bg,
                                STD_WINDOW_BASE_TILE_NUM + 5,
                                tilemapLeft + width,
                                i,
                                1,
                                1,
                                STD_WINDOW_PALETTE_NUM);
    }

    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 6,
                            tilemapLeft - 1,
                            tilemapTop + height,
                            1,
                            1,
                            STD_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 7,
                            tilemapLeft,
                            tilemapTop + height,
                            width,
                            1,
                            STD_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            STD_WINDOW_BASE_TILE_NUM + 8,
                            tilemapLeft + width,
                            tilemapTop + height,
                            1,
                            1,
                            STD_WINDOW_PALETTE_NUM);
}

static void WindowFunc_DrawDialogueFrame(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 1,
                            tilemapLeft - 2,
                            tilemapTop - 1,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 3,
                            tilemapLeft - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 4,
                            tilemapLeft,
                            tilemapTop - 1,
                            width - 1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 5,
                            tilemapLeft + width - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 6,
                            tilemapLeft + width,
                            tilemapTop - 1,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 7,
                            tilemapLeft - 2,
                            tilemapTop,
                            1,
                            5,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 9,
                            tilemapLeft - 1,
                            tilemapTop,
                            width + 1,
                            5,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            DLG_WINDOW_BASE_TILE_NUM + 10,
                            tilemapLeft + width,
                            tilemapTop,
                            1,
                            5,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(DLG_WINDOW_BASE_TILE_NUM + 1),
                            tilemapLeft - 2,
                            tilemapTop + height,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(DLG_WINDOW_BASE_TILE_NUM + 3),
                            tilemapLeft - 1,
                            tilemapTop + height,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(DLG_WINDOW_BASE_TILE_NUM + 4),
                            tilemapLeft,
                            tilemapTop + height,
                            width - 1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(DLG_WINDOW_BASE_TILE_NUM + 5),
                            tilemapLeft + width - 1,
                            tilemapTop + height,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(DLG_WINDOW_BASE_TILE_NUM + 6),
                            tilemapLeft + width,
                            tilemapTop + height,
                            1,
                            1,
                            DLG_WINDOW_PALETTE_NUM);
}

static void WindowFunc_ClearStdWindowAndFrame(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg, 0, tilemapLeft - 1, tilemapTop - 1, width + 2, height + 2, STD_WINDOW_PALETTE_NUM);
}

static void WindowFunc_ClearDialogWindowAndFrame(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg, 0, tilemapLeft - 3, tilemapTop - 1, width + 6, height + 2, STD_WINDOW_PALETTE_NUM);
}

void SetStandardWindowBorderStyle(u8 windowId, bool8 copyToVram)
{
    DrawStdFrameWithCustomTileAndPalette(windowId, copyToVram, STD_WINDOW_BASE_TILE_NUM, STD_WINDOW_PALETTE_NUM);
}

void LoadMessageBoxAndFrameGfx(u8 windowId, bool8 copyToVram)
{
    LoadMessageBoxGfx(windowId, DLG_WINDOW_BASE_TILE_NUM, BG_PLTT_ID(DLG_WINDOW_PALETTE_NUM));
    DrawDialogFrameWithCustomTileAndPalette(windowId, copyToVram, DLG_WINDOW_BASE_TILE_NUM, DLG_WINDOW_PALETTE_NUM);
}

void Menu_LoadStdPal(void)
{
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(STD_WINDOW_PALETTE_NUM), STD_WINDOW_PALETTE_SIZE);
}

void Menu_LoadStdPalAt(u16 offset)
{
    LoadPalette(gStandardMenuPalette, offset, STD_WINDOW_PALETTE_SIZE);
}

static UNUSED const u16* Menu_GetStdPal(void)
{
    return gStandardMenuPalette;
}

static u16 UNUSED Menu_GetStdPalColor(u8 colorNum)
{
    if (colorNum > 15)
        colorNum = 0;
    return gStandardMenuPalette[colorNum];
}

void DisplayItemMessageOnField(u8 taskId, const u8 *string, TaskFunc callback)
{
    LoadMessageBoxAndBorderGfx();
    DisplayMessageAndContinueTask(taskId, 0, DLG_WINDOW_BASE_TILE_NUM, DLG_WINDOW_PALETTE_NUM, FONT_NORMAL, GetPlayerTextSpeedDelay(), string, callback);
    CopyWindowToVram(0, COPYWIN_FULL);
}

void DisplayYesNoMenuDefaultYes(void)
{
    CreateYesNoMenu(&sYesNo_WindowTemplates, STD_WINDOW_BASE_TILE_NUM, STD_WINDOW_PALETTE_NUM, 0);
}

void DisplayYesNoMenuWithDefault(u8 initialCursorPos)
{
    CreateYesNoMenu(&sYesNo_WindowTemplates, STD_WINDOW_BASE_TILE_NUM, STD_WINDOW_PALETTE_NUM, initialCursorPos);
}

u32 GetPlayerTextSpeed(void)
{
    if (gTextFlags.forceMidTextSpeed)
        return OPTIONS_TEXT_SPEED_MID;
    return gSaveBlock2Ptr->optionsTextSpeed;
}

u8 GetPlayerTextSpeedDelay(void)
{
    u32 speed;
    if (gSaveBlock2Ptr->optionsTextSpeed > OPTIONS_TEXT_SPEED_FAST)
        gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_MID;
    speed = GetPlayerTextSpeed();
    return sTextSpeedFrameDelays[speed];
}

u8 AddStartMenuWindow(u8 numActions)
{
    if (sStartMenuWindowId == WINDOW_NONE)
        sStartMenuWindowId = AddWindowParameterized(0, 22, 1, 7, (numActions * 2) + 2, 15, 0x139);
    return sStartMenuWindowId;
}

u8 GetStartMenuWindowId(void)
{
    return sStartMenuWindowId;
}

void RemoveStartMenuWindow(void)
{
    if (sStartMenuWindowId != WINDOW_NONE)
    {
        RemoveWindow(sStartMenuWindowId);
        sStartMenuWindowId = WINDOW_NONE;
    }
}

static u16 UNUSED GetDialogFrameBaseTileNum(void)
{
    return DLG_WINDOW_BASE_TILE_NUM;
}

static u16 UNUSED GetStandardFrameBaseTileNum(void)
{
    return STD_WINDOW_BASE_TILE_NUM;
}

u8 AddMapNamePopUpWindow(void)
{
    if (sMapNamePopupWindowId == WINDOW_NONE)
        sMapNamePopupWindowId = AddWindowParameterized(0, 1, 1, 10, 3, 14, 0x107);
    return sMapNamePopupWindowId;
}

u8 GetMapNamePopUpWindowId(void)
{
    return sMapNamePopupWindowId;
}

void RemoveMapNamePopUpWindow(void)
{
    if (sMapNamePopupWindowId != WINDOW_NONE)
    {
        RemoveWindow(sMapNamePopupWindowId);
        sMapNamePopupWindowId = WINDOW_NONE;
    }
}

void AddTextPrinterWithCallbackForMessage(bool8 canSpeedUp, void (*callback)(struct TextPrinterTemplate *, u16))
{
    gTextFlags.canABSpeedUpPrint = canSpeedUp;
    AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVarBuffer, GetPlayerTextSpeedDelay(), callback, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

void EraseFieldMessageBox(bool8 copyToVram)
{
    FillBgTilemapBufferRect(0, 0, 0, 0, 32, 32, 17);
    if (copyToVram == TRUE)
        CopyBgTilemapBufferToVram(0);
}

void DrawDialogFrameWithCustomTileAndPalette(u8 windowId, bool8 copyToVram, u16 tileNum, u8 paletteNum)
{
    sTileNum = tileNum;
    sPaletteNum = paletteNum;
    CallWindowFunction(windowId, WindowFunc_DrawDialogFrameWithCustomTileAndPalette);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void UNUSED DrawDialogFrameWithCustomTile(u8 windowId, bool8 copyToVram, u16 tileNum)
{
    sTileNum = tileNum;
    sPaletteNum = GetWindowAttribute(windowId, WINDOW_PALETTE_NUM);
    CallWindowFunction(windowId, WindowFunc_DrawDialogFrameWithCustomTileAndPalette);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void WindowFunc_DrawDialogFrameWithCustomTileAndPalette(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg,
                            sTileNum + 1,
                            tilemapLeft - 2,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 3,
                            tilemapLeft - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 4,
                            tilemapLeft,
                            tilemapTop - 1,
                            width - 1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 5,
                            tilemapLeft + width - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 6,
                            tilemapLeft + width,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 7,
                            tilemapLeft - 2,
                            tilemapTop,
                            1,
                            5,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 9,
                            tilemapLeft - 1,
                            tilemapTop,
                            width + 1,
                            5,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 10,
                            tilemapLeft + width,
                            tilemapTop,
                            1,
                            5,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(sTileNum + 1),
                            tilemapLeft - 2,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(sTileNum + 3),
                            tilemapLeft - 1,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(sTileNum + 4),
                            tilemapLeft,
                            tilemapTop + height,
                            width - 1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(sTileNum + 5),
                            tilemapLeft + width - 1,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            BG_TILE_V_FLIP(sTileNum + 6),
                            tilemapLeft + width,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
}

void ClearDialogWindowAndFrameToTransparent(u8 windowId, bool8 copyToVram)
{
    // The palette slot doesn't matter, since the tiles are transparent.
    CallWindowFunction(windowId, WindowFunc_ClearDialogWindowAndFrameNullPalette);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void WindowFunc_ClearDialogWindowAndFrameNullPalette(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg, 0, tilemapLeft - 3, tilemapTop - 1, width + 6, height + 2, 0);
}

void DrawStdFrameWithCustomTileAndPalette(u8 windowId, bool8 copyToVram, u16 baseTileNum, u8 paletteNum)
{
    sTileNum = baseTileNum;
    sPaletteNum = paletteNum;
    CallWindowFunction(windowId, WindowFunc_DrawStdFrameWithCustomTileAndPalette);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

// Never used.
void DrawStdFrameWithCustomTile(u8 windowId, bool8 copyToVram, u16 baseTileNum)
{
    sTileNum = baseTileNum;
    sPaletteNum = GetWindowAttribute(windowId, WINDOW_PALETTE_NUM);
    CallWindowFunction(windowId, WindowFunc_DrawStdFrameWithCustomTileAndPalette);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void WindowFunc_DrawStdFrameWithCustomTileAndPalette(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg,
                            sTileNum + 0,
                            tilemapLeft - 1,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 1,
                            tilemapLeft,
                            tilemapTop - 1,
                            width,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 2,
                            tilemapLeft + width,
                            tilemapTop - 1,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 3,
                            tilemapLeft - 1,
                            tilemapTop,
                            1,
                            height,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 5,
                            tilemapLeft + width,
                            tilemapTop,
                            1,
                            height,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 6,
                            tilemapLeft - 1,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 7,
                            tilemapLeft,
                            tilemapTop + height,
                            width,
                            1,
                            sPaletteNum);
    FillBgTilemapBufferRect(bg,
                            sTileNum + 8,
                            tilemapLeft + width,
                            tilemapTop + height,
                            1,
                            1,
                            sPaletteNum);
}

void ClearStdWindowAndFrameToTransparent(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, WindowFunc_ClearStdWindowAndFrameToTransparent);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void WindowFunc_ClearStdWindowAndFrameToTransparent(u8 bg, u8 tilemapLeft, u8 tilemapTop, u8 width, u8 height, u8 paletteNum)
{
    FillBgTilemapBufferRect(bg, 0, tilemapLeft - 1, tilemapTop - 1, width + 2, height + 2, 0);
}

// Creates the window used to display the info bar at the top of the HOF PC that shows the controls and team number.
u8 HofPCTopBar_AddWindow(u8 bg, u8 xPos, u8 yPos, u8 palette, u16 baseTile)
{
    struct WindowTemplate window;
    memset(&window, 0, sizeof(window));

    if (bg > 3)
        window.bg = 0;
    else
        window.bg = bg;

    window.tilemapTop = yPos;
    window.height = 2;
    window.tilemapLeft = 30 - xPos;
    window.width = xPos;
    window.paletteNum = palette;
    window.baseBlock = baseTile;

    sHofPCTopBarWindowId = AddWindow(&window);

    if (palette > 15)
        palette = BG_PLTT_ID(15);
    else
        palette = BG_PLTT_ID(palette);

    LoadPalette(sHofPC_TopBar_Pal, palette, sizeof(sHofPC_TopBar_Pal));
    return sHofPCTopBarWindowId;
}

// All the below functions checking WINDOW_NONE only handle failure of AddWindow in the above function.
// Because sHofPCTopBarWindowId is not initialized to WINDOW_NONE anywhere it does not handle
// the window not having been drawn yet.
void HofPCTopBar_Print(const u8 *string, u8 left, bool8 copyToVram)
{
    u16 width = 0;

    if (sHofPCTopBarWindowId != WINDOW_NONE)
    {
        PutWindowTilemap(sHofPCTopBarWindowId);
        FillWindowPixelBuffer(sHofPCTopBarWindowId, PIXEL_FILL(15));
        width = GetStringWidth(FONT_SMALL, string, 0);
        AddTextPrinterParameterized3(sHofPCTopBarWindowId,
                  FONT_SMALL,
                  236 - (GetWindowAttribute(sHofPCTopBarWindowId, WINDOW_TILEMAP_LEFT) * 8) - left - width,
                  1,
                  sTextColors,
                  0,
                  string);
        if (copyToVram)
            CopyWindowToVram(sHofPCTopBarWindowId, COPYWIN_FULL);
    }
}

void HofPCTopBar_PrintPair(const u8 *string, const u8 *string2, bool8 noBg, u8 left, bool8 copyToVram)
{
    u8 color[3];
    u16 width = 0;

    if (sHofPCTopBarWindowId != WINDOW_NONE)
    {
        if (noBg)
        {
            color[0] = TEXT_COLOR_TRANSPARENT;
            color[1] = TEXT_COLOR_WHITE;
            color[2] = TEXT_COLOR_DARK_GRAY;
        }
        else
        {
            color[0] = TEXT_DYNAMIC_COLOR_6;
            color[1] = TEXT_COLOR_WHITE;
            color[2] = TEXT_COLOR_DARK_GRAY;
        }
        PutWindowTilemap(sHofPCTopBarWindowId);
        FillWindowPixelBuffer(sHofPCTopBarWindowId, PIXEL_FILL(15));
        if (string2 != NULL)
        {
            width = GetStringWidth(FONT_SMALL, string2, 0);
            AddTextPrinterParameterized3(sHofPCTopBarWindowId,
                      FONT_SMALL,
                      236 - (GetWindowAttribute(sHofPCTopBarWindowId, WINDOW_TILEMAP_LEFT) * 8) - left - width,
                      1,
                      color,
                      0,
                      string2);
        }
        AddTextPrinterParameterized4(sHofPCTopBarWindowId, FONT_NORMAL, 4, 1, 0, 0, color, 0, string);
        if (copyToVram)
            CopyWindowToVram(sHofPCTopBarWindowId, COPYWIN_FULL);
    }
}

static void UNUSED HofPCTopBar_CopyToVram(void)
{
    if (sHofPCTopBarWindowId != WINDOW_NONE)
        CopyWindowToVram(sHofPCTopBarWindowId, COPYWIN_FULL);
}

static void UNUSED HofPCTopBar_Clear(void)
{
    if (sHofPCTopBarWindowId != WINDOW_NONE)
    {
        FillWindowPixelBuffer(sHofPCTopBarWindowId, PIXEL_FILL(15));
        CopyWindowToVram(sHofPCTopBarWindowId, COPYWIN_FULL);
    }
}

void HofPCTopBar_RemoveWindow(void)
{
    if (sHofPCTopBarWindowId != WINDOW_NONE)
    {
        FillWindowPixelBuffer(sHofPCTopBarWindowId, PIXEL_FILL(0));
        ClearWindowTilemap(sHofPCTopBarWindowId);
        CopyWindowToVram(sHofPCTopBarWindowId, COPYWIN_FULL);
        RemoveWindow(sHofPCTopBarWindowId);
        sHofPCTopBarWindowId = WINDOW_NONE;
    }
}

static u8 InitMenu(u8 windowId, u8 fontId, u8 left, u8 top, u8 cursorHeight, u8 numChoices, u8 initialCursorPos, bool8 muteAPress)
{
    s32 pos;

    sMenu.left = left;
    sMenu.top = top;
    sMenu.minCursorPos = 0;
    sMenu.maxCursorPos = numChoices - 1;
    sMenu.windowId = windowId;
    sMenu.fontId = fontId;
    sMenu.optionHeight = cursorHeight;
    sMenu.APressMuted = muteAPress;

    pos = initialCursorPos;

    if (pos < 0 || pos > sMenu.maxCursorPos)
        sMenu.cursorPos = 0;
    else
        sMenu.cursorPos = pos;

    Menu_MoveCursor(0);
    return sMenu.cursorPos;
}

// There is no muted version of this, so the version that plays sound when A is pressed is the "Normal" one.
u8 InitMenuNormal(u8 windowId, u8 fontId, u8 left, u8 top, u8 cursorHeight, u8 numChoices, u8 initialCursorPos)
{
    return InitMenu(windowId, fontId, left, top, cursorHeight, numChoices, initialCursorPos, FALSE);
}

static u8 UNUSED InitMenuDefaultCursorHeight(u8 windowId, u8 fontId, u8 left, u8 top, u8 numChoices, u8 initialCursorPos)
{
    u8 cursorHeight = GetMenuCursorDimensionByFont(fontId, 1);
    return InitMenuNormal(windowId, fontId, left, top, cursorHeight, numChoices, initialCursorPos);
}

void RedrawMenuCursor(u8 oldPos, u8 newPos)
{
    u8 width, height;

    width = GetMenuCursorDimensionByFont(sMenu.fontId, 0);
    height = GetMenuCursorDimensionByFont(sMenu.fontId, 1);
    FillWindowPixelRect(sMenu.windowId, PIXEL_FILL(1), sMenu.left, sMenu.optionHeight * oldPos + sMenu.top, width, height);
    AddTextPrinterParameterized(sMenu.windowId, sMenu.fontId, gText_SelectorArrow3, sMenu.left, sMenu.optionHeight * newPos + sMenu.top, 0, 0);
}

u8 Menu_MoveCursor(s8 cursorDelta)
{
    u8 oldPos = sMenu.cursorPos;
    int newPos = sMenu.cursorPos + cursorDelta;

    if (newPos < sMenu.minCursorPos)
        sMenu.cursorPos = sMenu.maxCursorPos;
    else if (newPos > sMenu.maxCursorPos)
        sMenu.cursorPos = sMenu.minCursorPos;
    else
        sMenu.cursorPos += cursorDelta;

    RedrawMenuCursor(oldPos, sMenu.cursorPos);
    return sMenu.cursorPos;
}

u8 Menu_MoveCursorNoWrapAround(s8 cursorDelta)
{
    u8 oldPos = sMenu.cursorPos;
    int newPos = sMenu.cursorPos + cursorDelta;

    if (newPos < sMenu.minCursorPos)
        sMenu.cursorPos = sMenu.minCursorPos;
    else if (newPos > sMenu.maxCursorPos)
        sMenu.cursorPos = sMenu.maxCursorPos;
    else
        sMenu.cursorPos += cursorDelta;

    RedrawMenuCursor(oldPos, sMenu.cursorPos);
    return sMenu.cursorPos;
}

u8 Menu_GetCursorPos(void)
{
    return sMenu.cursorPos;
}

s8 Menu_ProcessInput(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (!sMenu.APressMuted)
            PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        PlaySE(SE_SELECT);
        Menu_MoveCursor(-1);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        Menu_MoveCursor(1);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

s8 Menu_ProcessInputNoWrap(void)
{
    u8 oldPos = sMenu.cursorPos;

    if (JOY_NEW(A_BUTTON))
    {
        if (!sMenu.APressMuted)
            PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (oldPos != Menu_MoveCursorNoWrapAround(-1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (oldPos != Menu_MoveCursorNoWrapAround(1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

s8 ProcessMenuInput_other(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (!sMenu.APressMuted)
            PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if ((JOY_REPEAT(DPAD_ANY)) == DPAD_UP)
    {
        PlaySE(SE_SELECT);
        Menu_MoveCursor(-1);
        return MENU_NOTHING_CHOSEN;
    }
    else if ((JOY_REPEAT(DPAD_ANY)) == DPAD_DOWN)
    {
        PlaySE(SE_SELECT);
        Menu_MoveCursor(1);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

s8 Menu_ProcessInputNoWrapAround_other(void)
{
    u8 oldPos = sMenu.cursorPos;

    if (JOY_NEW(A_BUTTON))
    {
        if (!sMenu.APressMuted)
            PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_UP)
    {
        if (oldPos != Menu_MoveCursorNoWrapAround(-1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_DOWN)
    {
        if (oldPos != Menu_MoveCursorNoWrapAround(1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

void PrintMenuActionTextsAtPos(u8 windowId, u8 fontId, u8 left, u8 top, u8 lineHeight, u8 itemCount, const struct MenuAction *menuActions)
{
    u8 i;
    for (i = 0; i < itemCount; i++)
        AddTextPrinterParameterized(windowId, fontId, menuActions[i].text, left, (lineHeight * i) + top, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionTextsWithSpacing(u8 windowId, u8 fontId, u8 left, u8 top, u8 lineHeight, u8 itemCount, const struct MenuAction *menuActions, u8 letterSpacing, u8 lineSpacing)
{
    u8 i;
    for (i = 0; i < itemCount; i++)
        AddTextPrinterParameterized5(windowId, fontId, menuActions[i].text, left, (lineHeight * i) + top, TEXT_SKIP_DRAW, NULL, letterSpacing, lineSpacing);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionTextsAtTop(u8 windowId, u8 fontId, u8 lineHeight, u8 itemCount, const struct MenuAction *menuActions)
{
    PrintMenuActionTextsAtPos(windowId, fontId, GetFontAttribute(fontId, FONTATTR_MAX_LETTER_WIDTH), 1, lineHeight, itemCount, menuActions);
}

void PrintMenuActionTexts(u8 windowId, u8 fontId, u8 left, u8 top, u8 letterSpacing, u8 lineHeight, u8 itemCount, const struct MenuAction *menuActions, const u8 *actionIds)
{
    u8 i;
    struct TextPrinterTemplate printer;

    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.fgColor = GetFontAttribute(fontId, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(fontId, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(fontId, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(fontId, FONTATTR_UNKNOWN);
    printer.letterSpacing = letterSpacing;
    printer.lineSpacing = GetFontAttribute(fontId, FONTATTR_LINE_SPACING);
    printer.x = left;
    printer.currentX = left;

    for (i = 0; i < itemCount; i++)
    {
        printer.currentChar = menuActions[actionIds[i]].text;
        printer.y = (lineHeight * i) + top;
        printer.currentY = printer.y;
        AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionTextsAtTopById(u8 windowId, u8 fontId, u8 lineHeight, u8 itemCount, const struct MenuAction *menuActions, const u8 *actionIds)
{
    PrintMenuActionTexts(windowId, fontId, GetFontAttribute(fontId, FONTATTR_MAX_LETTER_WIDTH), 1, GetFontAttribute(fontId, FONTATTR_LETTER_SPACING), lineHeight, itemCount, menuActions, actionIds);
}

void SetWindowTemplateFields(struct WindowTemplate *template, u8 bg, u8 left, u8 top, u8 width, u8 height, u8 paletteNum, u16 baseBlock)
{
    template->bg = bg;
    template->tilemapLeft = left;
    template->tilemapTop = top;
    template->width = width;
    template->height = height;
    template->paletteNum = paletteNum;
    template->baseBlock = baseBlock;
}

struct WindowTemplate CreateWindowTemplate(u8 bg, u8 left, u8 top, u8 width, u8 height, u8 paletteNum, u16 baseBlock)
{
    struct WindowTemplate template;
    SetWindowTemplateFields(&template, bg, left, top, width, height, paletteNum, baseBlock);
    return template;
}

u16 AddWindowParameterized(u8 bg, u8 left, u8 top, u8 width, u8 height, u8 paletteNum, u16 baseBlock)
{
    struct WindowTemplate template;
    SetWindowTemplateFields(&template, bg, left, top, width, height, paletteNum, baseBlock);
    return AddWindow(&template);
}

// As opposed to CreateYesNoMenu, which has a hard-coded position.
static void CreateYesNoMenuAtPos(const struct WindowTemplate *window, u8 fontId, u8 left, u8 top, u16 baseTileNum, u8 paletteNum, u8 initialCursorPos)
{
    struct TextPrinterTemplate printer;

    sYesNoWindowId = AddWindow(window);
    DrawStdFrameWithCustomTileAndPalette(sYesNoWindowId, TRUE, baseTileNum, paletteNum);

    printer.currentChar = gText_YesNo;
    printer.windowId = sYesNoWindowId;
    printer.fontId = fontId;
    printer.x = GetFontAttribute(fontId, FONTATTR_MAX_LETTER_WIDTH) + left;
    printer.y = top;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.fgColor = GetFontAttribute(fontId, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(fontId, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(fontId, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(fontId, FONTATTR_UNKNOWN);
    printer.letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
    printer.lineSpacing = GetFontAttribute(fontId, FONTATTR_LINE_SPACING);

    AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);

    InitMenuNormal(sYesNoWindowId, fontId, left, top, GetFontAttribute(fontId, FONTATTR_MAX_LETTER_HEIGHT), 2, initialCursorPos);
}

static void UNUSED CreateYesNoMenuInTopLeft(const struct WindowTemplate *window, u8 fontId, u16 baseTileNum, u8 paletteNum)
{
    CreateYesNoMenuAtPos(window, fontId, 0, 1, baseTileNum, paletteNum, 0);
}

s8 Menu_ProcessInputNoWrapClearOnChoose(void)
{
    s8 result = Menu_ProcessInputNoWrap();
    if (result != MENU_NOTHING_CHOSEN)
        EraseYesNoWindow();
    return result;
}

void EraseYesNoWindow(void)
{
    ClearStdWindowAndFrameToTransparent(sYesNoWindowId, TRUE);
    RemoveWindow(sYesNoWindowId);
}

static void PrintMenuActionGridText(u8 windowId, u8 fontId, u8 left, u8 top, u8 width, u8 height, u8 columns, u8 rows, const struct MenuAction *menuActions)
{
    u8 i;
    u8 j;
    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < columns; j++)
            AddTextPrinterParameterized(windowId, fontId, menuActions[(i * columns) + j].text, (width * j) + left, (height * i) + top, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionGridTextAtTop(u8 windowId, u8 fontId, u8 width, u8 height, u8 columns, u8 rows, const struct MenuAction *menuActions)
{
    PrintMenuActionGridText(windowId, fontId, GetFontAttribute(fontId, FONTATTR_MAX_LETTER_WIDTH), 0, width, height, columns, rows, menuActions);
}

void PrintMenuActionGrid(u8 windowId, u8 fontId, u8 left, u8 top, u8 optionWidth, u8 horizontalCount, u8 verticalCount, const struct MenuAction *menuActions, const u8 *actionIds)
{
    u8 i;
    u8 j;
    struct TextPrinterTemplate printer;

    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.fgColor = GetFontAttribute(fontId, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(fontId, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(fontId, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(fontId, FONTATTR_UNKNOWN);
    printer.letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
    printer.lineSpacing = GetFontAttribute(fontId, FONTATTR_LINE_SPACING);

    for (i = 0; i < verticalCount; i++)
    {
        for (j = 0; j < horizontalCount; j++)
        {
            printer.currentChar = menuActions[actionIds[(horizontalCount * i) + j]].text;
            printer.x = (optionWidth * j) + left;
            printer.y = (GetFontAttribute(fontId, FONTATTR_MAX_LETTER_HEIGHT) * i) + top;
            printer.currentX = printer.x;
            printer.currentY = printer.y;
            AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
        }
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionGrid_TopLeft(u8 windowId, u8 fontId, u8 optionWidth, u8 unused, u8 horizontalCount, u8 verticalCount, const struct MenuAction *menuActions, const u8 *actionIds)
{
    PrintMenuActionGrid(windowId, fontId, GetFontAttribute(fontId, FONTATTR_MAX_LETTER_WIDTH), 0, optionWidth, horizontalCount, verticalCount, menuActions, actionIds);
}

static u8 InitMenuGrid(u8 windowId, u8 fontId, u8 left, u8 top, u8 optionWidth, u8 optionHeight, u8 columns, u8 rows, u8 numChoices, u8 cursorPos)
{
    s32 pos;

    sMenu.left = left;
    sMenu.top = top;
    sMenu.minCursorPos = 0;
    sMenu.maxCursorPos = numChoices - 1;
    sMenu.windowId = windowId;
    sMenu.fontId = fontId;
    sMenu.optionWidth = optionWidth;
    sMenu.optionHeight = optionHeight;
    sMenu.columns = columns;
    sMenu.rows = rows;

    pos = cursorPos;

    if (pos < 0 || pos > sMenu.maxCursorPos)
        sMenu.cursorPos = 0;
    else
        sMenu.cursorPos = pos;

    // Why call this when it's not gonna move?
    ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_NONE);
    return sMenu.cursorPos;
}

static u8 UNUSED InitMenuGridDefaultCursorHeight(u8 windowId, u8 fontId, u8 left, u8 top, u8 width, u8 columns, u8 rows, u8 cursorPos)
{
    u8 cursorHeight = GetMenuCursorDimensionByFont(fontId, 1);
    u8 numChoices = columns * rows;
    return InitMenuGrid(windowId, fontId, left, top, width, cursorHeight, columns, rows, numChoices, cursorPos);
}

// Erase cursor at old position, draw cursor at new position.
static void MoveMenuGridCursor(u8 oldCursorPos, u8 newCursorPos)
{
    u8 cursorWidth = GetMenuCursorDimensionByFont(sMenu.fontId, 0);
    u8 cursorHeight = GetMenuCursorDimensionByFont(sMenu.fontId, 1);

    u8 xPos = (oldCursorPos % sMenu.columns) * sMenu.optionWidth + sMenu.left;
    u8 yPos = (oldCursorPos / sMenu.columns) * sMenu.optionHeight + sMenu.top;
    FillWindowPixelRect(sMenu.windowId, PIXEL_FILL(1), xPos, yPos, cursorWidth, cursorHeight);

    xPos = (newCursorPos % sMenu.columns) * sMenu.optionWidth + sMenu.left;
    yPos = (newCursorPos / sMenu.columns) * sMenu.optionHeight + sMenu.top;
    AddTextPrinterParameterized(sMenu.windowId, sMenu.fontId, gText_SelectorArrow3, xPos, yPos, 0, 0);
}

u8 ChangeMenuGridCursorPosition(s8 deltaX, s8 deltaY)
{
    u8 oldPos = sMenu.cursorPos;

    if (deltaX != 0)
    {
        if ((sMenu.cursorPos % sMenu.columns) + deltaX < 0)
            sMenu.cursorPos += sMenu.columns - 1;
        else if ((sMenu.cursorPos % sMenu.columns) + deltaX >= sMenu.columns)
            sMenu.cursorPos = (sMenu.cursorPos / sMenu.columns) * sMenu.columns;
        else
            sMenu.cursorPos += deltaX;
    }

    if (deltaY != 0)
    {
        if ((sMenu.cursorPos / sMenu.columns) + deltaY < 0)
            sMenu.cursorPos += sMenu.columns * (sMenu.rows - 1);
        else if ((sMenu.cursorPos / sMenu.columns) + deltaY >= sMenu.rows)
            sMenu.cursorPos -= sMenu.columns * (sMenu.rows - 1);
        else
            sMenu.cursorPos += (sMenu.columns * deltaY);
    }

    if (sMenu.cursorPos > sMenu.maxCursorPos)
    {
        sMenu.cursorPos = oldPos;
        return sMenu.cursorPos;
    }
    else
    {
        MoveMenuGridCursor(oldPos, sMenu.cursorPos);
        return sMenu.cursorPos;
    }
}

u8 ChangeGridMenuCursorPosition(s8 deltaX, s8 deltaY)
{
    u8 oldPos = sMenu.cursorPos;

    if (deltaX != 0)
    {
        if (((sMenu.cursorPos % sMenu.columns) + deltaX >= 0) &&
        ((sMenu.cursorPos % sMenu.columns) + deltaX < sMenu.columns))
        {
            sMenu.cursorPos += deltaX;
        }
    }

    if (deltaY != 0)
    {
        if (((sMenu.cursorPos / sMenu.columns) + deltaY >= 0) &&
        ((sMenu.cursorPos / sMenu.columns) + deltaY < sMenu.rows))
        {
            sMenu.cursorPos += (sMenu.columns * deltaY);
        }
    }

    if (sMenu.cursorPos > sMenu.maxCursorPos)
    {
        sMenu.cursorPos = oldPos;
        return sMenu.cursorPos;
    }
    else
    {
        MoveMenuGridCursor(oldPos, sMenu.cursorPos);
        return sMenu.cursorPos;
    }
}

static s8 UNUSED Menu_ProcessGridInput_NoSoundLimit(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_UP);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_DOWN);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_LEFT, MENU_CURSOR_DELTA_NONE);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_RIGHT, MENU_CURSOR_DELTA_NONE);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

s8 Menu_ProcessGridInput(void)
{
    u8 oldPos = sMenu.cursorPos;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, -1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, 1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(-1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

static s8 UNUSED Menu_ProcessGridInputRepeat_NoSoundLimit(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_UP)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_UP);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_DOWN)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_DOWN);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_LEFT || GetLRKeysPressedAndHeld() == MENU_L_PRESSED)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_LEFT, MENU_CURSOR_DELTA_NONE);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_RIGHT || GetLRKeysPressedAndHeld() == MENU_R_PRESSED)
    {
        PlaySE(SE_SELECT);
        ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_RIGHT, MENU_CURSOR_DELTA_NONE);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

static s8 UNUSED Menu_ProcessGridInputRepeat(void)
{
    u8 oldPos = sMenu.cursorPos;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        return sMenu.cursorPos;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return MENU_B_PRESSED;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_UP)
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, -1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_DOWN)
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, 1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_LEFT || GetLRKeysPressedAndHeld() == MENU_L_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(-1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_REPEAT(DPAD_ANY) == DPAD_RIGHT || GetLRKeysPressedAndHeld() == MENU_R_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}

u8 InitMenuInUpperLeftCorner(u8 windowId, u8 itemCount, u8 initialCursorPos, bool8 APressMuted)
{
    s32 pos;

    sMenu.left = 0;
    sMenu.top = 1;
    sMenu.minCursorPos = 0;
    sMenu.maxCursorPos = itemCount - 1;
    sMenu.windowId = windowId;
    sMenu.fontId = FONT_NORMAL;
    sMenu.optionHeight = 16;
    sMenu.APressMuted = APressMuted;

    pos = initialCursorPos;

    if (pos < 0 || pos > sMenu.maxCursorPos)
        sMenu.cursorPos = 0;
    else
        sMenu.cursorPos = pos;

    return Menu_MoveCursor(0);
}

// There is no muted version of this function, so the version that plays sound when A is pressed is the "Normal" one.
u8 InitMenuInUpperLeftCornerNormal(u8 windowId, u8 itemCount, u8 initialCursorPos)
{
    return InitMenuInUpperLeftCorner(windowId, itemCount, initialCursorPos, FALSE);
}

void PrintMenuTable(u8 windowId, u8 itemCount, const struct MenuAction *menuActions)
{
    u32 i;

    for (i = 0; i < itemCount; i++)
        AddTextPrinterParameterized(windowId, 1, menuActions[i].text, 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

void PrintMenuActionTextsInUpperLeftCorner(u8 windowId, u8 itemCount, const struct MenuAction *menuActions, const u8 *actionIds)
{
    u8 i;
    struct TextPrinterTemplate printer;

    printer.windowId = windowId;
    printer.fontId = FONT_NORMAL;
    printer.fgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(FONT_NORMAL, FONTATTR_UNKNOWN);
    printer.letterSpacing = 0;
    printer.lineSpacing = 0;
    printer.x = 8;
    printer.currentX = 8;

    for (i = 0; i < itemCount; i++)
    {
        printer.currentChar = menuActions[actionIds[i]].text;
        printer.y = (i * 16) + 1;
        printer.currentY = (i * 16) + 1;
        AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

void CreateYesNoMenu(const struct WindowTemplate *window, u16 baseTileNum, u8 paletteNum, u8 initialCursorPos)
{
    struct TextPrinterTemplate printer;

    sYesNoWindowId = AddWindow(window);
    DrawStdFrameWithCustomTileAndPalette(sYesNoWindowId, TRUE, baseTileNum, paletteNum);

    printer.currentChar = gText_YesNo;
    printer.windowId = sYesNoWindowId;
    printer.fontId = FONT_NORMAL;
    printer.x = 8;
    printer.y = 1;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.fgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(FONT_NORMAL, FONTATTR_UNKNOWN);
    printer.letterSpacing = 0;
    printer.lineSpacing = 0;

    AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
    InitMenuInUpperLeftCornerNormal(sYesNoWindowId, 2, initialCursorPos);
}

void PrintMenuGridTable(u8 windowId, u8 optionWidth, u8 columns, u8 rows, const struct MenuAction *menuActions)
{
    u32 i, j;

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < columns; j++)
            AddTextPrinterParameterized(windowId, 1, menuActions[(i * columns) + j].text, (optionWidth * j) + 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void UNUSED PrintMenuActionGridTextNoSpacing(u8 windowId, u8 optionWidth, u8 columns, u8 rows, const struct MenuAction *menuActions, const u8 *actionIds)
{
    u8 i;
    u8 j;
    struct TextPrinterTemplate printer;

    printer.windowId = windowId;
    printer.fontId = FONT_NORMAL;
    printer.fgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_SHADOW);
    printer.unk = GetFontAttribute(FONT_NORMAL, FONTATTR_UNKNOWN);
    printer.letterSpacing = 0;
    printer.lineSpacing = 0;

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < columns; j++)
        {
            printer.currentChar = menuActions[actionIds[(columns * i) + j]].text;
            printer.x = (optionWidth * j) + 8;
            printer.y = (16 * i) + 1;
            printer.currentX = printer.x;
            printer.currentY = printer.y;
            AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
        }
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

u8 InitMenuActionGrid(u8 windowId, u8 optionWidth, u8 columns, u8 rows, u8 initialCursorPos)
{
    s32 pos;

    sMenu.left = 0;
    sMenu.top = 1;
    sMenu.minCursorPos = 0;
    sMenu.maxCursorPos = (columns * rows) - 1;
    sMenu.windowId = windowId;
    sMenu.fontId = FONT_NORMAL;
    sMenu.optionWidth = optionWidth;
    sMenu.optionHeight = 16;
    sMenu.columns = columns;
    sMenu.rows = rows;

    pos = initialCursorPos;

    if (pos < 0 || pos > sMenu.maxCursorPos)
        sMenu.cursorPos = 0;
    else
        sMenu.cursorPos = pos;

    // Why call this when it's not gonna move?
    ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_NONE);
    return sMenu.cursorPos;
}

void ClearScheduledBgCopiesToVram(void)
{
    memset(sScheduledBgCopiesToVram, 0, sizeof(sScheduledBgCopiesToVram));
}

void ScheduleBgCopyTilemapToVram(u8 bgId)
{
    sScheduledBgCopiesToVram[bgId] = TRUE;
}

void DoScheduledBgTilemapCopiesToVram(void)
{
    if (sScheduledBgCopiesToVram[0] == TRUE)
    {
        CopyBgTilemapBufferToVram(0);
        sScheduledBgCopiesToVram[0] = FALSE;
    }
    if (sScheduledBgCopiesToVram[1] == TRUE)
    {
        CopyBgTilemapBufferToVram(1);
        sScheduledBgCopiesToVram[1] = FALSE;
    }
    if (sScheduledBgCopiesToVram[2] == TRUE)
    {
        CopyBgTilemapBufferToVram(2);
        sScheduledBgCopiesToVram[2] = FALSE;
    }
    if (sScheduledBgCopiesToVram[3] == TRUE)
    {
        CopyBgTilemapBufferToVram(3);
        sScheduledBgCopiesToVram[3] = FALSE;
    }
}

void ResetTempTileDataBuffers(void)
{
    int i;
    for (i = 0; i < (int)ARRAY_COUNT(sTempTileDataBuffer); i++)
        sTempTileDataBuffer[i] = NULL;
    sTempTileDataBufferIdx = 0;
}

bool8 FreeTempTileDataBuffersIfPossible(void)
{
    int i;

    if (!IsDma3ManagerBusyWithBgCopy())
    {
        if (sTempTileDataBufferIdx)
        {
            for (i = 0; i < sTempTileDataBufferIdx; i++)
                FREE_AND_SET_NULL(sTempTileDataBuffer[i]);
            sTempTileDataBufferIdx = 0;
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void *DecompressAndCopyTileDataToVram(u8 bgId, const void *src, u32 size, u16 offset, u8 mode)
{
    u32 sizeOut;
    if (sTempTileDataBufferIdx < ARRAY_COUNT(sTempTileDataBuffer))
    {
        void *ptr = malloc_and_decompress(src, &sizeOut);
        if (!size)
            size = sizeOut;
        if (ptr)
        {
            copy_decompressed_tile_data_to_vram(bgId, ptr, size, offset, mode);
            sTempTileDataBuffer[sTempTileDataBufferIdx++] = ptr;
        }
        return ptr;
    }
    return NULL;
}

void DecompressAndLoadBgGfxUsingHeap(u8 bgId, const void *src, u32 size, u16 offset, u8 mode)
{
    u32 sizeOut;
    void *ptr = malloc_and_decompress(src, &sizeOut);
    if (!size)
        size = sizeOut;
    if (ptr)
    {
        u8 taskId = CreateTask(task_free_buf_after_copying_tile_data_to_vram, 0);
        gTasks[taskId].data[0] = copy_decompressed_tile_data_to_vram(bgId, ptr, size, offset, mode);
        SetWordTaskArg(taskId, 1, (u32)ptr);
    }
}

void task_free_buf_after_copying_tile_data_to_vram(u8 taskId)
{
    if (!CheckForSpaceForDma3Request(gTasks[taskId].data[0]))
    {
        Free((void *)GetWordTaskArg(taskId, 1));
        DestroyTask(taskId);
    }
}

void *malloc_and_decompress(const void *src, u32 *size)
{
    void *ptr;
    u8 *sizeAsBytes = (u8 *)size;
    u8 *srcAsBytes = (u8 *)src;

    sizeAsBytes[0] = srcAsBytes[1];
    sizeAsBytes[1] = srcAsBytes[2];
    sizeAsBytes[2] = srcAsBytes[3];
    sizeAsBytes[3] = 0;

    ptr = Alloc(*size);
    if (ptr)
        LZ77UnCompWram(src, ptr);
    return ptr;
}

u16 copy_decompressed_tile_data_to_vram(u8 bgId, const void *src, u16 size, u16 offset, u8 mode)
{
    switch (mode)
    {
        case 0:
            return LoadBgTiles(bgId, src, size, offset);
        case 1:
            return LoadBgTilemap(bgId, src, size, offset);
        default:
            return -1;
    }
}

void SetBgTilemapPalette(u8 bgId, u8 left, u8 top, u8 width, u8 height, u8 palette)
{
    u8 i;
    u8 j;
    u16 *ptr = GetBgTilemapBuffer(bgId);

    for (i = top; i < top + height; i++)
    {
        for (j = left; j < left + width; j++)
        {
            ptr[(i * 32) + j] = (ptr[(i * 32) + j] & 0xFFF) | (palette << 12);
        }
    }
}

void CopyToBufferFromBgTilemap(u8 bgId, u16 *dest, u8 left, u8 top, u8 width, u8 height)
{
    u8 i;
    u8 j;
    const u16 *src = GetBgTilemapBuffer(bgId);

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
            dest[(i * width) + j] = src[(i + top) * 32 + j + left];
    }
}

void AddValToTilemapBuffer(void *ptr, int delta, int width, int height, bool32 isAffine)
{
    int i;
    int area = width * height;
    if (isAffine == TRUE)
    {
        u8 *as8BPP = ptr;
        for (i = 0; i < area; i++)
            as8BPP[i] += delta;
    }
    else
    {
        // Limit add to first 10 bits
        u16 *as4BPP = ptr;
        for (i = 0; i < area; i++)
            as4BPP[i] = (as4BPP[i] & 0xFC00) | ((as4BPP[i] + delta) & 0x3FF);
    }
}

void ResetBgPositions(void)
{
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
}

void BgDmaFill(u32 bg, u8 value, int offset, int size)
{
    int temp = (!GetBgAttribute(bg, BG_ATTR_PALETTEMODE)) ? 32 : 64;
    void *addr = (void *)((GetBgAttribute(bg, BG_ATTR_CHARBASEINDEX) * 0x4000) + (GetBgAttribute(bg, BG_ATTR_BASETILE) + offset) * temp);
    RequestDma3Fill(value << 24 | value << 16 | value << 8 | value, VRAM + addr, size * temp, 1);
}

void AddTextPrinterParameterized3(u8 windowId, u8 fontId, u8 left, u8 top, const u8 *color, s8 speed, const u8 *str)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = left;
    printer.y = top;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
    printer.lineSpacing = GetFontAttribute(fontId, FONTATTR_LINE_SPACING);
    printer.unk = 0;
    printer.fgColor = color[1];
    printer.bgColor = color[0];
    printer.shadowColor = color[2];

    AddTextPrinter(&printer, speed, NULL);
}

void AddTextPrinterParameterized4(u8 windowId, u8 fontId, u8 left, u8 top, u8 letterSpacing, u8 lineSpacing, const u8 *color, s8 speed, const u8 *str)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = left;
    printer.y = top;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.letterSpacing = letterSpacing;
    printer.lineSpacing = lineSpacing;
    printer.unk = 0;
    printer.fgColor = color[1];
    printer.bgColor = color[0];
    printer.shadowColor = color[2];

    AddTextPrinter(&printer, speed, NULL);
}

void AddTextPrinterParameterized5(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16), u8 letterSpacing, u8 lineSpacing)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = left;
    printer.y = top;
    printer.currentX = left;
    printer.currentY = top;
    printer.letterSpacing = letterSpacing;
    printer.lineSpacing = lineSpacing;
    printer.unk = 0;

    printer.fgColor = GetFontAttribute(fontId, FONTATTR_COLOR_FOREGROUND);
    printer.bgColor = GetFontAttribute(fontId, FONTATTR_COLOR_BACKGROUND);
    printer.shadowColor = GetFontAttribute(fontId, FONTATTR_COLOR_SHADOW);

    AddTextPrinter(&printer, speed, callback);
}

void PrintPlayerNameOnWindow(u8 windowId, const u8 *src, u16 x, u16 y)
{
    int count = 0;
    while (gSaveBlock2Ptr->playerName[count] != EOS)
        count++;

    StringExpandPlaceholders(gStringVarBuffer, src);

    AddTextPrinterParameterized(windowId, 1, gStringVarBuffer, x, y, TEXT_SKIP_DRAW, 0);
}

static void UNUSED UnusedBlitBitmapRect(const struct Bitmap *src, struct Bitmap *dst, u16 srcX, u16 srcY, u16 dstX, u16 dstY, u16 width, u16 height)
{
    int loopSrcY, loopDstY, loopSrcX, loopDstX, xEnd, yEnd, multiplierSrcY, multiplierDstY;
    const u8 *pixelsSrc;
    u8 *pixelsDst;
    u16 toOrr;

    if (dst->width - dstX < width)
        xEnd = dst->width - dstX + srcX;
    else
        xEnd = width + srcX;

    if (dst->height - dstY < height)
        yEnd = srcY + dst->height - dstY;
    else
        yEnd = srcY + height;

    multiplierSrcY = (src->width + (src->width % 8)) >> 3;
    multiplierDstY = (dst->width + (dst->width % 8)) >> 3;

    for (loopSrcY = srcY, loopDstY = dstY; loopSrcY < yEnd; loopSrcY++, loopDstY++)
    {
        for (loopSrcX = srcX, loopDstX = dstX; loopSrcX < xEnd; loopSrcX++, loopDstX++)
        {
            pixelsSrc = src->pixels + ((loopSrcX >> 1) & 3) + ((loopSrcX >> 3) << 5) + (((loopSrcY >> 3) * multiplierSrcY) << 5) + ((u32)(loopSrcY << 29) >> 27);
            pixelsDst = (void *) dst->pixels + ((loopDstX >> 1) & 3) + ((loopDstX >> 3) << 5) + ((( loopDstY >> 3) * multiplierDstY) << 5) + ((u32)(loopDstY << 29) >> 27);

            if ((uintptr_t)pixelsDst & 1)
            {
                pixelsDst--;
                if (loopDstX & 1)
                {
                    toOrr = *(vu16 *)pixelsDst;
                    toOrr &= 0x0fff;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 8);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 12);
                }
                else
                {
                    toOrr = *(vu16 *)pixelsDst;
                    toOrr &= 0xf0ff;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 4);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 8);
                }
            }
            else
            {
                if (loopDstX & 1)
                {
                    toOrr = *(vu16 *)pixelsDst;
                    toOrr &= 0xff0f;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 0);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 4);
                }
                else
                {
                    toOrr = *(vu16 *)pixelsDst;
                    toOrr &= 0xfff0;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) >> 4);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) >> 0);
                }
            }
            *(vu16 *)pixelsDst = toOrr;
        }
    }
}

static void UNUSED LoadMonIconPalAtOffset(u8 palOffset, u16 speciesId)
{
    LoadPalette(GetValidMonIconPalettePtr(speciesId), palOffset, PLTT_SIZE_4BPP);
}

static void UNUSED DrawMonIconAtPos(u8 windowId, u16 speciesId, u32 personality, u16 x, u16 y)
{
    BlitBitmapToWindow(windowId, GetMonIconPtr(speciesId, personality, 1), x, y, 32, 32);
}

void ListMenuLoadStdPalAt(u8 palOffset, u8 palId)
{
    const u16 *palette;

    switch (palId)
    {
        case 0:
        default:
            palette = gMenuInfoElements1_Pal;
            break;
        case 1:
            palette = gMenuInfoElements2_Pal;
            break;
        case 2:
            palette = gMenuInfoElements3_Pal;
            break;
    }

    LoadPalette(palette, palOffset, PLTT_SIZE_4BPP);
}

void BlitMenuInfoIcon(u8 windowId, u8 iconId, u16 x, u16 y)
{
    BlitBitmapRectToWindow(windowId, &gMenuInfoElements_Gfx[sMenuInfoIcons[iconId].offset * 32], 0, 0, 128, 128, x, y, sMenuInfoIcons[iconId].width, sMenuInfoIcons[iconId].height);
}

void BufferSaveMenuText(u8 textId, u8 *dest, u8 color)
{
    s32 curFlag;
    s32 flagCount;
    u8 *endOfString;
    u8 *string = dest;

    *(string++) = EXT_CTRL_CODE_BEGIN;
    *(string++) = EXT_CTRL_CODE_COLOR;
    *(string++) = color;
    *(string++) = EXT_CTRL_CODE_BEGIN;
    *(string++) = EXT_CTRL_CODE_SHADOW;
    *(string++) = color + 1;

    switch (textId)
    {
        case SAVE_MENU_NAME:
            StringCopy(string, gSaveBlock2Ptr->playerName);
            break;
        case SAVE_MENU_CAUGHT:
            if (IsNationalPokedexEnabled())
                string = ConvertIntToDecimalStringN(string, GetNationalPokedexCount(FLAG_GET_CAUGHT), STR_CONV_MODE_LEFT_ALIGN, 3);
            else
                string = ConvertIntToDecimalStringN(string, GetHoennPokedexCount(FLAG_GET_CAUGHT), STR_CONV_MODE_LEFT_ALIGN, 3);
            *string = EOS;
            break;
        case SAVE_MENU_PLAY_TIME:
            string = ConvertIntToDecimalStringN(string, gSaveBlock2Ptr->playTimeHours, STR_CONV_MODE_LEFT_ALIGN, 3);
            *(string++) = CHAR_COLON;
            ConvertIntToDecimalStringN(string, gSaveBlock2Ptr->playTimeMinutes, STR_CONV_MODE_LEADING_ZEROS, 2);
            break;
        case SAVE_MENU_LOCATION:
            GetMapNameGeneric(string, gMapHeader.regionMapSectionId);
            break;
        case SAVE_MENU_BADGES:
            for (curFlag = FLAG_BADGE01_GET, flagCount = 0, endOfString = string + 1; curFlag < FLAG_BADGE01_GET + NUM_BADGES; curFlag++)
            {
                if (FlagGet(curFlag))
                    flagCount++;
            }
            *string = flagCount + CHAR_0;
            *endOfString = EOS;
            break;
    }
}
