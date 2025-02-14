#include "global.h"
#include "battle_anim.h"
#include "berry.h"
#include "berry_powder.h"
#include "bg.h"
#include "decompress.h"
#include "dynamic_placeholder_text_util.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item_icon.h"
#include "item_menu.h"
#include "link.h"
#include "link_rfu.h"
#include "main.h"
#include "malloc.h"
#include "math_util.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "minigame_countdown.h"
#include "random.h"
#include "digit_obj_util.h"
#include "save.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "window.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define MAX_TIME (10 * 60 * 60) // Timer can go up to 9:59:59

#define TAG_CRUSHER_BASE  1
#define PALTAG_EFFECT     2 // The next two gfx tags share this pal tag
#define GFXTAG_IMPACT     2
#define GFXTAG_SPARKLE    3
#define TAG_TIMER_DIGITS  4
#define TAG_PLAYER1_BERRY 5
#define TAG_PLAYER2_BERRY 6
#define TAG_PLAYER3_BERRY 7
#define TAG_PLAYER4_BERRY 8
#define TAG_PLAYER5_BERRY 9

#define TAG_COUNTDOWN 0x1000

#define CRUSHER_START_Y (-104)

enum {
    RUN_CMD,
    SCHEDULE_CMD,
};

// IDs for the main berry crush game functions
enum {
    CMD_NONE,
    CMD_FADE,
    CMD_WAIT_FADE,
    CMD_PRINT_MSG,
    CMD_SHOW_GAME,
    CMD_HIDE_GAME,
    CMD_READY_BEGIN,
    CMD_ASK_PICK_BERRY,
    CMD_PICK_BERRY,
    CMD_WAIT_BERRIES,
    CMD_DROP_BERRIES,
    CMD_DROP_LID,
    CMD_COUNTDOWN,
    CMD_PLAY_GAME_LEADER,
    CMD_PLAY_GAME_MEMBER,
    CMD_FINISH_GAME,
    CMD_TIMES_UP,
    CMD_CALC_RESULTS,
    CMD_SHOW_RESULTS,
    CMD_SAVE,
    CMD_ASK_PLAY_AGAIN,
    CMD_COMM_PLAY_AGAIN,
    CMD_PLAY_AGAIN_YES,
    CMD_PLAY_AGAIN_NO,
    CMD_CLOSE_LINK,
    CMD_QUIT,
};

enum {
    MSG_PICK_BERRY,
    MSG_WAIT_PICK,
    MSG_POWDER,
    MSG_SAVING,
    MSG_PLAY_AGAIN,
    MSG_NO_BERRIES,
    MSG_DROPPED,
    MSG_TIMES_UP,
    MSG_COMM_STANDBY,
};

#define F_MSG_CLEAR  (1 << 0)
#define F_MSG_EXPAND (1 << 1)

// Main states for the game. Many are assigned but never checked
enum {
    STATE_INIT = 1,
    STATE_RESET,
    STATE_PICK_BERRY,
    STATE_DROP_BERRIES,
    STATE_DROP_LID,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_FINISHED,
    STATE_TIMES_UP,
    STATE_10, // Unused
    STATE_RESULTS_PRESSES,
    STATE_RESULTS_RANDOM,
    STATE_RESULTS_CRUSHING,
    STATE_14, // Unused
    STATE_PLAY_AGAIN,
};

#define RESULTS_STATE_START STATE_RESULTS_PRESSES
#define RESULTS_STATE_END   STATE_RESULTS_CRUSHING

// IDs for each results page that shows in succession at the game's end.
// Only 3 pages are shown for a given game. Presses and Crushing are always shown 1st and 3rd.
// The 2nd page is random, and can be rankings for either Neatness, Cooperative, or Power.
enum {
    RESULTS_PAGE_PRESSES,
    RESULTS_PAGE_RANDOM,
    RESULTS_PAGE_CRUSHING,
    NUM_RESULTS_PAGES,
};
// Random pages, see above
// "Neatness" is how many of the player's inputs were at a regular interval
// "Cooperative" is how often the player pressed A at the same time as others
// "Power" is how much of the time the player spent pressing A
enum {
    RESULTS_PAGE_NEATNESS,
    RESULTS_PAGE_COOPERATIVE,
    RESULTS_PAGE_POWER,
    NUM_RANDOM_RESULTS_PAGES
};

#define PLAY_AGAIN_YES        0
#define PLAY_AGAIN_NO         1
#define PLAY_AGAIN_NO_BERRIES 3

enum {
    COLORID_GRAY,
    COLORID_BLACK,
    COLORID_LIGHT_GRAY,
    COLORID_BLUE,
    COLORID_GREEN,
    COLORID_RED,
};

// Flags for the inputFlags field
// Field is 16 bits; 3 bits for each player, last bit is unused
// The first two bits are interchangeable
// Needlessly complicated system, the inputState field is sufficient by itself
#define F_INPUT_HIT_A (1 << 0)
#define F_INPUT_HIT_B (1 << 1)
#define F_INPUT_HIT_SYNC (1 << 2) // Input at same time as another player
#define INPUT_FLAGS_PER_PLAYER 3
#define INPUT_FLAG_MASK ((1 << INPUT_FLAGS_PER_PLAYER) - 1)

// Values for the inputState field
enum {
    INPUT_STATE_NONE,
    INPUT_STATE_HIT,      // Hit the crusher
    INPUT_STATE_HIT_SYNC, // Hit the crusher at same time as another player
};

// No reason for this to be 2
// Simply a flag for whether a given player has sent their data this round
// Data is only sent if the player is the leader or if they pressed A
#define SEND_GAME_STATE 2

struct BerryCrushGame_Player
{
    u8 name[PLAYER_NAME_LENGTH + 1 + 4];
    u16 berryId;
    u16 inputTime;
    u16 neatInputStreak;
    u16 timeSincePrevInput;
    u16 maxNeatInputStreak;
    u16 numAPresses;
    u16 numSyncedAPresses;
    u16 timePressingA;
    u8 inputFlags;
    u8 inputState;
};

// This data is set locally and sent to the other players
struct BerryCrushGame_LocalState
{
    u16 sendFlag;
    bool8 endGame:1;
    bool8 bigSparkle:1;
    bool8 pushedAButton:1;
    u8 playerPressedAFlags:5; // 1 bit for each player
    s8 vibration;
    u16 depth;
    u16 timer;
    u16 inputFlags;
    u16 sparkleAmount;
};

// This data is read from another player's local state above by casting the recvCmd buffer
// It is identical with exception to the addition of rfuCmd
struct BerryCrushGame_LinkState
{
    u16 rfuCmd;
    u16 sendFlag;
    bool8 endGame:1;
    bool8 bigSparkle:1;
    bool8 pushedAButton:1;
    u8 playerPressedAFlags:5;
    s8 vibration;
    u16 depth;
    u16 timer;
    u16 inputFlags;
    u16 sparkleAmount;
};

struct BerryCrushGame_Results
{
    u32 powder;
    u16 time;
    u16 targetPressesPerSec; // Never read
    u16 silkiness;
    u16 totalAPresses;
    u16 stats[2][MAX_RFU_PLAYERS];
    u8 playerIdsRanked[2][MAX_RFU_PLAYERS + 3];
};

// playerIdsRanked above has 3 additional elements after the players.
// Only 1 of these 2*3 is ever used, and it stores the id for which
// random results page to show. Its define below is for readability.
#define randomPageId playerIdsRanked[0][7]

// Holds position data for various player-associated graphics
struct BerryCrushPlayerCoords
{
    u8 playerId;
    u8 windowGfxX;
    u8 windowGfxY;
    s16 impactXOffset;
    s16 impactYOffset;
    s16 berryXOffset;
    s16 berryXDest;
};

struct BerryCrushGame_Gfx
{
    u8 counter;
    u8 vibrationIdx;
    u8 numVibrations;
    bool8 vibrating;
    s16 minutes;
    s16 secondsInt;
    s16 secondsFrac;
    const struct BerryCrushPlayerCoords *playerCoords[MAX_RFU_PLAYERS];
    struct Sprite *coreSprite;
    struct Sprite *impactSprites[MAX_RFU_PLAYERS];
    struct Sprite *berrySprites[MAX_RFU_PLAYERS];
    struct Sprite *sparkleSprites[11];
    struct Sprite *timerSprites[2];
    u8 resultsState;
    u8 unused;
    u8 resultsWindowId;
    u8 nameWindowIds[MAX_RFU_PLAYERS];
    u16 bgBuffers[4][0x800];
};

struct BerryCrushGame
{
    MainCallback exitCallback;
    u32 (*cmdCallback)(struct BerryCrushGame *, u8 *);
    u8 localId;
    u8 playerCount;
    u8 taskId;
    u8 textSpeed;
    u8 cmdState;
    u8 unused; // Never read
    u8 nextCmd;
    u8 afterPalFadeCmd;
    u16 cmdTimer;
    u16 gameState;
    u16 playAgainState;
    u16 pressingSpeed;
    s16 targetAPresses;
    s16 totalAPresses;
    s32 powder;
    s32 targetDepth;
    u8 newDepth;
    bool8 noRoomForPowder:1; // Never read
    bool8 newRecord:1;
    bool8 playedSound:1;
    bool8 endGame:1;
    bool8 bigSparkle:1;
    u8 sparkleAmount:3;
    u16 leaderTimer;
    u16 timer;
    s16 depth;
    s16 vibration;
    s16 bigSparkleCounter;
    s16 numBigSparkles;
    s16 numBigSparkleChecks;
    s16 sparkleCounter;
    u8 commandArgs[12];
    u16 sendCmd[6];
    u16 recvCmd[7];
    struct BerryCrushGame_LocalState localState;
    struct BerryCrushGame_Results results;
    struct BerryCrushGame_Player players[MAX_RFU_PLAYERS];
    struct BerryCrushGame_Gfx gfx;
};

static void VBlankCB(void);
static void MainCB(void);
static void MainTask(u8);
static void SetNamesAndTextSpeed(struct BerryCrushGame *);
static void RunOrScheduleCommand(u16, u8, u8 *);
static void SetPaletteFadeArgs(u8 *, bool8, u32, s8, u8, u8, u16);
static s32 UpdateGame(struct BerryCrushGame *);
static void CreatePlayerNameWindows(struct BerryCrushGame *);
static void DrawPlayerNameWindows(struct BerryCrushGame *);
static void CopyPlayerNameWindowGfxToBg(struct BerryCrushGame *);
static void CreateGameSprites(struct BerryCrushGame *);
static void DestroyGameSprites(struct BerryCrushGame *);
static void PrintTimer(struct BerryCrushGame_Gfx *, u16);
static void SpriteCB_Sparkle_Init(struct Sprite *);
static void HideTimer(struct BerryCrushGame_Gfx *);
static void ResetGame(struct BerryCrushGame *);
static void SetPrintMessageArgs(u8 *, u8, u8, u16, u8);
static void SpriteCB_Impact(struct Sprite *);
static u32 Cmd_BeginNormalPaletteFade(struct BerryCrushGame *, u8 *);
static u32 Cmd_WaitPaletteFade(struct BerryCrushGame *, u8 *);
static u32 Cmd_PrintMessage(struct BerryCrushGame *, u8 *);
static u32 Cmd_ShowGameDisplay(struct BerryCrushGame *, u8 *);
static u32 Cmd_HideGameDisplay(struct BerryCrushGame *, u8 *);
static u32 Cmd_SignalReadyToBegin(struct BerryCrushGame *, u8 *);
static u32 Cmd_AskPickBerry(struct BerryCrushGame *, u8 *);
static u32 Cmd_GoToBerryPouch(struct BerryCrushGame *, u8 *);
static u32 Cmd_WaitForOthersToPickBerries(struct BerryCrushGame *, u8 *);
static u32 Cmd_DropBerriesIntoCrusher(struct BerryCrushGame *, u8 *);
static u32 Cmd_DropLid(struct BerryCrushGame *, u8 *);
static u32 Cmd_Countdown(struct BerryCrushGame *, u8 *);
static u32 Cmd_PlayGame_Leader(struct BerryCrushGame *, u8 *);
static u32 Cmd_PlayGame_Member(struct BerryCrushGame *, u8 *);
static u32 Cmd_FinishGame(struct BerryCrushGame *, u8 *);
static u32 Cmd_HandleTimeUp(struct BerryCrushGame *, u8 *);
static u32 Cmd_TabulateResults(struct BerryCrushGame *, u8 *);
static u32 Cmd_ShowResults(struct BerryCrushGame *, u8 *);
static u32 Cmd_SaveGame(struct BerryCrushGame *, u8 *);
static u32 Cmd_AskPlayAgain(struct BerryCrushGame *, u8 *);
static u32 Cmd_CommunicatePlayAgainResponses(struct BerryCrushGame *, u8 *);
static u32 Cmd_PlayAgain(struct BerryCrushGame *, u8 *);
static u32 Cmd_StopGame(struct BerryCrushGame *, u8 *);
static u32 Cmd_CloseLink(struct BerryCrushGame *, u8 *);
static u32 Cmd_Quit(struct BerryCrushGame *, u8 *);

static EWRAM_DATA struct BerryCrushGame *sGame = NULL;

static const u8 sBitTable[] = {
    1 << 0,
    1 << 1,
    1 << 2,
    1 << 3,
    1 << 4,
    1 << 5,
    1 << 6,
    1 << 7
};
// Additional A presses are counted depending on the number of players
// The bonus of 5 is unobtainable
static const u8 sSyncPressBonus[MAX_RFU_PLAYERS] = { 0, 1, 2, 3, 5 };
ALIGNED(4)
static const s8 sIntroOutroVibrationData[][7] =
{
    { 4, 1, 0, -1,  0,  0, 0},
    { 4, 2, 0, -1,  0,  0, 0},
    { 4, 2, 0, -2,  0,  0, 0},
    { 6, 3, 1, -1, -3, -1, 0},
    { 6, 4, 1, -2, -4, -2, 0},
};

ALIGNED(4)
static const u8 sVibrationData[MAX_RFU_PLAYERS][4] =
{
    {3, 2, 1, 0},
    {3, 3, 1, 0},
    {3, 3, 2, 0},
    {3, 4, 2, 0},
    {3, 5, 3, 0},
};

static const u8 *const sMessages[] =
{
    [MSG_PICK_BERRY]   = gText_ReadyPickBerry,
    [MSG_WAIT_PICK]    = gText_WaitForAllChooseBerry,
    [MSG_POWDER]       = gText_EndedWithXUnitsPowder,
    [MSG_SAVING]       = gText_RecordingGameResults,
    [MSG_PLAY_AGAIN]   = gText_PlayBerryCrushAgain,
    [MSG_NO_BERRIES]   = gText_YouHaveNoBerries,
    [MSG_DROPPED]      = gText_MemberDroppedOut,
    [MSG_TIMES_UP]     = gText_TimesUpNoGoodPowder,
    [MSG_COMM_STANDBY] = gText_CommunicationStandby2,
};

static const struct BgTemplate sBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 15,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 13,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 12,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 11,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

static const u8 sTextColorTable[][3] =
{
    [COLORID_GRAY]       = {TEXT_COLOR_WHITE,       TEXT_COLOR_DARK_GRAY,  TEXT_COLOR_LIGHT_GRAY},
    [COLORID_BLACK]      = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,      TEXT_COLOR_DARK_GRAY},
    [COLORID_LIGHT_GRAY] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_RED},
    [COLORID_BLUE]       = {TEXT_COLOR_WHITE,       TEXT_COLOR_BLUE,       TEXT_COLOR_LIGHT_BLUE},
    [COLORID_GREEN]      = {TEXT_COLOR_WHITE,       TEXT_COLOR_GREEN,      TEXT_COLOR_LIGHT_GREEN},
    [COLORID_RED]        = {TEXT_COLOR_WHITE,       TEXT_COLOR_RED,        TEXT_COLOR_LIGHT_RED},
};


static const struct WindowTemplate sWindowTemplate_Rankings =
{
    .bg = 0,
    .tilemapLeft = 3,
    .tilemapTop = 4,
    .width = 24,
    .height = 13,
    .paletteNum = 15,
    .baseBlock = 1
};

static const struct WindowTemplate sWindowTemplates_PlayerNames[MAX_RFU_PLAYERS + 1] =
{
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 9,
        .height = 2,
        .paletteNum = 8,
        .baseBlock = 1005
    },
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 3,
        .width = 9,
        .height = 2,
        .paletteNum = 8,
        .baseBlock = 987
    },
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 6,
        .width = 9,
        .height = 2,
        .paletteNum = 8,
        .baseBlock = 969
    },
    {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 3,
        .width = 9,
        .height = 2,
        .paletteNum = 8,
        .baseBlock = 951
    },
    {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 6,
        .width = 9,
        .height = 2,
        .paletteNum = 8,
        .baseBlock = 933
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sWindowTemplates_Results[] =
{
    [STATE_RESULTS_PRESSES - RESULTS_STATE_START] = {
        .bg = 0,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 20,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [STATE_RESULTS_RANDOM - RESULTS_STATE_START] = {
        .bg = 0,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 20,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [STATE_RESULTS_CRUSHING - RESULTS_STATE_START] = {
        .bg = 0,
        .tilemapLeft = 4,
        .tilemapTop = 2,
        .width = 22,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 1
    },
    DUMMY_WIN_TEMPLATE,
};

// The height of the results window depending on the number of players
// 2 players, 3 players, 4 players, or 5 players
static const u8 sResultsWindowHeights[][MAX_RFU_PLAYERS - 1] =
{
    {6, 8, 9, 11},    // "Presses" and "Neatness/Cooperative/Power" pages
    {12, 14, 15, 16}, // "Crushing" page
};

static const u32 sPressingSpeedConversionTable[] =
{
    50000000, // 50
    25000000, // 25
    12500000, // 12.5
     6250000, //  6.25
     3125000, //  3.125
     1562500, //  1.5625
      781250, //  0.78125
      390625  //  0.390625
};

static const u16 sCrusherBase_Pal[]     = INCBIN_U16("graphics/berry_crush/crusher_base.gbapal");
static const u16 sEffects_Pal[]         = INCBIN_U16("graphics/berry_crush/effects.gbapal");
static const u16 sTimerDigits_Pal[]     = INCBIN_U16("graphics/berry_crush/timer_digits.gbapal");
static const u32 sCrusherBase_Gfx[]     = INCBIN_U32("graphics/berry_crush/crusher_base.4bpp.lz");
static const u32 sImpact_Gfx[]          = INCBIN_U32("graphics/berry_crush/impact.4bpp.lz");
static const u32 sSparkle_Gfx[]         = INCBIN_U32("graphics/berry_crush/sparkle.4bpp.lz");
static const u32 sTimerDigits_Gfx[]     = INCBIN_U32("graphics/berry_crush/timer_digits.4bpp.lz");
static const u8 sCrusherTop_Tilemap[]   = INCBIN_U8("graphics/berry_crush/crusher_top.bin.lz");
static const u8 sContainerCap_Tilemap[] = INCBIN_U8("graphics/berry_crush/container_cap.bin.lz");
static const u8 sBg_Tilemap[]           = INCBIN_U8("graphics/berry_crush/bg.bin.lz");

// Takes the number of players - 2 and a player id and returns the
// index into sPlayerCoords where that player should be seated
static const u8 sPlayerIdToPosId[MAX_RFU_PLAYERS - 1][MAX_RFU_PLAYERS] =
{
    {1, 3},
    {0, 1, 3},
    {1, 3, 2, 4},
    {0, 1, 3, 2, 4},
};

static const struct BerryCrushPlayerCoords sPlayerCoords[MAX_RFU_PLAYERS] =
{
    {
        .playerId = 0,
        .windowGfxX = 0,
        .windowGfxY = 0,
        .impactXOffset = 0,
        .impactYOffset = -16,
        .berryXOffset = 0,
        .berryXDest = 0,
    },
    {
        .playerId = 1,
        .windowGfxX = 0,
        .windowGfxY = 3,
        .impactXOffset = -28,
        .impactYOffset = -4,
        .berryXOffset = -24,
        .berryXDest = 16,
    },
    {
        .playerId = 2,
        .windowGfxX = 0,
        .windowGfxY = 6,
        .impactXOffset = -16,
        .impactYOffset = 20,
        .berryXOffset = -8,
        .berryXDest = 16,
    },
    {
        .playerId = 3,
        .windowGfxX = 20,
        .windowGfxY = 3,
        .impactXOffset = 28,
        .impactYOffset = -4,
        .berryXOffset = 32,
        .berryXDest = -8,
    },
    {
        .playerId = 4,
        .windowGfxX = 20,
        .windowGfxY = 6,
        .impactXOffset = 16,
        .impactYOffset = 20,
        .berryXOffset = 16,
        .berryXDest = -8,
    }
};


static const s8 sImpactCoords[][2] =
{
    { 0, 0},
    {-1, 0},
    { 1, 1},
};

static const s8 sSparkleCoords[][2] =
{
    {  0,   0},
    {-16,  -4},
    { 16,  -4},
    { -8,  -2},
    {  8,  -2},
    {-24,  -8},
    { 24,  -8},
    {-32, -12},
    { 32, -12},
    {-40, -16},
    { 40, -16},
};

static const u16 sPlayerBerrySpriteTags[MAX_RFU_PLAYERS] =
{
    TAG_PLAYER1_BERRY,
    TAG_PLAYER2_BERRY,
    TAG_PLAYER3_BERRY,
    TAG_PLAYER4_BERRY,
    TAG_PLAYER5_BERRY
};

// sTimerDigits_Gfx is part of this array but is (apparently) uncompressed
// It gets cast to raw uncompressed data when used in sDigitObjTemplates
static const struct CompressedSpriteSheet sSpriteSheets[] =
{
    { .data = sCrusherBase_Gfx, .size = 0x800, .tag = TAG_CRUSHER_BASE },
    { .data = sImpact_Gfx,      .size = 0xE00, .tag = GFXTAG_IMPACT },
    { .data = sSparkle_Gfx,     .size = 0x700, .tag = GFXTAG_SPARKLE },
    { .data = sTimerDigits_Gfx, .size = 0x2C0, .tag = TAG_TIMER_DIGITS },
    {}
};


static const struct SpritePalette sSpritePals[] =
{
    { .data = sCrusherBase_Pal, .tag = TAG_CRUSHER_BASE },
    { .data = sEffects_Pal,     .tag = PALTAG_EFFECT }, // For the impact and sparkle effects
    { .data = sTimerDigits_Pal, .tag = TAG_TIMER_DIGITS },
    {}
};

static const union AnimCmd sAnim_CrusherBase[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Impact_Small[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Impact_Big[] =
{
    ANIMCMD_FRAME(48, 2),
    ANIMCMD_FRAME(64, 2),
    ANIMCMD_FRAME(80, 2),
    ANIMCMD_FRAME(96, 2),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Sparkle_Small[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(4, 2),
    ANIMCMD_FRAME(8, 2),
    ANIMCMD_FRAME(12, 2),
    ANIMCMD_FRAME(16, 2),
    ANIMCMD_FRAME(20, 2),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Sparkle_Big[] =
{
    ANIMCMD_FRAME(24, 4),
    ANIMCMD_FRAME(28, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(36, 4),
    ANIMCMD_FRAME(40, 4),
    ANIMCMD_FRAME(44, 4),
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_FRAME(52, 4),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Timer[] =
{
    ANIMCMD_FRAME(20, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerBerry[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};


static const union AffineAnimCmd sAffineAnim_PlayerBerry_0[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(0, 0, 2, 1),
    AFFINEANIMCMD_JUMP(1)
};

static const union AffineAnimCmd sAffineAnim_PlayerBerry_1[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(0, 0, -2, 1),
    AFFINEANIMCMD_JUMP(1)
};

static const union AnimCmd *const sAnims_CrusherBase[] =
{
    sAnim_CrusherBase
};

static const union AnimCmd *const sAnims_Impact[] =
{
    sAnim_Impact_Small,
    sAnim_Impact_Big,
};

static const union AnimCmd *const sAnims_Sparkle[] =
{
    sAnim_Sparkle_Small,
    sAnim_Sparkle_Big,
};

static const union AnimCmd *const sAnims_Timer[] =
{
    sAnim_Timer
};

static const union AnimCmd *const sAnims_PlayerBerry[] =
{
    sAnim_PlayerBerry
};

static const union AffineAnimCmd *const sAffineAnims_PlayerBerry[] =
{
    sAffineAnim_PlayerBerry_0,
    sAffineAnim_PlayerBerry_1,
};

static const struct SpriteTemplate sSpriteTemplate_CrusherBase =
{
    .tileTag = TAG_CRUSHER_BASE,
    .paletteTag = TAG_CRUSHER_BASE,
    .oam = &gOamData_AffineOff_ObjNormal_64x64,
    .anims = sAnims_CrusherBase,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_Impact =
{
    .tileTag = GFXTAG_IMPACT,
    .paletteTag = PALTAG_EFFECT,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_Impact,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Impact
};

static const struct SpriteTemplate sSpriteTemplate_Sparkle =
{
    .tileTag = GFXTAG_SPARKLE,
    .paletteTag = PALTAG_EFFECT,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = sAnims_Sparkle,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_Timer =
{
    .tileTag = TAG_TIMER_DIGITS,
    .paletteTag = TAG_TIMER_DIGITS,
    .oam = &gOamData_AffineOff_ObjNormal_8x16,
    .anims = sAnims_Timer,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_PlayerBerry =
{
    .tileTag = TAG_PLAYER1_BERRY,
    .paletteTag = TAG_PLAYER1_BERRY,
    .oam = &gOamData_AffineDouble_ObjNormal_32x32,
    .anims = sAnims_PlayerBerry,
    .images = NULL,
    .affineAnims = sAffineAnims_PlayerBerry,
    .callback = SpriteCallbackDummy
};

static const struct DigitObjUtilTemplate sDigitObjTemplates[] =
{
    { // Minutes
        .strConvMode = 1,
        .shape = 2,
        .size = 0,
        .priority = 0,
        .oamCount = 2,
        .xDelta = 8,
        .x = 156,
        .y = 0,
        .spriteSheet = (void *) &sSpriteSheets[3],
        .spritePal = &sSpritePals[2],
    },
    { // Seconds
        .strConvMode = 0,
        .shape = 2,
        .size = 0,
        .priority = 0,
        .oamCount = 2,
        .xDelta = 8,
        .x = 180,
        .y = 0,
        .spriteSheet = (void *) &sSpriteSheets[3],
        .spritePal = &sSpritePals[2],
    },
    { // 1/60ths of a second
        .strConvMode = 0,
        .shape = 2,
        .size = 0,
        .priority = 0,
        .oamCount = 2,
        .xDelta = 8,
        .x = 204,
        .y = 0,
        .spriteSheet = (void *) &sSpriteSheets[3],
        .spritePal = &sSpritePals[2],
    }
};

static const u8 *const sResultsTexts[] =
{
    [RESULTS_PAGE_PRESSES]  = gText_SpaceTimes2, // " times"
    [RESULTS_PAGE_RANDOM]   = gText_XDotY,       // "##.##", for Neatness, Cooperation, or Power value
    [RESULTS_PAGE_CRUSHING] = gText_Var1Berry,

    [RESULTS_PAGE_NEATNESS + NUM_RESULTS_PAGES]    = gText_NeatnessRankings,
    [RESULTS_PAGE_COOPERATIVE + NUM_RESULTS_PAGES] = gText_CoopRankings,
    [RESULTS_PAGE_POWER + NUM_RESULTS_PAGES]       = gText_PressingPowerRankings,
};

static u32 (*const sBerryCrushCommands[])(struct BerryCrushGame * game, u8 * data) =
{
    [CMD_NONE]             = NULL,
    [CMD_FADE]             = Cmd_BeginNormalPaletteFade,
    [CMD_WAIT_FADE]        = Cmd_WaitPaletteFade,
    [CMD_PRINT_MSG]        = Cmd_PrintMessage,
    [CMD_SHOW_GAME]        = Cmd_ShowGameDisplay,
    [CMD_HIDE_GAME]        = Cmd_HideGameDisplay,
    [CMD_READY_BEGIN]      = Cmd_SignalReadyToBegin,
    [CMD_ASK_PICK_BERRY]   = Cmd_AskPickBerry,
    [CMD_PICK_BERRY]       = Cmd_GoToBerryPouch,
    [CMD_WAIT_BERRIES]     = Cmd_WaitForOthersToPickBerries,
    [CMD_DROP_BERRIES]     = Cmd_DropBerriesIntoCrusher,
    [CMD_DROP_LID]         = Cmd_DropLid,
    [CMD_COUNTDOWN]        = Cmd_Countdown,
    [CMD_PLAY_GAME_LEADER] = Cmd_PlayGame_Leader,
    [CMD_PLAY_GAME_MEMBER] = Cmd_PlayGame_Member,
    [CMD_FINISH_GAME]      = Cmd_FinishGame,
    [CMD_TIMES_UP]         = Cmd_HandleTimeUp,
    [CMD_CALC_RESULTS]     = Cmd_TabulateResults,
    [CMD_SHOW_RESULTS]     = Cmd_ShowResults,
    [CMD_SAVE]             = Cmd_SaveGame,
    [CMD_ASK_PLAY_AGAIN]   = Cmd_AskPlayAgain,
    [CMD_COMM_PLAY_AGAIN]  = Cmd_CommunicatePlayAgainResponses,
    [CMD_PLAY_AGAIN_YES]   = Cmd_PlayAgain,
    [CMD_PLAY_AGAIN_NO]    = Cmd_StopGame,
    [CMD_CLOSE_LINK]       = Cmd_CloseLink,
    [CMD_QUIT]             = Cmd_Quit,
};

// Per group size, the number of A presses required to increase the number of sparkles.
static const u8 sSparkleThresholds[MAX_RFU_PLAYERS - 1][4] =
{
    {2,  4,  6,  7}, // 2 players
    {3,  5,  8, 11}, // 3 players
    {3,  7, 11, 15}, // 4 players
    {4,  8, 12, 17}, // 5 players
};

// Per group size, the number of A presses required to get big sparkles
static const u8 sBigSparkleThresholds[MAX_RFU_PLAYERS - 1] = {5, 7, 9, 12};

static const u8 sReceivedPlayerBitmasks[] = {0x03, 0x07, 0x0F, 0x1F};

static struct BerryCrushGame * GetBerryCrushGame(void)
{
    return sGame;
}

static u32 QuitBerryCrush(MainCallback exitCallback)
{
    if (!sGame)
        return 2;

    if (!exitCallback)
        exitCallback = sGame->exitCallback;

    DestroyTask(sGame->taskId);
    FREE_AND_SET_NULL(sGame);
    SetMainCallback2(exitCallback);
    if (exitCallback == CB2_ReturnToField)
    {
        gTextFlags.autoScroll = TRUE;
        PlayNewMapMusic(MUS_POKE_CENTER);
        SetMainCallback1(CB1_Overworld);
    }

    return 0;
}

#define ERROR_EXIT(exitCallback)                    \
    {                                               \
        SetMainCallback2(exitCallback);             \
        gRfu.errorParam0 = 0;                       \
        gRfu.errorParam1 = 0;                       \
        gRfu.errorState = RFU_ERROR_STATE_OCCURRED; \
    }

void StartBerryCrush(MainCallback exitCallback)
{
    u8 playerCount = 0;
    u8 multiplayerId;

    if (!gReceivedRemoteLinkPlayers || gWirelessCommType == 0)
    {
        // Link disconnected
        ERROR_EXIT(exitCallback);
        return;
    }

    playerCount = GetLinkPlayerCount();
    multiplayerId = GetMultiplayerId();
    if (playerCount < 2 || multiplayerId >= playerCount)
    {
        // Too few players, or invalid id
        ERROR_EXIT(exitCallback);
        return;
    }

    sGame = AllocZeroed(sizeof(*sGame));
    if (!sGame)
    {
        // Alloc failed
        ERROR_EXIT(exitCallback);
        return;
    }

    sGame->exitCallback = exitCallback;
    sGame->localId = multiplayerId;
    sGame->playerCount = playerCount;
    SetNamesAndTextSpeed(sGame);
    sGame->gameState = STATE_INIT;
    sGame->nextCmd = CMD_FADE;
    sGame->afterPalFadeCmd = CMD_READY_BEGIN;
    SetPaletteFadeArgs(sGame->commandArgs, TRUE, PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    RunOrScheduleCommand(CMD_SHOW_GAME, 1, sGame->commandArgs);
    SetMainCallback2(MainCB);
    sGame->taskId = CreateTask(MainTask, 8);
    gTextFlags.autoScroll = FALSE;
}

static void GetBerryFromBag(void)
{
    if (gSpecialVar_ItemId < FIRST_BERRY_INDEX || gSpecialVar_ItemId > LAST_BERRY_INDEX + 1)
        gSpecialVar_ItemId = FIRST_BERRY_INDEX;
    else
        RemoveBagItem(gSpecialVar_ItemId, 1);

    sGame->players[sGame->localId].berryId = gSpecialVar_ItemId - FIRST_BERRY_INDEX;
    sGame->nextCmd = CMD_FADE;
    sGame->afterPalFadeCmd = CMD_WAIT_BERRIES;
    SetPaletteFadeArgs(sGame->commandArgs, FALSE, PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    RunOrScheduleCommand(CMD_SHOW_GAME, 1, sGame->commandArgs);
    sGame->taskId = CreateTask(MainTask, 8);
    SetMainCallback2(MainCB);
}

static void ChooseBerry(void)
{
    DestroyTask(sGame->taskId);
    ChooseBerryForMachine(GetBerryFromBag);
}

static void BerryCrush_SetVBlankCB(void)
{
    SetVBlankCallback(VBlankCB);
}

static void UNUSED BerryCrush_InitVBlankCB(void)
{
    SetVBlankCallback(NULL);
}

static void SaveResults(void)
{
    u32 time, presses;

    // Calculate pressing speed ((time / 60) / presses)
    time = sGame->results.time;
    time = Q_24_8(time);
    time = MathUtil_Div32(time, Q_24_8(60));
    presses = sGame->results.totalAPresses;
    presses = Q_24_8(presses);
    presses = MathUtil_Div32(presses, time) & 0xFFFF;
    sGame->pressingSpeed = presses;

    switch (sGame->playerCount)
    {
    case 2:
        if (sGame->pressingSpeed > gSaveBlock2Ptr->berryCrush.pressingSpeeds[0])
        {
            // New 2-player record
            sGame->newRecord = TRUE;
            gSaveBlock2Ptr->berryCrush.pressingSpeeds[0] = sGame->pressingSpeed;
        }
        break;
    case 3:
        if (sGame->pressingSpeed > gSaveBlock2Ptr->berryCrush.pressingSpeeds[1])
        {
            // New 3-player record
            sGame->newRecord = TRUE;
            gSaveBlock2Ptr->berryCrush.pressingSpeeds[1] = sGame->pressingSpeed;
        }
        break;
    case 4:
        if (sGame->pressingSpeed > gSaveBlock2Ptr->berryCrush.pressingSpeeds[2])
        {
            // New 4-player record
            sGame->newRecord = TRUE;
            gSaveBlock2Ptr->berryCrush.pressingSpeeds[2] = sGame->pressingSpeed;
        }
        break;
    case 5:
        if (sGame->pressingSpeed > gSaveBlock2Ptr->berryCrush.pressingSpeeds[3])
        {
            // New 5-player record
            sGame->newRecord = TRUE;
            gSaveBlock2Ptr->berryCrush.pressingSpeeds[3] = sGame->pressingSpeed;
        }
        break;
    }

    sGame->powder = sGame->results.powder;
    if (GiveBerryPowder(sGame->powder))
        return;

    sGame->noRoomForPowder = TRUE;
}

static void VBlankCB(void)
{
    TransferPlttBuffer();
    LoadOam();
    ProcessSpriteCopyRequests();
}

static void MainCB(void)
{
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
}

static void MainTask(u8 taskId)
{
    if (sGame->cmdCallback)
        sGame->cmdCallback(sGame, sGame->commandArgs);

    UpdateGame(sGame);
}

static void SetNamesAndTextSpeed(struct BerryCrushGame *game)
{
    u8 i;
    for (i = 0; i < game->playerCount; i++)
        StringCopy(game->players[i].name, gLinkPlayers[i].name);
    for (; i < MAX_RFU_PLAYERS; i++)
    {
        memset(game->players[i].name, 1, PLAYER_NAME_LENGTH);
        game->players[i].name[PLAYER_NAME_LENGTH] = EOS;
    }

    switch (gSaveBlock2Ptr->optionsTextSpeed)
    {
    case OPTIONS_TEXT_SPEED_SLOW:
        game->textSpeed = 8;
        break;
    case OPTIONS_TEXT_SPEED_MID:
        game->textSpeed = 4;
        break;
    case OPTIONS_TEXT_SPEED_FAST:
        game->textSpeed = 1;
        break;
    }
}

static s32 ShowGameDisplay(void)
{
    struct BerryCrushGame *game = GetBerryCrushGame();
    if (!game)
        return -1;

    switch (game->cmdState)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetHBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ScanlineEffect_Stop();
        ResetTempTileDataBuffers();
        break;
    case 1:
        CpuFill16(0, (void *)OAM, OAM_SIZE);
        gReservedSpritePaletteCount = 0;
        DigitObjUtil_Init(3);
        break;
    case 2:
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        break;
    case 3:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(1, game->gfx.bgBuffers[0]);
        SetBgTilemapBuffer(2, game->gfx.bgBuffers[2]);
        SetBgTilemapBuffer(3, game->gfx.bgBuffers[3]);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        break;
    case 4:
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 64);
        FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 32, 32);
        FillBgTilemapBufferRect_Palette0(3, 0, 0, 0, 32, 32);
        break;
    case 5:
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        DecompressAndCopyTileDataToVram(1, gBerryCrush_Crusher_Gfx, 0, 0, 0);
        break;
    case 6:
        if (FreeTempTileDataBuffersIfPossible())
            return 0;

        InitStandardTextBoxWindows();
        InitTextBoxGfxAndPrinters();
        CreatePlayerNameWindows(game);
        DrawPlayerNameWindows(game);
        gPaletteFade.bufferTransferDisabled = TRUE;
        break;
    case 7:
        LoadPalette(gBerryCrush_Crusher_Pal, BG_PLTT_ID(0), 12 * PLTT_SIZE_4BPP);
        CopyToBgTilemapBuffer(1, sCrusherTop_Tilemap, 0, 0);
        CopyToBgTilemapBuffer(2, sContainerCap_Tilemap, 0, 0);
        CopyToBgTilemapBuffer(3, sBg_Tilemap, 0, 0);
        CopyPlayerNameWindowGfxToBg(game);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        break;
    case 8:
        LoadWirelessStatusIndicatorSpriteGfx();
        CreateWirelessStatusIndicatorSprite(0,  0);
        CreateGameSprites(game);
        SetGpuReg(REG_OFFSET_BG1VOFS, -gSpriteCoordOffsetY);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        break;
    case 9:
        gPaletteFade.bufferTransferDisabled = FALSE;
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        BerryCrush_SetVBlankCB();
        game->cmdState = 0;
        return 1;
    }

    game->cmdState++;
    return 0;
}

static s32 HideGameDisplay(void)
{
    struct BerryCrushGame *game = GetBerryCrushGame();
    if (!game)
        return -1;

    switch (game->cmdState)
    {
    case 0:
        Rfu_SetLinkStandbyCallback();
        break;
    case 1:
        if (!IsLinkTaskFinished())
            return 0;
        // fall through
        // This will call BeginNormalPaletteFade() twice.
#ifdef BUGFIX
        break;
#endif
    case 2:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        UpdatePaletteFade();
        break;
    case 3:
        if (UpdatePaletteFade())
            return 0;
        break;
    case 4:
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);
        FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 32, 32);
        FillBgTilemapBufferRect_Palette0(3, 0, 0, 0, 32, 32);
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        break;
    case 5:
        FreeAllWindowBuffers();
        HideBg(0);
        UnsetBgTilemapBuffer(0);
        HideBg(1);
        UnsetBgTilemapBuffer(1);
        HideBg(2);
        UnsetBgTilemapBuffer(2);
        HideBg(3);
        UnsetBgTilemapBuffer(3);
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        break;
    case 6:
        DestroyWirelessStatusIndicatorSprite();
        DestroyGameSprites(game);
        DigitObjUtil_Free();
        break;
    case 7:
        game->cmdState = 0;
        return 1;
    }

    game->cmdState++;
    return 0;
}

// Handles the crusher vibration and the timer
static s32 UpdateGame(struct BerryCrushGame *game)
{
    gSpriteCoordOffsetY = game->depth + game->vibration;
    SetGpuReg(REG_OFFSET_BG1VOFS, -gSpriteCoordOffsetY);

    if (game->gameState == STATE_PLAYING)
        PrintTimer(&game->gfx, game->timer);

    return 0;
}

static void ResetCrusherPos(struct BerryCrushGame *game)
{
    game->depth = CRUSHER_START_Y;
    game->vibration = 0;
    gSpriteCoordOffsetX = 0;
    gSpriteCoordOffsetY = CRUSHER_START_Y;
}

// Sprite data for berry sprites. Identical to fields for sparkle sprites
#define sX         data[0]
#define sYSpeed    data[1]
#define sYAccel    data[2]
#define sXSpeed    data[3]
#define sSinIdx    data[4]
#define sSinSpeed  data[5]
#define sAmplitude data[6]
// The last element (data[7]) is a bitfield.
// The first 15 bits are the y coord to stop at.
// The last bit is a flag for whether or not to move horizontally too
#define sBitfield  data[7]
#define MASK_TARGET_Y 0x7FFF
#define F_MOVE_HORIZ  0x8000

static void CreateBerrySprites(struct BerryCrushGame *game, struct BerryCrushGame_Gfx *gfx)
{
    u8 i;
    u8 spriteId;
    s16 distance, var1;
    s16 *data;
    s16 speed;
    u32 var2;

    for (i = 0; i < game->playerCount; i++)
    {
        spriteId = AddCustomItemIconSprite(
            &sSpriteTemplate_PlayerBerry,
            sPlayerBerrySpriteTags[i],
            sPlayerBerrySpriteTags[i],
            game->players[i].berryId + FIRST_BERRY_INDEX);
        gfx->berrySprites[i] = &gSprites[spriteId];
        gfx->berrySprites[i]->oam.priority = 3;
        gfx->berrySprites[i]->affineAnimPaused = TRUE;
        gfx->berrySprites[i]->x = gfx->playerCoords[i]->berryXOffset + 120;
        gfx->berrySprites[i]->y = -16;
        data = gfx->berrySprites[i]->data;
        speed = 512;
        sYSpeed = speed;
        sYAccel = 32;
        sBitfield = 112; // Setting bits in MASK_TARGET_Y
        distance = gfx->playerCoords[i]->berryXDest - gfx->playerCoords[i]->berryXOffset;
        sAmplitude = distance / 4;
        distance *= 128;
        var2 = speed + 32;
        var2 = var2 / 2;
        var1 = MathUtil_Div16Shift(7, Q_8_8(63.5), var2);
        sX = (u16)gfx->berrySprites[i]->x * 128;
        sXSpeed = MathUtil_Div16Shift(7, distance, var1);
        var1 = MathUtil_Mul16Shift(7, var1, 85);
        sSinIdx = 0;
        sSinSpeed = MathUtil_Div16Shift(7, Q_8_8(63.5), var1);
        sBitfield |= F_MOVE_HORIZ;
        if (gfx->playerCoords[i]->berryXOffset < 0)
            StartSpriteAffineAnim(gfx->berrySprites[i], 1);
    }
}

static void SpriteCB_DropBerryIntoCrusher(struct Sprite *sprite)
{
    s16 *data = sprite->data;

    sYSpeed += sYAccel;
    sprite->y2 += sYSpeed >> 8;
    if (sBitfield & F_MOVE_HORIZ)
    {
        sprite->sX += sXSpeed;
        sSinIdx += sSinSpeed;
        sprite->x2 = Sin(sSinIdx >> 7, sAmplitude);
        if ((sBitfield & F_MOVE_HORIZ) && (sSinIdx >> 7) > 126)
        {
            sprite->x2 = 0;
            sBitfield &= MASK_TARGET_Y;
        }
    }

    sprite->x = sX >> 7;
    if (sprite->y + sprite->y2 >= (sBitfield & MASK_TARGET_Y))
    {
        sprite->callback = SpriteCallbackDummy;
        FreeSpriteOamMatrix(sprite);
        DestroySprite(sprite);
    }
}

#undef sX
#undef sYSpeed
#undef sYAccel
#undef sXSpeed
#undef sSinIdx
#undef sSinSpeed
#undef sAmplitude
#undef sBitfield
#undef MASK_TARGET_Y
#undef F_MOVE_HORIZ

static void BerryCrushFreeBerrySpriteGfx(struct BerryCrushGame *game, struct BerryCrushGame_Gfx *gfx)
{
    u8 i;
    for (i = 0; i < game->playerCount; i++)
    {
        FreeSpritePaletteByTag(sPlayerBerrySpriteTags[i]);
        FreeSpriteTilesByTag(sPlayerBerrySpriteTags[i]);
    }
}

static void UpdateInputEffects(struct BerryCrushGame *game, struct BerryCrushGame_Gfx *gfx)
{
    u8 numPlayersPressed;
    struct BerryCrushGame_LinkState *linkState;
    u8 i;
    u16 temp1, xModifier;

    numPlayersPressed = 0;
    linkState = (struct BerryCrushGame_LinkState *)game->recvCmd;

    // Read inputs and update impact effects
    for (i = 0; i < game->playerCount; i++)
    {
        #define flags temp1

        flags = linkState->inputFlags >> (i * INPUT_FLAGS_PER_PLAYER);
        flags &= INPUT_FLAG_MASK;
        if (flags)
        {
            numPlayersPressed++;
            if (flags & F_INPUT_HIT_SYNC)
                StartSpriteAnim(gfx->impactSprites[i], 1); // Big impact sprite
            else
                StartSpriteAnim(gfx->impactSprites[i], 0); // Small impact sprite

            gfx->impactSprites[i]->invisible = FALSE;
            gfx->impactSprites[i]->animPaused = FALSE;
            gfx->impactSprites[i]->x2 = sImpactCoords[(flags % (ARRAY_COUNT(sImpactCoords) + 1)) - 1][0];
            gfx->impactSprites[i]->y2 = sImpactCoords[(flags % (ARRAY_COUNT(sImpactCoords) + 1)) - 1][1];
        }

        #undef flags
    }

    if (numPlayersPressed == 0)
    {
        game->playedSound = FALSE;
    }
    else
    {
        // Update sparkle effect
        #define yModifier temp1

        yModifier = (u8)(game->timer % 3);
        xModifier = yModifier;
        for (i = 0; i < linkState->sparkleAmount * 2 + 3; i++)
        {
            if (gfx->sparkleSprites[i]->invisible)
            {
                gfx->sparkleSprites[i]->callback = SpriteCB_Sparkle_Init;
                gfx->sparkleSprites[i]->x = sSparkleCoords[i][0] + 120;
                gfx->sparkleSprites[i]->y = sSparkleCoords[i][1] + 136 - (yModifier * 4);
                gfx->sparkleSprites[i]->x2 = sSparkleCoords[i][0] + (sSparkleCoords[i][0] / (xModifier * 4));
                gfx->sparkleSprites[i]->y2 = sSparkleCoords[i][1];
                if (linkState->bigSparkle)
                    StartSpriteAnim(gfx->sparkleSprites[i], 1);
                else
                    StartSpriteAnim(gfx->sparkleSprites[i], 0);

                yModifier++;
                if (yModifier > 3)
                    yModifier = 0;
            }
        }

        #undef yModifier

        if (game->playedSound)
        {
            game->playedSound = FALSE;
        }
        else
        {
            if (numPlayersPressed == 1)
                PlaySE(SE_MUD_BALL);
            else
                PlaySE(SE_BREAKABLE_DOOR);

            game->playedSound = TRUE;
        }
    }
}

static bool32 AreEffectsFinished(struct BerryCrushGame *game, struct BerryCrushGame_Gfx *gfx)
{
    u8 i;

    // Are any impact sprites active
    for (i = 0; i < game->playerCount; i++)
    {
        if (!gfx->impactSprites[i]->invisible)
            return FALSE;
    }

    // Are any sparkle sprites active
    for (i = 0; i < ARRAY_COUNT(gfx->sparkleSprites); i++)
    {
        if (!gfx->sparkleSprites[i]->invisible)
            return FALSE;
    }

    if (game->vibration != 0)
        game->vibration = 0;

    return TRUE;
}

static void FramesToMinSec(struct BerryCrushGame_Gfx *gfx, u16 frames)
{
    u8 i = 0;
    u32 fractionalFrames = 0;
    s16 r3 = 0;

    gfx->minutes = frames / (60 * 60);
    gfx->secondsInt = (frames % (60 * 60)) / 60;
    r3 = MathUtil_Mul16(Q_8_8(frames % 60), 4);

    for (i = 0; i < 8; i++)
    {
        if ((r3 >> (7 - i)) & 1)
            fractionalFrames += sPressingSpeedConversionTable[i];
    }

    gfx->secondsFrac = fractionalFrames / 1000000;
}

static void PrintTextCentered(u8 windowId, u8 left, u8 colorId, const u8 *string)
{
    left = (left * 4) - (GetStringWidth(FONT_SHORT, string, -1) / 2u);
    AddTextPrinterParameterized3(windowId, FONT_SHORT, left, 0, sTextColorTable[colorId], 0, string);
}

static void PrintResultsText(struct BerryCrushGame * game, u8 page, u8 sp14, u8 baseY)
{
    u8 i, j;
    u8 playerId = 0;
    u8 ranking = 0;
    s32 x;
    u8 stat;
    struct BerryCrushGame_Results * results = &game->results;
    u32 xOffset;
    s32 y;

    baseY -= 16;
    if (page == RESULTS_PAGE_CRUSHING)
        baseY -= 42;

    y = baseY - 14 * game->playerCount;
    if (y > 0)
        y = y / 2 + 16;
    else
        y = 16;

    for (i = 0; i < game->playerCount; y += 14, i++)
    {
        DynamicPlaceholderTextUtil_Reset();
        switch (page)
        {
        case RESULTS_PAGE_PRESSES:
            playerId = results->playerIdsRanked[page][i];
            if (i != 0 && results->stats[page][i] != results->stats[page][i - 1])
                ranking = i;
            ConvertIntToDecimalStringN(gStringVarBuffer, results->stats[page][i], STR_CONV_MODE_RIGHT_ALIGN, 4);
            StringAppend(gStringVarBuffer, sResultsTexts[page]);
            break;
        case RESULTS_PAGE_RANDOM:
            playerId = results->playerIdsRanked[page][i];
            if (i != 0 && results->stats[page][i] != results->stats[page][i - 1])
                ranking = i;
            ConvertIntToDecimalStringN(gStringVar1, results->stats[page][i] >> 4, STR_CONV_MODE_RIGHT_ALIGN, 3);
            xOffset = 0;
            stat = results->stats[page][i] & 15;
            for (j = 0; j < 4; j++)
                if ((stat >> (3 - j)) & 1)
                    xOffset += sPressingSpeedConversionTable[j];
            stat = xOffset / 1000000u;
            ConvertIntToDecimalStringN(gStringVar2, stat, STR_CONV_MODE_LEADING_ZEROS, 2);
            StringExpandPlaceholders(gStringVarBuffer, sResultsTexts[page]);
            break;
        case RESULTS_PAGE_CRUSHING:
            playerId = i;
            ranking = i;
            j = game->players[i].berryId;
            if (j >= LAST_BERRY_INDEX - FIRST_BERRY_INDEX + 2)
                j = 0;
            StringCopy(gStringVar1, gBerries[j].name);
            StringExpandPlaceholders(gStringVarBuffer, sResultsTexts[page]);
            break;
        }
        x = GetStringRightAlignXOffset(FONT_SHORT, gStringVarBuffer, sp14 - 4);
        AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);
        if (playerId == game->localId)
            StringCopy(gStringVar3, gText_1DotBlueF700);
        else
            StringCopy(gStringVar3, gText_1DotF700);
        gStringVar3[0] = ranking + CHAR_1;
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, game->players[playerId].name);
        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, gStringVar3);
        AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, 4, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);
    }
}

static void PrintCrushingResults(struct BerryCrushGame *game)
{
    u8 i = 0;
    u8 x = 0;
    u32 pressingSpeedFrac = 0;
    struct BerryCrushGame_Results *results = &game->results;
    u8 y = GetWindowAttribute(game->gfx.resultsWindowId, WINDOW_HEIGHT) * 8 - 42;

    FramesToMinSec(&game->gfx, results->time);

    // Print time text
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gText_TimeColon);

    // Print seconds text
    x = 176 - (u8)GetStringWidth(FONT_SHORT, gText_SpaceSec, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gText_SpaceSec);

    // Print seconds value
    ConvertIntToDecimalStringN(gStringVar1, game->gfx.secondsInt, STR_CONV_MODE_LEADING_ZEROS, 2);
    ConvertIntToDecimalStringN(gStringVar2, game->gfx.secondsFrac, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_XDotY2);
    x -= GetStringWidth(FONT_SHORT, gStringVarBuffer, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);

    // Print minutes text
    x -= GetStringWidth(FONT_SHORT, gText_SpaceMin, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gText_SpaceMin);

    // Print minutes value
    ConvertIntToDecimalStringN(gStringVar1, game->gfx.minutes, STR_CONV_MODE_LEADING_ZEROS, 1);
    StringExpandPlaceholders(gStringVarBuffer, gText_StrVar1);
    x -= GetStringWidth(FONT_SHORT, gStringVarBuffer, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);

    // Print pressing speed text
    y += 14;
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, 0, y, sTextColorTable[COLORID_GRAY], 0, gText_PressingSpeed);
    x = 176 - (u8)GetStringWidth(FONT_SHORT, gText_TimesPerSec, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gText_TimesPerSec);

    // Print pressing speed value
    for (i = 0; i < 8; i++)
        if (((u8)game->pressingSpeed >> (7 - i)) & 1)
            pressingSpeedFrac += *(i + sPressingSpeedConversionTable); // It's accessed in a different way here for unknown reason
    ConvertIntToDecimalStringN(gStringVar1, game->pressingSpeed >> 8, STR_CONV_MODE_RIGHT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, pressingSpeedFrac / 1000000, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringExpandPlaceholders(gStringVarBuffer, gText_XDotY3);
    x -= GetStringWidth(FONT_SHORT, gStringVarBuffer, -1);
    if (game->newRecord)
        AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_RED], 0, gStringVarBuffer);
    else
        AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);

    // Print silkiness text
    y += 14;
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, 0, y, sTextColorTable[COLORID_GRAY], 0, gText_Silkiness);

    // Print silkiness value
    ConvertIntToDecimalStringN(gStringVar1, results->silkiness, STR_CONV_MODE_RIGHT_ALIGN, 3);
    StringExpandPlaceholders(gStringVarBuffer, gText_Var1Percent);
    x = 176 - (u8)GetStringWidth(FONT_SHORT, gStringVarBuffer, -1);
    AddTextPrinterParameterized3(game->gfx.resultsWindowId, FONT_SHORT, x, y, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);
}

static bool32 OpenResultsWindow(struct BerryCrushGame *game, struct BerryCrushGame_Gfx *gfx)
{
    u8 playerCountIdx;
    struct WindowTemplate template;

    switch (gfx->resultsState)
    {
    case 0:
        playerCountIdx = game->playerCount - 2;
        HideTimer(gfx);
        memcpy(&template, &sWindowTemplates_Results[game->gameState - RESULTS_STATE_START], sizeof(struct WindowTemplate));
        if (game->gameState == STATE_RESULTS_CRUSHING)
            template.height = sResultsWindowHeights[1][playerCountIdx];
        else
            template.height = sResultsWindowHeights[0][playerCountIdx];
        gfx->resultsWindowId = AddWindow(&template);
        break;
    case 1:
        PutWindowTilemap(gfx->resultsWindowId);
        FillWindowPixelBuffer(gfx->resultsWindowId, PIXEL_FILL(0));
        break;
    case 2:
        LoadUserWindowBorderGfx_(gfx->resultsWindowId, 541, BG_PLTT_ID(13));
        DrawStdFrameWithCustomTileAndPalette(gfx->resultsWindowId, FALSE, 541, 13);
        break;
    case 3:
        playerCountIdx = game->playerCount - 2;
        switch (game->gameState)
        {
        case STATE_RESULTS_PRESSES:
            PrintTextCentered(gfx->resultsWindowId, 20, COLORID_BLUE, gText_PressesRankings);
            PrintResultsText(game, RESULTS_PAGE_PRESSES, 0xA0, 8 * sResultsWindowHeights[0][playerCountIdx]);
            gfx->resultsState = 5; // Skip past Crushing Results text
            return FALSE;
        case STATE_RESULTS_RANDOM:
            PrintTextCentered(gfx->resultsWindowId, 20, COLORID_GREEN, sResultsTexts[game->results.randomPageId + NUM_RESULTS_PAGES]);
            PrintResultsText(game, RESULTS_PAGE_RANDOM, 0xA0, 8 * sResultsWindowHeights[0][playerCountIdx]);
            gfx->resultsState = 5; // Skip past Crushing Results text
            return FALSE;
        case STATE_RESULTS_CRUSHING:
            PrintTextCentered(gfx->resultsWindowId, 22, COLORID_BLUE, gText_CrushingResults);
            PrintResultsText(game, RESULTS_PAGE_CRUSHING, 0xB0, 8 * sResultsWindowHeights[1][playerCountIdx]);
            break;
        }
        break;
    case 4:
        PrintCrushingResults(game);
        break;
    case 5:
        CopyWindowToVram(gfx->resultsWindowId, COPYWIN_FULL);
        gfx->resultsState = 0;
        return TRUE;
    }
    gfx->resultsState++;
    return FALSE;
}

static void CloseResultsWindow(struct BerryCrushGame *game)
{
    ClearStdWindowAndFrameToTransparent(game->gfx.resultsWindowId, TRUE);
    RemoveWindow(game->gfx.resultsWindowId);
    DrawPlayerNameWindows(game);
}

#define tState             data[0]
#define tWindowId          data[1]
#define tPressingSpeeds(i) data[2 + (i)] // data[2]-[5], for different group sizes

static void Task_ShowRankings(u8 taskId)
{
    u8 i = 0, j, xPos, yPos;
    u32 score = 0;
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        tWindowId = AddWindow(&sWindowTemplate_Rankings);
        PutWindowTilemap(tWindowId);
        FillWindowPixelBuffer(tWindowId, PIXEL_FILL(0));
        LoadUserWindowBorderGfx_(tWindowId, 541, BG_PLTT_ID(13));
        DrawStdFrameWithCustomTileAndPalette(tWindowId, FALSE, 541, 13);
        break;
    case 1:
        // Print header text
        xPos = 96 - GetStringWidth(FONT_NORMAL, gText_BerryCrush2, -1) / 2u;
        AddTextPrinterParameterized3(tWindowId, FONT_NORMAL, xPos, 1, sTextColorTable[COLORID_BLUE], 0, gText_BerryCrush2);
        xPos = 96 - GetStringWidth(FONT_NORMAL, gText_PressingSpeedRankings, -1) / 2u;
        AddTextPrinterParameterized3(tWindowId, FONT_NORMAL, xPos, 17, sTextColorTable[COLORID_BLUE], 0, gText_PressingSpeedRankings);

        // Print pressing speed record for each group size, ranked
        yPos = 41;
        for (i = 0; i < MAX_RFU_PLAYERS - 1; i++)
        {
            ConvertIntToDecimalStringN(gStringVar1, i + 2, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringExpandPlaceholders(gStringVarBuffer, gText_Var1Players);
            AddTextPrinterParameterized3(tWindowId, FONT_NORMAL, 0, yPos, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);
            xPos = 192 - (u8)GetStringWidth(FONT_NORMAL, gText_TimesPerSec, -1);
            AddTextPrinterParameterized3(tWindowId, FONT_NORMAL, xPos, yPos, sTextColorTable[COLORID_GRAY], 0, gText_TimesPerSec);
            for (j = 0; j < 8; j++)
            {
                if (((tPressingSpeeds(i) & 0xFF) >> (7 - j)) & 1)
                    score += sPressingSpeedConversionTable[j];
            }
            ConvertIntToDecimalStringN(gStringVar1, (u16)tPressingSpeeds(i) >> 8, STR_CONV_MODE_RIGHT_ALIGN, 3);
            ConvertIntToDecimalStringN(gStringVar2, score / 1000000, STR_CONV_MODE_LEADING_ZEROS, 2);
            StringExpandPlaceholders(gStringVarBuffer, gText_XDotY3);
            xPos -= GetStringWidth(FONT_NORMAL, gStringVarBuffer, -1);
            AddTextPrinterParameterized3(tWindowId, FONT_NORMAL, xPos, yPos, sTextColorTable[COLORID_GRAY], 0, gStringVarBuffer);
            yPos += 16;
            score = 0;
        }
        CopyWindowToVram(tWindowId, COPYWIN_FULL);
        break;
    case 2:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            break;
        else
            return;
    case 3:
        ClearStdWindowAndFrameToTransparent(tWindowId, TRUE);
        ClearWindowTilemap(tWindowId);
        RemoveWindow(tWindowId);
        DestroyTask(taskId);
        ScriptContext_Enable();
        UnlockPlayerFieldControls();
        tState = 0;
        return;
    }
    tState++;
}

void ShowBerryCrushRankings(void)
{
    u8 taskId;

    LockPlayerFieldControls();
    taskId = CreateTask(Task_ShowRankings, 0);
    gTasks[taskId].tPressingSpeeds(0) = gSaveBlock2Ptr->berryCrush.pressingSpeeds[0];
    gTasks[taskId].tPressingSpeeds(1) = gSaveBlock2Ptr->berryCrush.pressingSpeeds[1];
    gTasks[taskId].tPressingSpeeds(2) = gSaveBlock2Ptr->berryCrush.pressingSpeeds[2];
    gTasks[taskId].tPressingSpeeds(3) = gSaveBlock2Ptr->berryCrush.pressingSpeeds[3];
}

static void PrintTimer(struct BerryCrushGame_Gfx *gfx, u16 timer)
{
    FramesToMinSec(gfx, timer);
    DigitObjUtil_PrintNumOn(0, gfx->minutes);
    DigitObjUtil_PrintNumOn(1, gfx->secondsInt);
    DigitObjUtil_PrintNumOn(2, gfx->secondsFrac);
}

static void HideTimer(struct BerryCrushGame_Gfx *gfx)
{
    gfx->timerSprites[0]->invisible = TRUE;
    gfx->timerSprites[1]->invisible = TRUE;
    DigitObjUtil_HideOrShow(2, TRUE);
    DigitObjUtil_HideOrShow(1, TRUE);
    DigitObjUtil_HideOrShow(0, TRUE);
}

static void CreatePlayerNameWindows(struct BerryCrushGame *game)
{
    u8 i;
    for (i = 0; i < game->playerCount; i++)
    {
        game->gfx.playerCoords[i] = &sPlayerCoords[sPlayerIdToPosId[game->playerCount - 2][i]];
        game->gfx.nameWindowIds[i] = AddWindow(&sWindowTemplates_PlayerNames[game->gfx.playerCoords[i]->playerId]);
        PutWindowTilemap(game->gfx.nameWindowIds[i]);
        FillWindowPixelBuffer(game->gfx.nameWindowIds[i], 0);
    }
}

static void DrawPlayerNameWindows(struct BerryCrushGame *game)
{
    u8 i;
    for (i = 0; i < game->playerCount; i++)
    {
        PutWindowTilemap(game->gfx.nameWindowIds[i]);
        if (i == game->localId)
        {
            // Print the player's name
            AddTextPrinterParameterized4(
                game->gfx.nameWindowIds[i],
                FONT_SHORT,
                36 - GetStringWidth(FONT_SHORT, game->players[i].name, 0) / 2u,
                1,
                0,
                0,
                sTextColorTable[COLORID_BLACK],
                0,
                game->players[i].name
            );
        }
        else
        {
            // Print a partner's name
            AddTextPrinterParameterized4(
                game->gfx.nameWindowIds[i],
                FONT_SHORT,
                36 - GetStringWidth(FONT_SHORT, game->players[i].name, 0) / 2u,
                1,
                0,
                0,
                sTextColorTable[COLORID_LIGHT_GRAY],
                0,
                game->players[i].name
            );
        }
        CopyWindowToVram(game->gfx.nameWindowIds[i], COPYWIN_FULL);
    }
    CopyBgTilemapBufferToVram(0);
}

// Each player name window border uses a color that corresponds to a slot of the crusher lid
static void CopyPlayerNameWindowGfxToBg(struct BerryCrushGame *game)
{
    u8 i = 0;
    u8 * windowGfx;

    LZ77UnCompWram(gBerryCrush_TextWindows_Tilemap, gDecompressionBuffer);

    for (windowGfx = gDecompressionBuffer; i < game->playerCount; i++)
    {
        CopyToBgTilemapBufferRect(
            3,
            &windowGfx[game->gfx.playerCoords[i]->playerId * 40],
            game->gfx.playerCoords[i]->windowGfxX,
            game->gfx.playerCoords[i]->windowGfxY,
            10,
            2
        );
    }
    CopyBgTilemapBufferToVram(3);
}

static void CreateGameSprites(struct BerryCrushGame *game)
{
    u8 i = 0;
    u8 spriteId;

    game->depth = CRUSHER_START_Y;
    game->vibration = 0;
    gSpriteCoordOffsetX = 0;
    gSpriteCoordOffsetY = CRUSHER_START_Y;
    for (i = 0; i < ARRAY_COUNT(sSpriteSheets) - 1; i++)
        LoadCompressedSpriteSheet(&sSpriteSheets[i]);
    LoadSpritePalettes(sSpritePals);

    // Create sprite for crusher base
    spriteId = CreateSprite(&sSpriteTemplate_CrusherBase, 120, 88, 5);
    game->gfx.coreSprite = &gSprites[spriteId];
    game->gfx.coreSprite->oam.priority = 3;
    game->gfx.coreSprite->coordOffsetEnabled = TRUE;
    game->gfx.coreSprite->animPaused = TRUE;

    // Create sprites for the impact effect
    for (i = 0; i < game->playerCount; i++)
    {
        spriteId = CreateSprite(
            &sSpriteTemplate_Impact,
            game->gfx.playerCoords[i]->impactXOffset + 120,
            game->gfx.playerCoords[i]->impactYOffset + 32,
            0
        );
        game->gfx.impactSprites[i] = &gSprites[spriteId];
        game->gfx.impactSprites[i]->oam.priority = 1;
        game->gfx.impactSprites[i]->invisible = TRUE;
        game->gfx.impactSprites[i]->coordOffsetEnabled = TRUE;
        game->gfx.impactSprites[i]->animPaused = TRUE;
    }

    // Create sprites for sparkle effect
    for (i = 0; i < ARRAY_COUNT(game->gfx.sparkleSprites); i++)
    {
        spriteId = CreateSprite(
            &sSpriteTemplate_Sparkle,
            sSparkleCoords[i][0] + 120,
            sSparkleCoords[i][1] + 136,
            6
        );
        game->gfx.sparkleSprites[i] = &gSprites[spriteId];
        game->gfx.sparkleSprites[i]->oam.priority = 3;
        game->gfx.sparkleSprites[i]->invisible = TRUE;
        game->gfx.sparkleSprites[i]->animPaused = TRUE;
        game->gfx.sparkleSprites[i]->data[0] = i;
    }

    // Create sprites for timer
    for (i = 0; i < ARRAY_COUNT(game->gfx.timerSprites); i++)
    {
        spriteId = CreateSprite(&sSpriteTemplate_Timer, 24 * i + 176, 8, 0);
        game->gfx.timerSprites[i] = &gSprites[spriteId];
        game->gfx.timerSprites[i]->oam.priority = 0;
        game->gfx.timerSprites[i]->invisible = FALSE;
        game->gfx.timerSprites[i]->animPaused = FALSE;
    }
    DigitObjUtil_CreatePrinter(0, 0, &sDigitObjTemplates[0]);
    DigitObjUtil_CreatePrinter(1, 0, &sDigitObjTemplates[1]);
    DigitObjUtil_CreatePrinter(2, 0, &sDigitObjTemplates[2]);

    if (game->gameState == STATE_INIT)
        HideTimer(&game->gfx);
}

static void DestroyGameSprites(struct BerryCrushGame *game)
{
    u8 i = 0;
    FreeSpriteTilesByTag(TAG_TIMER_DIGITS);
    FreeSpriteTilesByTag(GFXTAG_SPARKLE);
    FreeSpriteTilesByTag(GFXTAG_IMPACT);
    FreeSpriteTilesByTag(TAG_CRUSHER_BASE);
    FreeSpritePaletteByTag(TAG_TIMER_DIGITS);
    FreeSpritePaletteByTag(PALTAG_EFFECT);
    FreeSpritePaletteByTag(TAG_CRUSHER_BASE);
    for (i = 0; i < ARRAY_COUNT(game->gfx.timerSprites); i++)
        DestroySprite(game->gfx.timerSprites[i]);
    DigitObjUtil_DeletePrinter(2);
    DigitObjUtil_DeletePrinter(1);
    DigitObjUtil_DeletePrinter(0);
    for (i = 0; i < ARRAY_COUNT(game->gfx.sparkleSprites); i++)
        DestroySprite(game->gfx.sparkleSprites[i]);
    for (i = 0; i < game->playerCount; i++)
        DestroySprite(game->gfx.impactSprites[i]);
    if (game->gfx.coreSprite->inUse)
        DestroySprite(game->gfx.coreSprite);
}

static void SpriteCB_Impact(struct Sprite *sprite)
{
    if (sprite->animEnded)
    {
        sprite->invisible = TRUE;
        sprite->animPaused = TRUE;
    }
}

static void SpriteCB_Sparkle_End(struct Sprite *sprite)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sprite->data); i++)
        sprite->data[i] = 0;
    sprite->x2 = 0;
    sprite->y2 = 0;
    sprite->invisible = TRUE;
    sprite->animPaused = TRUE;
    sprite->callback = SpriteCallbackDummy;
}

#define sX         data[0]
#define sYSpeed    data[1]
#define sYAccel    data[2]
#define sXSpeed    data[3]
#define sSinIdx    data[4]
#define sSinSpeed  data[5]
#define sAmplitude data[6]
// The last element (data[7]) is a bitfield.
// The first 15 bits are the y coord to stop at.
// The last bit is a flag for whether or not to move on the x too
#define sBitfield  data[7]
#define MASK_TARGET_Y 0x7FFF
#define F_MOVE_HORIZ  0x8000

static void SpriteCB_Sparkle(struct Sprite *sprite)
{
    s16 *data = sprite->data;

    sYSpeed += sYAccel;
    sprite->y2 += sYSpeed >> 8;
    if (sBitfield & F_MOVE_HORIZ)
    {
        sprite->sX += sXSpeed;
        sSinIdx += sSinSpeed;
        sprite->x2 = Sin(sSinIdx >> 7, sAmplitude);
        if (sBitfield & F_MOVE_HORIZ && sSinIdx >> 7 > 126)
        {
            sprite->x2 = 0;
            sBitfield &= MASK_TARGET_Y;
        }
    }
    sprite->x = sX >> 7;
    if (sprite->y + sprite->y2 > (sBitfield & MASK_TARGET_Y))
        sprite->callback = SpriteCB_Sparkle_End;
}

static void SpriteCB_Sparkle_Init(struct Sprite *sprite)
{
    s16 *data = sprite->data;
    s16 xMult, xDiv;
    s32 var;
    u32 zero = 0;

    var = 640;
    sYSpeed = var;
    sYAccel = 32;
    sBitfield = 168; // Setting bits in MASK_TARGET_Y
    xMult = sprite->x2 * 128;
    xDiv = MathUtil_Div16Shift(7, (168 - sprite->y) << 7, (var + 32) >> 1);
    sprite->sX = sprite->x << 7;
    sXSpeed = MathUtil_Div16Shift(7, xMult, xDiv);
    var = MathUtil_Mul16Shift(7, xDiv, 85);
    sSinIdx = zero;
    sSinSpeed = MathUtil_Div16Shift(7, Q_8_8(63.5), var);
    sAmplitude = sprite->x2 / 4;
    sBitfield |= F_MOVE_HORIZ;
    sprite->y2 = zero;
    sprite->x2 = zero;
    sprite->callback = SpriteCB_Sparkle;
    sprite->animPaused = FALSE;
    sprite->invisible = FALSE;
}

#undef sX
#undef sYSpeed
#undef sYAccel
#undef sXSpeed
#undef sSinIdx
#undef sSinSpeed
#undef sAmplitude
#undef sBitfield
#undef MASK_TARGET_Y
#undef F_MOVE_HORIZ

static void RunOrScheduleCommand(u16 cmdId, u8 mode, u8 *args)
{
    struct BerryCrushGame *game = GetBerryCrushGame();

    if (cmdId >= ARRAY_COUNT(sBerryCrushCommands))
        cmdId = CMD_NONE;
    switch (mode)
    {
    case RUN_CMD:
        if (cmdId != CMD_NONE)
            sBerryCrushCommands[cmdId](game, args);
        if (game->nextCmd >= ARRAY_COUNT(sBerryCrushCommands))
            game->nextCmd = CMD_NONE;
        game->cmdCallback = sBerryCrushCommands[game->nextCmd];
        break;
    case SCHEDULE_CMD:
        game->cmdCallback = sBerryCrushCommands[cmdId];
        break;
    }
}

static u32 Cmd_BeginNormalPaletteFade(struct BerryCrushGame *game, u8 *args)
{
    // args points to packed values:
    // bytes 0-3: selectedPals (bitfield)
    // byte 4: delay
    // byte 5: startY
    // byte 6: stopY
    // bytes 7-8: fade color
    // byte 9: if TRUE, communicate on fade complete

    u16 color;
    u32 selectedPals[2];

    selectedPals[0] = (u32)args[0];
    selectedPals[1] = (u32)args[1];
    selectedPals[1] <<= 8;

    selectedPals[0] |= selectedPals[1];
    selectedPals[1] = (u32)args[2];
    selectedPals[1] <<= 16;

    selectedPals[0] |= selectedPals[1];
    selectedPals[1] = (u32)args[3];
    selectedPals[1] <<= 24;

    selectedPals[0] |= selectedPals[1];
    args[0] = args[9];

    color = args[8];
    color <<= 8;
    color |= args[7];

    gPaletteFade.bufferTransferDisabled = FALSE;
    BeginNormalPaletteFade(selectedPals[0], args[4], args[5], args[6], color);
    UpdatePaletteFade();
    game->nextCmd = CMD_WAIT_FADE;
    return 0;
}

static u32 Cmd_WaitPaletteFade(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        if (UpdatePaletteFade())
            return 0;
        if(args[0] != 0)
            game->cmdState++;
        else
            game->cmdState = 3;
        return 0;
    case 1:
        Rfu_SetLinkStandbyCallback();
        game->cmdState++;
        return 0;
    case 2:
        if (IsLinkTaskFinished())
        {
            game->cmdState++;
            return 0;
        }
        return 0;
    case 3:
        RunOrScheduleCommand(game->afterPalFadeCmd, SCHEDULE_CMD, NULL);
        game->cmdState = 0;
        return 0;
    default:
        game->cmdState++;
        return 0;
    }
}

static u32 Cmd_PrintMessage(struct BerryCrushGame *game, u8 *args)
{
    u16 keys = args[3];
    keys <<= 8;
    keys |= args[2];

    switch (game->cmdState)
    {
    case 0:
        DrawDialogueFrame(0, FALSE);
        if (args[1] & F_MSG_EXPAND)
        {
            StringExpandPlaceholders(gStringVarBuffer, sMessages[args[0]]);
            AddTextPrinterParameterized2(0, FONT_NORMAL, gStringVarBuffer, game->textSpeed, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        }
        else
        {
            AddTextPrinterParameterized2(0, FONT_NORMAL, sMessages[args[0]], game->textSpeed, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        }
        CopyWindowToVram(0, COPYWIN_FULL);
        break;
    case 1:
        if (!IsTextPrinterActive(0))
        {
            // If no wait keys are given, skip
            // waiting state below
            if (keys == 0)
                game->cmdState++;
            break;
        }
        return 0;
    case 2:
        if (!JOY_NEW(keys))
            return 0;
        break;
    case 3:
        if (args[1] & F_MSG_CLEAR)
            ClearDialogWindowAndFrame(0, TRUE);
        RunOrScheduleCommand(game->nextCmd, SCHEDULE_CMD, NULL);
        game->cmdState = args[4];
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_ShowGameDisplay(struct BerryCrushGame *game, u8 *args)
{
    if (ShowGameDisplay())
        RunOrScheduleCommand(game->nextCmd, RUN_CMD, game->commandArgs);
    return 0;
}

static u32 Cmd_HideGameDisplay(struct BerryCrushGame *game, u8 *args)
{
    if (HideGameDisplay())
        RunOrScheduleCommand(game->nextCmd, RUN_CMD, game->commandArgs);
    return 0;
}

static u32 Cmd_SignalReadyToBegin(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        Rfu_SetLinkStandbyCallback();
        break;
    case 1:
        if (IsLinkTaskFinished())
        {
            PlayNewMapMusic(MUS_RG_GAME_CORNER);
            RunOrScheduleCommand(CMD_ASK_PICK_BERRY, SCHEDULE_CMD, NULL);
            game->gameState = STATE_PICK_BERRY;
            game->cmdState = 0;
        }
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_AskPickBerry(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    default:
        game->cmdState++;
        break;
    case 0:
        ResetGame(game);
        SetPrintMessageArgs(args, MSG_PICK_BERRY, F_MSG_CLEAR, 0, 1);
        game->nextCmd = CMD_ASK_PICK_BERRY;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        break;
    case 1:
        game->nextCmd = CMD_PICK_BERRY;
        RunOrScheduleCommand(CMD_HIDE_GAME, SCHEDULE_CMD, NULL);
        game->cmdState = 2;
        break;
    }
    return 0;
}

static u32 Cmd_GoToBerryPouch(struct BerryCrushGame *game, u8 *args)
{
    game->cmdCallback = NULL;
    SetMainCallback2(ChooseBerry);
    return 0;
}

static u32 Cmd_WaitForOthersToPickBerries(struct BerryCrushGame *game, u8 *args)
{
    u8 i;

    switch (game->cmdState)
    {
    case 0:
        SetPrintMessageArgs(args, MSG_WAIT_PICK, 0, 0, 1);
        game->nextCmd = CMD_WAIT_BERRIES;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        return 0;
    case 1:
        Rfu_SetLinkStandbyCallback();
        break;
    case 2:
        if (!IsLinkTaskFinished())
            return 0;

        // Send player's chosen berry to partners
        memset(game->sendCmd, 0, sizeof(game->sendCmd));
        game->sendCmd[0] = game->players[game->localId].berryId;
        SendBlock(0, game->sendCmd, 2);
        break;
    case 3:
        if (!IsLinkTaskFinished())
            return 0;
        game->cmdTimer = 0;
        break;
    case 4:
        // Wait for partners responses
        if (GetBlockReceivedStatus() != sReceivedPlayerBitmasks[game->playerCount - 2])
            return 0;

        // Read partners chosen berries
        for (i = 0; i < game->playerCount; i++)
        {
            game->players[i].berryId = gBlockRecvBuffer[i][0];
            if (game->players[i].berryId > LAST_BERRY_INDEX + 1)
                game->players[i].berryId = 0;
            game->targetAPresses += gBerryCrush_BerryData[game->players[i].berryId].difficulty;
            game->powder += gBerryCrush_BerryData[game->players[i].berryId].powder;
        }
        game->cmdTimer = 0;
        ResetBlockReceivedFlags();
        game->targetDepth = MathUtil_Div32(Q_24_8(game->targetAPresses), Q_24_8(32));
        break;
    case 5:
        ClearDialogWindowAndFrame(0, TRUE);
        RunOrScheduleCommand(CMD_DROP_BERRIES, SCHEDULE_CMD, NULL);
        game->gameState = STATE_DROP_BERRIES;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_DropBerriesIntoCrusher(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        CreateBerrySprites(game, &game->gfx);
        Rfu_SetLinkStandbyCallback();
        break;
    case 1:
        if (!IsLinkTaskFinished())
            return 0;
        game->gfx.counter = 0;
        game->gfx.vibrationIdx = 0;
        game->gfx.numVibrations = 0;
        game->gfx.vibrating = FALSE;
        break;
    case 2:
        game->gfx.berrySprites[game->gfx.counter]->callback = SpriteCB_DropBerryIntoCrusher;
        game->gfx.berrySprites[game->gfx.counter]->affineAnimPaused = FALSE;
        PlaySE(SE_BALL_THROW);
        break;
    case 3:
        if (game->gfx.berrySprites[game->gfx.counter]->callback == SpriteCB_DropBerryIntoCrusher)
            return 0;
        game->gfx.berrySprites[game->gfx.counter] = NULL;
        game->gfx.counter++;
        Rfu_SetLinkStandbyCallback();
        break;
    case 4:
        if (!IsLinkTaskFinished())
            return 0;
        if (game->gfx.counter < game->playerCount)
        {
            game->cmdState = 2;
            return 0;
        }
        game->gfx.counter = 0;
        break;
    case 5:
        BerryCrushFreeBerrySpriteGfx(game, &game->gfx);
        Rfu_SetLinkStandbyCallback();
        break;
    case 6:
        if (!IsLinkTaskFinished())
            return 0;
        PlaySE(SE_FALL);
        RunOrScheduleCommand(CMD_DROP_LID, SCHEDULE_CMD, NULL);
        game->gameState = STATE_DROP_LID;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_DropLid(struct BerryCrushGame *game,  u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        game->depth += 4;
        if (game->depth < 0)
            return 0;
        game->depth = 0;
        game->gfx.vibrationIdx = 4;
        game->gfx.counter = 0;
        game->gfx.numVibrations = sIntroOutroVibrationData[game->gfx.vibrationIdx][0];
        PlaySE(SE_M_STRENGTH);
        break;
    case 1:
        game->vibration = sIntroOutroVibrationData[game->gfx.vibrationIdx][game->gfx.counter];
        SetGpuReg(REG_OFFSET_BG0VOFS, -game->vibration);
        SetGpuReg(REG_OFFSET_BG2VOFS, -game->vibration);
        SetGpuReg(REG_OFFSET_BG3VOFS, -game->vibration);
        game->gfx.counter++;
        if (game->gfx.counter < game->gfx.numVibrations)
            return 0;
        if (game->gfx.vibrationIdx == 0)
            break;
        game->gfx.vibrationIdx--;
        game->gfx.numVibrations = sIntroOutroVibrationData[game->gfx.vibrationIdx][0];
        game->gfx.counter = 0;
        return 0;
    case 2:
        game->vibration = 0;
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        SetGpuReg(REG_OFFSET_BG3VOFS, 0);
        Rfu_SetLinkStandbyCallback();
        break;
    case 3:
        if (!IsLinkTaskFinished())
            return 0;
        RunOrScheduleCommand(CMD_COUNTDOWN, SCHEDULE_CMD, NULL);
        game->gameState = STATE_COUNTDOWN;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_Countdown(struct BerryCrushGame *game,  u8 *args)
{
    switch (game->cmdState)
    {
    case 1:
        if (!IsLinkTaskFinished())
            return 0;
        StartMinigameCountdown(TAG_COUNTDOWN, TAG_COUNTDOWN, 120, 80, 0);
        break;
    case 2:
        if (IsMinigameCountdownRunning())
            return 0;
        // fallthrough
    case 0:
        Rfu_SetLinkStandbyCallback();
        break;
    case 3:
        if (!IsLinkTaskFinished())
            return 0;
        game->gfx.counter = 0;
        game->gfx.vibrationIdx = 0;
        game->gfx.numVibrations = 0;
        game->gfx.vibrating = FALSE;
        game->cmdTimer = 0;
        if (game->localId == 0)
            RunOrScheduleCommand(CMD_PLAY_GAME_LEADER, SCHEDULE_CMD, NULL);
        else
            RunOrScheduleCommand(CMD_PLAY_GAME_MEMBER, SCHEDULE_CMD, NULL);
        game->gameState = STATE_PLAYING;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

// Receive and process data from all players
// Only used by the link leader
static void HandlePartnerInput(struct BerryCrushGame *game)
{
    u8 numPlayersPressed = 0;
    u8 i = 0;
    u16 timeDiff;
    s32 temp = 0;
    struct BerryCrushGame_LinkState *linkState;

    for (i = 0; i < game->playerCount; i++)
    {
        linkState = (struct BerryCrushGame_LinkState *)gRecvCmds[i];

        // Skip player if we have not received a packet from them
        if ((linkState->rfuCmd & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
            continue;
        if (linkState->sendFlag != SEND_GAME_STATE)
            continue;

        if (linkState->pushedAButton)
        {
            game->localState.playerPressedAFlags |= sBitTable[i];
            game->players[i].inputState = INPUT_STATE_HIT;
            game->players[i].numAPresses++;
            numPlayersPressed++;
            timeDiff = game->timer - game->players[i].inputTime;

            // If the interval between inputs is regular, the input is considered "neat"
            // This counts toward the player's neatness score
            if (timeDiff >= game->players[i].timeSincePrevInput - 1
             && timeDiff <= game->players[i].timeSincePrevInput + 1)
            {
                // On neat input streak
                game->players[i].neatInputStreak++;
                game->players[i].timeSincePrevInput = timeDiff;
                if (game->players[i].neatInputStreak > game->players[i].maxNeatInputStreak)
                    game->players[i].maxNeatInputStreak = game->players[i].neatInputStreak;
            }
            else
            {
                // End neat input streak
                game->players[i].neatInputStreak = 0;
                game->players[i].timeSincePrevInput = timeDiff;
            }

            game->players[i].inputTime = game->timer;
            game->players[i].inputFlags++;
            if (game->players[i].inputFlags > F_INPUT_HIT_B)
                game->players[i].inputFlags = 0;
        }
        else
        {
            game->players[i].inputState = INPUT_STATE_NONE;
        }
    }
    if (numPlayersPressed > 1)
    {
        // For each player that pressed A, flag their input as synchronous
        // This is used to change their impact sprite to a big impact
        for (i = 0; i < game->playerCount; i++)
        {
            if (game->players[i].inputState == INPUT_STATE_NONE)
                continue;
            game->players[i].inputState |= INPUT_STATE_HIT_SYNC;
            game->players[i].numSyncedAPresses++;
        }
    }
    if (numPlayersPressed == 0)
        return;

    game->bigSparkleCounter += numPlayersPressed;
    numPlayersPressed += sSyncPressBonus[numPlayersPressed - 1];
    game->sparkleCounter += numPlayersPressed;
    game->totalAPresses += numPlayersPressed;
    if (game->targetAPresses - game->totalAPresses > 0)
    {
        temp = (s32)game->totalAPresses;
        temp = Q_24_8(temp);
        temp = MathUtil_Div32(temp, game->targetDepth);
        temp = Q_24_8_TO_INT(temp);
        game->newDepth = (u8)temp;
        return;
    }

    // Target number of A presses has been reached, game is complete
    game->newDepth = 32;
    game->localState.endGame = TRUE;
}

// Updates the crusher, input flags, and timer to send to group members
// Only used by the link leader
static void UpdateLeaderGameState(struct BerryCrushGame *game)
{
    u8 numPlayersPressed = 0;
    u16 flags = 0;
    u16 temp = 0;
    u8 i = 0;

    for (i = 0; i < game->playerCount; i++)
    {
        if (game->players[i].inputState != INPUT_STATE_NONE)
        {
            numPlayersPressed++;
            flags = game->players[i].inputFlags + F_INPUT_HIT_A;
            if (game->players[i].inputState & INPUT_STATE_HIT_SYNC)
                flags |= F_INPUT_HIT_SYNC;
            flags <<= INPUT_FLAGS_PER_PLAYER * i;
            game->localState.inputFlags |= flags;
        }
    }
    temp = (u16)game->newDepth;
    game->localState.depth = temp;
    if (numPlayersPressed == 0)
    {
        if (game->gfx.vibrating)
            game->gfx.counter++;
    }
    else if (game->gfx.vibrating)
    {
        if (numPlayersPressed != game->gfx.vibrationIdx)
        {
            game->gfx.vibrationIdx = numPlayersPressed - 1;
            game->gfx.numVibrations = sVibrationData[numPlayersPressed - 1][0];
        }
        else
        {
            game->gfx.counter++;
        }
    }
    else
    {
        game->gfx.counter = 0;
        game->gfx.vibrationIdx = numPlayersPressed - 1;
        game->gfx.numVibrations = sVibrationData[numPlayersPressed - 1][0];
        game->gfx.vibrating = TRUE;
    }

    if (game->gfx.vibrating)
    {
        if (game->gfx.counter >= game->gfx.numVibrations)
        {
            game->gfx.counter = 0;
            game->gfx.vibrationIdx = 0;
            game->gfx.numVibrations = 0;
            game->gfx.vibrating = FALSE;
            temp = 0;
        }
        else
        {
            temp = sVibrationData[game->gfx.vibrationIdx][game->gfx.counter + 1];
        }
        game->localState.vibration = (u8)temp;
    }
    else
    {
        game->localState.vibration = 0;
    }
    game->localState.timer = game->leaderTimer;
}

// Checks for input and sends data to group members
static void HandlePlayerInput(struct BerryCrushGame *game)
{
    if (JOY_NEW(A_BUTTON))
        game->localState.pushedAButton = TRUE;

    if (JOY_HELD(A_BUTTON))
    {
        if (game->players[game->localId].timePressingA < game->timer)
            game->players[game->localId].timePressingA++;
    }

    // Only send data to other players if you are the leader or you pressed A
    if (game->localId != 0 && !game->localState.pushedAButton)
        return;
    game->localState.sendFlag = SEND_GAME_STATE;

    // Every 30 frames, check whether the sparkles produced should be big,
    // depending on how many A presses there were in that time
    if (game->timer % 30 == 0)
    {
        if (game->bigSparkleCounter > sBigSparkleThresholds[game->playerCount - 2])
        {
            game->numBigSparkles++;
            game->bigSparkle = TRUE;
        }
        else
        {
            game->bigSparkle = FALSE;
        }
        game->bigSparkleCounter = 0;
        game->numBigSparkleChecks++;
    }

    // Every 15 frames, update the amount of sparkles that should be produced,
    // depending on how many A presses there were in that time (including the bonus)
    if (game->timer % 15 == 0)
    {
        // BUG: The wrong field is used twice below
        // As a result, only a sparkleAmount of 0, 1, or 4 is attainable
        #ifdef BUGFIX
        #define field sparkleAmount
        #else
        #define field sparkleCounter
        #endif

        if (game->sparkleCounter < sSparkleThresholds[game->playerCount - 2][0])
            game->sparkleAmount = 0;
        else if (game->sparkleCounter < sSparkleThresholds[game->playerCount - 2][1])
            game->sparkleAmount = 1;
        else if (game->sparkleCounter < sSparkleThresholds[game->playerCount - 2][2])
            game->field = 2;
        else if (game->sparkleCounter < sSparkleThresholds[game->playerCount - 2][3])
            game->field = 3;
        else
            game->sparkleAmount = 4;
        game->sparkleCounter = 0;

        #undef field
    }
    else
    {
        game->cmdTimer++;
        if (game->cmdTimer > 60)
        {
            if (game->cmdTimer > 70)
            {
                ClearRecvCommands();
                game->cmdTimer = 0;
            }
            else if (game->localState.playerPressedAFlags == 0)
            {
                ClearRecvCommands();
                game->cmdTimer = 0;
            }
        }

    }
    if (game->timer >= MAX_TIME)
        game->localState.endGame = TRUE;
    game->localState.bigSparkle = game->bigSparkle;
    game->localState.sparkleAmount = game->sparkleAmount;
    memcpy(game->sendCmd, &game->localState, sizeof(game->sendCmd));
    Rfu_SendPacket(game->sendCmd);
}

static void RecvLinkData(struct BerryCrushGame *game)
{
    u8 i = 0;
    struct BerryCrushGame_LinkState *linkState = NULL;

    for (i = 0; i < game->playerCount; i++)
        game->players[i].inputState = INPUT_STATE_NONE;

    if ((gRecvCmds[0][0] & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
    {
        game->playedSound = FALSE;
        return;
    }
    if (gRecvCmds[0][1] != 2)
    {
        game->playedSound = FALSE;
        return;
    }

    memcpy(game->recvCmd, gRecvCmds[0], sizeof(game->recvCmd));
    linkState = (struct BerryCrushGame_LinkState *)&game->recvCmd;
    game->depth = linkState->depth;
    game->vibration = (s16)linkState->vibration;
    game->timer = linkState->timer;
    UpdateInputEffects(game, &(game->gfx));

    if (linkState->endGame)
        game->endGame = TRUE;
}

static u32 Cmd_PlayGame_Leader(struct BerryCrushGame *game, u8 *args)
{
    memset(&game->localState, 0, sizeof(game->localState));
    memset(&game->recvCmd, 0, sizeof(game->recvCmd));
    RecvLinkData(game);
    SetGpuReg(REG_OFFSET_BG0VOFS, -game->vibration);
    SetGpuReg(REG_OFFSET_BG2VOFS, -game->vibration);
    SetGpuReg(REG_OFFSET_BG3VOFS, -game->vibration);
    if (game->endGame)
    {
        if (game->timer >= MAX_TIME)
        {
            game->timer = MAX_TIME;
            RunOrScheduleCommand(CMD_TIMES_UP, SCHEDULE_CMD, NULL);
        }
        else
        {
            RunOrScheduleCommand(CMD_FINISH_GAME, SCHEDULE_CMD, NULL);
        }
        game->cmdTimer = 0;
        game->cmdState = 0;
        return 0;
    }
    else
    {
        game->leaderTimer++;
        HandlePartnerInput(game);
        UpdateLeaderGameState(game);
        HandlePlayerInput(game);
        return 0;
    }
}

static u32 Cmd_PlayGame_Member(struct BerryCrushGame *game, u8 *args)
{
    memset(&game->localState, 0, sizeof(game->localState));
    memset(&game->recvCmd, 0, sizeof(game->recvCmd));
    RecvLinkData(game);
    SetGpuReg(REG_OFFSET_BG0VOFS, -game->vibration);
    SetGpuReg(REG_OFFSET_BG2VOFS, -game->vibration);
    SetGpuReg(REG_OFFSET_BG3VOFS, -game->vibration);
    if (game->endGame)
    {
        if (game->timer >= MAX_TIME)
        {
            game->timer = MAX_TIME;
            RunOrScheduleCommand(CMD_TIMES_UP, SCHEDULE_CMD, NULL);
        }
        else
        {
            RunOrScheduleCommand(CMD_FINISH_GAME, SCHEDULE_CMD, NULL);
        }
        game->cmdTimer = 0;
        game->cmdState = 0;
        return 0;
    }
    else
    {
        HandlePlayerInput(game);
        return 0;
    }
}

// Game was 'won', crusher was pushed down fully before time was up
static u32 Cmd_FinishGame(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        game->gameState = STATE_FINISHED;
        PlaySE(SE_M_STRENGTH);
        BlendPalettes(PALETTES_ALL, 8, RGB_YELLOW);
        game->gfx.counter = 2;
        break;
    case 1:
        if (--game->gfx.counter != (u8)-1)
            return 0;
        BlendPalettes(PALETTES_ALL, 0, RGB_YELLOW);
        game->gfx.vibrationIdx = 4;
        game->gfx.counter = 0;
        game->gfx.numVibrations = sIntroOutroVibrationData[game->gfx.vibrationIdx][0];
        break;
    case 2:
        game->vibration = sIntroOutroVibrationData[game->gfx.vibrationIdx][game->gfx.counter];
        SetGpuReg(REG_OFFSET_BG0VOFS, -game->vibration);
        SetGpuReg(REG_OFFSET_BG2VOFS, -game->vibration);
        SetGpuReg(REG_OFFSET_BG3VOFS, -game->vibration);
        if (++game->gfx.counter < game->gfx.numVibrations)
            return 0;
        if (game->gfx.vibrationIdx != 0)
        {
            game->gfx.vibrationIdx--;
            game->gfx.numVibrations = sIntroOutroVibrationData[game->gfx.vibrationIdx][0];
            game->gfx.counter = 0;
            return 0;
        }
        break;
    case 3:
        game->vibration = 0;
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        SetGpuReg(REG_OFFSET_BG3VOFS, 0);
        break;
    case 4:
        if (!AreEffectsFinished(game, &game->gfx))
            return 0;
        Rfu_SetLinkStandbyCallback();
        game->cmdTimer = 0;
        break;
    case 5:
        if (!IsLinkTaskFinished())
            return 0;
        RunOrScheduleCommand(CMD_CALC_RESULTS, SCHEDULE_CMD, NULL);
        game->cmdTimer = 0;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_HandleTimeUp(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        game->gameState = STATE_TIMES_UP;
        PlaySE(SE_FAILURE);
        BlendPalettes(PALETTES_ALL, 8, RGB_RED);
        game->gfx.counter = 4;
        break;
    case 1:
        if (--game->gfx.counter != (u8)-1)
            return 0;
        BlendPalettes(PALETTES_ALL, 0, RGB_RED);
        game->gfx.counter = 0;
        break;
    case 2:
        if (!AreEffectsFinished(game, &game->gfx))
            return 0;
        Rfu_SetLinkStandbyCallback();
        game->cmdTimer = 0;
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        SetGpuReg(REG_OFFSET_BG3VOFS, 0);
        break;
    case 3:
        if (!IsLinkTaskFinished())
            return 0;
        ConvertIntToDecimalStringN(gStringVar1, game->powder, STR_CONV_MODE_LEFT_ALIGN, 6);
        SetPrintMessageArgs(args, MSG_TIMES_UP, F_MSG_CLEAR, 0, 0);
        game->nextCmd = CMD_SAVE;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        game->cmdTimer = 0;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_TabulateResults(struct BerryCrushGame *game, u8 *args)
{
    u8 i, j, tempPlayerId;
    s32 temp1, temp2;
    u16 tempStat;

    switch (game->cmdState)
    {
    case 0:
        memset(game->sendCmd, 0, 2 * sizeof(u16));
        if (game->players[game->localId].timePressingA > game->timer)
            game->players[game->localId].timePressingA = game->timer;
        game->sendCmd[0] = game->players[game->localId].timePressingA;
        SendBlock(0, game->sendCmd, 2);
        break;
    case 1:
        if (!IsLinkTaskFinished())
            return 0;
        game->cmdTimer = 0;
        break;
    case 2:
        if (GetBlockReceivedStatus() != sReceivedPlayerBitmasks[game->playerCount - 2])
            return 0;
        for (i = 0; i < game->playerCount; i++)
            game->players[i].timePressingA = gBlockRecvBuffer[i][0];

        game->cmdTimer = 0;
        game->sendCmd[0] = 0;
        ResetBlockReceivedFlags();

        // If player is not leader, skip the steps
        // where the results are calculated and sent.
        // Group members just read the results sent
        // to them by the leader.
        if (game->localId == 0)
            game->cmdState = 3;
        else
            game->cmdState = 6;
        return 0;
    case 3:
        memset(&game->results, 0, sizeof(game->results));
        game->results.time = game->timer;
        game->results.targetPressesPerSec = game->targetAPresses / (game->timer / 60);

        // Calculate silkiness
        // Silkiness is the percentage of times big sparkles were produced when possible,
        // which itself depends on the number of A presses every 30 frames
        temp1 = MathUtil_Mul32(Q_24_8(game->numBigSparkles), Q_24_8(50));
        temp1 = MathUtil_Div32(temp1, Q_24_8(game->numBigSparkleChecks)) + Q_24_8(50);
        temp1 = Q_24_8_TO_INT(temp1);
        game->results.silkiness = temp1 & 0x7F;

        // Calculate amount of powder
        temp1 = Q_24_8(temp1);
        temp1 = MathUtil_Div32(temp1, Q_24_8(100));
        temp2 = Q_24_8(game->powder * game->playerCount);
        temp2 = MathUtil_Mul32(temp2, temp1);
        game->results.powder = Q_24_8_TO_INT(temp2);

        // Choose random second results page
        game->results.randomPageId = Random() % NUM_RANDOM_RESULTS_PAGES;

        for (i = 0; i < game->playerCount; i++)
        {
            game->results.playerIdsRanked[RESULTS_PAGE_PRESSES][i] = i;
            game->results.playerIdsRanked[RESULTS_PAGE_RANDOM][i] = i;
            game->results.stats[RESULTS_PAGE_PRESSES][i] = game->players[i].numAPresses;
            game->results.totalAPresses += game->results.stats[RESULTS_PAGE_PRESSES][i];

            // Calculate value for random second results page
            switch (game->results.randomPageId)
            {
            case RESULTS_PAGE_NEATNESS:
                if (game->players[i].numAPresses != 0)
                {
                    // Calculate percentage of inputs that were in largest "neat" streak
                    // "Neat" inputs are those done at a regular interval
                    temp1 = game->players[i].maxNeatInputStreak;
                    temp1 = Q_24_8(temp1);
                    temp1 = MathUtil_Mul32(temp1, Q_24_8(100));
                    temp2 = game->players[i].numAPresses;
                    temp2 = Q_24_8(temp2);
                    temp2 = MathUtil_Div32(temp1, temp2);
                }
                else
                {
                    temp2 = 0;
                }
                break;
            case RESULTS_PAGE_COOPERATIVE:
                if (game->players[i].numAPresses != 0)
                {
                    // Calculate percentage of inputs that were
                    // done at the same time as another player
                    temp1 = game->players[i].numSyncedAPresses;
                    temp1 = Q_24_8(temp1);
                    temp1 = MathUtil_Mul32(temp1, Q_24_8(100));
                    temp2 = game->players[i].numAPresses;
                    temp2 = Q_24_8(temp2);
                    temp2 = MathUtil_Div32(temp1, temp2);
                }
                else
                {
                    temp2 = 0;
                }
                break;
            case RESULTS_PAGE_POWER:
                if (game->players[i].numAPresses == 0)
                {
                    temp2 = 0;
                }
                else if (game->players[i].timePressingA >= game->timer)
                {
                    // Spent 100% of the time pressing A
                    temp2 = Q_24_8(100);
                }
                else
                {
                    // Calculate percentage of time the
                    // player spent pressing A
                    temp1 = game->players[i].timePressingA;
                    temp1 = Q_24_8(temp1);
                    temp1 = MathUtil_Mul32(temp1, Q_24_8(100));
                    temp2 = game->timer;
                    temp2 = Q_24_8(temp2);
                    temp2 = MathUtil_Div32(temp1, temp2);
                }
                break;
            }
            temp2 >>= 4;
            game->results.stats[RESULTS_PAGE_RANDOM][i] = temp2;
        }
        break;
    case 4:
        for (i = 0; i < game->playerCount - 1; i++)
        {
            for (j = game->playerCount - 1; j > i; j--)
            {
                // Calculate player rankings for "Number of Presses" by sorting arrays
                if (game->results.stats[RESULTS_PAGE_PRESSES][j - 1] < game->results.stats[RESULTS_PAGE_PRESSES][j])
                {
                    SWAP(game->results.stats[RESULTS_PAGE_PRESSES][j],
                         game->results.stats[RESULTS_PAGE_PRESSES][j - 1],
                         tempStat);
                    SWAP(game->results.playerIdsRanked[RESULTS_PAGE_PRESSES][j],
                         game->results.playerIdsRanked[RESULTS_PAGE_PRESSES][j - 1],
                         tempPlayerId);
                }
                // Calculate player rankings for random second results page by sorting arrays
                if (game->results.stats[RESULTS_PAGE_RANDOM][j - 1] < game->results.stats[RESULTS_PAGE_RANDOM][j])
                {
                    SWAP(game->results.stats[RESULTS_PAGE_RANDOM][j],
                         game->results.stats[RESULTS_PAGE_RANDOM][j - 1],
                         tempStat);
                    SWAP(game->results.playerIdsRanked[RESULTS_PAGE_RANDOM][j],
                         game->results.playerIdsRanked[RESULTS_PAGE_RANDOM][j - 1],
                         tempPlayerId);
                }
            }
        }
        SendBlock(0, &game->results, sizeof(game->results));
        break;
    case 5:
        if (!IsLinkTaskFinished())
            return 0;
        game->cmdTimer = 0;
        break;
    case 6:
        if (GetBlockReceivedStatus() != 1)
            return 0;

        // Receive results calculated by leader
        memset(&game->results, 0, sizeof(game->results));
        memcpy(&game->results, gBlockRecvBuffer, sizeof(game->results));
        ResetBlockReceivedFlags();
        game->cmdTimer = 0;
        break;
    case 7:
        SaveResults();
        RunOrScheduleCommand(CMD_SHOW_RESULTS, SCHEDULE_CMD, NULL);
        game->gameState = STATE_RESULTS_PRESSES;
        game->cmdState = 0;
        game->newDepth = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_ShowResults(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        if (!OpenResultsWindow(game, &game->gfx))
            return 0;
        break;
    case 1:
        CopyBgTilemapBufferToVram(0);
        game->gfx.counter = 30;
        break;
    case 2:
        if (game->gfx.counter != 0)
        {
            game->gfx.counter--;
            return 0;
        }
        if (!(JOY_NEW(A_BUTTON)))
            return 0;
        PlaySE(SE_SELECT);
        CloseResultsWindow(game);
        break;
    case 3:
        // Progress through each page of the results
        if (game->gameState < RESULTS_STATE_END)
        {
            game->gameState++;
            game->cmdState = 0;
            return 0;
        }
        break;
    case 4:
        // Print message showing how much powder was created
        ConvertIntToDecimalStringN(gStringVar1, game->powder, STR_CONV_MODE_LEFT_ALIGN, 6);
        ConvertIntToDecimalStringN(gStringVar2, GetBerryPowder(), STR_CONV_MODE_LEFT_ALIGN, 6);
        SetPrintMessageArgs(args, MSG_POWDER, F_MSG_CLEAR | F_MSG_EXPAND, 0, 0);
        game->nextCmd = CMD_SAVE;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_SaveGame(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        if (game->timer >= MAX_TIME)
            HideTimer(&game->gfx);
        SetPrintMessageArgs(args, MSG_COMM_STANDBY, 0, 0, 1);
        game->nextCmd = CMD_SAVE;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        game->cmdState = 0; // State is progressed by CMD_PRINT_MSG
        return 0;
    case 1:
        Rfu_SetLinkStandbyCallback();
        break;
    case 2:
        if (!IsLinkTaskFinished())
            return 0;
        DrawDialogueFrame(0, FALSE);
        AddTextPrinterParameterized2(0, FONT_NORMAL, gText_SavingDontTurnOffPower, 0, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        CopyWindowToVram(0, COPYWIN_FULL);
        CreateTask(Task_LinkFullSave, 0);
        break;
    case 3:
        if (FuncIsActiveTask(Task_LinkFullSave))
            return 0;
        break;
    case 4:
        RunOrScheduleCommand(CMD_ASK_PLAY_AGAIN, SCHEDULE_CMD, NULL);
        game->gameState = STATE_PLAY_AGAIN;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_AskPlayAgain(struct BerryCrushGame *game, u8 *args)
{
    s8 input = 0;

    switch (game->cmdState)
    {
    case 0:
        SetPrintMessageArgs(args, MSG_PLAY_AGAIN, 0, 0, 1);
        game->nextCmd = CMD_ASK_PLAY_AGAIN;
        RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
        game->cmdState = 0; // State is progressed by CMD_PRINT_MSG
        return 0;
    case 1:
        DisplayYesNoMenuDefaultYes();
        break;
    case 2:
        input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input != -2)
        {
            memset(game->sendCmd, 0, sizeof(game->sendCmd));
            if (input == 0)
            {
                // Selected Yes
                if (HasAtLeastOneBerry())
                    game->playAgainState = PLAY_AGAIN_YES;
                else
                    game->playAgainState = PLAY_AGAIN_NO_BERRIES;
            }
            else
            {
                // Selected No
                game->playAgainState = PLAY_AGAIN_NO;
            }

            // Close Yes/No and start communication
            ClearDialogWindowAndFrame(0, TRUE);
            SetPrintMessageArgs(args, MSG_COMM_STANDBY, 0, 0, 0);
            game->nextCmd = CMD_COMM_PLAY_AGAIN;
            RunOrScheduleCommand(CMD_PRINT_MSG, SCHEDULE_CMD, NULL);
            game->cmdState = 0;
        }
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_CommunicatePlayAgainResponses(struct BerryCrushGame *game, u8 *args)
{
    u8 i = 0;

    switch (game->cmdState)
    {
    case 0:
        Rfu_SetLinkStandbyCallback();
        break;
    case 1:
        if (!IsLinkTaskFinished())
            return 0;

        // Send player's Yes/No response to partners
        game->sendCmd[0] = game->playAgainState;
        game->recvCmd[0] = 0;
        SendBlock(0, game->sendCmd, sizeof(u16));
        break;
    case 2:
        if (!IsLinkTaskFinished())
            return 0;
        game->cmdTimer = 0;
        break;
    case 3:
        // Wait for partners responses
        if (GetBlockReceivedStatus() != sReceivedPlayerBitmasks[game->playerCount - 2])
            return 0;

        // Read partners responses
        for (i = 0; i < game->playerCount; i++)
            game->recvCmd[0] += gBlockRecvBuffer[i][0];

        if (game->recvCmd[0] != PLAY_AGAIN_YES)
            RunOrScheduleCommand(CMD_PLAY_AGAIN_NO, SCHEDULE_CMD, NULL);
        else
            RunOrScheduleCommand(CMD_PLAY_AGAIN_YES, SCHEDULE_CMD, NULL);
        ResetBlockReceivedFlags();
        game->sendCmd[0] = 0;
        game->recvCmd[0] = 0;
        game->cmdTimer = 0;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_PlayAgain(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 1, 0, 16, RGB_BLACK);
        UpdatePaletteFade();
        break;
    case 1:
        if (UpdatePaletteFade())
            return 0;
        break;
    case 2:
        ClearDialogWindowAndFrame(0, TRUE);
        ResetCrusherPos(game);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        UpdatePaletteFade();
        break;
    case 3:
        if (UpdatePaletteFade())
            return 0;
        RunOrScheduleCommand(CMD_ASK_PICK_BERRY, SCHEDULE_CMD, NULL);
        game->gameState = STATE_PICK_BERRY;
        game->cmdState = 0;
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_StopGame(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        DrawDialogueFrame(0, FALSE);
        if (game->playAgainState == PLAY_AGAIN_NO_BERRIES)
            AddTextPrinterParameterized2(0, FONT_NORMAL, sMessages[MSG_NO_BERRIES], game->textSpeed, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        else
            AddTextPrinterParameterized2(0, FONT_NORMAL, sMessages[MSG_DROPPED], game->textSpeed, 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        CopyWindowToVram(0, COPYWIN_FULL);
        break;
    case 1:
        if (IsTextPrinterActive(0))
            return 0;
        game->gfx.counter = 120;
        break;
    case 2:
        if (game->gfx.counter != 0)
        {
            game->gfx.counter--;
        }
        else
        {
            RunOrScheduleCommand(CMD_CLOSE_LINK, SCHEDULE_CMD, NULL);
            game->cmdState = 0;
        }
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_CloseLink(struct BerryCrushGame *game, u8 *args)
{
    switch (game->cmdState)
    {
    case 0:
        Rfu_SetLinkStandbyCallback();
        break;
    case 1:
        if (!IsLinkTaskFinished())
            return 0;
        SetCloseLinkCallback();
        break;
    case 2:
        if (gReceivedRemoteLinkPlayers)
            return 0;
        game->nextCmd = CMD_QUIT;
        RunOrScheduleCommand(CMD_HIDE_GAME, SCHEDULE_CMD, NULL);
        game->cmdState = 2; // ???
        return 0;
    }
    game->cmdState++;
    return 0;
}

static u32 Cmd_Quit(struct BerryCrushGame *game, u8 *args)
{
    QuitBerryCrush(NULL);
    return 0;
}

static void ResetGame(struct BerryCrushGame *game)
{
    u8 i = 0;

    IncrementGameStat(GAME_STAT_PLAYED_BERRY_CRUSH);
    game->unused = 0;
    game->cmdTimer = 0;
    game->gameState = STATE_RESET;
    game->playAgainState = 0;
    game->powder = 0;
    game->targetAPresses = 0;
    game->totalAPresses = 0;
    game->targetDepth = 0;
    game->newDepth = 0;
    game->noRoomForPowder = FALSE;
    game->newRecord = FALSE;
    game->playedSound = FALSE;
    game->endGame = FALSE;
    game->bigSparkle = FALSE;
    game->sparkleAmount = 0;
    game->leaderTimer = 0;
    game->timer = 0;
    game->bigSparkleCounter = 0;
    game->numBigSparkleChecks = -1;
    game->numBigSparkles = 0;
    game->sparkleCounter = 0;
    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        game->players[i].berryId = -1;
        game->players[i].inputTime = 0;
        game->players[i].neatInputStreak = 0;
        game->players[i].timeSincePrevInput = 1;
        game->players[i].maxNeatInputStreak = 0;
        game->players[i].numAPresses = 0;
        game->players[i].numSyncedAPresses = 0;
        game->players[i].timePressingA = 0;
        game->players[i].inputFlags = 0;
        game->players[i].inputState = INPUT_STATE_NONE;
    }
}

static void SetPaletteFadeArgs(u8 *args, bool8 communicateAfter, u32 selectedPals, s8 delay, u8 startY, u8 targetY, u16 palette)
{
    args[0] = ((u8 *)&selectedPals)[0];
    args[1] = ((u8 *)&selectedPals)[1];
    args[2] = ((u8 *)&selectedPals)[2];
    args[3] = ((u8 *)&selectedPals)[3];
    args[4] = delay;
    args[5] = startY;
    args[6] = targetY;
    args[7] = ((u8 *)&palette)[0];
    args[8] = ((u8 *)&palette)[1];
    args[9] = communicateAfter;
}

static void SetPrintMessageArgs(u8 *args, u8 msgId, u8 flags, u16 waitKeys, u8 followupState)
{
    args[0] = msgId;
    args[1] = flags;
    args[2] = ((u8 *)&waitKeys)[0];
    args[3] = ((u8 *)&waitKeys)[1];
    args[4] = followupState;
}
