#include "global.h"
#include "trainer_pokemon_sprites.h"
#include "bg.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "field_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "link.h"
#include "main.h"
#include "menu.h"
#include "list_menu.h"
#include "mystery_event_menu.h"
#include "naming_screen.h"
#include "option_menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokeball.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "rtc.h"
#include "save.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "title_screen.h"
#include "window.h"
#include "mystery_gift_menu.h"

/*
 * Main menu state machine
 * -----------------------
 *
 * Entry point: CB2_InitMainMenu
 *
 * Note: States advance sequentially unless otherwise stated.
 *
 * CB2_InitMainMenu / CB2_ReinitMainMenu
 *  - Both of these states call InitMainMenu, which does all the work.
 *  - In the Reinit case, the init code will check if the user came from
 *    the options screen. If they did, then the options menu item is
 *    pre-selected.
 *
 * Task_MainMenuCheckSaveFile
 *  - Determines how many menu options to show based on whether
 *    the save file is Ok, empty, corrupted, etc.
 *  - If there was an error loading the save file, advance to
 *    Task_WaitForSaveFileErrorWindow.
 *  - If there were no errors, advance to Task_MainMenuCheckBattery.
 *  - Note that the check to enable Mystery Events would normally happen
 *    here, but this version of Emerald has them disabled.
 *
 * Task_WaitForSaveFileErrorWindow
 *  - Wait for the text to finish printing and then for the A button
 *    to be pressed.
 *
 * Task_MainMenuCheckBattery
 *  - If the battery is OK, advance to Task_DisplayMainMenu.
 *  - If the battery is dry, advance to Task_WaitForBatteryDryErrorWindow.
 *
 * Task_WaitForBatteryDryErrorWindow
 *  - Wait for the text to finish printing and then for the A button
 *    to be pressed.
 *
 * Task_DisplayMainWindow
 *  - Display the buttons to the user. If the menu is in HAS_MYSTERY_EVENTS
 *    mode, there are too many buttons for one screen and a scrollbar is added,
 *    and the scrollbar task is spawned (Task_ScrollIndicatorArrowPairOnMainMenu).
 *
 * Task_HighlightSelectedMainMenuItem
 *  - Update the UI to match the currently selected item.
 *
 * Task_HandleMainMenuInput
 *  - If A is pressed, advance to Task_HandleMainMenuAPressed.
 *  - If B is pressed, return to the title screen via CB2_InitTitleScreen.
 *  - If Up or Down is pressed, handle scrolling if there is a scroll bar, change
 *    the selection, then go back to Task_HighlightSelectedMainMenuItem.
 *
 * Task_HandleMainMenuAPressed
 *  - If the user selected New Game, advance to Task_NewGameBirchSpeech_Init.
 *  - If the user selected Continue, advance to CB2_ContinueSavedGame.
 *  - If the user selected the Options menu, advance to CB2_InitOptionMenu.
 *  - If the user selected Mystery Gift, advance to CB2_InitMysteryGift. However,
 *    if the wireless adapter was removed, instead advance to
 *    Task_DisplayMainMenuInvalidActionError.
 *  - Code to start a Mystery Event is present here, but is unreachable in this
 *    version.
 *
 * Task_HandleMainMenuBPressed
 *  - Clean up the main menu and go back to CB2_InitTitleScreen.
 *
 * Task_DisplayMainMenuInvalidActionError
 *  - Print one of three different error messages, wait for the text to stop
 *    printing, and then wait for A or B to be pressed.
 * - Then advance to Task_HandleMainMenuBPressed.
 *
 * Task_NewGameBirchSpeech_Init
 *  - Load the sprites for the intro speech, start playing music
 * Task_NewGameBirchSpeech_WaitToShowBirch
 *  - Spawn Task_NewGameBirchSpeech_FadeInTarget1OutTarget2
 *  - Spawn Task_NewGameBirchSpeech_FadePlatformOut
 *  - Both of these tasks destroy themselves when done.
 * Task_NewGameBirchSpeech_WaitForSpriteFadeInWelcome
 * Task_NewGameBirchSpeech_ThisIsAPokemon
 *  - When the text is done printing, spawns Task_NewGameBirchSpeechSub_InitPokeball
 * Task_NewGameBirchSpeech_MainSpeech
 * Task_NewGameBirchSpeech_AndYouAre
 * Task_NewGameBirchSpeech_StartBirchLotadPlatformFade
 * Task_NewGameBirchSpeech_StartBirchLotadPlatformFade
 * Task_NewGameBirchSpeech_SlidePlatformAway
 * Task_NewGameBirchSpeech_StartPlayerFadeIn
 * Task_NewGameBirchSpeech_WaitForPlayerFadeIn
 * Task_NewGameBirchSpeech_BoyOrGirl
 * Task_NewGameBirchSpeech_WaitToShowGenderMenu
 * Task_NewGameBirchSpeech_ChooseGender
 *  - Animates by advancing to Task_NewGameBirchSpeech_SlideOutOldGenderSprite
 *    whenever the player's selection changes.
 *  - Advances to Task_NewGameBirchSpeech_WhatsYourName when done.
 *
 * Task_NewGameBirchSpeech_SlideOutOldGenderSprite
 * Task_NewGameBirchSpeech_SlideInNewGenderSprite
 *  - Returns back to Task_NewGameBirchSpeech_ChooseGender.
 *
 * Task_NewGameBirchSpeech_WhatsYourName
 * Task_NewGameBirchSpeech_WaitForWhatsYourNameToPrint
 * Task_NewGameBirchSpeech_WaitPressBeforeNameChoice
 * Task_NewGameBirchSpeech_StartNamingScreen
 * C2_NamingScreen
 *  - Returns to CB2_NewGameBirchSpeech_ReturnFromNamingScreen when done
 * CB2_NewGameBirchSpeech_ReturnFromNamingScreen
 * Task_NewGameBirchSpeech_ReturnFromNamingScreenShowTextbox
 * Task_NewGameBirchSpeech_SoItsPlayerName
 * Task_NewGameBirchSpeech_CreateNameYesNo
 * Task_NewGameBirchSpeech_ProcessNameYesNoMenu
 *  - If confirmed, advance to Task_NewGameBirchSpeech_SlidePlatformAway2.
 *  - Otherwise, return to Task_NewGameBirchSpeech_BoyOrGirl.
 *
 * Task_NewGameBirchSpeech_SlidePlatformAway2
 * Task_NewGameBirchSpeech_ReshowBirchLotad
 * Task_NewGameBirchSpeech_WaitForSpriteFadeInAndTextPrinter
 * Task_NewGameBirchSpeech_AreYouReady
 * Task_NewGameBirchSpeech_ShrinkPlayer
 * Task_NewGameBirchSpeech_WaitForPlayerShrink
 * Task_NewGameBirchSpeech_FadePlayerToWhite
 * Task_NewGameBirchSpeech_Cleanup
 *  - Advances to CB2_NewGame.
 *
 * Task_NewGameBirchSpeechSub_InitPokeball
 *  - Advances to Task_NewGameBirchSpeechSub_WaitForLotad
 * Task_NewGameBirchSpeechSub_WaitForLotad
 *  - Destroys itself when done.
 */

#define OPTION_MENU_FLAG (1 << 15)

// Static type declarations

// Static RAM declarations

static EWRAM_DATA bool8 sStartedPokeBallTask = 0;
static EWRAM_DATA u16 sCurrItemAndOptionMenuCheck = 0;

static u8 sBirchSpeechMainTaskId;

// Static ROM declarations

static u32 InitMainMenu(bool8);
static void Task_MainMenuCheckSaveFile(u8);
static void Task_MainMenuCheckBattery(u8);
static void Task_WaitForSaveFileErrorWindow(u8);
static void CreateMainMenuErrorWindow(const u8 *);
static void ClearMainMenuWindowTilemap(const struct WindowTemplate *);
static void Task_DisplayMainMenu(u8);
static void Task_WaitForBatteryDryErrorWindow(u8);
static void MainMenu_FormatSavegameText(void);
static void HighlightSelectedMainMenuItem(u8, u8, s16);
static void Task_HandleMainMenuInput(u8);
static void Task_HandleMainMenuAPressed(u8);
static void Task_HandleMainMenuBPressed(u8);
static void Task_NewGameBirchSpeech_Init(u8);
static void Task_DisplayMainMenuInvalidActionError(u8);
static void AddBirchSpeechObjects(u8);
static void Task_NewGameBirchSpeech_WaitToShowBirch(u8);
static void NewGameBirchSpeech_StartFadeInTarget1OutTarget2(u8, u8);
static void NewGameBirchSpeech_StartFadePlatformOut(u8, u8);
static void Task_NewGameBirchSpeech_WaitForSpriteFadeInWelcome(u8);
static void NewGameBirchSpeech_ShowDialogueWindow(u8, u8);
static void NewGameBirchSpeech_ClearWindow(u8);
static void Task_NewGameBirchSpeech_ThisIsAPokemon(u8);
static void Task_NewGameBirchSpeech_MainSpeech(u8);
static void NewGameBirchSpeech_WaitForThisIsPokemonText(struct TextPrinterTemplate *, u16);
static void Task_NewGameBirchSpeech_AndYouAre(u8);
static void Task_NewGameBirchSpeechSub_WaitForLotad(u8);
static void Task_NewGameBirchSpeech_StartBirchLotadPlatformFade(u8);
static void NewGameBirchSpeech_StartFadeOutTarget1InTarget2(u8, u8);
static void NewGameBirchSpeech_StartFadePlatformIn(u8, u8);
static void Task_NewGameBirchSpeech_SlidePlatformAway(u8);
static void Task_NewGameBirchSpeech_StartPlayerFadeIn(u8);
static void Task_NewGameBirchSpeech_WaitForPlayerFadeIn(u8);
static void Task_NewGameBirchSpeech_BoyOrGirl(u8);
static void LoadMainMenuWindowFrameTiles(u8, u16);
static void DrawMainMenuWindowBorder(const struct WindowTemplate *, u16);
static void Task_HighlightSelectedMainMenuItem(u8);
static void Task_NewGameBirchSpeech_WaitToShowGenderMenu(u8);
static void Task_NewGameBirchSpeech_ChooseGender(u8);
static void NewGameBirchSpeech_ShowGenderMenu(void);
static s8 NewGameBirchSpeech_ProcessGenderMenuInput(void);
static void NewGameBirchSpeech_ClearGenderWindow(u8, u8);
static void Task_NewGameBirchSpeech_WhatsYourName(u8);
static void Task_NewGameBirchSpeech_SlideOutOldGenderSprite(u8);
static void Task_NewGameBirchSpeech_SlideInNewGenderSprite(u8);
static void Task_NewGameBirchSpeech_WaitForWhatsYourNameToPrint(u8);
static void Task_NewGameBirchSpeech_WaitPressBeforeNameChoice(u8);
static void Task_NewGameBirchSpeech_StartNamingScreen(u8);
static void CB2_NewGameBirchSpeech_ReturnFromNamingScreen(void);
static void NewGameBirchSpeech_SetDefaultPlayerName(u8);
static void Task_NewGameBirchSpeech_CreateNameYesNo(u8);
static void Task_NewGameBirchSpeech_ProcessNameYesNoMenu(u8);
void CreateYesNoMenuParameterized(u8, u8, u16, u16, u8, u8);
static void Task_NewGameBirchSpeech_SlidePlatformAway2(u8);
static void Task_NewGameBirchSpeech_ReshowBirchLotad(u8);
static void Task_NewGameBirchSpeech_WaitForSpriteFadeInAndTextPrinter(u8);
static void Task_NewGameBirchSpeech_AreYouReady(u8);
static void Task_NewGameBirchSpeech_ShrinkPlayer(u8);
static void SpriteCB_MovePlayerDownWhileShrinking(struct Sprite *);
static void Task_NewGameBirchSpeech_WaitForPlayerShrink(u8);
static void Task_NewGameBirchSpeech_FadePlayerToWhite(u8);
static void Task_NewGameBirchSpeech_Cleanup(u8);
static void SpriteCB_Null();
static void Task_NewGameBirchSpeech_ReturnFromNamingScreenShowTextbox(u8);
static void MainMenu_FormatSavegamePlayer(void);
static void MainMenu_FormatSavegamePokedex(void);
static void MainMenu_FormatSavegameTime(void);
static void MainMenu_FormatSavegameBadges(void);
static void NewGameBirchSpeech_CreateDialogueWindowBorder(u8, u8, u8, u8, u8, u8);

// .rodata

static const u16 sBirchSpeechBgPals[][16] = {
    INCBIN_U16("graphics/birch_speech/bg0.gbapal"),
    INCBIN_U16("graphics/birch_speech/bg1.gbapal")
};

static const u32 sBirchSpeechShadowGfx[] = INCBIN_U32("graphics/birch_speech/shadow.4bpp.lz");
static const u32 sBirchSpeechBgMap[] = INCBIN_U32("graphics/birch_speech/map.bin.lz");
static const u16 sBirchSpeechBgGradientPal[] = INCBIN_U16("graphics/birch_speech/bg2.gbapal");

#define MENU_LEFT 2
#define MENU_TOP_WIN0 1
#define MENU_TOP_WIN1 5
#define MENU_TOP_WIN2 1
#define MENU_TOP_WIN3 9
#define MENU_TOP_WIN4 13
#define MENU_TOP_WIN5 17
#define MENU_TOP_WIN6 21
#define MENU_WIDTH 26
#define MENU_HEIGHT_WIN0 2
#define MENU_HEIGHT_WIN1 2
#define MENU_HEIGHT_WIN2 6
#define MENU_HEIGHT_WIN3 2
#define MENU_HEIGHT_WIN4 2
#define MENU_HEIGHT_WIN5 2
#define MENU_HEIGHT_WIN6 2

#define MENU_LEFT_ERROR 2
#define MENU_TOP_ERROR 15
#define MENU_WIDTH_ERROR 26
#define MENU_HEIGHT_ERROR 4

#define MENU_SHADOW_PADDING 1

#define MENU_WIN_HCOORDS WIN_RANGE(((MENU_LEFT - 1) * 8) + MENU_SHADOW_PADDING, (MENU_LEFT + MENU_WIDTH + 1) * 8 - MENU_SHADOW_PADDING)
#define MENU_WIN_VCOORDS(n) WIN_RANGE(((MENU_TOP_WIN##n - 1) * 8) + MENU_SHADOW_PADDING, (MENU_TOP_WIN##n + MENU_HEIGHT_WIN##n + 1) * 8 - MENU_SHADOW_PADDING)
#define MENU_SCROLL_SHIFT WIN_RANGE(32, 32)

static const struct WindowTemplate sWindowTemplates_MainMenu[] =
{
    // No saved game
    // NEW GAME
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN0,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN0,
        .paletteNum = 15,
        .baseBlock = 1
    },
    // OPTIONS
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN1,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN1,
        .paletteNum = 15,
        .baseBlock = 0x35
    },
    // Has saved game
    // CONTINUE
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN2,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN2,
        .paletteNum = 15,
        .baseBlock = 1
    },
    // NEW GAME
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN3,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN3,
        .paletteNum = 15,
        .baseBlock = 0x9D
    },
    // OPTION / MYSTERY GIFT
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN4,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN4,
        .paletteNum = 15,
        .baseBlock = 0xD1
    },
    // OPTION / MYSTERY EVENTS
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN5,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN5,
        .paletteNum = 15,
        .baseBlock = 0x105
    },
    // OPTION
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT,
        .tilemapTop = MENU_TOP_WIN6,
        .width = MENU_WIDTH,
        .height = MENU_HEIGHT_WIN6,
        .paletteNum = 15,
        .baseBlock = 0x139
    },
    // Error message window
    {
        .bg = 0,
        .tilemapLeft = MENU_LEFT_ERROR,
        .tilemapTop = MENU_TOP_ERROR,
        .width = MENU_WIDTH_ERROR,
        .height = MENU_HEIGHT_ERROR,
        .paletteNum = 15,
        .baseBlock = 0x16D
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sNewGameBirchSpeechTextWindows[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1
    },
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 5,
        .width = 6,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x6D
    },
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 2,
        .width = 9,
        .height = 10,
        .paletteNum = 15,
        .baseBlock = 0x85
    },
    DUMMY_WIN_TEMPLATE
};

static const u16 sMainMenuBgPal[] = INCBIN_U16("graphics/interface/main_menu_bg.gbapal");
static const u16 sMainMenuTextPal[] = INCBIN_U16("graphics/interface/main_menu_text.gbapal");

static const u8 sTextColor_Headers[] = {TEXT_DYNAMIC_COLOR_1, TEXT_DYNAMIC_COLOR_2, TEXT_DYNAMIC_COLOR_3};
static const u8 sTextColor_MenuInfo[] = {TEXT_DYNAMIC_COLOR_1, TEXT_COLOR_WHITE, TEXT_DYNAMIC_COLOR_3};

static const struct BgTemplate sMainMenuBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct BgTemplate sBirchBgTemplate = {
    .bg = 0,
    .charBaseIndex = 3,
    .mapBaseIndex = 30,
    .screenSize = 0,
    .paletteMode = 0,
    .priority = 0,
    .baseTile = 0
};

static const struct ScrollArrowsTemplate sScrollArrowsTemplate_MainMenu = {2, 0x78, 8, 3, 0x78, 0x98, 3, 4, 1, 1, 0};

static const union AffineAnimCmd sSpriteAffineAnim_PlayerShrink[] = {
    AFFINEANIMCMD_FRAME(-2, -2, 0, 0x30),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sSpriteAffineAnimTable_PlayerShrink[] =
{
    sSpriteAffineAnim_PlayerShrink
};

static const struct MenuAction sMenuActions_Gender[] = {
    {gText_BirchBoy, {NULL}},
    {gText_BirchGirl, {NULL}}
};

static const u8 *const sMalePresetNames[] = {
    gText_DefaultNameStu,
    gText_DefaultNameMilton,
    gText_DefaultNameTom,
    gText_DefaultNameKenny,
    gText_DefaultNameReid,
    gText_DefaultNameJude,
    gText_DefaultNameJaxson,
    gText_DefaultNameEaston,
    gText_DefaultNameWalker,
    gText_DefaultNameTeru,
    gText_DefaultNameJohnny,
    gText_DefaultNameBrett,
    gText_DefaultNameSeth,
    gText_DefaultNameTerry,
    gText_DefaultNameCasey,
    gText_DefaultNameDarren,
    gText_DefaultNameLandon,
    gText_DefaultNameCollin,
    gText_DefaultNameStanley,
    gText_DefaultNameQuincy
};

static const u8 *const sFemalePresetNames[] = {
    gText_DefaultNameKimmy,
    gText_DefaultNameTiara,
    gText_DefaultNameBella,
    gText_DefaultNameJayla,
    gText_DefaultNameAllie,
    gText_DefaultNameLianna,
    gText_DefaultNameSara,
    gText_DefaultNameMonica,
    gText_DefaultNameCamila,
    gText_DefaultNameAubree,
    gText_DefaultNameRuthie,
    gText_DefaultNameHazel,
    gText_DefaultNameNadine,
    gText_DefaultNameTanja,
    gText_DefaultNameYasmin,
    gText_DefaultNameNicola,
    gText_DefaultNameLillie,
    gText_DefaultNameTerra,
    gText_DefaultNameLucy,
    gText_DefaultNameHalie
};

// The number of male vs. female names is assumed to be the same.
// If they aren't, the smaller of the two sizes will be used and any extra names will be ignored.
#define NUM_PRESET_NAMES min(ARRAY_COUNT(sMalePresetNames), ARRAY_COUNT(sFemalePresetNames))

enum
{
    HAS_NO_SAVED_GAME,  //NEW GAME, OPTION
    HAS_SAVED_GAME,     //CONTINUE, NEW GAME, OPTION
    HAS_MYSTERY_GIFT,   //CONTINUE, NEW GAME, MYSTERY GIFT, OPTION
    HAS_MYSTERY_EVENTS, //CONTINUE, NEW GAME, MYSTERY GIFT, MYSTERY EVENTS, OPTION
};

enum
{
    ACTION_NEW_GAME,
    ACTION_CONTINUE,
    ACTION_OPTION,
    ACTION_MYSTERY_GIFT,
    ACTION_MYSTERY_EVENTS,
    ACTION_EREADER,
    ACTION_INVALID
};

#define MAIN_MENU_BORDER_TILE   0x1D5

static void CB2_MainMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_MainMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitMainMenu(void)
{
    InitMainMenu(FALSE);
}

void CB2_ReinitMainMenu(void)
{
    InitMainMenu(TRUE);
}

static u32 InitMainMenu(bool8 returningFromOptionsMenu)
{
    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);

    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)(PLTT + 2), PLTT_SIZE - 2);

    ResetPaletteFade();
    LoadPalette(sMainMenuBgPal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(sMainMenuTextPal, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    if (returningFromOptionsMenu)
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK); // fade to black
    else
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_WHITEALPHA); // fade to white
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sMainMenuBgTemplates, ARRAY_COUNT(sMainMenuBgTemplates));
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    InitWindows(sWindowTemplates_MainMenu);
    DeactivateAllTextPrinters();
    LoadMainMenuWindowFrameTiles(0, MAIN_MENU_BORDER_TILE);

    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

    EnableInterrupts(1);
    SetVBlankCallback(VBlankCB_MainMenu);
    SetMainCallback2(CB2_MainMenu);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    HideBg(1);
    CreateTask(Task_MainMenuCheckSaveFile, 0);

    return 0;
}

#define tMenuType data[0]
#define tCurrItem data[1]
#define tItemCount data[12]
#define tScrollArrowTaskId data[13]
#define tIsScrolled data[14]
#define tWirelessAdapterConnected data[15]

#define tArrowTaskIsScrolled data[15]   // For scroll indicator arrow task

static void Task_MainMenuCheckSaveFile(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_OBJ);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_DARKEN | BLDCNT_TGT1_BG0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 7);

        if (IsWirelessAdapterConnected())
            tWirelessAdapterConnected = TRUE;
        switch (gSaveFileStatus)
        {
            case SAVE_STATUS_OK:
                tMenuType = HAS_SAVED_GAME;
                if (IsMysteryGiftEnabled())
                    tMenuType++;
                gTasks[taskId].func = Task_MainMenuCheckBattery;
                break;
            case SAVE_STATUS_CORRUPT:
                CreateMainMenuErrorWindow(gText_SaveFileErased);
                tMenuType = HAS_NO_SAVED_GAME;
                gTasks[taskId].func = Task_WaitForSaveFileErrorWindow;
                break;
            case SAVE_STATUS_ERROR:
                CreateMainMenuErrorWindow(gText_SaveFileCorrupted);
                gTasks[taskId].func = Task_WaitForSaveFileErrorWindow;
                tMenuType = HAS_SAVED_GAME;
                if (IsMysteryGiftEnabled() == TRUE)
                    tMenuType++;
                break;
            case SAVE_STATUS_EMPTY:
            default:
                tMenuType = HAS_NO_SAVED_GAME;
                gTasks[taskId].func = Task_MainMenuCheckBattery;
                break;
            case SAVE_STATUS_NO_FLASH:
                CreateMainMenuErrorWindow(gJPText_No1MSubCircuit);
                gTasks[taskId].tMenuType = HAS_NO_SAVED_GAME;
                gTasks[taskId].func = Task_WaitForSaveFileErrorWindow;
                break;
        }
        if (sCurrItemAndOptionMenuCheck & OPTION_MENU_FLAG)   // are we returning from the options menu?
        {
            switch (tMenuType)  // if so, highlight the OPTIONS item
            {
                case HAS_NO_SAVED_GAME:
                case HAS_SAVED_GAME:
                    sCurrItemAndOptionMenuCheck = tMenuType + 1;
                    break;
                case HAS_MYSTERY_GIFT:
                    sCurrItemAndOptionMenuCheck = 3;
                    break;
                case HAS_MYSTERY_EVENTS:
                    sCurrItemAndOptionMenuCheck = 4;
                    break;
            }
        }
        sCurrItemAndOptionMenuCheck &= ~OPTION_MENU_FLAG;  // turn off the "returning from options menu" flag
        tCurrItem = sCurrItemAndOptionMenuCheck;
        tItemCount = tMenuType + 2;
    }
}

static void Task_WaitForSaveFileErrorWindow(u8 taskId)
{
    RunTextPrinters();
    if (!IsTextPrinterActive(7) && (JOY_NEW(A_BUTTON)))
    {
        ClearWindowTilemap(7);
        ClearMainMenuWindowTilemap(&sWindowTemplates_MainMenu[7]);
        gTasks[taskId].func = Task_MainMenuCheckBattery;
    }
}

static void Task_MainMenuCheckBattery(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_OBJ);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_DARKEN | BLDCNT_TGT1_BG0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 7);

        if (!(RtcGetErrorStatus() & RTC_ERR_FLAG_MASK))
        {
            gTasks[taskId].func = Task_DisplayMainMenu;
        }
        else
        {
            CreateMainMenuErrorWindow(gText_BatteryRunDry);
            gTasks[taskId].func = Task_WaitForBatteryDryErrorWindow;
        }
    }
}

static void Task_WaitForBatteryDryErrorWindow(u8 taskId)
{
    RunTextPrinters();
    if (!IsTextPrinterActive(7) && (JOY_NEW(A_BUTTON)))
    {
        ClearWindowTilemap(7);
        ClearMainMenuWindowTilemap(&sWindowTemplates_MainMenu[7]);
        gTasks[taskId].func = Task_DisplayMainMenu;
    }
}

static void Task_DisplayMainMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 palette;

    if (!gPaletteFade.active)
    {
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_OBJ);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_DARKEN | BLDCNT_TGT1_BG0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 7);

        palette = RGB_BLACK;
        LoadPalette(&palette, BG_PLTT_ID(15) + 14, PLTT_SIZEOF(1));

        palette = RGB_WHITE;
        LoadPalette(&palette, BG_PLTT_ID(15) + 10, PLTT_SIZEOF(1));

        palette = RGB(12, 12, 12);
        LoadPalette(&palette, BG_PLTT_ID(15) + 11, PLTT_SIZEOF(1));

        palette = RGB(26, 26, 25);
        LoadPalette(&palette, BG_PLTT_ID(15) + 12, PLTT_SIZEOF(1));

        // Note: If there is no save file, the save block is zeroed out,
        // so the default gender is MALE.
        if (gSaveBlock2Ptr->playerGender == MALE)
        {
            palette = RGB(4, 16, 31);
            LoadPalette(&palette, BG_PLTT_ID(15) + 1, PLTT_SIZEOF(1));
        }
        else
        {
            palette = RGB(31, 3, 21);
            LoadPalette(&palette, BG_PLTT_ID(15) + 1, PLTT_SIZEOF(1));
        }

        switch (gTasks[taskId].tMenuType)
        {
            case HAS_NO_SAVED_GAME:
            default:
                FillWindowPixelBuffer(0, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(1, PIXEL_FILL(0xA));
                AddTextPrinterParameterized3(0, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuNewGame);
                AddTextPrinterParameterized3(1, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuOption);
                PutWindowTilemap(0);
                PutWindowTilemap(1);
                CopyWindowToVram(0, COPYWIN_GFX);
                CopyWindowToVram(1, COPYWIN_GFX);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[0], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[1], MAIN_MENU_BORDER_TILE);
                break;
            case HAS_SAVED_GAME:
                FillWindowPixelBuffer(2, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(3, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(4, PIXEL_FILL(0xA));
                AddTextPrinterParameterized3(2, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuContinue);
                AddTextPrinterParameterized3(3, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuNewGame);
                AddTextPrinterParameterized3(4, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuOption);
                MainMenu_FormatSavegameText();
                PutWindowTilemap(2);
                PutWindowTilemap(3);
                PutWindowTilemap(4);
                CopyWindowToVram(2, COPYWIN_GFX);
                CopyWindowToVram(3, COPYWIN_GFX);
                CopyWindowToVram(4, COPYWIN_GFX);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[2], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[3], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[4], MAIN_MENU_BORDER_TILE);
                break;
            case HAS_MYSTERY_GIFT:
                FillWindowPixelBuffer(2, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(3, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(4, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(5, PIXEL_FILL(0xA));
                AddTextPrinterParameterized3(2, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuContinue);
                AddTextPrinterParameterized3(3, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuNewGame);
                AddTextPrinterParameterized3(4, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuMysteryGift);
                AddTextPrinterParameterized3(5, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuOption);
                MainMenu_FormatSavegameText();
                PutWindowTilemap(2);
                PutWindowTilemap(3);
                PutWindowTilemap(4);
                PutWindowTilemap(5);
                CopyWindowToVram(2, COPYWIN_GFX);
                CopyWindowToVram(3, COPYWIN_GFX);
                CopyWindowToVram(4, COPYWIN_GFX);
                CopyWindowToVram(5, COPYWIN_GFX);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[2], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[3], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[4], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[5], MAIN_MENU_BORDER_TILE);
                break;
            case HAS_MYSTERY_EVENTS:
                FillWindowPixelBuffer(2, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(3, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(4, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(5, PIXEL_FILL(0xA));
                FillWindowPixelBuffer(6, PIXEL_FILL(0xA));
                AddTextPrinterParameterized3(2, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuContinue);
                AddTextPrinterParameterized3(3, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuNewGame);
                AddTextPrinterParameterized3(4, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuMysteryGift2);
                AddTextPrinterParameterized3(5, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuMysteryEvents);
                AddTextPrinterParameterized3(6, FONT_NORMAL, 0, 1, sTextColor_Headers, TEXT_SKIP_DRAW, gText_MainMenuOption);
                MainMenu_FormatSavegameText();
                PutWindowTilemap(2);
                PutWindowTilemap(3);
                PutWindowTilemap(4);
                PutWindowTilemap(5);
                PutWindowTilemap(6);
                CopyWindowToVram(2, COPYWIN_GFX);
                CopyWindowToVram(3, COPYWIN_GFX);
                CopyWindowToVram(4, COPYWIN_GFX);
                CopyWindowToVram(5, COPYWIN_GFX);
                CopyWindowToVram(6, COPYWIN_GFX);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[2], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[3], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[4], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[5], MAIN_MENU_BORDER_TILE);
                DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[6], MAIN_MENU_BORDER_TILE);
                tScrollArrowTaskId = AddScrollIndicatorArrowPair(&sScrollArrowsTemplate_MainMenu, &sCurrItemAndOptionMenuCheck);
                gTasks[tScrollArrowTaskId].func = Task_ScrollIndicatorArrowPairOnMainMenu;
                if (sCurrItemAndOptionMenuCheck == 4)
                {
                    ChangeBgY(0, 0x2000, BG_COORD_ADD);
                    ChangeBgY(1, 0x2000, BG_COORD_ADD);
                    tIsScrolled = TRUE;
                    gTasks[tScrollArrowTaskId].tArrowTaskIsScrolled = TRUE;
                }
                break;
        }
        gTasks[taskId].func = Task_HighlightSelectedMainMenuItem;
    }
}

static void Task_HighlightSelectedMainMenuItem(u8 taskId)
{
    HighlightSelectedMainMenuItem(gTasks[taskId].tMenuType, gTasks[taskId].tCurrItem, gTasks[taskId].tIsScrolled);
    gTasks[taskId].func = Task_HandleMainMenuInput;
}

static bool8 HandleMainMenuInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        IsWirelessAdapterConnected();   // why bother calling this here? debug? Task_HandleMainMenuAPressed will check too
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        gTasks[taskId].func = Task_HandleMainMenuAPressed;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_WHITEALPHA);
        SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, DISPLAY_WIDTH));
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, DISPLAY_HEIGHT));
        gTasks[taskId].func = Task_HandleMainMenuBPressed;
    }
    else if ((JOY_NEW(DPAD_UP)) && tCurrItem > 0)
    {
        if (tMenuType == HAS_MYSTERY_EVENTS && tIsScrolled == TRUE && tCurrItem == 1)
        {
            ChangeBgY(0, 0x2000, BG_COORD_SUB);
            ChangeBgY(1, 0x2000, BG_COORD_SUB);
            gTasks[tScrollArrowTaskId].tArrowTaskIsScrolled = tIsScrolled = FALSE;
        }
        tCurrItem--;
        sCurrItemAndOptionMenuCheck = tCurrItem;
        return TRUE;
    }
    else if ((JOY_NEW(DPAD_DOWN)) && tCurrItem < tItemCount - 1)
    {
        if (tMenuType == HAS_MYSTERY_EVENTS && tCurrItem == 3 && tIsScrolled == FALSE)
        {
            ChangeBgY(0, 0x2000, BG_COORD_ADD);
            ChangeBgY(1, 0x2000, BG_COORD_ADD);
            gTasks[tScrollArrowTaskId].tArrowTaskIsScrolled = tIsScrolled = TRUE;
        }
        tCurrItem++;
        sCurrItemAndOptionMenuCheck = tCurrItem;
        return TRUE;
    }
    return FALSE;
}

static void Task_HandleMainMenuInput(u8 taskId)
{
    if (HandleMainMenuInput(taskId))
        gTasks[taskId].func = Task_HighlightSelectedMainMenuItem;
}

static void Task_HandleMainMenuAPressed(u8 taskId)
{
    bool8 wirelessAdapterConnected;
    u8 action;

    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tMenuType == HAS_MYSTERY_EVENTS)
            RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
        ClearStdWindowAndFrame(0, TRUE);
        ClearStdWindowAndFrame(1, TRUE);
        ClearStdWindowAndFrame(2, TRUE);
        ClearStdWindowAndFrame(3, TRUE);
        ClearStdWindowAndFrame(4, TRUE);
        ClearStdWindowAndFrame(5, TRUE);
        ClearStdWindowAndFrame(6, TRUE);
        ClearStdWindowAndFrame(7, TRUE);
        wirelessAdapterConnected = IsWirelessAdapterConnected();
        switch (gTasks[taskId].tMenuType)
        {
            case HAS_NO_SAVED_GAME:
            default:
                switch (gTasks[taskId].tCurrItem)
                {
                    case 0:
                    default:
                        action = ACTION_NEW_GAME;
                        break;
                    case 1:
                        action = ACTION_OPTION;
                        break;
                }
                break;
            case HAS_SAVED_GAME:
                switch (gTasks[taskId].tCurrItem)
                {
                    case 0:
                    default:
                        action = ACTION_CONTINUE;
                        break;
                    case 1:
                        action = ACTION_NEW_GAME;
                        break;
                    case 2:
                        action = ACTION_OPTION;
                        break;
                }
                break;
            case HAS_MYSTERY_GIFT:
                switch (gTasks[taskId].tCurrItem)
                {
                    case 0:
                    default:
                        action = ACTION_CONTINUE;
                        break;
                    case 1:
                        action = ACTION_NEW_GAME;
                        break;
                    case 2:
                        action = ACTION_MYSTERY_GIFT;
                        if (!wirelessAdapterConnected)
                        {
                            action = ACTION_INVALID;
                            gTasks[taskId].tMenuType = HAS_NO_SAVED_GAME;
                        }
                        break;
                    case 3:
                        action = ACTION_OPTION;
                        break;
                }
                break;
            case HAS_MYSTERY_EVENTS:
                switch (gTasks[taskId].tCurrItem)
                {
                    case 0:
                    default:
                        action = ACTION_CONTINUE;
                        break;
                    case 1:
                        action = ACTION_NEW_GAME;
                        break;
                    case 2:
                        if (gTasks[taskId].tWirelessAdapterConnected)
                        {
                            action = ACTION_MYSTERY_GIFT;
                            if (!wirelessAdapterConnected)
                            {
                                action = ACTION_INVALID;
                                gTasks[taskId].tMenuType = HAS_NO_SAVED_GAME;
                            }
                        }
                        else if (wirelessAdapterConnected)
                        {
                            action = ACTION_INVALID;
                            gTasks[taskId].tMenuType = HAS_SAVED_GAME;
                        }
                        else
                        {
                            action = ACTION_EREADER;
                        }
                        break;
                    case 3:
                        if (wirelessAdapterConnected)
                        {
                            action = ACTION_INVALID;
                            gTasks[taskId].tMenuType = HAS_MYSTERY_GIFT;
                        }
                        else
                        {
                            action = ACTION_MYSTERY_EVENTS;
                        }
                        break;
                    case 4:
                        action = ACTION_OPTION;
                        break;
                }
                break;
        }
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        switch (action)
        {
            case ACTION_NEW_GAME:
            default:
                gPlttBufferUnfaded[0] = RGB_BLACK;
                gPlttBufferFaded[0] = RGB_BLACK;
                gTasks[taskId].func = Task_NewGameBirchSpeech_Init;
                break;
            case ACTION_CONTINUE:
                gPlttBufferUnfaded[0] = RGB_BLACK;
                gPlttBufferFaded[0] = RGB_BLACK;
                SetMainCallback2(CB2_ContinueSavedGame);
                DestroyTask(taskId);
                break;
            case ACTION_OPTION:
                gMain.savedCallback = CB2_ReinitMainMenu;
                SetMainCallback2(CB2_InitOptionMenu);
                DestroyTask(taskId);
                break;
            case ACTION_MYSTERY_GIFT:
                SetMainCallback2(CB2_InitMysteryGift);
                DestroyTask(taskId);
                break;
            case ACTION_MYSTERY_EVENTS:
                SetMainCallback2(CB2_InitMysteryEventMenu);
                DestroyTask(taskId);
                break;
            case ACTION_EREADER:
                SetMainCallback2(CB2_InitEReader);
                DestroyTask(taskId);
                break;
            case ACTION_INVALID:
                gTasks[taskId].tCurrItem = 0;
                gTasks[taskId].func = Task_DisplayMainMenuInvalidActionError;
                gPlttBufferUnfaded[BG_PLTT_ID(15) + 1] = RGB_WHITE;
                gPlttBufferFaded[BG_PLTT_ID(15) + 1] = RGB_WHITE;
                SetGpuReg(REG_OFFSET_BG2HOFS, 0);
                SetGpuReg(REG_OFFSET_BG2VOFS, 0);
                SetGpuReg(REG_OFFSET_BG1HOFS, 0);
                SetGpuReg(REG_OFFSET_BG1VOFS, 0);
                SetGpuReg(REG_OFFSET_BG0HOFS, 0);
                SetGpuReg(REG_OFFSET_BG0VOFS, 0);
                BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
                return;
        }
        FreeAllWindowBuffers();
        if (action != ACTION_OPTION)
            sCurrItemAndOptionMenuCheck = 0;
        else
            sCurrItemAndOptionMenuCheck |= OPTION_MENU_FLAG;  // entering the options menu
    }
}

static void Task_HandleMainMenuBPressed(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tMenuType == HAS_MYSTERY_EVENTS)
            RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
        sCurrItemAndOptionMenuCheck = 0;
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_InitTitleScreen);
        DestroyTask(taskId);
    }
}

static void Task_DisplayMainMenuInvalidActionError(u8 taskId)
{
    switch (gTasks[taskId].tCurrItem)
    {
        case 0:
            FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
            switch (gTasks[taskId].tMenuType)
            {
                case 0:
                    CreateMainMenuErrorWindow(gText_WirelessNotConnected);
                    break;
                case 1:
                    CreateMainMenuErrorWindow(gText_MysteryGiftCantUse);
                    break;
                case 2:
                    CreateMainMenuErrorWindow(gText_MysteryEventsCantUse);
                    break;
            }
            gTasks[taskId].tCurrItem++;
            break;
        case 1:
            if (!gPaletteFade.active)
                gTasks[taskId].tCurrItem++;
            break;
        case 2:
            RunTextPrinters();
            if (!IsTextPrinterActive(7))
                gTasks[taskId].tCurrItem++;
            break;
        case 3:
            if (JOY_NEW(A_BUTTON | B_BUTTON))
            {
                PlaySE(SE_SELECT);
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
                gTasks[taskId].func = Task_HandleMainMenuBPressed;
            }
    }
}

#undef tMenuType
#undef tCurrItem
#undef tItemCount
#undef tScrollArrowTaskId
#undef tIsScrolled
#undef tWirelessAdapterConnected

#undef tArrowTaskIsScrolled

static void HighlightSelectedMainMenuItem(u8 menuType, u8 selectedMenuItem, s16 isScrolled)
{
    SetGpuReg(REG_OFFSET_WIN0H, MENU_WIN_HCOORDS);

    switch (menuType)
    {
        case HAS_NO_SAVED_GAME:
        default:
            switch (selectedMenuItem)
            {
                case 0:
                default:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(0));
                    break;
                case 1:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(1));
                    break;
            }
            break;
        case HAS_SAVED_GAME:
            switch (selectedMenuItem)
            {
                case 0:
                default:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(2));
                    break;
                case 1:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(3));
                    break;
                case 2:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(4));
                    break;
            }
            break;
        case HAS_MYSTERY_GIFT:
            switch (selectedMenuItem)
            {
                case 0:
                default:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(2));
                    break;
                case 1:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(3));
                    break;
                case 2:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(4));
                    break;
                case 3:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(5));
                    break;
            }
            break;
        case HAS_MYSTERY_EVENTS:
            switch (selectedMenuItem)
            {
                case 0:
                default:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(2));
                    break;
                case 1:
                    if (isScrolled)
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(3) - MENU_SCROLL_SHIFT);
                    else
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(3));
                    break;
                case 2:
                    if (isScrolled)
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(4) - MENU_SCROLL_SHIFT);
                    else
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(4));
                    break;
                case 3:
                    if (isScrolled)
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(5) - MENU_SCROLL_SHIFT);
                    else
                        SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(5));
                    break;
                case 4:
                    SetGpuReg(REG_OFFSET_WIN0V, MENU_WIN_VCOORDS(6) - MENU_SCROLL_SHIFT);
                    break;
            }
            break;
    }
}

#define tPlayerSpriteId data[2]
#define tBG1HOFS data[4]
#define tIsDoneFadingSprites data[5]
#define tPlayerGender data[6]
#define tTimer data[7]
#define tBirchSpriteId data[8]
#define tLotadSpriteId data[9]
#define tBrendanSpriteId data[10]
#define tMaySpriteId data[11]

static void Task_NewGameBirchSpeech_Init(u8 taskId)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    InitBgFromTemplate(&sBirchBgTemplate);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

    LZ77UnCompVram(sBirchSpeechShadowGfx, (void *)VRAM);
    LZ77UnCompVram(sBirchSpeechBgMap, (void *)(BG_SCREEN_ADDR(7)));
    LoadPalette(sBirchSpeechBgPals, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
    LoadPalette(&sBirchSpeechBgGradientPal[8], BG_PLTT_ID(0) + 1, PLTT_SIZEOF(8));
    ScanlineEffect_Stop();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    AddBirchSpeechObjects(taskId);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    gTasks[taskId].tBG1HOFS = 0;
    gTasks[taskId].func = Task_NewGameBirchSpeech_WaitToShowBirch;
    gTasks[taskId].tPlayerSpriteId = SPRITE_NONE;
    gTasks[taskId].data[3] = 0xFF;
    gTasks[taskId].tTimer = 0xD8;
    PlayBGM(MUS_ROUTE122);
    ShowBg(0);
    ShowBg(1);
}

static void Task_NewGameBirchSpeech_WaitToShowBirch(u8 taskId)
{
    u8 spriteId;

    if (gTasks[taskId].tTimer)
    {
        gTasks[taskId].tTimer--;
    }
    else
    {
        spriteId = gTasks[taskId].tBirchSpriteId;
        gSprites[spriteId].x = 136;
        gSprites[spriteId].y = 60;
        gSprites[spriteId].invisible = FALSE;
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        NewGameBirchSpeech_StartFadeInTarget1OutTarget2(taskId, 10);
        NewGameBirchSpeech_StartFadePlatformOut(taskId, 20);
        gTasks[taskId].tTimer = 80;
        gTasks[taskId].func = Task_NewGameBirchSpeech_WaitForSpriteFadeInWelcome;
    }
}

static void Task_NewGameBirchSpeech_WaitForSpriteFadeInWelcome(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tBirchSpriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            InitWindows(sNewGameBirchSpeechTextWindows);
            LoadMainMenuWindowFrameTiles(0, 0xF3);
            LoadMessageBoxGfx(0, 0xFC, BG_PLTT_ID(15));
            NewGameBirchSpeech_ShowDialogueWindow(0, 1);
            PutWindowTilemap(0);
            CopyWindowToVram(0, COPYWIN_GFX);
            NewGameBirchSpeech_ClearWindow(0);
            StringExpandPlaceholders(gStringVarBuffer, gText_Birch_Welcome);
            AddTextPrinterForMessage(TRUE);
            gTasks[taskId].func = Task_NewGameBirchSpeech_ThisIsAPokemon;
        }
    }
}

static void Task_NewGameBirchSpeech_ThisIsAPokemon(u8 taskId)
{
    if (!gPaletteFade.active && !RunTextPrintersAndIsPrinter0Active())
    {
        gTasks[taskId].func = Task_NewGameBirchSpeech_MainSpeech;
        StringExpandPlaceholders(gStringVarBuffer, gText_ThisIsAPokemon);
        AddTextPrinterWithCallbackForMessage(TRUE, NewGameBirchSpeech_WaitForThisIsPokemonText);
        sBirchSpeechMainTaskId = taskId;
    }
}

static void Task_NewGameBirchSpeech_MainSpeech(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_Birch_MainSpeech);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameBirchSpeech_AndYouAre;
    }
}

#define tState data[0]

static void Task_NewGameBirchSpeechSub_InitPokeBall(u8 taskId)
{
    u8 spriteId = gTasks[sBirchSpeechMainTaskId].tLotadSpriteId;

    gSprites[spriteId].x = 100;
    gSprites[spriteId].y = 75;
    gSprites[spriteId].invisible = FALSE;
    gSprites[spriteId].data[0] = 0;

    CreatePokeballSpriteToReleaseMon(spriteId, gSprites[spriteId].oam.paletteNum, 112, 58, 0, 0, 32, PALETTES_BG, SPECIES_LOTAD);
    gTasks[taskId].func = Task_NewGameBirchSpeechSub_WaitForLotad;
    gTasks[sBirchSpeechMainTaskId].tTimer = 0;
}

static void Task_NewGameBirchSpeechSub_WaitForLotad(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    struct Sprite *sprite = &gSprites[gTasks[sBirchSpeechMainTaskId].tLotadSpriteId];

    switch (tState)
    {
        case 0:
            if (sprite->callback != SpriteCallbackDummy)
                return;
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            break;
        case 1:
            if (gTasks[sBirchSpeechMainTaskId].tTimer >= 96)
            {
                DestroyTask(taskId);
                if (gTasks[sBirchSpeechMainTaskId].tTimer < 0x4000)
                    gTasks[sBirchSpeechMainTaskId].tTimer++;
            }
            return;
    }
    tState++;
    if (gTasks[sBirchSpeechMainTaskId].tTimer < 0x4000)
        gTasks[sBirchSpeechMainTaskId].tTimer++;
}

#undef tState

static void Task_NewGameBirchSpeech_AndYouAre(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        sStartedPokeBallTask = FALSE;
        StringExpandPlaceholders(gStringVarBuffer, gText_Birch_AndYouAre);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameBirchSpeech_StartBirchLotadPlatformFade;
    }
}

static void Task_NewGameBirchSpeech_StartBirchLotadPlatformFade(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        gSprites[gTasks[taskId].tBirchSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        gSprites[gTasks[taskId].tLotadSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        NewGameBirchSpeech_StartFadeOutTarget1InTarget2(taskId, 2);
        NewGameBirchSpeech_StartFadePlatformIn(taskId, 1);
        gTasks[taskId].tTimer = 64;
        gTasks[taskId].func = Task_NewGameBirchSpeech_SlidePlatformAway;
    }
}

static void Task_NewGameBirchSpeech_SlidePlatformAway(u8 taskId)
{
    if (gTasks[taskId].tBG1HOFS != -60)
    {
        gTasks[taskId].tBG1HOFS -= 2;
        SetGpuReg(REG_OFFSET_BG1HOFS, gTasks[taskId].tBG1HOFS);
    }
    else
    {
        gTasks[taskId].tBG1HOFS = -60;
        gTasks[taskId].func = Task_NewGameBirchSpeech_StartPlayerFadeIn;
    }
}

static void Task_NewGameBirchSpeech_StartPlayerFadeIn(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tBirchSpriteId].invisible = TRUE;
        gSprites[gTasks[taskId].tLotadSpriteId].invisible = TRUE;
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            u8 spriteId = gTasks[taskId].tBrendanSpriteId;

            gSprites[spriteId].x = 180;
            gSprites[spriteId].y = 60;
            gSprites[spriteId].invisible = FALSE;
            gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
            gTasks[taskId].tPlayerSpriteId = spriteId;
            gTasks[taskId].tPlayerGender = MALE;
            NewGameBirchSpeech_StartFadeInTarget1OutTarget2(taskId, 2);
            NewGameBirchSpeech_StartFadePlatformOut(taskId, 1);
            gTasks[taskId].func = Task_NewGameBirchSpeech_WaitForPlayerFadeIn;
        }
    }
}

static void Task_NewGameBirchSpeech_WaitForPlayerFadeIn(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tPlayerSpriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
        gTasks[taskId].func = Task_NewGameBirchSpeech_BoyOrGirl;
    }
}

static void Task_NewGameBirchSpeech_BoyOrGirl(u8 taskId)
{
    NewGameBirchSpeech_ClearWindow(0);
    StringExpandPlaceholders(gStringVarBuffer, gText_Birch_BoyOrGirl);
    AddTextPrinterForMessage(TRUE);
    gTasks[taskId].func = Task_NewGameBirchSpeech_WaitToShowGenderMenu;
}

static void Task_NewGameBirchSpeech_WaitToShowGenderMenu(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        NewGameBirchSpeech_ShowGenderMenu();
        gTasks[taskId].func = Task_NewGameBirchSpeech_ChooseGender;
    }
}

static void Task_NewGameBirchSpeech_ChooseGender(u8 taskId)
{
    int gender = NewGameBirchSpeech_ProcessGenderMenuInput();
    int gender2;

    switch (gender)
    {
        case MALE:
            PlaySE(SE_SELECT);
            gSaveBlock2Ptr->playerGender = gender;
            NewGameBirchSpeech_ClearGenderWindow(1, 1);
            gTasks[taskId].func = Task_NewGameBirchSpeech_WhatsYourName;
            break;
        case FEMALE:
            PlaySE(SE_SELECT);
            gSaveBlock2Ptr->playerGender = gender;
            NewGameBirchSpeech_ClearGenderWindow(1, 1);
            gTasks[taskId].func = Task_NewGameBirchSpeech_WhatsYourName;
            break;
    }
    gender2 = Menu_GetCursorPos();
    if (gender2 != gTasks[taskId].tPlayerGender)
    {
        gTasks[taskId].tPlayerGender = gender2;
        gSprites[gTasks[taskId].tPlayerSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        NewGameBirchSpeech_StartFadeOutTarget1InTarget2(taskId, 0);
        gTasks[taskId].func = Task_NewGameBirchSpeech_SlideOutOldGenderSprite;
    }
}

static void Task_NewGameBirchSpeech_SlideOutOldGenderSprite(u8 taskId)
{
    u8 spriteId = gTasks[taskId].tPlayerSpriteId;
    if (gTasks[taskId].tIsDoneFadingSprites == 0)
    {
        gSprites[spriteId].x += 4;
    }
    else
    {
        gSprites[spriteId].invisible = TRUE;
        if (gTasks[taskId].tPlayerGender != MALE)
            spriteId = gTasks[taskId].tMaySpriteId;
        else
            spriteId = gTasks[taskId].tBrendanSpriteId;
        gSprites[spriteId].x = DISPLAY_WIDTH;
        gSprites[spriteId].y = 60;
        gSprites[spriteId].invisible = FALSE;
        gTasks[taskId].tPlayerSpriteId = spriteId;
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        NewGameBirchSpeech_StartFadeInTarget1OutTarget2(taskId, 0);
        gTasks[taskId].func = Task_NewGameBirchSpeech_SlideInNewGenderSprite;
    }
}

static void Task_NewGameBirchSpeech_SlideInNewGenderSprite(u8 taskId)
{
    u8 spriteId = gTasks[taskId].tPlayerSpriteId;

    if (gSprites[spriteId].x > 180)
    {
        gSprites[spriteId].x -= 4;
    }
    else
    {
        gSprites[spriteId].x = 180;
        if (gTasks[taskId].tIsDoneFadingSprites)
        {
            gSprites[spriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
            gTasks[taskId].func = Task_NewGameBirchSpeech_ChooseGender;
        }
    }
}

static void Task_NewGameBirchSpeech_WhatsYourName(u8 taskId)
{
    NewGameBirchSpeech_ClearWindow(0);
    StringExpandPlaceholders(gStringVarBuffer, gText_Birch_WhatsYourName);
    AddTextPrinterForMessage(TRUE);
    gTasks[taskId].func = Task_NewGameBirchSpeech_WaitForWhatsYourNameToPrint;
}

static void Task_NewGameBirchSpeech_WaitForWhatsYourNameToPrint(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
        gTasks[taskId].func = Task_NewGameBirchSpeech_WaitPressBeforeNameChoice;
}

static void Task_NewGameBirchSpeech_WaitPressBeforeNameChoice(u8 taskId)
{
    if ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON)))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_NewGameBirchSpeech_StartNamingScreen;
    }
}

static void Task_NewGameBirchSpeech_StartNamingScreen(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        FreeAndDestroyMonPicSprite(gTasks[taskId].tLotadSpriteId);
        NewGameBirchSpeech_SetDefaultPlayerName(Random() % NUM_PRESET_NAMES);
        DestroyTask(taskId);
        DoNamingScreen(NAMING_SCREEN_PLAYER, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_NewGameBirchSpeech_ReturnFromNamingScreen);
    }
}

static void Task_NewGameBirchSpeech_SoItsPlayerName(u8 taskId)
{
    NewGameBirchSpeech_ClearWindow(0);
    StringExpandPlaceholders(gStringVarBuffer, gText_Birch_SoItsPlayer);
    AddTextPrinterForMessage(TRUE);
    gTasks[taskId].func = Task_NewGameBirchSpeech_CreateNameYesNo;
}

static void Task_NewGameBirchSpeech_CreateNameYesNo(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        CreateYesNoMenuParameterized(2, 1, 0xF3, 0xDF, 2, 15);
        gTasks[taskId].func = Task_NewGameBirchSpeech_ProcessNameYesNoMenu;
    }
}

static void Task_NewGameBirchSpeech_ProcessNameYesNoMenu(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
        case 0:
            PlaySE(SE_SELECT);
            gSprites[gTasks[taskId].tPlayerSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
            NewGameBirchSpeech_StartFadeOutTarget1InTarget2(taskId, 2);
            NewGameBirchSpeech_StartFadePlatformIn(taskId, 1);
            gTasks[taskId].func = Task_NewGameBirchSpeech_SlidePlatformAway2;
            break;
        case MENU_B_PRESSED:
        case 1:
            PlaySE(SE_SELECT);
            gTasks[taskId].func = Task_NewGameBirchSpeech_BoyOrGirl;
    }
}

static void Task_NewGameBirchSpeech_SlidePlatformAway2(u8 taskId)
{
    if (gTasks[taskId].tBG1HOFS)
    {
        gTasks[taskId].tBG1HOFS += 2;
        SetGpuReg(REG_OFFSET_BG1HOFS, gTasks[taskId].tBG1HOFS);
    }
    else
    {
        gTasks[taskId].func = Task_NewGameBirchSpeech_ReshowBirchLotad;
    }
}

static void Task_NewGameBirchSpeech_ReshowBirchLotad(u8 taskId)
{
    u8 spriteId;

    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tBrendanSpriteId].invisible = TRUE;
        gSprites[gTasks[taskId].tMaySpriteId].invisible = TRUE;
        spriteId = gTasks[taskId].tBirchSpriteId;
        gSprites[spriteId].x = 136;
        gSprites[spriteId].y = 60;
        gSprites[spriteId].invisible = FALSE;
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        spriteId = gTasks[taskId].tLotadSpriteId;
        gSprites[spriteId].x = 100;
        gSprites[spriteId].y = 75;
        gSprites[spriteId].invisible = FALSE;
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        NewGameBirchSpeech_StartFadeInTarget1OutTarget2(taskId, 2);
        NewGameBirchSpeech_StartFadePlatformOut(taskId, 1);
        NewGameBirchSpeech_ClearWindow(0);
        StringExpandPlaceholders(gStringVarBuffer, gText_Birch_YourePlayer);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameBirchSpeech_WaitForSpriteFadeInAndTextPrinter;
    }
}

static void Task_NewGameBirchSpeech_WaitForSpriteFadeInAndTextPrinter(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tBirchSpriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
        gSprites[gTasks[taskId].tLotadSpriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
        if (!RunTextPrintersAndIsPrinter0Active())
        {
            gSprites[gTasks[taskId].tBirchSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
            gSprites[gTasks[taskId].tLotadSpriteId].oam.objMode = ST_OAM_OBJ_BLEND;
            NewGameBirchSpeech_StartFadeOutTarget1InTarget2(taskId, 2);
            NewGameBirchSpeech_StartFadePlatformIn(taskId, 1);
            gTasks[taskId].tTimer = 64;
            gTasks[taskId].func = Task_NewGameBirchSpeech_AreYouReady;
        }
    }
}

static void Task_NewGameBirchSpeech_AreYouReady(u8 taskId)
{
    u8 spriteId;

    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tBirchSpriteId].invisible = TRUE;
        gSprites[gTasks[taskId].tLotadSpriteId].invisible = TRUE;
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
            return;
        }
        if (gSaveBlock2Ptr->playerGender != MALE)
            spriteId = gTasks[taskId].tMaySpriteId;
        else
            spriteId = gTasks[taskId].tBrendanSpriteId;
        gSprites[spriteId].x = 120;
        gSprites[spriteId].y = 60;
        gSprites[spriteId].invisible = FALSE;
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        gTasks[taskId].tPlayerSpriteId = spriteId;
        NewGameBirchSpeech_StartFadeInTarget1OutTarget2(taskId, 2);
        NewGameBirchSpeech_StartFadePlatformOut(taskId, 1);
        StringExpandPlaceholders(gStringVarBuffer, gText_Birch_AreYouReady);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameBirchSpeech_ShrinkPlayer;
    }
}

static void Task_NewGameBirchSpeech_ShrinkPlayer(u8 taskId)
{
    u8 spriteId;

    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        gSprites[gTasks[taskId].tPlayerSpriteId].oam.objMode = ST_OAM_OBJ_NORMAL;
        if (!RunTextPrintersAndIsPrinter0Active())
        {
            spriteId = gTasks[taskId].tPlayerSpriteId;
            gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
            gSprites[spriteId].affineAnims = sSpriteAffineAnimTable_PlayerShrink;
            InitSpriteAffineAnim(&gSprites[spriteId]);
            StartSpriteAffineAnim(&gSprites[spriteId], 0);
            gSprites[spriteId].callback = SpriteCB_MovePlayerDownWhileShrinking;
            BeginNormalPaletteFade(PALETTES_BG, 0, 0, 16, RGB_BLACK);
            FadeOutBGM(4);
            gTasks[taskId].func = Task_NewGameBirchSpeech_WaitForPlayerShrink;
        }
    }
}

static void Task_NewGameBirchSpeech_WaitForPlayerShrink(u8 taskId)
{
    u8 spriteId = gTasks[taskId].tPlayerSpriteId;

    if (gSprites[spriteId].affineAnimEnded)
        gTasks[taskId].func = Task_NewGameBirchSpeech_FadePlayerToWhite;
}

static void Task_NewGameBirchSpeech_FadePlayerToWhite(u8 taskId)
{
    u8 spriteId;

    if (!gPaletteFade.active)
    {
        spriteId = gTasks[taskId].tPlayerSpriteId;
        gSprites[spriteId].callback = SpriteCB_Null;
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        BeginNormalPaletteFade(PALETTES_OBJECTS, 0, 0, 16, RGB_WHITEALPHA);
        gTasks[taskId].func = Task_NewGameBirchSpeech_Cleanup;
    }
}

static void Task_NewGameBirchSpeech_Cleanup(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        FreeAndDestroyMonPicSprite(gTasks[taskId].tLotadSpriteId);
        ResetAllPicSprites();
        SetMainCallback2(CB2_NewGame);
        DestroyTask(taskId);
    }
}

static void CB2_NewGameBirchSpeech_ReturnFromNamingScreen(void)
{
    u8 taskId;
    u8 spriteId;
    u16 savedIme;

    ResetBgsAndClearDma3BusyFlags(0);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    InitBgsFromTemplates(0, sMainMenuBgTemplates, ARRAY_COUNT(sMainMenuBgTemplates));
    InitBgFromTemplate(&sBirchBgTemplate);
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ResetPaletteFade();
    LZ77UnCompVram(sBirchSpeechShadowGfx, (u8 *)VRAM);
    LZ77UnCompVram(sBirchSpeechBgMap, (u8 *)(BG_SCREEN_ADDR(7)));
    LoadPalette(sBirchSpeechBgPals, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
    LoadPalette(&sBirchSpeechBgGradientPal[1], BG_PLTT_ID(0) + 1, PLTT_SIZEOF(8));
    ResetTasks();
    taskId = CreateTask(Task_NewGameBirchSpeech_ReturnFromNamingScreenShowTextbox, 0);
    gTasks[taskId].tTimer = 5;
    gTasks[taskId].tBG1HOFS = -60;
    ScanlineEffect_Stop();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    AddBirchSpeechObjects(taskId);
    if (gSaveBlock2Ptr->playerGender != MALE)
    {
        gTasks[taskId].tPlayerGender = FEMALE;
        spriteId = gTasks[taskId].tMaySpriteId;
    }
    else
    {
        gTasks[taskId].tPlayerGender = MALE;
        spriteId = gTasks[taskId].tBrendanSpriteId;
    }
    gSprites[spriteId].x = 180;
    gSprites[spriteId].y = 60;
    gSprites[spriteId].invisible = FALSE;
    gTasks[taskId].tPlayerSpriteId = spriteId;
    SetGpuReg(REG_OFFSET_BG1HOFS, -60);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    ShowBg(0);
    ShowBg(1);
    savedIme = REG_IME;
    REG_IME = 0;
    REG_IE |= 1;
    REG_IME = savedIme;
    SetVBlankCallback(VBlankCB_MainMenu);
    SetMainCallback2(CB2_MainMenu);
    InitWindows(sNewGameBirchSpeechTextWindows);
    LoadMainMenuWindowFrameTiles(0, 0xF3);
    LoadMessageBoxGfx(0, 0xFC, BG_PLTT_ID(15));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void SpriteCB_Null(struct Sprite *sprite)
{
}

static void SpriteCB_MovePlayerDownWhileShrinking(struct Sprite *sprite)
{
    u32 y;

    y = (sprite->y << 16) + sprite->data[0] + 0xC000;
    sprite->y = y >> 16;
    sprite->data[0] = y;
}

static u8 NewGameBirchSpeech_CreateLotadSprite(u8 x, u8 y)
{
    return CreateMonPicSprite_Affine(SPECIES_LOTAD, SHINY_ODDS, 0, MON_PIC_AFFINE_FRONT, x, y, 14, TAG_NONE);
}

static void AddBirchSpeechObjects(u8 taskId)
{
    u8 birchSpriteId;
    u8 lotadSpriteId;
    u8 brendanSpriteId;
    u8 maySpriteId;

    birchSpriteId = AddNewGameBirchObject(0x88, 0x3C, 1);
    gSprites[birchSpriteId].callback = SpriteCB_Null;
    gSprites[birchSpriteId].oam.priority = 0;
    gSprites[birchSpriteId].invisible = TRUE;
    gTasks[taskId].tBirchSpriteId = birchSpriteId;
    lotadSpriteId = NewGameBirchSpeech_CreateLotadSprite(100, 0x4B);
    gSprites[lotadSpriteId].callback = SpriteCB_Null;
    gSprites[lotadSpriteId].oam.priority = 0;
    gSprites[lotadSpriteId].invisible = TRUE;
    gTasks[taskId].tLotadSpriteId = lotadSpriteId;
    brendanSpriteId = CreateTrainerSprite(FacilityClassToPicIndex(FACILITY_CLASS_BRENDAN), 120, 60, 0, &gDecompressionBuffer[0]);
    gSprites[brendanSpriteId].callback = SpriteCB_Null;
    gSprites[brendanSpriteId].invisible = TRUE;
    gSprites[brendanSpriteId].oam.priority = 0;
    gTasks[taskId].tBrendanSpriteId = brendanSpriteId;
    maySpriteId = CreateTrainerSprite(FacilityClassToPicIndex(FACILITY_CLASS_MAY), 120, 60, 0, &gDecompressionBuffer[TRAINER_PIC_SIZE]);
    gSprites[maySpriteId].callback = SpriteCB_Null;
    gSprites[maySpriteId].invisible = TRUE;
    gSprites[maySpriteId].oam.priority = 0;
    gTasks[taskId].tMaySpriteId = maySpriteId;
}

#undef tPlayerSpriteId
#undef tBG1HOFS
#undef tPlayerGender
#undef tBirchSpriteId
#undef tLotadSpriteId
#undef tBrendanSpriteId
#undef tMaySpriteId

#define tMainTask data[0]
#define tAlphaCoeff1 data[1]
#define tAlphaCoeff2 data[2]
#define tDelay data[3]
#define tDelayTimer data[4]

static void Task_NewGameBirchSpeech_FadeOutTarget1InTarget2(u8 taskId)
{
    int alphaCoeff2;

    if (gTasks[taskId].tAlphaCoeff1 == 0)
    {
        gTasks[gTasks[taskId].tMainTask].tIsDoneFadingSprites = TRUE;
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tAlphaCoeff1--;
        gTasks[taskId].tAlphaCoeff2++;
        alphaCoeff2 = gTasks[taskId].tAlphaCoeff2 << 8;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].tAlphaCoeff1 + alphaCoeff2);
    }
}

static void NewGameBirchSpeech_StartFadeOutTarget1InTarget2(u8 taskId, u8 delay)
{
    u8 taskId2;

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_OBJ);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 0));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gTasks[taskId].tIsDoneFadingSprites = 0;
    taskId2 = CreateTask(Task_NewGameBirchSpeech_FadeOutTarget1InTarget2, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tAlphaCoeff1 = 16;
    gTasks[taskId2].tAlphaCoeff2 = 0;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

static void Task_NewGameBirchSpeech_FadeInTarget1OutTarget2(u8 taskId)
{
    int alphaCoeff2;

    if (gTasks[taskId].tAlphaCoeff1 == 16)
    {
        gTasks[gTasks[taskId].tMainTask].tIsDoneFadingSprites = TRUE;
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tAlphaCoeff1++;
        gTasks[taskId].tAlphaCoeff2--;
        alphaCoeff2 = gTasks[taskId].tAlphaCoeff2 << 8;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].tAlphaCoeff1 + alphaCoeff2);
    }
}

static void NewGameBirchSpeech_StartFadeInTarget1OutTarget2(u8 taskId, u8 delay)
{
    u8 taskId2;

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_OBJ);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gTasks[taskId].tIsDoneFadingSprites = 0;
    taskId2 = CreateTask(Task_NewGameBirchSpeech_FadeInTarget1OutTarget2, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tAlphaCoeff1 = 0;
    gTasks[taskId2].tAlphaCoeff2 = 16;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

#undef tMainTask
#undef tAlphaCoeff1
#undef tAlphaCoeff2
#undef tDelay
#undef tDelayTimer

#undef tIsDoneFadingSprites

#define tMainTask data[0]
#define tPalIndex data[1]
#define tDelayBefore data[2]
#define tDelay data[3]
#define tDelayTimer data[4]

static void Task_NewGameBirchSpeech_FadePlatformIn(u8 taskId)
{
    if (gTasks[taskId].tDelayBefore)
    {
        gTasks[taskId].tDelayBefore--;
    }
    else if (gTasks[taskId].tPalIndex == 8)
    {
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tPalIndex++;
        LoadPalette(&sBirchSpeechBgGradientPal[gTasks[taskId].tPalIndex], BG_PLTT_ID(0) + 1, PLTT_SIZEOF(8));
    }
}

static void NewGameBirchSpeech_StartFadePlatformIn(u8 taskId, u8 delay)
{
    u8 taskId2;

    taskId2 = CreateTask(Task_NewGameBirchSpeech_FadePlatformIn, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tPalIndex = 0;
    gTasks[taskId2].tDelayBefore = 8;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

static void Task_NewGameBirchSpeech_FadePlatformOut(u8 taskId)
{
    if (gTasks[taskId].tDelayBefore)
    {
        gTasks[taskId].tDelayBefore--;
    }
    else if (gTasks[taskId].tPalIndex == 0)
    {
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tPalIndex--;
        LoadPalette(&sBirchSpeechBgGradientPal[gTasks[taskId].tPalIndex], BG_PLTT_ID(0) + 1, PLTT_SIZEOF(8));
    }
}

static void NewGameBirchSpeech_StartFadePlatformOut(u8 taskId, u8 delay)
{
    u8 taskId2;

    taskId2 = CreateTask(Task_NewGameBirchSpeech_FadePlatformOut, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tPalIndex = 8;
    gTasks[taskId2].tDelayBefore = 8;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

#undef tMainTask
#undef tPalIndex
#undef tDelayBefore
#undef tDelay
#undef tDelayTimer

static void NewGameBirchSpeech_ShowGenderMenu(void)
{
    DrawMainMenuWindowBorder(&sNewGameBirchSpeechTextWindows[1], 0xF3);
    FillWindowPixelBuffer(1, PIXEL_FILL(1));
    PrintMenuTable(1, ARRAY_COUNT(sMenuActions_Gender), sMenuActions_Gender);
    InitMenuInUpperLeftCornerNormal(1, ARRAY_COUNT(sMenuActions_Gender), 0);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static s8 NewGameBirchSpeech_ProcessGenderMenuInput(void)
{
    return Menu_ProcessInputNoWrap();
}

static void NewGameBirchSpeech_SetDefaultPlayerName(u8 nameId)
{
    const u8 *name;
    u8 i;

    if (gSaveBlock2Ptr->playerGender == MALE)
        name = sMalePresetNames[nameId];
    else
        name = sFemalePresetNames[nameId];
    for (i = 0; i < PLAYER_NAME_LENGTH; i++)
        gSaveBlock2Ptr->playerName[i] = name[i];
    gSaveBlock2Ptr->playerName[PLAYER_NAME_LENGTH] = EOS;
}

static void CreateMainMenuErrorWindow(const u8 *str)
{
    FillWindowPixelBuffer(7, PIXEL_FILL(1));
    AddTextPrinterParameterized(7, FONT_NORMAL, str, 0, 1, 2, 0);
    PutWindowTilemap(7);
    CopyWindowToVram(7, COPYWIN_GFX);
    DrawMainMenuWindowBorder(&sWindowTemplates_MainMenu[7], MAIN_MENU_BORDER_TILE);
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(9, DISPLAY_WIDTH - 9));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(113, DISPLAY_HEIGHT - 1));
}

static void MainMenu_FormatSavegameText(void)
{
    MainMenu_FormatSavegamePlayer();
    MainMenu_FormatSavegamePokedex();
    MainMenu_FormatSavegameTime();
    MainMenu_FormatSavegameBadges();
}

static void MainMenu_FormatSavegamePlayer(void)
{
    StringExpandPlaceholders(gStringVarBuffer, gText_ContinueMenuPlayer);
    AddTextPrinterParameterized3(2, FONT_NORMAL, 0, 17, sTextColor_MenuInfo, TEXT_SKIP_DRAW, gStringVarBuffer);
    AddTextPrinterParameterized3(2, FONT_NORMAL, GetStringRightAlignXOffset(FONT_NORMAL, gSaveBlock2Ptr->playerName, 100), 17, sTextColor_MenuInfo, TEXT_SKIP_DRAW, gSaveBlock2Ptr->playerName);
}

static void MainMenu_FormatSavegameTime(void)
{
    u8 str[0x20];
    u8 *ptr;

    StringExpandPlaceholders(gStringVarBuffer, gText_ContinueMenuTime);
    AddTextPrinterParameterized3(2, FONT_NORMAL, 0x6C, 17, sTextColor_MenuInfo, TEXT_SKIP_DRAW, gStringVarBuffer);
    ptr = ConvertIntToDecimalStringN(str, gSaveBlock2Ptr->playTimeHours, STR_CONV_MODE_LEFT_ALIGN, 3);
    *ptr = 0xF0;
    ConvertIntToDecimalStringN(ptr + 1, gSaveBlock2Ptr->playTimeMinutes, STR_CONV_MODE_LEADING_ZEROS, 2);
    AddTextPrinterParameterized3(2, FONT_NORMAL, GetStringRightAlignXOffset(FONT_NORMAL, str, 0xD0), 17, sTextColor_MenuInfo, TEXT_SKIP_DRAW, str);
}

static void MainMenu_FormatSavegamePokedex(void)
{
    u8 str[0x20];
    u16 dexCount;

    if (FlagGet(FLAG_SYS_POKEDEX_GET) == TRUE)
    {
        if (IsNationalPokedexEnabled())
            dexCount = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        else
            dexCount = GetHoennPokedexCount(FLAG_GET_CAUGHT);
        StringExpandPlaceholders(gStringVarBuffer, gText_ContinueMenuPokedex);
        AddTextPrinterParameterized3(2, FONT_NORMAL, 0, 33, sTextColor_MenuInfo, TEXT_SKIP_DRAW, gStringVarBuffer);
        ConvertIntToDecimalStringN(str, dexCount, STR_CONV_MODE_LEFT_ALIGN, 3);
        AddTextPrinterParameterized3(2, FONT_NORMAL, GetStringRightAlignXOffset(FONT_NORMAL, str, 100), 33, sTextColor_MenuInfo, TEXT_SKIP_DRAW, str);
    }
}

static void MainMenu_FormatSavegameBadges(void)
{
    u8 str[0x20];
    u8 badgeCount = 0;
    u32 i;

    for (i = FLAG_BADGE01_GET; i < FLAG_BADGE01_GET + NUM_BADGES; i++)
    {
        if (FlagGet(i))
            badgeCount++;
    }
    StringExpandPlaceholders(gStringVarBuffer, gText_ContinueMenuBadges);
    AddTextPrinterParameterized3(2, FONT_NORMAL, 0x6C, 33, sTextColor_MenuInfo, TEXT_SKIP_DRAW, gStringVarBuffer);
    ConvertIntToDecimalStringN(str, badgeCount, STR_CONV_MODE_LEADING_ZEROS, 1);
    AddTextPrinterParameterized3(2, FONT_NORMAL, GetStringRightAlignXOffset(FONT_NORMAL, str, 0xD0), 33, sTextColor_MenuInfo, TEXT_SKIP_DRAW, str);
}

static void LoadMainMenuWindowFrameTiles(u8 bgId, u16 tileOffset)
{
    LoadBgTiles(bgId, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, tileOffset);
    LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
}

static void DrawMainMenuWindowBorder(const struct WindowTemplate *template, u16 baseTileNum)
{
    u16 r9 = 1 + baseTileNum;
    u16 r10 = 2 + baseTileNum;
    u16 sp18 = 3 + baseTileNum;
    u16 spC = 5 + baseTileNum;
    u16 sp10 = 6 + baseTileNum;
    u16 sp14 = 7 + baseTileNum;
    u16 r6 = 8 + baseTileNum;

    FillBgTilemapBufferRect(template->bg, baseTileNum, template->tilemapLeft - 1, template->tilemapTop - 1, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, r9, template->tilemapLeft, template->tilemapTop - 1, template->width, 1, 2);
    FillBgTilemapBufferRect(template->bg, r10, template->tilemapLeft + template->width, template->tilemapTop - 1, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, sp18, template->tilemapLeft - 1, template->tilemapTop, 1, template->height, 2);
    FillBgTilemapBufferRect(template->bg, spC, template->tilemapLeft + template->width, template->tilemapTop, 1, template->height, 2);
    FillBgTilemapBufferRect(template->bg, sp10, template->tilemapLeft - 1, template->tilemapTop + template->height, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, sp14, template->tilemapLeft, template->tilemapTop + template->height, template->width, 1, 2);
    FillBgTilemapBufferRect(template->bg, r6, template->tilemapLeft + template->width, template->tilemapTop + template->height, 1, 1, 2);
    CopyBgTilemapBufferToVram(template->bg);
}

static void ClearMainMenuWindowTilemap(const struct WindowTemplate *template)
{
    FillBgTilemapBufferRect(template->bg, 0, template->tilemapLeft - 1, template->tilemapTop - 1, template->tilemapLeft + template->width + 1, template->tilemapTop + template->height + 1, 2);
    CopyBgTilemapBufferToVram(template->bg);
}

static void NewGameBirchSpeech_ClearGenderWindowTilemap(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 unused)
{
    FillBgTilemapBufferRect(bg, 0, x + 255, y + 255, width + 2, height + 2, 2);
}

static void NewGameBirchSpeech_ClearGenderWindow(u8 windowId, bool8 copyToVram)
{
    CallWindowFunction(windowId, NewGameBirchSpeech_ClearGenderWindowTilemap);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void NewGameBirchSpeech_ClearWindow(u8 windowId)
{
    u8 bgColor = GetFontAttribute(FONT_NORMAL, FONTATTR_COLOR_BACKGROUND);
    u8 maxCharWidth = GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_WIDTH);
    u8 maxCharHeight = GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT);
    u8 winWidth = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u8 winHeight = GetWindowAttribute(windowId, WINDOW_HEIGHT);

    FillWindowPixelRect(windowId, bgColor, 0, 0, maxCharWidth * winWidth, maxCharHeight * winHeight);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void NewGameBirchSpeech_WaitForThisIsPokemonText(struct TextPrinterTemplate *printer, u16 renderCmd)
{
    // Wait for Birch's "This is a Pokémon" text to reach the pause
    // Then start the PokéBall release (if it hasn't been started already)
    if (*(printer->currentChar - 2) == EXT_CTRL_CODE_PAUSE && !sStartedPokeBallTask)
    {
        sStartedPokeBallTask = TRUE;
        CreateTask(Task_NewGameBirchSpeechSub_InitPokeBall, 0);
    }
}

void CreateYesNoMenuParameterized(u8 x, u8 y, u16 baseTileNum, u16 baseBlock, u8 yesNoPalNum, u8 winPalNum)
{
    struct WindowTemplate template = CreateWindowTemplate(0, x + 1, y + 1, 5, 4, winPalNum, baseBlock);
    CreateYesNoMenu(&template, baseTileNum, yesNoPalNum, 0);
}

static void NewGameBirchSpeech_ShowDialogueWindow(u8 windowId, u8 copyToVram)
{
    CallWindowFunction(windowId, NewGameBirchSpeech_CreateDialogueWindowBorder);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void NewGameBirchSpeech_CreateDialogueWindowBorder(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palNum)
{
    FillBgTilemapBufferRect(bg, 0xFD,  x-2,       y-1, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, 0xFF,  x-1,       y-1, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, 0x100, x,         y-1, width,   1, palNum);
    FillBgTilemapBufferRect(bg, 0x101, x+width-1, y-1, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, 0x102, x+width,   y-1, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, 0x103, x-2,       y,   1,       5, palNum);
    FillBgTilemapBufferRect(bg, 0x105, x-1,       y,   width+1, 5, palNum);
    FillBgTilemapBufferRect(bg, 0x106, x+width,   y,   1,       5, palNum);

    FillBgTilemapBufferRect(bg, BG_TILE_V_FLIP(0xFD),  x-2,       y+height, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, BG_TILE_V_FLIP(0xFF),  x-1,       y+height, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, BG_TILE_V_FLIP(0x100), x,         y+height, width-1, 1, palNum);
    FillBgTilemapBufferRect(bg, BG_TILE_V_FLIP(0x101), x+width-1, y+height, 1,       1, palNum);
    FillBgTilemapBufferRect(bg, BG_TILE_V_FLIP(0x102), x+width,   y+height, 1,       1, palNum);
}

static void Task_NewGameBirchSpeech_ReturnFromNamingScreenShowTextbox(u8 taskId)
{
    if (gTasks[taskId].tTimer-- <= 0)
    {
        NewGameBirchSpeech_ShowDialogueWindow(0, 1);
        gTasks[taskId].func = Task_NewGameBirchSpeech_SoItsPlayerName;
    }
}

#undef tTimer
