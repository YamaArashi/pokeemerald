#include "global.h"
#include "malloc.h"
#include "bg.h"
#include "dodrio_berry_picking.h"
#include "dynamic_placeholder_text_util.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item.h"
#include "link.h"
#include "link_rfu.h"
#include "m4a.h"
#include "main.h"
#include "palette.h"
#include "minigame_countdown.h"
#include "random.h"
#include "save.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text_window.h"
#include "window.h"
#include "constants/items.h"
#include "constants/songs.h"

// Note that in this file 'Dodrio Berry Picking' is often
// shortened to DodrioGame or just Game for convenience

#define MAX_SCORE 999990
#define MAX_BERRIES 9999

// The minimum score needed to receive a prize
#define PRIZE_SCORE 3000

// Difficulty increases as berries are eaten. The rate of new berries increases and the types of berries changes
// When the max difficulty is reached it starts again from the beginning
#define NUM_DIFFICULTIES 7

#define MAX_FALL_DIST 10 // The number of times a berry needs to fall before hitting the ground
#define EAT_FALL_DIST 7 // The number of times a berry needs to fall to be available to eat

enum {
    BG_INTERFACE,
    BG_TREE_LEFT,
    BG_TREE_RIGHT,
    BG_SCENERY
};

enum {
    FUNC_INTRO,
    FUNC_INIT_COUNTDOWN,
    FUNC_COUNTDOWN,
    FUNC_WAIT_START,
    FUNC_PLAY_GAME,
    FUNC_INIT_RESULTS,
    FUNC_RESULTS,
    FUNC_ASK_PLAY_AGAIN,
    FUNC_END_LINK,
    FUNC_EXIT,
    FUNC_RESET_GAME,
    FUNC_WAIT_END_GAME,
};

enum {
    GFXFUNC_LOAD,
    GFXFUNC_SHOW_NAMES,
    GFXFUNC_SHOW_RESULTS,
    GFXFUNC_MSG_PLAY_AGAIN,
    GFXFUNC_MSG_SAVING,
    GFXFUNC_MSG_COMM_STANDBY,
    GFXFUNC_ERASE_MSG,
    GFXFUNC_MSG_PLAYER_DROPPED,
    GFXFUNC_STOP,
    GFXFUNC_IDLE,
};

enum {
    PACKET_READY_START = 1,
    PACKET_GAME_STATE,
    PACKET_PICK_STATE,
    PACKET_READY_END,
};

enum {
    PLAY_AGAIN_NONE,
    PLAY_AGAIN_YES,
    PLAY_AGAIN_NO,
    PLAY_AGAIN_DROPPED = 5,
};

enum {
    PICK_NONE,     // Dodrio standing still
    PICK_RIGHT,    // Dodrio reaching right
    PICK_MIDDLE,   // Dodrio reaching up
    PICK_LEFT,     // Dodrio reaching left
    PICK_DISABLED, // Dodrio down after game over
};

enum {
    BERRY_BLUE,
    BERRY_GREEN,
    BERRY_GOLD,
    BERRY_MISSED,
    BERRY_PRIZE,
    BERRY_IN_ROW,
    NUM_BERRY_IDS
};

#define NUM_BERRY_TYPES  4 // Blue, Green, Gold, and 'missed'

 // Eaten anim comes after the normal & missed versions of other berries
#define ANIM_EATEN (BERRY_MISSED * 2)

enum {
    BERRYSTATE_NONE,
    BERRYSTATE_PICKED,   // Berry has been picked by a Dodrio, replaced with blue hit sprite (still falling)
    BERRYSTATE_EATEN,    // Berry has been eaten (after being picked), berry is gone now
    BERRYSTATE_SQUISHED, // Berry has hit the ground
};

enum {
    INPUTSTATE_NONE,
    INPUTSTATE_TRY_PICK,
    INPUTSTATE_PICKED,
    INPUTSTATE_ATE_BERRY,
    INPUTSTATE_BAD_MISS,
};

// Colors for status bar squares
// Colored gray when a berry is missed
// Flash red when few yellow squares remain
enum {
    STATUS_YELLOW,
    STATUS_GRAY,
    STATUS_RED,
};

#define NUM_STATUS_SQUARES 10

// Berries fall in predefined columns.
// A total of 10 are available, though fewer will be used with < 5 players
// The 11th column is a repeat of the 1st column wrapped around, so only
// the values 0-9 are unique 'valid' columns
#define NUM_BERRY_COLUMNS 11

#define GFXTAG_DODRIO    0
#define GFXTAG_STATUS    1
#define GFXTAG_BERRIES   2
#define GFXTAG_CLOUD     5
#define GFXTAG_COUNTDOWN 7

#define PALTAG_DODRIO_NORMAL 0
#define PALTAG_DODRIO_SHINY  1
#define PALTAG_STATUS        2
#define PALTAG_BERRIES       3
#define PALTAG_CLOUD         6
#define PALTAG_COUNTDOWN     8

#define NUM_CLOUDS 2

#define PLAYER_NONE 0xFF

struct DodrioGame_Gfx
{
    u16 ALIGNED(4) tilemapBuffers[3][BG_SCREEN_SIZE];
    bool32 finished;
    u8 ALIGNED(4) taskId;
    u8 ALIGNED(4) windowIds[MAX_RFU_PLAYERS + 5]; // The latter 5 are never used
    u8 ALIGNED(4) state;
    u8 ALIGNED(4) loadState;
    u16 ALIGNED(4) timer;
    u8 ALIGNED(4) cursorSelection;
    u8 ALIGNED(4) playAgainState;
    void (*func)(void);
}; // size = 0x302C

struct StatusBar
{
    u8 unused[12];
    bool8 entered[NUM_STATUS_SQUARES];
    s16 yChange[NUM_STATUS_SQUARES];
    u16 spriteIds[NUM_STATUS_SQUARES];
    u16 flashTimer;
}; // size = 0x40

struct DodrioGame_Berries
{
    u8 ids[NUM_BERRY_COLUMNS];
    u8 fallDist[NUM_BERRY_COLUMNS];
};

struct DodrioGame_PlayerCommData
{
    u8 pickState;
    bool8 ALIGNED(4) ateBerry;
    bool8 ALIGNED(4) missedBerry;
};

struct DodrioGame_Player
{
    u8 name[16];
    bool32 receivedGameStatePacket; // Never read
    struct DodrioGame_Berries berries;
    struct DodrioGame_PlayerCommData comm;
    u32 unused;
}; // size = 0x3C

// Because Dodrio is required for this minigame,
// the only relevant information about the selected
// Pokémon is whether or not it's shiny
struct DodrioGame_MonInfo
{
    bool8 isShiny;
};

struct DodrioGame_ScoreResults
{
    u8 ranking;
    u32 score;
};

struct DodrioGame
{
    /*0x0000*/ void (*exitCallback)(void);
    /*0x0004*/ u8 ALIGNED(4) taskId;
    /*0x0008*/ u8 ALIGNED(4) playersReceived;
    /*0x000C*/ u8 ALIGNED(4) startState;
    /*0x0010*/ u8 ALIGNED(4) state;
    /*0x0014*/ u8 ALIGNED(4) timer;
    /*0x0018*/ u8 ALIGNED(4) funcId;
    /*0x001C*/ u8 ALIGNED(4) prevFuncId; // Set, never read
    /*0x0020*/ bool8 ALIGNED(4) isLeader;
    /*0x0024*/ u8 ALIGNED(4) numPlayers;
    /*0x0028*/ u8 ALIGNED(4) multiplayerId;
    /*0x0029*/ u8 unused1[7];
    /*0x0030*/ u8 ALIGNED(4) countdownEndDelay;
    /*0x0034*/ u8 ALIGNED(4) posToPlayerId[MAX_RFU_PLAYERS];
    /*0x003C*/ u8 ALIGNED(4) unused2; // Set to 0, never read
    /*0x0040*/ u8 ALIGNED(4) numGraySquares;
    /*0x0044*/ u8 ALIGNED(4) berryColStart;
    /*0x0048*/ u8 ALIGNED(4) berryColEnd;
    /*0x004A*/ u16 berryResults[MAX_RFU_PLAYERS][NUM_BERRY_IDS];
    /*0x0086*/ u16 berriesEaten[MAX_RFU_PLAYERS];
    /*0x0090*/ u8 ALIGNED(4) difficulty[MAX_RFU_PLAYERS];
    /*0x0098*/ u8 ALIGNED(4) pickStateQueue[4];
    /*0x009C*/ u8 ALIGNED(4) eatTimer[NUM_BERRY_COLUMNS];
    /*0x00A8*/ u8 ALIGNED(4) inputState[MAX_RFU_PLAYERS];
    /*0x00B0*/ u8 ALIGNED(4) inputDelay[MAX_RFU_PLAYERS];
    /*0x00B8*/ u8 ALIGNED(4) berryEatenBy[NUM_BERRY_COLUMNS];
    /*0x00C4*/ u8 ALIGNED(4) berryState[NUM_BERRY_COLUMNS];
    /*0x00D0*/ u8 ALIGNED(4) fallTimer[NUM_BERRY_COLUMNS];
    /*0x00DC*/ u8 ALIGNED(4) newBerryTimer[NUM_BERRY_COLUMNS];
    /*0x00E8*/ u8 ALIGNED(4) prevBerryIds[NUM_BERRY_COLUMNS];
    /*0x00F4*/ u8 ALIGNED(4) playersAttemptingPick[NUM_BERRY_COLUMNS][2];
    /*0x010C*/ u8 ALIGNED(4) playAgainStates[MAX_RFU_PLAYERS];
    /*0x0112*/ u16 berriesPickedInRow;
    /*0x0114*/ u16 maxBerriesPickedInRow;
    /*0x0118*/ bool32 startCountdown; // Never read
    /*0x011C*/ bool32 startGame;
    /*0x0120*/ bool32 berriesFalling;
    /*0x0124*/ u8 ALIGNED(4) clearRecvCmdTimer;
    /*0x0128*/ bool8 ALIGNED(4) clearRecvCmds;
    /*0x012C*/ bool32 allReadyToEnd;
    /*0x0130*/ bool32 readyToEnd[MAX_RFU_PLAYERS];
    /*0x0144*/ bool8 ALIGNED(4) playingPickSound;
    /*0x0148*/ bool8 ALIGNED(4) playingSquishSound[NUM_BERRY_COLUMNS];
    /*0x0154*/ u8 ALIGNED(4) endSoundState;
    /*0x0158*/ bool8 ALIGNED(4) readyToStart[MAX_RFU_PLAYERS];
    /*0x0160*/ struct DodrioGame_Gfx gfx;
    /*0x318C*/ struct DodrioGame_MonInfo monInfo[MAX_RFU_PLAYERS];
    /*0x31A0*/ struct DodrioGame_Player players[MAX_RFU_PLAYERS];
    /*0x32CC*/ struct DodrioGame_Player player;
    /*0x3308*/ struct DodrioGame_ScoreResults scoreResults[MAX_RFU_PLAYERS];
}; // size = 0x3330

EWRAM_DATA static struct DodrioGame * sGame = NULL;
EWRAM_DATA static u16 * sDodrioSpriteIds[MAX_RFU_PLAYERS] = {NULL};
EWRAM_DATA static u16 * sCloudSpriteIds[NUM_CLOUDS] = {NULL};
EWRAM_DATA static u16 * sBerrySpriteIds[NUM_BERRY_COLUMNS] = {NULL};
EWRAM_DATA static u16 * sBerryIconSpriteIds[NUM_BERRY_TYPES] = {NULL};
EWRAM_DATA static struct StatusBar * sStatusBar = NULL;
EWRAM_DATA static struct DodrioGame_Gfx * sGfx = NULL;

static bool32 sExitingGame;

static void ResetTasksAndSprites(void);
static void InitDodrioGame(struct DodrioGame *);
static void Task_StartDodrioGame(u8);
static void DoGameIntro(void);
static void InitCountdown(void);
static void DoCountdown(void);
static void WaitGameStart(void);
static void PlayGame_Leader(void);
static void PlayGame_Member(void);
static void WaitEndGame_Leader(void);
static void WaitEndGame_Member(void);
static void InitResults_Leader(void);
static void InitResults_Member(void);
static void DoResults(void);
static void AskPlayAgain(void);
static void EndLink(void);
static void ExitGame(void);
static void ResetGame(void);
static void Task_NewGameIntro(u8);
static void Task_CommunicateMonInfo(u8);
static void RecvLinkData_Leader(void);
static void SendLinkData_Leader(void);
static void RecvLinkData_Member(void);
static void SendLinkData_Member(void);
static void HandleSound_Leader(void);
static void HandleSound_Member(void);
static void CB2_DodrioGame(void);
static void VBlankCB_DodrioGame(void);
static void InitMonInfo(struct DodrioGame_MonInfo *, struct Pokemon *);
static void CreateTask_(TaskFunc, u8);
static void CreateDodrioGameTask(TaskFunc);
static void SetGameFunc(u8);
static bool32 SlideTreeBordersOut(void);
static void InitFirstWaveOfBerries(void);
static bool32 TryPickBerry(u8, u8, u8);
static void UpdateFallingBerries(void);
static void UpdateGame_Leader(void);
static void UpdateGame_Member(void);
static void GetActiveBerryColumns(u8, u8 *, u8 *);
static bool32 AllPlayersReadyToStart(void);
static void ResetReadyToStart(void);
static bool32 ReadyToEndGame_Leader(void);
static bool32 ReadyToEndGame_Member(void);
static void TryIncrementDifficulty(u8);
static u8 GetPlayerIdAtColumn(u8);
static u8 GetNewBerryId(u8, u8);
static void IncrementBerryResult(u8, u8, u8);
static void UpdateBerriesPickedInRow(bool32);
static void SetMaxBerriesPickedInRow(void);
static void ResetForPlayAgainPrompt(void);
static void SetRandomPrize(void);
static void TryUpdateRecords(void);
static u8 UpdatePickStateQueue(u8);
static void HandleWaitPlayAgainInput(void);
static void ResetPickState(void);
static u32 GetHighestScore(void);
static void SendPacket_ReadyToStart(bool32);
static void SendPacket_GameState(struct DodrioGame_Player *,
                                 struct DodrioGame_PlayerCommData *,
                                 struct DodrioGame_PlayerCommData *,
                                 struct DodrioGame_PlayerCommData *,
                                 struct DodrioGame_PlayerCommData *,
                                 struct DodrioGame_PlayerCommData *,
                                 u8 , bool32 , bool32 );
static bool32 RecvPacket_GameState(u32,
                                   struct DodrioGame_Player *,
                                   struct DodrioGame_PlayerCommData *,
                                   struct DodrioGame_PlayerCommData *,
                                   struct DodrioGame_PlayerCommData *,
                                   struct DodrioGame_PlayerCommData *,
                                   struct DodrioGame_PlayerCommData *,
                                   u8 *, bool32 *, bool32 *);
static void SendPacket_PickState(u8);
static bool32 RecvPacket_PickState(u32, u8 *);
static void SendPacket_ReadyToEnd(bool32);
static bool32 RecvPacket_ReadyToEnd(u32);
static void LoadDodrioGfx(void);
static void CreateDodrioSprite(struct DodrioGame_MonInfo *, u8, u8, u8);
static void StartDodrioMissedAnim(u8);
static void StartDodrioIntroAnim(u8);
static void FreeDodrioSprites(u8);
static void SetAllDodrioInvisibility(bool8, u8);
static void CreateStatusBarSprites(void);
static void FreeStatusBar(void);
static void SetStatusBarInvisibility(bool8);
static void InitStatusBarPos(void);
static bool32 DoStatusBarIntro(void);
static void LoadBerryGfx(void);
static void CreateBerrySprites(void);
static void FreeBerrySprites(void);
static void CreateCloudSprites(void);
static void ResetCloudPos(void);
static void StartCloudMovement(void);
static void FreeCloudSprites(void);
static void SetCloudInvisibility(bool8);
static void ResetBerryAndStatusBarSprites(void);
static void ResetGfxState(void);
static void InitGameGfx(struct DodrioGame_Gfx *);
static void SetGfxFuncById(u8);
static bool32 IsGfxFuncActive(void);
static u8 GetPlayAgainState(void);
static void SetBerryInvisibility(u8, bool8);
static void SetBerryIconsInvisibility(bool8);
static void SetBerryAnim(u16, u8);
static void SetBerryYPos(u8, u8);
static void SetDodrioAnim(u8, u8);
static u8 GetNewBerryIdByDifficulty(u8, u8);
static void UpdateStatusBarAnim(u8);
static u32 RecvPacket_ReadyToStart(u32);
static u32 IncrementWithLimit(u32, u32);
static u32 Min(u32, u32);
static u32 GetScore(u8);
static void Task_ShowDodrioBerryPickingRecords(u8);
static void Task_TryRunGfxFunc(u8);
static void PrintRecordsText(u8, s32);
static void SpriteCB_Status(struct Sprite *);
static void SpriteCB_Dodrio(struct Sprite *);
static u32 DoDodrioMissedAnim(struct Sprite *);
static u32 DoDodrioIntroAnim(struct Sprite *);
static s16 GetDodrioXPos(u8, u8);
static void SetDodrioInvisibility(bool8, u8);
static void LoadGfx(void);
static bool32 LoadBgGfx(void);
static void InitBgs(void);
static void SetGfxFunc(void (*func)(void));
static void (*GetGfxFunc(void))(void);
static void ShowNames(void);
static void ShowResults(void);
static void Msg_WantToPlayAgain(void);
static void Msg_SavingDontTurnOff(void);
static void Msg_CommunicationStandby(void);
static void EraseMessage(void);
static void Msg_SomeoneDroppedOut(void);
static void StopGfxFuncs(void);
static void GfxIdle(void);

// For each player, the array is a list of all the columns starting with the column to their left
// Only the range of active columns is read from the array (dependent on the number of players),
// so the arrays are spaced such that the numbers in the center are where the data that's read starts and end.
static const u8 sActiveColumnMap[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][NUM_BERRY_COLUMNS] =
{
    { // 1 player (never used), columns 4-6.
      // Sometimes read to get default order regardless of the current number of players
        {0, 1, 2, 3,     4, 5, 6,     7, 8, 9, 0},
    },
    { // 2 players (never used), columns 3-6
        {0, 1, 2,     3, 4, 5, 6, 3,     8, 9, 0},
        {0, 1, 2,     5, 6, 3, 4, 5,     8, 9, 0},
    },
    { // 3 players, columns 2-7
        {0, 1,     2, 3, 4, 5, 6, 7, 2,     9, 0},
        {0, 1,     4, 5, 6, 7, 2, 3, 4,     9, 0},
        {0, 1,     6, 7, 2, 3, 4, 5, 6,     9, 0},
    },
    { // 4 players, columns 1-8
        {0,     1, 2, 3, 4, 5, 6, 7, 8, 1,     0},
        {0,     3, 4, 5, 6, 7, 8, 1, 2, 3,     0},
        {0,     5, 6, 7, 8, 1, 2, 3, 4, 5,     0},
        {0,     7, 8, 1, 2, 3, 4, 5, 6, 7,     0},
    },
    { // 5 players, all columns (0-9)
        {    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0    },
        {    2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2    },
        {    4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4    },
        {    6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6    },
        {    8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8    },
    },
};

// A table for which falling berry column corresponds to which Dodrio head for each player
// The numbers in each array are the column number for each head, {left, middle, right}
// Dependent on the number of players
static const u8 sDodrioHeadToColumnMap[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][3] =
{
    { // 1 player (never used)
        {4, 5, 6},
    },
    { // 2 players (never used)
        {3, 4, 5},
        {5, 6, 3},
    },
    { // 3 players
        {4, 5, 6},
        {6, 7, 2},
        {2, 3, 4},
    },
    { // 4 players
        {3, 4, 5},
        {5, 6, 7},
        {7, 8, 1},
        {1, 2, 3},
    },
    { // 5 players
        {4, 5, 6},
        {6, 7, 8},
        {8, 9, 0},
        {0, 1, 2},
        {2, 3, 4},
    },
};

// A table of player ids and their neighbor, dependent on the total number of players
// {L, M, R}, where M is the player in question, L is their neighbor to the left, and R is their neighbor to the right
static const u8 sDodrioNeighborMap[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][3] =
{
    { // 1 player (never used)
        {1, 0, 1},
    },
    { // 2 players (never used)
        {1, 0, 1},
        {0, 1, 0},
    },
    { // 3 players
        {2, 0, 1},
        {0, 1, 2},
        {1, 2, 0},
    },
    { // 4 players
        {3, 0, 1},
        {0, 1, 2},
        {1, 2, 3},
        {2, 3, 0},
    },
    { // 5 players
        {4, 0, 1},
        {0, 1, 2},
        {1, 2, 3},
        {2, 3, 4},
        {3, 4, 0},
    },
};

#define x 9 // No player at this column. This may go out of bounds if this is returned

// Takes the number of players and a column and returns the player id at that column.
// Note that the assignment is somewhat arbitrary as players share neighboring columns.
ALIGNED(4)
static const u8 sPlayerIdAtColumn[MAX_RFU_PLAYERS][NUM_BERRY_COLUMNS] =
{
    {x, x, x, x, 1, 1, 1, x, x, x, x}, // 1 player
    {x, x, x, 0, 0, 1, 1, 0, x, x, x}, // 2 players
    {x, x, 2, 2, 0, 0, 1, 1, 1, x, x}, // 3 players
    {x, 3, 3, 0, 0, 1, 1, 2, 2, 3, x}, // 4 players
    {3, 3, 4, 4, 0, 0, 1, 1, 2, 2, 3}, // 5 players
};

#undef x

// Each array contains the columns that belong solely to one player, dependent on the number of players
// When determing how difficult the berries in a column should be, the highest
// difficulty of the players sharing that column is used.
// This table is used to skip that check, and instead automatically use the
// difficulty of the only player who can use the column.
static const u8 sUnsharedColumns[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS] =
{
    {5},
    {4, 6},
    {3, 5, 7},
    {2, 4, 6, 8},
#ifndef BUGFIX
    {1, 3, 5, 6, 9}, // BUG: Column 6 is shared, 7 is not. As a result, the player in column 7 will have their difficulty influenced by their neighbors
#else
    {1, 3, 5, 7, 9},
#endif
};

// Duplicate and unused gfx. Feel free to remove.
static const u32 sDuplicateGfx[] = INCBIN_U32("graphics/dodrio_berry_picking/tree_border.gbapal",
                                     "graphics/dodrio_berry_picking/bg.gbapal",
                                     "graphics/dodrio_berry_picking/dodrio.gbapal",
                                     "graphics/dodrio_berry_picking/shiny.gbapal",
                                     "graphics/dodrio_berry_picking/status.gbapal",
                                     "graphics/dodrio_berry_picking/berries.gbapal",
                                     "graphics/dodrio_berry_picking/berries.4bpp.lz",
                                     "graphics/dodrio_berry_picking/cloud.gbapal",
                                     "graphics/dodrio_berry_picking/bg.4bpp.lz",
                                     "graphics/dodrio_berry_picking/tree_border.4bpp.lz",
                                     "graphics/dodrio_berry_picking/status.4bpp.lz",
                                     "graphics/dodrio_berry_picking/cloud.4bpp.lz",
                                     "graphics/dodrio_berry_picking/dodrio.4bpp.lz",
                                     "graphics/dodrio_berry_picking/bg.bin.lz",
                                     "graphics/dodrio_berry_picking/tree_border_right.bin.lz",
                                     "graphics/dodrio_berry_picking/tree_border_left.bin.lz");


static const u8 sBerryFallDelays[][3] =
{
    { [BERRY_BLUE] = 40, [BERRY_GREEN] = 24, [BERRY_GOLD] = 13 },
    { [BERRY_BLUE] = 32, [BERRY_GREEN] = 19, [BERRY_GOLD] = 10 },
    { [BERRY_BLUE] = 22, [BERRY_GREEN] = 13, [BERRY_GOLD] =  7 },
};

// How far the outer tree borders should slide to reveal the game screen.
// Dependent on how many players are playing.
// Curiously the 2-player screen is narrower than the 1-player, though neither
// gets used as there's a 3 player minimum
ALIGNED(4)
static const u8 sTreeBorderXPos[MAX_RFU_PLAYERS] = {8, 5, 8, 11, 15};

// The number of berries eaten needed to progress to the next difficulty
ALIGNED(4)
static const u8 sDifficultyThresholds[NUM_DIFFICULTIES] = {5, 10, 20, 30, 50, 70, 100};

ALIGNED(4)
static const u8 sPrizeBerryIds[][10] =
{
    { // Possible prizes with 3 players
        ITEM_TO_BERRY(ITEM_RAZZ_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_BLUK_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_NANAB_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_WEPEAR_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_PINAP_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_PINAP_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_WEPEAR_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_NANAB_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_BLUK_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_RAZZ_BERRY) - 1
    },
    { // Possible prizes with 4 players
        ITEM_TO_BERRY(ITEM_POMEG_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_KELPSY_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_QUALOT_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_HONDEW_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_GREPA_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_TAMATO_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_CORNN_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_MAGOST_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_RABUTA_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_NOMEL_BERRY) - 1
    },
    { // Possible prizes with 5 players
        ITEM_TO_BERRY(ITEM_SPELON_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_PAMTRE_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_WATMEL_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_DURIN_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_BELUE_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_BELUE_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_DURIN_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_WATMEL_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_PAMTRE_BERRY) - 1,
        ITEM_TO_BERRY(ITEM_SPELON_BERRY) - 1
    },
};

static void (*const sLeaderFuncs[])(void) =
{
    [FUNC_INTRO]          = DoGameIntro,
    [FUNC_INIT_COUNTDOWN] = InitCountdown,
    [FUNC_COUNTDOWN]      = DoCountdown,
    [FUNC_WAIT_START]     = WaitGameStart,
    [FUNC_PLAY_GAME]      = PlayGame_Leader,
    [FUNC_INIT_RESULTS]   = InitResults_Leader,
    [FUNC_RESULTS]        = DoResults,
    [FUNC_ASK_PLAY_AGAIN] = AskPlayAgain,
    [FUNC_END_LINK]       = EndLink,
    [FUNC_EXIT]           = ExitGame,
    [FUNC_RESET_GAME]     = ResetGame,
    [FUNC_WAIT_END_GAME]  = WaitEndGame_Leader
};

static void (*const sMemberFuncs[])(void) =
{
    [FUNC_INTRO]          = DoGameIntro,
    [FUNC_INIT_COUNTDOWN] = InitCountdown,
    [FUNC_COUNTDOWN]      = DoCountdown,
    [FUNC_WAIT_START]     = WaitGameStart,
    [FUNC_PLAY_GAME]      = PlayGame_Member,
    [FUNC_INIT_RESULTS]   = InitResults_Member,
    [FUNC_RESULTS]        = DoResults,
    [FUNC_ASK_PLAY_AGAIN] = AskPlayAgain,
    [FUNC_END_LINK]       = EndLink,
    [FUNC_EXIT]           = ExitGame,
    [FUNC_RESET_GAME]     = ResetGame,
    [FUNC_WAIT_END_GAME]  = WaitEndGame_Member
};

void StartDodrioBerryPicking(u16 partyId, void (*exitCallback)(void))
{
    sExitingGame = FALSE;

    if (gReceivedRemoteLinkPlayers && (sGame = AllocZeroed(sizeof(*sGame))))
    {
        ResetTasksAndSprites();
        InitDodrioGame(sGame);
        sGame->exitCallback = exitCallback;
        sGame->multiplayerId = GetMultiplayerId();
        sGame->player = sGame->players[sGame->multiplayerId];
        InitMonInfo(&sGame->monInfo[sGame->multiplayerId], &gPlayerParty[partyId]);
        CreateTask(Task_StartDodrioGame, 1);
        SetMainCallback2(CB2_DodrioGame);
        SetRandomPrize();
        GetActiveBerryColumns(sGame->numPlayers, &sGame->berryColStart, &sGame->berryColEnd);
        StopMapMusic();
        PlayNewMapMusic(MUS_RG_BERRY_PICK);
    }
    else
    {
        // Exit - Alloc failed, or players not connected
        SetMainCallback2(exitCallback);
        return;
    }
}

static void ResetTasksAndSprites(void)
{
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
}

static void InitDodrioGame(struct DodrioGame * game)
{
    u8 i;

    game->startState = 0;
    game->state = 0;
    game->timer = 0;
    game->funcId = FUNC_INTRO;
    game->prevFuncId = FUNC_INTRO;
    game->startGame = FALSE;
    game->berriesFalling = FALSE;
    game->countdownEndDelay = 0;
    game->numGraySquares = 0;
    game->unused2 = 0;
    game->allReadyToEnd = FALSE;

    for (i = 0; i < ARRAY_COUNT(game->pickStateQueue); i++)
        game->pickStateQueue[i] = PICK_NONE;

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        game->inputState[i] = INPUTSTATE_NONE;
        game->inputDelay[i] = 0;
        game->berryResults[i][BERRY_BLUE] = 0;
        game->berryResults[i][BERRY_GREEN] = 0;
        game->berryResults[i][BERRY_GOLD] = 0;
        game->berryResults[i][BERRY_MISSED] = 0;
        game->berryResults[i][BERRY_IN_ROW] = 0;
        game->playAgainStates[i] = PLAY_AGAIN_NONE;
        game->readyToEnd[i] = FALSE;
    }

    for (i = 0; i < NUM_BERRY_COLUMNS; i++)
    {
        game->fallTimer[i] = 0;
        game->newBerryTimer[i] = 0;
        game->berryState[i] = BERRYSTATE_NONE;
        game->playersAttemptingPick[i][0] = PLAYER_NONE;
        game->playersAttemptingPick[i][1] = PLAYER_NONE;
    }

    game->isLeader = GetMultiplayerId() == 0 ? TRUE : FALSE;
    game->numPlayers = GetLinkPlayerCount();
    game->posToPlayerId[0] = GetMultiplayerId();
    for (i = 1; i < game->numPlayers; i++)
    {
        game->posToPlayerId[i] = game->posToPlayerId[i - 1] + 1;
        if (game->posToPlayerId[i] > game->numPlayers - 1)
            game->posToPlayerId[i] %= game->numPlayers;
    }
}

static void Task_StartDodrioGame(u8 taskId)
{
    u8 i, numPlayers;

    switch (sGame->startState)
    {
    case 0:
        SetVBlankCallback(NULL);
        CreateTask_(Task_CommunicateMonInfo, 4);
        sGame->startState++;
        break;
    case 1:
        if (!FuncIsActiveTask(Task_CommunicateMonInfo))
        {
            InitGameGfx(&sGame->gfx);
            sGame->startState++;
        }
        break;
    case 2:
        if (!IsGfxFuncActive())
        {
            Rfu_SetLinkStandbyCallback();
            sGame->startState++;
        }
        break;
    case 3:
        if (IsLinkTaskFinished())
        {
            if (gReceivedRemoteLinkPlayers)
            {
                LoadWirelessStatusIndicatorSpriteGfx();
                CreateWirelessStatusIndicatorSprite(0, 0);
            }
            sGame->startState++;
        }
        break;
    case 4:
        numPlayers = sGame->numPlayers;
        LoadDodrioGfx();
        for (i = 0; i < numPlayers; i++)
            CreateDodrioSprite(&sGame->monInfo[sGame->posToPlayerId[i]], i, sGame->posToPlayerId[i], sGame->numPlayers);

        SetAllDodrioInvisibility(FALSE, sGame->numPlayers);
        sGame->startState++;
        break;
    case 5:
        LoadBerryGfx();
        CreateBerrySprites();
        CreateCloudSprites();
        CreateStatusBarSprites();
        sGame->startState++;
        break;
    case 6:
        BlendPalettes(PALETTES_ALL, 0x10, 0x00);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, 0);
        SetVBlankCallback(VBlankCB_DodrioGame);
        sGame->startState++;
        break;
    case 7:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
            sGame->startState++;
        break;
    default:
        DestroyTask(taskId);
        CreateDodrioGameTask(Task_NewGameIntro);
        break;
    }
}

static void Task_DodrioGame_Leader(u8 taskId)
{
    RecvLinkData_Leader();
    sLeaderFuncs[sGame->funcId]();
    if (!sExitingGame)
        UpdateGame_Leader();

    SendLinkData_Leader();
}

static void Task_DodrioGame_Member(u8 taskId)
{
    RecvLinkData_Member();
    sMemberFuncs[sGame->funcId]();
    if (!sExitingGame)
        UpdateGame_Member();

    SendLinkData_Member();
}

static void DoGameIntro(void)
{
    switch (sGame->state)
    {
    case 0:
        StartDodrioIntroAnim(1);
        SetGfxFuncById(GFXFUNC_SHOW_NAMES);
        sGame->state++;
        break;
    case 1:
        if (!IsGfxFuncActive())
            SetGameFunc(FUNC_INIT_COUNTDOWN);
        break;
    }
}

static void InitCountdown(void)
{
    switch (sGame->state)
    {
    case 0:
        InitFirstWaveOfBerries();
        sGame->state++;
        break;
    default:
        sGame->startCountdown = TRUE;
        SetGameFunc(FUNC_COUNTDOWN);
        break;
    }
}

static void DoCountdown(void)
{
    switch (sGame->state)
    {
    case 0:
        StartMinigameCountdown(GFXTAG_COUNTDOWN, PALTAG_COUNTDOWN, 120, 80, 0);
        sGame->state++;
        break;
    case 1:
        Rfu_SetLinkStandbyCallback();
        sGame->state++;
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            sGame->state++;
            sGame->countdownEndDelay = 0;
        }
        break;
    case 3:
        if (!IsMinigameCountdownRunning())
            sGame->state++;
        break;
    case 4:
        if (++sGame->countdownEndDelay > 5)
        {
            Rfu_SetLinkStandbyCallback();
            sGame->state++;
        }
        break;
    case 5:
        if (IsLinkTaskFinished())
            SetGameFunc(FUNC_WAIT_START);
        break;
    }
}

static void WaitGameStart(void)
{
    switch (sGame->state)
    {
    case 0:
        if (sGame->startGame)
            SetGameFunc(FUNC_PLAY_GAME);
        break;
    }
}

static void PlayGame_Leader(void)
{
    switch (sGame->state)
    {
    case 0:
        if (sGame->numGraySquares < NUM_STATUS_SQUARES)
        {
            if (sGame->inputState[0] == INPUTSTATE_NONE)
            {
                if (JOY_NEW(DPAD_UP))
                {
                    if (sGame->players[0].comm.pickState == PICK_NONE)
                    {
                        sGame->players[0].comm.ateBerry = FALSE;
                        sGame->players[0].comm.pickState = UpdatePickStateQueue(PICK_MIDDLE);
                    }
                }
                else if (JOY_NEW(DPAD_RIGHT))
                {
                    if (sGame->players[0].comm.pickState == PICK_NONE)
                    {
                        sGame->players[0].comm.ateBerry = FALSE;
                        sGame->players[0].comm.pickState = UpdatePickStateQueue(PICK_RIGHT);
                    }
                }
                else if (JOY_NEW(DPAD_LEFT))
                {
                    if (sGame->players[0].comm.pickState == PICK_NONE)
                    {
                        sGame->players[0].comm.ateBerry = FALSE;
                        sGame->players[0].comm.pickState = UpdatePickStateQueue(PICK_LEFT);
                    }
                }
                else
                {
                    sGame->players[0].comm.pickState = UpdatePickStateQueue(PICK_NONE);
                }
            }
        }
        else
        {
            SetGameFunc(FUNC_WAIT_END_GAME);
        }
        UpdateFallingBerries();
        HandleSound_Leader();
        break;
    }
}

static void PlayGame_Member(void)
{
    if (sGame->numGraySquares < NUM_STATUS_SQUARES)
    {
        if (JOY_NEW(DPAD_UP))
        {
            if (sGame->players[sGame->multiplayerId].comm.pickState == PICK_NONE)
            {
                sGame->player.comm.pickState = PICK_MIDDLE;
            }
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            if (sGame->players[sGame->multiplayerId].comm.pickState == PICK_NONE)
            {
                sGame->player.comm.pickState = PICK_RIGHT;
            }
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            if (sGame->players[sGame->multiplayerId].comm.pickState == PICK_NONE)
            {
                sGame->player.comm.pickState = PICK_LEFT;
            }
        }
        else
        {
            sGame->player.comm.pickState = PICK_NONE;
        }
    }
    else
    {
        SetGameFunc(FUNC_WAIT_END_GAME);
    }
    HandleSound_Member();
}

static void WaitEndGame_Leader(void)
{
    u8 i;

    UpdateFallingBerries();
    HandleSound_Leader();
    if (ReadyToEndGame_Leader() == TRUE)
    {
        SetMaxBerriesPickedInRow();
        SetGameFunc(FUNC_INIT_RESULTS);
    }
    else
    {
        sGame->allReadyToEnd = TRUE;
        for (i = 1; i < sGame->numPlayers; i++)
        {
            if (sGame->readyToEnd[i] != TRUE)
            {
                sGame->allReadyToEnd = FALSE;
                break;
            }
        }
    }
}

static void WaitEndGame_Member(void)
{
    HandleSound_Member();
    if (ReadyToEndGame_Member() == TRUE)
        SetGameFunc(FUNC_INIT_RESULTS);
}

static bool32 AllLinkBlocksReceived(void)
{
    u8 recvStatus = GetBlockReceivedStatus();
    u8 playerFlags = GetLinkPlayerCountAsBitFlags();
    if (recvStatus == playerFlags)
    {
        ResetBlockReceivedFlags();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void InitResults_Leader(void)
{
    switch (sGame->state)
    {
    case 0:
        if (SendBlock(0, sGame->berryResults, sizeof(sGame->berryResults)))
        {
            sGame->playersReceived = 0;
            sGame->state++;
        }
        break;
    case 1:
        if (IsLinkTaskFinished())
        {
            sGame->state++;
        }
        break;
    case 2:
        if (AllLinkBlocksReceived())
        {
            sGame->playersReceived = sGame->numPlayers;
        }
        if (sGame->playersReceived >= sGame->numPlayers)
        {
            sGame->timer++;
            sGame->state++;
        }
        break;
    default:
        if (WaitFanfare(TRUE))
        {
            SetGameFunc(FUNC_RESULTS);
            FadeOutAndPlayNewMapMusic(MUS_RG_VICTORY_WILD, 4);
        }
        break;
    }
}

static void InitResults_Member(void)
{
    u8 i;

    switch (sGame->state)
    {
    case 0:
        if (SendBlock(0, sGame->berryResults[sGame->timer], sizeof(sGame->berryResults)))
        {
            sGame->playersReceived = 0;
            sGame->state++;
        }
        break;
    case 1:
        if (IsLinkTaskFinished())
        {
            sGame->state++;
        }
        break;
    case 2:
        if (AllLinkBlocksReceived())
        {
            for (i = 0; i < sGame->numPlayers; i++)
            {
                memcpy(sGame->berryResults, gBlockRecvBuffer, sizeof(sGame->berryResults));
                sGame->playersReceived = sGame->numPlayers;
            }
        }
        if (sGame->playersReceived >= sGame->numPlayers)
        {
            sGame->timer++;
            sGame->state++;
        }
        break;
    default:
        if (WaitFanfare(TRUE))
        {
            sGame->maxBerriesPickedInRow = sGame->berryResults[sGame->multiplayerId][BERRY_IN_ROW];
            SetGameFunc(FUNC_RESULTS);
            FadeOutAndPlayNewMapMusic(MUS_RG_VICTORY_WILD, 4);
        }
        break;
    }
}

static void DoResults(void)
{
    u8 playAgainState = PLAY_AGAIN_YES;
    u8 i;

    switch (sGame->state)
    {
    case 0:
        TryUpdateRecords();
        SetStatusBarInvisibility(TRUE);
        ResetCloudPos();
        SetCloudInvisibility(TRUE);
        SetGfxFuncById(GFXFUNC_SHOW_RESULTS);
        sGame->state++;
        break;
    case 1:
        if (!IsGfxFuncActive())
        {
            SetGfxFuncById(GFXFUNC_MSG_COMM_STANDBY);
            sGame->state++;
        }
        break;
    case 2:
        playAgainState = GetPlayAgainState();
        if (SendBlock(0, &playAgainState, sizeof(playAgainState)))
            sGame->state++;
        break;
    case 3:
        if (IsLinkTaskFinished())
        {
            sGame->state++;
            sGame->playersReceived = 0;
        }
        break;
    case 4:
        if (AllLinkBlocksReceived())
        {
            for (i = 0; i < sGame->numPlayers; i++)
            {
                *(&sGame->playAgainStates[i]) = *(u8 *)gBlockRecvBuffer[i];
                sGame->playersReceived = sGame->numPlayers;
            }
        }
        if (sGame->playersReceived >= sGame->numPlayers)
        {
            if (++sGame->timer >= 120)
            {
                SetGfxFuncById(GFXFUNC_ERASE_MSG);
                sGame->state++;
            }
        }
        break;
    default:
        if (!IsGfxFuncActive())
            SetGameFunc(FUNC_ASK_PLAY_AGAIN);
        break;
    }
}

static void AskPlayAgain(void)
{
    u8 playAgainState;
    u8 i;

    switch (sGame->state)
    {
    case 0:
        if (GetHighestScore() >= PRIZE_SCORE)
        {
            SetGfxFuncById(GFXFUNC_MSG_SAVING);
        }
        sGame->state++;
        break;
    case 1:
        if (!IsGfxFuncActive())
        {
            SetGfxFuncById(GFXFUNC_MSG_PLAY_AGAIN);
            sGame->state++;
        }
        break;
    case 2:
        ResetBerryAndStatusBarSprites();
        ResetForPlayAgainPrompt();
        sGame->state++;
        break;
    case 3:
        if ((playAgainState = GetPlayAgainState()) != PLAY_AGAIN_NONE)
        {
            sGame->state++;
        }
        break;
    case 4:
        if (!IsGfxFuncActive())
        {
            SetGfxFuncById(GFXFUNC_MSG_COMM_STANDBY);
            sGame->state++;
        }
        break;
    case 5:
        playAgainState = GetPlayAgainState();
        if (SendBlock(0, &playAgainState, sizeof(playAgainState)))
        {
            sGame->playersReceived = 0;
            sGame->state++;
        }
        break;
    case 6:
        if (IsLinkTaskFinished())
            sGame->state++;
        break;
    case 7:
        if (AllLinkBlocksReceived())
        {
            for (i = 0; i < sGame->numPlayers; i++)
            {
                *(&sGame->playAgainStates[i]) = *(u8 *)gBlockRecvBuffer[i];
                sGame->playersReceived = sGame->numPlayers;
            }
        }
        if (sGame->playersReceived >= sGame->numPlayers)
        {
            if (++sGame->timer >= 120)
            {
                ResetPickState();
                SetGfxFuncById(GFXFUNC_ERASE_MSG);
                sGame->state++;
            }
        }
        else
        {
            HandleWaitPlayAgainInput();
        }
        break;
    default:
        if (!IsGfxFuncActive())
        {
            for (i = 0; i < sGame->numPlayers; i++)
            {
                if (sGame->playAgainStates[i] == PLAY_AGAIN_NO)
                {
                    SetGameFunc(FUNC_END_LINK);
                    return;
                }
            }
            SetGameFunc(FUNC_RESET_GAME);
        }
        break;
    }
}

static void EndLink(void)
{
    switch (sGame->state)
    {
    case 0:
        SetCloseLinkCallback();
        SetGfxFuncById(GFXFUNC_MSG_PLAYER_DROPPED);
        sGame->state++;
        break;
    case 1:
        if (!IsGfxFuncActive())
            sGame->state++;
        break;
    case 2:
        if (GetPlayAgainState() == PLAY_AGAIN_DROPPED)
            sGame->state++;
        break;
    default:
        if (gReceivedRemoteLinkPlayers == 0)
        {
            SetGameFunc(FUNC_EXIT);
        }
        break;
    }
}

static void ExitGame(void)
{
    switch (sGame->state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, 0);
        sGame->state++;
        break;
    case 1:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
            sGame->state++;
        break;
    case 2:
        FreeBerrySprites();
        FreeStatusBar();
        FreeDodrioSprites(sGame->numPlayers);
        FreeCloudSprites();
        sExitingGame = TRUE;
        SetGfxFuncById(GFXFUNC_STOP);
        sGame->state++;
        break;
    default:
        if (!IsGfxFuncActive())
        {
            SetMainCallback2(sGame->exitCallback);
            DestroyTask(sGame->taskId);
            Free(sGame);
            FreeAllWindowBuffers();
        }
        break;
    }
}

static void ResetGame(void)
{
    switch (sGame->state)
    {
    case 0:
        SetGfxFuncById(GFXFUNC_IDLE);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, 0);
        sGame->state++;
        break;
    case 1:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            sGame->state++;
        }
        break;
    case 2:
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        sGame->state++;
        break;
    case 3:
        StopMapMusic();
        sGame->state++;
        break;
    case 4:
        PlayNewMapMusic(MUS_RG_BERRY_PICK);
        StartCloudMovement();
        sGame->state++;
        break;
    case 5:
        BlendPalettes(PALETTES_ALL, 16, 0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, 0);
        sGame->state++;
        break;
    case 6:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
            sGame->state++;
        break;
    default:
        DestroyTask(sGame->taskId);
        CreateDodrioGameTask(Task_NewGameIntro);
        ResetGfxState();
        InitDodrioGame(sGame);
        if (gReceivedRemoteLinkPlayers == 0)
            sGame->numPlayers = 1;

        SetRandomPrize();
        SetCloudInvisibility(FALSE);
        break;
    }
}

static void Task_NewGameIntro(u8 taskId)
{
    switch (sGame->state)
    {
    case 0:
        if (SlideTreeBordersOut() == TRUE)
            sGame->state++;
        break;
    case 1:
        InitStatusBarPos();
        sGame->state++;
        break;
    case 2:
        if (DoStatusBarIntro() == TRUE)
            sGame->state++;
        break;
    default:
        if (sGame->isLeader)
            CreateDodrioGameTask(Task_DodrioGame_Leader);
        else
            CreateDodrioGameTask(Task_DodrioGame_Member);

        DestroyTask(taskId);
        break;
    }
}

#define tState data[0]

static void Task_CommunicateMonInfo(u8 taskId)
{
    s16 * data = gTasks[taskId].data;
    u8 i;

    switch (tState)
    {
    case 0:
        if (SendBlock(0, &sGame->monInfo[sGame->multiplayerId].isShiny, sizeof(sGame->monInfo[sGame->multiplayerId].isShiny)))
        {
            sGame->playersReceived = 0;
            tState++;
        }
        break;
    case 1:
        if (IsLinkTaskFinished())
            tState++;
        break;
    case 2:
        if (AllLinkBlocksReceived())
        {
            for (i = 0; i < sGame->numPlayers; i++)
            {
                *(u8 *)&sGame->monInfo[i] = *(u8 *)gBlockRecvBuffer[i];
                sGame->playersReceived = sGame->numPlayers;
            }
        }
        if (sGame->playersReceived >= sGame->numPlayers)
        {
            DestroyTask(taskId);
            SetGfxFuncById(GFXFUNC_ERASE_MSG);
            sGame->state++;
        }
        break;
    }
}

#undef tState

static void RecvLinkData_Gameplay(void)
{
    u8 i;
    u8 numPlayers = sGame->numPlayers;

    sGame->players[0].receivedGameStatePacket = RecvPacket_GameState(0,
                                                  &sGame->players[0],
                                                  &sGame->players[0].comm,
                                                  &sGame->players[1].comm,
                                                  &sGame->players[2].comm,
                                                  &sGame->players[3].comm,
                                                  &sGame->players[4].comm,
                                                  &sGame->numGraySquares,
                                                  &sGame->berriesFalling,
                                                  &sGame->allReadyToEnd);
    sGame->clearRecvCmds = TRUE;

    for (i = 1; i < numPlayers; i++)
    {
        if (sGame->inputState[i] == INPUTSTATE_NONE && !RecvPacket_PickState(i, &sGame->players[i].comm.pickState))
        {
            sGame->players[i].comm.pickState = PICK_NONE;
            sGame->clearRecvCmds = FALSE;
        }
    }
    if (++sGame->clearRecvCmdTimer >= 60)
    {
        if (sGame->clearRecvCmds)
        {
            ClearRecvCommands();
            sGame->clearRecvCmdTimer = 0;
        }
        else if (sGame->clearRecvCmdTimer > 70)
        {
            ClearRecvCommands();
            sGame->clearRecvCmdTimer = 0;
        }
    }

    for (i = 0; i < numPlayers; i++)
    {
        if (sGame->players[i].comm.pickState != PICK_NONE && sGame->inputState[i] == INPUTSTATE_NONE)
        {
            sGame->inputState[i] = INPUTSTATE_TRY_PICK;
        }
        switch (sGame->inputState[i])
        {
        case INPUTSTATE_NONE:
        default:
            break;
        case INPUTSTATE_TRY_PICK:
        case INPUTSTATE_PICKED:
        case INPUTSTATE_ATE_BERRY:
            if (++sGame->inputDelay[i] >= 6)
            {
                sGame->inputDelay[i] = 0;
                sGame->inputState[i] = INPUTSTATE_NONE;
                sGame->players[i].comm.pickState = PICK_NONE;
                sGame->players[i].comm.ateBerry = FALSE;
                sGame->players[i].comm.missedBerry = FALSE;
            }
            break;
        case INPUTSTATE_BAD_MISS:
            // Tried to pick with no berry in range, long delay until next input
            if (++sGame->inputDelay[i] >= 40)
            {
                sGame->inputDelay[i] = 0;
                sGame->inputState[i] = INPUTSTATE_NONE;
                sGame->players[i].comm.pickState = PICK_NONE;
                sGame->players[i].comm.ateBerry = FALSE;
                sGame->players[i].comm.missedBerry = FALSE;
            }
            break;
        }
    }
}

static void RecvLinkData_ReadyToEnd(void)
{
    u8 i;
    u8 numPlayers = sGame->numPlayers;

    sGame->players[0].receivedGameStatePacket = RecvPacket_GameState(0,
                                                  &sGame->players[0],
                                                  &sGame->players[0].comm,
                                                  &sGame->players[1].comm,
                                                  &sGame->players[2].comm,
                                                  &sGame->players[3].comm,
                                                  &sGame->players[4].comm,
                                                  &sGame->numGraySquares,
                                                  &sGame->berriesFalling,
                                                  &sGame->allReadyToEnd);
    sGame->clearRecvCmds = TRUE;

    for (i = 1; i < numPlayers; i++)
    {
        if (RecvPacket_ReadyToEnd(i))
        {
            sGame->readyToEnd[i] = TRUE;
            sGame->clearRecvCmds = FALSE;
        }
    }
    if (++sGame->clearRecvCmdTimer >= 60)
    {
        if (sGame->clearRecvCmds)
        {
            ClearRecvCommands();
            sGame->clearRecvCmdTimer = 0;
        }
        else if (sGame->clearRecvCmdTimer > 70)
        {
            ClearRecvCommands();
            sGame->clearRecvCmdTimer = 0;
        }
    }
}

static void RecvLinkData_Leader(void)
{
    switch (sGame->funcId)
    {
    case FUNC_WAIT_START:
        if (AllPlayersReadyToStart() == TRUE)
        {
            ResetReadyToStart();
            sGame->startGame = TRUE;
        }
        break;
    case FUNC_PLAY_GAME:
        RecvLinkData_Gameplay();
        break;
    case FUNC_WAIT_END_GAME:
        RecvLinkData_ReadyToEnd();
        break;
    }
}

static void SendLinkData_Leader(void)
{
    switch (sGame->funcId)
    {
    case FUNC_PLAY_GAME:
        SendPacket_GameState(&sGame->player,
                             &sGame->players[0].comm,
                             &sGame->players[1].comm,
                             &sGame->players[2].comm,
                             &sGame->players[3].comm,
                             &sGame->players[4].comm,
                             sGame->numGraySquares,
                             sGame->berriesFalling,
                             sGame->allReadyToEnd);
        break;
    case FUNC_WAIT_END_GAME:
        SendPacket_GameState(&sGame->player,
                             &sGame->players[0].comm,
                             &sGame->players[1].comm,
                             &sGame->players[2].comm,
                             &sGame->players[3].comm,
                             &sGame->players[4].comm,
                             sGame->numGraySquares,
                             sGame->berriesFalling,
                             sGame->allReadyToEnd);
        break;
    }
}

static void RecvLinkData_Member(void)
{
    switch (sGame->funcId)
    {
    case FUNC_PLAY_GAME:
        RecvPacket_GameState(sGame->multiplayerId,
                            &sGame->players[sGame->multiplayerId],
                            &sGame->players[0].comm,
                            &sGame->players[1].comm,
                            &sGame->players[2].comm,
                            &sGame->players[3].comm,
                            &sGame->players[4].comm,
                            &sGame->numGraySquares,
                            &sGame->berriesFalling,
                            &sGame->allReadyToEnd);
        break;
    case FUNC_WAIT_END_GAME:
        RecvPacket_GameState(sGame->multiplayerId,
                            &sGame->players[sGame->multiplayerId],
                            &sGame->players[0].comm,
                            &sGame->players[1].comm,
                            &sGame->players[2].comm,
                            &sGame->players[3].comm,
                            &sGame->players[4].comm,
                            &sGame->numGraySquares,
                            &sGame->berriesFalling,
                            &sGame->allReadyToEnd);
        break;
    }
}

static void SendLinkData_Member(void)
{
    switch (sGame->funcId)
    {
    case FUNC_WAIT_START:
        SendPacket_ReadyToStart(TRUE);
        sGame->startGame = TRUE;
        break;
    case FUNC_PLAY_GAME:
        if (sGame->player.comm.pickState != PICK_NONE)
        {
            SendPacket_PickState(sGame->player.comm.pickState);
        }
        break;
    case FUNC_WAIT_END_GAME:
        if (!sGame->berriesFalling && !sGame->allReadyToEnd)
            SendPacket_ReadyToEnd(TRUE);
        break;
    }
}

static void HandleSound_Leader(void)
{
    if (sGame->players[sGame->multiplayerId].comm.pickState == PICK_NONE)
    {
        if (!IsSEPlaying())
            sGame->playingPickSound = FALSE;
    }
    else if (sGame->players[sGame->multiplayerId].comm.ateBerry == TRUE)
    {
        if (!sGame->playingPickSound)
        {
            m4aSongNumStop(SE_SUCCESS);
            PlaySE(SE_SUCCESS);
            sGame->playingPickSound = TRUE;
        }
    }
    else if (sGame->players[sGame->multiplayerId].comm.missedBerry == TRUE)
    {
        if (!sGame->playingPickSound && !IsSEPlaying())
        {
            PlaySE(SE_BOO);
            StartDodrioMissedAnim(1);
            sGame->playingPickSound = TRUE;
        }
    }

    if (sGame->endSoundState == 0 && sGame->numGraySquares >= NUM_STATUS_SQUARES)
    {
        // Ready to play game over sound
        StopMapMusic();
        sGame->endSoundState = 1;
    }
    else if (sGame->endSoundState == 1)
    {
        // Play game over sound
        PlayFanfareByFanfareNum(FANFARE_TOO_BAD);
        sGame->endSoundState = 2;
    }
}

static void HandleSound_Member(void)
{
    u8 berryStart = sGame->berryColStart;
    u8 berryEnd = sGame->berryColEnd;
    u8 i;
    if (sGame->players[sGame->multiplayerId].comm.pickState == PICK_NONE)
    {
        if (sGame->players[sGame->multiplayerId].comm.ateBerry != TRUE
         && sGame->players[sGame->multiplayerId].comm.missedBerry != TRUE)
            sGame->playingPickSound = 0;
    }
    else if (sGame->players[sGame->multiplayerId].comm.ateBerry == TRUE)
    {
        if (!sGame->playingPickSound)
        {
            m4aSongNumStop(SE_SUCCESS);
            PlaySE(SE_SUCCESS);
            sGame->playingPickSound = TRUE;
        }
    }
    else if (sGame->players[sGame->multiplayerId].comm.missedBerry == TRUE)
    {
        if (!sGame->playingPickSound && !IsSEPlaying())
        {
            PlaySE(SE_BOO);
            StartDodrioMissedAnim(1);
            sGame->playingPickSound = TRUE;
        }
    }
    for (i = berryStart; i < berryEnd; i++)
    {
        struct DodrioGame_Berries * berries = &sGame->players[sGame->multiplayerId].berries;
        if (berries->fallDist[i] >= MAX_FALL_DIST)
        {
            if (!sGame->playingSquishSound[i])
            {
                PlaySE(SE_BALLOON_RED + berries->ids[i]);
                sGame->playingSquishSound[i] = TRUE;
            }
        }
        else
        {
            sGame->playingSquishSound[i] = FALSE;
        }
    }
    if (sGame->endSoundState == 0 && sGame->numGraySquares >= NUM_STATUS_SQUARES)
    {
        // Ready to play game over sound
        StopMapMusic();
        sGame->endSoundState = 1;
    }
    else if (sGame->endSoundState == 1)
    {
        // Play game over sound
        PlayFanfareByFanfareNum(FANFARE_TOO_BAD);
        sGame->endSoundState = 2;
    }
}

static void CB2_DodrioGame(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_DodrioGame(void)
{
    TransferPlttBuffer();
    LoadOam();
    ProcessSpriteCopyRequests();
}

static void InitMonInfo(struct DodrioGame_MonInfo * monInfo, struct Pokemon *mon)
{
    monInfo->isShiny = IsMonShiny(mon);
}

static void CreateTask_(TaskFunc func, u8 priority)
{
    CreateTask(func, priority);
}

static void CreateDodrioGameTask(TaskFunc func)
{
    sGame->taskId = CreateTask(func, 1);
    sGame->state = 0;
    sGame->startState = 0;
    sGame->timer = 0;
}

static void SetGameFunc(u8 funcId)
{
    sGame->prevFuncId = sGame->funcId;
    sGame->funcId = funcId;
    sGame->state = 0;
    sGame->timer = 0;
}

static bool32 SlideTreeBordersOut(void)
{
    u8 x = sGame->timer / 4;
    sGame->timer++;
    if (x != 0 && sGame->timer % 4 == 0)
    {
        if (x < sTreeBorderXPos[sGame->numPlayers - 1])
        {
            // Update position
            SetGpuReg(REG_OFFSET_BG1HOFS,  (x * 8)); // BG_TREE_LEFT
            SetGpuReg(REG_OFFSET_BG2HOFS, -(x * 8)); // BG_TREE_RIGHT
            return FALSE;
        }
        else
        {
            // Animation finished
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}

static void InitFirstWaveOfBerries(void)
{
    u8 i;
    u8 berryStart = sGame->berryColStart;
    u8 berryEnd = sGame->berryColEnd;

    for (i = berryStart; i < berryEnd; i++)
    {
        struct DodrioGame_Berries * berries = &sGame->player.berries;
        berries->fallDist[i] = (i % 2 == 0) ? 1 : 0;
        berries->ids[i] = BERRY_BLUE;
    }
}

// This function checks every berry and resolves if it should be eaten or not.
// It's run in a loop that handles moving each individual berry, which means
// that every time any berry moves, every single berry is checked.
static void HandlePickBerries(void)
{
    u8 berryStart = sGame->berryColStart;
    u8 berryEnd = sGame->berryColEnd;
    u8 numPlayers = sGame->numPlayers;
    u8 i, j, k, column;

    // Game is already over
    if (sGame->numGraySquares >= NUM_STATUS_SQUARES)
        return;

    for (i = 0; i < numPlayers; i++)
    {
        u8 *pickState = &sGame->players[i].comm.pickState;
        if (*pickState != PICK_NONE && sGame->inputState[i] == INPUTSTATE_TRY_PICK)
        {
            // Player is attempting to pick a berry
            for (j = berryStart; j < berryEnd; j++)
            {
                column = sActiveColumnMap[0][0][j];

                // Attempt has already been checked
                if (sGame->playersAttemptingPick[column][0] == i
                 || sGame->playersAttemptingPick[column][1] == i)
                    break;

                // Check berry pick attempt
                if (TryPickBerry(i, *pickState, column) == TRUE)
                {
                    // Attempt was successful
                    for (k = 0; k < ARRAY_COUNT(sGame->playersAttemptingPick[0]); k++)
                    {
                        if (sGame->playersAttemptingPick[column][k] == PLAYER_NONE)
                        {
                            sGame->playersAttemptingPick[column][k] = i;
                            sGame->inputState[i] = INPUTSTATE_PICKED;
                            sGame->berryState[column] = BERRYSTATE_PICKED;
                            break;
                        }
                    }
                    break;
                }
                if (sGame->players[i].comm.missedBerry == TRUE)
                    break;
            }
        }
    }

    for (j = berryStart; j < berryEnd; j++)
    {
        u8 playerIdMissed = PLAYER_NONE;
        column = sActiveColumnMap[0][0][j];
        if (sGame->berryState[column] == BERRYSTATE_PICKED)
        {
            s32 delayRemaining;
            u8 playerIdPicked, delayStage = sGame->difficulty[GetPlayerIdAtColumn(column)] / NUM_DIFFICULTIES;
            if (delayStage >= ARRAY_COUNT(sBerryFallDelays) - 1)
                delayStage = ARRAY_COUNT(sBerryFallDelays) - 1;

            delayRemaining = sBerryFallDelays[delayStage][sGame->players[0].berries.ids[column]] - sGame->fallTimer[column];
            if (delayRemaining < 6)
                sGame->eatTimer[column] += delayRemaining;

            if (++sGame->eatTimer[column] >= 6)
            {
                sGame->eatTimer[column] = 0;

                if (sGame->playersAttemptingPick[column][0] == PLAYER_NONE
                 && sGame->playersAttemptingPick[column][1] == PLAYER_NONE)
                {
                    // No players attempting to pick this berry
                    continue;
                }
                else if (sGame->playersAttemptingPick[column][0] != PLAYER_NONE
                      && sGame->playersAttemptingPick[column][1] == PLAYER_NONE)
                {
                    // One player attempting to pick this berry
                    playerIdPicked = sGame->playersAttemptingPick[column][0];
                }
                else
                {
                    // Two players attempting to pick this berry
                    // Randomly give it to one of them
                    u8 playerId1 = sGame->playersAttemptingPick[column][0];
                    i = sGame->playersAttemptingPick[column][1]; // playerId2. Have to re-use the variable to match.
                    if (!(Random() & 1))
                    {
                        playerIdPicked = playerId1;
                        playerIdMissed = i;
                    }
                    else
                    {
                        playerIdPicked = i;
                        playerIdMissed = playerId1;
                    }
                }

                // Eat berry
                sGame->player.berries.fallDist[column] = EAT_FALL_DIST;
                sGame->berryState[column] = BERRYSTATE_EATEN;
                sGame->inputState[playerIdPicked] = INPUTSTATE_ATE_BERRY;
                sGame->berryEatenBy[column] = playerIdPicked;
                sGame->players[playerIdPicked].comm.ateBerry = TRUE;
#ifdef UBFIX
                if (playerIdMissed != PLAYER_NONE)
#endif
                    sGame->players[playerIdMissed].comm.missedBerry = TRUE; // UB: playerIdMissed can be PLAYER_NONE here, which is out of bounds

                sGame->berriesEaten[playerIdPicked]++;
                IncrementBerryResult(0, column, playerIdPicked);
                UpdateBerriesPickedInRow(TRUE);
                TryIncrementDifficulty(playerIdPicked);
                sGame->prevBerryIds[column] = sGame->player.berries.ids[column];
                sGame->player.berries.ids[column] = BERRY_MISSED; // Just to clear berry id, wasn't actually missed
                sGame->playersAttemptingPick[column][0] = PLAYER_NONE;
                sGame->playersAttemptingPick[column][1] = PLAYER_NONE;
            }
        }
    }
}

static bool32 TryPickBerry(u8 playerId, u8 pickState, u8 column)
{
    s32 pick = 0;
    u8 numPlayersIdx = sGame->numPlayers - 1;
    struct DodrioGame_Berries * berries = &sGame->player.berries;

    switch (pickState)
    {
    case PICK_LEFT:
    default:
        pick = 0;
        break;
    case PICK_MIDDLE:
        pick = 1;
        break;
    case PICK_RIGHT:
        pick = 2;
        break;
    }

    // Check if berry is within range to be picked
    if (berries->fallDist[column] == EAT_FALL_DIST - 1 || berries->fallDist[column] == EAT_FALL_DIST)
    {
        // Check if this berry is the one the player is trying to pick
        if (column == sDodrioHeadToColumnMap[numPlayersIdx][playerId][pick])
        {
            // Check if berry has been picked/eaten by another player
            if (sGame->berryState[column] == BERRYSTATE_PICKED || sGame->berryState[column] == BERRYSTATE_EATEN)
            {
                // Missed berry, picked by someone else
                sGame->players[playerId].comm.missedBerry = TRUE;
                return FALSE;
            }
            else
            {
                // Successfully picked berry
                return TRUE;
            }
        }
    }
    else
    {
        // Check if this berry is the one the player is trying to pick
        if (column == sDodrioHeadToColumnMap[numPlayersIdx][playerId][pick])
        {
            // Missed berry, out of range
            sGame->inputState[playerId] = INPUTSTATE_BAD_MISS;
            sGame->players[playerId].comm.missedBerry = TRUE;
        }
    }
    return FALSE;
}

static void UpdateFallingBerries(void)
{
    u8 berryStart = sGame->berryColStart;
    u8 berryEnd = sGame->berryColEnd;
    u8 delayStage = 0;
    u8 otherBerryMissed = 0;
    u8 i;

    sGame->berriesFalling = FALSE;

    for (i = berryStart; i < berryEnd - 1; i++)
    {
        struct DodrioGame *game = sGame;

        if (sGame->berryState[i] == BERRYSTATE_NONE || sGame->berryState[i] == BERRYSTATE_PICKED)
        {
            sGame->berriesFalling = TRUE;

            if (game->player.berries.fallDist[i] >= MAX_FALL_DIST)
            {
                // Berry hit the ground
                game->player.berries.fallDist[i] = MAX_FALL_DIST;
                sGame->berryState[i] = BERRYSTATE_SQUISHED;
                if (!sGame->playingSquishSound[i])
                {
                    sGame->playingSquishSound[i] = TRUE;
                    PlaySE(SE_BALLOON_RED + game->player.berries.ids[i]);
                }
                if (sGame->numGraySquares < NUM_STATUS_SQUARES || otherBerryMissed == TRUE)
                {
                    otherBerryMissed = TRUE;
                    sGame->playingSquishSound[i] = FALSE;
                    if (sGame->numGraySquares < NUM_STATUS_SQUARES)
                        sGame->numGraySquares++;

                    IncrementBerryResult(BERRY_MISSED, i, 0);
                    UpdateBerriesPickedInRow(FALSE);
                }
            }
            else
            {
                // Berry is still falling
                u8 delay;
                delayStage = sGame->difficulty[GetPlayerIdAtColumn(i)] / NUM_DIFFICULTIES;
                if (delayStage >= ARRAY_COUNT(sBerryFallDelays) - 1)
                    delayStage = ARRAY_COUNT(sBerryFallDelays) - 1;

                delay = sBerryFallDelays[delayStage][game->player.berries.ids[i]];
                if (++sGame->fallTimer[i] >= delay)
                {
                    game->player.berries.fallDist[i]++;
                    sGame->fallTimer[i] = 0;
                }
                HandlePickBerries();
            }
        }
        else if (sGame->berryState[i] == BERRYSTATE_EATEN)
        {
            // Berry has been eaten, wait and create a new berry
            sGame->berriesFalling = TRUE;
            if (++sGame->newBerryTimer[i] >= 20)
            {
                sGame->players[sGame->berryEatenBy[i]].comm.ateBerry = FALSE;
                sGame->newBerryTimer[i] = 0;
                sGame->fallTimer[i] = 0;
                sGame->berryState[i] = BERRYSTATE_NONE;
                game->player.berries.fallDist[i] = 1;
                game->player.berries.ids[i] = GetNewBerryId(GetPlayerIdAtColumn(i), i);
            }
        }
        else if (sGame->berryState[i] == BERRYSTATE_SQUISHED)
        {
            // Berry has already hit the ground, wait and create a new berry
            if (++sGame->newBerryTimer[i] >= 20)
            {
                if (sGame->numGraySquares < NUM_STATUS_SQUARES)
                {
                    sGame->newBerryTimer[i] = 0;
                    sGame->fallTimer[i] = 0;
                    sGame->berryState[i] = BERRYSTATE_NONE;
                    game->player.berries.fallDist[i] = 1;
                    sGame->prevBerryIds[i] = game->player.berries.ids[i];
                    game->player.berries.ids[i] = GetNewBerryId(GetPlayerIdAtColumn(i), i);
                }
            }
        }
    }
}

static void UpdateBerrySprites(void)
{
    u8 i;
    u8 berryStart = sGame->berryColStart;
    u8 berryEnd = sGame->berryColEnd;

    for (i = berryStart; i < berryEnd; i++)
    {
        struct DodrioGame_Player *player = &sGame->players[sGame->multiplayerId];
        u8 column = sActiveColumnMap[sGame->numPlayers - 1][sGame->multiplayerId][i];

        if (player->berries.fallDist[column] != 0)
            SetBerryInvisibility(i, FALSE);
        else
            SetBerryInvisibility(i, TRUE);

        if (player->berries.fallDist[column] >= MAX_FALL_DIST)
        {
            // Berry was missed, set squished anim
            SetBerryAnim(i, player->berries.ids[column] + BERRY_MISSED);
            SetBerryYPos(i, player->berries.fallDist[column] * 2 - 1);
        }
        else if (player->berries.ids[column] == 3)
        {
            // Berry was picked, set eaten anim
            player->berries.fallDist[column] = EAT_FALL_DIST;
            SetBerryAnim(i, ANIM_EATEN);
            SetBerryYPos(i, player->berries.fallDist[column] * 2 - 1);
        }
        else
        {
            // Berry is still falling
            SetBerryAnim(i, player->berries.ids[column]);
            SetBerryYPos(i, player->berries.fallDist[column] * 2);
        }
    }
}

static void UpdateAllDodrioAnims(void)
{
    u8 i, numPlayers;

    numPlayers = sGame->numPlayers;
    for (i = 0; i < numPlayers; i++)
    {
        struct DodrioGame_Player *player = &sGame->players[i];
        SetDodrioAnim(i, player->comm.pickState);
    }
}

static void SetAllDodrioDisabled(void)
{
    u8 i, numPlayers;

    numPlayers = sGame->numPlayers;
    for (i = 0; i < numPlayers; i++)
        SetDodrioAnim(i, PICK_DISABLED);
}

static void UpdateGame_Leader(void)
{
    UpdateBerrySprites();
    if (sGame->numGraySquares >= NUM_STATUS_SQUARES)
        SetAllDodrioDisabled();
    else
        UpdateAllDodrioAnims();

    UpdateStatusBarAnim(sGame->numGraySquares);
}

// Identical to UpdateGame_Leader
static void UpdateGame_Member(void)
{
    UpdateBerrySprites();
    if (sGame->numGraySquares >= NUM_STATUS_SQUARES)
        SetAllDodrioDisabled();
    else
        UpdateAllDodrioAnims();

    UpdateStatusBarAnim(sGame->numGraySquares);
}

static void GetActiveBerryColumns(u8 numPlayers, u8 *start, u8 *end)
{
    switch (numPlayers)
    {
    case 1:
        *start = 4, *end = 7;
        break;
    case 2:
        *start = 3, *end = 8;
        break;
    case 3:
        *start = 2, *end = 9;
        break;
    case 4:
        *start = 1, *end = 10;
        break;
    case 5:
        *start = 0, *end = 11;
        break;
    }
}

static bool32 AllPlayersReadyToStart(void)
{
    u8 i, numPlayers;

    numPlayers = sGame->numPlayers;
    for (i = 1; i < numPlayers; i++)
    {
        if (sGame->readyToStart[i] == FALSE)
            sGame->readyToStart[i] = RecvPacket_ReadyToStart(i);
    }

    numPlayers = numPlayers; // Needed to force compiler to keep loop below

#ifdef BUGFIX
    i = 1; // i isn't reset, loop below never runs. As a result, game can begin before all players ready
#endif
    for (; i < numPlayers; i++)
    {
        if (sGame->readyToStart[i] == FALSE)
            return FALSE;
    }

    return TRUE;
}

static void ResetReadyToStart(void)
{
    u8 i;

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
        sGame->readyToStart[i] = FALSE;
}

static bool32 ReadyToEndGame_Leader(void)
{
    if (sGame->numGraySquares >= NUM_STATUS_SQUARES && !sGame->berriesFalling)
    {
        sGame->numGraySquares = NUM_STATUS_SQUARES;
        if (sGame->allReadyToEnd)
            return TRUE;
    }

    return FALSE;
}

static bool32 ReadyToEndGame_Member(void)
{
    u8 i, berryStart, berryEnd;

    if (sGame->numGraySquares >= NUM_STATUS_SQUARES)
    {
        berryStart = sGame->berryColStart;
        berryEnd = sGame->berryColEnd;
        sGame->numGraySquares = NUM_STATUS_SQUARES;
        if (sGame->allReadyToEnd)
        {
            for (i = berryStart; i < berryEnd; i++)
            {
                struct DodrioGame_Player *player = &sGame->players[sGame->multiplayerId];
                u8 column = sActiveColumnMap[sGame->numPlayers - 1][sGame->multiplayerId][i];

                if (player->berries.fallDist[column] != MAX_FALL_DIST)
                    return FALSE;
            }
            return TRUE;
        }
    }

    return FALSE;
}

static void TryIncrementDifficulty(u8 playerId)
{
    u8 threshold = sDifficultyThresholds[sGame->difficulty[playerId] % NUM_DIFFICULTIES] + (sGame->difficulty[playerId] / NUM_DIFFICULTIES) * 100;
    if (sGame->berriesEaten[playerId] >= threshold)
        sGame->difficulty[playerId]++;
}

static u8 GetPlayerIdAtColumn(u8 column)
{
    return sPlayerIdAtColumn[sGame->numPlayers - 1][column];
}

// Get a berry id for when a new falling berry is created.
// What type of berry it is depends on the current difficulty
// level of players who can pick berries from that column.
static u8 GetNewBerryId(u8 playerId, u8 column)
{
    u8 i, highestDifficulty;
    u8 numPlayersIdx = sGame->numPlayers - 1;
    u8 leftPlayer = sDodrioNeighborMap[numPlayersIdx][playerId][0];
    u8 middlePlayer = sDodrioNeighborMap[numPlayersIdx][playerId][1];
    u8 rightPlayer = sDodrioNeighborMap[numPlayersIdx][playerId][2];

    for (i = 0; sUnsharedColumns[numPlayersIdx][i] != 0; i++)
    {
        // If only one player can use this column, just use their difficulty
        if (column == sUnsharedColumns[numPlayersIdx][i])
            return GetNewBerryIdByDifficulty(sGame->difficulty[middlePlayer], column);
    }

    // This column is shared, get the highest difficulty of adjacent players
    if (sGame->difficulty[leftPlayer] > sGame->difficulty[middlePlayer])
        highestDifficulty = sGame->difficulty[leftPlayer];
    else
        highestDifficulty = sGame->difficulty[middlePlayer];

    if (sGame->difficulty[rightPlayer] > highestDifficulty)
        highestDifficulty = sGame->difficulty[rightPlayer];

    return GetNewBerryIdByDifficulty(highestDifficulty, column);
}

// The berry types cycle through different distributions depending on the difficulty
static u8 GetNewBerryIdByDifficulty(u8 difficulty, u8 column)
{
    u8 prevBerryId = sGame->prevBerryIds[column];
    switch (difficulty % NUM_DIFFICULTIES)
    {
    default: return BERRY_BLUE;
    case 0:  return BERRY_BLUE;
    case 1:  return BERRY_GREEN;
    case 2:  return BERRY_GOLD;
    case 3:
        if (prevBerryId == BERRY_BLUE)
            return BERRY_GREEN;
        else
            return BERRY_BLUE;
    case 4:
        if (prevBerryId == BERRY_BLUE)
            return BERRY_GOLD;
        else
            return BERRY_BLUE;
    case 5:
        if (prevBerryId == BERRY_GOLD)
            return BERRY_GREEN;
        else
            return BERRY_GOLD;
    case 6:
        if (prevBerryId == BERRY_BLUE)
            return BERRY_GREEN;
        else if (prevBerryId == BERRY_GREEN)
            return BERRY_GOLD;
        else
            return BERRY_BLUE;
    }
}

static bool32 IsTotalBerriesMissedOver10(u16 berryResults[MAX_RFU_PLAYERS][NUM_BERRY_IDS])
{
    int missed = 0, i = 0;
    for (; i < GetLinkPlayerCount(); missed += berryResults[i][BERRY_MISSED], i++)
        ;

    if (missed > 10)
        return TRUE;
    else
        return FALSE;
}

// Despite being set up to take a berry id as an argument, this
// function is only ever given BERRY_BLUE or BERRY_MISSED.
// It reads the actual berry id (if necessary) from ids
static void IncrementBerryResult(u8 berryIdArg, u8 column, u8 playerId)
{
    u8 berryId;
    u8 numPlayers = sGame->numPlayers;
    switch (berryIdArg)
    {
    case BERRY_BLUE:
    case BERRY_GREEN:
    case BERRY_GOLD:
        berryId = sGame->players[0].berries.ids[column];
        sGame->berryResults[playerId][berryId] = IncrementWithLimit(sGame->berryResults[playerId][berryId], 20000);
        break;
    case BERRY_MISSED:
        if (IsTotalBerriesMissedOver10(sGame->berryResults))
            break;
        switch (numPlayers)
        {
        case 5:
            switch (column)
            {
            case 0:
                sGame->berryResults[2][BERRY_MISSED]++;
                sGame->berryResults[3][BERRY_MISSED]++;
                break;
            case 1:
                sGame->berryResults[3][BERRY_MISSED]++;
                break;
            case 2:
                sGame->berryResults[3][BERRY_MISSED]++;
                sGame->berryResults[4][BERRY_MISSED]++;
                break;
            case 3:
                sGame->berryResults[4][BERRY_MISSED]++;
                break;
            case 4:
                sGame->berryResults[4][BERRY_MISSED]++;
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 5:
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 6:
                sGame->berryResults[0][BERRY_MISSED]++;
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 7:
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 8:
                sGame->berryResults[1][BERRY_MISSED]++;
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            case 9:
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            }
            break;
        case 4:
            switch (column)
            {
            case 1:
                sGame->berryResults[2][BERRY_MISSED]++;
                sGame->berryResults[3][BERRY_MISSED]++;
                break;
            case 2:
                sGame->berryResults[3][BERRY_MISSED]++;
                break;
            case 3:
                sGame->berryResults[3][BERRY_MISSED]++;
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 4:
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 5:
                sGame->berryResults[0][BERRY_MISSED]++;
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 6:
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 7:
                sGame->berryResults[1][BERRY_MISSED]++;
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            case 8:
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            }
            break;
        case 3:
            switch (column)
            {
            case 2:
                sGame->berryResults[1][BERRY_MISSED]++;
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            case 3:
                sGame->berryResults[2][BERRY_MISSED]++;
                break;
            case 4:
                sGame->berryResults[2][BERRY_MISSED]++;
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 5:
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 6:
                sGame->berryResults[0][BERRY_MISSED]++;
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 7:
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            }
            break;
        case 2:
            switch (column)
            {
            case 3:
                sGame->berryResults[0][BERRY_MISSED]++;
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 4:
                sGame->berryResults[0][BERRY_MISSED]++;
                break;
            case 5:
                sGame->berryResults[0][BERRY_MISSED]++;
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            case 6:
                sGame->berryResults[1][BERRY_MISSED]++;
                break;
            }
            break;
        }
        break;
    }
}

static void UpdateBerriesPickedInRow(bool32 picked)
{
    // The 'berries picked in row' stat is only
    // counted for games with all 5 players
    if (sGame->numPlayers != MAX_RFU_PLAYERS)
        return;

    if (picked == TRUE)
    {
        if (++sGame->berriesPickedInRow > sGame->maxBerriesPickedInRow)
            sGame->maxBerriesPickedInRow = sGame->berriesPickedInRow;
        if (sGame->berriesPickedInRow > MAX_BERRIES)
            sGame->berriesPickedInRow = MAX_BERRIES;
    }
    else // missed
    {
        if (sGame->berriesPickedInRow > sGame->maxBerriesPickedInRow)
            sGame->maxBerriesPickedInRow = sGame->berriesPickedInRow;
        sGame->berriesPickedInRow = 0;
    }
}

static void SetMaxBerriesPickedInRow(void)
{
    u8 i;
    for (i = 0; i < sGame->numPlayers; i++)
        sGame->berryResults[i][BERRY_IN_ROW] = sGame->maxBerriesPickedInRow;
}

static void ResetForPlayAgainPrompt(void)
{
    u8 i, j;

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        for (j = 0; j < NUM_BERRY_COLUMNS; j++)
            sGame->players[i].berries.fallDist[j] = 0;
        sGame->players[i].comm.pickState = PICK_NONE;
        sGame->players[i].comm.ateBerry = FALSE;
        sGame->difficulty[i] = 0;
        sGame->berriesEaten[i] = 0;
        sGame->scoreResults[i].ranking = 0;
        sGame->scoreResults[i].score = 0;
        sGame->berryResults[i][BERRY_BLUE] = 0;
        sGame->berryResults[i][BERRY_GREEN] = 0;
        sGame->berryResults[i][BERRY_GOLD] = 0;
        sGame->berryResults[i][BERRY_MISSED] = 0;
        sGame->berryResults[i][BERRY_PRIZE] = 0;
        sGame->berryResults[i][BERRY_IN_ROW] = 0;
    }
    sGame->endSoundState = 0;
    sGame->berriesPickedInRow = 0;
    sGame->numGraySquares = 0;
    UpdateAllDodrioAnims();
    UpdateBerrySprites();
}

static const s16 sBerryScoreMultipliers[] = {
    [BERRY_BLUE]   = 10,
    [BERRY_GREEN]  = 30,
    [BERRY_GOLD]   = 50,
    [BERRY_MISSED] = 50 // Subtracted
};

static void SetRandomPrize(void)
{
    u8 i, prizeSet = 0, prizeIdx = 0;

    switch (sGame->numPlayers)
    {
    case 4:  prizeSet = 1; break;
    case 5:  prizeSet = 2; break;
    }

    prizeIdx = Random() % ARRAY_COUNT(sPrizeBerryIds[0]);
    for (i = 0; i < MAX_RFU_PLAYERS; i++)
        sGame->berryResults[i][BERRY_PRIZE] = sPrizeBerryIds[prizeSet][prizeIdx];
}

static u32 GetBerriesPicked(u8 playerId)
{
    u32 sum = sGame->berryResults[playerId][BERRY_BLUE]
            + sGame->berryResults[playerId][BERRY_GREEN]
            + sGame->berryResults[playerId][BERRY_GOLD];
    return min(sum, MAX_BERRIES);
}

static void TryUpdateRecords(void)
{
    u32 berriesPicked = Min(GetBerriesPicked(sGame->multiplayerId), MAX_BERRIES); // Min here is redundant
    u32 score = Min(GetScore(sGame->multiplayerId), MAX_SCORE);

    if (gSaveBlock2Ptr->berryPick.bestScore < score)
        gSaveBlock2Ptr->berryPick.bestScore = score;
    if (gSaveBlock2Ptr->berryPick.berriesPicked < berriesPicked)
        gSaveBlock2Ptr->berryPick.berriesPicked = berriesPicked;
    if (gSaveBlock2Ptr->berryPick.berriesPickedInRow < sGame->maxBerriesPickedInRow)
        gSaveBlock2Ptr->berryPick.berriesPickedInRow = sGame->maxBerriesPickedInRow;
}

// Enqueue the given state, and dequeue and return
// the state that should be used next
static u8 UpdatePickStateQueue(u8 pickState)
{
    u8 i, nextState;

    nextState = sGame->pickStateQueue[ARRAY_COUNT(sGame->pickStateQueue) - 1];
    for (i = ARRAY_COUNT(sGame->pickStateQueue) - 1; i != 0; i--)
        sGame->pickStateQueue[i] = sGame->pickStateQueue[i - 1];
    sGame->pickStateQueue[0] = pickState;
    return nextState;
}

// The player may extend their Dodrio's heads while they wait for
// other players to respond to the "Play again?" prompt
static void HandleWaitPlayAgainInput(void)
{
    if (sGame->inputDelay[sGame->multiplayerId] == 0)
    {
        if (JOY_NEW(DPAD_UP))
        {
            sGame->players[sGame->multiplayerId].comm.pickState = PICK_MIDDLE;
            sGame->inputDelay[sGame->multiplayerId] = 6;
            PlaySE(SE_M_CHARM);
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            sGame->players[sGame->multiplayerId].comm.pickState = PICK_LEFT;
            sGame->inputDelay[sGame->multiplayerId] = 6;
            PlaySE(SE_M_CHARM);
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            sGame->players[sGame->multiplayerId].comm.pickState = PICK_RIGHT;
            sGame->inputDelay[sGame->multiplayerId] = 6;
            PlaySE(SE_M_CHARM);
        }
        else
        {
            sGame->players[sGame->multiplayerId].comm.pickState = PICK_NONE;
        }
    }
    else
    {
        sGame->inputDelay[sGame->multiplayerId]--;
    }
}

static void ResetPickState(void)
{
    sGame->players[sGame->multiplayerId].comm.pickState = PICK_NONE;
}

static u16 GetPrizeItemId(void)
{
    return sGame->berryResults[sGame->multiplayerId][BERRY_PRIZE] + FIRST_BERRY_INDEX;
}

static u8 GetNumPlayers(void)
{
    return sGame->numPlayers;
}

static u8 *GetPlayerName(u8 id)
{
    if (gReceivedRemoteLinkPlayers)
        return gLinkPlayers[id].name;
    else
        return sGame->players[id].name;
}

static u16 GetBerryResult(u8 playerId, u8 berryId)
{
    return sGame->berryResults[playerId][berryId];
}

static u32 GetScore(u8 playerId)
{
    u8 i;
    u32 scoreLost, score = 0;

    // Sum up points for berries picked
    for (i = 0; i < BERRY_MISSED; i++)
        score += sGame->berryResults[playerId][i] * sBerryScoreMultipliers[i];

    // Get points lost for berries missed
    scoreLost = sGame->berryResults[playerId][BERRY_MISSED] * sBerryScoreMultipliers[BERRY_MISSED];

    if (score <= scoreLost)
        return 0;
    else
        return score - scoreLost;
}

static u32 GetHighestScore(void)
{
    u8 i, numPlayers = sGame->numPlayers;
    u32 maxScore = GetScore(0);

    for (i = 1; i < numPlayers; i++)
    {
        u32 score = GetScore(i);
        if (score > maxScore)
            maxScore = score;
    }
    return Min(maxScore, MAX_SCORE);
}

static u32 GetHighestBerryResult(u8 berryId)
{
    u8 i, numPlayers = sGame->numPlayers;
    u16 maxScore = sGame->berryResults[0][berryId];

    for (i = 0; i < numPlayers; i++)
    {
        u16 score = sGame->berryResults[i][berryId];
        if (score > maxScore)
            maxScore = score;
    }
    return maxScore;
}

static u32 GetScoreByRanking(u8 ranking)
{
    u32 scores[MAX_RFU_PLAYERS], temp;
    s16 unsorted = TRUE;
    u8 i;
    u8 numPlayers = sGame->numPlayers;

    for (i = 0; i < numPlayers; i++)
        scores[i] = temp = GetScore(i);

    // Sort the scores in the array highest to lowest
    while (unsorted)
    {
        unsorted = FALSE;
        for (i = 0; i < numPlayers - 1; i++)
        {
            if (scores[i] < scores[i + 1])
            {
                SWAP(scores[i], scores[i + 1], temp);
                unsorted = TRUE;
            }
        }
    }

    return scores[ranking];
}

static u32 SetScoreResults(void)
{
    u8 i, ranking = 0, nextRanking = 0, playersRanked = 0;
    u8 numPlayers = sGame->numPlayers;

    GetHighestScore(); // Useless call

    if (GetHighestScore() == 0)
    {
        // No one scored any points, put everyone in last place with a score of 0.
        // Presumably this was supposed to then return, as the assignments in this
        // loop are then overwritten by the rest of the function
        for (i = 0; i < numPlayers; i++)
        {
            sGame->scoreResults[i].ranking = MAX_RFU_PLAYERS - 1;
            sGame->scoreResults[i].score = 0;
        }
    }

    // Set scores
    for (i = 0; i < numPlayers; i++)
        sGame->scoreResults[i].score = Min(GetScore(i), MAX_SCORE);

    // Set rankings
    do
    {
        u32 score = GetScoreByRanking(ranking);
        u8 curRanking = nextRanking;

        // Find all players with the score for this ranking.
        // Increment nextRanking but not curRanking to allow
        // for ties
        for (i = 0; i < numPlayers; i++)
        {
            if (score == sGame->scoreResults[i].score)
            {
                sGame->scoreResults[i].ranking = curRanking;
                nextRanking++;
                playersRanked++;
            }
        }
        ranking = nextRanking;
    } while (playersRanked < numPlayers);

    return 0;
}

static void GetScoreResults(struct DodrioGame_ScoreResults *dst, u8 playerId)
{
    *dst = sGame->scoreResults[playerId];
}

// Returns where the specified player's score ranks, 0 being first (highest score)
static u8 UNUSED GetScoreRanking(u8 playerId)
{
    u8 i, ranking = 0;
    u8 numPlayers = sGame->numPlayers;
    u32 playersScore;
    u32 scores[MAX_RFU_PLAYERS] = {0};

    for (i = 0; i < numPlayers; i++)
        scores[i] = GetScore(i);

    playersScore = scores[playerId];
    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        if (i != playerId && playersScore < scores[i])
            ranking++;
    }

    return ranking;
}

enum {
    PRIZE_RECEIVED,
    PRIZE_FILLED_BAG,
    PRIZE_NO_ROOM,
    NO_PRIZE,
};

static u8 TryGivePrize(void)
{
    u8 multiplayerId = sGame->multiplayerId;
    u16 itemId = GetPrizeItemId();

    if (GetScore(multiplayerId) != GetHighestScore())
        return NO_PRIZE;
    if (!CheckBagHasSpace(itemId, 1))
        return PRIZE_NO_ROOM;

    AddBagItem(itemId, 1);
    if (!CheckBagHasSpace(itemId, 1))
        return PRIZE_FILLED_BAG;
    return PRIZE_RECEIVED;
}

static u32 IncrementWithLimit(u32 num, u32 max)
{
    if (num < max)
        return num + 1;
    else
        return max;
}

static u32 Min(u32 a, u32 b)
{
    if (a < b)
        return a;
    else
        return b;
}

static u8 GetPlayerIdByPos(u8 id)
{
    return sGame->posToPlayerId[id];
}

void IsDodrioInParty(void)
{
    int i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SANITY_HAS_SPECIES)
            && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_DODRIO)
        {
            gSpecialVar_Result = TRUE;
            return;
        }
    }

    gSpecialVar_Result = FALSE;
}

#define NUM_RECORD_TYPES 3

void ShowDodrioBerryPickingRecords(void)
{
    u8 taskId = CreateTask(Task_ShowDodrioBerryPickingRecords, 0);
    Task_ShowDodrioBerryPickingRecords(taskId);
}

static const struct WindowTemplate sWindowTemplates_Records =
{
    .bg = 0,
    .tilemapLeft = 5,
    .tilemapTop = 1,
    .width = 20,
    .height = 11,
    .paletteNum = 15,
    .baseBlock = 0x1,
};

static const u8 *const sRecordsTexts[NUM_RECORD_TYPES] = {gText_BerriesPicked, gText_BestScore, gText_BerriesInRowFivePlayers};
static const u8 sRecordNumMaxDigits[NUM_RECORD_TYPES] = {4, 7, 4};
ALIGNED(4)
static const u8 sRecordTextYCoords[NUM_RECORD_TYPES][2] = {{25}, {41}, {57}};
static const u8 sRecordNumYCoords[NUM_RECORD_TYPES][2] = {{25}, {41}, {73}};

#define tState data[0]
#define tWindowId data[1]

static void Task_ShowDodrioBerryPickingRecords(u8 taskId)
{
    struct WindowTemplate window;
    s32 i, width, widthCurr;
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        window = sWindowTemplates_Records;
        width = GetStringWidth(FONT_NORMAL, gText_BerryPickingRecords, 0);
        for (i = 0; i < ARRAY_COUNT(sRecordsTexts); i++)
        {
            widthCurr = GetStringWidth(FONT_NORMAL, sRecordsTexts[i], 0) + 50;
            if (widthCurr > width)
                width = widthCurr;
        }
        width = (width + 7) / 8;
        if (width & 1)
            width++;
        window.tilemapLeft = (30 - width) / 2;
        window.width = width;
        tWindowId = AddWindow(&window);
        PrintRecordsText(tWindowId, width);
        CopyWindowToVram(tWindowId, COPYWIN_FULL);
        tState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
            tState++;
        break;
    case 2:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            rbox_fill_rectangle(tWindowId);
            CopyWindowToVram(tWindowId, COPYWIN_MAP);
            tState++;
        }
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            RemoveWindow(tWindowId);
            DestroyTask(taskId);
            ScriptContext_Enable();
        }
        break;
    }
}

#undef tState
#undef tWindowId

static void PrintRecordsText(u8 windowId, s32 width)
{
    s32 i, x, numWidth;
    s32 recordNums[NUM_RECORD_TYPES];
    recordNums[0] = gSaveBlock2Ptr->berryPick.berriesPicked;
    recordNums[1] = gSaveBlock2Ptr->berryPick.bestScore;
    recordNums[2] = gSaveBlock2Ptr->berryPick.berriesPickedInRow;

    LoadUserWindowBorderGfx_(windowId, 0x21D, BG_PLTT_ID(13));
    DrawTextBorderOuter(windowId, 0x21D, 13);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gText_BerryPickingRecords, GetStringCenterAlignXOffset(FONT_NORMAL, gText_BerryPickingRecords, width * 8), 1, TEXT_SKIP_DRAW, NULL);
    for (i = 0; i < NUM_RECORD_TYPES; i++)
    {
        ConvertIntToDecimalStringN(gStringVar1, recordNums[i], STR_CONV_MODE_LEFT_ALIGN, sRecordNumMaxDigits[i]);
        numWidth = GetStringWidth(FONT_NORMAL, gStringVar1, -1);
        AddTextPrinterParameterized(windowId, FONT_NORMAL, sRecordsTexts[i], 0, sRecordTextYCoords[i][0], TEXT_SKIP_DRAW, NULL);
        x = (width * 8) - numWidth;
        AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar1, x, sRecordNumYCoords[i][0], TEXT_SKIP_DRAW, NULL);
    }
    PutWindowTilemap(windowId);
}

// Debug functions?
static const u16 sDebug_BerryResults[MAX_RFU_PLAYERS][4] =
{
    {
        [BERRY_BLUE]   = MAX_BERRIES,
        [BERRY_GREEN]  = 0,
        [BERRY_GOLD]   = 90,
        [BERRY_MISSED] = MAX_BERRIES
    },
    {
        [BERRY_BLUE]   = MAX_BERRIES,
        [BERRY_GREEN]  = MAX_BERRIES,
        [BERRY_GOLD]   = 70,
        [BERRY_MISSED] = MAX_BERRIES
    },
    {
        [BERRY_BLUE]   = MAX_BERRIES,
        [BERRY_GREEN]  = 0,
        [BERRY_GOLD]   = MAX_BERRIES,
        [BERRY_MISSED] = 0
    },
    {
        [BERRY_BLUE]   = MAX_BERRIES,
        [BERRY_GREEN]  = MAX_BERRIES,
        [BERRY_GOLD]   = 60,
        [BERRY_MISSED] = 0
    },
    {
        [BERRY_BLUE]   = MAX_BERRIES,
        [BERRY_GREEN]  = MAX_BERRIES,
        [BERRY_GOLD]   = MAX_BERRIES,
        [BERRY_MISSED] = 0
    },
};

static const u8 sJPText_Vowels[] = _("あいうえおかき");
static const u8 sText_Letters[] = _("ABCDEFG");
static const u8 sText_Digits[] = _("0123456");

static const u8 *const sDebug_PlayerNames[] =
{
    sJPText_Vowels,
    sJPText_Vowels,
    sJPText_Vowels,
    sText_Letters,
    sText_Digits
};

static void UNUSED Debug_UpdateNumPlayers(void)
{
    sGame->numPlayers = GetLinkPlayerCount();
}

static void UNUSED Debug_SetPlayerNamesAndResults(void)
{
    u8 i, playerId;

    for (playerId = sGame->numPlayers; playerId < ARRAY_COUNT(sDebug_PlayerNames); playerId++)
        StringCopy(gLinkPlayers[playerId].name, sDebug_PlayerNames[playerId]);

    sGame->numPlayers = MAX_RFU_PLAYERS;
    for (i = 0; i < NUM_BERRY_TYPES; i++)
    {
        for (playerId = 0; playerId < sGame->numPlayers; playerId++)
            sGame->berryResults[playerId][i] = sDebug_BerryResults[playerId][i];
    }
}

struct ReadyToStartPacket
{
    u8 id;
    bool8 ALIGNED(4) ready;
};

static void SendPacket_ReadyToStart(bool32 ready)
{
    struct ReadyToStartPacket packet;
    packet.id = PACKET_READY_START;
    packet.ready = ready;
    Rfu_SendPacket(&packet);
}

static u32 RecvPacket_ReadyToStart(u32 playerId)
{
    struct ReadyToStartPacket *packet;

    if ((gRecvCmds[0][0] & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
        return FALSE;

    packet = (void *)&gRecvCmds[playerId][1];
    if (packet->id == PACKET_READY_START)
        return packet->ready;

    return FALSE;
}

struct GameStatePacket
{
    u8 id;
    u8 fallDist_Col0:4;
    u8 fallDist_Col1:4;
    u16 fallDist_Col2:4;
    u16 fallDist_Col3:4;
    u16 fallDist_Col4:4;
    u16 fallDist_Col5:4;
    u16 fallDist_Col6:4;
    u16 fallDist_Col7:4;
    u16 fallDist_Col8:4;
    u16 fallDist_Col9:4;
    u16 berryId_Col0:2;
    u16 berryId_Col1:2;
    u16 berryId_Col2:2;
    u16 berryId_Col3:2;
    u16 berryId_Col4:2;
    u16 berryId_Col5:2;
    u16 berryId_Col6:2;
    u16 berryId_Col7:2;
    u8 berryId_Col8:2;
    u8 berryId_Col9:2;
    u8 pickState_Player1:2;
    u8 pickState_Player2:2;
    u8 pickState_Player3:2;
    u8 pickState_Player4:2;
    u8 pickState_Player5:2;
    bool8 ateBerry_Player1:1;
    bool8 ateBerry_Player2:1;
    bool8 ateBerry_Player3:1;
    bool8 ateBerry_Player4:1;
    bool8 ateBerry_Player5:1;
    u8 numGraySquares:5;
    bool8 allReadyToEnd:1;
    bool8 berriesFalling:1;
    bool8 missedBerry_Player1:1;
    bool8 missedBerry_Player2:1;
    bool8 missedBerry_Player3:1;
    bool8 missedBerry_Player4:1;
    bool8 missedBerry_Player5:1;
};

static void SendPacket_GameState(struct DodrioGame_Player *player,
                                 struct DodrioGame_PlayerCommData *player1,
                                 struct DodrioGame_PlayerCommData *player2,
                                 struct DodrioGame_PlayerCommData *player3,
                                 struct DodrioGame_PlayerCommData *player4,
                                 struct DodrioGame_PlayerCommData *player5,
                                 u8 numGraySquares,
                                 bool32 berriesFalling,
                                 bool32 allReadyToEnd)
{
    struct GameStatePacket packet;
    struct DodrioGame_Berries *berries = &player->berries;

    packet.id = PACKET_GAME_STATE;
    packet.fallDist_Col0 = berries->fallDist[0];
    packet.fallDist_Col1 = berries->fallDist[1];
    packet.fallDist_Col2 = berries->fallDist[2];
    packet.fallDist_Col3 = berries->fallDist[3];
    packet.fallDist_Col4 = berries->fallDist[4];
    packet.fallDist_Col5 = berries->fallDist[5];
    packet.fallDist_Col6 = berries->fallDist[6];
    packet.fallDist_Col7 = berries->fallDist[7];
    packet.fallDist_Col8 = berries->fallDist[8];
    packet.fallDist_Col9 = berries->fallDist[9];

    packet.berryId_Col0 = berries->ids[0];
    packet.berryId_Col1 = berries->ids[1];
    packet.berryId_Col2 = berries->ids[2];
    packet.berryId_Col3 = berries->ids[3];
    packet.berryId_Col4 = berries->ids[4];
    packet.berryId_Col5 = berries->ids[5];
    packet.berryId_Col6 = berries->ids[6];
    packet.berryId_Col7 = berries->ids[7];
    packet.berryId_Col8 = berries->ids[8];
    packet.berryId_Col9 = berries->ids[9];

    packet.pickState_Player1 = player1->pickState;
    packet.pickState_Player2 = player2->pickState;
    packet.pickState_Player3 = player3->pickState;
    packet.pickState_Player4 = player4->pickState;
    packet.pickState_Player5 = player5->pickState;

    packet.ateBerry_Player1 = player1->ateBerry;
    packet.ateBerry_Player2 = player2->ateBerry;
    packet.ateBerry_Player3 = player3->ateBerry;
    packet.ateBerry_Player4 = player4->ateBerry;
    packet.ateBerry_Player5 = player5->ateBerry;

    packet.missedBerry_Player1 = player1->missedBerry;
    packet.missedBerry_Player2 = player2->missedBerry;
    packet.missedBerry_Player3 = player3->missedBerry;
    packet.missedBerry_Player4 = player4->missedBerry;
    packet.missedBerry_Player5 = player5->missedBerry;

    packet.numGraySquares = numGraySquares;
    packet.berriesFalling = berriesFalling;
    packet.allReadyToEnd = allReadyToEnd;
    Rfu_SendPacket(&packet);
}

static bool32 RecvPacket_GameState(u32 playerId,
                                   struct DodrioGame_Player *player,
                                   struct DodrioGame_PlayerCommData *player1,
                                   struct DodrioGame_PlayerCommData *player2,
                                   struct DodrioGame_PlayerCommData *player3,
                                   struct DodrioGame_PlayerCommData *player4,
                                   struct DodrioGame_PlayerCommData *player5,
                                   u8 *numGraySquares,
                                   bool32 *berriesFalling,
                                   bool32 *allReadyToEnd)
{
    struct GameStatePacket *packet;
    struct DodrioGame_Berries *berries = &player->berries;

    if ((gRecvCmds[0][0] & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
        return FALSE;

    packet = (void *)&gRecvCmds[0][1];
    if (packet->id == PACKET_GAME_STATE)
    {
        berries->fallDist[0] = packet->fallDist_Col0;
        berries->fallDist[1] = packet->fallDist_Col1;
        berries->fallDist[2] = packet->fallDist_Col2;
        berries->fallDist[3] = packet->fallDist_Col3;
        berries->fallDist[4] = packet->fallDist_Col4;
        berries->fallDist[5] = packet->fallDist_Col5;
        berries->fallDist[6] = packet->fallDist_Col6;
        berries->fallDist[7] = packet->fallDist_Col7;
        berries->fallDist[8] = packet->fallDist_Col8;
        berries->fallDist[9] = packet->fallDist_Col9;
        berries->fallDist[10] = packet->fallDist_Col0;

        berries->ids[0] = packet->berryId_Col0;
        berries->ids[1] = packet->berryId_Col1;
        berries->ids[2] = packet->berryId_Col2;
        berries->ids[3] = packet->berryId_Col3;
        berries->ids[4] = packet->berryId_Col4;
        berries->ids[5] = packet->berryId_Col5;
        berries->ids[6] = packet->berryId_Col6;
        berries->ids[7] = packet->berryId_Col7;
        berries->ids[8] = packet->berryId_Col8;
        berries->ids[9] = packet->berryId_Col9;
        berries->ids[10] = packet->berryId_Col0;

        player1->pickState = packet->pickState_Player1;
        player1->ateBerry = packet->ateBerry_Player1;
        player1->missedBerry = packet->missedBerry_Player1;

        player2->pickState = packet->pickState_Player2;
        player2->ateBerry = packet->ateBerry_Player2;
        player2->missedBerry = packet->missedBerry_Player2;

        player3->pickState = packet->pickState_Player3;
        player3->ateBerry = packet->ateBerry_Player3;
        player3->missedBerry = packet->missedBerry_Player3;

        player4->pickState = packet->pickState_Player4;
        player4->ateBerry = packet->ateBerry_Player4;
        player4->missedBerry = packet->missedBerry_Player4;

        player5->pickState = packet->pickState_Player5;
        player5->ateBerry = packet->ateBerry_Player5;
        player5->missedBerry = packet->missedBerry_Player5;

        *numGraySquares = packet->numGraySquares;
        *berriesFalling = packet->berriesFalling;
        *allReadyToEnd = packet->allReadyToEnd;
        return TRUE;
    }

    return FALSE;
}

struct PickStatePacket
{
    u8 id;
    u8 ALIGNED(4) pickState;
};

static void SendPacket_PickState(u8 pickState)
{
    struct PickStatePacket packet;
    packet.id = PACKET_PICK_STATE;
    packet.pickState = pickState;
    Rfu_SendPacket(&packet);
}

static bool32 RecvPacket_PickState(u32 playerId, u8 *pickState)
{
    struct PickStatePacket *packet;

    if ((gRecvCmds[0][0] & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
        return FALSE;

    packet = (void *)&gRecvCmds[playerId][1];
    if (packet->id == PACKET_PICK_STATE)
    {
        *pickState = packet->pickState;
        return TRUE;
    }

    return FALSE;
}

struct ReadyToEndPacket
{
    u8 id;
    bool32 ready;
};

static void SendPacket_ReadyToEnd(bool32 ready)
{
    struct ReadyToEndPacket packet;
    packet.id = PACKET_READY_END;
    packet.ready = ready;
    Rfu_SendPacket(&packet);
}

static bool32 RecvPacket_ReadyToEnd(u32 playerId)
{
    struct ReadyToEndPacket *packet;

    if ((gRecvCmds[0][0] & RFUCMD_MASK) != RFUCMD_SEND_PACKET)
        return FALSE;

    packet = (void *)&gRecvCmds[playerId][1];
    if (packet->id == PACKET_READY_END)
        return packet->ready;

    return FALSE;
}

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = BG_INTERFACE,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = BG_TREE_LEFT,
        .charBaseIndex = 2,
        .mapBaseIndex = 12,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = BG_TREE_RIGHT,
        .charBaseIndex = 2,
        .mapBaseIndex = 14,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = BG_SCENERY,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct WindowTemplate sWindowTemplate_Dummy = DUMMY_WIN_TEMPLATE;

static const struct WindowTemplate sWindowTemplates_Results[] =
{
    {
        .bg = BG_INTERFACE,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 28,
        .height = 2,
        .paletteNum = 13,
        .baseBlock = 0x13,
    },
    {
        .bg = BG_INTERFACE,
        .tilemapLeft = 1,
        .tilemapTop = 5,
        .width = 28,
        .height = 14,
        .paletteNum = 13,
        .baseBlock = 0x4B,
    }
};

static const struct WindowTemplate sWindowTemplate_Prize =
{
    .bg = BG_INTERFACE,
    .tilemapLeft = 1,
    .tilemapTop = 5,
    .width = 28,
    .height = 7,
    .paletteNum = 13,
    .baseBlock = 0x4B,
};

enum {
    WIN_PLAY_AGAIN,
    WIN_YES_NO,
};

static const struct WindowTemplate sWindowTemplates_PlayAgain[] =
{
    [WIN_PLAY_AGAIN] = {
        .bg = BG_INTERFACE,
        .tilemapLeft = 1,
        .tilemapTop = 8,
        .width = 19,
        .height = 3,
        .paletteNum = 13,
        .baseBlock = 0x13,
    },
    [WIN_YES_NO] = {
        .bg = BG_INTERFACE,
        .tilemapLeft = 22,
        .tilemapTop = 7,
        .width = 6,
        .height = 4,
        .paletteNum = 13,
        .baseBlock = 0x4C,
    }
};

static const struct WindowTemplate sWindowTemplate_DroppedOut =
{
    .bg = BG_INTERFACE,
    .tilemapLeft = 4,
    .tilemapTop = 6,
    .width = 22,
    .height = 5,
    .paletteNum = 13,
    .baseBlock = 0x13,
};

static const struct WindowTemplate sWindowTemplate_CommStandby =
{
    .bg = BG_INTERFACE,
    .tilemapLeft = 5,
    .tilemapTop = 8,
    .width = 19,
    .height = 3,
    .paletteNum = 13,
    .baseBlock = 0x13,
};

// Unused duplicate of sActiveColumnMap
static const u8 sActiveColumnMap_Duplicate[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][NUM_BERRY_COLUMNS] =
{
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0},
    },
    {
        {0, 1, 2, 3, 4, 5, 6, 3, 8, 9, 0},
        {0, 1, 2, 5, 6, 3, 4, 5, 8, 9, 0},
    },
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 2, 9, 0},
        {0, 1, 4, 5, 6, 7, 2, 3, 4, 9, 0},
        {0, 1, 6, 7, 2, 3, 4, 5, 6, 9, 0},
    },
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 1, 0},
        {0, 3, 4, 5, 6, 7, 8, 1, 2, 3, 0},
        {0, 5, 6, 7, 8, 1, 2, 3, 4, 5, 0},
        {0, 7, 8, 1, 2, 3, 4, 5, 6, 7, 0},
    },
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0},
        {2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2},
        {4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4},
        {6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6},
        {8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8},
    },
};

// Unused duplicate of sDodrioHeadToColumnMap
static const u8 sDodrioHeadToColumnMap_Duplicate[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][3] =
{
    {
        {4, 5, 6},
    },
    {
        {3, 4, 5},
        {5, 6, 3},
    },
    {
        {4, 5, 6},
        {6, 7, 2},
        {2, 3, 4},
    },
    {
        {3, 4, 5},
        {5, 6, 7},
        {7, 8, 1},
        {1, 2, 3},
    },
    {
        {4, 5, 6},
        {6, 7, 8},
        {8, 9, 0},
        {0, 1, 2},
        {2, 3, 4},
    },
};

// Unused duplicate of sDodrioNeighborMap
static const u8 sDodrioNeighborMap_Duplicate[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS][3] =
{
    {
        {1, 0, 1},
    },
    {
        {1, 0, 1},
        {0, 1, 0},
    },
    {
        {2, 0, 1},
        {0, 1, 2},
        {1, 2, 0},
    },
    {
        {3, 0, 1},
        {0, 1, 2},
        {1, 2, 3},
        {2, 3, 0},
    },
    {
        {4, 0, 1},
        {0, 1, 2},
        {1, 2, 3},
        {2, 3, 4},
        {3, 4, 0},
    },
};

// Unused duplicate of sPlayerIdAtColumn
ALIGNED(4)
static const u8 sPlayerIdAtColumn_Duplicate[MAX_RFU_PLAYERS][NUM_BERRY_COLUMNS] =
{
    {9, 9, 9, 9, 1, 1, 1, 9, 9, 9, 9},
    {9, 9, 9, 0, 0, 1, 1, 0, 9, 9, 9},
    {9, 9, 2, 2, 0, 0, 1, 1, 1, 9, 9},
    {9, 3, 3, 0, 0, 1, 1, 2, 2, 3, 9},
    {3, 3, 4, 4, 0, 0, 1, 1, 2, 2, 3},
};

// Unused duplicate of sUnsharedColumns
static const u8 sUnsharedColumns_Duplicate[MAX_RFU_PLAYERS][MAX_RFU_PLAYERS] =
{
    {5},
    {4, 6},
    {3, 5, 7},
    {2, 4, 6, 8},
    {1, 3, 5, 6, 9},
};

static const u16 sBg_Pal[]                  = INCBIN_U16("graphics/dodrio_berry_picking/tree_border.gbapal",
                                                         "graphics/dodrio_berry_picking/bg.gbapal");
static const u16 sDodrioNormal_Pal[]        = INCBIN_U16("graphics/dodrio_berry_picking/dodrio.gbapal");
static const u16 sDodrioShiny_Pal[]         = INCBIN_U16("graphics/dodrio_berry_picking/shiny.gbapal");
static const u16 sStatus_Pal[]              = INCBIN_U16("graphics/dodrio_berry_picking/status.gbapal");
static const u16 sBerries_Pal[]             = INCBIN_U16("graphics/dodrio_berry_picking/berries.gbapal");
static const u32 sBerries_Gfx[]             = INCBIN_U32("graphics/dodrio_berry_picking/berries.4bpp.lz");
static const u16 sCloud_Pal[]               = INCBIN_U16("graphics/dodrio_berry_picking/cloud.gbapal");
static const u32 sBg_Gfx[]                  = INCBIN_U32("graphics/dodrio_berry_picking/bg.4bpp.lz");
static const u32 sTreeBorder_Gfx[]          = INCBIN_U32("graphics/dodrio_berry_picking/tree_border.4bpp.lz");
static const u32 sStatus_Gfx[]              = INCBIN_U32("graphics/dodrio_berry_picking/status.4bpp.lz");
static const u32 sCloud_Gfx[]               = INCBIN_U32("graphics/dodrio_berry_picking/cloud.4bpp.lz");
static const u32 sDodrio_Gfx[]              = INCBIN_U32("graphics/dodrio_berry_picking/dodrio.4bpp.lz");
static const u32 sBg_Tilemap[]              = INCBIN_U32("graphics/dodrio_berry_picking/bg.bin.lz");
static const u32 sTreeBorderRight_Tilemap[] = INCBIN_U32("graphics/dodrio_berry_picking/tree_border_right.bin.lz");
static const u32 sTreeBorderLeft_Tilemap[]  = INCBIN_U32("graphics/dodrio_berry_picking/tree_border_left.bin.lz");

static const struct OamData sOamData_Dodrio =
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
    .affineParam = 0
};

// Used by the status bar and the results screen berry icons
static const struct OamData sOamData_16x16_Priority0 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct OamData sOamData_Berry =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct OamData sOamData_Cloud =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 3,
    .paletteNum = 0,
    .affineParam = 0
};

static const union AnimCmd sAnim_Dodrio_Normal[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Dodrio_PickRight[] =
{
    ANIMCMD_FRAME(64, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Dodrio_PickMiddle[] =
{
    ANIMCMD_FRAME(128, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Dodrio_PickLeft[] =
{
    ANIMCMD_FRAME(192, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Dodrio_Down[] =
{
    ANIMCMD_FRAME(256, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Dodrio[] =
{
    [PICK_NONE]     = sAnim_Dodrio_Normal,
    [PICK_RIGHT]    = sAnim_Dodrio_PickRight,
    [PICK_MIDDLE]   = sAnim_Dodrio_PickMiddle,
    [PICK_LEFT]     = sAnim_Dodrio_PickLeft,
    [PICK_DISABLED] = sAnim_Dodrio_Down,
    // There is an unused 6th frame of Dodrio's graphic
};

static const union AnimCmd sAnims_StatusBar_Yellow[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnims_StatusBar_Gray[] =
{
    ANIMCMD_FRAME(4, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnims_StatusBar_Red[] =
{
    ANIMCMD_FRAME(8, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_StatusBar[] =
{
    [STATUS_YELLOW] = sAnims_StatusBar_Yellow,
    [STATUS_GRAY]   = sAnims_StatusBar_Gray,
    [STATUS_RED]    = sAnims_StatusBar_Red
};

static const union AnimCmd sAnim_Berry_Blue[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_Green[] =
{
    ANIMCMD_FRAME(4, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_Gold[] =
{
    ANIMCMD_FRAME(8, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_BlueSquished[] =
{
    ANIMCMD_FRAME(12, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_GreenSquished[] =
{
    ANIMCMD_FRAME(16, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_GoldSquished[] =
{
    ANIMCMD_FRAME(20, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_Eaten[] =
{
    ANIMCMD_FRAME(24, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_Empty1[] =
{
    ANIMCMD_FRAME(28, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Berry_Empty2[] =
{
    ANIMCMD_FRAME(32, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Berry[] =
{
    [BERRY_BLUE]  = sAnim_Berry_Blue,
    [BERRY_GREEN] = sAnim_Berry_Green,
    [BERRY_GOLD]  = sAnim_Berry_Gold,

    [BERRY_BLUE + BERRY_MISSED]  = sAnim_Berry_BlueSquished,
    [BERRY_GREEN + BERRY_MISSED] = sAnim_Berry_GreenSquished,
    [BERRY_GOLD + BERRY_MISSED]  = sAnim_Berry_GoldSquished,

    [ANIM_EATEN]  = sAnim_Berry_Eaten,

    sAnim_Berry_Empty1,
    sAnim_Berry_Empty2
};

static const union AnimCmd sAnim_Cloud[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Cloud[] =
{
    sAnim_Cloud
};

static void LoadDodrioGfx(void)
{
    void *ptr = AllocZeroed(0x3000);
    struct SpritePalette normal = {sDodrioNormal_Pal, PALTAG_DODRIO_NORMAL};
    struct SpritePalette shiny = {sDodrioShiny_Pal, PALTAG_DODRIO_SHINY};

    LZ77UnCompWram(sDodrio_Gfx, ptr);
    if (ptr)
    {
        struct SpriteSheet sheet = {ptr, 0x3000, GFXTAG_DODRIO};
        LoadSpriteSheet(&sheet);
        Free(ptr);
    }
    LoadSpritePalette(&normal);
    LoadSpritePalette(&shiny);
}

static void CreateDodrioSprite(struct DodrioGame_MonInfo * monInfo, u8 playerId, u8 id, u8 numPlayers)
{
    struct SpriteTemplate template =
    {
        .tileTag = GFXTAG_DODRIO,
        .paletteTag = monInfo->isShiny, // PALTAG_DODRIO_NORMAL / PALTAG_DODRIO_SHINY
        .oam = &sOamData_Dodrio,
        .anims = sAnims_Dodrio,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_Dodrio,
    };

    sDodrioSpriteIds[id] = AllocZeroed(4);
    *sDodrioSpriteIds[id] = CreateSprite(&template, GetDodrioXPos(playerId, numPlayers), 136, 3);
    SetDodrioInvisibility(TRUE, id);
}

#define sState   data[0]
#define sTimer   data[1]
#define sUnused1 data[2]
#define sUnused2 data[3]
#define sUnused3 data[4]

static void SpriteCB_Dodrio(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        break;
    case 1:
        DoDodrioMissedAnim(sprite);
        break;
    case 2:
        DoDodrioIntroAnim(sprite);
        break;
    }
}

static void StartDodrioMissedAnim(u8 unused)
{
    struct Sprite *sprite = &gSprites[*sDodrioSpriteIds[GetMultiplayerId()]];
    sprite->sState = 1;
    sprite->sTimer = 0;
    sprite->sUnused1 = 0;
    sprite->sUnused2 = 0;
    sprite->sUnused3 = 0;
}

static void StartDodrioIntroAnim(u8 unused)
{
    struct Sprite *sprite = &gSprites[*sDodrioSpriteIds[GetMultiplayerId()]];
    sprite->sState = 2;
    sprite->sTimer = 0;
    sprite->sUnused1 = 0;
    sprite->sUnused2 = 0;
    sprite->sUnused3 = 0;
}

// Do animation where Dodrio shakes horizontally after reaching for a berry and missing
static u32 DoDodrioMissedAnim(struct Sprite *sprite)
{
    s8 x;
    u8 state = (++sprite->sTimer / 2) % 4;

    if (sprite->sTimer >= 3)
    {
        switch (state)
        {
        default:
            x = 1;
            break;
        case 1:
        case 2:
            x = -1;
            break;
        }

        sprite->x += x;
        if (++sprite->sTimer >= 40)
        {
            sprite->sState = 0;
            sprite->x = GetDodrioXPos(0, GetNumPlayers());
        }
    }

    return 0;
}

// Does the intro animation where the player's Dodrio
// cycles through extending each head twice
#define FRAMES_PER_STATE  13
#define NUM_INTRO_PICK_STATES PICK_DISABLED // Cycle through 'Normal' and each head, but exclude the Disabled state

static u32 DoDodrioIntroAnim(struct Sprite *sprite)
{
    u8 pickState = (++sprite->sTimer / FRAMES_PER_STATE) % NUM_INTRO_PICK_STATES;

    // Play a sound effect at the start of each head extension
    if (sprite->sTimer % FRAMES_PER_STATE == 0 && pickState != PICK_NONE)
        PlaySE(SE_M_CHARM);

    if (sprite->sTimer >= FRAMES_PER_STATE * NUM_INTRO_PICK_STATES * 2)
    {
        // End animation
        sprite->sState = 0;
        pickState = PICK_NONE;
    }
    SetDodrioAnim(GetMultiplayerId(), pickState);
    return 0;
}

#undef sState
#undef sTimer
#undef sUnused1
#undef sUnused2
#undef sUnused3

static void FreeDodrioSprites(u8 numPlayers)
{
    u8 i;
    for (i = 0; i < numPlayers; i++)
    {
        struct Sprite *sprite = &gSprites[*sDodrioSpriteIds[i]];
        if (sprite)
            DestroySpriteAndFreeResources(sprite);
#ifdef BUGFIX
        FREE_AND_SET_NULL(sDodrioSpriteIds[i]); // Memory should be freed here but is not.
#endif
    }
}

static void SetDodrioInvisibility(bool8 invisible, u8 id)
{
    gSprites[*sDodrioSpriteIds[id]].invisible = invisible;
}

static void SetAllDodrioInvisibility(bool8 invisible, u8 count)
{
    u8 i;
    for (i = 0; i < count; i++)
        SetDodrioInvisibility(invisible, i);
}

static void SetDodrioAnim(u8 id, u8 pickState)
{
    StartSpriteAnim(&gSprites[*sDodrioSpriteIds[id]], pickState);
}

static void SpriteCB_Status(struct Sprite *sprite)
{

}

static void InitStatusBarPos(void)
{
    u8 i;
    for (i = 0; i < NUM_STATUS_SQUARES; i++)
    {
        struct Sprite *sprite = &gSprites[sStatusBar->spriteIds[i]];
        sprite->x = (i * 16) + 48;
        sprite->y = -8 - (i * 8);
        sStatusBar->entered[i] = FALSE;
    }
}

static void CreateStatusBarSprites(void)
{
    u8 i;
    void *ptr = AllocZeroed(0x180);
    struct SpritePalette pal = {sStatus_Pal, PALTAG_STATUS};

    LZ77UnCompWram(sStatus_Gfx, ptr);
    // This check should be one line up.
    if (ptr)
    {
        struct SpriteSheet sheet = {ptr, 0x180, GFXTAG_STATUS};
        struct SpriteTemplate template =
        {
            .tileTag = GFXTAG_STATUS,
            .paletteTag = PALTAG_STATUS,
            .oam = &sOamData_16x16_Priority0,
            .anims = sAnims_StatusBar,
            .images = NULL,
            .affineAnims = gDummySpriteAffineAnimTable,
            .callback = SpriteCB_Status,
        };

        sStatusBar = AllocZeroed(sizeof(*sStatusBar));
        LoadSpriteSheet(&sheet);
        LoadSpritePalette(&pal);
        for (i = 0; i < NUM_STATUS_SQUARES; i++)
            sStatusBar->spriteIds[i] = CreateSprite(&template, (i * 16) + 48, -8 - (i * 8), 0);
    }

    Free(ptr);
}

static void FreeStatusBar(void)
{
    u8 i;
    for (i = 0; i < NUM_STATUS_SQUARES; i++)
    {
        struct Sprite *sprite = &gSprites[sStatusBar->spriteIds[i]];
        if (sprite)
            DestroySpriteAndFreeResources(sprite);
    }
    FREE_AND_SET_NULL(sStatusBar);
}

// Progress an animation where each square of the
// status bar drops down into view, bounces up,
// then settles into position.
// Returns TRUE if the animation is complete
static bool32 DoStatusBarIntro(void)
{
    u8 i;
    bool32 animActive = FALSE;
    for (i = 0; i < NUM_STATUS_SQUARES; i++)
    {
        struct Sprite *sprite = &gSprites[sStatusBar->spriteIds[i]];
        sStatusBar->yChange[i] = 2;
        if (sStatusBar->entered[i] && sprite->y == 8)
            continue;

        animActive = TRUE;
        if (sprite->y == 8)
        {
            if (sStatusBar->entered[i])
                continue;

            // Square has entered screen, play click
            // sound and reverse direction
            sStatusBar->entered[i] = TRUE;
            sStatusBar->yChange[i] = -16;
            PlaySE(SE_CLICK);
        }
        sprite->y += sStatusBar->yChange[i];
    }

    if (animActive)
        return FALSE;
    else
        return TRUE;
}

// The status bar at the top changes color depending on the game performance.
// The squares start out yellow. For every berry missed, a square is colored gray.
// If there are 4 or fewer yellow squares left they also flash red
static void UpdateStatusBarAnim(u8 numEmpty)
{
    u8 i;

    if (numEmpty > NUM_STATUS_SQUARES)
    {
        // All squares gray
        for (i = 0; i < NUM_STATUS_SQUARES; i++)
            StartSpriteAnim(&gSprites[sStatusBar->spriteIds[i]], STATUS_GRAY);
    }
    else
    {
        // At least 1 square is yellow
        for (i = 0; i < NUM_STATUS_SQUARES - numEmpty; i++)
        {
            if (numEmpty > 6)
            {
                // Flash the yellow squares red
                // The flash cycles faster the fewer yellow squares remain
                sStatusBar->flashTimer += numEmpty - 6;
                if (sStatusBar->flashTimer > 30)
                    sStatusBar->flashTimer = 0;
                else if (sStatusBar->flashTimer > 10)
                    StartSpriteAnim(&gSprites[sStatusBar->spriteIds[i]], STATUS_RED);
                else
                    StartSpriteAnim(&gSprites[sStatusBar->spriteIds[i]], STATUS_YELLOW);
            }
            else
            {
                // Set yellow squares, no flash
                StartSpriteAnim(&gSprites[sStatusBar->spriteIds[i]], STATUS_YELLOW);
            }
        }

        // Set remaining squares gray
        for (; i < NUM_STATUS_SQUARES; i++)
            StartSpriteAnim(&gSprites[sStatusBar->spriteIds[i]], STATUS_GRAY);
    }
}

static void SetStatusBarInvisibility(bool8 invisible)
{
    u8 i;
    for (i = 0; i < NUM_STATUS_SQUARES; i++)
        gSprites[sStatusBar->spriteIds[i]].invisible = invisible;
}

static const u8 sUnusedSounds[] = {
    SE_M_CHARM,
    SE_NOTE_C,
    SE_NOTE_D,
    SE_NOTE_E,
    SE_NOTE_F,
    SE_NOTE_G,
    SE_NOTE_A,
    SE_NOTE_B,
    SE_NOTE_C_HIGH,
    SE_RG_CARD_OPEN
};

static void LoadBerryGfx(void)
{
    void *ptr = AllocZeroed(0x480);
    struct SpritePalette pal = {sBerries_Pal, PALTAG_BERRIES};

    LZ77UnCompWram(sBerries_Gfx, ptr);
    if (ptr)
    {
        struct SpriteSheet sheet = {ptr, 0x480, GFXTAG_BERRIES};
        LoadSpriteSheet(&sheet);
    }

    LoadSpritePalette(&pal);
    Free(ptr);
}

static const s16 sBerryIconXCoords[] = {88, 128, 168, 208};

static void CreateBerrySprites(void)
{
    u8 i;
    s16 x;

    struct SpriteTemplate berry =
    {
        .tileTag = GFXTAG_BERRIES,
        .paletteTag = PALTAG_BERRIES,
        .oam = &sOamData_Berry,
        .anims = sAnims_Berry,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    };
    struct SpriteTemplate berryIcon =
    {
        .tileTag = GFXTAG_BERRIES,
        .paletteTag = PALTAG_BERRIES,
        .oam = &sOamData_16x16_Priority0,
        .anims = sAnims_Berry,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    };

    // Create berry sprites that fall during gameplay
    for (i = 0; i < NUM_BERRY_COLUMNS; i++)
    {
        sBerrySpriteIds[i] = AllocZeroed(4);
        x = i * 16;
        *sBerrySpriteIds[i] = CreateSprite(&berry, x + (i * 8), 8, 1);
        SetBerryInvisibility(i, TRUE);
    }

    // Create berry icon sprites for results screen
    for (i = 0; i < NUM_BERRY_TYPES; i++)
    {
        sBerryIconSpriteIds[i] = AllocZeroed(4);
        if (i == BERRY_MISSED)
            *sBerryIconSpriteIds[i] = CreateSprite(&berryIcon, sBerryIconXCoords[i], 49, 0);
        else
            *sBerryIconSpriteIds[i] = CreateSprite(&berryIcon, sBerryIconXCoords[i], 52, 0);
        StartSpriteAnim(&gSprites[*sBerryIconSpriteIds[i]], i);
    }
    SetBerryIconsInvisibility(TRUE);
}

static void FreeBerrySprites(void)
{
    struct Sprite *sprite;
    u8 i;

    for (i = 0; i < NUM_BERRY_COLUMNS; i++)
    {
        sprite = &gSprites[*sBerrySpriteIds[i]];
        if (sprite)
            DestroySprite(sprite);
        FREE_AND_SET_NULL(sBerrySpriteIds[i]);
    }
    for (i = 0; i < NUM_BERRY_TYPES; i++)
    {
        sprite = &gSprites[*sBerryIconSpriteIds[i]];
        if (sprite)
            DestroySprite(sprite);
        FREE_AND_SET_NULL(sBerryIconSpriteIds[i]);
    }
}

static void SetBerryInvisibility(u8 id, bool8 invisible)
{
    gSprites[*sBerrySpriteIds[id]].invisible = invisible;
}

static void SetBerryIconsInvisibility(bool8 invisible)
{
    u8 i;
    for (i = 0; i < NUM_BERRY_TYPES; i++)
        gSprites[*sBerryIconSpriteIds[i]].invisible = invisible;
}

static void SetBerryYPos(u8 id, u8 y)
{
    gSprites[*sBerrySpriteIds[id]].y = y * 8;
}

static void SetBerryAnim(u16 id, u8 animNum)
{
    StartSpriteAnim(&gSprites[*sBerrySpriteIds[id]], animNum);
}

static void UNUSED UnusedSetSpritePos(u8 spriteId)
{
    gSprites[spriteId].x = 20 * spriteId + 50;
    gSprites[spriteId].y = 50;
}

// Gamefreak made a mistake there and goes out of bounds for the data array as it holds 8 elements
// in turn overwriting sprite's subpriority and subsprites fields.
#ifdef UBFIX
    #define sFrozen data[1]
#else
    #define sFrozen data[10]
#endif // UBFIX

static void SpriteCB_Cloud(struct Sprite *sprite)
{
    u8 i;
    static const u8 moveDelays[] = {30, 20};

    if (sprite->sFrozen != TRUE)
    {
        for (i = 0; i < NUM_CLOUDS; i++)
        {
            if (++sCloudSpriteIds[i][1] > moveDelays[i])
            {
                sprite->x--;
                sCloudSpriteIds[i][1] = 0;
            }
        }
    }
}

static const s16 sCloudStartCoords[NUM_CLOUDS][2] =
{
    {230, 55},
    { 30, 74}
};

static void CreateCloudSprites(void)
{
    u8 i;
    void *ptr = AllocZeroed(0x400);
    struct SpritePalette pal = {sCloud_Pal, PALTAG_CLOUD};

    LZ77UnCompWram(sCloud_Gfx, ptr);
    if (ptr)
    {
        struct SpriteSheet sheet = {ptr, 0x400, GFXTAG_CLOUD};
        struct SpriteTemplate template =
        {
            .tileTag = GFXTAG_CLOUD,
            .paletteTag = PALTAG_CLOUD,
            .oam = &sOamData_Cloud,
            .anims = sAnims_Cloud,
            .images = NULL,
            .affineAnims = gDummySpriteAffineAnimTable,
            .callback = SpriteCB_Cloud,
        };

        LoadSpriteSheet(&sheet);
        LoadSpritePalette(&pal);
        for (i = 0; i < NUM_CLOUDS; i++)
        {
            sCloudSpriteIds[i] = AllocZeroed(4);
            *sCloudSpriteIds[i] = CreateSprite(&template, sCloudStartCoords[i][0], sCloudStartCoords[i][1], 4);
        }
    }

    Free(ptr);
}

static void ResetCloudPos(void)
{
    u8 i;
    for (i = 0; i < NUM_CLOUDS; i++)
    {
        struct Sprite *sprite = &gSprites[*sCloudSpriteIds[i]];
        sprite->sFrozen = TRUE;
        sprite->x = sCloudStartCoords[i][0];
        sprite->y = sCloudStartCoords[i][1];
    }
}

static void StartCloudMovement(void)
{
    u8 i;
    for (i = 0; i < NUM_CLOUDS; i++)
    {
        struct Sprite *sprite = &gSprites[*sCloudSpriteIds[i]];
        sprite->sFrozen = FALSE;
    }
}

static void FreeCloudSprites(void)
{
    u8 i;
    for (i = 0; i < NUM_CLOUDS; i++)
    {
        struct Sprite *sprite = &gSprites[*sCloudSpriteIds[i]];
        if (sprite)
            DestroySprite(sprite);
        FREE_AND_SET_NULL(sCloudSpriteIds[i]);
    }
}

static void SetCloudInvisibility(bool8 invisible)
{
    u8 i;
    for (i = 0; i < NUM_CLOUDS; i++)
        gSprites[*sCloudSpriteIds[i]].invisible = invisible;
}

#undef sFrozen

static s16 GetDodrioXPos(u8 playerId, u8 numPlayers)
{
    s16 x = 0;
    switch (numPlayers)
    {
    case 1:
        x = 15;
        break;
    case 2:
        switch (playerId)
        {
            case 0: x = 12; break;
            case 1: x = 18; break;
        }
        break;
    case 3:
        switch (playerId)
        {
            case 0: x = 15; break;
            case 1: x = 21; break;
            case 2: x =  9; break;
        }
        break;
    case 4:
        switch (playerId)
        {
            case 0: x = 12; break;
            case 1: x = 18; break;
            case 2: x = 24; break;
            case 3: x =  6; break;
        }
        break;
    case 5:
        switch (playerId)
        {
            case 0: x = 15; break;
            case 1: x = 21; break;
            case 2: x = 27; break;
            case 3: x =  3; break;
            case 4: x =  9; break;
        }
        break;
    }

    return x * 8;
}

static void ResetBerryAndStatusBarSprites(void)
{
    u8 i;
    for (i = 0; i < NUM_BERRY_COLUMNS; i++)
    {
        SetBerryInvisibility(i, TRUE);
        SetBerryYPos(i, 1);
    }
    SetStatusBarInvisibility(FALSE);
}

static void LoadWindowFrameGfx(u8 frameId)
{
    LoadBgTiles(BG_INTERFACE, GetWindowFrameTilesPal(frameId)->tiles, 0x120, 1);
    LoadPalette(GetWindowFrameTilesPal(frameId)->pal, BG_PLTT_ID(10), PLTT_SIZE_4BPP);
}

static void LoadUserWindowFrameGfx(void)
{
    LoadUserWindowBorderGfx_(0, 0xA, BG_PLTT_ID(11));
}

static void ResetGfxState(void)
{
    sGfx->finished = FALSE;
    sGfx->state = 0;
    sGfx->loadState = 0;
    sGfx->cursorSelection = 0;
    sGfx->playAgainState = PLAY_AGAIN_NONE;
}

static void DrawYesNoMessageWindow(const struct WindowTemplate *template)
{
    u8 pal = 10;

    FillBgTilemapBufferRect(BG_INTERFACE, 1, template->tilemapLeft - 1,                template->tilemapTop - 1,                   1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 2, template->tilemapLeft,                    template->tilemapTop - 1,                   template->width, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 3, template->tilemapLeft + template->width,  template->tilemapTop - 1,                   1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 4, template->tilemapLeft - 1,                template->tilemapTop, 1,                    template->height, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 6, template->tilemapLeft + template->width,  template->tilemapTop, 1,                    template->height, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 7, template->tilemapLeft - 1,                template->tilemapTop + template->height,    1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 8, template->tilemapLeft,                    template->tilemapTop + template->height,    template->width, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 9, template->tilemapLeft + template->width,  template->tilemapTop + template->height,    1, 1, pal);
}

static void DrawMessageWindow(const struct WindowTemplate *template)
{
    u8 pal = 11;

    FillBgTilemapBufferRect(BG_INTERFACE, 10, template->tilemapLeft - 1,                template->tilemapTop - 1,                   1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 11, template->tilemapLeft,                    template->tilemapTop - 1,                   template->width, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 12, template->tilemapLeft + template->width,  template->tilemapTop - 1,                   1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 13, template->tilemapLeft - 1,                template->tilemapTop, 1,                    template->height, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 15, template->tilemapLeft + template->width,  template->tilemapTop, 1,                    template->height, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 16, template->tilemapLeft - 1,                template->tilemapTop + template->height,    1, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 17, template->tilemapLeft,                    template->tilemapTop + template->height,    template->width, 1, pal);
    FillBgTilemapBufferRect(BG_INTERFACE, 18, template->tilemapLeft + template->width,  template->tilemapTop + template->height,    1, 1, pal);
}

static void InitGameGfx(struct DodrioGame_Gfx *ptr)
{
    sGfx = ptr;
    sGfx->finished = FALSE;
    sGfx->state = 0;
    sGfx->loadState = 0;
    sGfx->cursorSelection = 0;
    sGfx->playAgainState = PLAY_AGAIN_NONE;
    sGfx->taskId = CreateTask(Task_TryRunGfxFunc, 3);
    SetGfxFunc(LoadGfx);
}

static void UNUSED FreeAllWindowBuffers_(void)
{
    FreeAllWindowBuffers();
}

// Data used by functions below.
struct WinCoords
{
    u8 left;
    u8 top;
};

enum {
    COLORID_GRAY,
    COLORID_RED,
    COLORID_BLUE,
    COLORID_GREEN, // Unused
};

static const u8 sTextColorTable[][3] =
{
    [COLORID_GRAY]  = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY},
    [COLORID_RED]   = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED},
    [COLORID_BLUE]  = {TEXT_COLOR_WHITE, TEXT_COLOR_BLUE, TEXT_COLOR_LIGHT_BLUE},
    [COLORID_GREEN] = {TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN},
};

static const struct WinCoords sNameWindowCoords_1Player[] = {{12, 6}};
static const struct WinCoords sNameWindowCoords_2Players[] = {{9, 10}, {15, 6}};
static const struct WinCoords sNameWindowCoords_3Players[] = {{12, 6}, {18, 10}, {6, 10}};
static const struct WinCoords sNameWindowCoords_4Players[] = {{9, 10}, {15, 6}, {21, 10}, {3, 6}};
static const struct WinCoords sNameWindowCoords_5Players[] = {{12, 6}, {18, 10}, {23, 6}, {1, 6}, {6, 10}};

static const struct WinCoords *const sNameWindowCoords[MAX_RFU_PLAYERS] =
{
    sNameWindowCoords_1Player,
    sNameWindowCoords_2Players,
    sNameWindowCoords_3Players,
    sNameWindowCoords_4Players,
    sNameWindowCoords_5Players,
};

static const u8 *const sRankingTexts[MAX_RFU_PLAYERS] =
{
    gText_1Colon,
    gText_2Colon,
    gText_3Colon,
    gText_4Colon,
    gText_5Colon,
};

static const u16 sResultsXCoords[] = {92, 132, 172, 212};
static const u16 sResultsYCoords[] = {33, 49, 65, 81, 97};
static const u16 sRankingYCoords[] = {17, 33, 49, 65, 81};

struct
{
    u8 id;
    void (*func)(void);
} static const sGfxFuncs[] =
{
    {GFXFUNC_LOAD,               LoadGfx}, // Element not used, LoadGfx is passed directly to SetGfxFunc
    {GFXFUNC_SHOW_NAMES,         ShowNames},
    {GFXFUNC_SHOW_RESULTS,       ShowResults},
    {GFXFUNC_MSG_PLAY_AGAIN,     Msg_WantToPlayAgain},
    {GFXFUNC_MSG_SAVING,         Msg_SavingDontTurnOff},
    {GFXFUNC_MSG_COMM_STANDBY,   Msg_CommunicationStandby},
    {GFXFUNC_ERASE_MSG,          EraseMessage},
    {GFXFUNC_MSG_PLAYER_DROPPED, Msg_SomeoneDroppedOut},
    {GFXFUNC_STOP,               StopGfxFuncs},
    {GFXFUNC_IDLE,               GfxIdle},
};

static void SetGfxFuncById(u8 funcId)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sGfxFuncs); i++)
    {
        if (sGfxFuncs[i].id == funcId)
            SetGfxFunc(sGfxFuncs[i].func);
    }
}

static void Task_TryRunGfxFunc(u8 taskId)
{
    // Continue calling function until it
    // has reached its finished state.
    // Another will not be called until
    // readied by SetGfxFunc
    if (!sGfx->finished)
        GetGfxFunc()();
}

static void LoadGfx(void)
{
    switch (sGfx->state)
    {
    case 0:
        InitBgs();
        sGfx->state++;
        break;
    case 1:
        if (LoadBgGfx() == TRUE)
            sGfx->state++;
        break;
    case 2:
        CopyToBgTilemapBuffer(BG_SCENERY, sBg_Tilemap, 0, 0);
        CopyToBgTilemapBuffer(BG_TREE_LEFT, sTreeBorderLeft_Tilemap, 0, 0);
        CopyToBgTilemapBuffer(BG_TREE_RIGHT, sTreeBorderRight_Tilemap, 0, 0);
        CopyBgTilemapBufferToVram(BG_SCENERY);
        CopyBgTilemapBufferToVram(BG_TREE_LEFT);
        CopyBgTilemapBufferToVram(BG_TREE_RIGHT);
        sGfx->state++;
        break;
    case 3:
        ShowBg(BG_INTERFACE);
        ShowBg(BG_SCENERY);
        ShowBg(BG_TREE_LEFT);
        ShowBg(BG_TREE_RIGHT);
        sGfx->state++;
        break;
    case 4:
        LoadWindowFrameGfx(gSaveBlock2Ptr->optionsWindowFrameType);
        LoadUserWindowFrameGfx();
        sGfx->state++;
        break;
    default:
        sGfx->finished = TRUE;
        break;
    }
}

static void ShowNames(void)
{
    u8 i, numPlayers, playerId, colorsId, *name;
    u32 left;
    struct WindowTemplate window;
    const struct WinCoords *coords;

    switch (sGfx->state)
    {
    case 0:
        numPlayers = GetNumPlayers();
        coords = sNameWindowCoords[numPlayers - 1];
        window.bg = BG_INTERFACE;
        window.width = 7;
        window.height = 2;
        window.paletteNum = 13;
        window.baseBlock = 0x13;
        for (i = 0; i < numPlayers; coords++, i++)
        {
            colorsId = COLORID_GRAY;
            playerId = GetPlayerIdByPos(i);
            left = (56 - GetStringWidth(FONT_NORMAL, GetPlayerName(playerId), -1)) / 2u;
            window.tilemapLeft = coords->left;
            window.tilemapTop = coords->top;
            sGfx->windowIds[i] = AddWindow(&window);
            ClearWindowTilemap(sGfx->windowIds[i]);
            FillWindowPixelBuffer(sGfx->windowIds[i], PIXEL_FILL(1));
            if (playerId == GetMultiplayerId())
                colorsId = COLORID_BLUE;
            name = GetPlayerName(playerId);
            AddTextPrinterParameterized3(sGfx->windowIds[i], FONT_NORMAL, left, 1, sTextColorTable[colorsId], TEXT_SKIP_DRAW, name);
            CopyWindowToVram(sGfx->windowIds[i], COPYWIN_GFX);
            window.baseBlock += 0xE;
            DrawMessageWindow(&window);
        }
        sGfx->state++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            numPlayers = GetNumPlayers();
            for (i = 0; i < numPlayers; i++)
                PutWindowTilemap(sGfx->windowIds[i]);
            CopyBgTilemapBufferToVram(BG_INTERFACE);
            sGfx->state++;
        }
        break;
    default:
        if (++sGfx->state > 180)
        {
            numPlayers = GetNumPlayers();
            for (i = 0; i < numPlayers; i++)
            {
                ClearWindowTilemap(sGfx->windowIds[i]);
                RemoveWindow(sGfx->windowIds[i]);
            }
            FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
            CopyBgTilemapBufferToVram(BG_INTERFACE);
            sGfx->finished = TRUE;
        }
        break;
    }
}

static void PrintRankedScores(u8 numPlayers_)
{
    u8 i, ranking = 0, rankedPlayers = 0;
    u8 numPlayers = numPlayers_; // Needed to match
    u8 *name;
    u32 x, numWidth;
    u8 numString[32];
    u8 playersByRanking[MAX_RFU_PLAYERS] = {0, 1, 2, 3, 4};
    struct DodrioGame_ScoreResults temp, scoreResults[MAX_RFU_PLAYERS];

    // Get all players scores and rankings
    for (i = 0; i < numPlayers; i++)
    {
        playersByRanking[i] = i;
        GetScoreResults(&temp, i);
        scoreResults[i] = temp;
    }

    // Sort player ids by ranking
    if (GetHighestScore() != 0)
    {
        do
        {
            for (i = 0; i < numPlayers; i++)
            {
                if (scoreResults[i].ranking == ranking)
                {
                    playersByRanking[rankedPlayers] = i;
                    rankedPlayers++;
                }
            }
            ranking = rankedPlayers;
        } while (rankedPlayers < numPlayers);
    }

    // Put any player with a score of 0 at lowest ranking
    for (i = 0; i < numPlayers; i++)
    {
        if (scoreResults[i].score == 0)
            scoreResults[i].ranking = numPlayers - 1;
    }

    // Print text
    x = 216 - GetStringWidth(FONT_NORMAL, gText_SpacePoints, 0);
    for (i = 0; i < numPlayers; i++)
    {
        u8 colorsId = COLORID_GRAY;
        u8 playerId = playersByRanking[i];
        u32 points = scoreResults[playerId].score;

        AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, sRankingTexts[scoreResults[playerId].ranking], 8, sRankingYCoords[i], TEXT_SKIP_DRAW, NULL);
        if (playerId == GetMultiplayerId())
            colorsId = COLORID_BLUE;
        name = GetPlayerName(playerId);
        AddTextPrinterParameterized3(sGfx->windowIds[1], FONT_NORMAL, 28, sRankingYCoords[i], sTextColorTable[colorsId], TEXT_SKIP_DRAW, name);
        ConvertIntToDecimalStringN(numString, points, STR_CONV_MODE_LEFT_ALIGN, 7);
        numWidth = GetStringWidth(FONT_NORMAL, numString, -1);
        AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, numString, x - numWidth, sRankingYCoords[i], TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, gText_SpacePoints, x, sRankingYCoords[i], TEXT_SKIP_DRAW, NULL);
    }
}

static void ShowResults(void)
{
    u8 i, j, prizeState, numPlayers = GetNumPlayers();
    u8 *name;
    u32 strWidth, x;

    switch (sGfx->state)
    {
    case 0:
        SetScoreResults();
        sGfx->timer = 0;
        sGfx->state++;
        break;
    case 1:
        sGfx->windowIds[0] = AddWindow(&sWindowTemplates_Results[0]);
        sGfx->windowIds[1] = AddWindow(&sWindowTemplates_Results[1]);
        ClearWindowTilemap(sGfx->windowIds[0]);
        ClearWindowTilemap(sGfx->windowIds[1]);
        DrawMessageWindow(&sWindowTemplates_Results[0]);
        DrawMessageWindow(&sWindowTemplates_Results[1]);
        sGfx->state++;
        break;
    case 2:
        FillWindowPixelBuffer(sGfx->windowIds[0], PIXEL_FILL(1));
        FillWindowPixelBuffer(sGfx->windowIds[1], PIXEL_FILL(1));
        strWidth = GetStringWidth(FONT_NORMAL, gText_BerryPickingResults, -1);
        x = (224 - strWidth) / 2;
        AddTextPrinterParameterized(sGfx->windowIds[0], FONT_NORMAL, gText_BerryPickingResults, x, 1, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, gText_10P30P50P50P, 68, 17, TEXT_SKIP_DRAW, NULL);
        for (i = 0; i < numPlayers; i++)
        {
            u8 colorsId = COLORID_GRAY;
            if (i == GetMultiplayerId())
                colorsId = COLORID_BLUE;

            name = GetPlayerName(i);
            AddTextPrinterParameterized3(sGfx->windowIds[1], FONT_NORMAL, 0, sResultsYCoords[i], sTextColorTable[colorsId], TEXT_SKIP_DRAW, name);
            for (j = 0; j < 4; j++)
            {
                u32 width;
                u16 berriesPicked = Min(GetBerryResult(i, j), MAX_BERRIES);
                u16 maxBerriesPicked = Min(GetHighestBerryResult(j), MAX_BERRIES);

                ConvertIntToDecimalStringN(gStringVarBuffer, berriesPicked, STR_CONV_MODE_LEFT_ALIGN, 4);
                width = GetStringWidth(FONT_NORMAL, gStringVarBuffer, -1);

                // If player got the most of a berry type, highlight their number in red
                if (maxBerriesPicked == berriesPicked && maxBerriesPicked != 0)
                    AddTextPrinterParameterized3(sGfx->windowIds[1], FONT_NORMAL, sResultsXCoords[j] - width, sResultsYCoords[i], sTextColorTable[COLORID_RED], TEXT_SKIP_DRAW, gStringVarBuffer);
                else
                    AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, gStringVarBuffer, sResultsXCoords[j] - width, sResultsYCoords[i], TEXT_SKIP_DRAW, NULL);
            }
        }
        CopyWindowToVram(sGfx->windowIds[0], COPYWIN_GFX);
        CopyWindowToVram(sGfx->windowIds[1], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            PutWindowTilemap(sGfx->windowIds[0]);
            PutWindowTilemap(sGfx->windowIds[1]);
        }
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        SetBerryIconsInvisibility(FALSE);
        sGfx->state++;
        break;
    case 4:
        if (++sGfx->timer >= 30 && JOY_NEW(A_BUTTON))
        {
            sGfx->timer = 0;
            PlaySE(SE_SELECT);
            SetBerryIconsInvisibility(TRUE);
            sGfx->state++;
        }
        break;
    case 5:
        FillWindowPixelBuffer(sGfx->windowIds[0], PIXEL_FILL(1));
        FillWindowPixelBuffer(sGfx->windowIds[1], PIXEL_FILL(1));
        strWidth = GetStringWidth(FONT_NORMAL, gText_AnnouncingRankings, -1);
        x = (224 - strWidth) / 2;
        AddTextPrinterParameterized(sGfx->windowIds[0], FONT_NORMAL, gText_AnnouncingRankings, x, 1, TEXT_SKIP_DRAW, NULL);
        sGfx->state++;
        break;
    case 6:
        PrintRankedScores(numPlayers);
        CopyWindowToVram(sGfx->windowIds[0], COPYWIN_GFX);
        CopyWindowToVram(sGfx->windowIds[1], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 7:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            PutWindowTilemap(sGfx->windowIds[0]);
            PutWindowTilemap(sGfx->windowIds[1]);
        }
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->state++;
        break;
    case 8:
        if (++sGfx->timer >= 30 && JOY_NEW(A_BUTTON))
        {
            sGfx->timer = 0;
            PlaySE(SE_SELECT);
            if (GetHighestScore() < PRIZE_SCORE)
            {
                sGfx->state = 127; // Skip to end, past giving prize
            }
            else
            {
                StopMapMusic();
                sGfx->state++;
            }

            FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 5, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT - 5);
            RemoveWindow(sGfx->windowIds[1]);
            sGfx->windowIds[1] = AddWindow(&sWindowTemplate_Prize);
            ClearWindowTilemap(sGfx->windowIds[1]);
            DrawMessageWindow(&sWindowTemplate_Prize);
        }
        break;
    case 9:
        PlayNewMapMusic(MUS_LEVEL_UP);
        FillWindowPixelBuffer(sGfx->windowIds[0], PIXEL_FILL(1));
        FillWindowPixelBuffer(sGfx->windowIds[1], PIXEL_FILL(1));
        strWidth = GetStringWidth(FONT_NORMAL, gText_AnnouncingPrizes, -1);
        x = (224 - strWidth) / 2;
        AddTextPrinterParameterized(sGfx->windowIds[0], FONT_NORMAL, gText_AnnouncingPrizes, x, 1, TEXT_SKIP_DRAW, NULL);
        DynamicPlaceholderTextUtil_Reset();
        CopyItemName(GetPrizeItemId(), gStringVar1);
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, gText_FirstPlacePrize);
        AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
        prizeState = TryGivePrize();
        if (prizeState != PRIZE_RECEIVED && prizeState != NO_PRIZE)
        {
            DynamicPlaceholderTextUtil_Reset();
            CopyItemName(GetPrizeItemId(), gStringVar1);
            DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
            if (prizeState == PRIZE_NO_ROOM)
                DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, gText_CantHoldAnyMore);
            else if (prizeState == PRIZE_FILLED_BAG)
                DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, gText_FilledStorageSpace);
            AddTextPrinterParameterized(sGfx->windowIds[1], FONT_NORMAL, gStringVarBuffer, 0, 41, TEXT_SKIP_DRAW, NULL);
        }
        CopyWindowToVram(sGfx->windowIds[0], COPYWIN_GFX);
        CopyWindowToVram(sGfx->windowIds[1], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 10:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            PutWindowTilemap(sGfx->windowIds[0]);
            PutWindowTilemap(sGfx->windowIds[1]);
        }
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        FadeOutAndFadeInNewMapMusic(MUS_RG_VICTORY_WILD, 20, 10);
        sGfx->state++;
        break;
    case 11:
        if (++sGfx->timer >= 30 && JOY_NEW(A_BUTTON))
        {
            sGfx->timer = 0;
            PlaySE(SE_SELECT);
            sGfx->state++;
        }
        break;
    default:
        ClearWindowTilemap(sGfx->windowIds[0]);
        ClearWindowTilemap(sGfx->windowIds[1]);
        RemoveWindow(sGfx->windowIds[0]);
        RemoveWindow(sGfx->windowIds[1]);
        FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->finished = TRUE;
        break;
    }
}

static void Msg_WantToPlayAgain(void)
{
    u8 y;

    switch (sGfx->state)
    {
    case 0:
        // Create windows
        sGfx->windowIds[WIN_PLAY_AGAIN] = AddWindow(&sWindowTemplates_PlayAgain[WIN_PLAY_AGAIN]);
        sGfx->windowIds[WIN_YES_NO] = AddWindow(&sWindowTemplates_PlayAgain[WIN_YES_NO]);
        ClearWindowTilemap(sGfx->windowIds[WIN_PLAY_AGAIN]);
        ClearWindowTilemap(sGfx->windowIds[WIN_YES_NO]);
        DrawMessageWindow(&sWindowTemplates_PlayAgain[WIN_PLAY_AGAIN]);
        DrawYesNoMessageWindow(&sWindowTemplates_PlayAgain[WIN_YES_NO]);
        sGfx->state++;
        sGfx->cursorSelection = PLAY_AGAIN_NONE;
        sGfx->playAgainState = PLAY_AGAIN_NONE;
        break;
    case 1:
        // Print text
        FillWindowPixelBuffer(sGfx->windowIds[WIN_PLAY_AGAIN], PIXEL_FILL(1));
        FillWindowPixelBuffer(sGfx->windowIds[WIN_YES_NO], PIXEL_FILL(1));
        AddTextPrinterParameterized(sGfx->windowIds[WIN_PLAY_AGAIN], FONT_NORMAL, gText_WantToPlayAgain, 0, 5, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_Yes, 8, 1, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_No, 8, 17, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_SelectorArrow2, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sGfx->windowIds[WIN_PLAY_AGAIN], COPYWIN_GFX);
        CopyWindowToVram(sGfx->windowIds[WIN_YES_NO], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 2:
        // Draw windows
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            PutWindowTilemap(sGfx->windowIds[WIN_PLAY_AGAIN]);
            PutWindowTilemap(sGfx->windowIds[WIN_YES_NO]);
        }
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->state++;
        break;
    case 3:
        // Handle input
        y = sGfx->cursorSelection;
        if (y == PLAY_AGAIN_NONE)
            y = PLAY_AGAIN_YES;
        FillWindowPixelBuffer(sGfx->windowIds[WIN_YES_NO], PIXEL_FILL(1));
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_Yes, 8, 1, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_No, 8, 17, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGfx->windowIds[WIN_YES_NO], FONT_NORMAL, gText_SelectorArrow2, 0, ((y - 1) * 16) + 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sGfx->windowIds[WIN_YES_NO], COPYWIN_FULL);

        // Increment state only if A or B button have been pressed.
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (sGfx->cursorSelection == PLAY_AGAIN_NONE)
                sGfx->cursorSelection = PLAY_AGAIN_YES;
            sGfx->state++;
        }
        else if (JOY_NEW(DPAD_UP | DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            switch (sGfx->cursorSelection)
            {
            case PLAY_AGAIN_NONE:
                sGfx->cursorSelection = PLAY_AGAIN_NO;
                break;
            case PLAY_AGAIN_YES:
                sGfx->cursorSelection = PLAY_AGAIN_NO;
                break;
            case PLAY_AGAIN_NO:
                sGfx->cursorSelection = PLAY_AGAIN_YES;
                break;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sGfx->cursorSelection = PLAY_AGAIN_NO;
            sGfx->state++;
        }
        break;
    default:
        sGfx->playAgainState = sGfx->cursorSelection;
        ClearWindowTilemap(sGfx->windowIds[WIN_PLAY_AGAIN]);
        ClearWindowTilemap(sGfx->windowIds[WIN_YES_NO]);
        RemoveWindow(sGfx->windowIds[WIN_PLAY_AGAIN]);
        RemoveWindow(sGfx->windowIds[WIN_YES_NO]);
        FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->finished = TRUE;
        break;
    }
}

static void Msg_SavingDontTurnOff(void)
{
    switch (sGfx->state)
    {
    case 0:
        DrawDialogueFrame(0, FALSE);
        AddTextPrinterParameterized2(0, FONT_NORMAL, gText_SavingDontTurnOffPower, 0, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        sGfx->state++;
        break;
    case 1:
        CopyWindowToVram(0, COPYWIN_FULL);
        sGfx->state++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CreateTask(Task_LinkFullSave, 0);
            sGfx->state++;
        }
        break;
    case 3:
        if (!FuncIsActiveTask(Task_LinkFullSave))
            sGfx->state++;
        break;
    default:
        FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->finished = TRUE;
        break;
    }
}

static void Msg_CommunicationStandby(void)
{
    switch (sGfx->state)
    {
    case 0:
        sGfx->windowIds[0] = AddWindow(&sWindowTemplate_CommStandby);
        ClearWindowTilemap(sGfx->windowIds[0]);
        DrawMessageWindow(&sWindowTemplate_CommStandby);
        sGfx->state++;
        break;
    case 1:
        FillWindowPixelBuffer(sGfx->windowIds[0], PIXEL_FILL(1));
        AddTextPrinterParameterized(sGfx->windowIds[0], FONT_NORMAL, gText_CommunicationStandby3, 0, 5, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sGfx->windowIds[0], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
            PutWindowTilemap(sGfx->windowIds[0]);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->state++;
        break;
    default:
        sGfx->finished = TRUE;
        break;
    }
}

static void EraseMessage(void)
{
    ClearWindowTilemap(sGfx->windowIds[0]);
    RemoveWindow(sGfx->windowIds[0]);
    FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
    CopyBgTilemapBufferToVram(BG_INTERFACE);
    sGfx->finished = TRUE;
}

static void Msg_SomeoneDroppedOut(void)
{
    switch (sGfx->state)
    {
    case 0:
        sGfx->windowIds[0] = AddWindow(&sWindowTemplate_DroppedOut);
        ClearWindowTilemap(sGfx->windowIds[0]);
        DrawMessageWindow(&sWindowTemplate_DroppedOut);
        sGfx->state++;
        sGfx->timer = 0;
        sGfx->cursorSelection = 0;
        sGfx->playAgainState = PLAY_AGAIN_NONE;
        break;
    case 1:
        FillWindowPixelBuffer(sGfx->windowIds[0], PIXEL_FILL(1));
        AddTextPrinterParameterized(sGfx->windowIds[0], FONT_NORMAL, gText_SomeoneDroppedOut, 0, 5, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sGfx->windowIds[0], COPYWIN_GFX);
        sGfx->state++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
            PutWindowTilemap(sGfx->windowIds[0]);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->state++;
        break;
    case 3:
        if (++sGfx->timer >= 120)
            sGfx->state++;
        break;
    default:
        sGfx->playAgainState = PLAY_AGAIN_DROPPED;
        ClearWindowTilemap(sGfx->windowIds[0]);
        RemoveWindow(sGfx->windowIds[0]);
        FillBgTilemapBufferRect_Palette0(BG_INTERFACE, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(BG_INTERFACE);
        sGfx->finished = TRUE;
        break;
    }
}

static void StopGfxFuncs(void)
{
    DestroyTask(sGfx->taskId);
    sGfx->finished = TRUE;
}

static void GfxIdle(void)
{

}

static void SetGfxFunc(void (*func)(void))
{
    sGfx->state = 0;
    sGfx->finished = FALSE;
    sGfx->func = func;
}

static void (*GetGfxFunc(void))(void)
{
    return sGfx->func;
}

static bool32 IsGfxFuncActive(void)
{
    if (sGfx->finished == TRUE)
        return FALSE;
    else
        return TRUE;
}

static u8 GetPlayAgainState(void)
{
    return sGfx->playAgainState;
}

static void InitBgs(void)
{
    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3,(void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
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
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetBgTilemapBuffer(BG_SCENERY, sGfx->tilemapBuffers[0]);
    SetBgTilemapBuffer(BG_TREE_LEFT, sGfx->tilemapBuffers[1]);
    SetBgTilemapBuffer(BG_TREE_RIGHT, sGfx->tilemapBuffers[2]);
}

static bool32 LoadBgGfx(void)
{
    switch (sGfx->loadState)
    {
    case 0:
        LoadPalette(sBg_Pal, BG_PLTT_ID(0), sizeof(sBg_Pal));
        break;
    case 1:
        ResetTempTileDataBuffers();
        break;
    case 2:
        DecompressAndCopyTileDataToVram(BG_SCENERY, sBg_Gfx, 0, 0, 0);
        break;
    case 3:
        DecompressAndCopyTileDataToVram(BG_TREE_LEFT, sTreeBorder_Gfx, 0, 0, 0);
        break;
    case 4:
        if (FreeTempTileDataBuffersIfPossible() == TRUE)
            return FALSE;
        break;
    case 5:
        LoadPalette(GetTextWindowPalette(3), BG_PLTT_ID(13), PLTT_SIZE_4BPP);
        break;
    default:
        sGfx->loadState = 0;
        return TRUE;
    }

    sGfx->loadState++;
    return FALSE;
}
