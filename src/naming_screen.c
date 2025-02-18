#include "global.h"
#include "naming_screen.h"
#include "malloc.h"
#include "palette.h"
#include "task.h"
#include "sprite.h"
#include "string_util.h"
#include "window.h"
#include "bg.h"
#include "gpu_regs.h"
#include "pokemon.h"
#include "field_specials.h"
#include "field_player_avatar.h"
#include "event_object_movement.h"
#include "event_data.h"
#include "constants/songs.h"
#include "pokemon_storage_system.h"
#include "graphics.h"
#include "sound.h"
#include "trig.h"
#include "field_effect.h"
#include "pokemon_icon.h"
#include "data.h"
#include "strings.h"
#include "menu.h"
#include "text_window.h"
#include "overworld.h"
#include "walda_phrase.h"
#include "main.h"
#include "constants/event_objects.h"
#include "constants/rgb.h"

enum {
    INPUT_NONE,
    INPUT_DPAD_UP,
    INPUT_DPAD_DOWN,
    INPUT_DPAD_LEFT,
    INPUT_DPAD_RIGHT,
    INPUT_A_BUTTON,
    INPUT_B_BUTTON,
    INPUT_LR_BUTTON,
    INPUT_SELECT,
    INPUT_START,
};

#define KBROW_COUNT 4
#define KBCOL_COUNT 8

enum {
    GFXTAG_BACK_BUTTON,
    GFXTAG_OK_BUTTON,
    GFXTAG_PAGE_SWAP_FRAME,
    GFXTAG_PAGE_SWAP_BUTTON,
    GFXTAG_PAGE_SWAP_UPPER,
    GFXTAG_PAGE_SWAP_LOWER,
    GFXTAG_PAGE_SWAP_OTHERS,
    GFXTAG_CURSOR,
    GFXTAG_CURSOR_SQUISHED,
    GFXTAG_CURSOR_FILLED,
    GFXTAG_INPUT_ARROW,
    GFXTAG_UNDERSCORE,
};

enum {
    PALTAG_MENU, // Also the PC icon
    PALTAG_PAGE_SWAP_UPPER,
    PALTAG_PAGE_SWAP_LOWER,
    PALTAG_PAGE_SWAP_OTHERS, // Also the input arrow/underscore
    PALTAG_PAGE_SWAP,
    PALTAG_CURSOR,
    PALTAG_BACK_BUTTON,
    PALTAG_OK_BUTTON,
};

enum {
    WIN_KB_PAGE_1, // Which of these two windows is in front is cycled as the player swaps
    WIN_KB_PAGE_2, // Initially WIN_KB_PAGE_1 is in front, with WIN_KB_PAGE_2 on deck
    WIN_TEXT_ENTRY,
    WIN_TEXT_ENTRY_BOX,
    WIN_BANNER,
    WIN_COUNT,
};

// The constants for the pages are needlessly complicated because GF didn't keep the indexing order consistent
// This set is used for sNamingScreen->currentPage. It uses the order that the pages are cycled in
enum {
    KBPAGE_SYMBOLS,
    KBPAGE_LETTERS_UPPER,
    KBPAGE_LETTERS_LOWER,
    KBPAGE_COUNT,
};

// This set is used for initializing a page's keyboard text and getting its number of columns
enum {
    KEYBOARD_LETTERS_LOWER,
    KEYBOARD_LETTERS_UPPER,
    KEYBOARD_SYMBOLS,
};

// This set is used for getting the gfx/pal tags of the page's swap button
enum {
    PAGE_SWAP_UPPER,
    PAGE_SWAP_OTHERS,
    PAGE_SWAP_LOWER,
};

enum {
    KEY_ROLE_CHAR,
    KEY_ROLE_PAGE,
    KEY_ROLE_BACKSPACE,
    KEY_ROLE_OK,
};

enum {
    BUTTON_PAGE,
    BUTTON_BACK,
    BUTTON_OK,
    BUTTON_COUNT,
};

// states for Task_NamingScreen
enum {
    STATE_FADE_IN,
    STATE_WAIT_FADE_IN,
    STATE_HANDLE_INPUT,
    STATE_MOVE_TO_OK_BUTTON,
    STATE_START_PAGE_SWAP,
    STATE_WAIT_PAGE_SWAP,
    STATE_PRESSED_OK,
    STATE_WAIT_SENT_TO_PC_MESSAGE,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// sates for Task_HandleInput
enum
{
    INPUT_STATE_DISABLED,
    INPUT_STATE_ENABLED,
    INPUT_STATE_OVERRIDE,
};

struct NamingScreenTemplate
{
    u8 copyExistingString;
    u8 maxChars;
    u8 iconFunction;
    u8 addGenderIcon;
    u8 initialPage;
    u8 unused;
    const u8 *title;
};

struct NamingScreenData
{
    u8 tilemapBuffer1[0x800];
    u8 tilemapBuffer2[0x800];
    u8 tilemapBuffer3[0x800];
    u8 textBuffer[16];
    u8 tileBuffer[0x600];
    u8 state;
    u8 windows[WIN_COUNT];
    u16 inputCharBaseXPos;
    u16 bg1vOffset;
    u16 bg2vOffset;
    u16 bg1Priority;
    u16 bg2Priority;
    u8 bgToReveal;
    u8 bgToHide;
    u8 currentPage;
    u8 cursorSpriteId;
    u8 swapBtnFrameSpriteId;
    u8 keyRepeatStartDelayCopy;
    const struct NamingScreenTemplate *template;
    u8 templateNum;
    u8 *destBuffer;
    u16 monSpecies;
    u16 monGender;
    u32 monPersonality;
    MainCallback returnCallback;
};

EWRAM_DATA static struct NamingScreenData *sNamingScreen = NULL;

static const u8 sPCIconOff_Gfx[] = INCBIN_U8("graphics/naming_screen/pc_icon_off.4bpp");
static const u8 sPCIconOn_Gfx[] = INCBIN_U8("graphics/naming_screen/pc_icon_on.4bpp");
static const u16 sKeyboard_Pal[] = INCBIN_U16("graphics/naming_screen/keyboard.gbapal");
static const u16 sRival_Pal[] = INCBIN_U16("graphics/naming_screen/rival.gbapal"); // Unused, leftover from FRLG rival

static const u8 *const sTransferredToPCMessages[] =
{
    gText_PkmnTransferredSomeonesPC,
    gText_PkmnTransferredLanettesPC,
    gText_PkmnTransferredSomeonesPCBoxFull,
    gText_PkmnTransferredLanettesPCBoxFull
};

static const u8 sText_AlphabetUpperLower[] = _("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!");

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .priority = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .priority = 1
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .priority = 2
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .priority = 3
    }
};

static const struct WindowTemplate sWindowTemplates[WIN_COUNT + 1] =
{
    [WIN_KB_PAGE_1] = {
        .bg = 1,
        .tilemapLeft = 3,
        .tilemapTop = 10,
        .width = 19,
        .height = 8,
        .paletteNum = 10,
        .baseBlock = 0x030
    },
    [WIN_KB_PAGE_2] = {
        .bg = 2,
        .tilemapLeft = 3,
        .tilemapTop = 10,
        .width = 19,
        .height = 8,
        .paletteNum = 10,
        .baseBlock = 0x0C8
    },
    [WIN_TEXT_ENTRY] = {
        .bg = 3,
        .tilemapLeft = 8,
        .tilemapTop = 6,
        .width = 17,
        .height = 2,
        .paletteNum = 10,
        .baseBlock = 0x030
    },
    [WIN_TEXT_ENTRY_BOX] = {
        .bg = 3,
        .tilemapLeft = 8,
        .tilemapTop = 4,
        .width = 17,
        .height = 2,
        .paletteNum = 10,
        .baseBlock = 0x052
    },
    [WIN_BANNER] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = DISPLAY_TILE_WIDTH,
        .height = 2,
        .paletteNum = 11,
        .baseBlock = 0x074
    },
    DUMMY_WIN_TEMPLATE
};

// This handles what characters get inserted when a key is pressed
// The keys shown on the keyboard are handled separately by sNamingScreenKeyboardText
static const u8 sKeyboardChars[KBPAGE_COUNT][KBROW_COUNT][KBCOL_COUNT] = {
    [KEYBOARD_LETTERS_LOWER] = {
        __("abcdef ."),
        __("ghijkl ,"),
        __("mnopqrs "),
        __("tuvwxyz "),
    },
    [KEYBOARD_LETTERS_UPPER] = {
        __("ABCDEF ."),
        __("GHIJKL ,"),
        __("MNOPQRS "),
        __("TUVWXYZ "),
    },
    [KEYBOARD_SYMBOLS] = {
        __("01234   "),
        __("56789   "),
        __("!?♂♀/-  "),
        __("…“”‘'   "),
    }
};

static const u8 sPageColumnCounts[KBPAGE_COUNT] = {
    [KEYBOARD_LETTERS_LOWER] = KBCOL_COUNT,
    [KEYBOARD_LETTERS_UPPER] = KBCOL_COUNT,
    [KEYBOARD_SYMBOLS]       = 6
};
static const u8 sPageColumnXPos[KBPAGE_COUNT][KBCOL_COUNT] = {
    [KEYBOARD_LETTERS_LOWER] = {0, 12, 24, 56, 68, 80, 92, 123},
    [KEYBOARD_LETTERS_UPPER] = {0, 12, 24, 56, 68, 80, 92, 123},
    [KEYBOARD_SYMBOLS]       = {0, 22, 44, 66, 88, 110}
};

static const struct NamingScreenTemplate *const sNamingScreenTemplates[];
static const struct SubspriteTable sSubspriteTable_PageSwapFrame[];
static const struct SubspriteTable sSubspriteTable_PageSwapText[];
static const struct SubspriteTable sSubspriteTable_Button[];
static const struct SubspriteTable sSubspriteTable_PCIcon[];
static const struct SpriteTemplate sSpriteTemplate_PageSwapFrame;
static const struct SpriteTemplate sSpriteTemplate_PageSwapButton;
static const struct SpriteTemplate sSpriteTemplate_PageSwapText;
static const struct SpriteTemplate sSpriteTemplate_BackButton;
static const struct SpriteTemplate sSpriteTemplate_OkButton;
static const struct SpriteTemplate sSpriteTemplate_Cursor;
static const struct SpriteTemplate sSpriteTemplate_InputArrow;
static const struct SpriteTemplate sSpriteTemplate_Underscore;
static const struct SpriteTemplate sSpriteTemplate_PCIcon;
static const u8 *const sNamingScreenKeyboardText[KBPAGE_COUNT][KBROW_COUNT];
static const struct SpriteSheet sSpriteSheets[];
static const struct SpritePalette sSpritePalettes[];

static void CB2_LoadNamingScreen(void);
static void NamingScreen_Init(void);
static void NamingScreen_InitBGs(void);
static void CreateNamingScreenTask(void);
static void Task_NamingScreen(u8 taskId);
static bool8 MainState_FadeIn(void);
static bool8 MainState_WaitFadeIn(void);
static bool8 MainState_HandleInput(void);
static bool8 MainState_MoveToOKButton(void);
static bool8 MainState_PressedOKButton(void);
static bool8 MainState_FadeOut(void);
static bool8 MainState_Exit(void);
static void DisplaySentToPCMessage(void);
static bool8 MainState_WaitSentToPCMessage(void);
static bool8 MainState_StartPageSwap(void);
static bool8 MainState_WaitPageSwap(void);
static void StartPageSwapAnim(void);
static void Task_HandlePageSwapAnim(u8);
static bool8 IsPageSwapAnimNotInProgress(void);
static void TryStartButtonFlash(u8, bool8, bool8);
static void Task_UpdateButtonFlash(u8);
static u16 GetButtonPalOffset(u8);
static void RestoreButtonColor(u8);
static void StartButtonFlash(struct Task *, u8, bool8);
static void CreateSprites(void);
static void CreateCursorSprite(void);
static void SetCursorPos(s16, s16);
static void GetCursorPos(s16 *x, s16 *y);
static void MoveCursorToOKButton(void);
static void SetCursorInvisibility(u8);
static void SetCursorFlashing(bool8);
static u8 IsCursorAnimFinished(void);
static u8 GetCurrentPageColumnCount(void);
static void CreatePageSwapButtonSprites(void);
static void StartPageSwapButtonAnim(void);
static void SetPageSwapButtonGfx(u8, struct Sprite *, struct Sprite *);
static void CreateBackOkSprites(void);
static void CreateTextEntrySprites(void);
static void CreateInputTargetIcon(void);
static u8 HandleKeyboardEvent(void);
static u8 SwapKeyboardPage(void);
static u8 GetInputEvent(void);
static void SetInputState(u8);
static void DrawTextEntryBox(void);
static u8 GetTextEntryPosition(void);
static void DeleteTextCharacter(void);
static bool8 AddTextCharacter(void);
static void BufferCharacter(u8);
static void SaveInputText(void);
static void LoadGfx(void);
static void CreateHelperTasks(void);
static void LoadPalettes(void);
static void DrawBgTilemap(u8, const void *);
static void NamingScreen_Dummy(u8, u8);
static void DrawTextEntry(void);
static void PrintKeyboardKeys(u8, u8);
static void DrawKeyboardPageOnDeck(void);
static void PrintControls(void);
static void CB2_NamingScreen(void);
static void ResetVHBlank(void);
static void SetVBlank(void);
static void VBlankCB_NamingScreen(void);
static void NamingScreen_ShowBgs(void);
static bool8 IsWideLetter(u8);

void DoNamingScreen(u8 templateNum, u8 *destBuffer, u16 monSpecies, u16 monGender, u32 monPersonality, MainCallback returnCallback)
{
    sNamingScreen = Alloc(sizeof(struct NamingScreenData));
    if (!sNamingScreen)
    {
        SetMainCallback2(returnCallback);
    }
    else
    {
        sNamingScreen->templateNum = templateNum;
        sNamingScreen->monSpecies = monSpecies;
        sNamingScreen->monGender = monGender;
        sNamingScreen->monPersonality = monPersonality;
        sNamingScreen->destBuffer = destBuffer;
        sNamingScreen->returnCallback = returnCallback;

        if (templateNum == NAMING_SCREEN_PLAYER)
            StartTimer1();

        SetMainCallback2(CB2_LoadNamingScreen);
    }
}

static void CB2_LoadNamingScreen(void)
{
    switch (gMain.state)
    {
    case 0:
        ResetVHBlank();
        NamingScreen_Init();
        gMain.state++;
        break;
    case 1:
        NamingScreen_InitBGs();
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        gMain.state++;
        break;
    case 3:
        ResetSpriteData();
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 4:
        ResetTasks();
        gMain.state++;
        break;
    case 5:
        LoadPalettes();
        gMain.state++;
        break;
    case 6:
        LoadGfx();
        gMain.state++;
        break;
    case 7:
        CreateSprites();
        UpdatePaletteFade();
        NamingScreen_ShowBgs();
        gMain.state++;
        break;
    default:
        CreateHelperTasks();
        CreateNamingScreenTask();
        break;
    }
}

static void NamingScreen_Init(void)
{
    sNamingScreen->state = STATE_FADE_IN;
    sNamingScreen->bg1vOffset = 0;
    sNamingScreen->bg2vOffset = 0;
    sNamingScreen->bg1Priority = BGCNT_PRIORITY(1);
    sNamingScreen->bg2Priority = BGCNT_PRIORITY(2);
    sNamingScreen->bgToReveal = 0;
    sNamingScreen->bgToHide = 1;
    sNamingScreen->template = sNamingScreenTemplates[sNamingScreen->templateNum];
    sNamingScreen->currentPage = sNamingScreen->template->initialPage;
    sNamingScreen->inputCharBaseXPos = (DISPLAY_WIDTH - sNamingScreen->template->maxChars * 8) / 2 + 6;
    if (sNamingScreen->templateNum == NAMING_SCREEN_WALDA)
        sNamingScreen->inputCharBaseXPos += 11;
    sNamingScreen->keyRepeatStartDelayCopy = gKeyRepeatStartDelay;
    memset(sNamingScreen->textBuffer, EOS, sizeof(sNamingScreen->textBuffer));
    if (sNamingScreen->template->copyExistingString)
        StringCopy(sNamingScreen->textBuffer, sNamingScreen->destBuffer);
    gKeyRepeatStartDelay = 16;
}

static void SetSpritesVisible(void)
{
    u8 i;
    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse)
            gSprites[i].invisible = FALSE;
    }
    SetCursorInvisibility(FALSE);
}

static void NamingScreen_InitBGs(void)
{
    u8 i;

    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));

    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);

    InitStandardTextBoxWindows();
    InitTextBoxGfxAndPrinters();

    for (i = 0; i < WIN_COUNT; i++)
        sNamingScreen->windows[i] = AddWindow(&sWindowTemplates[i]);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12, 8));

    SetBgTilemapBuffer(1, sNamingScreen->tilemapBuffer1);
    SetBgTilemapBuffer(2, sNamingScreen->tilemapBuffer2);
    SetBgTilemapBuffer(3, sNamingScreen->tilemapBuffer3);

    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 0x20, 0x20);
    FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 0x20, 0x20);
    FillBgTilemapBufferRect_Palette0(3, 0, 0, 0, 0x20, 0x20);
}

static void CreateNamingScreenTask(void)
{
    CreateTask(Task_NamingScreen, 2);
    SetMainCallback2(CB2_NamingScreen);
}

static void Task_NamingScreen(u8 taskId)
{
    switch (sNamingScreen->state)
    {
    case STATE_FADE_IN:
        MainState_FadeIn();
        SetSpritesVisible();
        SetVBlank();
        break;
    case STATE_WAIT_FADE_IN:
        MainState_WaitFadeIn();
        break;
    case STATE_HANDLE_INPUT:
        MainState_HandleInput();
        break;
    case STATE_MOVE_TO_OK_BUTTON:
        MainState_MoveToOKButton();
        MainState_HandleInput();
        break;
    case STATE_START_PAGE_SWAP:
        MainState_StartPageSwap();
        break;
    case STATE_WAIT_PAGE_SWAP:
        MainState_WaitPageSwap();
        break;
    case STATE_PRESSED_OK:
        MainState_PressedOKButton();
        break;
    case STATE_WAIT_SENT_TO_PC_MESSAGE:
        MainState_WaitSentToPCMessage();
        break;
    case STATE_FADE_OUT:
        MainState_FadeOut();
        break;
    case STATE_EXIT:
        MainState_Exit();
        break;
    }
}

// Which gfx/pal to load for the swap page button
static const u8 sPageToNextGfxId[KBPAGE_COUNT] =
{
    [KBPAGE_SYMBOLS]       = PAGE_SWAP_UPPER,
    [KBPAGE_LETTERS_UPPER] = PAGE_SWAP_LOWER,
    [KBPAGE_LETTERS_LOWER] = PAGE_SWAP_OTHERS
};

static const u8 sPageToNextKeyboardId[KBPAGE_COUNT] =
{
    [KBPAGE_SYMBOLS]       = KEYBOARD_LETTERS_UPPER,
    [KBPAGE_LETTERS_UPPER] = KEYBOARD_LETTERS_LOWER,
    [KBPAGE_LETTERS_LOWER] = KEYBOARD_SYMBOLS
};

static const u8 sPageToKeyboardId[KBPAGE_COUNT] =
{
    [KBPAGE_SYMBOLS]       = KEYBOARD_SYMBOLS,
    [KBPAGE_LETTERS_UPPER] = KEYBOARD_LETTERS_UPPER,
    [KBPAGE_LETTERS_LOWER] = KEYBOARD_LETTERS_LOWER
};

static u8 PageToNextGfxId(u8 page)
{
    return sPageToNextGfxId[page];
}

static u8 CurrentPageToNextKeyboardId(void)
{
    return sPageToNextKeyboardId[sNamingScreen->currentPage];
}

static u8 CurrentPageToKeyboardId(void)
{
    return sPageToKeyboardId[sNamingScreen->currentPage];
}

static bool8 MainState_FadeIn(void)
{
    DrawBgTilemap(3, gNamingScreenBackground_Tilemap);
    sNamingScreen->currentPage = KBPAGE_LETTERS_UPPER;
    DrawBgTilemap(2, gNamingScreenKeyboardLower_Tilemap);
    DrawBgTilemap(1, gNamingScreenKeyboardUpper_Tilemap);
    PrintKeyboardKeys(sNamingScreen->windows[WIN_KB_PAGE_2], KEYBOARD_LETTERS_LOWER);
    PrintKeyboardKeys(sNamingScreen->windows[WIN_KB_PAGE_1], KEYBOARD_LETTERS_UPPER);
    NamingScreen_Dummy(2, KEYBOARD_LETTERS_LOWER);
    NamingScreen_Dummy(1, KEYBOARD_LETTERS_UPPER);
    DrawTextEntry();
    DrawTextEntryBox();
    PrintControls();
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(2);
    CopyBgTilemapBufferToVram(3);
    BlendPalettes(PALETTES_ALL, 16, 0);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    sNamingScreen->state++;
    return FALSE;
}

static bool8 MainState_WaitFadeIn(void)
{
    if (!gPaletteFade.active)
    {
        SetInputState(INPUT_STATE_ENABLED);
        SetCursorFlashing(TRUE);
        sNamingScreen->state++;
    }
    return FALSE;
}

static bool8 MainState_HandleInput(void)
{
    return HandleKeyboardEvent();
}

static bool8 MainState_MoveToOKButton(void)
{
    if (IsCursorAnimFinished())
    {
        SetInputState(INPUT_STATE_ENABLED);
        MoveCursorToOKButton();
        sNamingScreen->state = STATE_HANDLE_INPUT;
    }
    return FALSE;
}

static bool8 MainState_PressedOKButton(void)
{
    SaveInputText();
    SetInputState(INPUT_STATE_DISABLED);
    SetCursorFlashing(FALSE);
    TryStartButtonFlash(BUTTON_COUNT, FALSE, TRUE);
    if (sNamingScreen->templateNum == NAMING_SCREEN_CAUGHT_MON
        && CalculatePlayerPartyCount() >= PARTY_SIZE)
    {
        DisplaySentToPCMessage();
        sNamingScreen->state = STATE_WAIT_SENT_TO_PC_MESSAGE;
        return FALSE;
    }
    else
    {
        sNamingScreen->state = STATE_FADE_OUT;
        return TRUE;
    }
}

static bool8 MainState_FadeOut(void)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    sNamingScreen->state++;
    return FALSE;
}

static bool8 MainState_Exit(void)
{
    if (!gPaletteFade.active)
    {
        if (sNamingScreen->templateNum == NAMING_SCREEN_PLAYER)
            SeedRngAndSetTrainerId();
        SetMainCallback2(sNamingScreen->returnCallback);
        DestroyTask(FindTaskIdByFunc(Task_NamingScreen));
        FreeAllWindowBuffers();
        FREE_AND_SET_NULL(sNamingScreen);
    }
    return FALSE;
}

static void DisplaySentToPCMessage(void)
{
    u8 stringToDisplay = 0;

    if (!IsDestinationBoxFull())
    {
        StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));
        StringCopy(gStringVar2, sNamingScreen->destBuffer);
    }
    else
    {
        StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));
        StringCopy(gStringVar2, sNamingScreen->destBuffer);
        StringCopy(gStringVar3, GetBoxNamePtr(GetPCBoxToSendMon()));
        stringToDisplay = 2;
    }

    if (FlagGet(FLAG_SYS_PC_LANETTE))
        stringToDisplay++;

    StringExpandPlaceholders(gStringVar4, sTransferredToPCMessages[stringToDisplay]);
    DrawDialogueFrame(0, FALSE);
    gTextFlags.canABSpeedUpPrint = TRUE;
    AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVar4, GetPlayerTextSpeedDelay(), 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static bool8 MainState_WaitSentToPCMessage(void)
{
    RunTextPrinters();
    if (!IsTextPrinterActive(0) && JOY_NEW(A_BUTTON))
        sNamingScreen->state = STATE_FADE_OUT;

    return FALSE;
}

static bool8 MainState_StartPageSwap(void)
{
    SetInputState(INPUT_STATE_DISABLED);
    StartPageSwapButtonAnim();
    StartPageSwapAnim();
    SetCursorInvisibility(TRUE);
    TryStartButtonFlash(BUTTON_PAGE, FALSE, TRUE);
    PlaySE(SE_WIN_OPEN);
    sNamingScreen->state = STATE_WAIT_PAGE_SWAP;
    return FALSE;
}

static bool8 MainState_WaitPageSwap(void)
{
    s16 cursorX;
    s16 cursorY;
    bool32 onLastColumn;

    if (IsPageSwapAnimNotInProgress())
    {

        GetCursorPos(&cursorX, &cursorY);
        onLastColumn = (cursorX == GetCurrentPageColumnCount());

        sNamingScreen->state = STATE_HANDLE_INPUT;
        sNamingScreen->currentPage++;
        sNamingScreen->currentPage %= KBPAGE_COUNT;

        if (onLastColumn)
        {
            cursorX = GetCurrentPageColumnCount();
        }
        else
        {
            if (cursorX >= GetCurrentPageColumnCount())
                cursorX = GetCurrentPageColumnCount() - 1;
        }

        SetCursorPos(cursorX, cursorY);
        DrawKeyboardPageOnDeck();
        SetInputState(INPUT_STATE_ENABLED);
        SetCursorInvisibility(FALSE);
    }
    return FALSE;
}

//--------------------------------------------------
// Page Swap
//--------------------------------------------------

#define tState data[0]
#define tFrameCount data[1]

static bool8 PageSwapAnimState_Init(struct Task *);
static bool8 PageSwapAnimState_1(struct Task *);
static bool8 PageSwapAnimState_2(struct Task *);
static bool8 PageSwapAnimState_Done(struct Task *);

static bool8 (*const sPageSwapAnimStateFuncs[])(struct Task *) =
{
    PageSwapAnimState_Init,
    PageSwapAnimState_1,
    PageSwapAnimState_2,
    PageSwapAnimState_Done,
};

static void StartPageSwapAnim(void)
{
    u8 taskId;

    taskId = CreateTask(Task_HandlePageSwapAnim, 0);
    Task_HandlePageSwapAnim(taskId);
}

static void Task_HandlePageSwapAnim(u8 taskId)
{
    while (sPageSwapAnimStateFuncs[gTasks[taskId].tState](&gTasks[taskId]) != 0);
}

static bool8 IsPageSwapAnimNotInProgress(void)
{
    if (FindTaskIdByFunc(Task_HandlePageSwapAnim) == TASK_NONE)
        return TRUE;
    else
        return FALSE;
}

static bool8 PageSwapAnimState_Init(struct Task *task)
{
    sNamingScreen->bg1vOffset = 0;
    sNamingScreen->bg2vOffset = 0;
    task->tState++;
    return 0;
}

static bool8 PageSwapAnimState_1(struct Task *task)
{
    u16 *const vOffsets[] =
    {
        &sNamingScreen->bg2vOffset,
        &sNamingScreen->bg1vOffset
    };

    task->tFrameCount += 4;
    *vOffsets[sNamingScreen->bgToReveal] = Sin(task->tFrameCount, 40);
    *vOffsets[sNamingScreen->bgToHide] = Sin((task->tFrameCount + 128) & 0xFF, 40);
    if (task->tFrameCount >= 64)
    {
        u8 temp = sNamingScreen->bg1Priority;  //Why u8 and not u16?

        sNamingScreen->bg1Priority = sNamingScreen->bg2Priority;
        sNamingScreen->bg2Priority = temp;
        task->tState++;
    }
    return 0;
}

static bool8 PageSwapAnimState_2(struct Task *task)
{
    u16 *const vOffsets[] =
    {
        &sNamingScreen->bg2vOffset,
        &sNamingScreen->bg1vOffset
    };

    task->tFrameCount += 4;
    *vOffsets[sNamingScreen->bgToReveal] = Sin(task->tFrameCount, 40);
    *vOffsets[sNamingScreen->bgToHide] = Sin((task->tFrameCount + 128) & 0xFF, 40);
    if (task->tFrameCount >= 128)
    {
        u8 temp = sNamingScreen->bgToReveal;

        sNamingScreen->bgToReveal = sNamingScreen->bgToHide;
        sNamingScreen->bgToHide = temp;
        task->tState++;
    }
    return 0;
}

static bool8 PageSwapAnimState_Done(struct Task *task)
{
    DestroyTask(FindTaskIdByFunc(Task_HandlePageSwapAnim));
    return 0;
}

#undef tState
#undef tFrameCount

//--------------------------------------------------
//
//--------------------------------------------------

#define tButtonId     data[0]
#define tKeepFlashing data[1]
#define tAllowFlash   data[2]
#define tColor        data[3]
#define tColorIncr    data[4]
#define tColorDelay   data[5]
#define tColorDelta   data[6]

static void CreateButtonFlashTask(void)
{
    u8 taskId;

    taskId = CreateTask(Task_UpdateButtonFlash, 3);
    gTasks[taskId].tButtonId = BUTTON_COUNT;
}

static void TryStartButtonFlash(u8 button, bool8 keepFlashing, bool8 interruptCurFlash)
{
    struct Task *task = &gTasks[FindTaskIdByFunc(Task_UpdateButtonFlash)];

    if (button == task->tButtonId && !interruptCurFlash)
    {
        task->tKeepFlashing = keepFlashing;
        task->tAllowFlash = TRUE;
        return;
    }
    if (button == BUTTON_COUNT && !task->tKeepFlashing && !interruptCurFlash)
        return;

    if (task->tButtonId != BUTTON_COUNT)
        RestoreButtonColor(task->tButtonId);

    StartButtonFlash(task, button, keepFlashing);
}

static void Task_UpdateButtonFlash(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (task->tButtonId == BUTTON_COUNT || !task->tAllowFlash)
        return;

    MultiplyInvertedPaletteRGBComponents(GetButtonPalOffset(task->tButtonId), task->tColor, task->tColor, task->tColor);

    if (task->tColorDelay && --task->tColorDelay)
        return;

    task->tColorDelay = 2;
    if (task->tColorIncr >= 0)
    {
        if (task->tColor < 14)
        {
            task->tColor += task->tColorIncr;
            task->tColorDelta += task->tColorIncr;
        }
        else
        {
            task->tColor = 16;
            task->tColorDelta++;
        }
    }
    else
    {
        task->tColor += task->tColorIncr;
        task->tColorDelta += task->tColorIncr;
    }

    if (task->tColor == 16 && task->tColorDelta == 22)
    {
        task->tColorIncr = -4;
    }
    else if (task->tColor == 0)
    {
        task->tAllowFlash = task->tKeepFlashing;
        task->tColorIncr = 2;
        task->tColorDelta = 0;
    }
}

static u16 GetButtonPalOffset(u8 button)
{
    const u16 palOffsets[BUTTON_COUNT + 1] =
    {
        [BUTTON_PAGE]  = OBJ_PLTT_ID(IndexOfSpritePaletteTag(PALTAG_PAGE_SWAP)) + 14,
        [BUTTON_BACK]  = OBJ_PLTT_ID(IndexOfSpritePaletteTag(PALTAG_BACK_BUTTON)) + 14,
        [BUTTON_OK]    = OBJ_PLTT_ID(IndexOfSpritePaletteTag(PALTAG_OK_BUTTON)) + 14,
        [BUTTON_COUNT] = OBJ_PLTT_ID(IndexOfSpritePaletteTag(PALTAG_OK_BUTTON)) + 1,
    };

    return palOffsets[button];
}

static void RestoreButtonColor(u8 button)
{
    u16 index = GetButtonPalOffset(button);
    gPlttBufferFaded[index] = gPlttBufferUnfaded[index];
}

static void StartButtonFlash(struct Task *task, u8 button, bool8 keepFlashing)
{
    task->tButtonId = button;
    task->tKeepFlashing = keepFlashing;
    task->tAllowFlash = TRUE;
    task->tColor = 4;
    task->tColorIncr = 2;
    task->tColorDelay = 0;
    task->tColorDelta = 4;
}

#undef tButtonId
#undef tColor

// Sprite data for the the cursor
#define sX          data[0]
#define sY          data[1]
#define sPrevX      data[2]
#define sPrevY      data[3]
#define sInvisible  data[4] & 0x00FF
#define sFlashing   data[4] & 0xFF00
#define sColor      data[5]
#define sColorIncr  data[6]
#define sColorDelay data[7]

static void SpriteCB_Cursor(struct Sprite *sprite)
{
    if (sprite->animEnded)
        StartSpriteAnim(sprite, 0);

    // Hide cursor when on button column
    sprite->invisible = sprite->sInvisible;
    if (sprite->sX == GetCurrentPageColumnCount())
        sprite->invisible = TRUE;

    if (sprite->invisible
       || !(sprite->sFlashing)
       || sprite->sX != sprite->sPrevX
       || sprite->sY != sprite->sPrevY)
    {
        sprite->sColor = 0;
        sprite->sColorIncr = 2;
        sprite->sColorDelay = 2;
    }

    sprite->sColorDelay--;
    if (sprite->sColorDelay == 0)
    {
        sprite->sColor += sprite->sColorIncr;
        if (sprite->sColor == 16 || sprite->sColor == 0)
            sprite->sColorIncr = -sprite->sColorIncr;
        sprite->sColorDelay = 2;
    }

    if (sprite->sFlashing)
    {
        s8 gb = sprite->sColor;
        s8 r = sprite->sColor >> 1;
        u16 index = OBJ_PLTT_ID(IndexOfSpritePaletteTag(PALTAG_CURSOR)) + 1;

        MultiplyInvertedPaletteRGBComponents(index, r, gb, gb);
    }
}

#define sDelay  data[0]
#define sXPosId data[1]

static void SpriteCB_InputArrow(struct Sprite *sprite)
{
    const s16 x[] = {0, -4, -2, -1};

    if (sprite->sDelay == 0 || --sprite->sDelay == 0)
    {
        sprite->sDelay = 8;
        sprite->sXPosId = MOD(sprite->sXPosId + 1, ARRAY_COUNT(x));
    }
    sprite->x2 = x[sprite->sXPosId];
}

#undef sDelay
#undef sXPosId

#define sId     data[0] // set in CreateTextEntrySprites
#define sYPosId data[1]
#define sDelay  data[2]

static void SpriteCB_Underscore(struct Sprite *sprite)
{
    const s16 y[] = {2, 3, 2, 1};
    u8 pos;

    pos = GetTextEntryPosition();
    if (pos != (u8)sprite->sId)
    {
        sprite->y2 = 0;
        sprite->sYPosId = 0;
        sprite->sDelay = 0;
    }
    else
    {
        sprite->y2 = y[sprite->sYPosId];
        sprite->sDelay++;
        if (sprite->sDelay > 8)
        {
            sprite->sYPosId = MOD(sprite->sYPosId + 1, ARRAY_COUNT(y));
            sprite->sDelay = 0;
        }
    }
}

#undef sId
#undef sYPosId
#undef sDelay

static void CreateSprites(void)
{
    CreateCursorSprite();
    CreatePageSwapButtonSprites();
    CreateBackOkSprites();
    CreateTextEntrySprites();
    CreateInputTargetIcon();
}

static void CreateCursorSprite(void)
{
    sNamingScreen->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor, 38, 88, 1);
    SetCursorInvisibility(TRUE);
    gSprites[sNamingScreen->cursorSpriteId].oam.priority = 1;
    gSprites[sNamingScreen->cursorSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
    gSprites[sNamingScreen->cursorSpriteId].sColorIncr = 1; // ? immediately overwritten
    gSprites[sNamingScreen->cursorSpriteId].sColorIncr = 2;
    SetCursorPos(0, 0);
}

static void SetCursorPos(s16 x, s16 y)
{
    struct Sprite *cursorSprite = &gSprites[sNamingScreen->cursorSpriteId];

    if (x < sPageColumnCounts[CurrentPageToKeyboardId()])
        cursorSprite->x = sPageColumnXPos[CurrentPageToKeyboardId()][x] + 38;
    else
        cursorSprite->x = 0;

    cursorSprite->y = y * 16 + 88;
    cursorSprite->sPrevX = cursorSprite->sX;
    cursorSprite->sPrevY = cursorSprite->sY;
    cursorSprite->sX = x;
    cursorSprite->sY = y;
}

static void GetCursorPos(s16 *x, s16 *y)
{
    struct Sprite *cursorSprite = &gSprites[sNamingScreen->cursorSpriteId];

    *x = cursorSprite->sX;
    *y = cursorSprite->sY;
}

static void MoveCursorToOKButton(void)
{
    SetCursorPos(GetCurrentPageColumnCount(), 2);
}

static void SetCursorInvisibility(bool8 invisible)
{
    gSprites[sNamingScreen->cursorSpriteId].data[4] &= 0xFF00;
    gSprites[sNamingScreen->cursorSpriteId].data[4] |= invisible; // sInvisible
    StartSpriteAnim(&gSprites[sNamingScreen->cursorSpriteId], 0);
}

static void SetCursorFlashing(bool8 flashing)
{
    gSprites[sNamingScreen->cursorSpriteId].data[4] &= 0xFF;
    gSprites[sNamingScreen->cursorSpriteId].data[4] |= flashing << 8; // sFlashing
}

static void SquishCursor(void)
{
    StartSpriteAnim(&gSprites[sNamingScreen->cursorSpriteId], 1);
}

static bool8 IsCursorAnimFinished(void)
{
    return gSprites[sNamingScreen->cursorSpriteId].animEnded;
}

static const u8 sButtonKeyRoles[] = {KEY_ROLE_PAGE, KEY_ROLE_BACKSPACE, KEY_ROLE_OK};

static u8 GetKeyRoleAtCursorPos(void)
{
    s16 cursorX;
    s16 cursorY;

    GetCursorPos(&cursorX, &cursorY);
    if (cursorX < GetCurrentPageColumnCount())
        return KEY_ROLE_CHAR;
    else
        return sButtonKeyRoles[cursorY];
}

// If the cursor's x is equal to the column count, cursor is in the button column
static u8 GetCurrentPageColumnCount(void)
{
    return sPageColumnCounts[CurrentPageToKeyboardId()];
}

#undef sX
#undef sY
#undef sPrevX
#undef sPrevY
#undef sInvisible
#undef sFlashing
#undef sColor
#undef sColorIncr
#undef sColorDelay

static bool8 PageSwapSprite_Init(struct Sprite *);
static bool8 PageSwapSprite_Idle(struct Sprite *);
static bool8 PageSwapSprite_SlideOff(struct Sprite *);
static bool8 PageSwapSprite_SlideOn(struct Sprite *);

#define sState          data[0]
#define sPage           data[1]
#define sTextSpriteId   data[6]
#define sButtonSpriteId data[7]

static void CreatePageSwapButtonSprites(void)
{
    u8 frameSpriteId;
    u8 textSpriteId;
    u8 buttonSpriteId;

    frameSpriteId = CreateSprite(&sSpriteTemplate_PageSwapFrame, 204, 88, 0);
    sNamingScreen->swapBtnFrameSpriteId = frameSpriteId;
    SetSubspriteTables(&gSprites[frameSpriteId], sSubspriteTable_PageSwapFrame);
    gSprites[frameSpriteId].invisible = TRUE;

    textSpriteId = CreateSprite(&sSpriteTemplate_PageSwapText, 204, 84, 1);
    gSprites[frameSpriteId].sTextSpriteId = textSpriteId;
    SetSubspriteTables(&gSprites[textSpriteId], sSubspriteTable_PageSwapText);
    gSprites[textSpriteId].invisible = TRUE;

    buttonSpriteId = CreateSprite(&sSpriteTemplate_PageSwapButton, 204, 83, 2);
    gSprites[buttonSpriteId].oam.priority = 1;
    gSprites[frameSpriteId].sButtonSpriteId = buttonSpriteId;
    gSprites[buttonSpriteId].invisible = TRUE;
}

static void StartPageSwapButtonAnim(void)
{
    struct Sprite *sprite = &gSprites[sNamingScreen->swapBtnFrameSpriteId];

    sprite->sState = 2; // go to PageSwapSprite_SlideOff
    sprite->sPage = sNamingScreen->currentPage;
}

static u8 (*const sPageSwapSpriteFuncs[])(struct Sprite *) =
{
    PageSwapSprite_Init,
    PageSwapSprite_Idle,
    PageSwapSprite_SlideOff,
    PageSwapSprite_SlideOn,
};

static void SpriteCB_PageSwap(struct Sprite *sprite)
{
    while (sPageSwapSpriteFuncs[sprite->sState](sprite));
}

static bool8 PageSwapSprite_Init(struct Sprite *sprite)
{
    struct Sprite *text = &gSprites[sprite->sTextSpriteId];
    struct Sprite *button = &gSprites[sprite->sButtonSpriteId];

    SetPageSwapButtonGfx(PageToNextGfxId(sNamingScreen->currentPage), text, button);
    sprite->sState++;
    return FALSE;
}

static bool8 PageSwapSprite_Idle(struct Sprite *sprite)
{
    return FALSE;
}

static bool8 PageSwapSprite_SlideOff(struct Sprite *sprite)
{
    struct Sprite *text = &gSprites[sprite->sTextSpriteId];
    struct Sprite *button = &gSprites[sprite->sButtonSpriteId];

    text->y2++;
    if (text->y2 > 7)
    {
        sprite->sState++;
        text->y2 = -4;
        text->invisible = TRUE;
        SetPageSwapButtonGfx(PageToNextGfxId(((u8)sprite->sPage + 1) % KBPAGE_COUNT), text, button);
    }
    return FALSE;
}

static bool8 PageSwapSprite_SlideOn(struct Sprite *sprite)
{
    struct Sprite *text = &gSprites[sprite->sTextSpriteId];

    text->invisible = FALSE;
    text->y2++;
    if (text->y2 >= 0)
    {
        text->y2 = 0;
        sprite->sState = 1; // go to PageSwapSprite_Idle
    }
    return FALSE;
}

static const u16 sPageSwapPalTags[] = {
    [PAGE_SWAP_UPPER]  = PALTAG_PAGE_SWAP_UPPER,
    [PAGE_SWAP_OTHERS] = PALTAG_PAGE_SWAP_OTHERS,
    [PAGE_SWAP_LOWER]  = PALTAG_PAGE_SWAP_LOWER
};

static const u16 sPageSwapGfxTags[] = {
    [PAGE_SWAP_UPPER]  = GFXTAG_PAGE_SWAP_UPPER,
    [PAGE_SWAP_OTHERS] = GFXTAG_PAGE_SWAP_OTHERS,
    [PAGE_SWAP_LOWER]  = GFXTAG_PAGE_SWAP_LOWER
};

static void SetPageSwapButtonGfx(u8 page, struct Sprite *text, struct Sprite *button)
{
    button->oam.paletteNum = IndexOfSpritePaletteTag(sPageSwapPalTags[page]);
    text->sheetTileStart = GetSpriteTileStartByTag(sPageSwapGfxTags[page]);
    text->subspriteTableNum = page;
}

#undef sState
#undef sPage
#undef sTextSpriteId
#undef sButtonSpriteId

static void CreateBackOkSprites(void)
{
    u8 spriteId;

    spriteId = CreateSprite(&sSpriteTemplate_BackButton, 204, 116, 0);
    SetSubspriteTables(&gSprites[spriteId], sSubspriteTable_Button);
    gSprites[spriteId].invisible = TRUE;

    spriteId = CreateSprite(&sSpriteTemplate_OkButton, 204, 140, 0);
    SetSubspriteTables(&gSprites[spriteId], sSubspriteTable_Button);
    gSprites[spriteId].invisible = TRUE;
}

static void CreateTextEntrySprites(void)
{
    u8 spriteId;
    s16 xPos;
    u8 i;

    xPos = sNamingScreen->inputCharBaseXPos - 5;
    spriteId = CreateSprite(&sSpriteTemplate_InputArrow, xPos, 56, 0);
    gSprites[spriteId].oam.priority = 3;
    gSprites[spriteId].invisible = TRUE;
    xPos = sNamingScreen->inputCharBaseXPos;
    for (i = 0; i < sNamingScreen->template->maxChars; i++, xPos += 8)
    {
        spriteId = CreateSprite(&sSpriteTemplate_Underscore, xPos + 3, 60, 0);
        gSprites[spriteId].oam.priority = 3;
        gSprites[spriteId].data[0] = i;
        gSprites[spriteId].invisible = TRUE;
    }
}

//--------------------------------------------------
// Icon creation (the thing you're naming or giving input to)
//--------------------------------------------------

static void NamingScreen_NoIcon(void);
static void NamingScreen_CreatePlayerIcon(void);
static void NamingScreen_CreatePCIcon(void);
static void NamingScreen_CreateMonIcon(void);
static void NamingScreen_CreateWaldaDadIcon(void);

static void (*const sIconFunctions[])(void) =
{
    NamingScreen_NoIcon,
    NamingScreen_CreatePlayerIcon,
    NamingScreen_CreatePCIcon,
    NamingScreen_CreateMonIcon,
    NamingScreen_CreateWaldaDadIcon,
};

static void CreateInputTargetIcon(void)
{
    sIconFunctions[sNamingScreen->template->iconFunction]();
}

static void NamingScreen_NoIcon(void)
{

}

static void NamingScreen_CreatePlayerIcon(void)
{
    u8 rivalGfxId;
    u8 spriteId;

    rivalGfxId = GetRivalAvatarGraphicsIdByStateIdAndGender(PLAYER_AVATAR_STATE_NORMAL, sNamingScreen->monSpecies);
    spriteId = CreateObjectGraphicsSprite(rivalGfxId, SpriteCallbackDummy, 56, 37, 0);
    gSprites[spriteId].oam.priority = 3;
    StartSpriteAnim(&gSprites[spriteId], ANIM_STD_GO_SOUTH);
}

static void NamingScreen_CreatePCIcon(void)
{
    u8 spriteId;

    spriteId = CreateSprite(&sSpriteTemplate_PCIcon, 56, 41, 0);
    SetSubspriteTables(&gSprites[spriteId], sSubspriteTable_PCIcon);
    gSprites[spriteId].oam.priority = 3;
}

static void NamingScreen_CreateMonIcon(void)
{
    u8 spriteId;

    LoadMonIconPalettes();
    spriteId = CreateMonIcon(sNamingScreen->monSpecies, SpriteCallbackDummy, 56, 40, 0, sNamingScreen->monPersonality, 1);
    gSprites[spriteId].oam.priority = 3;
}

static void NamingScreen_CreateWaldaDadIcon(void)
{
    u8 spriteId;

    spriteId = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_MAN_1, SpriteCallbackDummy, 56, 37, 0);
    gSprites[spriteId].oam.priority = 3;
    StartSpriteAnim(&gSprites[spriteId], ANIM_STD_GO_SOUTH);
}

//--------------------------------------------------
// Keyboard handling
//--------------------------------------------------

static bool8 KeyboardKeyHandler_Character(u8);
static bool8 KeyboardKeyHandler_Page(u8);
static bool8 KeyboardKeyHandler_Backspace(u8);
static bool8 KeyboardKeyHandler_OK(u8);

static bool8 (*const sKeyboardKeyHandlers[])(u8) =
{
    [KEY_ROLE_CHAR]      = KeyboardKeyHandler_Character,
    [KEY_ROLE_PAGE]      = KeyboardKeyHandler_Page,
    [KEY_ROLE_BACKSPACE] = KeyboardKeyHandler_Backspace,
    [KEY_ROLE_OK]        = KeyboardKeyHandler_OK,
};

static bool8 HandleKeyboardEvent(void)
{
    u8 input = GetInputEvent();
    u8 keyRole = GetKeyRoleAtCursorPos();

    if (input == INPUT_SELECT)
    {
        return SwapKeyboardPage();
    }
    else if (input == INPUT_B_BUTTON)
    {
        DeleteTextCharacter();
        return FALSE;
    }
    else if (input == INPUT_START)
    {
        MoveCursorToOKButton();
        return FALSE;
    }
    else
    {
        return sKeyboardKeyHandlers[keyRole](input);
    }
}

static bool8 KeyboardKeyHandler_Character(u8 input)
{
    TryStartButtonFlash(BUTTON_COUNT, FALSE, FALSE);
    if (input == INPUT_A_BUTTON)
    {
        bool8 textFull = AddTextCharacter();

        SquishCursor();
        if (textFull)
        {
            SetInputState(INPUT_STATE_OVERRIDE);
            sNamingScreen->state = STATE_MOVE_TO_OK_BUTTON;
        }
    }
    return FALSE;
}

static bool8 KeyboardKeyHandler_Page(u8 input)
{
    TryStartButtonFlash(BUTTON_PAGE, TRUE, FALSE);
    if (input == INPUT_A_BUTTON)
        return SwapKeyboardPage();
    else
        return FALSE;
}

static bool8 KeyboardKeyHandler_Backspace(u8 input)
{
    TryStartButtonFlash(BUTTON_BACK, TRUE, FALSE);
    if (input == INPUT_A_BUTTON)
        DeleteTextCharacter();
    return FALSE;
}

static bool8 KeyboardKeyHandler_OK(u8 input)
{
    TryStartButtonFlash(BUTTON_OK, TRUE, FALSE);
    if (input == INPUT_A_BUTTON)
    {
        PlaySE(SE_SELECT);
        sNamingScreen->state = STATE_PRESSED_OK;
        return TRUE;
    }
    else
        return FALSE;
}

static bool8 SwapKeyboardPage(void)
{
    sNamingScreen->state = STATE_START_PAGE_SWAP;
    return TRUE;
}

//--------------------------------------------------
// Input handling
//--------------------------------------------------

#define tState data[0]
#define tKeyboardEvent data[1]
#define tButtonId data[2]


static void Input_Disabled(struct Task *);
static void Input_Enabled(struct Task *);
static void Input_Override(struct Task *);

static void (*const sInputFuncs[])(struct Task *) =
{
    [INPUT_STATE_DISABLED] = Input_Disabled,
    [INPUT_STATE_ENABLED]  = Input_Enabled,
    [INPUT_STATE_OVERRIDE] = Input_Override,
};

static void Task_HandleInput(u8);
static void HandleDpadMovement(struct Task *);

static void CreateInputHandlerTask(void)
{
    CreateTask(Task_HandleInput, 1);
}

static u8 GetInputEvent(void)
{
    u8 taskId = FindTaskIdByFunc(Task_HandleInput);

    return gTasks[taskId].tKeyboardEvent;
}

static void SetInputState(u8 state)
{
    u8 taskId = FindTaskIdByFunc(Task_HandleInput);

    gTasks[taskId].tState = state;
}

static void Task_HandleInput(u8 taskId)
{
    sInputFuncs[gTasks[taskId].tState](&gTasks[taskId]);
}

static void Input_Disabled(struct Task *task)
{
    task->tKeyboardEvent = INPUT_NONE;
}

static void Input_Enabled(struct Task *task)
{
    task->tKeyboardEvent = INPUT_NONE;

    if (JOY_NEW(A_BUTTON))
        task->tKeyboardEvent = INPUT_A_BUTTON;
    else if (JOY_NEW(B_BUTTON))
        task->tKeyboardEvent = INPUT_B_BUTTON;
    else if (JOY_NEW(SELECT_BUTTON))
        task->tKeyboardEvent = INPUT_SELECT;
    else if (JOY_NEW(START_BUTTON))
        task->tKeyboardEvent = INPUT_START;
    else
        HandleDpadMovement(task);
}

static void Input_Override(struct Task *task)
{
    task->tKeyboardEvent = INPUT_NONE;
}

static void HandleDpadMovement(struct Task *task)
{
    const s16 sDpadDeltaX[] =
    {
        [INPUT_NONE]       = 0,
        [INPUT_DPAD_UP]    = 0,
        [INPUT_DPAD_DOWN]  = 0,
        [INPUT_DPAD_LEFT]  = -1,
        [INPUT_DPAD_RIGHT] = 1
    };

    const s16 sDpadDeltaY[] =
    {
        [INPUT_NONE]       = 0,
        [INPUT_DPAD_UP]    = -1,
        [INPUT_DPAD_DOWN]  = 1,
        [INPUT_DPAD_LEFT]  = 0,
        [INPUT_DPAD_RIGHT] = 0
    };

    const s16 sKeyRowToButtonRow[KBROW_COUNT] = {0, 1, 1, 2};
    const s16 sButtonRowToKeyRow[BUTTON_COUNT] = {0, 0, 3};

    s16 cursorX;
    s16 cursorY;
    u16 input;
    s16 prevCursorX;

    GetCursorPos(&cursorX, &cursorY);
    input = INPUT_NONE;
    if (JOY_REPEAT(DPAD_UP))
        input = INPUT_DPAD_UP;
    if (JOY_REPEAT(DPAD_DOWN))
        input = INPUT_DPAD_DOWN;
    if (JOY_REPEAT(DPAD_LEFT))
        input = INPUT_DPAD_LEFT;
    if (JOY_REPEAT(DPAD_RIGHT))
        input = INPUT_DPAD_RIGHT;

    // Get new cursor position
    prevCursorX = cursorX;
    cursorX += sDpadDeltaX[input];
    cursorY += sDpadDeltaY[input];

    // Wrap cursor position in the X direction
    if (cursorX < 0)
        cursorX = GetCurrentPageColumnCount();
    if (cursorX > GetCurrentPageColumnCount())
        cursorX = 0;


    // Handle moving on/off the button column
    if (sDpadDeltaX[input] != 0)
    {
        if (cursorX == GetCurrentPageColumnCount())
        {
            // Moved onto button column
            // Save cursor pos in tButtonId for moving back onto keys
            task->tButtonId = cursorY;
            cursorY = sKeyRowToButtonRow[cursorY];
        }
        else if (prevCursorX == GetCurrentPageColumnCount())
        {
            // Moved off button column
            // If you're on the middle button, go to the row that
            // the cursor was on previously (saved in tButtonId above)
            if (cursorY == BUTTON_COUNT / 2)
                cursorY = task->tButtonId;
            else
                cursorY = sButtonRowToKeyRow[cursorY];
        }
    }

    // Wrap cursor position in the y direction
    if (cursorX == GetCurrentPageColumnCount())
    {
        // There are only 3 keys in the button column
        // so wrap Y accordingly
        if (cursorY < 0)
            cursorY = BUTTON_COUNT - 1;
        if (cursorY >= BUTTON_COUNT)
            cursorY = 0;

        if (cursorY == 0)
            task->tButtonId = BUTTON_BACK;
        else if (cursorY == BUTTON_COUNT - 1)
            task->tButtonId = BUTTON_OK;
    }
    else
    {
        if (cursorY < 0)
            cursorY = KBROW_COUNT - 1;
        if (cursorY > KBROW_COUNT - 1)
            cursorY = 0;
    }
    SetCursorPos(cursorX, cursorY);
}

#undef tState
#undef tKeyboardEvent
#undef tButtonId

static void DrawNormalTextEntryBox(void)
{
    FillWindowPixelBuffer(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX], PIXEL_FILL(1));
    AddTextPrinterParameterized(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX], FONT_NORMAL, sNamingScreen->template->title, 8, 1, 0, 0);
    PutWindowTilemap(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX]);
}

static void DrawMonTextEntryBox(void)
{
    u8 buffer[32];

    StringCopy(buffer, gSpeciesNames[sNamingScreen->monSpecies]);
    StringAppendN(buffer, sNamingScreen->template->title, 15);
    FillWindowPixelBuffer(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX], PIXEL_FILL(1));
    AddTextPrinterParameterized(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX], FONT_NORMAL, buffer, 8, 1, 0, 0);
    PutWindowTilemap(sNamingScreen->windows[WIN_TEXT_ENTRY_BOX]);
}

static void (*const sDrawTextEntryBoxFuncs[])(void) =
{
    [NAMING_SCREEN_PLAYER]     = DrawNormalTextEntryBox,
    [NAMING_SCREEN_BOX]        = DrawNormalTextEntryBox,
    [NAMING_SCREEN_CAUGHT_MON] = DrawMonTextEntryBox,
    [NAMING_SCREEN_NICKNAME]   = DrawMonTextEntryBox,
    [NAMING_SCREEN_WALDA]      = DrawNormalTextEntryBox,
};

static void DrawTextEntryBox(void)
{
    sDrawTextEntryBoxFuncs[sNamingScreen->templateNum]();
}

static void DummyGenderIcon(void);
static void DrawGenderIcon(void);

static void (*const sDrawGenderIconFuncs[])(void) =
{
    [FALSE] = DummyGenderIcon,
    [TRUE]  = DrawGenderIcon,
};

static void TryDrawGenderIcon(void)
{
    sDrawGenderIconFuncs[sNamingScreen->template->addGenderIcon]();
}

static void DummyGenderIcon(void)
{

}

static const u8 sGenderColors[2][3] =
{
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_BLUE, TEXT_COLOR_BLUE},
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_RED}
};

static void DrawGenderIcon(void)
{
    u8 text[2];
    bool8 isFemale = FALSE;

    StringCopy(text, gText_MaleSymbol);
    if (sNamingScreen->monGender != MON_GENDERLESS)
    {
        if (sNamingScreen->monGender == MON_FEMALE)
        {
            StringCopy(text, gText_FemaleSymbol);
            isFemale = TRUE;
        }
        AddTextPrinterParameterized3(sNamingScreen->windows[WIN_TEXT_ENTRY], FONT_NORMAL, (POKEMON_NAME_LENGTH * 4) + 64, 1, sGenderColors[isFemale], TEXT_SKIP_DRAW, text);
    }
}

static u8 GetCharAtKeyboardPos(s16 x, s16 y)
{
    return sKeyboardChars[CurrentPageToKeyboardId()][y][x];
}


static u8 GetTextEntryPosition(void)
{
    u8 i;

    for (i = 0; i < sNamingScreen->template->maxChars; i++)
    {
        if (sNamingScreen->textBuffer[i] == EOS)
            return i;
    }
    return sNamingScreen->template->maxChars - 1;
}

static u8 GetPreviousTextCaretPosition(void)
{
    s8 i;

    for (i = sNamingScreen->template->maxChars - 1; i > 0; i--)
    {
        if (sNamingScreen->textBuffer[i] != EOS)
            return i;
    }
    return 0;
}

static void DeleteTextCharacter(void)
{
    u8 index;
    u8 keyRole;

    index = GetPreviousTextCaretPosition();
    sNamingScreen->textBuffer[index] = 0;
    DrawTextEntry();
    CopyBgTilemapBufferToVram(3);
    sNamingScreen->textBuffer[index] = EOS;
    keyRole = GetKeyRoleAtCursorPos();

    // The below flashes the Back key once on delete
    // It incorrectly leaves the Back key 1 shade lighter than its default
    if (keyRole == KEY_ROLE_CHAR || keyRole == KEY_ROLE_BACKSPACE)
        TryStartButtonFlash(BUTTON_BACK, FALSE, TRUE);
    PlaySE(SE_BALL);
}

// Returns TRUE if the text entry is now full
static bool8 AddTextCharacter(void)
{
    s16 x;
    s16 y;

    GetCursorPos(&x, &y);
    BufferCharacter(GetCharAtKeyboardPos(x, y));
    DrawTextEntry();
    CopyBgTilemapBufferToVram(3);
    PlaySE(SE_SELECT);

    if (GetPreviousTextCaretPosition() != sNamingScreen->template->maxChars - 1)
        return FALSE;
    else
        return TRUE;
}

static void BufferCharacter(u8 ch)
{
    u8 index = GetTextEntryPosition();
    sNamingScreen->textBuffer[index] = ch;
}

static void SaveInputText(void)
{
    u8 i;

    for (i = 0; i < sNamingScreen->template->maxChars; i++)
    {
        if (sNamingScreen->textBuffer[i] != CHAR_SPACE && sNamingScreen->textBuffer[i] != EOS)
        {
            StringCopyN(sNamingScreen->destBuffer, sNamingScreen->textBuffer, sNamingScreen->template->maxChars + 1);
            break;
        }
    }
}

static void LoadGfx(void)
{
    LZ77UnCompWram(gNamingScreenMenu_Gfx, sNamingScreen->tileBuffer);
    LoadBgTiles(1, sNamingScreen->tileBuffer, sizeof(sNamingScreen->tileBuffer), 0);
    LoadBgTiles(2, sNamingScreen->tileBuffer, sizeof(sNamingScreen->tileBuffer), 0);
    LoadBgTiles(3, sNamingScreen->tileBuffer, sizeof(sNamingScreen->tileBuffer), 0);
    LoadSpriteSheets(sSpriteSheets);
    LoadSpritePalettes(sSpritePalettes);
}

static void CreateHelperTasks(void)
{
    CreateInputHandlerTask();
    CreateButtonFlashTask();
}

static void LoadPalettes(void)
{
    LoadPalette(gNamingScreenMenu_Pal, BG_PLTT_ID(0), sizeof(gNamingScreenMenu_Pal));
    LoadPalette(sKeyboard_Pal, BG_PLTT_ID(10), sizeof(sKeyboard_Pal));
    LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(11), PLTT_SIZE_4BPP);
}

static void DrawBgTilemap(u8 bg, const void *src)
{
    CopyToBgTilemapBuffer(bg, src, 0, 0);
}

static void NamingScreen_Dummy(u8 bg, u8 page)
{

}

static void DrawTextEntry(void)
{
    u8 i;
    u8 temp[2];
    u16 extraWidth;
    u8 maxChars = sNamingScreen->template->maxChars;
    u16 x = sNamingScreen->inputCharBaseXPos - 0x40;

    FillWindowPixelBuffer(sNamingScreen->windows[WIN_TEXT_ENTRY], PIXEL_FILL(1));

    for (i = 0; i < maxChars; i++)
    {
        temp[0] = sNamingScreen->textBuffer[i];
        temp[1] = gText_ExpandedPlaceholder_Empty[0];
        extraWidth = (IsWideLetter(temp[0]) == TRUE) ? 2 : 0;

        AddTextPrinterParameterized(sNamingScreen->windows[WIN_TEXT_ENTRY], FONT_NORMAL, temp, i * 8 + x + extraWidth, 1, TEXT_SKIP_DRAW, NULL);
    }

    TryDrawGenderIcon();
    CopyWindowToVram(sNamingScreen->windows[WIN_TEXT_ENTRY], COPYWIN_GFX);
    PutWindowTilemap(sNamingScreen->windows[WIN_TEXT_ENTRY]);
}

struct TextColor   // Needed because of alignment
{
    u8 colors[3][4];
};

static const struct TextColor sTextColorStruct =
{
    {
        {TEXT_DYNAMIC_COLOR_4, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY},
        {TEXT_DYNAMIC_COLOR_5, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY},
        {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY}
    }
};

static const u8 sFillValues[KBPAGE_COUNT] =
{
    [KEYBOARD_LETTERS_LOWER] = PIXEL_FILL(14),
    [KEYBOARD_LETTERS_UPPER] = PIXEL_FILL(13),
    [KEYBOARD_SYMBOLS]       = PIXEL_FILL(15)
};

static const u8 *const sKeyboardTextColors[KBPAGE_COUNT] =
{
    [KEYBOARD_LETTERS_LOWER] = sTextColorStruct.colors[1],
    [KEYBOARD_LETTERS_UPPER] = sTextColorStruct.colors[0],
    [KEYBOARD_SYMBOLS]       = sTextColorStruct.colors[2]
};

static void PrintKeyboardKeys(u8 window, u8 page)
{
    u8 i;

    FillWindowPixelBuffer(window, sFillValues[page]);

    for (i = 0; i < KBROW_COUNT; i++)
        AddTextPrinterParameterized3(window, FONT_NORMAL, 0, i * 16 + 1, sKeyboardTextColors[page], 0, sNamingScreenKeyboardText[page][i]);

    PutWindowTilemap(window);
}

static const u32 *const sNextKeyboardPageTilemaps[] =
{
    [KBPAGE_SYMBOLS] = gNamingScreenKeyboardUpper_Tilemap,
    [KBPAGE_LETTERS_UPPER] = gNamingScreenKeyboardLower_Tilemap, // lower
    [KBPAGE_LETTERS_LOWER] = gNamingScreenKeyboardSymbols_Tilemap  // symbols
};

// There are always 2 keyboard pages drawn, the current page and the one that will shown next if the player swaps
// When the page swap is complete this function invisibly replaces the old page with the new next one
static void DrawKeyboardPageOnDeck(void)
{
    u8 bg;
    u8 bg_;
    u8 windowId;
    u8 bg1Priority = GetGpuReg(REG_OFFSET_BG1CNT) & 3;
    u8 bg2Priority = GetGpuReg(REG_OFFSET_BG2CNT) & 3;

    if (bg1Priority > bg2Priority)
    {
        bg = 1;
        bg_ = 1;
        windowId = sNamingScreen->windows[WIN_KB_PAGE_1];
    }
    else
    {
        bg = 2;
        bg_ = 2;
        windowId = sNamingScreen->windows[WIN_KB_PAGE_2];
    }

    DrawBgTilemap(bg, sNextKeyboardPageTilemaps[sNamingScreen->currentPage]);
    PrintKeyboardKeys(windowId, CurrentPageToNextKeyboardId());
    NamingScreen_Dummy(bg, CurrentPageToNextKeyboardId());
    CopyBgTilemapBufferToVram(bg_);
}

static void PrintControls(void)
{
    const u8 color[3] = { TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };

    FillWindowPixelBuffer(sNamingScreen->windows[WIN_BANNER], PIXEL_FILL(15));
    AddTextPrinterParameterized3(sNamingScreen->windows[WIN_BANNER], FONT_SMALL, 2, 1, color, 0, gText_MoveOkBack);
    PutWindowTilemap(sNamingScreen->windows[WIN_BANNER]);
    CopyWindowToVram(sNamingScreen->windows[WIN_BANNER], COPYWIN_FULL);
}

static void CB2_NamingScreen(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void ResetVHBlank(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
}

static void SetVBlank(void)
{
    SetVBlankCallback(VBlankCB_NamingScreen);
}

static void VBlankCB_NamingScreen(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    SetGpuReg(REG_OFFSET_BG1VOFS, sNamingScreen->bg1vOffset);
    SetGpuReg(REG_OFFSET_BG2VOFS, sNamingScreen->bg2vOffset);
    SetGpuReg(REG_OFFSET_BG1CNT, GetGpuReg(REG_OFFSET_BG1CNT) & 0xFFFC);
    SetGpuRegBits(REG_OFFSET_BG1CNT, sNamingScreen->bg1Priority);
    SetGpuReg(REG_OFFSET_BG2CNT, GetGpuReg(REG_OFFSET_BG2CNT) & 0xFFFC);
    SetGpuRegBits(REG_OFFSET_BG2CNT, sNamingScreen->bg2Priority);
}

static void NamingScreen_ShowBgs(void)
{
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

// Always false (presumably for non-latin languages)
static bool8 IsWideLetter(u8 character)
{
    u8 i;

    for (i = 0; sText_AlphabetUpperLower[i] != EOS; i++)
    {
        if (character == sText_AlphabetUpperLower[i])
            return FALSE;
    }
    return FALSE;
}

// Debug? Arguments aren't sensible for non-player screens.
static void UNUSED Debug_NamingScreenPlayer(void)
{
    DoNamingScreen(NAMING_SCREEN_PLAYER, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToFieldWithOpenMenu);
}

static void UNUSED Debug_NamingScreenBox(void)
{
    DoNamingScreen(NAMING_SCREEN_BOX, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToFieldWithOpenMenu);
}

static void UNUSED Debug_NamingScreenCaughtMon(void)
{
    DoNamingScreen(NAMING_SCREEN_CAUGHT_MON, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToFieldWithOpenMenu);
}

static void UNUSED Debug_NamingScreenNickname(void)
{
    DoNamingScreen(NAMING_SCREEN_NICKNAME, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToFieldWithOpenMenu);
}

//--------------------------------------------------
// Forward-declared variables
//--------------------------------------------------

// Initial pages below are pointless, they're overwritten with KBPAGE_LETTERS_UPPER in MainState_FadeIn()
static const struct NamingScreenTemplate sPlayerNamingScreenTemplate =
{
    .copyExistingString = FALSE,
    .maxChars = PLAYER_NAME_LENGTH,
    .iconFunction = 1,
    .addGenderIcon = FALSE,
    .initialPage = KBPAGE_LETTERS_UPPER,
    .unused = 35,
    .title = gText_YourName,
};

static const struct NamingScreenTemplate sPCBoxNamingTemplate =
{
    .copyExistingString = FALSE,
    .maxChars = BOX_NAME_LENGTH,
    .iconFunction = 2,
    .addGenderIcon = FALSE,
    .initialPage = KBPAGE_LETTERS_UPPER,
    .unused = 19,
    .title = gText_BoxName,
};

static const struct NamingScreenTemplate sMonNamingScreenTemplate =
{
    .copyExistingString = FALSE,
    .maxChars = POKEMON_NAME_LENGTH,
    .iconFunction = 3,
    .addGenderIcon = TRUE,
    .initialPage = KBPAGE_LETTERS_UPPER,
    .unused = 35,
    .title = gText_PkmnsNickname,
};

static const struct NamingScreenTemplate sWaldaWordsScreenTemplate =
{
    .copyExistingString = TRUE,
    .maxChars = WALDA_PHRASE_LENGTH,
    .iconFunction = 4,
    .addGenderIcon = FALSE,
    .initialPage = KBPAGE_LETTERS_UPPER,
    .unused = 11,
    .title = gText_TellHimTheWords,
};

static const struct NamingScreenTemplate *const sNamingScreenTemplates[] =
{
    [NAMING_SCREEN_PLAYER]     = &sPlayerNamingScreenTemplate,
    [NAMING_SCREEN_BOX]        = &sPCBoxNamingTemplate,
    [NAMING_SCREEN_CAUGHT_MON] = &sMonNamingScreenTemplate,
    [NAMING_SCREEN_NICKNAME]   = &sMonNamingScreenTemplate,
    [NAMING_SCREEN_WALDA]      = &sWaldaWordsScreenTemplate,
};

static const struct OamData sOam_8x8 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const struct OamData sOam_16x16 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const struct OamData sOam_32x16 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

/*
[0_____][] <-1   40x32
[2_____][] <-3
[4___+_][] <-5/Origin
[6     ][] <-7
*/
static const struct Subsprite sSubsprites_PageSwapFrame[] =
{
    {
        .x = -20,
        .y = -16,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 0,
        .priority = 1
    },
    {
        .x =  12,
        .y = -16,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 4,
        .priority = 1
    },
    {
        .x = -20,
        .y = -8,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 5,
        .priority = 1
    },
    {
        .x =  12,
        .y = -8,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 9,
        .priority = 1
    },
    {
        .x = -20,
        .y =  0,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 10,
        .priority = 1
    },
    {
        .x =  12,
        .y =  0,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 14,
        .priority = 1
    },
    {
        .x = -20,
        .y =  8,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 15,
        .priority = 1
    },
    {
        .x =  12,
        .y =  8,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 19,
        .priority = 1
    }
};

/*
[0_][] <-1    24x8
   ^-- Origin
*/
static const struct Subsprite sSubsprites_PageSwapText[] =
{
    {
        .x = -12,
        .y = -4,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 0,
        .priority = 1
    },
    {
        .x =   4,
        .y = -4,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 2,
        .priority = 1
    }
};

/*
[0_____][] <-1   40x24
[2_____][] <-3
[4___+_][] <-5/Origin
*/
static const struct Subsprite sSubsprites_Button[] =
{
    {
        .x = -20,
        .y = -12,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 0,
        .priority = 1
    },
    {
        .x =  12,
        .y = -12,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 4,
        .priority = 1
    },
    {
        .x = -20,
        .y = -4,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 5,
        .priority = 1
    },
    {
        .x =  12,
        .y = -4,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 9,
        .priority = 1
    },
    {
        .x = -20,
        .y =  4,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 10,
        .priority = 1
    },
    {
        .x =  12,
        .y =  4,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 14,
        .priority = 1
    }
};

/*
[0_]    16x24
[1+] <--Origin
[2_]
*/
static const struct Subsprite sSubsprites_PCIcon[] =
{
    {
        .x = -8,
        .y = -12,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 0,
        .priority = 3
    },
    {
        .x = -8,
        .y = -4,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 2,
        .priority = 3
    },
    {
        .x = -8,
        .y =  4,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 4,
        .priority = 3
    }
};

static const struct SubspriteTable sSubspriteTable_PageSwapFrame[] =
{
    {ARRAY_COUNT(sSubsprites_PageSwapFrame), sSubsprites_PageSwapFrame}
};

static const struct SubspriteTable sSubspriteTable_PageSwapText[] =
{
    {ARRAY_COUNT(sSubsprites_PageSwapText), sSubsprites_PageSwapText},
    {ARRAY_COUNT(sSubsprites_PageSwapText), sSubsprites_PageSwapText},
    {ARRAY_COUNT(sSubsprites_PageSwapText), sSubsprites_PageSwapText}
};

static const struct SubspriteTable sSubspriteTable_Button[] =
{
    {ARRAY_COUNT(sSubsprites_Button), sSubsprites_Button}
};

static const struct SubspriteTable sSubspriteTable_PCIcon[] =
{
    {ARRAY_COUNT(sSubsprites_PCIcon), sSubsprites_PCIcon}
};

static const struct SpriteFrameImage sImageTable_PCIcon[] =
{
    {sPCIconOff_Gfx, sizeof(sPCIconOff_Gfx)},
    {sPCIconOn_Gfx, sizeof(sPCIconOn_Gfx)},
};

static const union AnimCmd sAnim_Loop[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_CursorSquish[] =
{
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_FRAME(8, 8),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PCIcon[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(1, 2),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Loop[] =
{
    sAnim_Loop
};

static const union AnimCmd *const sAnims_Cursor[] =
{
    sAnim_Loop,
    sAnim_CursorSquish
};

static const union AnimCmd *const sAnims_PCIcon[] =
{
    sAnim_PCIcon
};

static const struct SpriteTemplate sSpriteTemplate_PageSwapFrame =
{
    .tileTag = GFXTAG_PAGE_SWAP_FRAME,
    .paletteTag = PALTAG_PAGE_SWAP,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_PageSwap
};

static const struct SpriteTemplate sSpriteTemplate_PageSwapButton =
{
    .tileTag = GFXTAG_PAGE_SWAP_BUTTON,
    .paletteTag = PALTAG_PAGE_SWAP_UPPER,
    .oam = &sOam_32x16,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_PageSwapText =
{
    .tileTag = GFXTAG_PAGE_SWAP_UPPER,
    .paletteTag = PALTAG_PAGE_SWAP,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_BackButton =
{
    .tileTag = GFXTAG_BACK_BUTTON,
    .paletteTag = PALTAG_BACK_BUTTON,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_OkButton =
{
    .tileTag = GFXTAG_OK_BUTTON,
    .paletteTag = PALTAG_OK_BUTTON,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = GFXTAG_CURSOR,
    .paletteTag = PALTAG_CURSOR,
    .oam = &sOam_16x16,
    .anims = sAnims_Cursor,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Cursor
};

static const struct SpriteTemplate sSpriteTemplate_InputArrow =
{
    .tileTag = GFXTAG_INPUT_ARROW,
    .paletteTag = PALTAG_PAGE_SWAP_OTHERS,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_InputArrow
};

static const struct SpriteTemplate sSpriteTemplate_Underscore =
{
    .tileTag = GFXTAG_UNDERSCORE,
    .paletteTag = PALTAG_PAGE_SWAP_OTHERS,
    .oam = &sOam_8x8,
    .anims = sAnims_Loop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Underscore
};

static const struct SpriteTemplate sSpriteTemplate_PCIcon =
{
    .tileTag = TAG_NONE,
    .paletteTag = PALTAG_MENU,
    .oam = &sOam_8x8,
    .anims = sAnims_PCIcon,
    .images = sImageTable_PCIcon,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const u8 *const sNamingScreenKeyboardText[KBPAGE_COUNT][KBROW_COUNT] =
{
    [KEYBOARD_LETTERS_LOWER] =
    {
        gText_NamingScreenKeyboard_abcdef,
        gText_NamingScreenKeyboard_ghijkl,
        gText_NamingScreenKeyboard_mnopqrs,
        gText_NamingScreenKeyboard_tuvwxyz
    },
    [KEYBOARD_LETTERS_UPPER] =
    {
        gText_NamingScreenKeyboard_ABCDEF,
        gText_NamingScreenKeyboard_GHIJKL,
        gText_NamingScreenKeyboard_MNOPQRS,
        gText_NamingScreenKeyboard_TUVWXYZ
    },
    [KEYBOARD_SYMBOLS] =
    {
        gText_NamingScreenKeyboard_01234,
        gText_NamingScreenKeyboard_56789,
        gText_NamingScreenKeyboard_Symbols1,
        gText_NamingScreenKeyboard_Symbols2
    },
};

static const struct SpriteSheet sSpriteSheets[] =
{
    {gNamingScreenBackButton_Gfx,     0x1E0,  GFXTAG_BACK_BUTTON},
    {gNamingScreenOKButton_Gfx,       0x1E0,  GFXTAG_OK_BUTTON},
    {gNamingScreenPageSwapFrame_Gfx,  0x280,  GFXTAG_PAGE_SWAP_FRAME},
    {gNamingScreenPageSwapButton_Gfx, 0x100,  GFXTAG_PAGE_SWAP_BUTTON},
    {gNamingScreenPageSwapUpper_Gfx,  0x060,  GFXTAG_PAGE_SWAP_UPPER},
    {gNamingScreenPageSwapLower_Gfx,  0x060,  GFXTAG_PAGE_SWAP_LOWER},
    {gNamingScreenPageSwapOthers_Gfx, 0x060,  GFXTAG_PAGE_SWAP_OTHERS},
    {gNamingScreenCursor_Gfx,         0x080,  GFXTAG_CURSOR},
    {gNamingScreenCursorSquished_Gfx, 0x080,  GFXTAG_CURSOR_SQUISHED},
    {gNamingScreenCursorFilled_Gfx,   0x080,  GFXTAG_CURSOR_FILLED},
    {gNamingScreenInputArrow_Gfx,     0x020,  GFXTAG_INPUT_ARROW},
    {gNamingScreenUnderscore_Gfx,     0x020,  GFXTAG_UNDERSCORE},
    {}
};

static const struct SpritePalette sSpritePalettes[] =
{
    {gNamingScreenMenu_Pal[0], PALTAG_MENU},
    {gNamingScreenMenu_Pal[1], PALTAG_PAGE_SWAP_UPPER},
    {gNamingScreenMenu_Pal[2], PALTAG_PAGE_SWAP_LOWER},
    {gNamingScreenMenu_Pal[3], PALTAG_PAGE_SWAP_OTHERS},
    {gNamingScreenMenu_Pal[4], PALTAG_PAGE_SWAP},
    {gNamingScreenMenu_Pal[5], PALTAG_CURSOR},
    {gNamingScreenMenu_Pal[4], PALTAG_BACK_BUTTON},
    {gNamingScreenMenu_Pal[4], PALTAG_OK_BUTTON},
    {}
};


