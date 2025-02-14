#include "global.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "event_data.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "save.h"
#include "strings.h"
#include "fieldmap.h"
#include "party_menu.h"
#include "palette.h"
#include "field_screen_effect.h"
#include "field_message_box.h"
#include "random.h"
#include "item.h"
#include "util.h"
#include "sound.h"
#include "task.h"
#include "start_menu.h"
#include "string_util.h"
#include "trainer_see.h"
#include "main.h"
#include "load_save.h"
#include "script.h"
#include "malloc.h"
#include "overworld.h"
#include "event_scripts.h"
#include "graphics.h"
#include "constants/battle_frontier.h"
#include "constants/battle_pyramid.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/frontier_util.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/metatile_labels.h"
#include "constants/moves.h"
#include "constants/trainers.h"

#define NUM_LAYOUT_OFFSETS 8

extern const struct MapLayout *const gMapLayouts[];

struct PyramidWildMon
{
    u16 species;
    u8 lvl;
    u8 abilityNum;
    u16 moves[MAX_MON_MOVES];
};

struct PyramidFloorTemplate
{
    u8 numItems;
    u8 numTrainers;
    u8 itemPositions;
    u8 trainerPositions;
    u8 runMultiplier;
    u8 layoutOffsets[NUM_LAYOUT_OFFSETS];
};

struct PyramidTrainerEncounterMusic
{
    u8 trainerClass;
    u8 trainerEncounterMusic;
};

// This file's functions.
static void InitPyramidChallenge(void);
static void GetBattlePyramidData(void);
static void SetBattlePyramidData(void);
static void SavePyramidChallenge(void);
static void SetBattlePyramidPrize(void);
static void GiveBattlePyramidPrize(void);
static void SeedPyramidFloor(void);
static void SetPickupItem(void);
static void HidePyramidItem(void);
static void SetPyramidFacilityTrainers(void);
static void ShowPostBattleHintText(void);
static void UpdatePyramidWinStreak(void);
static void GetCurrentBattlePyramidLocation(void);
static void UpdatePyramidLightRadius(void);
static void ClearPyramidPartyHeldItems(void);
static void SetPyramidFloorPalette(void);
static void BattlePyramidStartMenu(void);
static void RestorePyramidPlayerParty(void);
static void InitPyramidBagItems(u8);
static u8 GetPyramidFloorTemplateId(void);
static u8 GetPostBattleDirectionHintTextIndex(int *, u8, u8);
static void Task_SetPyramidFloorPalette(u8);
static void MarkPyramidTrainerAsBattled(u16);
static void GetPyramidFloorLayoutOffsets(u8 *);
static void GetPyramidEntranceAndExitSquareIds(u8 *, u8 *);
static void SetPyramidObjectPositionsUniformly(u8);
static bool8 SetPyramidObjectPositionsInAndNearSquare(u8, u8);
static bool8 SetPyramidObjectPositionsNearSquare(u8, u8);
static bool8 TrySetPyramidObjectEventPositionInSquare(u8, u8 *, u8, u8);
static bool8 TrySetPyramidObjectEventPositionAtCoords(bool8, u8, u8, u8 *, u8, u8);

// Const rom data.
#define ABILITY_RANDOM 2 // For wild mons data.

#include "data/battle_frontier/battle_pyramid_level_50_wild_mons.h"
#include "data/battle_frontier/battle_pyramid_open_level_wild_mons.h"

static const struct PyramidFloorTemplate sPyramidFloorTemplates[] =
{
    {
        .numItems = 7,
        .numTrainers = 3,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 128,
        .layoutOffsets = {0, 0, 1, 1, 2, 2, 3, 3},
    },
    {
        .numItems = 6,
        .numTrainers = 3,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 128,
        .layoutOffsets = {1, 1, 2, 2, 3, 3, 4, 4},
    },
    {
        .numItems = 5,
        .numTrainers = 3,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 120,
        .layoutOffsets = {2, 2, 3, 3, 4, 4, 5, 5},
    },
    {
        .numItems = 4,
        .numTrainers = 4,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 120,
        .layoutOffsets = {3, 3, 4, 4, 5, 5, 6, 6},
    },
    {
        .numItems = 4,
        .numTrainers = 4,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_IN_AND_NEAR_ENTRANCE,
        .runMultiplier = 112,
        .layoutOffsets = {4, 4, 5, 5, 6, 6, 7, 7},
    },
    {
        .numItems = 3,
        .numTrainers = 5,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_IN_AND_NEAR_EXIT,
        .runMultiplier = 112,
        .layoutOffsets = {5, 6, 7, 8, 9, 10, 11, 12},
    },
    {
        .numItems = 3,
        .numTrainers = 5,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 104,
        .layoutOffsets = {6, 7, 8, 9, 10, 11, 12, 13},
    },
    {
        .numItems = 2,
        .numTrainers = 4,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_IN_AND_NEAR_ENTRANCE,
        .runMultiplier = 104,
        .layoutOffsets = {7, 8, 9, 10, 11, 12, 13, 14},
    },
    {
        .numItems = 4,
        .numTrainers = 5,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_IN_AND_NEAR_EXIT,
        .runMultiplier = 96,
        .layoutOffsets = {8, 9, 10, 11, 12, 13, 14, 15},
    },
    {
        .numItems = 3,
        .numTrainers = 6,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_NEAR_EXIT,
        .runMultiplier = 96,
        .layoutOffsets = {8, 9, 10, 11, 12, 13, 14, 15},
    },
    {
        .numItems = 2,
        .numTrainers = 3,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 88,
        .layoutOffsets = {12, 13, 14, 12, 13, 14, 12, 13},
    },
    {
        .numItems = 4,
        .numTrainers = 5,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 88,
        .layoutOffsets = {11, 11, 11, 11, 11, 11, 11, 11},
    },
    {
        .numItems = 3,
        .numTrainers = 7,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 80,
        .layoutOffsets = {12, 12, 12, 12, 12, 12, 12, 12},
    },
    {
        .numItems = 2,
        .numTrainers = 4,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 80,
        .layoutOffsets = {13, 13, 13, 13, 13, 13, 13, 13},
    },
    {
        .numItems = 3,
        .numTrainers = 6,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 80,
        .layoutOffsets = {14, 14, 14, 14, 14, 14, 14, 14},
    },
    {
        .numItems = 3,
        .numTrainers = 8,
        .itemPositions = OBJ_POSITIONS_UNIFORM,
        .trainerPositions = OBJ_POSITIONS_UNIFORM,
        .runMultiplier = 80,
        .layoutOffsets = {15, 15, 15, 15, 15, 15, 15, 15},
    }
};

static const u8 sPyramidFloorTemplateOptions[][2] =
{
    // Floor 0
    {40,  0},
    {70,  1},
    {90,  2},
    {100, 3},
    // Floor 1
    {35,  1},
    {55,  2},
    {75,  3},
    {90,  4},
    {100, 10},
    // Floor 2
    {35,  2},
    {55,  3},
    {75,  4},
    {90,  5},
    {100, 11},
    // Floor 3
    {35,  3},
    {55,  4},
    {75,  5},
    {90,  6},
    {100, 12},
    // Floor 4
    {35,  4},
    {55,  5},
    {75,  6},
    {90,  7},
    {100, 13},
    // Floor 5
    {35,  5},
    {55,  6},
    {75,  7},
    {90,  8},
    {100, 14},
    // Floor 6
    {35,  6},
    {55,  7},
    {75,  8},
    {90,  9},
    {100, 15}
};

static const u8 sFloorTemplateOffsets[FRONTIER_STAGES_PER_CHALLENGE] =
{
    0, 4, 9, 14, 19, 24, 29
};

static const u16 sPickupItemsLvl50[TOTAL_PYRAMID_ROUNDS][PICKUP_ITEMS_PER_ROUND] =
{
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_CHERI_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_PECHA_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_RAWST_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPEED, ITEM_CHESTO_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_ACCURACY, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPECIAL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_GUARD_SPEC, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPEED, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_ACCURACY, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPECIAL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_GUARD_SPEC, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
};

static const u16 sPickupItemsLvlOpen[TOTAL_PYRAMID_ROUNDS][PICKUP_ITEMS_PER_ROUND] =
{
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_CHERI_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_PECHA_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_RAWST_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPEED, ITEM_CHESTO_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_ACCURACY, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPECIAL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_GUARD_SPEC, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPEED, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_ACCURACY, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_SPECIAL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_GUARD_SPEC, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_FLUFFY_TAIL, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_BRIGHT_POWDER, ITEM_SHELL_BELL, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_DIRE_HIT, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_LEFTOVERS, ITEM_CHOICE_BAND, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
    {ITEM_HYPER_POTION, ITEM_X_ATTACK, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LUM_BERRY, ITEM_REVIVE, ITEM_SCOPE_LENS, ITEM_FOCUS_BAND, ITEM_MAX_REVIVE, ITEM_SACRED_ASH},
    {ITEM_HYPER_POTION, ITEM_X_DEFEND, ITEM_LUM_BERRY, ITEM_ETHER, ITEM_LEPPA_BERRY, ITEM_REVIVE, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE, ITEM_MAX_ELIXIR},
};

static const u8 sPickupItemSlots[][2] =
{
    // Floor 0
    { 31, 0},
    { 46, 1},
    { 61, 2},
    { 71, 3},
    { 81, 4},
    { 91, 5},
    { 94, 6},
    { 97, 7},
    {100, 8},
    // Floor 1
    { 15, 0},
    { 46, 1},
    { 61, 2},
    { 71, 3},
    { 81, 4},
    { 91, 5},
    { 94, 6},
    { 97, 8},
    {100, 9},
    // Floor 2
    { 15, 0},
    { 30, 1},
    { 61, 2},
    { 71, 3},
    { 81, 4},
    { 91, 5},
    { 94, 6},
    { 97, 7},
    {100, 8},
    // Floor 3
    { 28, 0},
    { 43, 1},
    { 58, 2},
    { 68, 3},
    { 78, 4},
    { 88, 5},
    { 92, 7},
    { 96, 8},
    {100, 9},
    // Floor 4
    { 15, 0},
    { 43, 1},
    { 58, 2},
    { 68, 3},
    { 78, 4},
    { 88, 5},
    { 92, 6},
    { 96, 7},
    {100, 9},
    // Floor 5
    { 15, 0},
    { 30, 1},
    { 58, 2},
    { 68, 3},
    { 78, 4},
    { 88, 5},
    { 92, 6},
    { 96, 7},
    {100, 8},
    // Floor 6
    { 28, 0},
    { 43, 1},
    { 58, 2},
    { 68, 3},
    { 78, 4},
    { 88, 5},
    { 92, 6},
    { 96, 8},
    {100, 9},
};

static const u8 sPickupItemOffsets[FRONTIER_STAGES_PER_CHALLENGE] = {0, 9, 18, 27, 36, 45, 54};

static const struct PyramidTrainerEncounterMusic sTrainerClassEncounterMusic[54] =
{
    {TRAINER_CLASS_TEAM_AQUA, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AQUA_ADMIN, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AQUA_LEADER, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AROMA_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_BATTLE_GIRL, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SWIMMER_F, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_POKEFAN, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_DRAGON_TAMER, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_COOLTRAINER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_GUITARIST, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SAILOR, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TWINS, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_INTERVIEWER, TRAINER_ENCOUNTER_MUSIC_INTERVIEWER},
    {TRAINER_CLASS_RUIN_MANIAC, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_GENTLEMAN, TRAINER_ENCOUNTER_MUSIC_RICH},
    {TRAINER_CLASS_SWIMMER_M, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_POKEMANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_BLACK_BELT, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_OLD_COUPLE, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_BUG_MANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_CAMPER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_KINDLER, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_TEAM_MAGMA, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_MAGMA_ADMIN, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_MAGMA_LEADER, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_LASS, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_BUG_CATCHER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_NINJA_BOY, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_RICH_BOY, TRAINER_ENCOUNTER_MUSIC_RICH},
    {TRAINER_CLASS_HEX_MANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_BEAUTY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_PARASOL_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_PICNICKER, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_PKMN_BREEDER, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_COLLECTOR, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_PKMN_RANGER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_RIVAL, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_YOUNG_COUPLE, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_PSYCHIC, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SR_AND_JR, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_ELITE_FOUR, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_YOUNGSTER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_EXPERT, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_TRIATHLETE, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_BIRD_KEEPER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_FISHERMAN, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_CHAMPION, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TUBER_M, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TUBER_F, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_SIS_AND_BRO, TRAINER_ENCOUNTER_MUSIC_SWIMMER},
    {TRAINER_CLASS_HIKER, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_LEADER, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_SCHOOL_KID, TRAINER_ENCOUNTER_MUSIC_MALE},
};

static const u8 sTrainerTextGroups[50][2] =
{
    {FACILITY_CLASS_AROMA_LADY, 3},
    {FACILITY_CLASS_RUIN_MANIAC, 4},
    {FACILITY_CLASS_TUBER_F, 1},
    {FACILITY_CLASS_TUBER_M, 0},
    {FACILITY_CLASS_COOLTRAINER_M, 2},
    {FACILITY_CLASS_COOLTRAINER_F, 3},
    {FACILITY_CLASS_HEX_MANIAC, 3},
    {FACILITY_CLASS_LADY, 3},
    {FACILITY_CLASS_BEAUTY, 3},
    {FACILITY_CLASS_RICH_BOY, 2},
    {FACILITY_CLASS_POKEMANIAC, 2},
    {FACILITY_CLASS_SWIMMER_M, 2},
    {FACILITY_CLASS_BLACK_BELT, 4},
    {FACILITY_CLASS_GUITARIST, 2},
    {FACILITY_CLASS_KINDLER, 2},
    {FACILITY_CLASS_CAMPER, 0},
    {FACILITY_CLASS_BUG_MANIAC, 2},
    {FACILITY_CLASS_PSYCHIC_M, 2},
    {FACILITY_CLASS_PSYCHIC_F, 3},
    {FACILITY_CLASS_GENTLEMAN, 4},
    {FACILITY_CLASS_SCHOOL_KID_M, 0},
    {FACILITY_CLASS_SCHOOL_KID_F, 1},
    {FACILITY_CLASS_POKEFAN_M, 4},
    {FACILITY_CLASS_POKEFAN_F, 5},
    {FACILITY_CLASS_EXPERT_M, 4},
    {FACILITY_CLASS_EXPERT_F, 5},
    {FACILITY_CLASS_YOUNGSTER, 0},
    {FACILITY_CLASS_FISHERMAN, 4},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_M, 2},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_M, 2},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_M, 2},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_F, 3},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_F, 3},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_F, 3},
    {FACILITY_CLASS_DRAGON_TAMER, 2},
    {FACILITY_CLASS_BIRD_KEEPER, 2},
    {FACILITY_CLASS_NINJA_BOY, 0},
    {FACILITY_CLASS_BATTLE_GIRL, 3},
    {FACILITY_CLASS_PARASOL_LADY, 3},
    {FACILITY_CLASS_SWIMMER_F, 3},
    {FACILITY_CLASS_PICNICKER, 1},
    {FACILITY_CLASS_SAILOR, 2},
    {FACILITY_CLASS_COLLECTOR, 2},
    {FACILITY_CLASS_PKMN_BREEDER_M, 2},
    {FACILITY_CLASS_PKMN_BREEDER_F, 3},
    {FACILITY_CLASS_PKMN_RANGER_M, 2},
    {FACILITY_CLASS_PKMN_RANGER_F, 3},
    {FACILITY_CLASS_LASS, 3},
    {FACILITY_CLASS_BUG_CATCHER, 0},
    {FACILITY_CLASS_HIKER, 4},
};

static const u8 *const sExitDirectionHintTexts1[] =
{
    BattlePyramid_Text_ExitHintUp1,
    BattlePyramid_Text_ExitHintLeft1,
    BattlePyramid_Text_ExitHintRight1,
    BattlePyramid_Text_ExitHintDown1,
};

static const u8 *const sRemainingItemsHintTexts1[] =
{
    BattlePyramid_Text_ZeroItemsRemaining1,
    BattlePyramid_Text_OneItemRemaining1,
    BattlePyramid_Text_TwoItemsRemaining1,
    BattlePyramid_Text_ThreeItemsRemaining1,
    BattlePyramid_Text_FourItemsRemaining1,
    BattlePyramid_Text_FiveItemsRemaining1,
    BattlePyramid_Text_SixItemsRemaining1,
    BattlePyramid_Text_SevenItemsRemaining1,
    BattlePyramid_Text_EightItemsRemaining1,
};

static const u8 *const sRemainingTrainersHintTexts1[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining1,
    BattlePyramid_Text_OneTrainersRemaining1,
    BattlePyramid_Text_TwoTrainersRemaining1,
    BattlePyramid_Text_ThreeTrainersRemaining1,
    BattlePyramid_Text_FourTrainersRemaining1,
    BattlePyramid_Text_FiveTrainersRemaining1,
    BattlePyramid_Text_SixTrainersRemaining1,
    BattlePyramid_Text_SevenTrainersRemaining1,
};

static const u8 *const sExitDirectionHintTexts2[] =
{
    BattlePyramid_Text_ExitHintUp2,
    BattlePyramid_Text_ExitHintLeft2,
    BattlePyramid_Text_ExitHintRight2,
    BattlePyramid_Text_ExitHintDown2,
};

static const u8 *const sRemainingItemsHintTexts2[] =
{
    BattlePyramid_Text_ZeroItemsRemaining2,
    BattlePyramid_Text_OneItemRemaining2,
    BattlePyramid_Text_TwoItemsRemaining2,
    BattlePyramid_Text_ThreeItemsRemaining2,
    BattlePyramid_Text_FourItemsRemaining2,
    BattlePyramid_Text_FiveItemsRemaining2,
    BattlePyramid_Text_SixItemsRemaining2,
    BattlePyramid_Text_SevenItemsRemaining2,
    BattlePyramid_Text_EightItemsRemaining2,
};

static const u8 *const sRemainingTrainersHintTexts2[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining2,
    BattlePyramid_Text_OneTrainersRemaining2,
    BattlePyramid_Text_TwoTrainersRemaining2,
    BattlePyramid_Text_ThreeTrainersRemaining2,
    BattlePyramid_Text_FourTrainersRemaining2,
    BattlePyramid_Text_FiveTrainersRemaining2,
    BattlePyramid_Text_SixTrainersRemaining2,
    BattlePyramid_Text_SevenTrainersRemaining2,
};

static const u8 *const sExitDirectionHintTexts3[] =
{
    BattlePyramid_Text_ExitHintUp3,
    BattlePyramid_Text_ExitHintLeft3,
    BattlePyramid_Text_ExitHintRight3,
    BattlePyramid_Text_ExitHintDown3,
};

static const u8 *const sRemainingItemsHintTexts3[] =
{
    BattlePyramid_Text_ZeroItemsRemaining3,
    BattlePyramid_Text_OneItemRemaining3,
    BattlePyramid_Text_TwoItemsRemaining3,
    BattlePyramid_Text_ThreeItemsRemaining3,
    BattlePyramid_Text_FourItemsRemaining3,
    BattlePyramid_Text_FiveItemsRemaining3,
    BattlePyramid_Text_SixItemsRemaining3,
    BattlePyramid_Text_SevenItemsRemaining3,
    BattlePyramid_Text_EightItemsRemaining3,
};

static const u8 *const sRemainingTrainersHintTexts3[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining3,
    BattlePyramid_Text_OneTrainersRemaining3,
    BattlePyramid_Text_TwoTrainersRemaining3,
    BattlePyramid_Text_ThreeTrainersRemaining3,
    BattlePyramid_Text_FourTrainersRemaining3,
    BattlePyramid_Text_FiveTrainersRemaining3,
    BattlePyramid_Text_SixTrainersRemaining3,
    BattlePyramid_Text_SevenTrainersRemaining3,
};

static const u8 *const sExitDirectionHintTexts4[] =
{
    BattlePyramid_Text_ExitHintUp4,
    BattlePyramid_Text_ExitHintLeft4,
    BattlePyramid_Text_ExitHintRight4,
    BattlePyramid_Text_ExitHintDown4,
};

static const u8 *const sRemainingItemsHintTexts4[] =
{
    BattlePyramid_Text_ZeroItemsRemaining4,
    BattlePyramid_Text_OneItemRemaining4,
    BattlePyramid_Text_TwoItemsRemaining4,
    BattlePyramid_Text_ThreeItemsRemaining4,
    BattlePyramid_Text_FourItemsRemaining4,
    BattlePyramid_Text_FiveItemsRemaining4,
    BattlePyramid_Text_SixItemsRemaining4,
    BattlePyramid_Text_SevenItemsRemaining4,
    BattlePyramid_Text_EightItemsRemaining4,
};

static const u8 *const sRemainingTrainersHintTexts4[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining4,
    BattlePyramid_Text_OneTrainersRemaining4,
    BattlePyramid_Text_TwoTrainersRemaining4,
    BattlePyramid_Text_ThreeTrainersRemaining4,
    BattlePyramid_Text_FourTrainersRemaining4,
    BattlePyramid_Text_FiveTrainersRemaining4,
    BattlePyramid_Text_SixTrainersRemaining4,
    BattlePyramid_Text_SevenTrainersRemaining4,
};

static const u8 *const sExitDirectionHintTexts5[] =
{
    BattlePyramid_Text_ExitHintUp5,
    BattlePyramid_Text_ExitHintLeft5,
    BattlePyramid_Text_ExitHintRight5,
    BattlePyramid_Text_ExitHintDown5,
};

static const u8 *const sRemainingItemsHintTexts5[] =
{
    BattlePyramid_Text_ZeroItemsRemaining5,
    BattlePyramid_Text_OneItemRemaining5,
    BattlePyramid_Text_TwoItemsRemaining5,
    BattlePyramid_Text_ThreeItemsRemaining5,
    BattlePyramid_Text_FourItemsRemaining5,
    BattlePyramid_Text_FiveItemsRemaining5,
    BattlePyramid_Text_SixItemsRemaining5,
    BattlePyramid_Text_SevenItemsRemaining5,
    BattlePyramid_Text_EightItemsRemaining5,
};

static const u8 *const sRemainingTrainersHintTexts5[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining5,
    BattlePyramid_Text_OneTrainersRemaining5,
    BattlePyramid_Text_TwoTrainersRemaining5,
    BattlePyramid_Text_ThreeTrainersRemaining5,
    BattlePyramid_Text_FourTrainersRemaining5,
    BattlePyramid_Text_FiveTrainersRemaining5,
    BattlePyramid_Text_SixTrainersRemaining5,
    BattlePyramid_Text_SevenTrainersRemaining5,
};

static const u8 *const sExitDirectionHintTexts6[] =
{
    BattlePyramid_Text_ExitHintUp6,
    BattlePyramid_Text_ExitHintLeft6,
    BattlePyramid_Text_ExitHintRight6,
    BattlePyramid_Text_ExitHintDown6,
};

static const u8 *const sRemainingItemsHintTexts6[] =
{
    BattlePyramid_Text_ZeroItemsRemaining6,
    BattlePyramid_Text_OneItemRemaining6,
    BattlePyramid_Text_TwoItemsRemaining6,
    BattlePyramid_Text_ThreeItemsRemaining6,
    BattlePyramid_Text_FourItemsRemaining6,
    BattlePyramid_Text_FiveItemsRemaining6,
    BattlePyramid_Text_SixItemsRemaining6,
    BattlePyramid_Text_SevenItemsRemaining6,
    BattlePyramid_Text_EightItemsRemaining6,
};

static const u8 *const sRemainingTrainersHintTexts6[] =
{
    BattlePyramid_Text_ZeroTrainersRemaining6,
    BattlePyramid_Text_OneTrainersRemaining6,
    BattlePyramid_Text_TwoTrainersRemaining6,
    BattlePyramid_Text_ThreeTrainersRemaining6,
    BattlePyramid_Text_FourTrainersRemaining6,
    BattlePyramid_Text_FiveTrainersRemaining6,
    BattlePyramid_Text_SixTrainersRemaining6,
    BattlePyramid_Text_SevenTrainersRemaining6,
};

static const u8 *const *const sPostBattleHintTexts1[] =
{
    sExitDirectionHintTexts1,
    sRemainingItemsHintTexts1,
    sRemainingTrainersHintTexts1,
};

static const u8 *const *const sPostBattleHintTexts2[] =
{
    sExitDirectionHintTexts2,
    sRemainingItemsHintTexts2,
    sRemainingTrainersHintTexts2,
};

static const u8 *const *const sPostBattleHintTexts3[] =
{
    sExitDirectionHintTexts3,
    sRemainingItemsHintTexts3,
    sRemainingTrainersHintTexts3,
};

static const u8 *const *const sPostBattleHintTexts4[] =
{
    sExitDirectionHintTexts4,
    sRemainingItemsHintTexts4,
    sRemainingTrainersHintTexts4,
};

static const u8 *const *const sPostBattleHintTexts5[] =
{
    sExitDirectionHintTexts5,
    sRemainingItemsHintTexts5,
    sRemainingTrainersHintTexts5,
};

static const u8 *const *const sPostBattleHintTexts6[] =
{
    sExitDirectionHintTexts6,
    sRemainingItemsHintTexts6,
    sRemainingTrainersHintTexts6,
};

static const u8 *const *const *const sPostBattleTexts[] =
{
    sPostBattleHintTexts1,
    sPostBattleHintTexts2,
    sPostBattleHintTexts3,
    sPostBattleHintTexts4,
    sPostBattleHintTexts5,
    sPostBattleHintTexts6,
};

static const u8 sHintTextTypes[] =
{
    HINT_EXIT_SHORT_REMAINING_TRAINERS,
    HINT_EXIT_SHORT_REMAINING_ITEMS,
    HINT_EXIT_MEDIUM_REMAINING_TRAINERS,
    HINT_EXIT_MEDIUM_REMAINING_ITEMS,
    HINT_EXIT_FAR_REMAINING_TRAINERS,
    HINT_EXIT_FAR_REMAINING_ITEMS,
    HINT_EXIT_SHORT_REMAINING_TRAINERS,
    HINT_EXIT_SHORT_REMAINING_ITEMS,
};

static void (* const sBattlePyramidFunctions[])(void) =
{
    [BATTLE_PYRAMID_FUNC_INIT]              = InitPyramidChallenge,
    [BATTLE_PYRAMID_FUNC_GET_DATA]          = GetBattlePyramidData,
    [BATTLE_PYRAMID_FUNC_SET_DATA]          = SetBattlePyramidData,
    [BATTLE_PYRAMID_FUNC_SAVE]              = SavePyramidChallenge,
    [BATTLE_PYRAMID_FUNC_SET_PRIZE]         = SetBattlePyramidPrize,
    [BATTLE_PYRAMID_FUNC_GIVE_PRIZE]        = GiveBattlePyramidPrize,
    [BATTLE_PYRAMID_FUNC_SEED_FLOOR]        = SeedPyramidFloor,
    [BATTLE_PYRAMID_FUNC_SET_ITEM]          = SetPickupItem,
    [BATTLE_PYRAMID_FUNC_HIDE_ITEM]         = HidePyramidItem,
    [BATTLE_PYRAMID_FUNC_SET_TRAINERS]      = SetPyramidFacilityTrainers,
    [BATTLE_PYRAMID_FUNC_SHOW_HINT_TEXT]    = ShowPostBattleHintText,
    [BATTLE_PYRAMID_FUNC_UPDATE_STREAK]     = UpdatePyramidWinStreak,
    [BATTLE_PYRAMID_FUNC_CURRENT_LOCATION]  = GetCurrentBattlePyramidLocation,
    [BATTLE_PYRAMID_FUNC_UPDATE_LIGHT]      = UpdatePyramidLightRadius,
    [BATTLE_PYRAMID_FUNC_CLEAR_HELD_ITEMS]  = ClearPyramidPartyHeldItems,
    [BATTLE_PYRAMID_FUNC_SET_FLOOR_PALETTE] = SetPyramidFloorPalette,
    [BATTLE_PYRAMID_FUNC_START_MENU]        = BattlePyramidStartMenu,
    [BATTLE_PYRAMID_FUNC_RESTORE_PARTY]     = RestorePyramidPlayerParty,
};

static const u16 sShortStreakRewardItems[] = {ITEM_HP_UP, ITEM_PROTEIN, ITEM_IRON, ITEM_CALCIUM, ITEM_CARBOS, ITEM_ZINC};
static const u16 sLongStreakRewardItems[] = {ITEM_BRIGHT_POWDER, ITEM_WHITE_HERB, ITEM_QUICK_CLAW, ITEM_LEFTOVERS, ITEM_MENTAL_HERB, ITEM_KINGS_ROCK, ITEM_FOCUS_BAND, ITEM_SCOPE_LENS, ITEM_CHOICE_BAND};

static const u8 sBorderedSquareIds[][4] =
{
    {1,   4, -1, -1},
    {0,   2,  5, -1},
    {1,   3,  6, -1},
    {2,   7, -1, -1},
    {0,   5,  8, -1},
    {1,   4,  6,  9},
    {2,   5,  7, 10},
    {3,   6, 11, -1},
    {4,   9, 12, -1},
    {5,   8, 10, 13},
    {6,   9, 11, 14},
    {7,  10, 15, -1},
    {8,  13, -1, -1},
    {9,  12, 14, -1},
    {10, 13, 15, -1},
    {11, 14, -1, -1},
};

static const u8 sPickupPercentages[PICKUP_ITEMS_PER_ROUND] = {30, 40, 50, 60, 70, 80, 85, 90, 95, 100};

// code
void CallBattlePyramidFunction(void)
{
    sBattlePyramidFunctions[gSpecialVar_0x8004]();
}

static void InitPyramidChallenge(void)
{
    bool32 isCurrent;
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;

    gSaveBlock2Ptr->frontier.challengeStatus = 0;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    if (lvlMode != FRONTIER_LVL_50)
        isCurrent = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_OPEN;
    else
        isCurrent = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_50;

    if (!isCurrent)
    {
        gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] = 0;
        InitPyramidBagItems(lvlMode);
    }

    InitBattlePyramidBagCursorPosition();
    gTrainerBattleOpponent_A = 0;
    gBattleOutcome = 0;
}

static void GetBattlePyramidData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;

    switch (gSpecialVar_0x8005)
    {
    case PYRAMID_DATA_PRIZE:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.pyramidPrize;
        break;
    case PYRAMID_DATA_WIN_STREAK:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode];
        break;
    case PYRAMID_DATA_WIN_STREAK_ACTIVE:
        if (lvlMode != FRONTIER_LVL_50)
            gSpecialVar_Result = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_OPEN;
        else
            gSpecialVar_Result = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_50;
        break;
    case PYRAMID_DATA_WIN_STREAK_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.pyramidWinStreaks[FRONTIER_LVL_50];
        break;
    case PYRAMID_DATA_WIN_STREAK_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.pyramidWinStreaks[FRONTIER_LVL_OPEN];
        break;
    case PYRAMID_DATA_WIN_STREAK_ACTIVE_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_50;
        break;
    case PYRAMID_DATA_WIN_STREAK_ACTIVE_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.winStreakActiveFlags & STREAK_PYRAMID_OPEN;
        break;
    }
}

static void SetBattlePyramidData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;

    switch (gSpecialVar_0x8005)
    {
    case PYRAMID_DATA_PRIZE:
        gSaveBlock2Ptr->frontier.pyramidPrize = gSpecialVar_0x8006;
        break;
    case PYRAMID_DATA_WIN_STREAK:
        gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] = gSpecialVar_0x8006;
        break;
    case PYRAMID_DATA_WIN_STREAK_ACTIVE:
        if (lvlMode != FRONTIER_LVL_50)
        {
            if (gSpecialVar_0x8006)
                gSaveBlock2Ptr->frontier.winStreakActiveFlags |= STREAK_PYRAMID_OPEN;
            else
                gSaveBlock2Ptr->frontier.winStreakActiveFlags &= ~(STREAK_PYRAMID_OPEN);
        }
        else
        {
            if (gSpecialVar_0x8006)
                gSaveBlock2Ptr->frontier.winStreakActiveFlags |= STREAK_PYRAMID_50;
            else
                gSaveBlock2Ptr->frontier.winStreakActiveFlags &= ~(STREAK_PYRAMID_50);
        }
        break;
    case PYRAMID_DATA_TRAINER_FLAGS:
        gSaveBlock2Ptr->frontier.pyramidTrainerFlags = gSpecialVar_0x8006;
        break;
    }
}

static void SavePyramidChallenge(void)
{
    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveMapView();
    TrySavingData(SAVE_LINK);
}

static void SetBattlePyramidPrize(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;

    if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] > 41)
        gSaveBlock2Ptr->frontier.pyramidPrize = sLongStreakRewardItems[Random() % ARRAY_COUNT(sLongStreakRewardItems)];
    else
        gSaveBlock2Ptr->frontier.pyramidPrize = sShortStreakRewardItems[Random() % ARRAY_COUNT(sShortStreakRewardItems)];
}

static void GiveBattlePyramidPrize(void)
{
    if (AddBagItem(gSaveBlock2Ptr->frontier.pyramidPrize, 1) == TRUE)
    {
        CopyItemName(gSaveBlock2Ptr->frontier.pyramidPrize, gStringVar1);
        gSaveBlock2Ptr->frontier.pyramidPrize = 0;
        gSpecialVar_Result = TRUE;
    }
    else
    {
        gSpecialVar_Result = FALSE;
    }
}

static void SeedPyramidFloor(void)
{
    int i;

    for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.pyramidRandoms); i++)
        gSaveBlock2Ptr->frontier.pyramidRandoms[i] = Random();

    gSaveBlock2Ptr->frontier.pyramidTrainerFlags = 0;
}

static void SetPickupItem(void)
{
    int i;
    int itemIndex;
    int rand;
    u8 id;
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 floor = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    u32 round = (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] / FRONTIER_STAGES_PER_CHALLENGE) % TOTAL_PYRAMID_ROUNDS;

    if (round >= TOTAL_PYRAMID_ROUNDS)
        round = TOTAL_PYRAMID_ROUNDS - 1;

    id = GetPyramidFloorTemplateId();
    itemIndex = (gSpecialVar_LastTalked - sPyramidFloorTemplates[id].numTrainers) - 1;
    rand = gSaveBlock2Ptr->frontier.pyramidRandoms[itemIndex / 2];
    SeedRng2(rand);

    for (i = 0; i < itemIndex + 1; i++)
        rand = Random2() % 100;

    for (i = sPickupItemOffsets[floor]; i < ARRAY_COUNT(sPickupItemSlots); i++)
    {
        if (rand < sPickupItemSlots[i][0])
            break;
    }

    if (lvlMode != FRONTIER_LVL_50)
        gSpecialVar_0x8000 = sPickupItemsLvlOpen[round][sPickupItemSlots[i][1]];
    else
        gSpecialVar_0x8000 = sPickupItemsLvl50[round][sPickupItemSlots[i][1]];

    // Quantity of item to give
    gSpecialVar_0x8001 = 1;
}

static void HidePyramidItem(void)
{
    struct ObjectEventTemplate *events = gSaveBlock1Ptr->objectEventTemplates;
    int i = 0;

    for (;;)
    {
        if (events[i].localId == gSpecialVar_LastTalked)
        {
            // Rather than using event flags to hide the item object event,
            // it moves them far off the map bounds.
            events[i].x = SHRT_MAX;
            events[i].y = SHRT_MAX;
            break;
        }
        i++;
        if (events[i].localId == 0)
            break;
    }
}

static void SetPyramidFacilityTrainers(void)
{
    gFacilityTrainers = gBattleFrontierTrainers;
}

static void ShowPostBattleHintText(void)
{
    int i;
    int hintType;
    u8 id;
    int textGroup = 0;
    int textIndex = 0;
    struct ObjectEventTemplate *events = gSaveBlock1Ptr->objectEventTemplates;
    u16 trainerId = LocalIdToPyramidTrainerId(gObjectEvents[gSelectedObjectEvent].localId);

    for (i = 0; i < ARRAY_COUNT(sTrainerTextGroups); i++)
    {
        if (sTrainerTextGroups[i][0] == gFacilityTrainers[trainerId].facilityClass)
        {
            textGroup = sTrainerTextGroups[i][1];
            break;
        }
    }

    hintType = sHintTextTypes[gObjectEvents[gSelectedObjectEvent].localId - 1];
    i = 0;
    while (!i)
    {
        switch (hintType)
        {
        case HINT_EXIT_DIRECTION:
            textIndex = GetPostBattleDirectionHintTextIndex(&hintType, 8, HINT_EXIT_DIRECTION);
            i = 1;
            break;
        case HINT_REMAINING_ITEMS:
            for (i = 0; i < GetNumBattlePyramidObjectEvents(); i++)
            {
                if (events[i].graphicsId == OBJ_EVENT_GFX_ITEM_BALL && events[i].x != SHRT_MAX && events[i].y != SHRT_MAX)
                    textIndex++;
            }
            i = 1;
            break;
        case HINT_REMAINING_TRAINERS:
            id = GetPyramidFloorTemplateId();
            textIndex = sPyramidFloorTemplates[id].numTrainers;
            for (i = 0; i < MAX_PYRAMID_TRAINERS; i++)
            {
                if (gBitTable[i] & gSaveBlock2Ptr->frontier.pyramidTrainerFlags)
                    textIndex--;
            }
            i = 1;
            break;
        case HINT_EXIT_SHORT_REMAINING_TRAINERS:
            GetPostBattleDirectionHintTextIndex(&hintType, 8, HINT_REMAINING_TRAINERS);
            break;
        case HINT_EXIT_SHORT_REMAINING_ITEMS:
            GetPostBattleDirectionHintTextIndex(&hintType, 8, HINT_REMAINING_ITEMS);
            break;
        case HINT_EXIT_MEDIUM_REMAINING_TRAINERS:
            GetPostBattleDirectionHintTextIndex(&hintType, 16, HINT_REMAINING_TRAINERS);
            break;
        case HINT_EXIT_MEDIUM_REMAINING_ITEMS:
            GetPostBattleDirectionHintTextIndex(&hintType, 16, HINT_REMAINING_ITEMS);
            break;
        case HINT_EXIT_FAR_REMAINING_TRAINERS:
            GetPostBattleDirectionHintTextIndex(&hintType, 24, HINT_REMAINING_TRAINERS);
            break;
        case HINT_EXIT_FAR_REMAINING_ITEMS:
            GetPostBattleDirectionHintTextIndex(&hintType, 24, HINT_REMAINING_ITEMS);
            break;
        }
    }
    ShowFieldMessage(sPostBattleTexts[textGroup][hintType][textIndex]);
}

static void UpdatePyramidWinStreak(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;

    if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] < 999)
        gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode]++;
    if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] > gSaveBlock2Ptr->frontier.pyramidRecordStreaks[lvlMode])
        gSaveBlock2Ptr->frontier.pyramidRecordStreaks[lvlMode] = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode];
}

static void GetCurrentBattlePyramidLocation(void)
{
    gSpecialVar_Result = CurrentBattlePyramidLocation();
}

static void UpdatePyramidLightRadius(void)
{
    switch (gSpecialVar_0x8006)
    {
    case PYRAMID_LIGHT_SET_RADIUS:
        gSaveBlock2Ptr->frontier.pyramidLightRadius = gSpecialVar_0x8005;
        break;
    case PYRAMID_LIGHT_INCR_RADIUS:
        switch (gSpecialVar_Result)
        {
        case 0:
            if (!gPaletteFade.active)
            {
                if (gSaveBlock2Ptr->frontier.pyramidLightRadius >= 120)
                    gSaveBlock2Ptr->frontier.pyramidLightRadius = 120;
                else
                    PlaySE(gSpecialVar_0x8007);
                gSpecialVar_Result++;
            }
            break;
        case 1:
            if (gSpecialVar_0x8005 != 0)
            {
                gSpecialVar_0x8005--;
                gSaveBlock2Ptr->frontier.pyramidLightRadius++;
                if (gSaveBlock2Ptr->frontier.pyramidLightRadius > 120)
                {
                    gSaveBlock2Ptr->frontier.pyramidLightRadius = 120;
                    gSpecialVar_Result++;
                }
                WriteBattlePyramidViewScanlineEffectBuffer();
            }
            else
            {
                gSpecialVar_Result = 2;
            }
            break;
        case 2:
        default:
            break;
        }
        break;
    }
}

static void ClearPyramidPartyHeldItems(void)
{
    int i, j;
    u16 item = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        for (j = 0; j < MAX_FRONTIER_PARTY_SIZE; j++)
        {
            if (gSaveBlock2Ptr->frontier.selectedPartyMons[j] != 0 && gSaveBlock2Ptr->frontier.selectedPartyMons[j] - 1 == i)
                SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &item);
        }
    }
}

static void SetPyramidFloorPalette(void)
{
    CreateTask(Task_SetPyramidFloorPalette, 0);
}

static void Task_SetPyramidFloorPalette(u8 taskId)
{
    if (gPaletteFade.active)
    {
        CpuCopy16(gBattlePyramidFloor_Pal[gSaveBlock2Ptr->frontier.curChallengeBattleNum], &gPlttBufferUnfaded[BG_PLTT_ID(6)], PLTT_SIZE_4BPP);
        DestroyTask(taskId);
    }
}

// Unused. Handled by BuildStartMenuActions
static void BattlePyramidStartMenu(void)
{
    ShowBattlePyramidStartMenu();
}

static void RestorePyramidPlayerParty(void)
{
    int i, j, k, l;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        int partyIndex = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            if (GetMonData(&gSaveBlock1Ptr->playerParty[partyIndex], MON_DATA_SPECIES, NULL) == GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL))
            {
                for (k = 0; k < MAX_MON_MOVES; k++)
                {
                    for (l = 0; l < MAX_MON_MOVES; l++)
                    {
                        if (GetMonData(&gSaveBlock1Ptr->playerParty[partyIndex], MON_DATA_MOVE1 + l, NULL) == GetMonData(&gPlayerParty[j], MON_DATA_MOVE1 + k, NULL))
                            break;
                    }
                    if (l == MAX_MON_MOVES)
                        SetMonMoveSlot(&gPlayerParty[j], MOVE_SKETCH, k);
                }
                gSaveBlock1Ptr->playerParty[partyIndex] = gPlayerParty[j];
                gSelectedOrderFromParty[j] = partyIndex + 1;
                break;
            }
        }
    }

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        gSaveBlock2Ptr->frontier.selectedPartyMons[i] = gSelectedOrderFromParty[i];
}

static u8 GetPostBattleDirectionHintTextIndex(int *hintType, u8 minDistanceForExitHint, u8 defaultHintType)
{
    int x, y;
    u8 textIndex = 0;
    u16 *map = gBackupMapLayout.map;
    map += gBackupMapLayout.width * 7 + MAP_OFFSET;

    for (y = 0; y < 32; map += 47, y++)
    {
        for (x = 0; x < 32; x++)
        {
            if ((map[x] & MAPGRID_METATILE_ID_MASK) == METATILE_BattlePyramid_Exit)
            {
                x += MAP_OFFSET - gObjectEvents[gSelectedObjectEvent].initialCoords.x;
                y += MAP_OFFSET - gObjectEvents[gSelectedObjectEvent].initialCoords.y;
                if (x >= minDistanceForExitHint
                 || x <= -minDistanceForExitHint
                 || y >= minDistanceForExitHint
                 || y <= -minDistanceForExitHint
                 || defaultHintType == HINT_EXIT_DIRECTION)
                {
                    if (x > 0 && y > 0)
                    {
                        if (x >= y)
                            textIndex = 2;
                        else
                            textIndex = 3;
                    }
                    else if (x < 0 && y < 0)
                    {
                        if (x > y)
                            textIndex = 0;
                        else
                            textIndex = 1;
                    }
                    else if (x == 0)
                    {
                        if (y > 0)
                            textIndex = 3;
                        else
                            textIndex = 0;
                    }
                    else if (y == 0)
                    {
                        if (x > 0)
                            textIndex = 2;
                        else
                            textIndex = 1;
                    }
                    else if (x < 0)
                    {
                        if (x + y > 0)
                            textIndex = 3;
                        else
                            textIndex = 1;
                    }
                    else
                    {
                        textIndex = (~(x + y) >= 0) ? 0 : 2;
                    }
                    *hintType = HINT_EXIT_DIRECTION;
                }
                else
                {
                    *hintType = defaultHintType;
                }
                return textIndex;
            }
        }
    }

    return textIndex;
}

u16 LocalIdToPyramidTrainerId(u8 localId)
{
    return gSaveBlock2Ptr->frontier.trainerIds[localId - 1];
}

bool8 GetBattlePyramidTrainerFlag(u8 eventId)
{
    return gSaveBlock2Ptr->frontier.pyramidTrainerFlags & gBitTable[gObjectEvents[eventId].localId - 1];
}

void MarkApproachingPyramidTrainersAsBattled(void)
{
    MarkPyramidTrainerAsBattled(gTrainerBattleOpponent_A);
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
    {
        gSelectedObjectEvent = GetChosenApproachingTrainerObjectEventId(1);
        MarkPyramidTrainerAsBattled(gTrainerBattleOpponent_B);
    }
}

static void MarkPyramidTrainerAsBattled(u16 trainerId)
{
    int i;

    for (i = 0; i < MAX_PYRAMID_TRAINERS; i++)
    {
        if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
            gSaveBlock2Ptr->frontier.pyramidTrainerFlags |= gBitTable[i];
    }

    gObjectEvents[gSelectedObjectEvent].movementType = MOVEMENT_TYPE_WANDER_AROUND;
    gSaveBlock1Ptr->objectEventTemplates[gSpecialVar_LastTalked - 1].movementType = MOVEMENT_TYPE_WANDER_AROUND;
    gObjectEvents[gSelectedObjectEvent].initialCoords.x = gObjectEvents[gSelectedObjectEvent].currentCoords.x;
    gObjectEvents[gSelectedObjectEvent].initialCoords.y = gObjectEvents[gSelectedObjectEvent].currentCoords.y;
}

void GenerateBattlePyramidWildMon(void)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    int i;
    const struct PyramidWildMon *wildMons;
    u32 id;
    u32 lvl = gSaveBlock2Ptr->frontier.lvlMode;
    u16 round = (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvl] / FRONTIER_STAGES_PER_CHALLENGE) % TOTAL_PYRAMID_ROUNDS;

    if (round >= TOTAL_PYRAMID_ROUNDS)
        round = TOTAL_PYRAMID_ROUNDS - 1;

    if (lvl != FRONTIER_LVL_50)
        wildMons = sOpenLevelWildMonPointers[round];
    else
        wildMons = sLevel50WildMonPointers[round];

    id = GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL) - 1;
    SetMonData(&gEnemyParty[0], MON_DATA_SPECIES, &wildMons[id].species);
    GetSpeciesName(name, wildMons[id].species);
    SetMonData(&gEnemyParty[0], MON_DATA_NICKNAME, &name);
    if (lvl != FRONTIER_LVL_50)
    {
        lvl = SetFacilityPtrsGetLevel();
        lvl -= wildMons[id].lvl;
        lvl = lvl - 5 + (Random() % 11);
    }
    else
    {
        lvl = wildMons[id].lvl - 5 + ((Random() % 11));
    }
    SetMonData(&gEnemyParty[0],
               MON_DATA_EXP,
               &gExperienceTables[gSpeciesInfo[wildMons[id].species].growthRate][lvl]);

    switch (wildMons[id].abilityNum)
    {
    case 0:
    case 1:
        SetMonData(&gEnemyParty[0], MON_DATA_ABILITY_NUM, &wildMons[id].abilityNum);
        break;
    case ABILITY_RANDOM:
    default:
        if (gSpeciesInfo[wildMons[id].species].abilities[1])
        {
            i = GetMonData(&gEnemyParty[0], MON_DATA_PERSONALITY, NULL) % 2;
            SetMonData(&gEnemyParty[0], MON_DATA_ABILITY_NUM, &i);
        }
        else
        {
            i = 0;
            SetMonData(&gEnemyParty[0], MON_DATA_ABILITY_NUM, &i);
        }
        break;
    }

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(&gEnemyParty[0], wildMons[id].moves[i], i);

    // UB: Reading outside the array as lvl was used for mon level instead of frontier lvl mode.
    #ifndef UBFIX
    if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvl] >= 140)
    #else
    if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[gSaveBlock2Ptr->frontier.lvlMode] >= 140)
    #endif
    {
        id = (Random() % 17) + 15;
        for (i = 0; i < NUM_STATS; i++)
            SetMonData(&gEnemyParty[0], MON_DATA_HP_IV + i, &id);
    }
    CalculateMonStats(&gEnemyParty[0]);
}

u8 GetPyramidRunMultiplier(void)
{
    u8 id = GetPyramidFloorTemplateId();
    return sPyramidFloorTemplates[id].runMultiplier;
}

u8 CurrentBattlePyramidLocation(void)
{
    if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
        return PYRAMID_LOCATION_FLOOR;
    else if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP)
        return PYRAMID_LOCATION_TOP;
    else
        return PYRAMID_LOCATION_NONE;
}

bool8 InBattlePyramid_(void)
{
    return gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR
        || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP;
}

void PausePyramidChallenge(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        RestorePyramidPlayerParty();
        gSaveBlock2Ptr->frontier.challengeStatus = CHALLENGE_STATUS_PAUSED;
        VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
        LoadPlayerParty();
    }
}

void SoftResetInBattlePyramid(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        DoSoftReset();
}

void CopyPyramidTrainerSpeechBefore(u16 trainerId)
{
    FrontierSpeechToString(gFacilityTrainers[trainerId].speechBefore);
}

void CopyPyramidTrainerWinSpeech(u16 trainerId)
{
    FrontierSpeechToString(gFacilityTrainers[trainerId].speechWin);
}

void CopyPyramidTrainerLoseSpeech(u16 trainerId)
{
    FrontierSpeechToString(gFacilityTrainers[trainerId].speechLose);
}

u8 GetTrainerEncounterMusicIdInBattlePyramid(u16 trainerId)
{
    int i;

    for (i = 0; i < ARRAY_COUNT(sTrainerClassEncounterMusic); i++)
    {
        if (sTrainerClassEncounterMusic[i].trainerClass == gFacilityClassToTrainerClass[gFacilityTrainers[trainerId].facilityClass])
            return sTrainerClassEncounterMusic[i].trainerEncounterMusic;
    }
    return TRAINER_ENCOUNTER_MUSIC_MALE;
}

static void UNUSED BattlePyramidRetireChallenge(void)
{
    ScriptContext_SetupScript(BattlePyramid_Retire);
}

static u16 GetUniqueTrainerId(u8 objectEventId)
{
    int i;
    u16 trainerId;
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 challengeNum = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
    u32 floor = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    if (floor == FRONTIER_STAGES_PER_CHALLENGE)
    {
        do
        {
            trainerId = GetRandomScaledFrontierTrainerId(challengeNum + 1, floor);
            for (i = 0; i < objectEventId; i++)
            {
                if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
                    break;
            }
        } while (i != objectEventId);
    }
    else
    {
        do
        {
            trainerId = GetRandomScaledFrontierTrainerId(challengeNum, floor);
            for (i = 0; i < objectEventId; i++)
            {
                if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
                    break;
            }
        } while (i != objectEventId);
    }

    return trainerId;
}

void GenerateBattlePyramidFloorLayout(u16 *backupMapData, bool8 setPlayerPosition)
{
    int y, x;
    int i;
    u8 entranceSquareId, exitSquareId;
    u8 *floorLayoutOffsets = AllocZeroed(NUM_PYRAMID_FLOOR_SQUARES);

    GetPyramidFloorLayoutOffsets(floorLayoutOffsets);
    GetPyramidEntranceAndExitSquareIds(&entranceSquareId, &exitSquareId);
    for (i = 0; i < NUM_PYRAMID_FLOOR_SQUARES; i++)
    {
        u16 *map;
        int yOffset, xOffset;
        const struct MapLayout *mapLayout = gMapLayouts[floorLayoutOffsets[i] + LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR];
        const u16 *layoutMap = mapLayout->map;

        gBackupMapLayout.map = backupMapData;
        gBackupMapLayout.width = mapLayout->width * PYRAMID_FLOOR_SQUARES_WIDE + MAP_OFFSET_W;
        gBackupMapLayout.height = mapLayout->height * PYRAMID_FLOOR_SQUARES_HIGH + MAP_OFFSET_H;
        map = backupMapData;
        yOffset = ((i / PYRAMID_FLOOR_SQUARES_WIDE * mapLayout->height) + MAP_OFFSET) * gBackupMapLayout.width;
        xOffset = (i % PYRAMID_FLOOR_SQUARES_WIDE * mapLayout->width) + MAP_OFFSET;
        map += yOffset + xOffset;
        for (y = 0; y < mapLayout->height; y++)
        {
            for (x = 0; x < mapLayout->width; x++)
            {
                if ((layoutMap[x] & MAPGRID_METATILE_ID_MASK) != METATILE_BattlePyramid_Exit)
                {
                    map[x] = layoutMap[x];
                }
                else if (i != exitSquareId)
                {
                    if (i == entranceSquareId && setPlayerPosition == FALSE)
                    {
                        gSaveBlock1Ptr->pos.x = (mapLayout->width * (i % PYRAMID_FLOOR_SQUARES_WIDE)) + x;
                        gSaveBlock1Ptr->pos.y = (mapLayout->height * (i / PYRAMID_FLOOR_SQUARES_WIDE)) + y;
                    }
                    map[x] = (layoutMap[x] & (MAPGRID_ELEVATION_MASK | MAPGRID_COLLISION_MASK)) | METATILE_BattlePyramid_Floor;
                }
                else
                {
                    map[x] = layoutMap[x];
                }
            }
            map += MAP_OFFSET_W + (mapLayout->width * PYRAMID_FLOOR_SQUARES_WIDE);
            layoutMap += mapLayout->width;
        }
    }
    RunOnLoadMapScript();
    Free(floorLayoutOffsets);
}

void LoadBattlePyramidObjectEventTemplates(void)
{
    int i;
    u8 id;
    u8 entranceSquareId, exitSquareId;

    for (i = 0; i < MAX_PYRAMID_TRAINERS; i++)
        gSaveBlock2Ptr->frontier.trainerIds[i] = 0xFFFF;

    id = GetPyramidFloorTemplateId();
    GetPyramidEntranceAndExitSquareIds(&entranceSquareId, &exitSquareId);
    CpuFill32(0, gSaveBlock1Ptr->objectEventTemplates, sizeof(gSaveBlock1Ptr->objectEventTemplates));
    for (i = 0; i < 2; i++)
    {
        u8 objectPositionsType;

        if (i == OBJ_TRAINERS)
            objectPositionsType = sPyramidFloorTemplates[id].trainerPositions;
        else  // OBJ_ITEMS
            objectPositionsType = sPyramidFloorTemplates[id].itemPositions;

        switch (objectPositionsType)
        {
        case OBJ_POSITIONS_UNIFORM:
            SetPyramidObjectPositionsUniformly(i);
            break;
        case OBJ_POSITIONS_IN_AND_NEAR_ENTRANCE:
            if (SetPyramidObjectPositionsInAndNearSquare(i, entranceSquareId))
                SetPyramidObjectPositionsUniformly(i);
            break;
        case OBJ_POSITIONS_IN_AND_NEAR_EXIT:
            if (SetPyramidObjectPositionsInAndNearSquare(i, exitSquareId))
                SetPyramidObjectPositionsUniformly(i);
            break;
        case OBJ_POSITIONS_NEAR_ENTRANCE:
            if (SetPyramidObjectPositionsNearSquare(i, entranceSquareId))
                SetPyramidObjectPositionsUniformly(i);
            break;
        case OBJ_POSITIONS_NEAR_EXIT:
            if (SetPyramidObjectPositionsNearSquare(i, exitSquareId))
                SetPyramidObjectPositionsUniformly(i);
            break;
        }
    }
}

void LoadBattlePyramidFloorObjectEventScripts(void)
{
    int i;
    struct ObjectEventTemplate *events = gSaveBlock1Ptr->objectEventTemplates;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (events[i].graphicsId != OBJ_EVENT_GFX_ITEM_BALL)
            events[i].script = BattlePyramid_TrainerBattle;
        else
            events[i].script = BattlePyramid_FindItemBall;
    }
}

static void GetPyramidEntranceAndExitSquareIds(u8 *entranceSquareId, u8 *exitSquareId)
{
    *entranceSquareId = gSaveBlock2Ptr->frontier.pyramidRandoms[3] % NUM_PYRAMID_FLOOR_SQUARES;
    *exitSquareId = gSaveBlock2Ptr->frontier.pyramidRandoms[0] % NUM_PYRAMID_FLOOR_SQUARES;

    if (*entranceSquareId == *exitSquareId)
    {
        *entranceSquareId = (gSaveBlock2Ptr->frontier.pyramidRandoms[3] + 1 ) % NUM_PYRAMID_FLOOR_SQUARES;
        *exitSquareId = (gSaveBlock2Ptr->frontier.pyramidRandoms[0] + NUM_PYRAMID_FLOOR_SQUARES - 1) % NUM_PYRAMID_FLOOR_SQUARES;
    }
}

static void SetPyramidObjectPositionsUniformly(u8 objType)
{
    int i;
    int numObjects;
    int objectStartIndex;
    int squareId;
    u32 bits = 0;
    u8 id = GetPyramidFloorTemplateId();
    u8 *floorLayoutOffsets = AllocZeroed(NUM_PYRAMID_FLOOR_SQUARES);

    GetPyramidFloorLayoutOffsets(floorLayoutOffsets);
    squareId = gSaveBlock2Ptr->frontier.pyramidRandoms[2] % NUM_PYRAMID_FLOOR_SQUARES;
    if (objType == OBJ_TRAINERS)
    {
        numObjects = sPyramidFloorTemplates[id].numTrainers;
        objectStartIndex = 0;
    }
    else // OBJ_ITEMS
    {
        numObjects = sPyramidFloorTemplates[id].numItems;
        objectStartIndex = sPyramidFloorTemplates[id].numTrainers;
    }

    for (i = 0; i < numObjects; i++)
    {
        do
        {
            do
            {
                if (bits & 1)
                {
                    if (!(gBitTable[squareId] & gSaveBlock2Ptr->frontier.pyramidRandoms[3]))
                        bits |= 2;
                }
                else
                {
                    if (gBitTable[squareId] & gSaveBlock2Ptr->frontier.pyramidRandoms[3])
                        bits |= 2;
                }
                if (++squareId >= NUM_PYRAMID_FLOOR_SQUARES)
                    squareId = 0;

                if (squareId == gSaveBlock2Ptr->frontier.pyramidRandoms[2] % NUM_PYRAMID_FLOOR_SQUARES)
                {
                    if (bits & 1)
                        bits |= 6;
                    else
                        bits |= 1;
                }
            } while (!(bits & 2));

        } while (!(bits & 4) && TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, squareId, objectStartIndex + i));
        bits &= 1;
    }
    Free(floorLayoutOffsets);
}

static bool8 SetPyramidObjectPositionsInAndNearSquare(u8 objType, u8 squareId)
{
    int i;
    int objectStartIndex;
    int borderedIndex = 0;
    int r7 = 0;
    int numPlacedObjects = 0;
    int numObjects;
    u8 id = GetPyramidFloorTemplateId();
    u8 *floorLayoutOffsets = AllocZeroed(NUM_PYRAMID_FLOOR_SQUARES);

    GetPyramidFloorLayoutOffsets(floorLayoutOffsets);
    if (objType == OBJ_TRAINERS)
    {
        numObjects = sPyramidFloorTemplates[id].numTrainers;
        objectStartIndex = 0;
    }
    else // OBJ_ITEMS
    {
        numObjects = sPyramidFloorTemplates[id].numItems;
        objectStartIndex = sPyramidFloorTemplates[id].numTrainers;
    }

    for (i = 0; i < numObjects; i++)
    {
        if (r7 == 0)
        {
            if (TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, squareId, objectStartIndex + i))
                r7 = 1;
            else
                numPlacedObjects++;
        }
        if (r7 & 1)
        {
            if (TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, sBorderedSquareIds[squareId][borderedIndex], objectStartIndex + i))
            {
                do
                {
                    borderedIndex++;
                    if (sBorderedSquareIds[squareId][borderedIndex] == 0xFF || borderedIndex >= 4)
                        borderedIndex = 0;
                    r7 += 2;
                } while (r7 >> 1 != 4 && TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, sBorderedSquareIds[squareId][borderedIndex], objectStartIndex + i));
                numPlacedObjects++;
            }
            else
            {
                borderedIndex++;
                if (sBorderedSquareIds[squareId][borderedIndex] == 0xFF || borderedIndex >= 4)
                    borderedIndex = 0;
                numPlacedObjects++;
            }
        }

        if (r7 >> 1 == 4)
            break;

        r7 &= 1;
    }
    #ifdef BUGFIX
    Free(floorLayoutOffsets);
    #endif

    return (numObjects / 2) > numPlacedObjects;
}

static bool8 SetPyramidObjectPositionsNearSquare(u8 objType, u8 squareId)
{
    int i;
    int objectStartIndex;
    int borderOffset = 0;
    int numPlacedObjects = 0;
    int r8 = 0;
    int numObjects;
    u8 id = GetPyramidFloorTemplateId();
    u8 *floorLayoutOffsets = AllocZeroed(NUM_PYRAMID_FLOOR_SQUARES);

    GetPyramidFloorLayoutOffsets(floorLayoutOffsets);
    if (objType == OBJ_TRAINERS)
    {
        numObjects = sPyramidFloorTemplates[id].numTrainers;
        objectStartIndex = 0;
    }
    else // OBJ_ITEMS
    {
        numObjects = sPyramidFloorTemplates[id].numItems;
        objectStartIndex = sPyramidFloorTemplates[id].numTrainers;
    }

    for (i = 0; i < numObjects; i++)
    {
        if (TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, sBorderedSquareIds[squareId][borderOffset], objectStartIndex + i))
        {
            do
            {
                borderOffset++;
                if (sBorderedSquareIds[squareId][borderOffset] == 0xFF || borderOffset >= 4)
                    borderOffset = 0;
                r8++;
            } while (r8 != 4 && TrySetPyramidObjectEventPositionInSquare(objType, floorLayoutOffsets, sBorderedSquareIds[squareId][borderOffset], objectStartIndex + i));
            numPlacedObjects++;
        }
        else
        {
            borderOffset++;
            if (sBorderedSquareIds[squareId][borderOffset] == 0xFF || borderOffset >= 4)
                borderOffset = 0;
            numPlacedObjects++;
        }

        if (r8 == 4)
            break;
    }
    #ifdef BUGFIX
    Free(floorLayoutOffsets);
    #endif

    return (numObjects / 2) > numPlacedObjects;
}

static bool8 TrySetPyramidObjectEventPositionInSquare(u8 objType, u8 *floorLayoutOffsets, u8 squareId, u8 objectEventId)
{
    int x, y;

    if (gSaveBlock2Ptr->frontier.pyramidRandoms[0] & 1)
    {
        for (y = 7; y > -1; y--)
        {
            for (x = 7; x > -1; x--)
            {
                if (!TrySetPyramidObjectEventPositionAtCoords(objType, x, y, floorLayoutOffsets, squareId, objectEventId))
                    return FALSE;
            }
        }
    }
    else
    {
        for (y = 0; y < 8; y++)
        {
            for (x = 0; x < 8; x++)
            {
                if (!TrySetPyramidObjectEventPositionAtCoords(objType, x, y, floorLayoutOffsets, squareId, objectEventId))
                    return FALSE;
            }
        }
    }

    return TRUE;
}

static bool8 TrySetPyramidObjectEventPositionAtCoords(u8 objType, u8 x, u8 y, u8 *floorLayoutOffsets, u8 squareId, u8 objectEventId)
{
    int i, j;
    const struct MapHeader *mapHeader;
    struct ObjectEventTemplate *floorEvents = gSaveBlock1Ptr->objectEventTemplates;

    mapHeader = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(BATTLE_PYRAMID_SQUARE01), floorLayoutOffsets[squareId] + MAP_NUM(BATTLE_PYRAMID_SQUARE01));
    for (i = 0; i < mapHeader->events->objectEventCount; i++)
    {
        if (mapHeader->events->objectEvents[i].x != x || mapHeader->events->objectEvents[i].y != y)
            continue;

        if (objType != OBJ_TRAINERS || mapHeader->events->objectEvents[i].graphicsId == OBJ_EVENT_GFX_ITEM_BALL)
        {
            if (objType != OBJ_ITEMS || mapHeader->events->objectEvents[i].graphicsId != OBJ_EVENT_GFX_ITEM_BALL)
                continue;
        }

        // Ensure an object wasn't previously placed in the exact same position.
        for (j = 0; j < objectEventId; j++)
        {
            if (floorEvents[j].x == x + ((squareId % 4) * 8) && floorEvents[j].y == y + ((squareId / 4) * 8))
                break;
        }

        if (j == objectEventId)
        {
            floorEvents[objectEventId] = mapHeader->events->objectEvents[i];
            floorEvents[objectEventId].x += (squareId % 4) * 8;
            floorEvents[objectEventId].y += (squareId / 4) * 8;
            floorEvents[objectEventId].localId = objectEventId + 1;
            if (floorEvents[objectEventId].graphicsId != OBJ_EVENT_GFX_ITEM_BALL)
            {
                i = GetUniqueTrainerId(objectEventId);
                floorEvents[objectEventId].graphicsId = GetBattleFacilityTrainerGfxId(i);
                gSaveBlock2Ptr->frontier.trainerIds[objectEventId] = i;
            }
            return FALSE;
        }
    }

    return TRUE;
}

static void GetPyramidFloorLayoutOffsets(u8 *layoutOffsets)
{
    int i;
    int rand = (gSaveBlock2Ptr->frontier.pyramidRandoms[0]) | (gSaveBlock2Ptr->frontier.pyramidRandoms[1] << 16);
    u8 id = GetPyramidFloorTemplateId();

    for (i = 0; i < NUM_PYRAMID_FLOOR_SQUARES; i++)
    {
        layoutOffsets[i] = sPyramidFloorTemplates[id].layoutOffsets[MOD(rand, NUM_LAYOUT_OFFSETS)];
        rand >>= 3;
        if (i == 7)
        {
            rand = (gSaveBlock2Ptr->frontier.pyramidRandoms[2]) | (gSaveBlock2Ptr->frontier.pyramidRandoms[3] << 16);
            rand >>= 8;
        }
    }
}

static u8 GetPyramidFloorTemplateId(void)
{
    int i;
    int rand = gSaveBlock2Ptr->frontier.pyramidRandoms[3] % 100;
    int floor = gSaveBlock2Ptr->frontier.curChallengeBattleNum;

    for (i = sFloorTemplateOffsets[floor]; i < ARRAY_COUNT(sPyramidFloorTemplateOptions); i++)
    {
        if (rand < sPyramidFloorTemplateOptions[i][0])
            return sPyramidFloorTemplateOptions[i][1];
    }
    return 0;
}

u8 GetNumBattlePyramidObjectEvents(void)
{
    u8 i;
    struct ObjectEventTemplate *events = gSaveBlock1Ptr->objectEventTemplates;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (events[i].localId == 0)
            break;
    }

    return i;
}

static void InitPyramidBagItems(u8 lvlMode)
{
    int i;

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        gSaveBlock2Ptr->frontier.pyramidBag.itemId[lvlMode][i] = ITEM_NONE;
        gSaveBlock2Ptr->frontier.pyramidBag.quantity[lvlMode][i] = ITEM_NONE;
    }

    AddPyramidBagItem(ITEM_HYPER_POTION, 1);
    AddPyramidBagItem(ITEM_ETHER, 1);
}

u16 GetBattlePyramidPickupItemId(void)
{
    int rand;
    u32 i;
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    int round = (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] / FRONTIER_STAGES_PER_CHALLENGE);

    if (round >= TOTAL_PYRAMID_ROUNDS)
        round = TOTAL_PYRAMID_ROUNDS - 1;

    rand = Random() % 100;

    for (i = 0; i < ARRAY_COUNT(sPickupPercentages); i++)
    {
        if (sPickupPercentages[i] > rand)
            break;
    }

    if (i >= PICKUP_ITEMS_PER_ROUND)
        i = PICKUP_ITEMS_PER_ROUND - 1;

    if (lvlMode != FRONTIER_LVL_50)
        return sPickupItemsLvlOpen[round][i];
    else
        return sPickupItemsLvl50[round][i];
}
