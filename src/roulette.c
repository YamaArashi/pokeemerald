#include "global.h"
#include "malloc.h"
#include "bg.h"
#include "coins.h"
#include "decompress.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "m4a.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "palette_util.h"
#include "random.h"
#include "roulette.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "trig.h"
#include "tv.h"
#include "window.h"
#include "constants/coins.h"
#include "constants/rgb.h"
#include "constants/roulette.h"
#include "constants/songs.h"

#define BALLS_PER_ROUND 6

// "Board" is used in this file to refer to both the wheel and the bet selection grid
#define NUM_BOARD_COLORS 3 // Rows on grid
#define NUM_BOARD_POKES  4 // Columns on grid
#define NUM_ROULETTE_SLOTS (NUM_BOARD_COLORS * NUM_BOARD_POKES)

// The degree change between each slot on the roulette wheel
#define DEGREES_PER_SLOT (360 / NUM_ROULETTE_SLOTS)

// Where in the slot the ball will drop when landing
#define SLOT_MIDPOINT (DEGREES_PER_SLOT / 2 - 1)

// IDs for grid selections when betting
#define SELECTION_NONE      0
#define COL_WYNAUT          1
#define COL_AZURILL         2
#define COL_SKITTY          3
#define COL_MAKUHITA        4
#define ROW_ORANGE          (COL_MAKUHITA + 1)
#define SQU_ORANGE_WYNAUT   (ROW_ORANGE + COL_WYNAUT)
#define SQU_ORANGE_AZURILL  (ROW_ORANGE + COL_AZURILL)
#define SQU_ORANGE_SKITTY   (ROW_ORANGE + COL_SKITTY)
#define SQU_ORANGE_MAKUHITA (ROW_ORANGE + COL_MAKUHITA)
#define ROW_GREEN           (SQU_ORANGE_MAKUHITA + 1)
#define SQU_GREEN_WYNAUT    (ROW_GREEN + COL_WYNAUT)
#define SQU_GREEN_AZURILL   (ROW_GREEN + COL_AZURILL)
#define SQU_GREEN_SKITTY    (ROW_GREEN + COL_SKITTY)
#define SQU_GREEN_MAKUHITA  (ROW_GREEN + COL_MAKUHITA)
#define ROW_PURPLE          (SQU_GREEN_MAKUHITA + 1)
#define SQU_PURPLE_WYNAUT   (ROW_PURPLE + COL_WYNAUT)
#define SQU_PURPLE_AZURILL  (ROW_PURPLE + COL_AZURILL)
#define SQU_PURPLE_SKITTY   (ROW_PURPLE + COL_SKITTY)
#define SQU_PURPLE_MAKUHITA (ROW_PURPLE + COL_MAKUHITA)
#define NUM_GRID_SELECTIONS SQU_PURPLE_MAKUHITA

// Get the id of the col/row from the selection ID
// e.g. GET_ROW(SQU_PURPLE_SKITTY) is ROW_PURPLE
#define GET_COL(selectionId) ((selectionId) % (NUM_BOARD_POKES + 1))
#define GET_ROW(selectionId) ((selectionId) / (NUM_BOARD_POKES + 1) * (NUM_BOARD_POKES + 1))

// Get the col/row index from the selection ID
// e.g. GET_ROW_IDX(SQU_PURPLE_SKITTY) is 2 (purple being the 3rd row)
#define GET_COL_IDX(selectionId) (selectionId - 1)
#define GET_ROW_IDX(selectionId) (selectionId / 5 - 1)

// Flags for the above selections, used to set which spaces have been hit or bet on
#define F_WYNAUT_COL      (1 << COL_WYNAUT)
#define F_AZURILL_COL     (1 << COL_AZURILL)
#define F_SKITTY_COL      (1 << COL_SKITTY)
#define F_MAKUHITA_COL    (1 << COL_MAKUHITA)
#define F_ORANGE_ROW      (1 << ROW_ORANGE)
#define F_ORANGE_WYNAUT   (1 << SQU_ORANGE_WYNAUT)
#define F_ORANGE_AZURILL  (1 << SQU_ORANGE_AZURILL)
#define F_ORANGE_SKITTY   (1 << SQU_ORANGE_SKITTY)
#define F_ORANGE_MAKUHITA (1 << SQU_ORANGE_MAKUHITA)
#define F_GREEN_ROW       (1 << ROW_GREEN)
#define F_GREEN_WYNAUT    (1 << SQU_GREEN_WYNAUT)
#define F_GREEN_AZURILL   (1 << SQU_GREEN_AZURILL)
#define F_GREEN_SKITTY    (1 << SQU_GREEN_SKITTY)
#define F_GREEN_MAKUHITA  (1 << SQU_GREEN_MAKUHITA)
#define F_PURPLE_ROW      (1 << ROW_PURPLE)
#define F_PURPLE_WYNAUT   (1 << SQU_PURPLE_WYNAUT)
#define F_PURPLE_AZURILL  (1 << SQU_PURPLE_AZURILL)
#define F_PURPLE_SKITTY   (1 << SQU_PURPLE_SKITTY)
#define F_PURPLE_MAKUHITA (1 << SQU_PURPLE_MAKUHITA)

// Flags for flashing selections on the roulette wheel
#define F_FLASH_COLOR_O_WYNAUT   (1 << 0)
#define F_FLASH_COLOR_G_AZURILL  (1 << 1)
#define F_FLASH_COLOR_P_SKITTY   (1 << 2)
#define F_FLASH_COLOR_O_MAKUHITA (1 << 3)
#define F_FLASH_COLOR_G_WYNAUT   (1 << 4)
#define F_FLASH_COLOR_P_AZURILL  (1 << 5)
#define F_FLASH_COLOR_O_SKITTY   (1 << 6)
#define F_FLASH_COLOR_G_MAKUHITA (1 << 7)
#define F_FLASH_COLOR_P_WYNAUT   (1 << 8)
#define F_FLASH_COLOR_O_AZURILL  (1 << 9)
#define F_FLASH_COLOR_G_SKITTY   (1 << 10)
#define F_FLASH_COLOR_P_MAKUHITA (1 << 11)
#define F_FLASH_OUTER_EDGES      (1 << 12) // when the player wins
#define FLASH_ICON               (NUM_ROULETTE_SLOTS + 1)
#define FLASH_ICON_2             (FLASH_ICON + 1)
#define FLASH_ICON_3             (FLASH_ICON + 2)
#define F_FLASH_ICON             (1 << FLASH_ICON)
#define F_FLASH_COLUMN           (1 << FLASH_ICON | 1 << FLASH_ICON_2 | 1 << FLASH_ICON_3)

#define MAX_MULTIPLIER 12

#define PALTAG_SHADOW 1
#define PALTAG_BALL 2
#define PALTAG_BALL_COUNTER 3
#define PALTAG_CURSOR 4
#define PALTAG_INTERFACE 5
#define PALTAG_SHROOMISH 6
#define PALTAG_TAILLOW   7
#define PALTAG_GRID_ICONS 8
#define PALTAG_WYNAUT    9
#define PALTAG_AZURILL   10
#define PALTAG_SKITTY    11
#define PALTAG_MAKUHITA  12

#define GFXTAG_WHEEL_ICONS 0
#define GFXTAG_HEADERS 4
#define GFXTAG_GRID_ICONS 5
#define GFXTAG_WHEEL_CENTER 6
#define GFXTAG_CREDIT 7
#define GFXTAG_CREDIT_DIGIT 8
#define GFXTAG_MULTIPLIER 9
#define GFXTAG_BALL_COUNTER 10
#define GFXTAG_CURSOR 11
#define GFXTAG_BALL 12
#define GFXTAG_SHROOMISH_TAILLOW 13
#define GFXTAG_SHADOW 14

// 2 different Roulette tables with 2 different rates (normal vs service day special)
// & 1 gets which table, >> 7 gets if ROULETTE_SPECIAL_RATE is set
#define GET_MIN_BET_ID(var) (((var) & 1) + (((var) >> 7) * 2))

// Having Shroomish or Taillow in the party can make rolls more consistent in length
// It also increases the likelihood that, if they appear to unstick a ball, they'll move it to a slot the player bet on
#define HAS_SHROOMISH  (1 << 0)
#define HAS_TAILLOW    (1 << 1)

#define NO_DELAY 0xFFFF

enum {
    BALL_STATE_ROLLING,
    BALL_STATE_STUCK,
    BALL_STATE_LANDED = 0xFF,
};

enum {
    SELECT_STATE_WAIT,
    SELECT_STATE_DRAW,
    SELECT_STATE_UPDATE,
    SELECT_STATE_ERASE = 0xFF,
};

// Roulette uses a large amount of sprites, and stores ids for these in a single array
// Many are looped over rather than referenced directly
enum {
    SPR_WHEEL_BALL_1,
    SPR_WHEEL_BALL_2,
    SPR_WHEEL_BALL_3,
    SPR_WHEEL_BALL_4,
    SPR_WHEEL_BALL_5,
    SPR_WHEEL_BALL_6,
    SPR_WHEEL_CENTER,
    SPR_WHEEL_ICON_ORANGE_WYNAUT,
    SPR_WHEEL_ICON_GREEN_AZURILL,
    SPR_WHEEL_ICON_PURPLE_SKITTY,
    SPR_WHEEL_ICON_ORANGE_MAKUHITA,
    SPR_WHEEL_ICON_GREEN_WYNAUT,
    SPR_WHEEL_ICON_PURPLE_AZURILL,
    SPR_WHEEL_ICON_ORANGE_SKITTY,
    SPR_WHEEL_ICON_GREEN_MAKUHITA,
    SPR_WHEEL_ICON_PURPLE_WYNAUT,
    SPR_WHEEL_ICON_ORANGE_AZURILL,
    SPR_WHEEL_ICON_GREEN_SKITTY,
    SPR_WHEEL_ICON_PURPLE_MAKUHITA,
    SPR_19, // Unused
    SPR_CREDIT,
    SPR_CREDIT_DIG_1,
    SPR_CREDIT_DIG_10,
    SPR_CREDIT_DIG_100,
    SPR_CREDIT_DIG_1000,
    SPR_MULTIPLIER,
    SPR_BALL_COUNTER_1,
    SPR_BALL_COUNTER_2,
    SPR_BALL_COUNTER_3,
    SPR_GRID_ICON_ORANGE_WYNAUT,
    SPR_GRID_ICON_GREEN_AZURILL,
    SPR_GRID_ICON_PURPLE_SKITTY,
    SPR_GRID_ICON_ORANGE_MAKUHITA,
    SPR_GRID_ICON_GREEN_WYNAUT,
    SPR_GRID_ICON_PURPLE_AZURILL,
    SPR_GRID_ICON_ORANGE_SKITTY,
    SPR_GRID_ICON_GREEN_MAKUHITA,
    SPR_GRID_ICON_PURPLE_WYNAUT,
    SPR_GRID_ICON_ORANGE_AZURILL,
    SPR_GRID_ICON_GREEN_SKITTY,
    SPR_GRID_ICON_PURPLE_MAKUHITA,
    SPR_POKE_HEADER_1,
    SPR_POKE_HEADER_2,
    SPR_POKE_HEADER_3,
    SPR_POKE_HEADER_4,
    SPR_COLOR_HEADER_1,
    SPR_COLOR_HEADER_2,
    SPR_COLOR_HEADER_3,
    SPR_WIN_SLOT_CURSOR,
    SPR_GRID_BALL_1,
    SPR_GRID_BALL_2,
    SPR_GRID_BALL_3,
    SPR_GRID_BALL_4,
    SPR_GRID_BALL_5,
    SPR_GRID_BALL_6,
    SPR_CLEAR_MON, // Shroomish/Taillow
    SPR_CLEAR_MON_SHADOW_1,
    SPR_CLEAR_MON_SHADOW_2,
    SPR_58, // Here below unused
    SPR_59,
    SPR_60,
    SPR_61,
    SPR_62,
    SPR_63,
};

// Start points for sprite IDs that are looped over
#define SPR_WHEEL_BALLS SPR_WHEEL_BALL_1
#define SPR_WHEEL_ICONS SPR_WHEEL_ICON_ORANGE_WYNAUT
#define SPR_BALL_COUNTER SPR_BALL_COUNTER_1
#define SPR_CREDIT_DIGITS SPR_CREDIT_DIG_1
#define SPR_GRID_ICONS SPR_GRID_ICON_ORANGE_WYNAUT
#define SPR_POKE_HEADERS SPR_POKE_HEADER_1
#define SPR_COLOR_HEADERS SPR_COLOR_HEADER_1
#define SPR_GRID_BALLS SPR_GRID_BALL_1

struct Shroomish
{
    u16 startAngle;
    u16 dropAngle;
    u16 fallSlowdown;
};

struct Taillow
{
    u16 baseDropDelay;
    u16 rightStartAngle;
    u16 leftStartAngle;
};

struct RouletteTable
{
    u8 minBet; // Never read
    u8 randDistanceHigh;
    u8 randDistanceLow;
    u8 wheelSpeed;
    u8 wheelDelay;
    struct Shroomish shroomish;
    struct Taillow taillow;
    u16 ballSpeed;
    u16 baseTravelDist;
    f32 var1C;
};

struct GridSelection
{
    u8 spriteIdOffset;
    u8 baseMultiplier:4;
    u8 column:4; // Never read
    u8 row;      // Never read
    u8 x;
    u8 y;
    u8 var05;    // Never read
    u8 tilemapOffset;
    u32 flag;
    u32 inSelectionFlags;
    u16 flashFlags;
};

struct RouletteSlot
{
    u8 id1; // Never read
    u8 id2; // Never read
    u8 gridSquare;
    u32 flag;
};

static EWRAM_DATA struct Roulette
{
    u8 unk0; // Never read
    u8 shroomishShadowTimer;
    u8 partySpeciesFlags;
    bool8 useTaillow:5;
    bool8 ballStuck:1;
    bool8 ballUnstuck:1;
    bool8 ballRolling:1; // Never read
    u8 tableId:2;
    u8 unused:5;
    bool8 isSpecialRate:1;
    u32 hitFlags;
    u8 hitSquares[BALLS_PER_ROUND];
    u8 pokeHits[NUM_BOARD_POKES];
    u8 colorHits[NUM_BOARD_COLORS];
    u8 minBet;
    u8 curBallNum:4; // Never actually gets incremented, tracked with tBallNum instead
    u8 unk1:4; // Never read
    u8 betSelection[BALLS_PER_ROUND]; // Because curBallNum is used as the only index, only the first element is ever used (prev bet selections are never needed)
    u8 wheelDelayTimer;
    u8 wheelSpeed;
    u8 wheelDelay;
    s16 wheelAngle;
    s16 gridX;
    s16 selectionRectDrawState;
    s16 updateGridHighlight;
    struct OamMatrix wheelRotation;
    u16 shroomishShadowAlpha;
    struct Sprite *ball;
    u8 spriteIds[MAX_SPRITES];
    u8 curBallSpriteId;
    u8 ballState;
    u8 hitSlot;
    u8 stuckHitSlot;
    s16 ballTravelDist; // Never read
    s16 ballTravelDistFast;
    u16 ballTravelDistMed;
    u16 ballTravelDistSlow;
    f32 ballAngle;
    f32 ballAngleSpeed;
    f32 ballAngleAccel;
    f32 ballDistToCenter;
    f32 ballFallSpeed;
    f32 ballFallAccel;
    f32 varA0;
    u8 playTaskId;
    u8 spinTaskId;
    u8 filler_1[2];
    u16 taskWaitDelay;
    u16 taskWaitKey;
    TaskFunc nextTask;
    u8 filler_2[4];
    TaskFunc prevTask;
    struct RouletteFlashUtil flashUtil;
    u16 tilemapBuffers[7][0x400];
    u16 *gridTilemap;
} *sRoulette = NULL;

static EWRAM_DATA u8 sTextWindowId = 0;

static void Task_SpinWheel(u8);
static void Task_StartPlaying(u8);
static void Task_ContinuePlaying(u8);
static void Task_StopPlaying(u8);
static void Task_SelectFirstEmptySquare(u8);
static void Task_HandleBetGridInput(u8);
static void Task_SlideGridOffscreen(u8);
static void Task_InitBallRoll(u8);
static void Task_RollBall(u8);
static void Task_RecordBallHit(u8);
static void Task_SlideGridOnscreen(u8);
static void Task_FlashBallOnWinningSquare(u8);
static void Task_PrintSpinResult(u8);
static void Task_PrintPayout(u8);
static void Task_EndTurn(u8);
static void Task_TryPrintEndTurnMsg(u8);
static void Task_ClearBoard(u8);
static void ExitRoulette(u8);
static void Task_ExitRoulette(u8);
static void StartTaskAfterDelayOrInput(u8, TaskFunc, u16, u16);
static void ResetBallDataForNewSpin(u8);
static void ResetHits(void);
static void Task_AcceptMinBet(u8);
static void Task_DeclineMinBet(u8);
static u8 RecordHit(u8, u8);
static bool8 IsHitInBetSelection(u8, u8);
static void FlashSelectionOnWheel(u8);
static void DrawGridBackground(u8);
static u8 GetMultiplier(u8);
static void UpdateWheelPosition(void);
static void LoadOrFreeMiscSpritePalettesAndSheets(u8);
static void CreateGridSprites(void);
static void ShowHideGridIcons(bool8, u8);
static void CreateGridBallSprites(void);
static void ShowHideGridBalls(bool8, u8);
static void ShowHideWinSlotCursor(u8);
static void CreateWheelIconSprites(void);
static void SpriteCB_WheelIcon(struct Sprite *);
static void CreateInterfaceSprites(void);
static void SetCreditDigits(u16);
static void SetMultiplierSprite(u8);
static void SetBallCounterNumLeft(u8);
static void SpriteCB_GridSquare(struct Sprite *);
static void CreateWheelCenterSprite(void);
static void SpriteCB_WheelCenter(struct Sprite *);
static void CreateWheelBallSprites(void);
static void HideWheelBalls(void);
static void SpriteCB_RollBall_Start(struct Sprite *);
static void CreateShroomishSprite(struct Sprite *);
static void CreateTaillowSprite(struct Sprite *);
static void SetBallStuck(struct Sprite *);
static void SpriteCB_Shroomish(struct Sprite *);
static void SpriteCB_Taillow(struct Sprite *);

static const u16 sWheel_Pal[] = INCBIN_U16("graphics/roulette/wheel.gbapal"); // also palette for grid
static const u32 sGrid_Tilemap[] = INCBIN_U32("graphics/roulette/grid.bin.lz");
static const u32 sWheel_Tilemap[] = INCBIN_U32("graphics/roulette/wheel.bin.lz");
static const struct BgTemplate sBgTemplates[] =
{
    // Text box
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    // Selection grid
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 4,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    // Wheel
    {
        .bg = 2,
        .charBaseIndex = 1,
        .mapBaseIndex = 6,
        .screenSize = 1,
        .paletteMode = 1,
        .priority = 2,
        .baseTile = 0
    }
};
static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 24,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0xC5
    },
    #ifdef UBFIX
    DUMMY_WIN_TEMPLATE,
    #endif
};

static const struct GridSelection sGridSelections[NUM_GRID_SELECTIONS + 1] =
{
    [SELECTION_NONE] = {
        .spriteIdOffset = 0xFF,
        .baseMultiplier = 0,
        .column = 0,
        .row = 0,
        .x = 7,
        .y = 7,
        .var05 = 0,
        .tilemapOffset = 0,
        .flag = 0,
        .inSelectionFlags = 0,
        .flashFlags = 0,
    },
    [COL_WYNAUT] = {
        .spriteIdOffset = 12,
        .baseMultiplier = NUM_BOARD_POKES,
        .column = 1,
        .row = 0,
        .x = 17,
        .y = 7,
        .var05 = 0,
        .tilemapOffset = 0,
        .flag = F_WYNAUT_COL,
        .inSelectionFlags = F_WYNAUT_COL | F_ORANGE_WYNAUT | F_GREEN_WYNAUT | F_PURPLE_WYNAUT,
        .flashFlags = F_FLASH_COLUMN,
    },
    [COL_AZURILL] = {
        .spriteIdOffset = 13,
        .baseMultiplier = NUM_BOARD_POKES,
        .column = 2,
        .row = 0,
        .x = 20,
        .y = 7,
        .var05 = 0,
        .tilemapOffset = 0,
        .flag = F_AZURILL_COL,
        .inSelectionFlags = F_AZURILL_COL | F_ORANGE_AZURILL | F_GREEN_AZURILL | F_PURPLE_AZURILL,
        .flashFlags = F_FLASH_COLUMN,
    },
    [COL_SKITTY] = {
        .spriteIdOffset = 14,
        .baseMultiplier = NUM_BOARD_POKES,
        .column = 3,
        .row = 0,
        .x = 23,
        .y = 7,
        .var05 = 0,
        .tilemapOffset = 0,
        .flag = F_SKITTY_COL,
        .inSelectionFlags = F_SKITTY_COL | F_ORANGE_SKITTY | F_GREEN_SKITTY | F_PURPLE_SKITTY,
        .flashFlags = F_FLASH_COLUMN,
    },
    [COL_MAKUHITA] = {
        .spriteIdOffset = 15,
        .baseMultiplier = NUM_BOARD_POKES,
        .column = 4,
        .row = 0,
        .x = 26,
        .y = 7,
        .var05 = 0,
        .tilemapOffset = 0,
        .flag = F_MAKUHITA_COL,
        .inSelectionFlags = F_MAKUHITA_COL | F_ORANGE_MAKUHITA | F_GREEN_MAKUHITA | F_PURPLE_MAKUHITA,
        .flashFlags = F_FLASH_COLUMN,
    },
    [ROW_ORANGE] = {
        .spriteIdOffset = 16,
        .baseMultiplier = NUM_BOARD_COLORS,
        .column = 0,
        .row = 1,
        .x = 14,
        .y = 10,
        .var05 = 0,
        .tilemapOffset = 12,
        .flag = F_ORANGE_ROW,
        .inSelectionFlags = F_ORANGE_ROW | F_ORANGE_WYNAUT | F_ORANGE_AZURILL | F_ORANGE_SKITTY | F_ORANGE_MAKUHITA,
        .flashFlags = F_FLASH_COLOR_O_WYNAUT | F_FLASH_COLOR_O_AZURILL | F_FLASH_COLOR_O_SKITTY | F_FLASH_COLOR_O_MAKUHITA,
    },
    [SQU_ORANGE_WYNAUT] = {
        .spriteIdOffset = 0,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 1,
        .row = 1,
        .x = 17,
        .y = 10,
        .var05 = 3,
        .tilemapOffset = 3,
        .flag = F_ORANGE_WYNAUT,
        .inSelectionFlags = F_ORANGE_WYNAUT,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_O_WYNAUT,
    },
    [SQU_ORANGE_AZURILL] = {
        .spriteIdOffset = 9,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 2,
        .row = 1,
        .x = 20,
        .y = 10,
        .var05 = 3,
        .tilemapOffset = 3,
        .flag = F_ORANGE_AZURILL,
        .inSelectionFlags = F_ORANGE_AZURILL,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_O_AZURILL,
    },
    [SQU_ORANGE_SKITTY] = {
        .spriteIdOffset = 6,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 3,
        .row = 1,
        .x = 23,
        .y = 10,
        .var05 = 3,
        .tilemapOffset = 3,
        .flag = F_ORANGE_SKITTY,
        .inSelectionFlags = F_ORANGE_SKITTY,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_O_SKITTY,
    },
    [SQU_ORANGE_MAKUHITA] = {
        .spriteIdOffset = 3,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 4,
        .row = 1,
        .x = 26,
        .y = 10,
        .var05 = 3,
        .tilemapOffset = 3,
        .flag = F_ORANGE_MAKUHITA,
        .inSelectionFlags = F_ORANGE_MAKUHITA,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_O_MAKUHITA,
    },
    [ROW_GREEN] = {
        .spriteIdOffset = 17,
        .baseMultiplier = NUM_BOARD_COLORS,
        .column = 0,
        .row = 2,
        .x = 14,
        .y = 13,
        .var05 = 3,
        .tilemapOffset = 15,
        .flag = F_GREEN_ROW,
        .inSelectionFlags = F_GREEN_ROW | F_GREEN_WYNAUT | F_GREEN_AZURILL | F_GREEN_SKITTY | F_GREEN_MAKUHITA,
        .flashFlags = F_FLASH_COLOR_G_WYNAUT | F_FLASH_COLOR_G_AZURILL | F_FLASH_COLOR_G_SKITTY | F_FLASH_COLOR_G_MAKUHITA,
    },
    [SQU_GREEN_WYNAUT] = {
        .spriteIdOffset = 4,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 1,
        .row = 2,
        .x = 17,
        .y = 13,
        .var05 = 6,
        .tilemapOffset = 6,
        .flag = F_GREEN_WYNAUT,
        .inSelectionFlags = F_GREEN_WYNAUT,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_G_WYNAUT,
    },
    [SQU_GREEN_AZURILL] = {
        .spriteIdOffset = 1,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 2,
        .row = 2,
        .x = 20,
        .y = 13,
        .var05 = 6,
        .tilemapOffset = 6,
        .flag = F_GREEN_AZURILL,
        .inSelectionFlags = F_GREEN_AZURILL,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_G_AZURILL,
    },
    [SQU_GREEN_SKITTY] = {
        .spriteIdOffset = 10,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 3,
        .row = 2,
        .x = 23,
        .y = 13,
        .var05 = 6,
        .tilemapOffset = 6,
        .flag = F_GREEN_SKITTY,
        .inSelectionFlags = F_GREEN_SKITTY,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_G_SKITTY,
    },
    [SQU_GREEN_MAKUHITA] = {
        .spriteIdOffset = 7,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 4,
        .row = 2,
        .x = 26,
        .y = 13,
        .var05 = 6,
        .tilemapOffset = 6,
        .flag = F_GREEN_MAKUHITA,
        .inSelectionFlags = F_GREEN_MAKUHITA,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_G_MAKUHITA,
    },
    [ROW_PURPLE] = {
        .spriteIdOffset = 18,
        .baseMultiplier = NUM_BOARD_COLORS,
        .column = 0,
        .row = 3,
        .x = 14,
        .y = 16,
        .var05 = 6,
        .tilemapOffset = 18,
        .flag = F_PURPLE_ROW,
        .inSelectionFlags = F_PURPLE_ROW | F_PURPLE_WYNAUT | F_PURPLE_AZURILL | F_PURPLE_SKITTY | F_PURPLE_MAKUHITA,
        .flashFlags = F_FLASH_COLOR_P_WYNAUT | F_FLASH_COLOR_P_AZURILL | F_FLASH_COLOR_P_SKITTY | F_FLASH_COLOR_P_MAKUHITA,
    },
    [SQU_PURPLE_WYNAUT] = {
        .spriteIdOffset = 8,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 1,
        .row = 3,
        .x = 17,
        .y = 16,
        .var05 = 9,
        .tilemapOffset = 9,
        .flag = F_PURPLE_WYNAUT,
        .inSelectionFlags = F_PURPLE_WYNAUT,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_P_WYNAUT,
    },
    [SQU_PURPLE_AZURILL] = {
        .spriteIdOffset = 5,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 2,
        .row = 3,
        .x = 20,
        .y = 16,
        .var05 = 9,
        .tilemapOffset = 9,
        .flag = F_PURPLE_AZURILL,
        .inSelectionFlags = F_PURPLE_AZURILL,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_P_AZURILL,
    },
    [SQU_PURPLE_SKITTY] = {
        .spriteIdOffset = 2,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 3,
        .row = 3,
        .x = 23,
        .y = 16,
        .var05 = 9,
        .tilemapOffset = 9,
        .flag = F_PURPLE_SKITTY,
        .inSelectionFlags = F_PURPLE_SKITTY,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_P_SKITTY,
    },
    [SQU_PURPLE_MAKUHITA] = {
        .spriteIdOffset = 11,
        .baseMultiplier = NUM_ROULETTE_SLOTS,
        .column = 4,
        .row = 3,
        .x = 26,
        .y = 16,
        .var05 = 9,
        .tilemapOffset = 9,
        .flag = F_PURPLE_MAKUHITA,
        .inSelectionFlags = F_PURPLE_MAKUHITA,
        .flashFlags = F_FLASH_ICON | F_FLASH_COLOR_P_MAKUHITA,
    },
};

static const struct RouletteSlot sRouletteSlots[] =
{
    {
        .id1 = 0,
        .id2 = 1,
        .gridSquare = SQU_ORANGE_WYNAUT,
        .flag = F_ORANGE_WYNAUT,
    },
    {
        .id1 = 1,
        .id2 = 3,
        .gridSquare = SQU_GREEN_AZURILL,
        .flag = F_GREEN_AZURILL,
    },
    {
        .id1 = 2,
        .id2 = 5,
        .gridSquare = SQU_PURPLE_SKITTY,
        .flag = F_PURPLE_SKITTY,
    },
    {
        .id1 = 3,
        .id2 = 7,
        .gridSquare = SQU_ORANGE_MAKUHITA,
        .flag = F_ORANGE_MAKUHITA,
    },
    {
        .id1 = 4,
        .id2 = 9,
        .gridSquare = SQU_GREEN_WYNAUT,
        .flag = F_GREEN_WYNAUT,
    },
    {
        .id1 = 5,
        .id2 = 11,
        .gridSquare = SQU_PURPLE_AZURILL,
        .flag = F_PURPLE_AZURILL,
    },
    {
        .id1 = 6,
        .id2 = 13,
        .gridSquare = SQU_ORANGE_SKITTY,
        .flag = F_ORANGE_SKITTY,
    },
    {
        .id1 = 7,
        .id2 = 15,
        .gridSquare = SQU_GREEN_MAKUHITA,
        .flag = F_GREEN_MAKUHITA,
    },
    {
        .id1 = 8,
        .id2 = 17,
        .gridSquare = SQU_PURPLE_WYNAUT,
        .flag = F_PURPLE_WYNAUT,
    },
    {
        .id1 = 9,
        .id2 = 19,
        .gridSquare = SQU_ORANGE_AZURILL,
        .flag = F_ORANGE_AZURILL,
    },
    {
        .id1 = 10,
        .id2 = 21,
        .gridSquare = SQU_GREEN_SKITTY,
        .flag = F_GREEN_SKITTY,
    },
    {
        .id1 = 11,
        .id2 = 23,
        .gridSquare = SQU_PURPLE_MAKUHITA,
        .flag = F_PURPLE_MAKUHITA,
    },
};
static const u8 sTableMinBets[] = {1, 3, 1, 6};

static const struct RouletteTable sRouletteTables[] =
{
    // Left table
    {
        .minBet = 1,
        .randDistanceHigh = DEGREES_PER_SLOT * 2,
        .randDistanceLow = DEGREES_PER_SLOT,
        .wheelSpeed = 1,
        .wheelDelay = 1,
        .shroomish = {
            .startAngle = 45,
            .dropAngle = 30,
            .fallSlowdown = 1,
        },
        .taillow = {
            .baseDropDelay = 75,
            .rightStartAngle = 27,
            .leftStartAngle = 24,
        },
        .ballSpeed = 10,
        .baseTravelDist = 360,
        .var1C = -0.5f
    },
    // Right table
    {
        .minBet = 3,
        .randDistanceHigh = DEGREES_PER_SLOT,
        .randDistanceLow = DEGREES_PER_SLOT / 2,
        .wheelSpeed = 1,
        .wheelDelay = 0,
        .shroomish = {
            .startAngle = 75,
            .dropAngle = 60,
            .fallSlowdown = 2,
        },
        .taillow = {
            .baseDropDelay = 0,
            .rightStartAngle = 54,
            .leftStartAngle = 48,
        },
        .ballSpeed = 10,
        .baseTravelDist = 270,
        .var1C = -1.0f
    }
};

// Data to flash the color indicator for each slot on the roulette wheel
static const struct RouletteFlashSettings sFlashData_Colors[NUM_ROULETTE_SLOTS + 1] =
{
    { // F_FLASH_COLOR_O_WYNAUT
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(0) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_G_AZURILL
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(0) + 10,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_P_SKITTY
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(1) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_O_MAKUHITA
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(5) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_G_WYNAUT
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(5) + 10,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_P_AZURILL
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(6) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_O_SKITTY
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(7) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_G_MAKUHITA
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(7) + 10,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_P_WYNAUT
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(8) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_O_AZURILL
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(9) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_G_SKITTY
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(9) + 10,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_COLOR_P_MAKUHITA
        .color = FLASHUTIL_USE_EXISTING_COLOR,
        .paletteOffset = BG_PLTT_ID(10) + 5,
        .numColors = 1,
        .delay = 1,
        .unk6 = -1,
        .numFadeCycles = 8,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    { // F_FLASH_OUTER_EDGES
        .color = RGB(22, 30, 29),
        .paletteOffset = BG_PLTT_ID(2) + 8,
        .numColors = 2,
        .delay = 10,
        .unk6 = -1,
        .numFadeCycles = 14,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
};

// Data to flash any Pokémon icon (F_FLASH_ICON) on the roulette wheel. One entry for each color row
// Each poke icon flashes with the tint of the row color it belongs to, so the Pokémon itself is irrelevant
static const struct RouletteFlashSettings sFlashData_PokeIcons[NUM_BOARD_COLORS] =
{
    [GET_ROW_IDX(ROW_ORANGE)] = {
        .color = RGB(31, 31, 20),
        .paletteOffset = OBJ_PLTT_ID(0) + 1,
        .numColors = 5,
        .delay = 30,
        .unk6 = -1,
        .numFadeCycles = 14,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    [GET_ROW_IDX(ROW_GREEN)] = {
        .color = RGB(27, 31, 31),
        .paletteOffset = OBJ_PLTT_ID(0) + 6,
        .numColors = 5,
        .delay = 30,
        .unk6 = -1,
        .numFadeCycles = 14,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    },
    [GET_ROW_IDX(ROW_PURPLE)] = {
        .color = RGB(31, 27, 31),
        .paletteOffset = OBJ_PLTT_ID(0) + 11,
        .numColors = 5,
        .delay = 30,
        .unk6 = -1,
        .numFadeCycles = 14,
        .unk7_5 = -2,
        .colorDeltaDir = 0,
    }
};

static const struct YesNoFuncTable sYesNoTable_AcceptMinBet =
{
    Task_AcceptMinBet,
    Task_DeclineMinBet
};

static const struct YesNoFuncTable sYesNoTable_KeepPlaying =
{
    Task_ContinuePlaying,
    Task_StopPlaying
};

static void CB2_Roulette(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    if (sRoulette->flashUtil.enabled)
        RouletteFlash_Run(&sRoulette->flashUtil);
}

static void VBlankCB_Roulette(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    UpdateWheelPosition();
    SetGpuReg(REG_OFFSET_BG1HOFS, 0x200 - sRoulette->gridX);

    if (sRoulette->shroomishShadowTimer)
        SetGpuReg(REG_OFFSET_BLDALPHA, sRoulette->shroomishShadowAlpha);

    if (sRoulette->updateGridHighlight)
    {
        DmaCopy16(3, &sRoulette->tilemapBuffers[2][0xE0], (void *)BG_SCREEN_ADDR(4) + 0x1C0, 0x340);
        sRoulette->updateGridHighlight = FALSE;
    }
    switch (sRoulette->selectionRectDrawState)
    {
    case SELECT_STATE_DRAW:
        SetBgAttribute(0, BG_ATTR_CHARBASEINDEX, 0);
        ShowBg(0);
        DmaCopy16(3, &sRoulette->tilemapBuffers[0][0xE0], (void *)BG_SCREEN_ADDR(31) + 0x1C0, 0x340);
        sRoulette->selectionRectDrawState = SELECT_STATE_UPDATE;
        break;
    case SELECT_STATE_UPDATE:
        DmaCopy16(3, &sRoulette->tilemapBuffers[0][0xE0], (void *)BG_SCREEN_ADDR(31) + 0x1C0, 0x340);
        break;
    case SELECT_STATE_ERASE:
        SetBgAttribute(0, BG_ATTR_CHARBASEINDEX, 2);
        ShowBg(0);
        DmaFill16(3, 0, (void *)BG_SCREEN_ADDR(31) + 0x1C0, 0x340);
        sRoulette->selectionRectDrawState = SELECT_STATE_WAIT;
    case SELECT_STATE_WAIT:
        break;
    }
}

static void InitRouletteBgAndWindows(void)
{
    u32 size = 0;

    sRoulette = AllocZeroed(sizeof(*sRoulette));
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(1, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    SetBgTilemapBuffer(0, sRoulette->tilemapBuffers[0]);
    SetBgTilemapBuffer(1, sRoulette->tilemapBuffers[2]);
    SetBgTilemapBuffer(2, sRoulette->tilemapBuffers[6]);
    InitWindows(sWindowTemplates);
    InitTextBoxGfxAndPrinters();
    sTextWindowId = 0;
    sRoulette->gridTilemap = malloc_and_decompress(sGrid_Tilemap, &size);
}

static void FreeRoulette(void)
{
    FREE_AND_SET_NULL(sRoulette->gridTilemap);
    FreeAllWindowBuffers();
    UnsetBgTilemapBuffer(0);
    UnsetBgTilemapBuffer(1);
    UnsetBgTilemapBuffer(2);
    ResetBgsAndClearDma3BusyFlags(0);
    memset(sRoulette, 0, sizeof(*sRoulette));
    FREE_AND_SET_NULL(sRoulette);
}

static void InitRouletteTableData(void)
{
    u8 i;
    u16 bgColors[3] = {RGB(24, 4, 10), RGB(10, 19, 6), RGB(24, 4, 10)}; // 3rd is never used, same as 1st

    sRoulette->tableId = (gSpecialVar_0x8004 & 1);

    if (gSpecialVar_0x8004 & ROULETTE_SPECIAL_RATE)
        sRoulette->isSpecialRate = TRUE;

    sRoulette->wheelSpeed = sRouletteTables[sRoulette->tableId].wheelSpeed;
    sRoulette->wheelDelay = sRouletteTables[sRoulette->tableId].wheelDelay;
    sRoulette->minBet = sTableMinBets[sRoulette->tableId + sRoulette->isSpecialRate * 2];
    sRoulette->unk1 = 1;

    // Left table (with min bet of 1) has red background, other table has green
    if (sRoulette->minBet == 1)
        gPlttBufferUnfaded[BG_PLTT_ID(0)] = gPlttBufferUnfaded[BG_PLTT_ID(5) + 1] = gPlttBufferFaded[BG_PLTT_ID(0)] = gPlttBufferFaded[BG_PLTT_ID(5) + 1] = bgColors[0];
    else
        gPlttBufferUnfaded[BG_PLTT_ID(0)] = gPlttBufferUnfaded[BG_PLTT_ID(5) + 1] = gPlttBufferFaded[BG_PLTT_ID(0)] = gPlttBufferFaded[BG_PLTT_ID(5) + 1] = bgColors[1];

    RouletteFlash_Reset(&sRoulette->flashUtil);

    // Init flash util for flashing the selected colors on the wheel
    // + 1 for the additional entry to flash the outer edges on a win
    for (i = 0; i < NUM_ROULETTE_SLOTS + 1; i++)
    {
        RouletteFlash_Add(&sRoulette->flashUtil, i, &sFlashData_Colors[i]);
    }

    for (i = 0; i < PARTY_SIZE; i++)
    {
        switch (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG))
        {
        case SPECIES_SHROOMISH:
            sRoulette->partySpeciesFlags |= HAS_SHROOMISH;
            break;
        case SPECIES_TAILLOW:
            sRoulette->partySpeciesFlags |= HAS_TAILLOW;
            break;
        }
    }
    RtcCalcLocalTime();
}

// Task data for the roulette game tasks, starting with Task_StartPlaying
#define tMultiplier      data[2]
#define tSelectionId     data[4]
#define tWonBet          data[5]
#define tBallNum         data[6]
#define tTotalBallNum    data[8] // Same as tBallNum but isn't cleared every 6 balls
#define tConsecutiveWins data[11]
#define tWinningSquare   data[12]
#define tCoins           data[13]

static void CB2_LoadRoulette(void)
{
    u8 taskId;

    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        SetVBlankHBlankCallbacksToNull();
        ResetVramOamAndBgCntRegs();
        ResetAllBgsCoordinates();
        break;
    case 1:
        InitRouletteBgAndWindows();
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_NONE |
                                     BLDCNT_TGT2_BG2 |
                                     BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(10, 6));
        break;
    case 2:
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        ResetTempTileDataBuffers();
        break;
    case 3:
        LoadPalette(&sWheel_Pal, BG_PLTT_ID(0), 14 * PLTT_SIZE_4BPP);
        DecompressAndCopyTileDataToVram(1, gRouletteMenu_Gfx, 0, 0, 0);
        DecompressAndCopyTileDataToVram(2, gRouletteWheel_Gfx, 0, 0, 0);
        break;
    case 4:
        if (FreeTempTileDataBuffersIfPossible())
            return;

        InitRouletteTableData();
        CopyToBgTilemapBuffer(2, sWheel_Tilemap, 0, 0);
        break;
    case 5:
        LoadOrFreeMiscSpritePalettesAndSheets(FALSE);
        CreateWheelBallSprites();
        CreateWheelCenterSprite();
        CreateInterfaceSprites();
        CreateGridSprites();
        CreateGridBallSprites();
        CreateWheelIconSprites();
        break;
    case 6:
        AnimateSprites();
        BuildOamBuffer();
        SetCreditDigits(GetCoins());
        SetBallCounterNumLeft(BALLS_PER_ROUND);
        SetMultiplierSprite(SELECTION_NONE);
        DrawGridBackground(SELECTION_NONE);
        DrawStdWindowFrame(sTextWindowId, FALSE);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_ControlsInstruction, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        gSpriteCoordOffsetX = -60;
        gSpriteCoordOffsetY = 0;
        break;
    case 7:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_OBJ_ON);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        break;
    case 8:
        EnableInterrupts(INTR_FLAG_VBLANK);
        SetVBlankCallback(VBlankCB_Roulette);
        BeginHardwarePaletteFade(0xFF, 0, 16, 0, 1);
        taskId = sRoulette->playTaskId = CreateTask(Task_StartPlaying, 0);
        gTasks[taskId].tBallNum = BALLS_PER_ROUND;
        gTasks[taskId].tCoins = GetCoins();
        AlertTVThatPlayerPlayedRoulette(GetCoins());
        sRoulette->spinTaskId = CreateTask(Task_SpinWheel, 1);
        SetMainCallback2(CB2_Roulette);
        return;
    }
    gMain.state++;
}

static void Task_SpinWheel(u8 taskId)
{
    s16 sin;
    s16 cos;

    if (sRoulette->wheelDelayTimer++ == sRoulette->wheelDelay)
    {
        sRoulette->wheelDelayTimer = 0;
        if ((sRoulette->wheelAngle -= sRoulette->wheelSpeed) < 0)
            sRoulette->wheelAngle = 360 - sRoulette->wheelSpeed;
    }
    sin = Sin2(sRoulette->wheelAngle);
    cos = Cos2(sRoulette->wheelAngle);
    sin = sin / 16;
    sRoulette->wheelRotation.a = sRoulette->wheelRotation.d = cos / 16;
    sRoulette->wheelRotation.b =  sin;
    sRoulette->wheelRotation.c = -sin;
}

static void Task_StartPlaying(u8 taskId)
{
    if (UpdatePaletteFade() == 0)
    {
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_NONE |
                                     BLDCNT_TGT2_BG2 |
                                     BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 8));
        gTasks[taskId].tBallNum = 0;
        ResetBallDataForNewSpin(taskId);
        ResetHits();
        HideWheelBalls();
        DrawGridBackground(SELECTION_NONE);
        SetBallCounterNumLeft(BALLS_PER_ROUND);
        StartTaskAfterDelayOrInput(taskId, Task_ContinuePlaying, NO_DELAY, A_BUTTON | B_BUTTON);
    }
}

static void Task_AskKeepPlaying(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DrawStdWindowFrame(sTextWindowId, FALSE);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_KeepPlaying, 0, 1, TEXT_SKIP_DRAW, 0);
    CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
    DoYesNoFuncWithChoice(taskId, &sYesNoTable_KeepPlaying);
}

static void Task_ContinuePlaying(u8 taskId)
{
    ClearStdWindowAndFrame(0, TRUE);
    gTasks[taskId].func = Task_SelectFirstEmptySquare;
}

static void Task_StopPlaying(u8 taskId)
{
    DestroyTask(sRoulette->spinTaskId);
    ExitRoulette(taskId);
}

static void UpdateGridSelectionRect(u8 selectionId)
{
    u8 temp0, temp1;
    switch (selectionId)
    {
    case SELECTION_NONE:
        FillTilemapRect(&sRoulette->tilemapBuffers[0][0], 0, 14, 7, 16, 13);
        break;
    case COL_WYNAUT:
    case COL_AZURILL:
    case COL_SKITTY:
    case COL_MAKUHITA:
        temp0 = (selectionId * 3 + 14);
        FillTilemapRect(&sRoulette->tilemapBuffers[0][0], 0, 14, 7, 16, 13);
        SetTilemapRect(&sRoulette->tilemapBuffers[0][0], &sRoulette->gridTilemap[281], temp0, 7, 3, 13);
        break;
    case ROW_ORANGE:
    case ROW_GREEN:
    case ROW_PURPLE:
        temp1 = ((selectionId - 1) / 5 * 3 + 10);
        FillTilemapRect(&sRoulette->tilemapBuffers[0][0], 0, 14, 7, 16, 13);
        SetTilemapRect(&sRoulette->tilemapBuffers[0][0], &sRoulette->gridTilemap[320], 14, temp1, 16, 3);
        break;
    // Individual square
    default:
        temp0 = GET_COL(selectionId) * 3 + 14;
        temp1 = ((selectionId - 1) / 5 * 3 + 7);
        FillTilemapRect(&sRoulette->tilemapBuffers[0][0], 0, 14, 7, 16, 13);
        SetTilemapRect(&sRoulette->tilemapBuffers[0][0], &sRoulette->gridTilemap[272], temp0, temp1, 3, 3);
        break;
    }
}

static void UpdateGridSelection(u8 taskId)
{
    SetMultiplierSprite(gTasks[taskId].tSelectionId);
    UpdateGridSelectionRect(gTasks[taskId].tSelectionId);
}

static void Task_StartHandleBetGridInput(u8 taskId)
{
    sRoulette->selectionRectDrawState = SELECT_STATE_DRAW;
    UpdateGridSelectionRect(gTasks[taskId].tSelectionId);
    sRoulette->wheelDelay = 2;
    sRoulette->wheelDelayTimer = 0;
    gTasks[taskId].func = Task_HandleBetGridInput;
}

static void Task_SelectFirstEmptySquare(u8 taskId)
{
    s16 i;

    if (sRoulette->hitFlags & F_ORANGE_ROW)
    {
        // If the whole orange row is filled, get first in green row
        for (i = SQU_GREEN_WYNAUT; i < SQU_GREEN_MAKUHITA; i++)
        {
            if (!(sRoulette->hitFlags & sGridSelections[i].flag))
                break;
        }
    }
    else
    {
        // Otherwise get first in orange row
        // With only 6 balls both rows can't be filled, no need to check purple row
        for (i = SQU_ORANGE_WYNAUT; i <= SQU_ORANGE_MAKUHITA; i++) // <= is accidental, but it will never get that far
        {
            if (!(sRoulette->hitFlags & sGridSelections[i].flag))
                break;
        }
    }
    gTasks[taskId].tSelectionId = i;
    ResetBallDataForNewSpin(taskId);
    DrawGridBackground(gTasks[taskId].tSelectionId);
    SetMultiplierSprite(gTasks[taskId].tSelectionId);
    FlashSelectionOnWheel(gTasks[taskId].tSelectionId);
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].func = Task_StartHandleBetGridInput;
}

static bool8 CanMoveSelectionInDir(s16 *selectionId, u8 dir)
{
    s8 temp1 = 0;
    s8 temp = 0;
    s8 moveOffsets[4] = {-5, 5, -1, 1};
    s8 originalSelection = *selectionId;

    switch (dir)
    {
    case 0: // UP
    case 1: // DOWN
        temp1 = GET_COL(*selectionId);
        temp = temp1 + ROW_PURPLE;
        if (temp1 == SELECTION_NONE)
            temp1 = 5;
        break;
    case 2: // LEFT
    case 3: // RIGHT
        temp1 = GET_ROW(*selectionId);
        temp = temp1 + COL_MAKUHITA;
        if (temp1 == SELECTION_NONE)
            temp1 = 1;
        break;
    }

    *selectionId += moveOffsets[dir];

    if (*selectionId < temp1)
        *selectionId = temp;

    if (*selectionId > temp)
        *selectionId = temp1;

    if (*selectionId != originalSelection)
        return TRUE;

    return FALSE;
}

static void ProcessBetGridInput(u8 taskId)
{
    u8 headerOffset = 0;
    bool8 dirPressed = FALSE;
    if ((!(JOY_NEW(DPAD_UP))    || ((dirPressed = TRUE) && CanMoveSelectionInDir(&gTasks[taskId].tSelectionId, 0)))
     && (!(JOY_NEW(DPAD_DOWN))  || ((dirPressed = TRUE) && CanMoveSelectionInDir(&gTasks[taskId].tSelectionId, 1)))
     && (!(JOY_NEW(DPAD_LEFT))  || ((dirPressed = TRUE) && CanMoveSelectionInDir(&gTasks[taskId].tSelectionId, 2)))
     && (!(JOY_NEW(DPAD_RIGHT)) || ((dirPressed = TRUE) && CanMoveSelectionInDir(&gTasks[taskId].tSelectionId, 3)))
     && (dirPressed))
    {
        u8 i;
        DrawGridBackground(gTasks[taskId].tSelectionId);
        UpdateGridSelection(taskId);
        gTasks[taskId].data[1] = 0;
        PlaySE(SE_SELECT);
        RouletteFlash_Stop(&sRoulette->flashUtil, 0xFFFF);
        sRoulette->flashUtil.palettes[FLASH_ICON].available = sRoulette->flashUtil.palettes[FLASH_ICON_2].available = sRoulette->flashUtil.palettes[FLASH_ICON_3].available = FALSE;
        FlashSelectionOnWheel(gTasks[taskId].tSelectionId);

        // Switch all the poke (column) headers to gray outlines
        for (i = 0; i < NUM_BOARD_POKES; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].oam.tileNum =
            gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].anims)->type;
        }
        // If the current selection is a column with at least 1 unhit space, fill in the header
        if ((u16)(gTasks[taskId].tSelectionId - 1) < COL_MAKUHITA && !(sRoulette->hitFlags & sGridSelections[gTasks[taskId].tSelectionId].flag))
        {
            headerOffset = gTasks[taskId].tSelectionId - 1;
            gSprites[sRoulette->spriteIds[headerOffset + SPR_POKE_HEADERS]].oam.tileNum =
            gSprites[sRoulette->spriteIds[headerOffset + SPR_POKE_HEADERS]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[headerOffset + SPR_POKE_HEADERS]].anims + 1)->type;
        }
    }
}

static void Task_StartSpin(u8 taskId)
{
    IncrementDailyRouletteUses();
    sRoulette->selectionRectDrawState = SELECT_STATE_ERASE;
    if (sRoulette->minBet == 1)
        sRoulette->wheelDelay = 1;
    else
        sRoulette->wheelDelay = 0;
    sRoulette->wheelDelayTimer = 0;
    gTasks[taskId].data[1] = 32;
    gTasks[taskId].func = Task_SlideGridOffscreen;
}

static void Task_PlaceBet(u8 taskId)
{
    sRoulette->betSelection[sRoulette->curBallNum] = gTasks[taskId].tSelectionId;
    gTasks[taskId].tMultiplier = GetMultiplier(sRoulette->betSelection[sRoulette->curBallNum]);
    SetMultiplierSprite(sRoulette->betSelection[sRoulette->curBallNum]);
    if ((gTasks[taskId].tCoins -= sRoulette->minBet) < 0)
        gTasks[taskId].tCoins = 0;
    SetCreditDigits(gTasks[taskId].tCoins);
    gTasks[taskId].func = Task_StartSpin;
}

static void Task_HandleBetGridInput(u8 taskId)
{
    ProcessBetGridInput(taskId);

    // Flash selection rect
    switch (gTasks[taskId].data[1])
    {
    case 0:
        UpdateGridSelectionRect(gTasks[taskId].tSelectionId);
        gTasks[taskId].data[1]++;
        break;
    case 30:
        UpdateGridSelectionRect(SELECTION_NONE);
        gTasks[taskId].data[1]++;
        break;
    case 59:
        gTasks[taskId].data[1] = 0;
        break;
    default:
        gTasks[taskId].data[1]++;
    }

    if (JOY_NEW(A_BUTTON))
    {
        if (sRoulette->hitFlags & sGridSelections[gTasks[taskId].tSelectionId].flag)
        {
            // Ball has already landed on this space
            PlaySE(SE_BOO);
        }
        else
        {
            m4aSongNumStart(SE_SHOP);
            gTasks[taskId].func = Task_PlaceBet;
        }
    }
}

static void Task_SlideGridOffscreen(u8 taskId)
{
    if (gTasks[taskId].data[1]-- > 0)
    {
        // Slide wheel over
        if (gTasks[taskId].data[1] > 2)
            gSpriteCoordOffsetX += 2;

        // Slide grid over
        if ((sRoulette->gridX += 4) == 104)
            gSprites[sRoulette->spriteIds[SPR_MULTIPLIER]].callback = &SpriteCallbackDummy;
    }
    else
    {
        ShowHideGridIcons(TRUE, -1);
        ShowHideGridBalls(TRUE, -1);
        gTasks[taskId].func = Task_InitBallRoll;
        gTasks[taskId].data[1] = 0;
    }
}

// Each table has a set base distance used to determine how far the ball will travel
// Each roll a random value is generated to add onto this distance
// Half the value returned by this function is the max distance that can be added on per roll
// i.e. the lower this value is, the closer the roll will be to a consistent distance
// Odds of a lower value increase as play continues, if the player has Shroomish and/or Taillow in the party, and dependent on the time
static u8 GetRandomForBallTravelDistance(u16 ballNum, u16 rand)
{
    switch (sRoulette->partySpeciesFlags)
    {
    case HAS_SHROOMISH:
    case HAS_TAILLOW:
        // one of the two is in party
        if (gLocalTime.hours > 3 && gLocalTime.hours < 10)
        {
            if (ballNum < BALLS_PER_ROUND * 2 || (rand & 1))
                return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
            else
                return 1;
        }
        else if (!(rand & 3))
        {
            return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
        }
        else
        {
            return sRouletteTables[sRoulette->tableId].randDistanceLow;
        }
        break;
    case HAS_SHROOMISH | HAS_TAILLOW:
        // both are in party
        if (gLocalTime.hours > 3 && gLocalTime.hours < 11)
        {
            if (ballNum < BALLS_PER_ROUND || (rand & 1))
                return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
            else
                return 1;
        }
        else if ((rand & 1) && ballNum > BALLS_PER_ROUND)
        {
            return sRouletteTables[sRoulette->tableId].randDistanceLow / 4;
        }
        else
        {
            return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
        }
        break;
    case 0:
    default:
        // neither is in party
        if (gLocalTime.hours > 3 && gLocalTime.hours < 10)
        {
            if (!(rand & 3))
                return 1;
            else
                return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
        }
        else if (!(rand & 3))
        {
            if (ballNum > BALLS_PER_ROUND * 2)
                return sRouletteTables[sRoulette->tableId].randDistanceLow / 2;
            else
                return sRouletteTables[sRoulette->tableId].randDistanceLow;
        }
        else if (rand & (1 << 15))
        {
            if (ballNum > BALLS_PER_ROUND * 2)
                return sRouletteTables[sRoulette->tableId].randDistanceLow;
            else
                return sRouletteTables[sRoulette->tableId].randDistanceHigh;
        }
        else
        {
            return sRouletteTables[sRoulette->tableId].randDistanceHigh * 2;
        }
        break;
    }
}

static void Task_InitBallRoll(u8 taskId)
{
    u8 randTravelMod;
    s8 randTravelDist;
    s8 startAngleId;
    u16 travelDist = 0;
    u16 rand;
    u16 randmod;
    u16 startAngles[4] = {0, 180, 90, 270}; // possible angles to start ball from

    rand = Random();
    randmod = rand % 100;
    sRoulette->curBallSpriteId = gTasks[taskId].tBallNum;
    // BALL_STATE_ROLLING set below
    sRoulette->ballState = sRoulette->hitSlot = sRoulette->stuckHitSlot = 0;
    randTravelMod = GetRandomForBallTravelDistance(gTasks[taskId].tTotalBallNum, rand);
    randTravelDist = (rand % randTravelMod) - (randTravelMod / 2);

    if (gLocalTime.hours < 13)
        startAngleId = 0;
    else
        startAngleId = 1;

    if (randmod < 80)
        startAngleId *= 2;
    else
        startAngleId = (1 - startAngleId) * 2;

    sRoulette->ballTravelDist = travelDist = sRouletteTables[sRoulette->tableId].baseTravelDist + randTravelDist;

    travelDist = S16TOPOSFLOAT(travelDist) / 5.0f;
    sRoulette->ballTravelDistFast = travelDist * 3;
    sRoulette->ballTravelDistSlow = sRoulette->ballTravelDistMed = travelDist;

    sRoulette->ballAngle = S16TOPOSFLOAT(startAngles[(rand & 1) + startAngleId]);
    sRoulette->ballAngleSpeed = S16TOPOSFLOAT(sRouletteTables[sRoulette->tableId].ballSpeed);
    sRoulette->ballAngleAccel = ((sRoulette->ballAngleSpeed * 0.5f) - sRoulette->ballAngleSpeed) / S16TOPOSFLOAT(sRoulette->ballTravelDistFast);
    sRoulette->ballDistToCenter = 68.0f;
    sRoulette->ballFallAccel = 0.0f;
    sRoulette->ballFallSpeed = -(8.0f / S16TOPOSFLOAT(sRoulette->ballTravelDistFast));
    sRoulette->varA0 = 36.0f;
    gTasks[taskId].func = Task_RollBall;
}

static void Task_RollBall(u8 taskId)
{
    sRoulette->ballRolling = TRUE;
    sRoulette->ball = &gSprites[sRoulette->spriteIds[sRoulette->curBallSpriteId]];
    sRoulette->ball->callback = SpriteCB_RollBall_Start;
    gTasks[taskId].tBallNum++;
    gTasks[taskId].tTotalBallNum++;
    SetBallCounterNumLeft(BALLS_PER_ROUND - gTasks[taskId].tBallNum);
    m4aSongNumStart(SE_ROULETTE_BALL);
    gTasks[taskId].func = Task_RecordBallHit;
}

static void Task_RecordBallHit(u8 taskId)
{
    // Wait for ball to finish rolling
    if (sRoulette->ballState != BALL_STATE_ROLLING)
    {
        // If the ball got stuck, wait for it to be unstuck
        if (sRoulette->ballStuck)
        {
            if (sRoulette->ballUnstuck)
            {
                sRoulette->ballUnstuck = FALSE;
                sRoulette->ballStuck = FALSE;
            }
        }
        else
        {
            if (gTasks[taskId].data[1] == 0)
            {
                bool8 won = IsHitInBetSelection(RecordHit(taskId, sRoulette->hitSlot), sRoulette->betSelection[sRoulette->curBallNum]);
                gTasks[taskId].tWonBet = won;
                if (won == TRUE)
                    RouletteFlash_Enable(&sRoulette->flashUtil, F_FLASH_OUTER_EDGES);
            }
            if (gTasks[taskId].data[1] <= 60)
            {
                if (JOY_NEW(A_BUTTON))
                    gTasks[taskId].data[1] = 60;
                gTasks[taskId].data[1]++;
            }
            else
            {
                DrawGridBackground(sRoulette->betSelection[sRoulette->curBallNum]);
                ShowHideGridIcons(FALSE, gTasks[taskId].tWinningSquare);
                ShowHideGridBalls(FALSE, gTasks[taskId].tBallNum - 1);
                gTasks[taskId].data[1] = 32;
                gTasks[taskId].func = Task_SlideGridOnscreen;
            }
        }
    }
}

static void Task_SlideGridOnscreen(u8 taskId)
{
    if (gTasks[taskId].data[1]-- > 0)
    {
        // Slide wheel over
        if (gTasks[taskId].data[1] > 2)
            gSpriteCoordOffsetX -= 2;

        // Slide grid over
        if ((sRoulette->gridX -= 4) == 104)
            gSprites[sRoulette->spriteIds[SPR_MULTIPLIER]].callback = SpriteCB_GridSquare;
    }
    else
    {
        ShowHideWinSlotCursor(gTasks[taskId].tWinningSquare);
        if (gTasks[taskId].tWonBet == TRUE)
            gTasks[taskId].data[1] = 121;
        else
            gTasks[taskId].data[1] = 61;
        gTasks[taskId].func = Task_FlashBallOnWinningSquare;
    }
}

static void Task_FlashBallOnWinningSquare(u8 taskId)
{
    if (gTasks[taskId].data[1]-- > 1)
    {
        switch (gTasks[taskId].data[1] % 16)
        {
        case 8:
            // Winning square uncovered
            ShowHideGridIcons(FALSE, -1);
            ShowHideGridBalls(FALSE, -1);
            break;
        case 0:
            // Winning square occluded by ball
            ShowHideGridIcons(FALSE, gTasks[taskId].tWinningSquare);
            ShowHideGridBalls(FALSE, gTasks[taskId].tBallNum - 1);
            break;
        }
    }
    else
    {
        StartTaskAfterDelayOrInput(taskId, Task_PrintSpinResult, 30, 0);
    }
}

static void Task_TryIncrementWins(u8 taskId)
{
    switch (gTasks[taskId].tWonBet)
    {
    case TRUE:
    case 2: // never happens
        if (IsFanfareTaskInactive())
        {
            u32 wins = GetGameStat(GAME_STAT_CONSECUTIVE_ROULETTE_WINS);
            if (wins < ++gTasks[taskId].tConsecutiveWins)
                SetGameStat(GAME_STAT_CONSECUTIVE_ROULETTE_WINS, gTasks[taskId].tConsecutiveWins);
            StartTaskAfterDelayOrInput(taskId, Task_PrintPayout, NO_DELAY, A_BUTTON | B_BUTTON);
        }
        break;
    case FALSE:
    default:
        if (!IsSEPlaying())
        {
            gTasks[taskId].tConsecutiveWins = 0;
            StartTaskAfterDelayOrInput(taskId, Task_EndTurn, NO_DELAY, A_BUTTON | B_BUTTON);
        }
        break;
    }
}

static void Task_PrintSpinResult(u8 taskId)
{
    switch (gTasks[taskId].tWonBet)
    {
    case TRUE:
    case 2: // never happens
        if (gTasks[taskId].tMultiplier == MAX_MULTIPLIER)
        {
            PlayFanfare(MUS_SLOTS_JACKPOT);
            DrawStdWindowFrame(sTextWindowId, FALSE);
            AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_Jackpot, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        }
        else
        {
            PlayFanfare(MUS_SLOTS_WIN);
            DrawStdWindowFrame(sTextWindowId, FALSE);
            AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_ItsAHit, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        }
        break;
    case FALSE:
    default:
        m4aSongNumStart(SE_FAILURE);
        DrawStdWindowFrame(sTextWindowId, FALSE);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_NothingDoing, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        break;
    }
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].func = Task_TryIncrementWins;
}

#define tPayout data[1]

static void Task_GivePayout(u8 taskId)
{
    switch (gTasks[taskId].data[7])
    {
    case 0:
        gTasks[taskId].tCoins++;
        m4aSongNumStart(SE_PIN);
        SetCreditDigits(gTasks[taskId].tCoins);
        if (gTasks[taskId].tCoins >= MAX_COINS)
        {
            gTasks[taskId].tPayout = 0;
        }
        else
        {
            gTasks[taskId].tPayout--;
            gTasks[taskId].data[7]++;
        }
        break;
    case 3:
        m4aSongNumStop(SE_PIN);
        gTasks[taskId].data[7] = 0;
        break;
    default:
        gTasks[taskId].data[7]++;
        break;
    }
    if (gTasks[taskId].tPayout == 0)
        StartTaskAfterDelayOrInput(taskId, Task_EndTurn, NO_DELAY, A_BUTTON | B_BUTTON);
}

static void Task_PrintPayout(u8 taskId)
{
    ConvertIntToDecimalStringN(gStringVar1, (sRoulette->minBet * gTasks[taskId].tMultiplier), STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVarBuffer, Roulette_Text_YouveWonXCoins);
    DrawStdWindowFrame(sTextWindowId, FALSE);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
    gTasks[taskId].tPayout = (sRoulette->minBet * gTasks[taskId].tMultiplier);
    gTasks[taskId].data[7] = 0;
    gTasks[taskId].func = Task_GivePayout;
}

#undef tPayout

static void Task_EndTurn(u8 taskId)
{
    RouletteFlash_Stop(&sRoulette->flashUtil, 0xFFFF);
    sRoulette->flashUtil.palettes[FLASH_ICON].available = sRoulette->flashUtil.palettes[FLASH_ICON_2].available = sRoulette->flashUtil.palettes[FLASH_ICON_3].available = FALSE;
    gSprites[sRoulette->spriteIds[SPR_WHEEL_ICONS + sGridSelections[gTasks[taskId].tWinningSquare].spriteIdOffset]].invisible = TRUE;
    gTasks[taskId].func = Task_TryPrintEndTurnMsg;
}

static void Task_TryPrintEndTurnMsg(u8 taskId)
{
    u8 i = 0;
    gTasks[taskId].tSelectionId = i;
    sRoulette->betSelection[sRoulette->curBallNum] = SELECTION_NONE;
    DrawGridBackground(SELECTION_NONE);
    gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].invisible = TRUE;
    for (i = 0; i < NUM_BOARD_POKES; i++)
    {
        gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].oam.tileNum =
            gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[i + SPR_POKE_HEADERS]].anims)->type;
    }
    if (gTasks[taskId].tCoins >= sRoulette->minBet)
    {
        if (gTasks[taskId].tBallNum == BALLS_PER_ROUND)
        {
            // Reached Ball 6, clear board
            DrawStdWindowFrame(sTextWindowId, FALSE);
            AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_BoardWillBeCleared, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
            StartTaskAfterDelayOrInput(taskId, Task_ClearBoard, NO_DELAY, A_BUTTON | B_BUTTON);
        }
        else if (gTasks[taskId].tCoins == MAX_COINS)
        {
            // Player maxed out coins
            DrawStdWindowFrame(sTextWindowId, FALSE);
            AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_CoinCaseIsFull, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
            StartTaskAfterDelayOrInput(taskId, Task_AskKeepPlaying, NO_DELAY, A_BUTTON | B_BUTTON);
        }
        else
        {
            // No special msg, ask to continue
            gTasks[taskId].func = Task_AskKeepPlaying;
        }
    }
    else
    {
        // Player out of coins
        DrawStdWindowFrame(sTextWindowId, FALSE);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_NoCoinsLeft, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        StartTaskAfterDelayOrInput(taskId, Task_StopPlaying, 60, A_BUTTON | B_BUTTON);
    }
}

static void Task_ClearBoard(u8 taskId)
{
    u8 i = 0;

    gTasks[taskId].tBallNum = 0;
    ResetBallDataForNewSpin(taskId);
    ResetHits();
    HideWheelBalls();
    DrawGridBackground(SELECTION_NONE);
    SetBallCounterNumLeft(BALLS_PER_ROUND);

    for (i = 0; i < NUM_ROULETTE_SLOTS; i++)
    {
        gSprites[sRoulette->spriteIds[i + SPR_WHEEL_ICONS]].invisible = FALSE;
    }

    if (gTasks[taskId].tCoins == MAX_COINS)
    {
        DrawStdWindowFrame(sTextWindowId, FALSE);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, Roulette_Text_CoinCaseIsFull, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        StartTaskAfterDelayOrInput(taskId, Task_AskKeepPlaying, NO_DELAY, A_BUTTON | B_BUTTON);
    }
    else
    {
        gTasks[taskId].func = Task_AskKeepPlaying;
    }
}

static void ExitRoulette(u8 taskId)
{
    RouletteFlash_Stop(&sRoulette->flashUtil, 0xFFFF);
    RouletteFlash_Reset(&sRoulette->flashUtil);
    SetCoins(gTasks[taskId].tCoins);
    if (GetCoins() < sRoulette->minBet)
        gSpecialVar_0x8004 = TRUE;
    else
        gSpecialVar_0x8004 = FALSE;
    TryPutFindThatGamerOnAir(GetCoins());
    BeginHardwarePaletteFade(0xFF, 0, 0, 16, 0);
    gTasks[taskId].func = Task_ExitRoulette;
}

static void Task_ExitRoulette(u8 taskId)
{
    if (UpdatePaletteFade() == 0)
    {
        SetVBlankCallback(NULL);
        gSpriteCoordOffsetX = gSpriteCoordOffsetY = 0;
        ResetVramOamAndBgCntRegs();
        ResetAllBgsCoordinates();
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        FreeRoulette();
        gFieldCallback = FieldCB_ContinueScriptHandleMusic;
        SetMainCallback2(CB2_ReturnToField);
        DestroyTask(taskId);
    }
}

static void Task_WaitForNextTask(u8 taskId)
{
    if (sRoulette->taskWaitDelay == 0 || JOY_NEW(sRoulette->taskWaitKey))
    {
        gTasks[taskId].func = sRoulette->nextTask;
        if (sRoulette->taskWaitKey > 0)
            PlaySE(SE_SELECT);
        sRoulette->nextTask = NULL;
        sRoulette->taskWaitKey = 0;
        sRoulette->taskWaitDelay = 0;
    }
    if (sRoulette->taskWaitDelay != NO_DELAY)
        sRoulette->taskWaitDelay--;
}

static void StartTaskAfterDelayOrInput(u8 taskId, TaskFunc task, u16 delay, u16 key)
{
    sRoulette->prevTask = gTasks[taskId].func;
    if (task == NULL)
        task = sRoulette->prevTask;
    sRoulette->nextTask = task;
    sRoulette->taskWaitDelay = delay;
    if (delay == NO_DELAY && key == 0)
        sRoulette->taskWaitKey = 0xFFFF;
    else
        sRoulette->taskWaitKey = key;
    gTasks[taskId].func = Task_WaitForNextTask;
}

static void ResetBallDataForNewSpin(u8 taskId)
{
    u8 i = 0;
    sRoulette->unk0 = FALSE;
    sRoulette->ballRolling = FALSE;
    sRoulette->ballStuck = FALSE;
    sRoulette->ballUnstuck = FALSE;
    sRoulette->useTaillow = FALSE;

    for (i = 0; i < BALLS_PER_ROUND; i++)
        sRoulette->betSelection[i] = SELECTION_NONE;

    sRoulette->curBallNum = 0;
    gTasks[taskId].data[1] = 0;
}

static void ResetHits(void)
{
    u8 i;
    sRoulette->hitFlags = 0;

    for (i = 0; i < BALLS_PER_ROUND; i++)
        sRoulette->hitSquares[i] = 0;

    for (i = 0; i < NUM_BOARD_POKES; i++)
        sRoulette->pokeHits[i] = 0;

    for (i = 0; i < NUM_BOARD_COLORS; i++)
        sRoulette->colorHits[i] = 0;

    ShowHideGridBalls(TRUE, -1);
}

static u8 RecordHit(u8 taskId, u8 slotId)
{
    u8 i, j;
    u32 columnFlags[NUM_BOARD_POKES] = {
        F_WYNAUT_COL | F_ORANGE_WYNAUT | F_GREEN_WYNAUT | F_PURPLE_WYNAUT,
        F_AZURILL_COL | F_ORANGE_AZURILL | F_GREEN_AZURILL | F_PURPLE_AZURILL,
        F_SKITTY_COL | F_ORANGE_SKITTY | F_GREEN_SKITTY | F_PURPLE_SKITTY,
        F_MAKUHITA_COL | F_ORANGE_MAKUHITA | F_GREEN_MAKUHITA | F_PURPLE_MAKUHITA
    };
    u32 rowFlags[NUM_BOARD_COLORS] = {
        F_ORANGE_ROW | F_ORANGE_WYNAUT | F_ORANGE_AZURILL | F_ORANGE_SKITTY | F_ORANGE_MAKUHITA,
        F_GREEN_ROW | F_GREEN_WYNAUT | F_GREEN_AZURILL | F_GREEN_SKITTY | F_GREEN_MAKUHITA,
        F_PURPLE_ROW | F_PURPLE_WYNAUT | F_PURPLE_AZURILL | F_PURPLE_SKITTY | F_PURPLE_MAKUHITA
    };

    if (slotId >= NUM_ROULETTE_SLOTS)
        return 0;

    sRoulette->hitSquares[gTasks[taskId].tBallNum - 1] = sRouletteSlots[slotId].gridSquare;
    gTasks[taskId].tWinningSquare = sRouletteSlots[slotId].gridSquare;
    sRoulette->hitFlags |= sRouletteSlots[slotId].flag;
    for (i = 0; i < NUM_BOARD_POKES; i++)
    {
        if (sRouletteSlots[slotId].flag & columnFlags[i])
            sRoulette->pokeHits[i]++;
        // If hit every color of a poke, set column completed
        if (sRoulette->pokeHits[i] >= NUM_BOARD_COLORS)
            sRoulette->hitFlags |= columnFlags[i];
    }
    for (j = 0; j < NUM_BOARD_COLORS; j++)
    {
        if (sRouletteSlots[slotId].flag & rowFlags[j])
            sRoulette->colorHits[j]++;
        // If hit every poke of a color, set row completed
        if (sRoulette->colorHits[j] >= NUM_BOARD_POKES)
            sRoulette->hitFlags |= rowFlags[j];
    }
    return sRouletteSlots[slotId].gridSquare;
}

static bool8 IsHitInBetSelection(u8 gridSquare, u8 betSelection)
{
    u8 hit = gridSquare;
    if (--gridSquare < NUM_GRID_SELECTIONS)
    {
        switch (betSelection)
        {
        case SELECTION_NONE:
            return 3; // should never happen, player must place bet
        case COL_WYNAUT:
        case COL_AZURILL:
        case COL_SKITTY:
        case COL_MAKUHITA:
            if (hit == betSelection + ROW_ORANGE
             || hit == betSelection + ROW_GREEN
             || hit == betSelection + ROW_PURPLE)
                return TRUE;
            break;
        case ROW_ORANGE:
        case ROW_GREEN:
        case ROW_PURPLE:
            if (hit >= (betSelection + COL_WYNAUT)
             && hit <= (betSelection + COL_MAKUHITA))
                return TRUE;
            break;
        // Individual square
        default:
            if (hit == betSelection)
                return TRUE;
        }
    }
    return FALSE;
}

static void FlashSelectionOnWheel(u8 selectionId)
{
    u16 flashFlags = 0;
    u8 numSelected;
    u16 palOffset;
    u8 i;

    switch (selectionId)
    {
    case ROW_ORANGE:
    case ROW_GREEN:
    case ROW_PURPLE:
        // Flash colors of row selection
        for (i = (selectionId + 1); i < (selectionId + 5); i++)
        {
            if (!(sRoulette->hitFlags & sGridSelections[i].flag))
                flashFlags |= sGridSelections[i].flashFlags;
        }
        RouletteFlash_Enable(&sRoulette->flashUtil, flashFlags &= ~(F_FLASH_ICON));
        break;
    default:
    {
        // Selection is either a column or individual square
        struct RouletteFlashSettings iconFlash[NUM_BOARD_COLORS];
        memcpy(iconFlash, sFlashData_PokeIcons, sizeof(iconFlash));

        if (selectionId >= COL_WYNAUT && selectionId <= COL_MAKUHITA)
            numSelected = NUM_BOARD_COLORS; // Selection is full column
        else
            numSelected = 1;

        palOffset = GET_ROW_IDX(selectionId);
        switch (GET_COL(selectionId))
        {
        // The specific color of the poke it references doesn't matter, because the icons of a poke share a palette
        // So it just uses the first sprite ID of each
        case COL_WYNAUT:
            palOffset = PLTT_ID(gSprites[sRoulette->spriteIds[SPR_WHEEL_ICON_ORANGE_WYNAUT]].oam.paletteNum);
            break;
        case COL_AZURILL:
            palOffset = PLTT_ID(gSprites[sRoulette->spriteIds[SPR_WHEEL_ICON_GREEN_AZURILL]].oam.paletteNum);
            break;
        case COL_SKITTY:
            palOffset = PLTT_ID(gSprites[sRoulette->spriteIds[SPR_WHEEL_ICON_PURPLE_SKITTY]].oam.paletteNum);
            break;
        case COL_MAKUHITA:
            palOffset = PLTT_ID(gSprites[sRoulette->spriteIds[SPR_WHEEL_ICON_ORANGE_MAKUHITA]].oam.paletteNum);
            break;
        }
        if (numSelected == 1)
        {
            // Selection is individual square, add entry in flash util for its icon
            if (!(sRoulette->hitFlags & sGridSelections[selectionId].flag))
            {
                iconFlash[GET_ROW_IDX(selectionId)].paletteOffset += palOffset;
                RouletteFlash_Add(&sRoulette->flashUtil, NUM_ROULETTE_SLOTS + 1, &iconFlash[GET_ROW_IDX(selectionId)]);
            }
            else
            {
                // Square was already hit, don't flash it
                break;
            }
        }
        else
        {
            // Selection is full column, add entry in flash util for each unhit space's icon
            // If there is only 1 unhit space, also add its flags so its color will flash as well
            for (i = 0; i < NUM_BOARD_COLORS; i++)
            {
                u8 columnSlotId = i * 5 + selectionId + 5;
                if (!(sRoulette->hitFlags & sGridSelections[columnSlotId].flag))
                {
                    iconFlash[GET_ROW_IDX(columnSlotId)].paletteOffset += palOffset;
                    RouletteFlash_Add(&sRoulette->flashUtil, i + NUM_ROULETTE_SLOTS + 1, &iconFlash[GET_ROW_IDX(columnSlotId)]);
                    if (numSelected == 3)
                        flashFlags = sGridSelections[columnSlotId].flashFlags;
                    numSelected--;
                }
            }
            // If there was more than 1 space in the column, reset flags; only icons will flash
            if (numSelected != 2)
                flashFlags = 0;
        }
        // Do flash
        RouletteFlash_Enable(&sRoulette->flashUtil, flashFlags |= sGridSelections[selectionId].flashFlags);
        break;
    }
    }
}

static void DrawGridBackground(u8 selectionId)
{
    vu8 i, j;
    vu16 x, y;
    vu8 tilemapOffset;
    u8 selectionIds[NUM_BOARD_POKES >= NUM_BOARD_COLORS ? NUM_BOARD_POKES + 1 : NUM_BOARD_COLORS + 1];
    u8 numSquares;
    sRoulette->updateGridHighlight = TRUE;
    ShowHideGridIcons(FALSE, 0);
    SetTilemapRect(sRoulette->tilemapBuffers[2], sRoulette->gridTilemap, 14, 7, 16, 13);

    // Highlight selected square
    // (just buffers the highlight, it's actually drawn in VBlankCB_Roulette)
    switch (selectionId)
    {
    case SELECTION_NONE:
        return;
    case COL_WYNAUT:
    case COL_AZURILL:
    case COL_SKITTY:
    case COL_MAKUHITA:
        numSquares = NUM_BOARD_COLORS + 1; // For each poke column, 3 colors and a header
        for (i = 0; i < numSquares; i++)
            selectionIds[i] = i * ROW_ORANGE + selectionId;
        break;
    case ROW_ORANGE:
    case ROW_GREEN:
    case ROW_PURPLE:
        numSquares = NUM_BOARD_POKES + 1; // For each color row, 4 pokes and a header
        for (i = 0; i < numSquares; i++)
            selectionIds[i] = i + selectionId;
        break;
    // Individual square
    default:
        numSquares = 1;
        selectionIds[0] = selectionId;
    }
    for (i = 0; i < numSquares; i++)
    {
        tilemapOffset = sGridSelections[selectionIds[i]].tilemapOffset;
        x = sGridSelections[selectionIds[i]].x;

        // Grid square highlight is drawn in 9 segments, starting from the top left of the square
        // Each iteration of this loop draws 3 segments to form a single horizontal segment
        for (j = 0; j < 3; j++)
        {
            y = (sGridSelections[selectionIds[i]].y + j) * 32;
            sRoulette->tilemapBuffers[2][x + y + 0] = sRoulette->gridTilemap[(tilemapOffset + j) * 3 + 208 + 0];
            sRoulette->tilemapBuffers[2][x + y + 1] = sRoulette->gridTilemap[(tilemapOffset + j) * 3 + 208 + 1];
            sRoulette->tilemapBuffers[2][x + y + 2] = sRoulette->gridTilemap[(tilemapOffset + j) * 3 + 208 + 2];
        }
    }
}

static u8 GetMultiplier(u8 selectionId)
{
    u8 multipliers[5] = {0, 3, 4, 6, MAX_MULTIPLIER};

    if (selectionId > NUM_GRID_SELECTIONS)
        selectionId = 0;

    switch (sGridSelections[selectionId].baseMultiplier)
    {
    case NUM_BOARD_COLORS:
        selectionId = GET_ROW_IDX(selectionId);
        // If already hit all pokes of this color, multiplier is 0
        if (sRoulette->colorHits[selectionId] >= NUM_BOARD_POKES)
            return 0;
        return multipliers[sRoulette->colorHits[selectionId] + 1];
    case NUM_BOARD_POKES:
        selectionId = GET_COL_IDX(selectionId);
        // If already hit all colors of this poke, multiplier is 0
        if (sRoulette->pokeHits[selectionId] >= NUM_BOARD_COLORS)
            return 0;
        return multipliers[sRoulette->pokeHits[selectionId] + 2];
    case NUM_ROULETTE_SLOTS:
        // If square has been hit already, multiplier is 0
        if (sRoulette->hitFlags & sGridSelections[selectionId].flag)
            return 0;
        return multipliers[ARRAY_COUNT(multipliers) - 1];
    }
    return 0;
}

static void UpdateWheelPosition(void)
{
    s32 bg2x;
    s32 bg2y;
    SetGpuReg(REG_OFFSET_BG2PA, sRoulette->wheelRotation.a);
    SetGpuReg(REG_OFFSET_BG2PB, sRoulette->wheelRotation.b);
    SetGpuReg(REG_OFFSET_BG2PC, sRoulette->wheelRotation.c);
    SetGpuReg(REG_OFFSET_BG2PD, sRoulette->wheelRotation.d);
    bg2x = 0x7400 - sRoulette->wheelRotation.a * (gSpriteCoordOffsetX + 116)
                - sRoulette->wheelRotation.b * (gSpriteCoordOffsetY + 80);
    bg2y = 0x5400 - sRoulette->wheelRotation.c * (gSpriteCoordOffsetX + 116)
                - sRoulette->wheelRotation.d * (gSpriteCoordOffsetY + 80);
    SetGpuReg(REG_OFFSET_BG2X_L, bg2x);
    SetGpuReg(REG_OFFSET_BG2X_H, (bg2x & 0x0fff0000) >> 16);
    SetGpuReg(REG_OFFSET_BG2Y_L, bg2y);
    SetGpuReg(REG_OFFSET_BG2Y_H, (bg2y & 0x0fff0000) >> 16);
}

static const u8 sFiller[3] = {};
static const u16 sShadow_Pal[] = INCBIN_U16("graphics/roulette/shadow.gbapal");
static const u16 sBall_Pal[] = INCBIN_U16("graphics/roulette/ball.gbapal");
static const u16 sBallCounter_Pal[] = INCBIN_U16("graphics/roulette/ball_counter.gbapal");
static const u16 sCursor_Pal[] = INCBIN_U16("graphics/roulette/cursor.gbapal");
static const u16 sCredit_Pal[] = INCBIN_U16("graphics/roulette/credit.gbapal");
static const u16 sShroomish_Pal[] = INCBIN_U16("graphics/roulette/shroomish.gbapal");
static const u16 sTaillow_Pal[] = INCBIN_U16("graphics/roulette/tailow.gbapal");
static const u16 sGridIcons_Pal[] = INCBIN_U16("graphics/roulette/grid_icons.gbapal");
static const u16 sWynaut_Pal[] = INCBIN_U16("graphics/roulette/wynaut.gbapal");
static const u16 sAzurill_Pal[] = INCBIN_U16("graphics/roulette/azurill.gbapal");
static const u16 sSkitty_Pal[] = INCBIN_U16("graphics/roulette/skitty.gbapal");
static const u16 sMakuhita_Pal[] = INCBIN_U16("graphics/roulette/makuhita.gbapal");
static const u16 sUnused1_Pal[] = INCBIN_U16("graphics/roulette/unused_1.gbapal");
static const u16 sUnused2_Pal[] = INCBIN_U16("graphics/roulette/unused_2.gbapal");
static const u16 sUnused3_Pal[] = INCBIN_U16("graphics/roulette/unused_3.gbapal");
static const u16 sUnused4_Pal[] = INCBIN_U16("graphics/roulette/unused_4.gbapal");
static const u32 sBall_Gfx[] = INCBIN_U32("graphics/roulette/ball.4bpp.lz");
static const u32 sBallCounter_Gfx[] = INCBIN_U32("graphics/roulette/ball_counter.4bpp.lz");
static const u32 sShroomishTaillow_Gfx[] = INCBIN_U32("graphics/roulette/roulette_tilt.4bpp.lz");
static const u32 sGridIcons_Gfx[] = INCBIN_U32("graphics/roulette/grid_icons.4bpp.lz");
static const u32 sWheelIcons_Gfx[] = INCBIN_U32("graphics/roulette/wheel_icons.4bpp.lz");
static const u32 sShadow_Gfx[] = INCBIN_U32("graphics/roulette/shadow.4bpp.lz");
static const u32 sCursor_Gfx[] = INCBIN_U32("graphics/roulette/cursor.4bpp.lz");

static const struct SpritePalette sSpritePalettes[] =
{
    { .data = sShadow_Pal,      .tag = PALTAG_SHADOW },
    { .data = sBall_Pal,        .tag = PALTAG_BALL },
    { .data = sBallCounter_Pal, .tag = PALTAG_BALL_COUNTER },
    { .data = sCursor_Pal,      .tag = PALTAG_CURSOR },
    { .data = sCredit_Pal,      .tag = PALTAG_INTERFACE },
    { .data = sShroomish_Pal,   .tag = PALTAG_SHROOMISH },
    { .data = sTaillow_Pal,     .tag = PALTAG_TAILLOW },
    { .data = sGridIcons_Pal,   .tag = PALTAG_GRID_ICONS },
    { .data = sWynaut_Pal,      .tag = PALTAG_WYNAUT },
    { .data = sAzurill_Pal,     .tag = PALTAG_AZURILL },
    { .data = sSkitty_Pal,      .tag = PALTAG_SKITTY },
    { .data = sMakuhita_Pal,    .tag = PALTAG_MAKUHITA },
    {}
};

static const struct OamData sOam_GridHeader =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

static const struct OamData sOam_GridIcon =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 1,
};

static const struct OamData sOam_WheelIcon =
{
    .y = 60,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x32),
    .size = SPRITE_SIZE(16x32),
    .priority = 2,
};

static const union AnimCmd sAffineAnim_Unused1[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAffineAnims_Unused1[] =
{
    sAffineAnim_Unused1
};

static const union AffineAnimCmd sAffineAnim_Unused2[] =
{
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_Unused2[] =
{
    sAffineAnim_Unused2
};

static const struct CompressedSpriteSheet sSpriteSheet_WheelIcons =
{
    .data = sWheelIcons_Gfx,
    .size = 0xC00,
    .tag = GFXTAG_WHEEL_ICONS
};

static const union AnimCmd sAnim_WheelIcons[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_FRAME(64, 0),
    ANIMCMD_FRAME(72, 0),
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_FRAME(40, 0),
    ANIMCMD_FRAME(48, 0),
    ANIMCMD_FRAME(80, 0),
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_FRAME(24, 0),
    ANIMCMD_FRAME(56, 0),
    ANIMCMD_FRAME(88, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnim_WheelIcon_OrangeWynaut[] =
{
    &sAnim_WheelIcons[0]
};

static const union AnimCmd *const sAnim_WheelIcon_GreenAzurill[] =
{
    &sAnim_WheelIcons[1]
};

static const union AnimCmd *const sAnim_WheelIcon_PurpleSkitty[] =
{
    &sAnim_WheelIcons[2]
};

static const union AnimCmd *const sAnim_WheelIcon_OrangeMakuhita[] =
{
    &sAnim_WheelIcons[3]
};

static const union AnimCmd *const sAnim_WheelIcon_GreenWynaut[] =
{
    &sAnim_WheelIcons[4]
};

static const union AnimCmd *const sAnim_WheelIcon_PurpleAzurill[] =
{
    &sAnim_WheelIcons[5]
};

static const union AnimCmd *const sAnim_WheelIcon_OrangeSkitty[] =
{
    &sAnim_WheelIcons[6]
};

static const union AnimCmd *const sAnim_WheelIcon_GreenMakuhita[] =
{
    &sAnim_WheelIcons[7]
};

static const union AnimCmd *const sAnim_WheelIcon_PurpleWynaut[] =
{
    &sAnim_WheelIcons[8]
};

static const union AnimCmd *const sAnim_WheelIcon_OrangeAzurill[] =
{
    &sAnim_WheelIcons[9]
};

static const union AnimCmd *const sAnim_WheelIcon_GreenSkitty[] =
{
    &sAnim_WheelIcons[10]
};

static const union AnimCmd *const sAnim_WheelIcon_PurpleMakuhita[] =
{
    &sAnim_WheelIcons[11]
};

static const struct CompressedSpriteSheet sSpriteSheet_Headers =
{
    .data = gRouletteHeaders_Gfx,
    .size = 0x1600,
    .tag = GFXTAG_HEADERS
};

static const struct CompressedSpriteSheet sSpriteSheet_GridIcons =
{
    .data = sGridIcons_Gfx,
    .size = 0x400,
    .tag = GFXTAG_GRID_ICONS
};

static const union AnimCmd sAnim_Headers[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_FRAME(48, 0),
    ANIMCMD_FRAME(64, 0),
    ANIMCMD_FRAME(80, 0),
    ANIMCMD_FRAME(96, 0),
    ANIMCMD_FRAME(112, 0),
    ANIMCMD_FRAME(128, 0),
    ANIMCMD_FRAME(144, 0),
    ANIMCMD_FRAME(160, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GridIcons[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_FRAME(12, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnim_WynautHeader[] =
{
    &sAnim_Headers[0]
};

static const union AnimCmd *const sAnim_AzurillHeader[] =
{
    &sAnim_Headers[2]
};

static const union AnimCmd *const sAnim_SkittyHeader[] =
{
    &sAnim_Headers[4]
};

static const union AnimCmd *const sAnim_MakuhitaHeader[] =
{
    &sAnim_Headers[6]
};

static const union AnimCmd *const sAnim_OrangeHeader[] =
{
    &sAnim_Headers[8]
};

static const union AnimCmd *const sAnim_GreenHeader[] =
{
    &sAnim_Headers[9]
};

static const union AnimCmd *const sAnim_PurpleHeader[] =
{
    &sAnim_Headers[10]
};

static const union AnimCmd *const sAnim_GridIcon_Wynaut[] =
{
    &sAnim_GridIcons[0]
};

static const union AnimCmd *const sAnim_GridIcon_Azurill[] =
{
    &sAnim_GridIcons[1]
};

static const union AnimCmd *const sAnim_GridIcon_Skitty[] =
{
    &sAnim_GridIcons[2]
};

static const union AnimCmd *const sAnim_GridIcon_Makuhita[] =
{
    &sAnim_GridIcons[3]
};

static const struct SpriteTemplate sSpriteTemplates_PokeHeaders[NUM_BOARD_POKES] =
{
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_WynautHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_AzurillHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_SkittyHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_MakuhitaHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    }
};

static const struct SpriteTemplate sSpriteTemplates_ColorHeaders[NUM_BOARD_COLORS] =
{
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_OrangeHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_GreenHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_HEADERS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridHeader,
        .anims = sAnim_PurpleHeader,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    }
};

static const struct SpriteTemplate sSpriteTemplates_GridIcons[NUM_BOARD_POKES] =
{
    {
        .tileTag = GFXTAG_GRID_ICONS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridIcon,
        .anims = sAnim_GridIcon_Wynaut,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_GRID_ICONS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridIcon,
        .anims = sAnim_GridIcon_Azurill,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_GRID_ICONS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridIcon,
        .anims = sAnim_GridIcon_Skitty,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    },
    {
        .tileTag = GFXTAG_GRID_ICONS,
        .paletteTag = PALTAG_GRID_ICONS,
        .oam = &sOam_GridIcon,
        .anims = sAnim_GridIcon_Makuhita,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GridSquare
    }
};

// Wheel icons are listed clockwise starting from 1 oclock on the roulette wheel (with Poké Ball upside right)
// They go Wynaut -> Azurill -> Skitty -> Makuhita, and Orange -> Green -> Purple
static const struct SpriteTemplate sSpriteTemplates_WheelIcons[NUM_ROULETTE_SLOTS] =
{
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_WYNAUT,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_OrangeWynaut,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_AZURILL,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_GreenAzurill,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_SKITTY,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_PurpleSkitty,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_MAKUHITA,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_OrangeMakuhita,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_WYNAUT,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_GreenWynaut,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_AZURILL,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_PurpleAzurill,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_SKITTY,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_OrangeSkitty,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_MAKUHITA,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_GreenMakuhita,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_WYNAUT,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_PurpleWynaut,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_AZURILL,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_OrangeAzurill,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_SKITTY,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_GreenSkitty,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    },
    {
        .tileTag = GFXTAG_WHEEL_ICONS,
        .paletteTag = PALTAG_MAKUHITA,
        .oam = &sOam_WheelIcon,
        .anims = sAnim_WheelIcon_PurpleMakuhita,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_WheelIcon
    }
};

static const struct OamData sOam_Credit =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 1,
};

static const struct OamData sOam_CreditDigit =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(8x16),
    .size = SPRITE_SIZE(8x16),
    .priority = 1,
};

static const struct OamData sOam_Multiplier =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 1,
};

static const struct OamData sOam_BallCounter =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x8),
    .size = SPRITE_SIZE(16x8),
    .priority = 1,
};

static const struct CompressedSpriteSheet sSpriteSheets_Interface[] =
{
    {
        .data = gRouletteCredit_Gfx,
        .size = 0x400,
        .tag = GFXTAG_CREDIT
    },
    {
        .data = gRouletteNumbers_Gfx,
        .size = 0x280,
        .tag = GFXTAG_CREDIT_DIGIT
    },
    {
        .data = gRouletteMultiplier_Gfx,
        .size = 0x500,
        .tag = GFXTAG_MULTIPLIER
    },
    {
        .data = sBallCounter_Gfx,
        .size = 0x140,
        .tag = GFXTAG_BALL_COUNTER
    },
    {
        .data = sCursor_Gfx,
        .size = 0x200,
        .tag = GFXTAG_CURSOR
    },
    {}
};

static const union AnimCmd sAnim_CreditDigit[] =
{
    ANIMCMD_FRAME(0, 0),  // 0
    ANIMCMD_FRAME(2, 0),  // 1
    ANIMCMD_FRAME(4, 0),  // 2
    ANIMCMD_FRAME(6, 0),  // 3
    ANIMCMD_FRAME(8, 0),  // 4
    ANIMCMD_FRAME(10, 0), // 5
    ANIMCMD_FRAME(12, 0), // 6
    ANIMCMD_FRAME(14, 0), // 7
    ANIMCMD_FRAME(16, 0), // 8
    ANIMCMD_FRAME(18, 0), // 9
    // BUG: Animation not terminated properly
    // Doesn't matter in practice, the frames are set directly and not looped
#ifdef BUGFIX
    ANIMCMD_END
#endif
};

static const union AnimCmd *const sAnims_CreditDigit[] =
{
    sAnim_CreditDigit
};

static const union AnimCmd sAnim_Multiplier[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_FRAME(24, 0),
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Multiplier[] =
{
    sAnim_Multiplier
};

static const union AnimCmd sAnim_BallCounter[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_FRAME(2, 0),
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_FRAME(6, 0),
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_BallCounter[] =
{
    sAnim_BallCounter
};

static const struct SpriteTemplate sSpriteTemplate_Credit =
{
    .tileTag = GFXTAG_CREDIT,
    .paletteTag = PALTAG_INTERFACE,
    .oam = &sOam_Credit,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_CreditDigit =
{
    .tileTag = GFXTAG_CREDIT_DIGIT,
    .paletteTag = PALTAG_INTERFACE,
    .oam = &sOam_CreditDigit,
    .anims = sAnims_CreditDigit,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_Multiplier =
{
    .tileTag = GFXTAG_MULTIPLIER,
    .paletteTag = PALTAG_INTERFACE,
    .oam = &sOam_Multiplier,
    .anims = sAnims_Multiplier,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_GridSquare
};

static const struct SpriteTemplate sSpriteTemplate_BallCounter =
{
    .tileTag = GFXTAG_BALL_COUNTER,
    .paletteTag = PALTAG_BALL_COUNTER,
    .oam = &sOam_BallCounter,
    .anims = sAnims_BallCounter,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

// NOTE: This cursor is only used to identify the winning square on the grid
static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = GFXTAG_CURSOR,
    .paletteTag = PALTAG_INTERFACE,
    .oam = &sOam_GridHeader,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOam_Ball =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_Ball = {
    .data = sBall_Gfx,
    .size = 0x200,
    .tag = GFXTAG_BALL
};

static const union AnimCmd sAnim_Ball_RollFast[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Ball_RollMedium[] =
{
    ANIMCMD_FRAME(0, 10),
    ANIMCMD_FRAME(4, 10),
    ANIMCMD_FRAME(8, 10),
    ANIMCMD_FRAME(4, 10),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Ball_RollSlow[] =
{
    ANIMCMD_FRAME(0, 15),
    ANIMCMD_FRAME(4, 15),
    ANIMCMD_FRAME(8, 15),
    ANIMCMD_FRAME(4, 15),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Ball_StopOnFrame1[] =
{
    ANIMCMD_FRAME(4, 2),
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_FRAME(12, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Ball_StopOnFrame3[] =
{
    ANIMCMD_FRAME(4, 2),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(4, 4),
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_FRAME(12, 4),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Ball_StopOnFrame4[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_FRAME(12, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Ball_Still[] =
{
    ANIMCMD_FRAME(12, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Ball_StopOnFrame2[] =
{
    ANIMCMD_FRAME(8, 2),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_FRAME(12, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Ball[] =
{
    sAnim_Ball_RollFast,
    sAnim_Ball_RollMedium,
    sAnim_Ball_RollSlow,
    sAnim_Ball_StopOnFrame1,
    sAnim_Ball_StopOnFrame2,
    sAnim_Ball_StopOnFrame3,
    sAnim_Ball_StopOnFrame4,
    sAnim_Ball_StopOnFrame4,
    sAnim_Ball_Still
};

static const struct SpriteTemplate sSpriteTemplate_Ball =
{
    .tileTag = GFXTAG_BALL,
    .paletteTag = PALTAG_BALL,
    .oam = &sOam_Ball,
    .anims = sAnims_Ball,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOam_WheelCenter =
{
    .y = 81,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_WheelCenter =
{
    .data = gRouletteCenter_Gfx,
    .size = 0x800,
    .tag = GFXTAG_WHEEL_CENTER
};

static const struct SpriteTemplate sSpriteTemplate_WheelCenter =
{
    .tileTag = GFXTAG_WHEEL_CENTER,
    .paletteTag = PALTAG_BALL,
    .oam = &sOam_WheelCenter,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_WheelCenter
};

static const struct OamData sOam_Shroomish =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 2,
};

static const struct OamData sOam_Taillow =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_ShroomishTaillow =
{
    .data = sShroomishTaillow_Gfx,
    .size = 0xE00,
    .tag = GFXTAG_SHROOMISH_TAILLOW
};

static const union AnimCmd sAnim_Shroomish[] =
{
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_FRAME(16, 6),
    ANIMCMD_FRAME(32, 6),
    ANIMCMD_FRAME(48, 6),
    ANIMCMD_FRAME(32, 6),
    ANIMCMD_FRAME(64, 6),
    ANIMCMD_JUMP(2)
};

static const union AnimCmd sAnim_Taillow_WingDown_Left[] =
{
    ANIMCMD_FRAME(80, 10),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Taillow_WingDown_Right[] =
{
    ANIMCMD_FRAME(80, 10, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Taillow_FlapSlow_Left[] =
{
    ANIMCMD_FRAME(80, 20),
    ANIMCMD_FRAME(96, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Taillow_FlapSlow_Right[] =
{
    ANIMCMD_FRAME(80, 20, .hFlip = TRUE),
    ANIMCMD_FRAME(96, 20, .hFlip = TRUE),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Taillow_FlapFast_Left[] =
{
    ANIMCMD_FRAME(80, 10),
    ANIMCMD_FRAME(96, 10),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Taillow_FlapFast_Right[] =
{
    ANIMCMD_FRAME(80, 10, .hFlip = TRUE),
    ANIMCMD_FRAME(96, 10, .hFlip = TRUE),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Shroomish[] =
{
    sAnim_Shroomish
};

static const union AnimCmd *const sAnims_Taillow[] =
{
    sAnim_Taillow_WingDown_Left,   // While gliding in
    sAnim_Taillow_WingDown_Right,
    sAnim_Taillow_FlapSlow_Left,   // While carrying ball
    sAnim_Taillow_FlapSlow_Right,
    sAnim_Taillow_FlapFast_Left,   // While flying off
    sAnim_Taillow_FlapFast_Right
};

static const struct SpriteTemplate sSpriteTemplate_Shroomish =
{
    .tileTag = GFXTAG_SHROOMISH_TAILLOW,
    .paletteTag = PALTAG_SHROOMISH,
    .oam = &sOam_Shroomish,
    .anims = sAnims_Shroomish,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct SpriteTemplate sSpriteTemplate_Taillow =
{
    .tileTag = GFXTAG_SHROOMISH_TAILLOW,
    .paletteTag = PALTAG_TAILLOW,
    .oam = &sOam_Taillow,
    .anims = sAnims_Taillow,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Taillow
};

static const struct OamData sOam_ShroomishBallShadow =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2,
};

static const struct OamData sOam_ShroomishShadow =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 2,
};

static const struct OamData sOam_TaillowShadow =
{
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_Shadow =
{
    .data = sShadow_Gfx,
    .size = 0x180,
    .tag = GFXTAG_SHADOW
};

static const union AffineAnimCmd sAffineAnim_Unused3[] =
{
    AFFINEANIMCMD_FRAME(0x80, 0x80, 0, 0),
    AFFINEANIMCMD_FRAME(2,    2,    0, 60),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_TaillowShadow[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_FRAME(-2,    0x0,   0, 15),
    AFFINEANIMCMD_FRAME(-1,    -2,    0, 15),
    AFFINEANIMCMD_FRAME(-1,    -5,    0, 24),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_Unused3[] =
{
    sAffineAnim_Unused3
};

static const union AffineAnimCmd *const sAffineAnims_TaillowShadow[] =
{
    sAffineAnim_TaillowShadow
};

static const union AffineAnimCmd sAffineAnim_Unused4[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_Unused4[] =
{
    sAffineAnim_Unused4
};

static const union AnimCmd sAnim_ShroomishBallShadow[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_UnstickMonShadow[] =
{
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_ShroomishBallShadow[] =
{
    sAnim_ShroomishBallShadow
};

static const union AnimCmd *const sAnims_UnstickMonShadow[] =
{
    sAnim_UnstickMonShadow
};

static const struct SpriteTemplate sSpriteTemplate_ShroomishShadow[] =
{
    // Ball's shadow as it flies up
    {
        .tileTag = GFXTAG_SHADOW,
        .paletteTag = PALTAG_SHADOW,
        .oam = &sOam_ShroomishBallShadow,
        .anims = sAnims_ShroomishBallShadow,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy
    },
    // Shroomish's Shadow
    {
        .tileTag = GFXTAG_SHADOW,
        .paletteTag = PALTAG_SHADOW,
        .oam = &sOam_ShroomishShadow,
        .anims = sAnims_UnstickMonShadow,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_Shroomish
    }
};

static const struct SpriteTemplate sSpriteTemplate_TaillowShadow =
{
    .tileTag = GFXTAG_SHADOW,
    .paletteTag = PALTAG_SHADOW,
    .oam = &sOam_TaillowShadow,
    .anims = sAnims_UnstickMonShadow,
    .images = NULL,
    .affineAnims = sAffineAnims_TaillowShadow,
    .callback = SpriteCB_Taillow
};

static void Task_ShowMinBetYesNo(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    DoYesNoFuncWithChoice(taskId, &sYesNoTable_AcceptMinBet);
}

static void Task_FadeToRouletteGame(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetVBlankCallback(NULL);
        SetMainCallback2(CB2_LoadRoulette);
        DestroyTask(taskId);
    }
}

static void Task_AcceptMinBet(u8 taskId)
{
    ClearStdWindowAndFrame(0, TRUE);
    HideCoinsWindow();
    FreeAllWindowBuffers();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gPaletteFade.delayCounter = gPaletteFade.multipurpose2;
    UpdatePaletteFade();
    gTasks[taskId].func = Task_FadeToRouletteGame;
}

static void Task_DeclineMinBet(u8 taskId)
{
    ClearStdWindowAndFrame(0, FALSE);
    HideCoinsWindow();
    UnlockPlayerFieldControls();
    DestroyTask(taskId);
}

static void Task_NotEnoughForMinBet(u8 taskId)
{
    gTasks[taskId].data[0]++;
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        gSpecialVar_0x8004 = 1;
        HideCoinsWindow();
        ClearStdWindowAndFrame(0, TRUE);
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
    }
}

static void Task_PrintMinBet(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        u32 minBet = sTableMinBets[GET_MIN_BET_ID(gSpecialVar_0x8004)];
        ConvertIntToDecimalStringN(gStringVar1, minBet, STR_CONV_MODE_LEADING_ZEROS, 1);
        StringExpandPlaceholders(gStringVarBuffer, Roulette_Text_PlayMinimumWagerIsX);
        DrawStdWindowFrame(0, FALSE);
        AddTextPrinterParameterized(0, FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(0, COPYWIN_FULL);
        gTasks[taskId].func = Task_ShowMinBetYesNo;
    }
}

static void Task_PrintRouletteEntryMsg(u8 taskId)
{
    s32 minBet;
    PrintCoinsString(gTasks[taskId].tCoins);
    minBet = sTableMinBets[GET_MIN_BET_ID(gSpecialVar_0x8004)];
    ConvertIntToDecimalStringN(gStringVar1, minBet, STR_CONV_MODE_LEADING_ZEROS, 1);

    if (gTasks[taskId].tCoins >= minBet)
    {
        if ((gSpecialVar_0x8004 & ROULETTE_SPECIAL_RATE) && (gSpecialVar_0x8004 & 1))
        {
            // Special rate for Game Corner service day (only at second table)
            DrawStdWindowFrame(0, FALSE);
            AddTextPrinterParameterized(0, FONT_NORMAL, Roulette_Text_SpecialRateTable, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(0, COPYWIN_FULL);
            gTasks[taskId].func = Task_PrintMinBet;
        }
        else
        {
            // Print minimum bet
            StringExpandPlaceholders(gStringVarBuffer, Roulette_Text_PlayMinimumWagerIsX);
            DrawStdWindowFrame(0, FALSE);
            AddTextPrinterParameterized(0, FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(0, COPYWIN_FULL);
            gTasks[taskId].func = Task_ShowMinBetYesNo;
        }
    }
    else
    {
        // Not enough for minimum bet
        StringExpandPlaceholders(gStringVarBuffer, Roulette_Text_NotEnoughCoins);
        DrawStdWindowFrame(0, FALSE);
        AddTextPrinterParameterized(0, FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(0, COPYWIN_FULL);
        gTasks[taskId].func = Task_NotEnoughForMinBet;
        gTasks[taskId].tCoins = 0;
        gTasks[taskId].data[0] = 0;
    }
}

void PlayRoulette(void)
{
    u8 taskId;
    LockPlayerFieldControls();
    ShowCoinsWindow(GetCoins(), 1, 1);
    taskId = CreateTask(Task_PrintRouletteEntryMsg, 0);
    gTasks[taskId].tCoins = GetCoins();
}

static void LoadOrFreeMiscSpritePalettesAndSheets(bool8 free)
{
    if (!free)
    {
        FreeAllSpritePalettes();
        LoadSpritePalettes(sSpritePalettes);
        LoadCompressedSpriteSheet(&sSpriteSheet_Ball);
        LoadCompressedSpriteSheet(&sSpriteSheet_ShroomishTaillow);
        LoadCompressedSpriteSheet(&sSpriteSheet_Shadow);
    }
    else
    {
        // Unused
        FreeSpriteTilesByTag(GFXTAG_SHADOW);
        FreeSpriteTilesByTag(GFXTAG_SHROOMISH_TAILLOW);
        FreeSpriteTilesByTag(GFXTAG_BALL);
        FreeAllSpritePalettes();
    }
}

static u8 CreateWheelIconSprite(const struct SpriteTemplate *template, u8 r1, u16 *angle)
{
    u16 temp;
    u8 spriteId = CreateSprite(template, 116, 80, template->oam->y);
    gSprites[spriteId].data[0] = *angle;
    gSprites[spriteId].data[1] = r1;
    gSprites[spriteId].coordOffsetEnabled = TRUE;
    gSprites[spriteId].animPaused = TRUE;
    gSprites[spriteId].affineAnimPaused = TRUE;
    temp = *angle;
    *angle += DEGREES_PER_SLOT;
    if (*angle >= 360)
        *angle = temp - (360 - DEGREES_PER_SLOT);
    return spriteId;
}

static void CreateGridSprites(void)
{
    u8 i, j;
    u8 spriteId;
    struct SpriteSheet s;
    LZ77UnCompWram(sSpriteSheet_Headers.data, gDecompressionBuffer);
    s.data = gDecompressionBuffer;
    s.size = sSpriteSheet_Headers.size;
    s.tag  = sSpriteSheet_Headers.tag;
    LoadSpriteSheet(&s);
    LZ77UnCompWram(sSpriteSheet_GridIcons.data, gDecompressionBuffer);
    s.data = gDecompressionBuffer;
    s.size = sSpriteSheet_GridIcons.size;
    s.tag  = sSpriteSheet_GridIcons.tag;
    LoadSpriteSheet(&s);
    for (i = 0; i < NUM_BOARD_COLORS; i++)
    {
        u8 y = i * 24;
        for (j = 0; j < NUM_BOARD_POKES; j++)
        {
            spriteId = sRoulette->spriteIds[(i * NUM_BOARD_POKES) + SPR_GRID_ICONS + j] = CreateSprite(&sSpriteTemplates_GridIcons[j], (j * 24) + 148, y + 92, 30);
            gSprites[spriteId].animPaused = TRUE;
            y += 24;
            if (y >= 72)
                y = 0;
        }
    }
    for (i = 0; i < ARRAY_COUNT(sSpriteTemplates_PokeHeaders); i++)
    {
        spriteId = sRoulette->spriteIds[i + SPR_POKE_HEADERS] = CreateSprite(&sSpriteTemplates_PokeHeaders[i], (i * 24) + 148, 70, 30);
        gSprites[spriteId].animPaused = TRUE;
    }
    for (i = 0; i < ARRAY_COUNT(sSpriteTemplates_ColorHeaders); i++)
    {
        spriteId = sRoulette->spriteIds[i + SPR_COLOR_HEADERS] = CreateSprite(&sSpriteTemplates_ColorHeaders[i], 126, (i * 24) + 92, 30);
        gSprites[spriteId].animPaused = TRUE;
    }
}

static void UNUSED DestroyGridSprites(void)
{
    u8 i;
    for (i = 0; i < NUM_ROULETTE_SLOTS; i++)
    {
        DestroySprite(&gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]]);
    }
}

static void ShowHideGridIcons(bool8 hideAll, u8 hideSquare)
{
    u8 i;
    switch (hideAll)
    {
    case TRUE:
        // Hide grid icons and headers
        for (i = 0; i < NUM_GRID_SELECTIONS; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]].invisible = TRUE;
        }
        break;
    case FALSE:
        for (i = 0; i < NUM_ROULETTE_SLOTS; i++)
        {
            if (!(sRoulette->hitFlags & sRouletteSlots[i].flag))
                gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]].invisible = FALSE;
            else if (sRouletteSlots[i].gridSquare != hideSquare)
                gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]].invisible = TRUE;
            else
                gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]].invisible = FALSE;
        }
        // Always show grid headers
        for (; i < NUM_GRID_SELECTIONS; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_GRID_ICONS]].invisible = FALSE;
        }
        break;
    }
}

static void CreateGridBallSprites(void)
{
    u8 i;
    for (i = 0; i < BALLS_PER_ROUND; i++)
    {
        sRoulette->spriteIds[i + SPR_GRID_BALLS] = CreateSprite(&sSpriteTemplate_Ball, 116, 20, 10);
        gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].invisible = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].data[0] = 1;
        gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].callback = SpriteCB_GridSquare;
        gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].oam.priority = 1;
        StartSpriteAnim(&gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]], 8);
    }
}

static void ShowHideGridBalls(bool8 hideAll, u8 hideBallId)
{
    u8 i = 0;
    if (hideAll)
    {
        for (; i < BALLS_PER_ROUND; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].invisible = TRUE;
        }
    }
    else
    {
        for (; i < BALLS_PER_ROUND; i++)
        {
            if (!sRoulette->hitSquares[i] || i == hideBallId)
            {
                gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].invisible = TRUE;
            }
            else
            {
                gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].invisible = FALSE;
                gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].x = (sGridSelections[sRoulette->hitSquares[i]].x + 1) * 8 + 4;
                gSprites[sRoulette->spriteIds[i + SPR_GRID_BALLS]].y = (sGridSelections[sRoulette->hitSquares[i]].y + 1) * 8 + 3;
            }
        }
    }
}

static void ShowHideWinSlotCursor(u8 selectionId)
{
    if (selectionId == 0)
    {
        gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].invisible = TRUE;
    }
    else
    {
        gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].invisible = FALSE;
        gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].x = (sGridSelections[selectionId].x + 2) * 8;
        gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].y = (sGridSelections[selectionId].y + 2) * 8;
    }
}

static void CreateWheelIconSprites(void)
{
    u8 i, j;
    u16 angle;
    struct SpriteSheet s;

    LZ77UnCompWram(sSpriteSheet_WheelIcons.data, gDecompressionBuffer);
    s.data = gDecompressionBuffer;
    s.size = sSpriteSheet_WheelIcons.size;
    s.tag  = sSpriteSheet_WheelIcons.tag;
    LoadSpriteSheet(&s);

    angle = 15;
    for (i = 0; i < NUM_BOARD_COLORS; i++)
    {
        for (j = 0; j < NUM_BOARD_POKES; j++)
        {
            u8 spriteId;
            spriteId = sRoulette->spriteIds[(i * NUM_BOARD_POKES) + SPR_WHEEL_ICONS + j] = CreateWheelIconSprite(&sSpriteTemplates_WheelIcons[i * NUM_BOARD_POKES + j], 40, &angle);
            gSprites[spriteId].animPaused = TRUE;
            gSprites[spriteId].affineAnimPaused = TRUE;
        }
    }
}

static void SpriteCB_WheelIcon(struct Sprite *sprite)
{
    s16 cos;
    s16 sin;
    u32 matrixNum;
    s16 angle = sRoulette->wheelAngle + sprite->data[0];
    if (angle >= 360)
        angle -= 360;
    sin = Sin2(angle);
    cos = Cos2(angle);
    sprite->x2 =  sin * sprite->data[1] >> 12;
    sprite->y2 = -cos * sprite->data[1] >> 12;
    matrixNum = sprite->oam.matrixNum;
    sin /= 16;
    gOamMatrices[matrixNum].d = cos /= 16;
    gOamMatrices[matrixNum].a = cos;
    gOamMatrices[matrixNum].b = sin;
    gOamMatrices[matrixNum].c = -sin;
}

static void CreateInterfaceSprites(void)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sSpriteSheets_Interface) - 1; i++)
    {
        struct SpriteSheet s;
        LZ77UnCompWram(sSpriteSheets_Interface[i].data, gDecompressionBuffer);
        s.data = gDecompressionBuffer;
        s.size = sSpriteSheets_Interface[i].size;
        s.tag  = sSpriteSheets_Interface[i].tag;
        LoadSpriteSheet(&s);
    }
    sRoulette->spriteIds[SPR_CREDIT] = CreateSprite(&sSpriteTemplate_Credit, 208, 16, 4);
    gSprites[sRoulette->spriteIds[SPR_CREDIT]].animPaused = TRUE;
    for (i = 0; i < MAX_COIN_DIGITS; i++)
    {
        sRoulette->spriteIds[i + SPR_CREDIT_DIGITS] = CreateSprite(&sSpriteTemplate_CreditDigit, i * 8 + 196, 24, 0);
        gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].invisible = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].animPaused = TRUE;
    }
    sRoulette->spriteIds[SPR_MULTIPLIER] = CreateSprite(&sSpriteTemplate_Multiplier, 120, 68, 4);
    gSprites[sRoulette->spriteIds[SPR_MULTIPLIER]].animPaused = TRUE;
    for (i = 0; i < BALLS_PER_ROUND / 2; i++)
    {
        // Each ball counter sprite has 2 balls
        sRoulette->spriteIds[i + SPR_BALL_COUNTER] = CreateSprite(&sSpriteTemplate_BallCounter, i * 16 + 192, 36, 4);
        gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].invisible = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].animPaused = TRUE;
    }
    sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR] = CreateSprite(&sSpriteTemplate_Cursor, 152, 96, 9);
    gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].oam.priority = 1;
    gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].animPaused = TRUE;
    gSprites[sRoulette->spriteIds[SPR_WIN_SLOT_CURSOR]].invisible = TRUE;
}

static void SetCreditDigits(u16 num)
{
    u8 i;
    u16 d = 1000;
    bool8 printZero = FALSE;
    for (i = 0; i < MAX_COIN_DIGITS; i++)
    {
        u8 digit = num / d;
        gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].invisible = TRUE;
        if (digit > 0 || printZero || i == MAX_COIN_DIGITS - 1)
        {
            gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].invisible = FALSE;
            gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].oam.tileNum =
                gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].sheetTileStart
                + (*gSprites[sRoulette->spriteIds[i + SPR_CREDIT_DIGITS]].anims + digit)->type;
            printZero = TRUE;
        }
        num = num % d;
        d = d / 10;
    }
}

// Identical to GetMultiplier but with different data array
static u8 GetMultiplierAnimId(u8 selectionId)
{
    u8 animIds[5] = {0, 1, 2, 3, 4};

    if (selectionId > NUM_GRID_SELECTIONS)
        selectionId = 0;

    switch (sGridSelections[selectionId].baseMultiplier)
    {
    case NUM_BOARD_COLORS:
        selectionId = GET_ROW_IDX(selectionId);
        if (sRoulette->colorHits[selectionId] > 3)
            return 0;
        return animIds[sRoulette->colorHits[selectionId] + 1];
    case NUM_BOARD_POKES:
        selectionId = GET_COL_IDX(selectionId);
        if (sRoulette->pokeHits[selectionId] > 2)
            return 0;
        return animIds[sRoulette->pokeHits[selectionId] + 2];
    case NUM_ROULETTE_SLOTS:
        if (sRoulette->hitFlags & sGridSelections[selectionId].flag)
            return 0;
        return animIds[4];
    }
    return 0;
}

static void SetMultiplierSprite(u8 selectionId)
{
    struct Sprite *sprite = &gSprites[sRoulette->spriteIds[SPR_MULTIPLIER]];
    sprite->animCmdIndex = GetMultiplierAnimId(selectionId);
    sprite->oam.tileNum = sprite->sheetTileStart + (*sprite->anims + sprite->animCmdIndex)->type;
}

static void SetBallCounterNumLeft(u8 numBalls)
{
    u8 i;
    u8 t = 0;
    if (sRoulette->minBet == 1)
        t = 2;
    switch (numBalls)
    {
    case 6:
        for (i = 0; i < BALLS_PER_ROUND / 2; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].invisible = FALSE;
            gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].oam.tileNum =
                gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].sheetTileStart
                + (*gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].anims)->type;
        }
        break;
    case 5:
        gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].oam.tileNum =
            gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].anims + t + 1)->type;
        break;
    case 4:
        gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].oam.tileNum =
            gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_3]].anims + t + 2)->type;
        break;
    case 3:
        gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].oam.tileNum =
            gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].anims + t + 1)->type;
        break;
    case 2:
        gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].oam.tileNum =
            gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_2]].anims + t + 2)->type;
        break;
    case 1:
        gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_1]].oam.tileNum =
            gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_1]].sheetTileStart
            + (*gSprites[sRoulette->spriteIds[SPR_BALL_COUNTER_1]].anims + t + 1)->type;
        break;
    case 0:
    default:
        for (i = 0; i < BALLS_PER_ROUND / 2; i++)
        {
            gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].oam.tileNum =
                gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].sheetTileStart
                + (*gSprites[sRoulette->spriteIds[i + SPR_BALL_COUNTER]].anims + t + 2)->type;
        }
    }
}

static void SpriteCB_GridSquare(struct Sprite *sprite)
{
    sprite->x2 = sRoulette->gridX;
}

static void CreateWheelCenterSprite(void)
{
    u8 spriteId;
    struct SpriteSheet s;
    LZ77UnCompWram(sSpriteSheet_WheelCenter.data, gDecompressionBuffer);
    s.data = gDecompressionBuffer;
    s.size = sSpriteSheet_WheelCenter.size;
    s.tag = sSpriteSheet_WheelCenter.tag;
    LoadSpriteSheet(&s);
    // This sprite id isn't saved because it doesn't need to be referenced again
    // but by virtue of creation order it's SPR_WHEEL_CENTER
    spriteId = CreateSprite(&sSpriteTemplate_WheelCenter, 116, 80, 81);
    gSprites[spriteId].data[0] = sRoulette->wheelAngle;
    gSprites[spriteId].data[1] = 0;
    gSprites[spriteId].animPaused = TRUE;
    gSprites[spriteId].affineAnimPaused = TRUE;
    gSprites[spriteId].coordOffsetEnabled = TRUE;
}

static void SpriteCB_WheelCenter(struct Sprite *sprite)
{
    u32 matrixNum = sprite->oam.matrixNum;
    struct OamMatrix *matrix = &gOamMatrices[0];
    matrix[matrixNum].d = sRoulette->wheelRotation.a;
    matrix[matrixNum].a = sRoulette->wheelRotation.a;
    matrix[matrixNum].b = sRoulette->wheelRotation.b;
    matrix[matrixNum].c = sRoulette->wheelRotation.c;
}

static void CreateWheelBallSprites(void)
{
    u8 i;
    for (i = 0; i < BALLS_PER_ROUND; i++)
    {
        sRoulette->spriteIds[i] = CreateSprite(&sSpriteTemplate_Ball, 116, 80, 57 - i);
        if (sRoulette->spriteIds[i] != MAX_SPRITES)
        {
            gSprites[sRoulette->spriteIds[i]].invisible = TRUE;
            gSprites[sRoulette->spriteIds[i]].coordOffsetEnabled = TRUE;
        }
    }
}

static void HideWheelBalls(void)
{
    u8 spriteId = sRoulette->spriteIds[SPR_WHEEL_BALLS];
    u8 i;
    for (i = 0; i < BALLS_PER_ROUND; i++)
    {
        u8 j;
        gSprites[spriteId].invisible = TRUE;
        gSprites[spriteId].callback = &SpriteCallbackDummy;
        StartSpriteAnim(&gSprites[spriteId], 0);
        for (j = 0; j < 8; j++)
            gSprites[spriteId].data[j] = 0;

        spriteId++;
    }
}

// Sprite data for the roulette ball
#define sStuckOnWheelLeft data[0] // if true, ball got stuck in left half of wheel, else got stuck in right half
#define sState            data[1]
#define sSlotMidpointDist data[2]
#define sBallAngle        data[3]
#define sBallDistToCenter data[4]
#define sBallWheelAngle   data[6]

#define LandBall()                                                                                  \
{                                                                                                   \
    sRoulette->ballState = BALL_STATE_LANDED;                                                       \
    sRoulette->ballRolling = FALSE;                                                                 \
    StartSpriteAnim(sprite, sprite->animCmdIndex + 3);                                              \
    UpdateSlotBelowBall(sprite);                                                                    \
    sprite->sBallDistToCenter = 30;                                                                 \
    UpdateBallRelativeWheelAngle(sprite);                                                           \
    sprite->sBallWheelAngle = (sprite->sBallWheelAngle / DEGREES_PER_SLOT) * DEGREES_PER_SLOT + 15; \
    sprite->callback = SpriteCB_BallLandInSlot;                                                     \
    m4aSongNumStartOrChange(SE_BRIDGE_WALK);                                                              \
}

// "wheelAngle" and "sBallAngle" are relative to the screen (e.g. 180 degrees for either is always screen bottom)
// "sBallWheelAngle" is the ball's angle relative to the wheel
//   e.g. if the ball is screen right (90), but wheel is upside down (180), sBallWheelAngle is 270 (because the ball is wheel left)
static s16 UpdateBallRelativeWheelAngle(struct Sprite *sprite)
{
    if (sRoulette->wheelAngle > sprite->sBallAngle)
    {
        sprite->sBallWheelAngle = 360 - sRoulette->wheelAngle + sprite->sBallAngle;
        if (sprite->sBallWheelAngle >= 360)
            sprite->sBallWheelAngle -= 360;
    }
    else
    {
        sprite->sBallWheelAngle = sprite->sBallAngle - sRoulette->wheelAngle;
    }

    return sprite->sBallWheelAngle;
}

static u8 UpdateSlotBelowBall(struct Sprite *sprite)
{
    sRoulette->hitSlot = UpdateBallRelativeWheelAngle(sprite) / (f32)DEGREES_PER_SLOT;
    return sRoulette->hitSlot;
}

static s16 GetBallDistanceToSlotMidpoint(struct Sprite *sprite)
{
    s16 angleIntoSlot = UpdateBallRelativeWheelAngle(sprite) % DEGREES_PER_SLOT;
    u16 distanceToMidpoint;
    if (angleIntoSlot == SLOT_MIDPOINT)
    {
        // Ball is at midpoint, ok to drop into slot
        distanceToMidpoint = 0;
        return sprite->sSlotMidpointDist = distanceToMidpoint;
    }
    else if (angleIntoSlot >= SLOT_MIDPOINT)
    {
        // Ball has passed midpoint, travel to midpoint of next slot
        distanceToMidpoint = (DEGREES_PER_SLOT - 1) + SLOT_MIDPOINT - angleIntoSlot;
        return sprite->sSlotMidpointDist = distanceToMidpoint;
    }
    else
    {
        // Ball hasn't reached midpoint of this slot yet
        distanceToMidpoint = SLOT_MIDPOINT - angleIntoSlot;
        return sprite->sSlotMidpointDist = distanceToMidpoint;
    }
}

static void UpdateBallPos(struct Sprite *sprite)
{
    s16 sin, cos;
    sRoulette->ballAngleSpeed += sRoulette->ballAngleAccel;
    sRoulette->ballAngle += sRoulette->ballAngleSpeed;

    if (sRoulette->ballAngle >= 360)
        sRoulette->ballAngle -= 360.0f;
    else if (sRoulette->ballAngle < 0.0f)
        sRoulette->ballAngle += 360.0f;

    sprite->sBallAngle = sRoulette->ballAngle;
    sRoulette->ballFallSpeed += sRoulette->ballFallAccel;
    sRoulette->ballDistToCenter += sRoulette->ballFallSpeed;
    sprite->sBallDistToCenter = sRoulette->ballDistToCenter;
    sin = Sin2(sprite->sBallAngle);
    cos = Cos2(sprite->sBallAngle);
    sprite->x2 =  sin * sprite->sBallDistToCenter >> 12;
    sprite->y2 = -cos * sprite->sBallDistToCenter >> 12;
    if (IsSEPlaying())
    {
        m4aMPlayPanpotControl(&gMPlayInfo_SE1, TRACKS_ALL, sprite->x2);
        m4aMPlayPanpotControl(&gMPlayInfo_SE2, TRACKS_ALL, sprite->x2);
    }
}

// Snap to the bottom of the slot and continue to spin with the wheel
static void SpriteCB_BallLandInSlot(struct Sprite *sprite)
{
    s16 sin, cos;
    sprite->sBallAngle = sRoulette->wheelAngle + sprite->sBallWheelAngle;
    if (sprite->sBallAngle >= 360)
        sprite->sBallAngle -= 360;
    sin = Sin2(sprite->sBallAngle);
    cos = Cos2(sprite->sBallAngle);
    sprite->x2 =  sin * sprite->sBallDistToCenter >> 12;
    sprite->y2 = -cos * sprite->sBallDistToCenter >> 12;
    sprite->y2 += gSpriteCoordOffsetY;
}

static void SpriteCB_UnstickBall_ShroomishBallFall(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    sprite->data[2]++;
    if (sprite->sBallDistToCenter < -132 || sprite->sBallDistToCenter > 80)
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;

    if (sprite->data[2] >= DEGREES_PER_SLOT)
    {
        if (!sprite->sStuckOnWheelLeft)
        {
            if (sRoulette->ballDistToCenter <= sRoulette->varA0 - 2.0f)
            {
                LandBall()
                sRoulette->ballFallAccel = sRoulette->ballFallSpeed = 0.0f;
                sRoulette->ballAngleSpeed = -1.0f;
            }
        }
        else
        {
            if (sRoulette->ballDistToCenter >= sRoulette->varA0 - 2.0f)
            {
                LandBall()
                sRoulette->ballFallAccel = sRoulette->ballFallSpeed = 0.0f;
                sRoulette->ballAngleSpeed = -1.0f;
            }
        }
    }
}

static void SpriteCB_UnstickBall_Shroomish(struct Sprite *sprite)
{
    f32 slotOffset, ballFallDist, ballFallSpeed;
    UpdateBallPos(sprite);

    switch (sprite->sBallAngle)
    {
    case 0:
        if (sprite->sStuckOnWheelLeft != TRUE)
        {
            slotOffset = sprite->data[7];
            ballFallDist = (slotOffset * sRouletteTables[sRoulette->tableId].randDistanceHigh + (sRouletteTables[sRoulette->tableId].randDistanceLow - 1));
            ballFallSpeed = (slotOffset / sRouletteTables[sRoulette->tableId].shroomish.fallSlowdown);
        }
        else
        {
            return;
        }
        break;
    case 180:
        if (sprite->sStuckOnWheelLeft)
        {
            slotOffset = sprite->data[7];
            ballFallDist = (slotOffset * sRouletteTables[sRoulette->tableId].randDistanceHigh + (sRouletteTables[sRoulette->tableId].randDistanceLow - 1));
            ballFallSpeed = -(slotOffset / sRouletteTables[sRoulette->tableId].shroomish.fallSlowdown);
        }
        else
        {
            return;
        }
        break;
    default:
        return;
    }
    sRoulette->varA0 = sRoulette->ballDistToCenter;
    sRoulette->ballFallSpeed = ballFallSpeed;
    sRoulette->ballFallAccel = -((ballFallSpeed * 2.0f) / ballFallDist + (2.0f / (ballFallDist * ballFallDist)));
    sRoulette->ballAngleSpeed = 0.0f;
    sprite->animPaused = FALSE;
    sprite->animNum = 0;
    sprite->animBeginning = TRUE;
    sprite->animEnded = FALSE;
    sprite->callback = SpriteCB_UnstickBall_ShroomishBallFall;
    sprite->data[2] = 0;
}

static void SpriteCB_UnstickBall_TaillowDrop(struct Sprite *sprite)
{
    sprite->y2 = (s16)(sprite->data[2] * 0.05f * sprite->data[2]) - 45;
    sprite->data[2]++;
    if (sprite->data[2] >= DEGREES_PER_SLOT && sprite->y2 >= 0)
    {
        LandBall()
        sRoulette->ballUnstuck = TRUE;
    }
}

static void SpriteCB_UnstickBall_TaillowPickUp(struct Sprite *sprite)
{
    if (sprite->data[2]++ < 45)
    {
        sprite->y2--;
        if (sprite->data[2] == 45)
        {
            if (gSprites[sRoulette->spriteIds[SPR_CLEAR_MON]].animCmdIndex == 1)
                sprite->y2++;
        }
    }
    else
    {
        if (sprite->data[2] < sprite->data[7])
        {
            if (gSprites[sRoulette->spriteIds[SPR_CLEAR_MON]].animDelayCounter == 0)
            {
                if (gSprites[sRoulette->spriteIds[SPR_CLEAR_MON]].animCmdIndex == 1)
                    sprite->y2++;
                else
                    sprite->y2--;
            }
        }
        else
        {
            sprite->animPaused = FALSE;
            sprite->animNum = 1;
            sprite->animBeginning = TRUE;
            sprite->animEnded = FALSE;
            sprite->data[2] = 0;
            sprite->callback = SpriteCB_UnstickBall_TaillowDrop;
            m4aSongNumStart(SE_BALL_THROW);
        }
    }
}

static void SpriteCB_UnstickBall_Taillow(struct Sprite *sprite)
{
    UpdateBallPos(sprite);

    switch (sprite->sBallAngle)
    {
    case 90:
        if (sprite->sStuckOnWheelLeft != TRUE)
        {
            sprite->callback = &SpriteCB_UnstickBall_TaillowPickUp;
            sprite->data[2] = 0;
        }
        break;
    case 270:
        if (sprite->sStuckOnWheelLeft)
        {
            sprite->callback = &SpriteCB_UnstickBall_TaillowPickUp;
            sprite->data[2] = 0;
        }
        break;
    }
}

// The below SpriteCB_UnstickBall_* callbacks handle the ball while its being cleared by Shroomish/Taillow
// For what Shroomish/Taillow do during this sequence, see SpriteCB_Shroomish / SpriteCB_Taillow
static void SpriteCB_UnstickBall(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    switch (sRoulette->useTaillow)
    {
    default:
    case FALSE:
        CreateShroomishSprite(sprite);
        sprite->callback = SpriteCB_UnstickBall_Shroomish;
        break;
    case TRUE:
        CreateTaillowSprite(sprite);
        sprite->callback = SpriteCB_UnstickBall_Taillow;
        break;
    }
}

#define sStillStuck data[0]

static void SpriteCB_RollBall_TryLandAdjacent(struct Sprite *sprite)
{
    UpdateBallPos(sprite);

    if (sprite->data[2]-- == 16)
        sRoulette->ballFallSpeed *= -1.0f;

    if (sprite->data[2] == 0)
    {
        if (!sprite->sStillStuck)
        {
            // Ball can successfully fall into adjacent space
            LandBall()
        }
        else
        {
            // Ball is stuck, need Shroomish/Taillow to clear ball
            sprite->animPaused = TRUE;
            m4aSongNumStart(SE_BALL_BOUNCE_1);
            SetBallStuck(sprite);
        }
    }
}

static void SpriteCB_RollBall_TryLand(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    sprite->data[2] = 0;
    UpdateSlotBelowBall(sprite);
    if (!(sRouletteSlots[sRoulette->hitSlot].flag & sRoulette->hitFlags))
    {
        // Space is empty, land successfully
        LandBall()
    }
    else
    {
        // Space has already been landed on, try to fall into adjacent space
        u8 slotId;
        u32 fallRight;
        m4aSongNumStart(SE_BALL_BOUNCE_1);
        fallRight = Random() & 1;
        if (fallRight)
        {
            sRoulette->ballAngleSpeed = 0.0f;
            sRoulette->stuckHitSlot = slotId = (sRoulette->hitSlot + 1) % NUM_ROULETTE_SLOTS;
        }
        else // fall left
        {
            f32 temp;
            sRoulette->ballAngleSpeed = (temp = sRouletteTables[sRoulette->tableId].var1C) * 2.0f;
            slotId = (sRoulette->hitSlot + NUM_ROULETTE_SLOTS - 1) % NUM_ROULETTE_SLOTS;
            sRoulette->stuckHitSlot = sRoulette->hitSlot;
        }
        if (sRouletteSlots[slotId].flag & sRoulette->hitFlags)
        {
            // Attempted adjacent space has also been landed on
            sprite->sStillStuck = TRUE;
            sprite->data[2] = sRouletteTables[sRoulette->tableId].randDistanceLow;
        }
        else
        {
            sprite->sStillStuck = FALSE;
            if (sRoulette->tableId)
            {
                sprite->data[2] = sRouletteTables[sRoulette->tableId].randDistanceHigh;
            }
            else
            {
                sprite->data[2] = sRouletteTables[sRoulette->tableId].randDistanceLow;
                if (fallRight)
                    sRoulette->ballAngleSpeed = 0.5f;
                else
                    sRoulette->ballAngleSpeed = -1.5f;
            }
        }
        sRoulette->ballFallSpeed = 0.085f;
        sprite->callback = SpriteCB_RollBall_TryLandAdjacent;
        sprite->sState = 5;
    }
}

#undef sStillStuck

static void SpriteCB_RollBall_Slow(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    if (sRoulette->ballAngleSpeed > 0.5f)
        return;

    UpdateSlotBelowBall(sprite);
    if (GetBallDistanceToSlotMidpoint(sprite) == 0)
    {
        // Reached slot to land in
        sRoulette->ballAngleAccel = 0.0f;
        sRoulette->ballAngleSpeed -= (f32)(sRouletteTables[sRoulette->tableId].wheelSpeed)
            / (sRouletteTables[sRoulette->tableId].wheelDelay + 1);
        sprite->sState = 4;
        sprite->callback = SpriteCB_RollBall_TryLand;
    }
    else
    {
        if (sRoulette->ballAngleAccel != 0.0f)
        {
            if (sRoulette->ballAngleSpeed < 0.0f)
            {
                sRoulette->ballAngleAccel = 0.0f;
                sRoulette->ballAngleSpeed = 0.0f;
                sRoulette->ballFallSpeed /= 1.2;
            }
        }
    }
}

static void SpriteCB_RollBall_Medium(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    if (sRoulette->ballDistToCenter > 40.0f)
        return;

    sRoulette->ballFallSpeed = -(4.0f / (f32)(sRoulette->ballTravelDistSlow));
    sRoulette->ballAngleAccel = -(sRoulette->ballAngleSpeed / (f32)(sRoulette->ballTravelDistSlow));
    sprite->animNum = 2;
    sprite->animBeginning = TRUE;
    sprite->animEnded = FALSE;
    sprite->sState = 3;
    sprite->callback = SpriteCB_RollBall_Slow;
}

static void SpriteCB_RollBall_Fast(struct Sprite *sprite)
{
    UpdateBallPos(sprite);
    if (sRoulette->ballDistToCenter > 60.0f)
        return;

    m4aSongNumStartOrChange(SE_ROULETTE_BALL2);
    sRoulette->ballFallSpeed = -(20.0f / (f32)(sRoulette->ballTravelDistMed));
    sRoulette->ballAngleAccel = ((1.0f - sRoulette->ballAngleSpeed) / (f32)(sRoulette->ballTravelDistMed));
    sprite->animNum = 1;
    sprite->animBeginning = TRUE;
    sprite->animEnded = FALSE;
    sprite->sState = 2;
    sprite->callback = SpriteCB_RollBall_Medium;
}

static void SpriteCB_RollBall_Start(struct Sprite *sprite)
{
    sprite->sState = 1;
    sprite->data[2] = 0;
    UpdateBallPos(sprite);
    sprite->invisible = FALSE;
    sprite->callback = SpriteCB_RollBall_Fast;
}

// Sprite data for Shroomish / its shadows
#define sMonSpriteId        data[4]
#define sBallShadowSpriteId data[5]
#define sMonShadowSpriteId  data[6]

static void CreateShroomishSprite(struct Sprite *ball)
{
    u16 t;
    u8 i;
    s16 coords[2][2] = {
        {116, 44},
        {116, 112}
    };
    struct Roulette UNUSED *roulette;

    t = ball->data[7] - 2;
    roulette = sRoulette;  // Unnecessary, needed to match
    sRoulette->spriteIds[SPR_CLEAR_MON] = CreateSprite(&sSpriteTemplate_Shroomish, 36, -12, 50);
    sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1] = CreateSprite(&sSpriteTemplate_ShroomishShadow[0], coords[ball->sStuckOnWheelLeft][0], coords[ball->sStuckOnWheelLeft][1], 59);
    sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_2] = CreateSprite(&sSpriteTemplate_ShroomishShadow[1], 36, 140, 51);
    gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_2]].oam.objMode = ST_OAM_OBJ_BLEND;
    for (i = 0; i < 3; i++)
    {
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].coordOffsetEnabled = FALSE;
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].invisible = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].animPaused = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].affineAnimPaused = TRUE;
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].sMonSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON];
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].sBallShadowSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1];
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].sMonShadowSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_2];
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].data[2] = t;
        gSprites[sRoulette->spriteIds[i + SPR_CLEAR_MON]].data[3] = (ball->data[7] * sRouletteTables[sRoulette->tableId].randDistanceHigh) +
                                                                (sRouletteTables[sRoulette->tableId].randDistanceLow + 0xFFFF);
    }
    gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1]].coordOffsetEnabled = TRUE;
    sRoulette->ball = ball;
}

static void CreateTaillowSprite(struct Sprite *ball)
{
    u8 i = 0;
    s16 t;
    s16 coords[2][2] = {
        {256, 84}, // Right approach
        {-16, 84}  // Left approach
    };

    t = ball->data[7] - 2;
    sRoulette->spriteIds[SPR_CLEAR_MON] = CreateSprite(&sSpriteTemplate_Taillow, coords[ball->sStuckOnWheelLeft][0], coords[ball->sStuckOnWheelLeft][1], 50);
    StartSpriteAnim(&gSprites[sRoulette->spriteIds[SPR_CLEAR_MON]], ball->sStuckOnWheelLeft);
    sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1] = CreateSprite(&sSpriteTemplate_TaillowShadow, coords[ball->sStuckOnWheelLeft][0], coords[ball->sStuckOnWheelLeft][1], 51);
    gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1]].affineAnimPaused = TRUE;
    gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1]].animPaused = TRUE;
    ball->data[7] = (t * sRouletteTables[sRoulette->tableId].randDistanceHigh) + (sRouletteTables[sRoulette->tableId].taillow.baseDropDelay + 45);
    for (; i < 2; i++)
    {
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON + i]].sMonSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON];
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON + i]].sBallShadowSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1];
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON + i]].sMonShadowSpriteId = sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1];
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON + i]].data[2] = t;
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON + i]].data[3] = ball->data[7] - 45;
    }
    sRoulette->ball = ball;
}

static void SetBallStuck(struct Sprite *sprite)
{
    u8 slotId;
    u16 angle;
    u8 numCandidates = 0;
    u8 maxSlotToCheck = 5;
    u8 betSlotId = 0;
    u8 i = 0;
    u8 slotsToSkip;
    u8 slotCandidates[NUM_ROULETTE_SLOTS - 2] = {}; // - 2 because we know at least 2 are already occupied
    u16 rand = Random();

    sRoulette->ballState = BALL_STATE_STUCK;
    sRoulette->ballStuck = TRUE;
    sRoulette->ballUnstuck = FALSE;
    sRoulette->hitSlot = 0xFF;
    sRoulette->ballAngle = sprite->sBallAngle;
    sRoulette->ballFallSpeed = 0.0f;
    sRoulette->ballAngleSpeed = sRouletteTables[sRoulette->tableId].var1C;

    angle = (sRoulette->tableId * DEGREES_PER_SLOT + 33) + (1 - sRoulette->useTaillow) * 15;

    // Determine which quadrant the ball got stuck in
    // Use either Shroomish or Taillow to clear the ball depending on where it's stuck
    for (i = 0; i < 4; i++)
    {
        if (angle < sprite->sBallAngle && sprite->sBallAngle <= angle + 90)
        {
            sprite->sStuckOnWheelLeft = i / 2;
            sRoulette->useTaillow = i % 2;
            break;
        }
        if (i == 3)
        {
            sprite->sStuckOnWheelLeft = TRUE;
            sRoulette->useTaillow = TRUE;
            break;
        }
        angle += 90;
    }

    if (sRoulette->useTaillow)
    {
        if (sprite->sStuckOnWheelLeft)
            PlayCry_Normal(SPECIES_TAILLOW, -63);
        else
            PlayCry_Normal(SPECIES_TAILLOW, 63);
    }
    else
    {
        PlayCry_Normal(SPECIES_SHROOMISH, -63);
    }

    slotsToSkip = 2;
    slotId = (sRoulette->stuckHitSlot + 2) % NUM_ROULETTE_SLOTS;

    if (sRoulette->useTaillow == TRUE && sRoulette->tableId == 1)
        maxSlotToCheck += 6; // Check all remaining slots
    else
        maxSlotToCheck += slotsToSkip; // Check enough slots to guarantee an empty will be found

    // Identify open slots on the wheel that the stuck ball could be moved to
    for (i = slotsToSkip; i < maxSlotToCheck; i++)
    {
        if (!(sRoulette->hitFlags & sRouletteSlots[slotId].flag))
        {
            slotCandidates[numCandidates++] = i;
            if (betSlotId == 0 && (sRouletteSlots[slotId].flag & sGridSelections[sRoulette->betSelection[sRoulette->curBallNum]].inSelectionFlags))
                betSlotId = i;
        }
        slotId = (slotId + 1) % NUM_ROULETTE_SLOTS;
    }

    // Determine which identified slot the ball should be moved to
    // The below slot ids are relative to the slot the ball got stuck on
    if ((sRoulette->useTaillow + 1) & sRoulette->partySpeciesFlags)
    {
        // If the player has the corresponding Pokémon in their party (HAS_SHROOMISH or HAS_TAILLOW),
        // there's a 75% chance that the ball will be moved to a spot they bet on
        // assuming it was one of the slots identified as a candidate
        if (betSlotId && (rand % 256) < 192)
            sprite->data[7] = betSlotId;
        else
            sprite->data[7] = slotCandidates[rand % numCandidates];
    }
    else
    {
        sprite->data[7] = slotCandidates[rand % numCandidates];
    }

    sprite->callback = SpriteCB_UnstickBall;
}

static const u16 sShroomishShadowAlphas[] = {
    0x907,
    0x808,
    0x709,
    0x60A,
    0x50B,
    0x40C,
    0x30D,
    0x20E,
    0x10F,
    0x010,
};

static void SpriteCB_ShroomishExit(struct Sprite *sprite)
{
    // Delay for screen shaking, then exit left
    if (sprite->data[1]++ >= sprite->data[3])
    {
        sprite->x -= 2;
        if (sprite->x < -16)
        {
            if (!sRoulette->ballUnstuck)
                sRoulette->ballUnstuck = TRUE;
            DestroySprite(sprite);
            sRoulette->shroomishShadowTimer = 0;
            sRoulette->shroomishShadowAlpha = sShroomishShadowAlphas[0];
        }
    }
}

// Handles both the screen shake and ball shadow effect for when Shroomish unsticks the ball
static void SpriteCB_ShroomishShakeScreen(struct Sprite *sprite)
{
    int screenShakeIdx;
    u16 screenShakeOffsets[][4] = {
        {-1, 0, 1, 0},
        {-2, 0, 2, 0},
        {-3, 0, 3, 0},
    };

    if (sprite->data[1]++ < sprite->data[3])
    {
        if (sprite->data[1] & 1)
        {
            // Shake screen
            gSpriteCoordOffsetY = screenShakeOffsets[sprite->data[2] / 2][sprite->data[7]];
            screenShakeIdx = sprite->data[7] + 1;
            sprite->data[7] = screenShakeIdx - ((screenShakeIdx / 4) * 4);
        }
        // Flicker shadow
        sprite->invisible ^= 1;
    }
    else
    {
        gSpriteCoordOffsetY = 0;
        gSprites[sRoulette->spriteIds[SPR_CLEAR_MON]].animPaused = FALSE;
        DestroySprite(sprite);
    }
}

static void SpriteCB_ShroomishFall(struct Sprite *sprite)
{
    f32 timer;
    sprite->data[1]++;
    timer = sprite->data[1];
    sprite->y2 = timer * 0.039f * timer;
    sRoulette->shroomishShadowAlpha = sShroomishShadowAlphas[(sRoulette->shroomishShadowTimer - 1) / 2];
    if (sRoulette->shroomishShadowTimer < ARRAY_COUNT(sShroomishShadowAlphas) * 2 - 1)
        sRoulette->shroomishShadowTimer++;
    if (sprite->data[1] > 60)
    {
        sprite->data[1] = 0;
        sprite->callback = SpriteCB_ShroomishExit;
        gSprites[sprite->sMonShadowSpriteId].callback  = SpriteCB_ShroomishExit;
        gSprites[sprite->sMonShadowSpriteId].data[1] = -2;
        gSprites[sprite->sBallShadowSpriteId].invisible = FALSE;
        gSprites[sprite->sBallShadowSpriteId].callback  = SpriteCB_ShroomishShakeScreen;
        m4aSongNumStart(SE_M_STRENGTH);
    }
}

static void SpriteCB_Shroomish(struct Sprite *sprite)
{
    if (sprite->data[7] == 0)
    {
        // Wait for the ball to be a specific angle (or its 180 degree opposite) specified by the table
        // Once it is, reveal the shadow for Shroomish falling in
        if (!sRoulette->ball->sStuckOnWheelLeft)
        {
            if (sRoulette->ball->sBallAngle != sRouletteTables[sRoulette->tableId].shroomish.startAngle)
                return;
        }
        else
        {
            if (sRoulette->ball->sBallAngle != sRouletteTables[sRoulette->tableId].shroomish.startAngle + 180)
                return;
        }

        sprite->invisible = FALSE;
        sprite->data[7]++;
        m4aSongNumStart(SE_FALL);
        sRoulette->shroomishShadowTimer = 1;
        sRoulette->shroomishShadowAlpha = sShroomishShadowAlphas[0];
    }
    else
    {
        sRoulette->shroomishShadowAlpha = sShroomishShadowAlphas[(sRoulette->shroomishShadowTimer - 1) / 2];
        if (sRoulette->shroomishShadowTimer < 19)
            sRoulette->shroomishShadowTimer++;

        // Wait for the ball to be a specific angle (or its 180 degree opposite) specified by the table
        // Once it is, have Shroomish begin to fall in
        // On both tables this angle is 15 degrees off the "start" angle
        if (!sRoulette->ball->sStuckOnWheelLeft)
        {
            if (sRoulette->ball->sBallAngle != sRouletteTables[sRoulette->tableId].shroomish.dropAngle)
                return;
        }
        else
        {
            if (sRoulette->ball->sBallAngle != sRouletteTables[sRoulette->tableId].shroomish.dropAngle + 180)
                return;
        }

        gSprites[sprite->sMonSpriteId].callback  = SpriteCB_ShroomishFall;
        gSprites[sprite->sMonSpriteId].invisible = FALSE;
        sprite->callback  = &SpriteCallbackDummy;
        sprite->data[7] = 0;
    }
}

static void SpriteCB_TaillowShadow_Flash(struct Sprite *sprite)
{
    sprite->invisible ^= 1;
}

static void SpriteCB_Taillow_FlyAway(struct Sprite *sprite)
{
    if (sprite->y > -16)
    {
        sprite->y--;
    }
    else
    {
        sprite->callback = SpriteCallbackDummy;
        sprite->invisible = TRUE;
        sprite->animPaused = TRUE;
        m4aSongNumStop(SE_TAILLOW_WING_FLAP);
        DestroySprite(sprite);
        FreeOamMatrix(gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1]].oam.matrixNum);
        DestroySprite(&gSprites[sRoulette->spriteIds[SPR_CLEAR_MON_SHADOW_1]]);
    }
}

static void SpriteCB_Taillow_PickUpBall(struct Sprite *sprite)
{
    if (sprite->data[1] >= 0)
    {
        sprite->data[1]--;
        sprite->y--;
        if (sprite->data[1] == 0 && sprite->animCmdIndex == 1)
            sprite->y2++;
    }
    else
    {
        if (sprite->data[3] >= 0)
        {
            sprite->data[3]--;
            if (sprite->animDelayCounter == 0)
            {
                if (sprite->animCmdIndex == 1)
                    sprite->y2++;
                else
                    sprite->y2--;
            }
        }
        else
        {
            m4aSongNumStart(SE_FALL);
            StartSpriteAnim(sprite, sRoulette->ball->sStuckOnWheelLeft + 4);
            sprite->callback = SpriteCB_Taillow_FlyAway;
            gSprites[sprite->sMonShadowSpriteId].affineAnimPaused = FALSE;
        }
    }
}

static void SpriteCB_Taillow_FlyIn(struct Sprite *sprite)
{
    s8 xMoveOffsets[2] = {-1, 1};
    s8 yMoveOffsets[][2] = {
        {2, 0},
        {2, 0},
        {2, -1},
        {2, -1},
        {2, -1},
        {2, -1},
        {2, -2},
        {2, -2},
    };

    if (sprite->data[1]-- > 7)
    {
        sprite->x += xMoveOffsets[sRoulette->ball->sStuckOnWheelLeft] * 2;
        if (IsSEPlaying())
        {
            s8 pan = -((116 - sprite->x) / 2);
            m4aMPlayPanpotControl(&gMPlayInfo_SE1, TRACKS_ALL, pan);
            m4aMPlayPanpotControl(&gMPlayInfo_SE2, TRACKS_ALL, pan);
        }
    }
    else
    {
        if (sprite->data[1] >= 0)
        {
            sprite->x += xMoveOffsets[sRoulette->ball->sStuckOnWheelLeft] * yMoveOffsets[7 - sprite->data[1]][0];
            sprite->y += yMoveOffsets[7 - sprite->data[1]][1];
        }
        else
        {
            m4aSongNumStartOrChange(SE_TAILLOW_WING_FLAP);
            if (sRoulette->ball->sStuckOnWheelLeft == 0)
                PlayCry_Normal(SPECIES_TAILLOW, 63);
            else
                PlayCry_Normal(SPECIES_TAILLOW, -63);
            StartSpriteAnim(sprite, sRoulette->ball->sStuckOnWheelLeft + 2);
            sprite->data[1] = 45;
            sprite->callback = SpriteCB_Taillow_PickUpBall;
        }
    }
}

static void SpriteCB_TaillowShadow_FlyIn(struct Sprite *sprite)
{
    s8 moveDir[2] = {-1, 1};

    if (sprite->data[1]-- >= 0)
    {
        sprite->x += moveDir[sRoulette->ball->sStuckOnWheelLeft] * 2;
        gSprites[sprite->sMonShadowSpriteId].invisible ^= 1;
    }
    else
    {
        sprite->callback = SpriteCB_TaillowShadow_Flash;
    }
}

static void SpriteCB_Taillow(struct Sprite *sprite)
{
    if (sRoulette->ball->sStuckOnWheelLeft == FALSE)
    {
        if (sRoulette->ball->sBallAngle == sRouletteTables[sRoulette->tableId].taillow.rightStartAngle + 90)
        {
            gSprites[sprite->sMonShadowSpriteId].data[1] = 52;
            gSprites[sprite->sMonSpriteId].data[1] = 52;
        }
        else
        {
            return;
        }
    }
    else
    {
        if (sRoulette->ball->sBallAngle == sRouletteTables[sRoulette->tableId].taillow.leftStartAngle + 270)
        {
            gSprites[sprite->sMonShadowSpriteId].data[1] = 46;
            gSprites[sprite->sMonSpriteId].data[1] = 46;
        }
        else
        {
            return;
        }
    }
    gSprites[sprite->sMonShadowSpriteId].callback = SpriteCB_TaillowShadow_FlyIn;
    gSprites[sprite->sMonSpriteId].callback = SpriteCB_Taillow_FlyIn;
    m4aSongNumStart(SE_FALL);
}
