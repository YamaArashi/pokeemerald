#include "global.h"
#include "main.h"
#include "battle.h"
#include "battle_anim.h"
#include "frontier_util.h"
#include "battle_message.h"
#include "battle_tent.h"
#include "battle_factory.h"
#include "bg.h"
#include "contest.h"
#include "contest_effect.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "dynamic_placeholder_text_util.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "link.h"
#include "m4a.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "mon_markings.h"
#include "party_menu.h"
#include "palette.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "region_map.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "tv.h"
#include "window.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum {
    PSS_PAGE_INFO,
    PSS_PAGE_SKILLS,
    PSS_PAGE_BATTLE_MOVES,
    PSS_PAGE_CONTEST_MOVES,
    PSS_PAGE_COUNT,
};

// Screen titles (upper left)
#define PSS_LABEL_WINDOW_POKEMON_INFO_TITLE 0
#define PSS_LABEL_WINDOW_POKEMON_SKILLS_TITLE 1
#define PSS_LABEL_WINDOW_BATTLE_MOVES_TITLE 2
#define PSS_LABEL_WINDOW_CONTEST_MOVES_TITLE 3

// Button control text (upper right)
#define PSS_LABEL_WINDOW_PROMPT_CANCEL 4
#define PSS_LABEL_WINDOW_PROMPT_INFO 5
#define PSS_LABEL_WINDOW_PROMPT_SWITCH 6
#define PSS_LABEL_WINDOW_UNUSED1 7

// Info screen
#define PSS_LABEL_WINDOW_POKEMON_INFO_RENTAL 8
#define PSS_LABEL_WINDOW_POKEMON_INFO_TYPE 9

// Skills screen
#define PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT 10 // HP, Attack, Defense
#define PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT 11 // Sp. Attack, Sp. Defense, Speed
#define PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP 12 // EXP, Next Level
#define PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS 13

// Moves screen
#define PSS_LABEL_WINDOW_MOVES_POWER_ACC 14 // Also contains the power and accuracy values
#define PSS_LABEL_WINDOW_MOVES_APPEAL_JAM 15
#define PSS_LABEL_WINDOW_UNUSED2 16

// Above/below the pokemon's portrait (left)
#define PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER 17
#define PSS_LABEL_WINDOW_PORTRAIT_NICKNAME 18 // The upper name
#define PSS_LABEL_WINDOW_PORTRAIT_SPECIES 19 // The lower name
#define PSS_LABEL_WINDOW_END 20

// Dynamic fields for the Pokémon Info page
#define PSS_DATA_WINDOW_INFO_ORIGINAL_TRAINER 0
#define PSS_DATA_WINDOW_INFO_ID 1
#define PSS_DATA_WINDOW_INFO_ABILITY 2
#define PSS_DATA_WINDOW_INFO_MEMO 3

// Dynamic fields for the Pokémon Skills page
#define PSS_DATA_WINDOW_SKILLS_HELD_ITEM 0
#define PSS_DATA_WINDOW_SKILLS_RIBBON_COUNT 1
#define PSS_DATA_WINDOW_SKILLS_STATS_LEFT 2 // HP, Attack, Defense
#define PSS_DATA_WINDOW_SKILLS_STATS_RIGHT 3 // Sp. Attack, Sp. Defense, Speed
#define PSS_DATA_WINDOW_EXP 4 // Exp, next level

// Dynamic fields for the Battle Moves and Contest Moves pages.
#define PSS_DATA_WINDOW_MOVE_NAMES 0
#define PSS_DATA_WINDOW_MOVE_PP 1
#define PSS_DATA_WINDOW_MOVE_DESCRIPTION 2

#define MOVE_SELECTOR_SPRITES_COUNT 10
#define TYPE_ICON_SPRITE_COUNT (MAX_MON_MOVES + 1)
// for the spriteIds field in PokemonSummaryScreenData
enum
{
    SPRITE_ARR_ID_MON,
    SPRITE_ARR_ID_BALL,
    SPRITE_ARR_ID_STATUS,
    SPRITE_ARR_ID_TYPE, // 2 for mon types, 5 for move types(4 moves and 1 to learn), used interchangeably, because mon types and move types aren't shown on the same screen
    SPRITE_ARR_ID_MOVE_SELECTOR1 = SPRITE_ARR_ID_TYPE + TYPE_ICON_SPRITE_COUNT, // 10 sprites that make up the selector
    SPRITE_ARR_ID_MOVE_SELECTOR2 = SPRITE_ARR_ID_MOVE_SELECTOR1 + MOVE_SELECTOR_SPRITES_COUNT,
    SPRITE_ARR_ID_COUNT = SPRITE_ARR_ID_MOVE_SELECTOR2 + MOVE_SELECTOR_SPRITES_COUNT
};

#define TILE_EMPTY_APPEAL_HEART  0x1039
#define TILE_FILLED_APPEAL_HEART 0x103A
#define TILE_FILLED_JAM_HEART    0x103C
#define TILE_EMPTY_JAM_HEART     0x103D

static EWRAM_DATA struct PokemonSummaryScreenData
{
    /*0x00*/ union {
        struct Pokemon *mons;
        struct BoxPokemon *boxMons;
    } monList;
    /*0x04*/ MainCallback callback;
    /*0x08*/ struct Sprite *markingsSprite;
    /*0x0C*/ struct Pokemon currentMon;
    /*0x70*/ struct PokeSummary
    {
        u16 species; // 0x0
        u16 species2; // 0x2
        u8 isEgg; // 0x4
        u8 level; // 0x5
        u8 ribbonCount; // 0x6
        u8 ailment; // 0x7
        u8 abilityNum; // 0x8
        u8 metLocation; // 0x9
        u8 metLevel; // 0xA
        u8 metGame; // 0xB
        u32 pid; // 0xC
        u32 exp; // 0x10
        u16 moves[MAX_MON_MOVES]; // 0x14
        u8 pp[MAX_MON_MOVES]; // 0x1C
        u16 currentHP; // 0x20
        u16 maxHP; // 0x22
        u16 atk; // 0x24
        u16 def; // 0x26
        u16 spatk; // 0x28
        u16 spdef; // 0x2A
        u16 speed; // 0x2C
        u16 item; // 0x2E
        u16 friendship; // 0x30
        u8 OTGender; // 0x32
        u8 nature; // 0x33
        u8 ppBonuses; // 0x34
        u8 sanity; // 0x35
        u8 OTName[17]; // 0x36
        u32 OTID; // 0x48
    } summary;
    u16 bgTilemapBuffers[PSS_PAGE_COUNT][2][0x400];
    u8 mode;
    bool8 isBoxMon;
    u8 curMonIndex;
    u8 maxMonIndex;
    u8 currPageIndex;
    u8 minPageIndex;
    u8 maxPageIndex;
    bool8 lockMonFlag; // This is used to prevent the player from changing Pokémon in the move deleter select, etc, but it is not needed because the input is handled differently there
    u16 newMove;
    u8 firstMoveIndex;
    u8 secondMoveIndex;
    bool8 lockMovesFlag; // This is used to prevent the player from changing position of moves in a battle or when trading.
    u8 bgDisplayOrder; // Determines the order page backgrounds are loaded while scrolling between them
    u8 filler40CA;
    u8 windowIds[8];
    u8 spriteIds[SPRITE_ARR_ID_COUNT];
    bool8 handleDeoxys;
    s16 switchCounter; // Used for various switch statement cases that decompress/load graphics or Pokémon data
    u8 unk_filler4[6];
} *sMonSummaryScreen = NULL;
EWRAM_DATA u8 gLastViewedMonIndex = 0;
static EWRAM_DATA u8 sMoveSlotToReplace = 0;
ALIGNED(4) static EWRAM_DATA u8 sAnimDelayTaskId = 0;

// forward declarations
static bool8 LoadGraphics(void);
static void CB2_InitSummaryScreen(void);
static void InitBGs(void);
static bool8 DecompressGraphics(void);
static void CopyMonToSummaryStruct(struct Pokemon *);
static bool8 ExtractMonDataToSummaryStruct(struct Pokemon *);
static void SetDefaultTilemaps(void);
static void CloseSummaryScreen(u8);
static void Task_HandleInput(u8);
static void ChangeSummaryPokemon(u8, s8);
static void Task_ChangeSummaryMon(u8);
static s8 AdvanceMonIndex(s8);
static s8 AdvanceMultiBattleMonIndex(s8);
static bool8 IsValidToViewInMulti(struct Pokemon *);
static void ChangePage(u8, s8);
static void PssScrollRight(u8);
static void PssScrollRightEnd(u8);
static void PssScrollLeft(u8);
static void PssScrollLeftEnd(u8);
static void TryDrawExperienceProgressBar(void);
static void SwitchToMoveSelection(u8);
static void Task_HandleInput_MoveSelect(u8);
static bool8 HasMoreThanOneMove(void);
static void ChangeSelectedMove(s16 *, s8, u8 *);
static void CloseMoveSelectMode(u8);
static void SwitchToMovePositionSwitchMode(u8);
static void Task_HandleInput_MovePositionSwitch(u8);
static void ExitMovePositionSwitchMode(u8, bool8);
static void SwapMonMoves(struct Pokemon *, u8, u8);
static void SwapBoxMonMoves(struct BoxPokemon *, u8, u8);
static void Task_SetHandleReplaceMoveInput(u8);
static void Task_HandleReplaceMoveInput(u8);
static bool8 CanReplaceMove(void);
static void ShowCantForgetHMsWindow(u8);
static void Task_HandleInputCantForgetHMsMoves(u8);
static void DrawPagination(void);
static void HandlePowerAccTilemap(u16, s16);
static void Task_ShowPowerAccWindow(u8);
static void HandleAppealJamTilemap(u16, s16, u16);
static void Task_ShowAppealJamWindow(u8);
static void HandleStatusTilemap(u16, s16);
static void Task_ShowStatusWindow(u8);
static void TilemapFiveMovesDisplay(u16 *, u16, bool8);
static void DrawPokerusCuredSymbol(struct Pokemon *);
static void DrawExperienceProgressBar(struct Pokemon *);
static void DrawContestMoveHearts(u16);
static void LimitEggSummaryPageDisplay(void);
static void ResetWindows(void);
static void PrintMonInfo(void);
static void PrintNotEggInfo(void);
static void PrintEggInfo(void);
static void PrintGenderSymbol(struct Pokemon *, u16);
static void PrintPageNamesAndStats(void);
static void PutPageWindowTilemaps(u8);
static void ClearPageWindowTilemaps(u8);
static void RemoveWindowByIndex(u8);
static void PrintPageSpecificText(u8);
static void CreateTextPrinterTask(u8);
static void PrintInfoPageText(void);
static void Task_PrintInfoPage(u8);
static void PrintMonOTName(void);
static void PrintMonOTID(void);
static void PrintMonAbilityName(void);
static void PrintMonAbilityDescription(void);
static void BufferMonTrainerMemo(void);
static void PrintMonTrainerMemo(void);
static void BufferNatureString(void);
static void GetMetLevelString(u8 *);
static bool8 DoesMonOTMatchOwner(void);
static bool8 DidMonComeFromGBAGames(void);
static bool8 IsInGamePartnerMon(void);
static void PrintEggOTName(void);
static void PrintEggOTID(void);
static void PrintEggState(void);
static void PrintEggMemo(void);
static void Task_PrintSkillsPage(u8);
static void PrintHeldItemName(void);
static void PrintSkillsPageText(void);
static void PrintRibbonCount(void);
static void BufferLeftColumnStats(void);
static void PrintLeftColumnStats(void);
static void BufferRightColumnStats(void);
static void PrintRightColumnStats(void);
static void PrintExpPointsNextLevel(void);
static void PrintBattleMoves(void);
static void Task_PrintBattleMoves(u8);
static void PrintMoveNameAndPP(u8);
static void PrintContestMoves(void);
static void Task_PrintContestMoves(u8);
static void PrintContestMoveDescription(u8);
static void PrintMoveDetails(u16);
static void PrintNewMoveDetailsOrCancelText(void);
static void AddAndFillMoveNamesWindow(void);
static void SwapMovesNamesPP(u8, u8);
static void PrintHMMovesCantBeForgotten(void);
static void ResetSpriteIds(void);
static void SetSpriteInvisibility(u8, bool8);
static void HidePageSpecificSprites(void);
static void SetTypeIcons(void);
static void CreateMoveTypeIcons(void);
static void SetMonTypeIcons(void);
static void SetMoveTypeIcons(void);
static void SetContestMoveTypeIcons(void);
static void SetNewMoveTypeIcon(void);
static void SwapMovesTypeSprites(u8, u8);
static u8 LoadMonGfxAndSprite(struct Pokemon *, s16 *);
static u8 CreateMonSprite(struct Pokemon *);
static void SpriteCB_Pokemon(struct Sprite *);
static void StopPokemonAnimations(void);
static void CreateMonMarkingsSprite(struct Pokemon *);
static void RemoveAndCreateMonMarkingsSprite(struct Pokemon *);
static void CreateCaughtBallSprite(struct Pokemon *);
static void CreateSetStatusSprite(void);
static void CreateMoveSelectorSprites(u8);
static void SpriteCB_MoveSelector(struct Sprite *);
static void DestroyMoveSelectorSprites(u8);
static void SetMainMoveSelectorColor(u8);
static void KeepMoveSelectorVisible(u8);
static void SummaryScreen_DestroyAnimDelayTask(void);

// const rom data
#include "data/text/move_descriptions.h"
#include "data/text/nature_names.h"

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 27,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 25,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

struct TilemapCtrl
{
    const u16 *gfx;
    u16 field_4;
    u8 field_6;
    u8 field_7;
    u8 field_8;
    u8 field_9;
};

static const u16 sStatusTilemap[] = INCBIN_U16("graphics/summary_screen/status_tilemap.bin");
static const struct TilemapCtrl sStatusTilemapCtrl1 =
{
    sStatusTilemap, 1, 10, 2, 0, 18
};
static const struct TilemapCtrl sStatusTilemapCtrl2 =
{
    sStatusTilemap, 1, 10, 2, 0, 50
};
static const struct TilemapCtrl sBattleMoveTilemapCtrl =
{
    gSummaryScreen_MoveEffect_Battle_Tilemap, 0, 10, 7, 0, 45
};
static const struct TilemapCtrl sContestMoveTilemapCtrl =
{
    gSummaryScreen_MoveEffect_Contest_Tilemap, 0, 10, 7, 0, 45
};
static const s8 sMultiBattleOrder[] = {0, 2, 3, 1, 4, 5};
static const struct WindowTemplate sSummaryTemplate[] =
{
    [PSS_LABEL_WINDOW_POKEMON_INFO_TITLE] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 11,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 1,
    },
    [PSS_LABEL_WINDOW_POKEMON_SKILLS_TITLE] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 11,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 23,
    },
    [PSS_LABEL_WINDOW_BATTLE_MOVES_TITLE] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 11,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 45,
    },
    [PSS_LABEL_WINDOW_CONTEST_MOVES_TITLE] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 11,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 67,
    },
    [PSS_LABEL_WINDOW_PROMPT_CANCEL] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 0,
        .width = 8,
        .height = 2,
        .paletteNum = 7,
        .baseBlock = 89,
    },
    [PSS_LABEL_WINDOW_PROMPT_INFO] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 0,
        .width = 8,
        .height = 2,
        .paletteNum = 7,
        .baseBlock = 105,
    },
    [PSS_LABEL_WINDOW_PROMPT_SWITCH] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 0,
        .width = 8,
        .height = 2,
        .paletteNum = 7,
        .baseBlock = 121,
    },
    [PSS_LABEL_WINDOW_UNUSED1] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 4,
        .width = 0,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 137,
    },
    [PSS_LABEL_WINDOW_POKEMON_INFO_RENTAL] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 4,
        .width = 18,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 137,
    },
    [PSS_LABEL_WINDOW_POKEMON_INFO_TYPE] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 6,
        .width = 18,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 173,
    },
    [PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT] = {
        .bg = 0,
        .tilemapLeft = 10,
        .tilemapTop = 7,
        .width = 6,
        .height = 6,
        .paletteNum = 6,
        .baseBlock = 209,
    },
    [PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 7,
        .width = 5,
        .height = 6,
        .paletteNum = 6,
        .baseBlock = 245,
    },
    [PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP] = {
        .bg = 0,
        .tilemapLeft = 10,
        .tilemapTop = 14,
        .width = 11,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 275,
    },
    [PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 6,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 319,
    },
    [PSS_LABEL_WINDOW_MOVES_POWER_ACC] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 9,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 331,
    },
    [PSS_LABEL_WINDOW_MOVES_APPEAL_JAM] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 5,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 367,
    },
    [PSS_LABEL_WINDOW_UNUSED2] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 4,
        .width = 0,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 387,
    },
    [PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 2,
        .width = 4,
        .height = 2,
        .paletteNum = 7,
        .baseBlock = 387,
    },
    [PSS_LABEL_WINDOW_PORTRAIT_NICKNAME] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 12,
        .width = 9,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 395,
    },
    [PSS_LABEL_WINDOW_PORTRAIT_SPECIES] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 14,
        .width = 9,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 413,
    },
    [PSS_LABEL_WINDOW_END] = DUMMY_WIN_TEMPLATE
};
static const struct WindowTemplate sPageInfoTemplate[] =
{
    [PSS_DATA_WINDOW_INFO_ORIGINAL_TRAINER] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 4,
        .width = 11,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 449,
    },
    [PSS_DATA_WINDOW_INFO_ID] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 4,
        .width = 7,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 471,
    },
    [PSS_DATA_WINDOW_INFO_ABILITY] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 9,
        .width = 18,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 485,
    },
    [PSS_DATA_WINDOW_INFO_MEMO] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 14,
        .width = 18,
        .height = 6,
        .paletteNum = 6,
        .baseBlock = 557,
    },
};
static const struct WindowTemplate sPageSkillsTemplate[] =
{
    [PSS_DATA_WINDOW_SKILLS_HELD_ITEM] = {
        .bg = 0,
        .tilemapLeft = 10,
        .tilemapTop = 4,
        .width = 10,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 449,
    },
    [PSS_DATA_WINDOW_SKILLS_RIBBON_COUNT] = {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 4,
        .width = 10,
        .height = 2,
        .paletteNum = 6,
        .baseBlock = 469,
    },
    [PSS_DATA_WINDOW_SKILLS_STATS_LEFT] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 7,
        .width = 6,
        .height = 6,
        .paletteNum = 6,
        .baseBlock = 489,
    },
    [PSS_DATA_WINDOW_SKILLS_STATS_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 27,
        .tilemapTop = 7,
        .width = 3,
        .height = 6,
        .paletteNum = 6,
        .baseBlock = 525,
    },
    [PSS_DATA_WINDOW_EXP] = {
        .bg = 0,
        .tilemapLeft = 24,
        .tilemapTop = 14,
        .width = 6,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 543,
    },
};
static const struct WindowTemplate sPageMovesTemplate[] = // This is used for both battle and contest moves
{
    [PSS_DATA_WINDOW_MOVE_NAMES] = {
        .bg = 0,
        .tilemapLeft = 15,
        .tilemapTop = 4,
        .width = 9,
        .height = 10,
        .paletteNum = 6,
        .baseBlock = 449,
    },
    [PSS_DATA_WINDOW_MOVE_PP] = {
        .bg = 0,
        .tilemapLeft = 24,
        .tilemapTop = 4,
        .width = 6,
        .height = 10,
        .paletteNum = 8,
        .baseBlock = 539,
    },
    [PSS_DATA_WINDOW_MOVE_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 10,
        .tilemapTop = 15,
        .width = 20,
        .height = 4,
        .paletteNum = 6,
        .baseBlock = 599,
    },
};
static const u8 sTextColors[][3] =
{
    {0, 1, 2},
    {0, 3, 4},
    {0, 5, 6},
    {0, 7, 8},
    {0, 9, 10},
    {0, 11, 12},
    {0, 13, 14},
    {0, 7, 8},
    {13, 15, 14},
    {0, 1, 2},
    {0, 3, 4},
    {0, 5, 6},
    {0, 7, 8}
};

static const u8 sButtons_Gfx[][4 * TILE_SIZE_4BPP] = {
    INCBIN_U8("graphics/summary_screen/a_button.4bpp"),
    INCBIN_U8("graphics/summary_screen/b_button.4bpp"),
};

static void (*const sTextPrinterFunctions[])(void) =
{
    [PSS_PAGE_INFO] = PrintInfoPageText,
    [PSS_PAGE_SKILLS] = PrintSkillsPageText,
    [PSS_PAGE_BATTLE_MOVES] = PrintBattleMoves,
    [PSS_PAGE_CONTEST_MOVES] = PrintContestMoves
};

static void (*const sTextPrinterTasks[])(u8 taskId) =
{
    [PSS_PAGE_INFO] = Task_PrintInfoPage,
    [PSS_PAGE_SKILLS] = Task_PrintSkillsPage,
    [PSS_PAGE_BATTLE_MOVES] = Task_PrintBattleMoves,
    [PSS_PAGE_CONTEST_MOVES] = Task_PrintContestMoves
};

static const u8 sMemoNatureTextColor[] = _("{COLOR LIGHT_RED}{SHADOW GREEN}");
static const u8 sMemoMiscTextColor[] = _("{COLOR WHITE}{SHADOW DARK_GRAY}"); // This is also affected by palettes, apparently
static const u8 sStatsLeftColumnLayout[] = _("{DYNAMIC 0}/{DYNAMIC 1}\n{DYNAMIC 2}\n{DYNAMIC 3}");
static const u8 sStatsRightColumnLayout[] = _("{DYNAMIC 0}\n{DYNAMIC 1}\n{DYNAMIC 2}");
static const u8 sMovesPPLayout[] = _("{PP}{DYNAMIC 0}/{DYNAMIC 1}");

#define TAG_MOVE_SELECTOR 30000
#define TAG_MON_STATUS 30001
#define TAG_MOVE_TYPES 30002
#define TAG_MON_MARKINGS 30003

static const struct OamData sOamData_MoveTypes =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sSpriteAnim_TypeNormal[] = {
    ANIMCMD_FRAME(TYPE_NORMAL * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFighting[] = {
    ANIMCMD_FRAME(TYPE_FIGHTING * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFlying[] = {
    ANIMCMD_FRAME(TYPE_FLYING * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypePoison[] = {
    ANIMCMD_FRAME(TYPE_POISON * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGround[] = {
    ANIMCMD_FRAME(TYPE_GROUND * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeRock[] = {
    ANIMCMD_FRAME(TYPE_ROCK * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeBug[] = {
    ANIMCMD_FRAME(TYPE_BUG * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGhost[] = {
    ANIMCMD_FRAME(TYPE_GHOST * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeSteel[] = {
    ANIMCMD_FRAME(TYPE_STEEL * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeMystery[] = {
    ANIMCMD_FRAME(TYPE_MYSTERY * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFire[] = {
    ANIMCMD_FRAME(TYPE_FIRE * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeWater[] = {
    ANIMCMD_FRAME(TYPE_WATER * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGrass[] = {
    ANIMCMD_FRAME(TYPE_GRASS * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeElectric[] = {
    ANIMCMD_FRAME(TYPE_ELECTRIC * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypePsychic[] = {
    ANIMCMD_FRAME(TYPE_PSYCHIC * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeIce[] = {
    ANIMCMD_FRAME(TYPE_ICE * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeDragon[] = {
    ANIMCMD_FRAME(TYPE_DRAGON * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeDark[] = {
    ANIMCMD_FRAME(TYPE_DARK * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_CategoryCool[] = {
    ANIMCMD_FRAME((CONTEST_CATEGORY_COOL + NUMBER_OF_MON_TYPES) * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_CategoryBeauty[] = {
    ANIMCMD_FRAME((CONTEST_CATEGORY_BEAUTY + NUMBER_OF_MON_TYPES) * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_CategoryCute[] = {
    ANIMCMD_FRAME((CONTEST_CATEGORY_CUTE + NUMBER_OF_MON_TYPES) * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_CategorySmart[] = {
    ANIMCMD_FRAME((CONTEST_CATEGORY_SMART + NUMBER_OF_MON_TYPES) * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_CategoryTough[] = {
    ANIMCMD_FRAME((CONTEST_CATEGORY_TOUGH + NUMBER_OF_MON_TYPES) * 8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd *const sSpriteAnimTable_MoveTypes[NUMBER_OF_MON_TYPES + CONTEST_CATEGORIES_COUNT] = {
    sSpriteAnim_TypeNormal,
    sSpriteAnim_TypeFighting,
    sSpriteAnim_TypeFlying,
    sSpriteAnim_TypePoison,
    sSpriteAnim_TypeGround,
    sSpriteAnim_TypeRock,
    sSpriteAnim_TypeBug,
    sSpriteAnim_TypeGhost,
    sSpriteAnim_TypeSteel,
    sSpriteAnim_TypeMystery,
    sSpriteAnim_TypeFire,
    sSpriteAnim_TypeWater,
    sSpriteAnim_TypeGrass,
    sSpriteAnim_TypeElectric,
    sSpriteAnim_TypePsychic,
    sSpriteAnim_TypeIce,
    sSpriteAnim_TypeDragon,
    sSpriteAnim_TypeDark,
    sSpriteAnim_CategoryCool,
    sSpriteAnim_CategoryBeauty,
    sSpriteAnim_CategoryCute,
    sSpriteAnim_CategorySmart,
    sSpriteAnim_CategoryTough,
};

static const struct CompressedSpriteSheet sSpriteSheet_MoveTypes =
{
    .data = gMoveTypes_Gfx,
    .size = (NUMBER_OF_MON_TYPES + CONTEST_CATEGORIES_COUNT) * 0x100,
    .tag = TAG_MOVE_TYPES
};
static const struct SpriteTemplate sSpriteTemplate_MoveTypes =
{
    .tileTag = TAG_MOVE_TYPES,
    .paletteTag = TAG_MOVE_TYPES,
    .oam = &sOamData_MoveTypes,
    .anims = sSpriteAnimTable_MoveTypes,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};
static const u8 sMoveTypeToOamPaletteNum[NUMBER_OF_MON_TYPES + CONTEST_CATEGORIES_COUNT] =
{
    [TYPE_NORMAL] = 13,
    [TYPE_FIGHTING] = 13,
    [TYPE_FLYING] = 14,
    [TYPE_POISON] = 14,
    [TYPE_GROUND] = 13,
    [TYPE_ROCK] = 13,
    [TYPE_BUG] = 15,
    [TYPE_GHOST] = 14,
    [TYPE_STEEL] = 13,
    [TYPE_MYSTERY] = 15,
    [TYPE_FIRE] = 13,
    [TYPE_WATER] = 14,
    [TYPE_GRASS] = 15,
    [TYPE_ELECTRIC] = 13,
    [TYPE_PSYCHIC] = 14,
    [TYPE_ICE] = 14,
    [TYPE_DRAGON] = 15,
    [TYPE_DARK] = 13,
    [NUMBER_OF_MON_TYPES + CONTEST_CATEGORY_COOL] = 13,
    [NUMBER_OF_MON_TYPES + CONTEST_CATEGORY_BEAUTY] = 14,
    [NUMBER_OF_MON_TYPES + CONTEST_CATEGORY_CUTE] = 14,
    [NUMBER_OF_MON_TYPES + CONTEST_CATEGORY_SMART] = 15,
    [NUMBER_OF_MON_TYPES + CONTEST_CATEGORY_TOUGH] = 13,
};
static const struct OamData sOamData_MoveSelector =
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
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sSpriteAnim_MoveSelector0[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector1[] = {
    ANIMCMD_FRAME(4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector2[] = {
    ANIMCMD_FRAME(8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector3[] = {
    ANIMCMD_FRAME(12, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelectorLeft[] = {
    ANIMCMD_FRAME(16, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelectorRight[] = {
    ANIMCMD_FRAME(16, 0, TRUE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelectorMiddle[] = {
    ANIMCMD_FRAME(20, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector7[] = {
    ANIMCMD_FRAME(24, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector8[] = {
    ANIMCMD_FRAME(24, 0, TRUE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MoveSelector9[] = {
    ANIMCMD_FRAME(28, 0, FALSE, FALSE),
    ANIMCMD_END
};
// All except left, middle and right are unused
static const union AnimCmd *const sSpriteAnimTable_MoveSelector[] = {
    sSpriteAnim_MoveSelector0,
    sSpriteAnim_MoveSelector1,
    sSpriteAnim_MoveSelector2,
    sSpriteAnim_MoveSelector3,
    sSpriteAnim_MoveSelectorLeft,
    sSpriteAnim_MoveSelectorRight,
    sSpriteAnim_MoveSelectorMiddle,
    sSpriteAnim_MoveSelector7,
    sSpriteAnim_MoveSelector8,
    sSpriteAnim_MoveSelector9,
};
static const struct CompressedSpriteSheet sMoveSelectorSpriteSheet =
{
    .data = gSummaryMoveSelect_Gfx,
    .size = 0x400,
    .tag = TAG_MOVE_SELECTOR
};
static const struct CompressedSpritePalette sMoveSelectorSpritePal =
{
    .data = gSummaryMoveSelect_Pal,
    .tag = TAG_MOVE_SELECTOR
};
static const struct SpriteTemplate sMoveSelectorSpriteTemplate =
{
    .tileTag = TAG_MOVE_SELECTOR,
    .paletteTag = TAG_MOVE_SELECTOR,
    .oam = &sOamData_MoveSelector,
    .anims = sSpriteAnimTable_MoveSelector,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};
static const struct OamData sOamData_StatusCondition =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 3,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sSpriteAnim_StatusPoison[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusParalyzed[] = {
    ANIMCMD_FRAME(4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusSleep[] = {
    ANIMCMD_FRAME(8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusFrozen[] = {
    ANIMCMD_FRAME(12, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusBurn[] = {
    ANIMCMD_FRAME(16, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusPokerus[] = {
    ANIMCMD_FRAME(20, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_StatusFaint[] = {
    ANIMCMD_FRAME(24, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd *const sSpriteAnimTable_StatusCondition[] = {
    sSpriteAnim_StatusPoison,
    sSpriteAnim_StatusParalyzed,
    sSpriteAnim_StatusSleep,
    sSpriteAnim_StatusFrozen,
    sSpriteAnim_StatusBurn,
    sSpriteAnim_StatusPokerus,
    sSpriteAnim_StatusFaint,
};
static const struct CompressedSpriteSheet sStatusIconsSpriteSheet =
{
    .data = gStatusGfx_Icons,
    .size = 0x380,
    .tag = TAG_MON_STATUS
};
static const struct CompressedSpritePalette sStatusIconsSpritePalette =
{
    .data = gStatusPal_Icons,
    .tag = TAG_MON_STATUS
};
static const struct SpriteTemplate sSpriteTemplate_StatusCondition =
{
    .tileTag = TAG_MON_STATUS,
    .paletteTag = TAG_MON_STATUS,
    .oam = &sOamData_StatusCondition,
    .anims = sSpriteAnimTable_StatusCondition,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};
static const u16 sMarkings_Pal[] = INCBIN_U16("graphics/summary_screen/markings.gbapal");

// code
void ShowPokemonSummaryScreen(u8 mode, void *mons, u8 monIndex, u8 maxMonIndex, void (*callback)(void))
{
    sMonSummaryScreen = AllocZeroed(sizeof(*sMonSummaryScreen));
    sMonSummaryScreen->mode = mode;
    sMonSummaryScreen->monList.mons = mons;
    sMonSummaryScreen->curMonIndex = monIndex;
    sMonSummaryScreen->maxMonIndex = maxMonIndex;
    sMonSummaryScreen->callback = callback;

    if (mode == SUMMARY_MODE_BOX)
        sMonSummaryScreen->isBoxMon = TRUE;
    else
        sMonSummaryScreen->isBoxMon = FALSE;

    switch (mode)
    {
    case SUMMARY_MODE_NORMAL:
    case SUMMARY_MODE_BOX:
        sMonSummaryScreen->minPageIndex = 0;
        sMonSummaryScreen->maxPageIndex = PSS_PAGE_COUNT - 1;
        break;
    case SUMMARY_MODE_LOCK_MOVES:
        sMonSummaryScreen->minPageIndex = 0;
        sMonSummaryScreen->maxPageIndex = PSS_PAGE_COUNT - 1;
        sMonSummaryScreen->lockMovesFlag = TRUE;
        break;
    case SUMMARY_MODE_SELECT_MOVE:
        sMonSummaryScreen->minPageIndex = PSS_PAGE_BATTLE_MOVES;
        sMonSummaryScreen->maxPageIndex = PSS_PAGE_COUNT - 1;
        sMonSummaryScreen->lockMonFlag = TRUE;
        break;
    }

    sMonSummaryScreen->currPageIndex = sMonSummaryScreen->minPageIndex;
    SummaryScreen_SetAnimDelayTaskId(TASK_NONE);

    if (gMonSpritesGfxPtr == NULL)
        CreateMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A, MON_SPR_GFX_MODE_NORMAL);

    SetMainCallback2(CB2_InitSummaryScreen);
}

void ShowSelectMovePokemonSummaryScreen(struct Pokemon *mons, u8 monIndex, u8 maxMonIndex, void (*callback)(void), u16 newMove)
{
    ShowPokemonSummaryScreen(SUMMARY_MODE_SELECT_MOVE, mons, monIndex, maxMonIndex, callback);
    sMonSummaryScreen->newMove = newMove;
}

void ShowPokemonSummaryScreenHandleDeoxys(u8 mode, struct BoxPokemon *mons, u8 monIndex, u8 maxMonIndex, void (*callback)(void))
{
    ShowPokemonSummaryScreen(mode, mons, monIndex, maxMonIndex, callback);
    sMonSummaryScreen->handleDeoxys = TRUE;
}

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlank(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_InitSummaryScreen(void)
{
    while (MenuHelpers_ShouldWaitForLinkRecv() != TRUE && LoadGraphics() != TRUE && MenuHelpers_IsLinkActive() != TRUE);
}

static bool8 LoadGraphics(void)
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
        InitBGs();
        sMonSummaryScreen->switchCounter = 0;
        gMain.state++;
        break;
    case 6:
        if (DecompressGraphics() != FALSE)
            gMain.state++;
        break;
    case 7:
        ResetWindows();
        gMain.state++;
        break;
    case 8:
        DrawPagination();
        gMain.state++;
        break;
    case 9:
        CopyMonToSummaryStruct(&sMonSummaryScreen->currentMon);
        sMonSummaryScreen->switchCounter = 0;
        gMain.state++;
        break;
    case 10:
        if (ExtractMonDataToSummaryStruct(&sMonSummaryScreen->currentMon) != 0)
            gMain.state++;
        break;
    case 11:
        PrintMonInfo();
        gMain.state++;
        break;
    case 12:
        PrintPageNamesAndStats();
        gMain.state++;
        break;
    case 13:
        PrintPageSpecificText(sMonSummaryScreen->currPageIndex);
        gMain.state++;
        break;
    case 14:
        SetDefaultTilemaps();
        gMain.state++;
        break;
    case 15:
        PutPageWindowTilemaps(sMonSummaryScreen->currPageIndex);
        gMain.state++;
        break;
    case 16:
        ResetSpriteIds();
        CreateMoveTypeIcons();
        sMonSummaryScreen->switchCounter = 0;
        gMain.state++;
        break;
    case 17:
        sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON] = LoadMonGfxAndSprite(&sMonSummaryScreen->currentMon, &sMonSummaryScreen->switchCounter);
        if (sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON] != SPRITE_NONE)
        {
            sMonSummaryScreen->switchCounter = 0;
            gMain.state++;
        }
        break;
    case 18:
        CreateMonMarkingsSprite(&sMonSummaryScreen->currentMon);
        gMain.state++;
        break;
    case 19:
        CreateCaughtBallSprite(&sMonSummaryScreen->currentMon);
        gMain.state++;
        break;
    case 20:
        CreateSetStatusSprite();
        gMain.state++;
        break;
    case 21:
        SetTypeIcons();
        gMain.state++;
        break;
    case 22:
        if (sMonSummaryScreen->mode != SUMMARY_MODE_SELECT_MOVE)
            CreateTask(Task_HandleInput, 0);
        else
            CreateTask(Task_SetHandleReplaceMoveInput, 0);
        gMain.state++;
        break;
    case 23:
        BlendPalettes(PALETTES_ALL, 16, 0);
        gMain.state++;
        break;
    case 24:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = 0;
        gMain.state++;
        break;
    default:
        SetVBlankCallback(VBlank);
        SetMainCallback2(MainCB2);
        return TRUE;
    }
    return FALSE;
}

static void InitBGs(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    SetBgTilemapBuffer(1, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0]);
    SetBgTilemapBuffer(2, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_SKILLS][0]);
    SetBgTilemapBuffer(3, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0]);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static bool8 DecompressGraphics(void)
{
    switch (sMonSummaryScreen->switchCounter)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, &gSummaryScreen_Gfx, 0, 0, 0);
        sMonSummaryScreen->switchCounter++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != 1)
        {
            LZDecompressWram(gSummaryPage_Info_Tilemap, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0]);
            sMonSummaryScreen->switchCounter++;
        }
        break;
    case 2:
        LZDecompressWram(gSummaryPage_InfoEgg_Tilemap, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][1]);
        sMonSummaryScreen->switchCounter++;
        break;
    case 3:
        LZDecompressWram(gSummaryPage_Skills_Tilemap, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_SKILLS][1]);
        sMonSummaryScreen->switchCounter++;
        break;
    case 4:
        LZDecompressWram(gSummaryPage_BattleMoves_Tilemap, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][1]);
        sMonSummaryScreen->switchCounter++;
        break;
    case 5:
        LZDecompressWram(gSummaryPage_ContestMoves_Tilemap, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][1]);
        sMonSummaryScreen->switchCounter++;
        break;
    case 6:
        LoadCompressedPalette(gSummaryScreen_Pal, BG_PLTT_ID(0), 8 * PLTT_SIZE_4BPP);
        LoadPalette(&gPPTextPalette, BG_PLTT_ID(8) + 1, PLTT_SIZEOF(16 - 1));
        sMonSummaryScreen->switchCounter++;
        break;
    case 7:
        LoadCompressedSpriteSheet(&sSpriteSheet_MoveTypes);
        sMonSummaryScreen->switchCounter++;
        break;
    case 8:
        LoadCompressedSpriteSheet(&sMoveSelectorSpriteSheet);
        sMonSummaryScreen->switchCounter++;
        break;
    case 9:
        LoadCompressedSpriteSheet(&sStatusIconsSpriteSheet);
        sMonSummaryScreen->switchCounter++;
        break;
    case 10:
        LoadCompressedSpritePalette(&sStatusIconsSpritePalette);
        sMonSummaryScreen->switchCounter++;
        break;
    case 11:
        LoadCompressedSpritePalette(&sMoveSelectorSpritePal);
        sMonSummaryScreen->switchCounter++;
        break;
    case 12:
        LoadCompressedPalette(gMoveTypes_Pal, OBJ_PLTT_ID(13), 3 * PLTT_SIZE_4BPP);
        sMonSummaryScreen->switchCounter = 0;
        return TRUE;
    }
    return FALSE;
}

static void CopyMonToSummaryStruct(struct Pokemon *mon)
{
    if (!sMonSummaryScreen->isBoxMon)
    {
        struct Pokemon *partyMon = sMonSummaryScreen->monList.mons;
        *mon = partyMon[sMonSummaryScreen->curMonIndex];
    }
    else
    {
        struct BoxPokemon *boxMon = sMonSummaryScreen->monList.boxMons;
        BoxMonToMon(&boxMon[sMonSummaryScreen->curMonIndex], mon);
    }
}

static bool8 ExtractMonDataToSummaryStruct(struct Pokemon *mon)
{
    u32 i;
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    // Spread the data extraction over multiple frames.
    switch (sMonSummaryScreen->switchCounter)
    {
    case 0:
        sum->species = GetMonData(mon, MON_DATA_SPECIES);
        sum->species2 = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
        sum->exp = GetMonData(mon, MON_DATA_EXP);
        sum->level = GetMonData(mon, MON_DATA_LEVEL);
        sum->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM);
        sum->item = GetMonData(mon, MON_DATA_HELD_ITEM);
        sum->pid = GetMonData(mon, MON_DATA_PERSONALITY);
        sum->sanity = GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG);

        if (sum->sanity)
            sum->isEgg = TRUE;
        else
            sum->isEgg = GetMonData(mon, MON_DATA_IS_EGG);

        break;
    case 1:
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            sum->moves[i] = GetMonData(mon, MON_DATA_MOVE1+i);
            sum->pp[i] = GetMonData(mon, MON_DATA_PP1+i);
        }
        sum->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);
        break;
    case 2:
        if (sMonSummaryScreen->monList.mons == gPlayerParty || sMonSummaryScreen->mode == SUMMARY_MODE_BOX || sMonSummaryScreen->handleDeoxys == TRUE)
        {
            sum->nature = GetNature(mon);
            sum->currentHP = GetMonData(mon, MON_DATA_HP);
            sum->maxHP = GetMonData(mon, MON_DATA_MAX_HP);
            sum->atk = GetMonData(mon, MON_DATA_ATK);
            sum->def = GetMonData(mon, MON_DATA_DEF);
            sum->spatk = GetMonData(mon, MON_DATA_SPATK);
            sum->spdef = GetMonData(mon, MON_DATA_SPDEF);
            sum->speed = GetMonData(mon, MON_DATA_SPEED);
        }
        else
        {
            sum->nature = GetNature(mon);
            sum->currentHP = GetMonData(mon, MON_DATA_HP);
            sum->maxHP = GetMonData(mon, MON_DATA_MAX_HP);
            sum->atk = GetMonData(mon, MON_DATA_ATK2);
            sum->def = GetMonData(mon, MON_DATA_DEF2);
            sum->spatk = GetMonData(mon, MON_DATA_SPATK2);
            sum->spdef = GetMonData(mon, MON_DATA_SPDEF2);
            sum->speed = GetMonData(mon, MON_DATA_SPEED2);
        }
        break;
    case 3:
        GetMonData(mon, MON_DATA_OT_NAME, sum->OTName);
        ConvertInternationalString(sum->OTName, GetMonData(mon, MON_DATA_LANGUAGE));
        sum->ailment = GetMonAilment(mon);
        sum->OTGender = GetMonData(mon, MON_DATA_OT_GENDER);
        sum->OTID = GetMonData(mon, MON_DATA_OT_ID);
        sum->metLocation = GetMonData(mon, MON_DATA_MET_LOCATION);
        sum->metLevel = GetMonData(mon, MON_DATA_MET_LEVEL);
        sum->metGame = GetMonData(mon, MON_DATA_MET_GAME);
        sum->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
        break;
    default:
        sum->ribbonCount = GetMonData(mon, MON_DATA_RIBBON_COUNT);
        return TRUE;
    }
    sMonSummaryScreen->switchCounter++;
    return FALSE;
}

static void SetDefaultTilemaps(void)
{
    if (sMonSummaryScreen->currPageIndex != PSS_PAGE_BATTLE_MOVES && sMonSummaryScreen->currPageIndex != PSS_PAGE_CONTEST_MOVES)
    {
        HandlePowerAccTilemap(0, 0xFF);
        HandleAppealJamTilemap(0, 0xFF, 0);
    }
    else
    {
        DrawContestMoveHearts(sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex]);
        TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0], 3, FALSE);
        TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0], 1, FALSE);
        SetBgTilemapBuffer(1, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0]);
        SetBgTilemapBuffer(2, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0]);
        ChangeBgX(2, 0x10000, BG_COORD_ADD);
        ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
    }

    if (sMonSummaryScreen->summary.ailment == AILMENT_NONE)
        HandleStatusTilemap(0, 0xFF);
    else if (sMonSummaryScreen->currPageIndex != PSS_PAGE_BATTLE_MOVES && sMonSummaryScreen->currPageIndex != PSS_PAGE_CONTEST_MOVES)
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);

    LimitEggSummaryPageDisplay();
    DrawPokerusCuredSymbol(&sMonSummaryScreen->currentMon);
}

static void FreeSummaryScreen(void)
{
    FreeAllWindowBuffers();
    Free(sMonSummaryScreen);
}

static void BeginCloseSummaryScreen(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = CloseSummaryScreen;
}

static void CloseSummaryScreen(u8 taskId)
{
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE && !gPaletteFade.active)
    {
        SetMainCallback2(sMonSummaryScreen->callback);
        gLastViewedMonIndex = sMonSummaryScreen->curMonIndex;
        SummaryScreen_DestroyAnimDelayTask();
        ResetSpriteData();
        FreeAllSpritePalettes();
        StopCryAndClearCrySongs();
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        if (gMonSpritesGfxPtr == NULL)
            DestroyMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A);
        FreeSummaryScreen();
        DestroyTask(taskId);
    }
}

static void Task_HandleInput(u8 taskId)
{
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE && !gPaletteFade.active)
    {
        if (JOY_NEW(DPAD_UP))
        {
            ChangeSummaryPokemon(taskId, -1);
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            ChangeSummaryPokemon(taskId, 1);
        }
        else if ((JOY_NEW(DPAD_LEFT)) || GetLRKeysPressed() == MENU_L_PRESSED)
        {
            ChangePage(taskId, -1);
        }
        else if ((JOY_NEW(DPAD_RIGHT)) || GetLRKeysPressed() == MENU_R_PRESSED)
        {
            ChangePage(taskId, 1);
        }
        else if (JOY_NEW(A_BUTTON))
        {
            if (sMonSummaryScreen->currPageIndex != PSS_PAGE_SKILLS)
            {
                if (sMonSummaryScreen->currPageIndex == PSS_PAGE_INFO)
                {
                    StopPokemonAnimations();
                    PlaySE(SE_SELECT);
                    BeginCloseSummaryScreen(taskId);
                }
                else // Contest or Battle Moves
                {
                    PlaySE(SE_SELECT);
                    SwitchToMoveSelection(taskId);
                }
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            StopPokemonAnimations();
            PlaySE(SE_SELECT);
            BeginCloseSummaryScreen(taskId);
        }
    }
}

static void ChangeSummaryPokemon(u8 taskId, s8 delta)
{
    s8 monId;

    if (!sMonSummaryScreen->lockMonFlag)
    {
        if (sMonSummaryScreen->isBoxMon == TRUE)
        {
            if (sMonSummaryScreen->currPageIndex != PSS_PAGE_INFO)
            {
                if (delta == 1)
                    delta = 0;
                else
                    delta = 2;
            }
            else
            {
                if (delta == 1)
                    delta = 1;
                else
                    delta = 3;
            }
            monId = AdvanceStorageMonIndex(sMonSummaryScreen->monList.boxMons, sMonSummaryScreen->curMonIndex, sMonSummaryScreen->maxMonIndex, delta);
        }
        else if (IsMultiBattle() == TRUE)
        {
            monId = AdvanceMultiBattleMonIndex(delta);
        }
        else
        {
            monId = AdvanceMonIndex(delta);
        }

        if (monId != -1)
        {
            PlaySE(SE_SELECT);
            if (sMonSummaryScreen->summary.ailment != AILMENT_NONE)
            {
                SetSpriteInvisibility(SPRITE_ARR_ID_STATUS, TRUE);
                ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
                ScheduleBgCopyTilemapToVram(0);
                HandleStatusTilemap(0, 2);
            }
            sMonSummaryScreen->curMonIndex = monId;
            gTasks[taskId].data[0] = 0;
            gTasks[taskId].func = Task_ChangeSummaryMon;
        }
    }
}

static void Task_ChangeSummaryMon(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (data[0])
    {
    case 0:
        StopCryAndClearCrySongs();
        break;
    case 1:
        SummaryScreen_DestroyAnimDelayTask();
        DestroySpriteAndFreeResources(&gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]]);
        break;
    case 2:
        DestroySpriteAndFreeResources(&gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_BALL]]);
        break;
    case 3:
        CopyMonToSummaryStruct(&sMonSummaryScreen->currentMon);
        sMonSummaryScreen->switchCounter = 0;
        break;
    case 4:
        if (ExtractMonDataToSummaryStruct(&sMonSummaryScreen->currentMon) == FALSE)
            return;
        break;
    case 5:
        RemoveAndCreateMonMarkingsSprite(&sMonSummaryScreen->currentMon);
        break;
    case 6:
        CreateCaughtBallSprite(&sMonSummaryScreen->currentMon);
        break;
    case 7:
        if (sMonSummaryScreen->summary.ailment != AILMENT_NONE)
            HandleStatusTilemap(10, -2);
        DrawPokerusCuredSymbol(&sMonSummaryScreen->currentMon);
        data[1] = 0;
        break;
    case 8:
        sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON] = LoadMonGfxAndSprite(&sMonSummaryScreen->currentMon, &data[1]);
        if (sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON] == SPRITE_NONE)
            return;
        gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].data[2] = 1;
        TryDrawExperienceProgressBar();
        data[1] = 0;
        break;
    case 9:
        SetTypeIcons();
        break;
    case 10:
        PrintMonInfo();
        break;
    case 11:
        PrintPageSpecificText(sMonSummaryScreen->currPageIndex);
        LimitEggSummaryPageDisplay();
        break;
    case 12:
        gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].data[2] = 0;
        break;
    default:
        if (!MenuHelpers_ShouldWaitForLinkRecv() && !FuncIsActiveTask(Task_ShowStatusWindow))
        {
            data[0] = 0;
            gTasks[taskId].func = Task_HandleInput;
        }
        return;
    }
    data[0]++;
}

static s8 AdvanceMonIndex(s8 delta)
{
    struct Pokemon *mon = sMonSummaryScreen->monList.mons;

    if (sMonSummaryScreen->currPageIndex == PSS_PAGE_INFO)
    {
        if (delta == -1 && sMonSummaryScreen->curMonIndex == 0)
            return -1;
        else if (delta == 1 && sMonSummaryScreen->curMonIndex >= sMonSummaryScreen->maxMonIndex)
            return -1;
        else
            return sMonSummaryScreen->curMonIndex + delta;
    }
    else
    {
        s8 index = sMonSummaryScreen->curMonIndex;

        do
        {
            index += delta;
            if (index < 0 || index > sMonSummaryScreen->maxMonIndex)
                return -1;
        } while (GetMonData(&mon[index], MON_DATA_IS_EGG));
        return index;
    }
}

static s8 AdvanceMultiBattleMonIndex(s8 delta)
{
    struct Pokemon *mons = sMonSummaryScreen->monList.mons;
    s8 index, arrId = 0;
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (sMultiBattleOrder[i] == sMonSummaryScreen->curMonIndex)
        {
            arrId = i;
            break;
        }
    }

    while (TRUE)
    {
        const s8 *order = sMultiBattleOrder;

        arrId += delta;
        if (arrId < 0 || arrId >= PARTY_SIZE)
            return -1;
        index = order[arrId];
        if (IsValidToViewInMulti(&mons[index]) == TRUE)
            return index;
    }
}

static bool8 IsValidToViewInMulti(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
        return FALSE;
    else if (sMonSummaryScreen->curMonIndex != 0 || !GetMonData(mon, MON_DATA_IS_EGG))
        return TRUE;
    else
        return FALSE;
}

static void ChangePage(u8 taskId, s8 delta)
{
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    s16 *data = gTasks[taskId].data;

    if (summary->isEgg)
        return;
    else if (delta == -1 && sMonSummaryScreen->currPageIndex == sMonSummaryScreen->minPageIndex)
        return;
    else if (delta == 1 && sMonSummaryScreen->currPageIndex == sMonSummaryScreen->maxPageIndex)
        return;

    PlaySE(SE_SELECT);
    ClearPageWindowTilemaps(sMonSummaryScreen->currPageIndex);
    sMonSummaryScreen->currPageIndex += delta;
    data[0] = 0;
    if (delta == 1)
        SetTaskFuncWithFollowupFunc(taskId, PssScrollRight, gTasks[taskId].func);
    else
        SetTaskFuncWithFollowupFunc(taskId, PssScrollLeft, gTasks[taskId].func);
    CreateTextPrinterTask(sMonSummaryScreen->currPageIndex);
    HidePageSpecificSprites();
}

static void PssScrollRight(u8 taskId) // Scroll right
{
    s16 *data = gTasks[taskId].data;
    if (data[0] == 0)
    {
        if (sMonSummaryScreen->bgDisplayOrder == 0)
        {
            data[1] = 1;
            SetBgAttribute(1, BG_ATTR_PRIORITY, 1);
            SetBgAttribute(2, BG_ATTR_PRIORITY, 2);
            ScheduleBgCopyTilemapToVram(1);
        }
        else
        {
            data[1] = 2;
            SetBgAttribute(2, BG_ATTR_PRIORITY, 1);
            SetBgAttribute(1, BG_ATTR_PRIORITY, 2);
            ScheduleBgCopyTilemapToVram(2);
        }
        ChangeBgX(data[1], 0, BG_COORD_SET);
        SetBgTilemapBuffer(data[1], sMonSummaryScreen->bgTilemapBuffers[sMonSummaryScreen->currPageIndex][0]);
        ShowBg(1);
        ShowBg(2);
    }
    ChangeBgX(data[1], 0x2000, BG_COORD_ADD);
    data[0] += 32;
    if (data[0] > 0xFF)
        gTasks[taskId].func = PssScrollRightEnd;
}

static void PssScrollRightEnd(u8 taskId) // display right
{
    s16 *data = gTasks[taskId].data;
    sMonSummaryScreen->bgDisplayOrder ^= 1;
    data[1] = 0;
    data[0] = 0;
    DrawPagination();
    PutPageWindowTilemaps(sMonSummaryScreen->currPageIndex);
    SetTypeIcons();
    TryDrawExperienceProgressBar();
    SwitchTaskToFollowupFunc(taskId);
}

static void PssScrollLeft(u8 taskId) // Scroll left
{
    s16 *data = gTasks[taskId].data;
    if (data[0] == 0)
    {
        if (sMonSummaryScreen->bgDisplayOrder == 0)
            data[1] = 2;
        else
            data[1] = 1;
        ChangeBgX(data[1], 0x10000, BG_COORD_SET);
    }
    ChangeBgX(data[1], 0x2000, BG_COORD_SUB);
    data[0] += 32;
    if (data[0] > 0xFF)
        gTasks[taskId].func = PssScrollLeftEnd;
}

static void PssScrollLeftEnd(u8 taskId) // display left
{
    s16 *data = gTasks[taskId].data;
    if (sMonSummaryScreen->bgDisplayOrder == 0)
    {
        SetBgAttribute(1, BG_ATTR_PRIORITY, 1);
        SetBgAttribute(2, BG_ATTR_PRIORITY, 2);
        ScheduleBgCopyTilemapToVram(2);
    }
    else
    {
        SetBgAttribute(2, BG_ATTR_PRIORITY, 1);
        SetBgAttribute(1, BG_ATTR_PRIORITY, 2);
        ScheduleBgCopyTilemapToVram(1);
    }
    if (sMonSummaryScreen->currPageIndex > 1)
    {
        SetBgTilemapBuffer(data[1], sMonSummaryScreen->bgTilemapBuffers[sMonSummaryScreen->currPageIndex - 1][0]);
        ChangeBgX(data[1], 0x10000, BG_COORD_SET);
    }
    ShowBg(1);
    ShowBg(2);
    sMonSummaryScreen->bgDisplayOrder ^= 1;
    data[1] = 0;
    data[0] = 0;
    DrawPagination();
    PutPageWindowTilemaps(sMonSummaryScreen->currPageIndex);
    SetTypeIcons();
    TryDrawExperienceProgressBar();
    SwitchTaskToFollowupFunc(taskId);
}

static void TryDrawExperienceProgressBar(void)
{
    if (sMonSummaryScreen->currPageIndex == PSS_PAGE_SKILLS)
        DrawExperienceProgressBar(&sMonSummaryScreen->currentMon);
}

static void SwitchToMoveSelection(u8 taskId)
{
    u16 move;

    sMonSummaryScreen->firstMoveIndex = 0;
    move = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
    ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
    if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
    HandlePowerAccTilemap(9, -3);
    HandleAppealJamTilemap(9, -3, move);
    if (!sMonSummaryScreen->lockMovesFlag)
    {
        ClearWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
        PutWindowTilemap(PSS_LABEL_WINDOW_PROMPT_SWITCH);
    }
    TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0], 3, FALSE);
    TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0], 1, FALSE);
    PrintMoveDetails(move);
    PrintNewMoveDetailsOrCancelText();
    SetNewMoveTypeIcon();
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    CreateMoveSelectorSprites(SPRITE_ARR_ID_MOVE_SELECTOR1);
    gTasks[taskId].func = Task_HandleInput_MoveSelect;
}

static void Task_HandleInput_MoveSelect(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (JOY_NEW(DPAD_UP))
        {
            data[0] = 4;
            ChangeSelectedMove(data, -1, &sMonSummaryScreen->firstMoveIndex);
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            data[0] = 4;
            ChangeSelectedMove(data, 1, &sMonSummaryScreen->firstMoveIndex);
        }
        else if (JOY_NEW(A_BUTTON))
        {
            if (sMonSummaryScreen->lockMovesFlag == TRUE
             || (sMonSummaryScreen->newMove == MOVE_NONE && sMonSummaryScreen->firstMoveIndex == MAX_MON_MOVES))
            {
                PlaySE(SE_SELECT);
                CloseMoveSelectMode(taskId);
            }
            else if (HasMoreThanOneMove() == TRUE)
            {
                PlaySE(SE_SELECT);
                SwitchToMovePositionSwitchMode(taskId);
            }
            else
            {
                PlaySE(SE_FAILURE);
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            CloseMoveSelectMode(taskId);
        }
    }
}

static bool8 HasMoreThanOneMove(void)
{
    u8 i;
    for (i = 1; i < MAX_MON_MOVES; i++)
    {
        if (sMonSummaryScreen->summary.moves[i] != 0)
            return TRUE;
    }
    return FALSE;
}

static void ChangeSelectedMove(s16 *taskData, s8 direction, u8 *moveIndexPtr)
{
    s8 i, newMoveIndex;
    u16 move;

    PlaySE(SE_SELECT);
    newMoveIndex = *moveIndexPtr;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        newMoveIndex += direction;
        if (newMoveIndex > taskData[0])
            newMoveIndex = 0;
        else if (newMoveIndex < 0)
            newMoveIndex = taskData[0];

        if (newMoveIndex == MAX_MON_MOVES)
        {
            move = sMonSummaryScreen->newMove;
            break;
        }
        move = sMonSummaryScreen->summary.moves[newMoveIndex];
        if (move != 0)
            break;
    }
    DrawContestMoveHearts(move);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    PrintMoveDetails(move);
    if ((*moveIndexPtr == MAX_MON_MOVES && sMonSummaryScreen->newMove == MOVE_NONE)
        || taskData[1] == 1)
    {
        ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
        if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
            ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
        ScheduleBgCopyTilemapToVram(0);
        HandlePowerAccTilemap(9, -3);
        HandleAppealJamTilemap(9, -3, move);
    }
    if (*moveIndexPtr != MAX_MON_MOVES
        && newMoveIndex == MAX_MON_MOVES
        && sMonSummaryScreen->newMove == MOVE_NONE)
    {
        ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
        ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
        ScheduleBgCopyTilemapToVram(0);
        HandlePowerAccTilemap(0, 3);
        HandleAppealJamTilemap(0, 3, 0);
    }

    *moveIndexPtr = newMoveIndex;
    // Get rid of the 'flicker' effect(while idle) when scrolling.
    if (moveIndexPtr == &sMonSummaryScreen->firstMoveIndex)
        KeepMoveSelectorVisible(SPRITE_ARR_ID_MOVE_SELECTOR1);
    else
        KeepMoveSelectorVisible(SPRITE_ARR_ID_MOVE_SELECTOR2);
}

static void CloseMoveSelectMode(u8 taskId)
{
    DestroyMoveSelectorSprites(SPRITE_ARR_ID_MOVE_SELECTOR1);
    ClearWindowTilemap(PSS_LABEL_WINDOW_PROMPT_SWITCH);
    PutWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
    PrintMoveDetails(0);
    TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0], 3, TRUE);
    TilemapFiveMovesDisplay(sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0], 1, TRUE);
    AddAndFillMoveNamesWindow(); // This function seems to have no effect.
    if (sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
    {
        ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
        ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
        HandlePowerAccTilemap(0, 3);
        HandleAppealJamTilemap(0, 3, 0);
    }
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_HandleInput;
}

static void SwitchToMovePositionSwitchMode(u8 taskId)
{
    sMonSummaryScreen->secondMoveIndex = sMonSummaryScreen->firstMoveIndex;
    SetMainMoveSelectorColor(1);
    CreateMoveSelectorSprites(SPRITE_ARR_ID_MOVE_SELECTOR2);
    gTasks[taskId].func = Task_HandleInput_MovePositionSwitch;
}

static void Task_HandleInput_MovePositionSwitch(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (JOY_NEW(DPAD_UP))
        {
            data[0] = 3;
            ChangeSelectedMove(&data[0], -1, &sMonSummaryScreen->secondMoveIndex);
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            data[0] = 3;
            ChangeSelectedMove(&data[0], 1, &sMonSummaryScreen->secondMoveIndex);
        }
        else if (JOY_NEW(A_BUTTON))
        {
            if (sMonSummaryScreen->firstMoveIndex == sMonSummaryScreen->secondMoveIndex)
                ExitMovePositionSwitchMode(taskId, FALSE);
            else
                ExitMovePositionSwitchMode(taskId, TRUE);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            ExitMovePositionSwitchMode(taskId, FALSE);
        }
    }
}

static void ExitMovePositionSwitchMode(u8 taskId, bool8 swapMoves)
{
    u16 move;

    PlaySE(SE_SELECT);
    SetMainMoveSelectorColor(0);
    DestroyMoveSelectorSprites(SPRITE_ARR_ID_MOVE_SELECTOR2);

    if (swapMoves == TRUE)
    {
        if (!sMonSummaryScreen->isBoxMon)
        {
            struct Pokemon *mon = sMonSummaryScreen->monList.mons;
            SwapMonMoves(&mon[sMonSummaryScreen->curMonIndex], sMonSummaryScreen->firstMoveIndex, sMonSummaryScreen->secondMoveIndex);
        }
        else
        {
            struct BoxPokemon *boxMon = sMonSummaryScreen->monList.boxMons;
            SwapBoxMonMoves(&boxMon[sMonSummaryScreen->curMonIndex], sMonSummaryScreen->firstMoveIndex, sMonSummaryScreen->secondMoveIndex);
        }
        CopyMonToSummaryStruct(&sMonSummaryScreen->currentMon);
        SwapMovesNamesPP(sMonSummaryScreen->firstMoveIndex, sMonSummaryScreen->secondMoveIndex);
        SwapMovesTypeSprites(sMonSummaryScreen->firstMoveIndex, sMonSummaryScreen->secondMoveIndex);
        sMonSummaryScreen->firstMoveIndex = sMonSummaryScreen->secondMoveIndex;
    }

    move = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
    PrintMoveDetails(move);
    DrawContestMoveHearts(move);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_HandleInput_MoveSelect;
}

static void SwapMonMoves(struct Pokemon *mon, u8 moveIndex1, u8 moveIndex2)
{
    struct PokeSummary* summary = &sMonSummaryScreen->summary;

    u16 move1 = summary->moves[moveIndex1];
    u16 move2 = summary->moves[moveIndex2];
    u8 move1pp = summary->pp[moveIndex1];
    u8 move2pp = summary->pp[moveIndex2];
    u8 ppBonuses = summary->ppBonuses;

    // Calculate PP bonuses
    u8 ppUpMask1 = gPPUpGetMask[moveIndex1];
    u8 ppBonusMove1 = (ppBonuses & ppUpMask1) >> (moveIndex1 * 2);
    u8 ppUpMask2 = gPPUpGetMask[moveIndex2];
    u8 ppBonusMove2 = (ppBonuses & ppUpMask2) >> (moveIndex2 * 2);
    ppBonuses &= ~ppUpMask1;
    ppBonuses &= ~ppUpMask2;
    ppBonuses |= (ppBonusMove1 << (moveIndex2 * 2)) + (ppBonusMove2 << (moveIndex1 * 2));

    // Swap the moves
    SetMonData(mon, MON_DATA_MOVE1 + moveIndex1, &move2);
    SetMonData(mon, MON_DATA_MOVE1 + moveIndex2, &move1);
    SetMonData(mon, MON_DATA_PP1 + moveIndex1, &move2pp);
    SetMonData(mon, MON_DATA_PP1 + moveIndex2, &move1pp);
    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);

    summary->moves[moveIndex1] = move2;
    summary->moves[moveIndex2] = move1;

    summary->pp[moveIndex1] = move2pp;
    summary->pp[moveIndex2] = move1pp;

    summary->ppBonuses = ppBonuses;
}

static void SwapBoxMonMoves(struct BoxPokemon *mon, u8 moveIndex1, u8 moveIndex2)
{
    struct PokeSummary* summary = &sMonSummaryScreen->summary;

    u16 move1 = summary->moves[moveIndex1];
    u16 move2 = summary->moves[moveIndex2];
    u8 move1pp = summary->pp[moveIndex1];
    u8 move2pp = summary->pp[moveIndex2];
    u8 ppBonuses = summary->ppBonuses;

    // Calculate PP bonuses
    u8 ppUpMask1 = gPPUpGetMask[moveIndex1];
    u8 ppBonusMove1 = (ppBonuses & ppUpMask1) >> (moveIndex1 * 2);
    u8 ppUpMask2 = gPPUpGetMask[moveIndex2];
    u8 ppBonusMove2 = (ppBonuses & ppUpMask2) >> (moveIndex2 * 2);
    ppBonuses &= ~ppUpMask1;
    ppBonuses &= ~ppUpMask2;
    ppBonuses |= (ppBonusMove1 << (moveIndex2 * 2)) + (ppBonusMove2 << (moveIndex1 * 2));

    // Swap the moves
    SetBoxMonData(mon, MON_DATA_MOVE1 + moveIndex1, &move2);
    SetBoxMonData(mon, MON_DATA_MOVE1 + moveIndex2, &move1);
    SetBoxMonData(mon, MON_DATA_PP1 + moveIndex1, &move2pp);
    SetBoxMonData(mon, MON_DATA_PP1 + moveIndex2, &move1pp);
    SetBoxMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);

    summary->moves[moveIndex1] = move2;
    summary->moves[moveIndex2] = move1;

    summary->pp[moveIndex1] = move2pp;
    summary->pp[moveIndex2] = move1pp;

    summary->ppBonuses = ppBonuses;
}

static void Task_SetHandleReplaceMoveInput(u8 taskId)
{
    SetNewMoveTypeIcon();
    CreateMoveSelectorSprites(SPRITE_ARR_ID_MOVE_SELECTOR1);
    gTasks[taskId].func = Task_HandleReplaceMoveInput;
}

static void Task_HandleReplaceMoveInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (gPaletteFade.active != TRUE)
        {
            if (JOY_NEW(DPAD_UP))
            {
                data[0] = 4;
                ChangeSelectedMove(data, -1, &sMonSummaryScreen->firstMoveIndex);
            }
            else if (JOY_NEW(DPAD_DOWN))
            {
                data[0] = 4;
                ChangeSelectedMove(data, 1, &sMonSummaryScreen->firstMoveIndex);
            }
            else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
            {
                ChangePage(taskId, -1);
            }
            else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
            {
                ChangePage(taskId, 1);
            }
            else if (JOY_NEW(A_BUTTON))
            {
                if (CanReplaceMove() == TRUE)
                {
                    StopPokemonAnimations();
                    PlaySE(SE_SELECT);
                    sMoveSlotToReplace = sMonSummaryScreen->firstMoveIndex;
                    gSpecialVar_0x8005 = sMoveSlotToReplace;
                    BeginCloseSummaryScreen(taskId);
                }
                else
                {
                    PlaySE(SE_FAILURE);
                    ShowCantForgetHMsWindow(taskId);
                }
            }
            else if (JOY_NEW(B_BUTTON))
            {
                StopPokemonAnimations();
                PlaySE(SE_SELECT);
                sMoveSlotToReplace = MAX_MON_MOVES;
                gSpecialVar_0x8005 = MAX_MON_MOVES;
                BeginCloseSummaryScreen(taskId);
            }
        }
    }
}

static bool8 CanReplaceMove(void)
{
    if (sMonSummaryScreen->firstMoveIndex == MAX_MON_MOVES
        || sMonSummaryScreen->newMove == MOVE_NONE
        || IsMoveHm(sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex]) != TRUE)
        return TRUE;
    else
        return FALSE;
}

static void ShowCantForgetHMsWindow(u8 taskId)
{
    ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
    ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
    ScheduleBgCopyTilemapToVram(0);
    HandlePowerAccTilemap(0, 3);
    HandleAppealJamTilemap(0, 3, 0);
    PrintHMMovesCantBeForgotten();
    gTasks[taskId].func = Task_HandleInputCantForgetHMsMoves;
}

// This redraws the power/accuracy window when the player scrolls out of the "HM Moves can't be forgotten" message
static void Task_HandleInputCantForgetHMsMoves(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 move;
    if (FuncIsActiveTask(Task_ShowPowerAccWindow) != 1)
    {
        if (JOY_NEW(DPAD_UP))
        {
            data[1] = 1;
            data[0] = 4;
            ChangeSelectedMove(&data[0], -1, &sMonSummaryScreen->firstMoveIndex);
            data[1] = 0;
            gTasks[taskId].func = Task_HandleReplaceMoveInput;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            data[1] = 1;
            data[0] = 4;
            ChangeSelectedMove(&data[0], 1, &sMonSummaryScreen->firstMoveIndex);
            data[1] = 0;
            gTasks[taskId].func = Task_HandleReplaceMoveInput;
        }
        else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
        {
            if (sMonSummaryScreen->currPageIndex != PSS_PAGE_BATTLE_MOVES)
            {
                ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
                if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
                    ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
                move = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
                gTasks[taskId].func = Task_HandleReplaceMoveInput;
                ChangePage(taskId, -1);
                HandlePowerAccTilemap(9, -2);
                HandleAppealJamTilemap(9, -2, move);
            }
        }
        else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
        {
            if (sMonSummaryScreen->currPageIndex != PSS_PAGE_CONTEST_MOVES)
            {
                ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
                if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
                    ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
                move = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
                gTasks[taskId].func = Task_HandleReplaceMoveInput;
                ChangePage(taskId, 1);
                HandlePowerAccTilemap(9, -2);
                HandleAppealJamTilemap(9, -2, move);
            }
        }
        else if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
            if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
                ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
            move = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
            PrintMoveDetails(move);
            ScheduleBgCopyTilemapToVram(0);
            HandlePowerAccTilemap(9, -3);
            HandleAppealJamTilemap(9, -3, move);
            gTasks[taskId].func = Task_HandleReplaceMoveInput;
        }
    }
}

u8 GetMoveSlotToReplace(void)
{
    return sMoveSlotToReplace;
}

static void DrawPagination(void) // Updates the pagination dots at the top of the summary screen
{
    u16 *tilemap = Alloc(8 * PSS_PAGE_COUNT);
    u8 i;

    for (i = 0; i < PSS_PAGE_COUNT; i++)
    {
        u8 j = i * 2;

        if (i < sMonSummaryScreen->minPageIndex)
        {
            tilemap[j + 0] = 0x40;
            tilemap[j + 1] = 0x40;
            tilemap[j + 2 * PSS_PAGE_COUNT] = 0x50;
            tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x50;
        }
        else if (i > sMonSummaryScreen->maxPageIndex)
        {
            tilemap[j + 0] = 0x4A;
            tilemap[j + 1] = 0x4A;
            tilemap[j + 2 * PSS_PAGE_COUNT] = 0x5A;
            tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x5A;
        }
        else if (i < sMonSummaryScreen->currPageIndex)
        {
            tilemap[j + 0] = 0x46;
            tilemap[j + 1] = 0x47;
            tilemap[j + 2 * PSS_PAGE_COUNT] = 0x56;
            tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x57;
        }
        else if (i == sMonSummaryScreen->currPageIndex)
        {
            if (i != sMonSummaryScreen->maxPageIndex)
            {
                tilemap[j + 0] = 0x41;
                tilemap[j + 1] = 0x42;
                tilemap[j + 2 * PSS_PAGE_COUNT] = 0x51;
                tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x52;
            }
            else
            {
                tilemap[j + 0] = 0x4B;
                tilemap[j + 1] = 0x4C;
                tilemap[j + 2 * PSS_PAGE_COUNT] = 0x5B;
                tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x5C;
            }
        }
        else if (i != sMonSummaryScreen->maxPageIndex)
        {
            tilemap[j + 0] = 0x43;
            tilemap[j + 1] = 0x44;
            tilemap[j + 2 * PSS_PAGE_COUNT] = 0x53;
            tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x54;
        }
        else
        {
            tilemap[j + 0] = 0x48;
            tilemap[j + 1] = 0x49;
            tilemap[j + 2 * PSS_PAGE_COUNT] = 0x58;
            tilemap[j + 2 * PSS_PAGE_COUNT + 1] = 0x59;
        }
    }
    CopyToBgTilemapBufferRect_ChangePalette(3, tilemap, 11, 0, PSS_PAGE_COUNT * 2, 2, 16);
    ScheduleBgCopyTilemapToVram(3);
    Free(tilemap);
}

static void ChangeTilemap(const struct TilemapCtrl *unkStruct, u16 *dest, u8 c, bool8 d)
{
    u16 i;
    u16 *alloced = Alloc(unkStruct->field_6 * 2 * unkStruct->field_7);
    CpuFill16(unkStruct->field_4, alloced, unkStruct->field_6 * 2 * unkStruct->field_7);
    if (unkStruct->field_6 != c)
    {
        if (!d)
        {
            for (i = 0; i < unkStruct->field_7; i++)
                CpuCopy16(&unkStruct->gfx[c + unkStruct->field_6 * i], &alloced[unkStruct->field_6 * i], (unkStruct->field_6 - c) * 2);
        }
        else
        {
            for (i = 0; i < unkStruct->field_7; i++)
                CpuCopy16(&unkStruct->gfx[unkStruct->field_6 * i], &alloced[c + unkStruct->field_6 * i], (unkStruct->field_6 - c) * 2);
        }
    }

    for (i = 0; i < unkStruct->field_7; i++)
        CpuCopy16(&alloced[unkStruct->field_6 * i], &dest[(unkStruct->field_9 + i) * 32 + unkStruct->field_8], unkStruct->field_6 * 2);

    Free(alloced);
}

static void HandlePowerAccTilemap(u16 a, s16 b)
{
    if (b > sBattleMoveTilemapCtrl.field_6)
        b = sBattleMoveTilemapCtrl.field_6;
    if (b == 0 || b == sBattleMoveTilemapCtrl.field_6)
    {
        ChangeTilemap(&sBattleMoveTilemapCtrl, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0], b, TRUE);
    }
    else
    {
        u8 taskId = FindTaskIdByFunc(Task_ShowPowerAccWindow);
        if (taskId == TASK_NONE)
            taskId = CreateTask(Task_ShowPowerAccWindow, 8);
        gTasks[taskId].data[0] = b;
        gTasks[taskId].data[1] = a;
    }
}

static void Task_ShowPowerAccWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    data[1] += data[0];
    if (data[1] < 0)
    {
        data[1] = 0;
    }
    else if (data[1] > sBattleMoveTilemapCtrl.field_6)
    {
        data[1] = sBattleMoveTilemapCtrl.field_6;
    }
    ChangeTilemap(&sBattleMoveTilemapCtrl, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_BATTLE_MOVES][0], data[1], TRUE);
    if (data[1] <= 0 || data[1] >= sBattleMoveTilemapCtrl.field_6)
    {
        if (data[0] < 0)
        {
            if (sMonSummaryScreen->currPageIndex == PSS_PAGE_BATTLE_MOVES)
                PutWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
        }
        else
        {
            if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
                PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
            PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
        }
        ScheduleBgCopyTilemapToVram(0);
        DestroyTask(taskId);
    }
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
}

static void HandleAppealJamTilemap(u16 a, s16 b, u16 move)
{
    if (b > sContestMoveTilemapCtrl.field_6)
        b = sContestMoveTilemapCtrl.field_6;

    if (b == 0 || b == sContestMoveTilemapCtrl.field_6)
    {
        ChangeTilemap(&sContestMoveTilemapCtrl, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0], b, TRUE);
    }
    else
    {
        u8 taskId = FindTaskIdByFunc(Task_ShowAppealJamWindow);
        if (taskId == TASK_NONE)
            taskId = CreateTask(Task_ShowAppealJamWindow, 8);
        gTasks[taskId].data[0] = b;
        gTasks[taskId].data[1] = a;
        gTasks[taskId].data[2] = move;
    }
}

static void Task_ShowAppealJamWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    data[1] += data[0];
    if (data[1] < 0)
    {
        data[1] = 0;
    }
    else if (data[1] > sContestMoveTilemapCtrl.field_6)
    {
        data[1] = sContestMoveTilemapCtrl.field_6;
    }
    ChangeTilemap(&sContestMoveTilemapCtrl, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][0], data[1], TRUE);
    if (data[1] <= 0 || data[1] >= sContestMoveTilemapCtrl.field_6)
    {
        if (data[0] < 0)
        {
            if (sMonSummaryScreen->currPageIndex == PSS_PAGE_CONTEST_MOVES && FuncIsActiveTask(PssScrollRight) == 0)
                PutWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
            DrawContestMoveHearts(data[2]);
        }
        else
        {
            if (!gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS]].invisible)
            {
                PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
            }
            PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
        }
        ScheduleBgCopyTilemapToVram(0);
        DestroyTask(taskId);
    }
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
}

static void HandleStatusTilemap(u16 a, s16 b)
{
    if (b > sStatusTilemapCtrl1.field_6)
        b = sStatusTilemapCtrl1.field_6;
    if (b == 0 || b == sStatusTilemapCtrl1.field_6)
    {
        ChangeTilemap(&sStatusTilemapCtrl1, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0], b, FALSE);
        ChangeTilemap(&sStatusTilemapCtrl2, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0], b, FALSE);
    }
    else
    {
        u8 taskId = CreateTask(Task_ShowStatusWindow, 8);
        gTasks[taskId].data[0] = b;
        gTasks[taskId].data[1] = a;
    }
}

static void Task_ShowStatusWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    data[1] += data[0];
    if (data[1] < 0)
        data[1] = 0;
    else if (data[1] > sStatusTilemapCtrl1.field_6)
        data[1] = sStatusTilemapCtrl1.field_6;
    ChangeTilemap(&sStatusTilemapCtrl1, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0], data[1], FALSE);
    ChangeTilemap(&sStatusTilemapCtrl2, sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0], data[1], FALSE);
    ScheduleBgCopyTilemapToVram(3);
    if (data[1] <= 0 || data[1] >= sStatusTilemapCtrl1.field_6)
    {
        if (data[0] < 0)
        {
            CreateSetStatusSprite();
            PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS);
            ScheduleBgCopyTilemapToVram(0);
        }
        DestroyTask(taskId);
    }
}

// Toggles the "Cancel" window that appears when selecting a move
static void TilemapFiveMovesDisplay(u16 *dst, u16 palette, bool8 remove)
{
    u16 i, id;

    palette *= 0x1000;
    id = 0x56A;
    if (!remove)
    {
        for (i = 0; i < 20; i++)
        {
            dst[id + i] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i] + palette;
            dst[id + i + 0x20] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i] + palette;
            dst[id + i + 0x40] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i + 20] + palette;
        }
    }
    else // Remove
    {
        for (i = 0; i < 20; i++)
        {
            dst[id + i] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i + 20] + palette;
            dst[id + i + 0x20] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i + 40] + palette;
            dst[id + i + 0x40] = gSummaryScreen_MoveEffect_Cancel_Tilemap[i + 40] + palette;
        }
    }
}

static void DrawPokerusCuredSymbol(struct Pokemon *mon) // This checks if the mon has been cured of pokerus
{
    if (!CheckPartyPokerus(mon, 0) && CheckPartyHasHadPokerus(mon, 0)) // If yes it draws the cured symbol
    {
        sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0][0x223] = 0x2C;
        sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][1][0x223] = 0x2C;
    }
    else
    {
        sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][0][0x223] = 0x81A;
        sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_INFO][1][0x223] = 0x81A;
    }
    ScheduleBgCopyTilemapToVram(3);
}

static void SetMonPicBackgroundPalette(bool8 isMonShiny)
{
    if (!isMonShiny)
        SetBgTilemapPalette(3, 1, 4, 8, 8, 0);
    else
        SetBgTilemapPalette(3, 1, 4, 8, 8, 5);
    ScheduleBgCopyTilemapToVram(3);
}

static void DrawExperienceProgressBar(struct Pokemon *unused)
{
    s64 numExpProgressBarTicks;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    u16 *dst;
    u8 i;

    if (summary->level < MAX_LEVEL)
    {
        u32 expBetweenLevels = gExperienceTables[gSpeciesInfo[summary->species].growthRate][summary->level + 1] - gExperienceTables[gSpeciesInfo[summary->species].growthRate][summary->level];
        u32 expSinceLastLevel = summary->exp - gExperienceTables[gSpeciesInfo[summary->species].growthRate][summary->level];

        // Calculate the number of 1-pixel "ticks" to illuminate in the experience progress bar.
        // There are 8 tiles that make up the bar, and each tile has 8 "ticks". Hence, the numerator
        // is multiplied by 64.
        numExpProgressBarTicks = expSinceLastLevel * 64 / expBetweenLevels;
        if (numExpProgressBarTicks == 0 && expSinceLastLevel != 0)
            numExpProgressBarTicks = 1;
    }
    else
    {
        numExpProgressBarTicks = 0;
    }

    dst = &sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_SKILLS][1][0x255];
    for (i = 0; i < 8; i++)
    {
        if (numExpProgressBarTicks > 7)
            dst[i] = 0x206A;
        else
            dst[i] = 0x2062 + (numExpProgressBarTicks % 8);
        numExpProgressBarTicks -= 8;
        if (numExpProgressBarTicks < 0)
            numExpProgressBarTicks = 0;
    }

    if (GetBgTilemapBuffer(1) == sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_SKILLS][0])
        ScheduleBgCopyTilemapToVram(1);
    else
        ScheduleBgCopyTilemapToVram(2);
}

static void DrawContestMoveHearts(u16 move)
{
    u16 *tilemap = sMonSummaryScreen->bgTilemapBuffers[PSS_PAGE_CONTEST_MOVES][1];
    u8 i;

    if (move != MOVE_NONE)
    {
        // Draw appeal hearts
        u8 effectValue = gContestEffects[gContestMoves[move].effect].appeal;
        if (effectValue != 0xFF)
            effectValue /= 10;

        for (i = 0; i < MAX_CONTEST_MOVE_HEARTS; i++)
        {
            if (effectValue != 0xFF && i < effectValue)
                tilemap[(i / 4 * 32) + (i & 3) + 0x1E6] = TILE_FILLED_APPEAL_HEART;
            else
                tilemap[(i / 4 * 32) + (i & 3) + 0x1E6] = TILE_EMPTY_APPEAL_HEART;
        }

        // Draw jam hearts
        effectValue = gContestEffects[gContestMoves[move].effect].jam;
        if (effectValue != 0xFF)
            effectValue /= 10;

        for (i = 0; i < MAX_CONTEST_MOVE_HEARTS; i++)
        {
            if (effectValue != 0xFF && i < effectValue)
                tilemap[(i / 4 * 32) + (i & 3) + 0x226] = TILE_FILLED_JAM_HEART;
            else
                tilemap[(i / 4 * 32) + (i & 3) + 0x226] = TILE_EMPTY_JAM_HEART;
        }
    }
}

static void LimitEggSummaryPageDisplay(void) // If the Pokémon is an egg, limit the number of pages displayed to 1
{
    if (sMonSummaryScreen->summary.isEgg)
        ChangeBgX(3, 0x10000, BG_COORD_SET);
    else
        ChangeBgX(3, 0, BG_COORD_SET);
}

static void ResetWindows(void)
{
    u8 i;

    InitWindows(sSummaryTemplate);
    DeactivateAllTextPrinters();
    for (i = 0; i < PSS_LABEL_WINDOW_END; i++)
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    for (i = 0; i < ARRAY_COUNT(sMonSummaryScreen->windowIds); i++)
        sMonSummaryScreen->windowIds[i] = WINDOW_NONE;
}

static void PrintTextOnWindow(u8 windowId, const u8 *string, u8 x, u8 y, u8 lineSpacing, u8 colorId)
{
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 0, lineSpacing, sTextColors[colorId], 0, string);
}

static void PrintMonInfo(void)
{
    FillWindowPixelBuffer(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER, PIXEL_FILL(0));
    FillWindowPixelBuffer(PSS_LABEL_WINDOW_PORTRAIT_NICKNAME, PIXEL_FILL(0));
    FillWindowPixelBuffer(PSS_LABEL_WINDOW_PORTRAIT_SPECIES, PIXEL_FILL(0));
    if (!sMonSummaryScreen->summary.isEgg)
        PrintNotEggInfo();
    else
        PrintEggInfo();
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintNotEggInfo(void)
{
    u8 strArray[16];
    struct Pokemon *mon = &sMonSummaryScreen->currentMon;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    u16 dexNum = SpeciesToPokedexNum(summary->species);

    if (dexNum != 0xFFFF)
    {
        StringCopy(gStringVar1, &gText_NumberClear01[0]);
        ConvertIntToDecimalStringN(gStringVar2, dexNum, STR_CONV_MODE_LEADING_ZEROS, 3);
        StringAppend(gStringVar1, gStringVar2);
        if (!IsMonShiny(mon))
        {
            PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER, gStringVar1, 0, 1, 0, 1);
            SetMonPicBackgroundPalette(FALSE);
        }
        else
        {
            PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER, gStringVar1, 0, 1, 0, 7);
            SetMonPicBackgroundPalette(TRUE);
        }
        PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER);
    }
    else
    {
        ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER);
        if (!IsMonShiny(mon))
            SetMonPicBackgroundPalette(FALSE);
        else
            SetMonPicBackgroundPalette(TRUE);
    }
    StringCopy(gStringVar1, gText_LevelSymbol);
    ConvertIntToDecimalStringN(gStringVar2, summary->level, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(gStringVar1, gStringVar2);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_SPECIES, gStringVar1, 24, 17, 0, 1);
    GetMonNickname(mon, gStringVar1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_NICKNAME, gStringVar1, 0, 1, 0, 1);
    strArray[0] = CHAR_SLASH;
    StringCopy(&strArray[1], &gSpeciesNames[summary->species2][0]);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_SPECIES, strArray, 0, 1, 0, 1);
    PrintGenderSymbol(mon, summary->species2);
    PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_NICKNAME);
    PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
}

static void PrintEggInfo(void)
{
    GetMonNickname(&sMonSummaryScreen->currentMon, gStringVar1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_NICKNAME, gStringVar1, 0, 1, 0, 1);
    PutWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_NICKNAME);
    ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_DEX_NUMBER);
    ClearWindowTilemap(PSS_LABEL_WINDOW_PORTRAIT_SPECIES);
}

static void PrintGenderSymbol(struct Pokemon *mon, u16 species)
{
    if (species != SPECIES_NIDORAN_M && species != SPECIES_NIDORAN_F)
    {
        switch (GetMonGender(mon))
        {
        case MON_MALE:
            PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_SPECIES, gText_MaleSymbol, 57, 17, 0, 3);
            break;
        case MON_FEMALE:
            PrintTextOnWindow(PSS_LABEL_WINDOW_PORTRAIT_SPECIES, gText_FemaleSymbol, 57, 17, 0, 4);
            break;
        }
    }
}

static void PrintAOrBButtonIcon(u8 windowId, bool8 bButton, u32 x)
{
    const u8 *button;
    if (!bButton)
        button = sButtons_Gfx[0];
    else
        button = sButtons_Gfx[1];

    BlitBitmapToWindow(windowId, button, x, 0, 16, 16);
}

static void PrintPageNamesAndStats(void)
{
    int stringXPos;
    int iconXPos;
    int statsXPos;

    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_INFO_TITLE, gText_PkmnInfo, 2, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_TITLE, gText_PkmnSkills, 2, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_BATTLE_MOVES_TITLE, gText_BattleMoves, 2, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_CONTEST_MOVES_TITLE, gText_ContestMoves, 2, 1, 0, 1);

    stringXPos = GetStringRightAlignXOffset(FONT_NORMAL, gText_Cancel2, 62);
    iconXPos = stringXPos - 16;
    if (iconXPos < 0)
        iconXPos = 0;
    PrintAOrBButtonIcon(PSS_LABEL_WINDOW_PROMPT_CANCEL, FALSE, iconXPos);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PROMPT_CANCEL, gText_Cancel2, stringXPos, 1, 0, 0);

    stringXPos = GetStringRightAlignXOffset(FONT_NORMAL, gText_Info, 62);
    iconXPos = stringXPos - 16;
    if (iconXPos < 0)
        iconXPos = 0;
    PrintAOrBButtonIcon(PSS_LABEL_WINDOW_PROMPT_INFO, FALSE, iconXPos);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PROMPT_INFO, gText_Info, stringXPos, 1, 0, 0);

    stringXPos = GetStringRightAlignXOffset(FONT_NORMAL, gText_Switch, 62);
    iconXPos = stringXPos - 16;
    if (iconXPos < 0)
        iconXPos = 0;
    PrintAOrBButtonIcon(PSS_LABEL_WINDOW_PROMPT_SWITCH, FALSE, iconXPos);
    PrintTextOnWindow(PSS_LABEL_WINDOW_PROMPT_SWITCH, gText_Switch, stringXPos, 1, 0, 0);

    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_INFO_RENTAL, gText_RentalPkmn, 0, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_INFO_TYPE, gText_TypeSlash, 0, 1, 0, 0);
    statsXPos = 6 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_HP4, 42);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT, gText_HP4, statsXPos, 1, 0, 1);
    statsXPos = 6 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_Attack3, 42);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT, gText_Attack3, statsXPos, 17, 0, 1);
    statsXPos = 6 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_Defense3, 42);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT, gText_Defense3, statsXPos, 33, 0, 1);
    statsXPos = 2 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_SpAtk4, 36);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT, gText_SpAtk4, statsXPos, 1, 0, 1);
    statsXPos = 2 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_SpDef4, 36);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT, gText_SpDef4, statsXPos, 17, 0, 1);
    statsXPos = 2 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_Speed2, 36);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT, gText_Speed2, statsXPos, 33, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP, gText_ExpPoints, 6, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP, gText_NextLv, 6, 17, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATUS, gText_Status, 2, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_POWER_ACC, gText_Power, 0, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_POWER_ACC, gText_Accuracy2, 0, 17, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM, gText_Appeal, 0, 1, 0, 1);
    PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM, gText_Jam, 0, 17, 0, 1);
}

static void PutPageWindowTilemaps(u8 page)
{
    u8 i;

    ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_TITLE);
    ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_TITLE);
    ClearWindowTilemap(PSS_LABEL_WINDOW_BATTLE_MOVES_TITLE);
    ClearWindowTilemap(PSS_LABEL_WINDOW_CONTEST_MOVES_TITLE);

    switch (page)
    {
    case PSS_PAGE_INFO:
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_TITLE);
        PutWindowTilemap(PSS_LABEL_WINDOW_PROMPT_CANCEL);
        if (InBattleFactory() == TRUE || InSlateportBattleTent() == TRUE)
            PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_RENTAL);
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_TYPE);
        break;
    case PSS_PAGE_SKILLS:
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_TITLE);
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT);
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT);
        PutWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP);
        break;
    case PSS_PAGE_BATTLE_MOVES:
        PutWindowTilemap(PSS_LABEL_WINDOW_BATTLE_MOVES_TITLE);
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                PutWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
        }
        else
        {
            PutWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
        }
        break;
    case PSS_PAGE_CONTEST_MOVES:
        PutWindowTilemap(PSS_LABEL_WINDOW_CONTEST_MOVES_TITLE);
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                PutWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
        }
        else
        {
            PutWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
        }
        break;
    }

    for (i = 0; i < ARRAY_COUNT(sMonSummaryScreen->windowIds); i++)
        PutWindowTilemap(sMonSummaryScreen->windowIds[i]);

    ScheduleBgCopyTilemapToVram(0);
}

static void ClearPageWindowTilemaps(u8 page)
{
    u8 i;

    switch (page)
    {
    case PSS_PAGE_INFO:
        ClearWindowTilemap(PSS_LABEL_WINDOW_PROMPT_CANCEL);
        if (InBattleFactory() == TRUE || InSlateportBattleTent() == TRUE)
            ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_RENTAL);
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_INFO_TYPE);
        break;
    case PSS_PAGE_SKILLS:
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_LEFT);
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_STATS_RIGHT);
        ClearWindowTilemap(PSS_LABEL_WINDOW_POKEMON_SKILLS_EXP);
        break;
    case PSS_PAGE_BATTLE_MOVES:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_POWER_ACC);
        }
        else
        {
            ClearWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
        }
        break;
    case PSS_PAGE_CONTEST_MOVES:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                ClearWindowTilemap(PSS_LABEL_WINDOW_MOVES_APPEAL_JAM);
        }
        else
        {
            ClearWindowTilemap(PSS_LABEL_WINDOW_PROMPT_INFO);
        }
        break;
    }

    for (i = 0; i < ARRAY_COUNT(sMonSummaryScreen->windowIds); i++)
        RemoveWindowByIndex(i);

    ScheduleBgCopyTilemapToVram(0);
}

static u8 AddWindowFromTemplateList(const struct WindowTemplate *template, u8 templateId)
{
    u8 *windowIdPtr = &sMonSummaryScreen->windowIds[templateId];
    if (*windowIdPtr == WINDOW_NONE)
    {
        *windowIdPtr = AddWindow(&template[templateId]);
        FillWindowPixelBuffer(*windowIdPtr, PIXEL_FILL(0));
    }
    return *windowIdPtr;
}

static void RemoveWindowByIndex(u8 windowIndex)
{
    u8 *windowIdPtr = &sMonSummaryScreen->windowIds[windowIndex];
    if (*windowIdPtr != WINDOW_NONE)
    {
        ClearWindowTilemap(*windowIdPtr);
        RemoveWindow(*windowIdPtr);
        *windowIdPtr = WINDOW_NONE;
    }
}

static void PrintPageSpecificText(u8 pageIndex)
{
    u16 i;
    for (i = 0; i < ARRAY_COUNT(sMonSummaryScreen->windowIds); i++)
    {
        if (sMonSummaryScreen->windowIds[i] != WINDOW_NONE)
            FillWindowPixelBuffer(sMonSummaryScreen->windowIds[i], PIXEL_FILL(0));
    }
    sTextPrinterFunctions[pageIndex]();
}

static void CreateTextPrinterTask(u8 pageIndex)
{
    CreateTask(sTextPrinterTasks[pageIndex], 16);
}

static void PrintInfoPageText(void)
{
    if (sMonSummaryScreen->summary.isEgg)
    {
        PrintEggOTName();
        PrintEggOTID();
        PrintEggState();
        PrintEggMemo();
    }
    else
    {
        PrintMonOTName();
        PrintMonOTID();
        PrintMonAbilityName();
        PrintMonAbilityDescription();
        BufferMonTrainerMemo();
        PrintMonTrainerMemo();
    }
}

static void Task_PrintInfoPage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (data[0])
    {
    case 1:
        PrintMonOTName();
        break;
    case 2:
        PrintMonOTID();
        break;
    case 3:
        PrintMonAbilityName();
        break;
    case 4:
        PrintMonAbilityDescription();
        break;
    case 5:
        BufferMonTrainerMemo();
        break;
    case 6:
        PrintMonTrainerMemo();
        break;
    case 7:
        DestroyTask(taskId);
        return;
    }
    data[0]++;
}

static void PrintMonOTName(void)
{
    int x, windowId;
    if (InBattleFactory() != TRUE && InSlateportBattleTent() != TRUE)
    {
        windowId = AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ORIGINAL_TRAINER);
        PrintTextOnWindow(windowId, gText_OTSlash, 0, 1, 0, 1);
        x = GetStringWidth(FONT_NORMAL, gText_OTSlash, 0);
        if (sMonSummaryScreen->summary.OTGender == 0)
            PrintTextOnWindow(windowId, sMonSummaryScreen->summary.OTName, x, 1, 0, 5);
        else
            PrintTextOnWindow(windowId, sMonSummaryScreen->summary.OTName, x, 1, 0, 6);
    }
}

static void PrintMonOTID(void)
{
    int xPos;
    if (InBattleFactory() != TRUE && InSlateportBattleTent() != TRUE)
    {
        ConvertIntToDecimalStringN(StringCopy(gStringVar1, gText_IDNumber2), (u16)sMonSummaryScreen->summary.OTID, STR_CONV_MODE_LEADING_ZEROS, 5);
        xPos = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar1, 56);
        PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ID), gStringVar1, xPos, 1, 0, 1);
    }
}

static void PrintMonAbilityName(void)
{
    u8 ability = GetAbilityBySpecies(sMonSummaryScreen->summary.species, sMonSummaryScreen->summary.abilityNum);
    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ABILITY), gAbilityNames[ability], 0, 1, 0, 1);
}

static void PrintMonAbilityDescription(void)
{
    u8 ability = GetAbilityBySpecies(sMonSummaryScreen->summary.species, sMonSummaryScreen->summary.abilityNum);
    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ABILITY), gAbilityDescriptionPointers[ability], 0, 17, 0, 0);
}

static void BufferMonTrainerMemo(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    const u8 *text;

    DynamicPlaceholderTextUtil_Reset();
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, sMemoNatureTextColor);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, sMemoMiscTextColor);
    BufferNatureString();

    if (InBattleFactory() == TRUE || InSlateportBattleTent() == TRUE || IsInGamePartnerMon() == TRUE)
    {
        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, gText_XNature);
    }
    else
    {
        u8 *metLevelString = Alloc(32);
        u8 *metLocationString = Alloc(32);
        GetMetLevelString(metLevelString);

        if (sum->metLocation < MAPSEC_NONE)
        {
            GetMapNameHandleAquaHideout(metLocationString, sum->metLocation);
            DynamicPlaceholderTextUtil_SetPlaceholderPtr(4, metLocationString);
        }

        if (DoesMonOTMatchOwner() == TRUE)
        {
            if (sum->metLevel == 0)
                text = (sum->metLocation >= MAPSEC_NONE) ? gText_XNatureHatchedSomewhereAt : gText_XNatureHatchedAtYZ;
            else
                text = (sum->metLocation >= MAPSEC_NONE) ? gText_XNatureMetSomewhereAt : gText_XNatureMetAtYZ;
        }
        else if (sum->metLocation == METLOC_FATEFUL_ENCOUNTER)
        {
            text = gText_XNatureFatefulEncounter;
        }
        else if (sum->metLocation != METLOC_IN_GAME_TRADE && DidMonComeFromGBAGames())
        {
            text = (sum->metLocation >= MAPSEC_NONE) ? gText_XNatureObtainedInTrade : gText_XNatureProbablyMetAt;
        }
        else
        {
            text = gText_XNatureObtainedInTrade;
        }

        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, text);
        Free(metLevelString);
        Free(metLocationString);
    }
}

static void PrintMonTrainerMemo(void)
{
    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_MEMO), gStringVarBuffer, 0, 1, 0, 0);
}

static void BufferNatureString(void)
{
    struct PokemonSummaryScreenData *sumStruct = sMonSummaryScreen;
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(2, gNatureNamePointers[sumStruct->summary.nature]);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(5, gText_EmptyString5);
}

static void GetMetLevelString(u8 *output)
{
    u8 level = sMonSummaryScreen->summary.metLevel;
    if (level == 0)
        level = EGG_HATCH_LEVEL;
    ConvertIntToDecimalStringN(output, level, STR_CONV_MODE_LEFT_ALIGN, 3);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(3, output);
}

static bool8 DoesMonOTMatchOwner(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    u32 trainerId;
    u8 gender;

    if (sMonSummaryScreen->monList.mons == gEnemyParty)
    {
        u8 multiID = GetMultiplayerId() ^ 1;
        trainerId = gLinkPlayers[multiID].trainerId & 0xFFFF;
        gender = gLinkPlayers[multiID].gender;
        StringCopy(gStringVar1, gLinkPlayers[multiID].name);
    }
    else
    {
        trainerId = GetPlayerIDAsU32() & 0xFFFF;
        gender = gSaveBlock2Ptr->playerGender;
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    }

    if (gender != sum->OTGender || trainerId != (sum->OTID & 0xFFFF) || StringCompareWithoutExtCtrlCodes(gStringVar1, sum->OTName))
        return FALSE;
    else
        return TRUE;
}

static bool8 DidMonComeFromGBAGames(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    if (sum->metGame > 0 && sum->metGame <= VERSION_LEAF_GREEN)
        return TRUE;
    return FALSE;
}

bool8 DidMonComeFromRSE(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    if (sum->metGame > 0 && sum->metGame <= VERSION_EMERALD)
        return TRUE;
    return FALSE;
}

static bool8 IsInGamePartnerMon(void)
{
    if ((gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) && gMain.inBattle)
    {
        if (sMonSummaryScreen->curMonIndex == 1 || sMonSummaryScreen->curMonIndex == 4 || sMonSummaryScreen->curMonIndex == 5)
            return TRUE;
    }
    return FALSE;
}

static void PrintEggOTName(void)
{
    u32 windowId = AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ORIGINAL_TRAINER);
    u32 width = GetStringWidth(FONT_NORMAL, gText_OTSlash, 0);
    PrintTextOnWindow(windowId, gText_OTSlash, 0, 1, 0, 1);
    PrintTextOnWindow(windowId, gText_FiveMarks, width, 1, 0, 1);
}

static void PrintEggOTID(void)
{
    int x;
    StringCopy(gStringVar1, gText_IDNumber2);
    StringAppend(gStringVar1, gText_FiveMarks);
    x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar1, 56);
    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ID), gStringVar1, x, 1, 0, 1);
}

static void PrintEggState(void)
{
    const u8 *text;
    struct PokeSummary *sum = &sMonSummaryScreen->summary;

    if (sMonSummaryScreen->summary.sanity == TRUE)
        text = gText_EggWillTakeALongTime;
    else if (sum->friendship <= 5)
        text = gText_EggAboutToHatch;
    else if (sum->friendship <= 10)
        text = gText_EggWillHatchSoon;
    else if (sum->friendship <= 40)
        text = gText_EggWillTakeSomeTime;
    else
        text = gText_EggWillTakeALongTime;

    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_ABILITY), text, 0, 1, 0, 0);
}

static void PrintEggMemo(void)
{
    const u8 *text;
    struct PokeSummary *sum = &sMonSummaryScreen->summary;

    if (sMonSummaryScreen->summary.sanity != 1)
    {
        if (sum->metLocation == METLOC_FATEFUL_ENCOUNTER)
            text = gText_PeculiarEggNicePlace;
        else if (DidMonComeFromGBAGames() == FALSE || DoesMonOTMatchOwner() == FALSE)
            text = gText_PeculiarEggTrade;
        else if (sum->metLocation == METLOC_SPECIAL_EGG)
            text = (DidMonComeFromRSE() == TRUE) ? gText_EggFromHotSprings : gText_EggFromTraveler;
        else
            text = gText_OddEggFoundByCouple;
    }
    else
    {
        text = gText_OddEggFoundByCouple;
    }

    PrintTextOnWindow(AddWindowFromTemplateList(sPageInfoTemplate, PSS_DATA_WINDOW_INFO_MEMO), text, 0, 1, 0, 0);
}

static void PrintSkillsPageText(void)
{
    PrintHeldItemName();
    PrintRibbonCount();
    BufferLeftColumnStats();
    PrintLeftColumnStats();
    BufferRightColumnStats();
    PrintRightColumnStats();
    PrintExpPointsNextLevel();
}

static void Task_PrintSkillsPage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (data[0])
    {
    case 1:
        PrintHeldItemName();
        break;
    case 2:
        PrintRibbonCount();
        break;
    case 3:
        BufferLeftColumnStats();
        break;
    case 4:
        PrintLeftColumnStats();
        break;
    case 5:
        BufferRightColumnStats();
        break;
    case 6:
        PrintRightColumnStats();
        break;
    case 7:
        PrintExpPointsNextLevel();
        break;
    case 8:
        DestroyTask(taskId);
        return;
    }
    data[0]++;
}

static void PrintHeldItemName(void)
{
    const u8 *text;
    int x;

    if (sMonSummaryScreen->summary.item == ITEM_ENIGMA_BERRY
        && IsMultiBattle() == TRUE
        && (sMonSummaryScreen->curMonIndex == 1 || sMonSummaryScreen->curMonIndex == 4 || sMonSummaryScreen->curMonIndex == 5))
    {
        text = ItemId_GetName(ITEM_ENIGMA_BERRY);
    }
    else if (sMonSummaryScreen->summary.item == ITEM_NONE)
    {
        text = gText_None;
    }
    else
    {
        CopyItemName(sMonSummaryScreen->summary.item, gStringVar1);
        text = gStringVar1;
    }

    x = GetStringCenterAlignXOffset(FONT_NORMAL, text, 72) + 6;
    PrintTextOnWindow(AddWindowFromTemplateList(sPageSkillsTemplate, PSS_DATA_WINDOW_SKILLS_HELD_ITEM), text, x, 1, 0, 0);
}

static void PrintRibbonCount(void)
{
    const u8 *text;
    int x;

    if (sMonSummaryScreen->summary.ribbonCount == 0)
    {
        text = gText_None;
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar1, sMonSummaryScreen->summary.ribbonCount, STR_CONV_MODE_RIGHT_ALIGN, 2);
        StringExpandPlaceholders(gStringVarBuffer, gText_RibbonsVar1);
        text = gStringVarBuffer;
    }

    x = GetStringCenterAlignXOffset(FONT_NORMAL, text, 70) + 6;
    PrintTextOnWindow(AddWindowFromTemplateList(sPageSkillsTemplate, PSS_DATA_WINDOW_SKILLS_RIBBON_COUNT), text, x, 1, 0, 0);
}

static void BufferLeftColumnStats(void)
{
    u8 *currentHPString = Alloc(8);
    u8 *maxHPString = Alloc(8);
    u8 *attackString = Alloc(8);
    u8 *defenseString = Alloc(8);

    ConvertIntToDecimalStringN(currentHPString, sMonSummaryScreen->summary.currentHP, STR_CONV_MODE_RIGHT_ALIGN, 3);
    ConvertIntToDecimalStringN(maxHPString, sMonSummaryScreen->summary.maxHP, STR_CONV_MODE_RIGHT_ALIGN, 3);
    ConvertIntToDecimalStringN(attackString, sMonSummaryScreen->summary.atk, STR_CONV_MODE_RIGHT_ALIGN, 7);
    ConvertIntToDecimalStringN(defenseString, sMonSummaryScreen->summary.def, STR_CONV_MODE_RIGHT_ALIGN, 7);

    DynamicPlaceholderTextUtil_Reset();
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, currentHPString);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, maxHPString);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(2, attackString);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(3, defenseString);
    DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, sStatsLeftColumnLayout);

    Free(currentHPString);
    Free(maxHPString);
    Free(attackString);
    Free(defenseString);
}

static void PrintLeftColumnStats(void)
{
    PrintTextOnWindow(AddWindowFromTemplateList(sPageSkillsTemplate, PSS_DATA_WINDOW_SKILLS_STATS_LEFT), gStringVarBuffer, 4, 1, 0, 0);
}

static void BufferRightColumnStats(void)
{
    ConvertIntToDecimalStringN(gStringVar1, sMonSummaryScreen->summary.spatk, STR_CONV_MODE_RIGHT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, sMonSummaryScreen->summary.spdef, STR_CONV_MODE_RIGHT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, sMonSummaryScreen->summary.speed, STR_CONV_MODE_RIGHT_ALIGN, 3);

    DynamicPlaceholderTextUtil_Reset();
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, gStringVar2);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(2, gStringVar3);
    DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, sStatsRightColumnLayout);
}

static void PrintRightColumnStats(void)
{
    PrintTextOnWindow(AddWindowFromTemplateList(sPageSkillsTemplate, PSS_DATA_WINDOW_SKILLS_STATS_RIGHT), gStringVarBuffer, 2, 1, 0, 0);
}

static void PrintExpPointsNextLevel(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    u8 windowId = AddWindowFromTemplateList(sPageSkillsTemplate, PSS_DATA_WINDOW_EXP);
    int x;
    u32 expToNextLevel;

    ConvertIntToDecimalStringN(gStringVar1, sum->exp, STR_CONV_MODE_RIGHT_ALIGN, 7);
    x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar1, 42) + 2;
    PrintTextOnWindow(windowId, gStringVar1, x, 1, 0, 0);

    if (sum->level < MAX_LEVEL)
        expToNextLevel = gExperienceTables[gSpeciesInfo[sum->species].growthRate][sum->level + 1] - sum->exp;
    else
        expToNextLevel = 0;

    ConvertIntToDecimalStringN(gStringVar1, expToNextLevel, STR_CONV_MODE_RIGHT_ALIGN, 6);
    x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar1, 42) + 2;
    PrintTextOnWindow(windowId, gStringVar1, x, 17, 0, 0);
}

static void PrintBattleMoves(void)
{
    PrintMoveNameAndPP(0);
    PrintMoveNameAndPP(1);
    PrintMoveNameAndPP(2);
    PrintMoveNameAndPP(3);

    if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
    {
        PrintNewMoveDetailsOrCancelText();
        if (sMonSummaryScreen->firstMoveIndex == MAX_MON_MOVES)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE)
                PrintMoveDetails(sMonSummaryScreen->newMove);
        }
        else
        {
            PrintMoveDetails(sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex]);
        }
    }
}

static void Task_PrintBattleMoves(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (data[0])
    {
    case 1:
        PrintMoveNameAndPP(0);
        break;
    case 2:
        PrintMoveNameAndPP(1);
        break;
    case 3:
        PrintMoveNameAndPP(2);
        break;
    case 4:
        PrintMoveNameAndPP(3);
        break;
    case 5:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
            PrintNewMoveDetailsOrCancelText();
        break;
    case 6:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->firstMoveIndex == MAX_MON_MOVES)
                data[1] = sMonSummaryScreen->newMove;
            else
                data[1] = sMonSummaryScreen->summary.moves[sMonSummaryScreen->firstMoveIndex];
        }
        break;
    case 7:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                PrintMoveDetails(data[1]);
        }
        break;
    case 8:
        DestroyTask(taskId);
        return;
    }
    data[0]++;
}

static void PrintMoveNameAndPP(u8 moveIndex)
{
    u8 pp;
    int ppState, x;
    const u8 *text;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    u8 moveNameWindowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_NAMES);
    u8 ppValueWindowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_PP);
    u16 move = summary->moves[moveIndex];

    if (move != 0)
    {
        pp = CalculatePPWithBonus(move, summary->ppBonuses, moveIndex);
        PrintTextOnWindow(moveNameWindowId, gMoveNames[move], 0, moveIndex * 16 + 1, 0, 1);
        ConvertIntToDecimalStringN(gStringVar1, summary->pp[moveIndex], STR_CONV_MODE_RIGHT_ALIGN, 2);
        ConvertIntToDecimalStringN(gStringVar2, pp, STR_CONV_MODE_RIGHT_ALIGN, 2);
        DynamicPlaceholderTextUtil_Reset();
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, gStringVar2);
        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, sMovesPPLayout);
        text = gStringVarBuffer;
        ppState = GetCurrentPpToMaxPpState(summary->pp[moveIndex], pp) + 9;
        x = GetStringRightAlignXOffset(FONT_NORMAL, text, 44);
    }
    else
    {
        PrintTextOnWindow(moveNameWindowId, gText_OneDash, 0, moveIndex * 16 + 1, 0, 1);
        text = gText_TwoDashes;
        ppState = 12;
        x = GetStringCenterAlignXOffset(FONT_NORMAL, text, 44);
    }

    PrintTextOnWindow(ppValueWindowId, text, x, moveIndex * 16 + 1, 0, ppState);
}

static void PrintMovePowerAndAccuracy(u16 moveIndex)
{
    const u8 *text;
    if (moveIndex != 0)
    {
        FillWindowPixelRect(PSS_LABEL_WINDOW_MOVES_POWER_ACC, PIXEL_FILL(0), 53, 0, 19, 32);

        if (gBattleMoves[moveIndex].power < 2)
        {
            text = gText_ThreeDashes;
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[moveIndex].power, STR_CONV_MODE_RIGHT_ALIGN, 3);
            text = gStringVar1;
        }

        PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_POWER_ACC, text, 53, 1, 0, 0);

        if (gBattleMoves[moveIndex].accuracy == 0)
        {
            text = gText_ThreeDashes;
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[moveIndex].accuracy, STR_CONV_MODE_RIGHT_ALIGN, 3);
            text = gStringVar1;
        }

        PrintTextOnWindow(PSS_LABEL_WINDOW_MOVES_POWER_ACC, text, 53, 17, 0, 0);
    }
}

static void PrintContestMoves(void)
{
    PrintMoveNameAndPP(0);
    PrintMoveNameAndPP(1);
    PrintMoveNameAndPP(2);
    PrintMoveNameAndPP(3);

    if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
    {
        PrintNewMoveDetailsOrCancelText();
        PrintContestMoveDescription(sMonSummaryScreen->firstMoveIndex);
    }
}

static void Task_PrintContestMoves(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (data[0])
    {
    case 1:
        PrintMoveNameAndPP(0);
        break;
    case 2:
        PrintMoveNameAndPP(1);
        break;
    case 3:
        PrintMoveNameAndPP(2);
        break;
    case 4:
        PrintMoveNameAndPP(3);
        break;
    case 5:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
            PrintNewMoveDetailsOrCancelText();
        break;
    case 6:
        if (sMonSummaryScreen->mode == SUMMARY_MODE_SELECT_MOVE)
        {
            if (sMonSummaryScreen->newMove != MOVE_NONE || sMonSummaryScreen->firstMoveIndex != MAX_MON_MOVES)
                PrintContestMoveDescription(sMonSummaryScreen->firstMoveIndex);
        }
        break;
    case 7:
        DestroyTask(taskId);
        return;
    }
    data[0]++;
}

static void PrintContestMoveDescription(u8 moveSlot)
{
    u16 move;

    if (moveSlot == MAX_MON_MOVES)
        move = sMonSummaryScreen->newMove;
    else
        move = sMonSummaryScreen->summary.moves[moveSlot];

    if (move != MOVE_NONE)
    {
        u8 windowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_DESCRIPTION);
        PrintTextOnWindow(windowId, gContestEffectDescriptionPointers[gContestMoves[move].effect], 6, 1, 0, 0);
    }
}

static void PrintMoveDetails(u16 move)
{
    u8 windowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_DESCRIPTION);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    if (move != MOVE_NONE)
    {
        if (sMonSummaryScreen->currPageIndex == PSS_PAGE_BATTLE_MOVES)
        {
            PrintMovePowerAndAccuracy(move);
            PrintTextOnWindow(windowId, gMoveDescriptionPointers[move - 1], 6, 1, 0, 0);
        }
        else
        {
            PrintTextOnWindow(windowId, gContestEffectDescriptionPointers[gContestMoves[move].effect], 6, 1, 0, 0);
        }
        PutWindowTilemap(windowId);
    }
    else
    {
        ClearWindowTilemap(windowId);
    }

    ScheduleBgCopyTilemapToVram(0);
}

static void PrintNewMoveDetailsOrCancelText(void)
{
    u8 windowId1 = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_NAMES);
    u8 windowId2 = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_PP);

    if (sMonSummaryScreen->newMove == MOVE_NONE)
    {
        PrintTextOnWindow(windowId1, gText_Cancel, 0, 65, 0, 1);
    }
    else
    {
        u16 move = sMonSummaryScreen->newMove;

        if (sMonSummaryScreen->currPageIndex == PSS_PAGE_BATTLE_MOVES)
            PrintTextOnWindow(windowId1, gMoveNames[move], 0, 65, 0, 6);
        else
            PrintTextOnWindow(windowId1, gMoveNames[move], 0, 65, 0, 5);

        ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[move].pp, STR_CONV_MODE_RIGHT_ALIGN, 2);
        DynamicPlaceholderTextUtil_Reset();
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, gStringVar1);
        DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVarBuffer, sMovesPPLayout);
        PrintTextOnWindow(windowId2, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NORMAL, gStringVarBuffer, 44), 65, 0, 12);
    }
}

static void AddAndFillMoveNamesWindow(void)
{
    u8 windowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_NAMES);
    FillWindowPixelRect(windowId, PIXEL_FILL(0), 0, 66, 72, 16);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void SwapMovesNamesPP(u8 moveIndex1, u8 moveIndex2)
{
    u8 windowId1 = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_NAMES);
    u8 windowId2 = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_PP);

    FillWindowPixelRect(windowId1, PIXEL_FILL(0), 0, moveIndex1 * 16, 72, 16);
    FillWindowPixelRect(windowId1, PIXEL_FILL(0), 0, moveIndex2 * 16, 72, 16);

    FillWindowPixelRect(windowId2, PIXEL_FILL(0), 0, moveIndex1 * 16, 48, 16);
    FillWindowPixelRect(windowId2, PIXEL_FILL(0), 0, moveIndex2 * 16, 48, 16);

    PrintMoveNameAndPP(moveIndex1);
    PrintMoveNameAndPP(moveIndex2);
}

static void PrintHMMovesCantBeForgotten(void)
{
    u8 windowId = AddWindowFromTemplateList(sPageMovesTemplate, PSS_DATA_WINDOW_MOVE_DESCRIPTION);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    PrintTextOnWindow(windowId, gText_HMMovesCantBeForgotten2, 6, 1, 0, 0);
}

static void ResetSpriteIds(void)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sMonSummaryScreen->spriteIds); i++)
        sMonSummaryScreen->spriteIds[i] = SPRITE_NONE;
}

static void DestroySpriteInArray(u8 spriteArrayId)
{
    if (sMonSummaryScreen->spriteIds[spriteArrayId] != SPRITE_NONE)
    {
        DestroySprite(&gSprites[sMonSummaryScreen->spriteIds[spriteArrayId]]);
        sMonSummaryScreen->spriteIds[spriteArrayId] = SPRITE_NONE;
    }
}

static void SetSpriteInvisibility(u8 spriteArrayId, bool8 invisible)
{
    gSprites[sMonSummaryScreen->spriteIds[spriteArrayId]].invisible = invisible;
}

static void HidePageSpecificSprites(void)
{
    // Keeps Pok�mon, caught ball and status sprites visible.
    u8 i;

    for (i = SPRITE_ARR_ID_TYPE; i < ARRAY_COUNT(sMonSummaryScreen->spriteIds); i++)
    {
        if (sMonSummaryScreen->spriteIds[i] != SPRITE_NONE)
            SetSpriteInvisibility(i, TRUE);
    }
}

static void SetTypeIcons(void)
{
    switch (sMonSummaryScreen->currPageIndex)
    {
    case PSS_PAGE_INFO:
        SetMonTypeIcons();
        break;
    case PSS_PAGE_BATTLE_MOVES:
        SetMoveTypeIcons();
        SetNewMoveTypeIcon();
        break;
    case PSS_PAGE_CONTEST_MOVES:
        SetContestMoveTypeIcons();
        SetNewMoveTypeIcon();
        break;
    }
}

static void CreateMoveTypeIcons(void)
{
    u8 i;

    for (i = SPRITE_ARR_ID_TYPE; i < SPRITE_ARR_ID_TYPE + TYPE_ICON_SPRITE_COUNT; i++)
    {
        if (sMonSummaryScreen->spriteIds[i] == SPRITE_NONE)
            sMonSummaryScreen->spriteIds[i] = CreateSprite(&sSpriteTemplate_MoveTypes, 0, 0, 2);

        SetSpriteInvisibility(i, TRUE);
    }
}

static void SetTypeSpritePosAndPal(u8 typeId, u8 x, u8 y, u8 spriteArrayId)
{
    struct Sprite *sprite = &gSprites[sMonSummaryScreen->spriteIds[spriteArrayId]];
    StartSpriteAnim(sprite, typeId);
    sprite->oam.paletteNum = sMoveTypeToOamPaletteNum[typeId];
    sprite->x = x + 16;
    sprite->y = y + 8;
    SetSpriteInvisibility(spriteArrayId, FALSE);
}

static void SetMonTypeIcons(void)
{
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    if (summary->isEgg)
    {
        SetTypeSpritePosAndPal(TYPE_MYSTERY, 120, 48, SPRITE_ARR_ID_TYPE);
        SetSpriteInvisibility(SPRITE_ARR_ID_TYPE + 1, TRUE);
    }
    else
    {
        SetTypeSpritePosAndPal(gSpeciesInfo[summary->species].types[0], 120, 48, SPRITE_ARR_ID_TYPE);
        if (gSpeciesInfo[summary->species].types[0] != gSpeciesInfo[summary->species].types[1])
        {
            SetTypeSpritePosAndPal(gSpeciesInfo[summary->species].types[1], 160, 48, SPRITE_ARR_ID_TYPE + 1);
            SetSpriteInvisibility(SPRITE_ARR_ID_TYPE + 1, FALSE);
        }
        else
        {
            SetSpriteInvisibility(SPRITE_ARR_ID_TYPE + 1, TRUE);
        }
    }
}

static void SetMoveTypeIcons(void)
{
    u8 i;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (summary->moves[i] != MOVE_NONE)
            SetTypeSpritePosAndPal(gBattleMoves[summary->moves[i]].type, 85, 32 + (i * 16), i + SPRITE_ARR_ID_TYPE);
        else
            SetSpriteInvisibility(i + SPRITE_ARR_ID_TYPE, TRUE);
    }
}

static void SetContestMoveTypeIcons(void)
{
    u8 i;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (summary->moves[i] != MOVE_NONE)
            SetTypeSpritePosAndPal(NUMBER_OF_MON_TYPES + gContestMoves[summary->moves[i]].contestCategory, 85, 32 + (i * 16), i + SPRITE_ARR_ID_TYPE);
        else
            SetSpriteInvisibility(i + SPRITE_ARR_ID_TYPE, TRUE);
    }
}

static void SetNewMoveTypeIcon(void)
{
    if (sMonSummaryScreen->newMove == MOVE_NONE)
    {
        SetSpriteInvisibility(SPRITE_ARR_ID_TYPE + 4, TRUE);
    }
    else
    {
        if (sMonSummaryScreen->currPageIndex == PSS_PAGE_BATTLE_MOVES)
            SetTypeSpritePosAndPal(gBattleMoves[sMonSummaryScreen->newMove].type, 85, 96, SPRITE_ARR_ID_TYPE + 4);
        else
            SetTypeSpritePosAndPal(NUMBER_OF_MON_TYPES + gContestMoves[sMonSummaryScreen->newMove].contestCategory, 85, 96, SPRITE_ARR_ID_TYPE + 4);
    }
}

static void SwapMovesTypeSprites(u8 moveIndex1, u8 moveIndex2)
{
    struct Sprite *sprite1 = &gSprites[sMonSummaryScreen->spriteIds[moveIndex1 + SPRITE_ARR_ID_TYPE]];
    struct Sprite *sprite2 = &gSprites[sMonSummaryScreen->spriteIds[moveIndex2 + SPRITE_ARR_ID_TYPE]];

    u8 temp = sprite1->animNum;
    sprite1->animNum = sprite2->animNum;
    sprite2->animNum = temp;

    temp = sprite1->oam.paletteNum;
    sprite1->oam.paletteNum = sprite2->oam.paletteNum;
    sprite2->oam.paletteNum = temp;

    sprite1->animBeginning = TRUE;
    sprite1->animEnded = FALSE;
    sprite2->animBeginning = TRUE;
    sprite2->animEnded = FALSE;
}

static u8 LoadMonGfxAndSprite(struct Pokemon *mon, s16 *state)
{
    const struct CompressedSpritePalette *pal;
    struct PokeSummary *summary = &sMonSummaryScreen->summary;

    switch (*state)
    {
    default:
        return CreateMonSprite(mon);
    case 0:
        if (gMain.inBattle)
        {
            if (ShouldIgnoreDeoxysForm(3, sMonSummaryScreen->curMonIndex))
                HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[summary->species2],
                                                          gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                                                          summary->species2,
                                                          summary->pid);
            else
                HandleLoadSpecialPokePic_2(&gMonFrontPicTable[summary->species2],
                                           gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                                           summary->species2,
                                           summary->pid);
        }
        else
        {
            if (gMonSpritesGfxPtr != NULL)
            {
                if (sMonSummaryScreen->monList.mons == gPlayerParty || sMonSummaryScreen->mode == SUMMARY_MODE_BOX || sMonSummaryScreen->handleDeoxys == TRUE)
                    HandleLoadSpecialPokePic_2(&gMonFrontPicTable[summary->species2],
                                               gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                                               summary->species2,
                                               summary->pid);
                else
                    HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[summary->species2],
                                                              gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                                                              summary->species2,
                                                              summary->pid);
            }
            else
            {
                if (sMonSummaryScreen->monList.mons == gPlayerParty || sMonSummaryScreen->mode == SUMMARY_MODE_BOX || sMonSummaryScreen->handleDeoxys == TRUE)
                    HandleLoadSpecialPokePic_2(&gMonFrontPicTable[summary->species2],
                                                MonSpritesGfxManager_GetSpritePtr(MON_SPR_GFX_MANAGER_A, B_POSITION_OPPONENT_LEFT),
                                                summary->species2,
                                                summary->pid);
                else
                    HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[summary->species2],
                                                              MonSpritesGfxManager_GetSpritePtr(MON_SPR_GFX_MANAGER_A, B_POSITION_OPPONENT_LEFT),
                                                              summary->species2,
                                                              summary->pid);
            }
        }
        (*state)++;
        return 0xFF;
    case 1:
        pal = GetMonSpritePalStructFromOtIdPersonality(summary->species2, summary->OTID, summary->pid);
        LoadCompressedSpritePalette(pal);
        SetMultiuseSpriteTemplateToPokemon(pal->tag, B_POSITION_OPPONENT_LEFT);
        (*state)++;
        return 0xFF;
    }
}

static void PlayMonCry(void)
{
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    if (!summary->isEgg)
    {
        if (ShouldPlayNormalMonCry(&sMonSummaryScreen->currentMon) == TRUE)
            PlayCry_ByMode(summary->species2, 0, CRY_MODE_NORMAL);
        else
            PlayCry_ByMode(summary->species2, 0, CRY_MODE_WEAK);
    }
}

static u8 CreateMonSprite(struct Pokemon *unused)
{
    struct PokeSummary *summary = &sMonSummaryScreen->summary;
    u8 spriteId = CreateSprite(&gMultiuseSpriteTemplate, 40, 64, 5);

    FreeSpriteOamMatrix(&gSprites[spriteId]);
    gSprites[spriteId].data[0] = summary->species2;
    gSprites[spriteId].data[2] = 0;
    gSprites[spriteId].callback = SpriteCB_Pokemon;
    gSprites[spriteId].oam.priority = 0;

    if (!IsMonSpriteNotFlipped(summary->species2))
        gSprites[spriteId].hFlip = TRUE;
    else
        gSprites[spriteId].hFlip = FALSE;

    return spriteId;
}

static void SpriteCB_Pokemon(struct Sprite *sprite)
{
    struct PokeSummary *summary = &sMonSummaryScreen->summary;

    if (!gPaletteFade.active && sprite->data[2] != 1)
    {
        sprite->data[1] = IsMonSpriteNotFlipped(sprite->data[0]);
        PlayMonCry();
        PokemonSummaryDoMonAnimation(sprite, sprite->data[0], summary->isEgg);
    }
}

// Track and then destroy Task_PokemonSummaryAnimateAfterDelay
// Normally destroys itself but it can be interrupted before the animation starts
void SummaryScreen_SetAnimDelayTaskId(u8 taskId)
{
    sAnimDelayTaskId = taskId;
}

static void SummaryScreen_DestroyAnimDelayTask(void)
{
    if (sAnimDelayTaskId != TASK_NONE)
    {
        DestroyTask(sAnimDelayTaskId);
        sAnimDelayTaskId = TASK_NONE;
    }
}

static bool32 UNUSED IsMonAnimationFinished(void)
{
    if (gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].callback == SpriteCallbackDummy)
        return FALSE;
    else
        return TRUE;
}

static void StopPokemonAnimations(void)  // A subtle effect, this function stops Pokémon animations when leaving the PSS
{
    u16 i;
    u16 paletteIndex;

    gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].animPaused = TRUE;
    gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].callback = SpriteCallbackDummy;
    StopPokemonAnimationDelayTask();

    paletteIndex = OBJ_PLTT_ID(gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MON]].oam.paletteNum);

    for (i = 0; i < 16; i++)
    {
        u16 id = i + paletteIndex;
        gPlttBufferUnfaded[id] = gPlttBufferFaded[id];
    }
}

static void CreateMonMarkingsSprite(struct Pokemon *mon)
{
    struct Sprite *sprite = CreateMonMarkingAllCombosSprite(TAG_MON_MARKINGS, TAG_MON_MARKINGS, sMarkings_Pal);

    sMonSummaryScreen->markingsSprite = sprite;
    if (sprite != NULL)
    {
        StartSpriteAnim(sprite, GetMonData(mon, MON_DATA_MARKINGS));
        sMonSummaryScreen->markingsSprite->x = 60;
        sMonSummaryScreen->markingsSprite->y = 26;
        sMonSummaryScreen->markingsSprite->oam.priority = 1;
    }
}

static void RemoveAndCreateMonMarkingsSprite(struct Pokemon *mon)
{
    DestroySprite(sMonSummaryScreen->markingsSprite);
    FreeSpriteTilesByTag(TAG_MON_MARKINGS);
    CreateMonMarkingsSprite(mon);
}

static void CreateCaughtBallSprite(struct Pokemon *mon)
{
    u8 ball = ItemIdToBallId(GetMonData(mon, MON_DATA_POKEBALL));

    LoadBallGfx(ball);
    sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_BALL] = CreateSprite(&gBallSpriteTemplates[ball], 16, 136, 0);
    gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_BALL]].callback = SpriteCallbackDummy;
    gSprites[sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_BALL]].oam.priority = 3;
}

static void CreateSetStatusSprite(void)
{
    u8 *spriteId = &sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_STATUS];
    u8 statusAnim;

    if (*spriteId == SPRITE_NONE)
        *spriteId = CreateSprite(&sSpriteTemplate_StatusCondition, 64, 152, 0);

    statusAnim = GetMonAilment(&sMonSummaryScreen->currentMon);
    if (statusAnim != 0)
    {
        StartSpriteAnim(&gSprites[*spriteId], statusAnim - 1);
        SetSpriteInvisibility(SPRITE_ARR_ID_STATUS, FALSE);
    }
    else
    {
        SetSpriteInvisibility(SPRITE_ARR_ID_STATUS, TRUE);
    }
}

static void CreateMoveSelectorSprites(u8 idArrayStart)
{
    u8 i;
    u8 *spriteIds = &sMonSummaryScreen->spriteIds[idArrayStart];

    if (sMonSummaryScreen->currPageIndex >= PSS_PAGE_BATTLE_MOVES)
    {
        u8 subpriority = 0;
        if (idArrayStart == SPRITE_ARR_ID_MOVE_SELECTOR1)
            subpriority = 1;

        for (i = 0; i < MOVE_SELECTOR_SPRITES_COUNT; i++)
        {
            spriteIds[i] = CreateSprite(&sMoveSelectorSpriteTemplate, i * 16 + 89, 40, subpriority);
            if (i == 0)
                StartSpriteAnim(&gSprites[spriteIds[i]], 4); // left
            else if (i == 9)
                StartSpriteAnim(&gSprites[spriteIds[i]], 5); // right, actually the same as left, but flipped
            else
                StartSpriteAnim(&gSprites[spriteIds[i]], 6); // middle

            gSprites[spriteIds[i]].callback = SpriteCB_MoveSelector;
            gSprites[spriteIds[i]].data[0] = idArrayStart;
            gSprites[spriteIds[i]].data[1] = 0;
        }
    }
}

static void SpriteCB_MoveSelector(struct Sprite *sprite)
{
    if (sprite->animNum > 3 && sprite->animNum < 7)
    {
        sprite->data[1] = (sprite->data[1] + 1) & 0x1F;
        if (sprite->data[1] > 24)
            sprite->invisible = TRUE;
        else
            sprite->invisible = FALSE;
    }
    else
    {
        sprite->data[1] = 0;
        sprite->invisible = FALSE;
    }

    if (sprite->data[0] == SPRITE_ARR_ID_MOVE_SELECTOR1)
        sprite->y2 = sMonSummaryScreen->firstMoveIndex * 16;
    else
        sprite->y2 = sMonSummaryScreen->secondMoveIndex * 16;
}

static void DestroyMoveSelectorSprites(u8 firstArrayId)
{
    u8 i;
    for (i = 0; i < MOVE_SELECTOR_SPRITES_COUNT; i++)
        DestroySpriteInArray(firstArrayId + i);
}

static void SetMainMoveSelectorColor(u8 which)
{
    u8 i;
    u8 *spriteIds = &sMonSummaryScreen->spriteIds[SPRITE_ARR_ID_MOVE_SELECTOR1];

    which *= 3;
    for (i = 0; i < MOVE_SELECTOR_SPRITES_COUNT; i++)
    {
        if (i == 0)
            StartSpriteAnim(&gSprites[spriteIds[i]], which + 4);
        else if (i == 9)
            StartSpriteAnim(&gSprites[spriteIds[i]], which + 5);
        else
            StartSpriteAnim(&gSprites[spriteIds[i]], which + 6);
    }
}

static void KeepMoveSelectorVisible(u8 firstSpriteId)
{
    u8 i;
    u8 *spriteIds = &sMonSummaryScreen->spriteIds[firstSpriteId];

    for (i = 0; i < MOVE_SELECTOR_SPRITES_COUNT; i++)
    {
        gSprites[spriteIds[i]].data[1] = 0;
        gSprites[spriteIds[i]].invisible = FALSE;
    }
}
