#include "global.h"
#include "frontier_util.h"
#include "event_data.h"
#include "battle_setup.h"
#include "overworld.h"
#include "random.h"
#include "battle_tower.h"
#include "field_specials.h"
#include "battle.h"
#include "script_pokemon_util.h"
#include "main.h"
#include "window.h"
#include "menu.h"
#include "text.h"
#include "battle_records.h"
#include "international_string_util.h"
#include "string_util.h"
#include "new_game.h"
#include "link.h"
#include "tv.h"
#include "apprentice.h"
#include "pokedex.h"
#include "recorded_battle.h"
#include "data.h"
#include "record_mixing.h"
#include "strings.h"
#include "malloc.h"
#include "save.h"
#include "load_save.h"
#include "battle_dome.h"
#include "constants/battle_frontier.h"
#include "constants/battle_pike.h"
#include "constants/frontier_util.h"
#include "constants/trainers.h"
#include "constants/game_stat.h"
#include "constants/moves.h"
#include "constants/items.h"
#include "constants/event_objects.h"
#include "party_menu.h"

struct FrontierBrainMon
{
    u16 species;
    u16 heldItem;
    u8 fixedIV;
    u8 nature;
    u8 evs[NUM_STATS];
    u16 moves[MAX_MON_MOVES];
};

// This file's functions.
static void GetChallengeStatus(void);
static void GetFrontierData(void);
static void SetFrontierData(void);
static void SetSelectedPartyOrder(void);
static void DoSoftReset_(void);
static void SetFrontierTrainers(void);
static void SaveSelectedParty(void);
static void ShowFacilityResultsWindow(void);
static void CheckPutFrontierTVShowOnAir(void);
static void Script_GetFrontierBrainStatus(void);
static void IsTrainerFrontierBrain(void);
static void GiveBattlePoints(void);
static void GetFacilitySymbolCount(void);
static void GiveFacilitySymbol(void);
static void CheckBattleTypeFlag(void);
static void CheckPartyIneligibility(void);
static void ValidateVisitingTrainer(void);
static void IncrementWinStreak(void);
static void RestoreHeldItems(void);
static void SaveRecordBattle(void);
static void BufferFrontierTrainerName(void);
static void ResetSketchedMoves(void);
static void SetFacilityBrainObjectEvent(void);
static void ShowTowerResultsWindow(u8);
static void ShowDomeResultsWindow(u8);
static void ShowPalaceResultsWindow(u8);
static void ShowPikeResultsWindow(void);
static void ShowFactoryResultsWindow(u8);
static void ShowArenaResultsWindow(void);
static void ShowPyramidResultsWindow(void);
static void ShowLinkContestResultsWindow(void);
static void CopyFrontierBrainText(bool8 playerWonText);

// const rom data
static const u8 sFrontierBrainStreakAppearances[NUM_FRONTIER_FACILITIES][4] =
{
    [FRONTIER_FACILITY_TOWER]   = {35,  70, 35, 1},
    [FRONTIER_FACILITY_DOME]    = { 4,   9,  5, 0},
    [FRONTIER_FACILITY_PALACE]  = {21,  42, 21, 1},
    [FRONTIER_FACILITY_ARENA]   = {28,  56, 28, 1},
    [FRONTIER_FACILITY_FACTORY] = {21,  42, 21, 1},
    [FRONTIER_FACILITY_PIKE]    = {28, 140, 56, 1},
    [FRONTIER_FACILITY_PYRAMID] = {21,  70, 35, 0},
};

static const struct FrontierBrainMon sFrontierBrainsMons[][2][FRONTIER_PARTY_SIZE] =
{
    [FRONTIER_FACILITY_TOWER] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_ALAKAZAM,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = 24,
                .nature = NATURE_MODEST,
                .evs = {106, 0, 152, 152, 100, 0},
                .moves = {MOVE_THUNDER_PUNCH, MOVE_FIRE_PUNCH, MOVE_ICE_PUNCH, MOVE_DISABLE},
            },
            {
                .species = SPECIES_ENTEI,
                .heldItem = ITEM_LUM_BERRY,
                .fixedIV = 24,
                .nature = NATURE_LONELY,
                .evs = {100, 152, 152, 0, 100, 6},
                .moves = {MOVE_FIRE_BLAST, MOVE_CALM_MIND, MOVE_RETURN, MOVE_ROAR},
            },
            {
                .species = SPECIES_SNORLAX,
                .heldItem = ITEM_QUICK_CLAW,
                .fixedIV = 24,
                .nature = NATURE_ADAMANT,
                .evs = {152, 152, 0, 0, 106, 100},
                .moves = {MOVE_BODY_SLAM, MOVE_BELLY_DRUM, MOVE_YAWN, MOVE_SHADOW_BALL},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_RAIKOU,
                .heldItem = ITEM_LUM_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MODEST,
                .evs = {158, 0, 252, 100, 0, 0},
                .moves = {MOVE_THUNDERBOLT, MOVE_CALM_MIND, MOVE_REFLECT, MOVE_REST},
            },
            {
                .species = SPECIES_LATIOS,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MODEST,
                .evs = {252, 0, 252, 6, 0, 0},
                .moves = {MOVE_PSYCHIC, MOVE_CALM_MIND, MOVE_RECOVER, MOVE_DRAGON_CLAW},
            },
            {
                .species = SPECIES_SNORLAX,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_ADAMANT,
                .evs = {252, 252, 0, 0, 6, 0},
                .moves = {MOVE_CURSE, MOVE_RETURN, MOVE_REST, MOVE_SHADOW_BALL},
            },
        },
    },
    [FRONTIER_FACILITY_DOME] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_SWAMPERT,
                .heldItem = ITEM_FOCUS_BAND,
                .fixedIV = 20,
                .nature = NATURE_BRAVE,
                .evs = {152, 152, 106, 0, 100, 0},
                .moves = {MOVE_SURF, MOVE_EARTHQUAKE, MOVE_ICE_BEAM, MOVE_COUNTER},
            },
            {
                .species = SPECIES_SALAMENCE,
                .heldItem = ITEM_LUM_BERRY,
                .fixedIV = 20,
                .nature = NATURE_ADAMANT,
                .evs = {152, 152, 106, 100, 0, 0},
                .moves = {MOVE_EARTHQUAKE, MOVE_BRICK_BREAK, MOVE_DRAGON_CLAW, MOVE_AERIAL_ACE},
            },
            {
                .species = SPECIES_CHARIZARD,
                .heldItem = ITEM_WHITE_HERB,
                .fixedIV = 20,
                .nature = NATURE_QUIET,
                .evs = {100, 152, 106, 152, 0, 0},
                .moves = {MOVE_OVERHEAT, MOVE_ROCK_SLIDE, MOVE_AERIAL_ACE, MOVE_EARTHQUAKE},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_SWAMPERT,
                .heldItem = ITEM_LEFTOVERS,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BRAVE,
                .evs = {252, 252, 6, 0, 0, 0},
                .moves = {MOVE_SURF, MOVE_EARTHQUAKE, MOVE_ICE_BEAM, MOVE_MIRROR_COAT},
            },
            {
                .species = SPECIES_METAGROSS,
                .heldItem = ITEM_QUICK_CLAW,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BRAVE,
                .evs = {252, 252, 6, 0, 0, 0},
                .moves = {MOVE_PSYCHIC, MOVE_METEOR_MASH, MOVE_EARTHQUAKE, MOVE_PROTECT},
            },
            {
                .species = SPECIES_LATIAS,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MODEST,
                .evs = {252, 0, 252, 6, 0, 0},
                .moves = {MOVE_THUNDERBOLT, MOVE_PSYCHIC, MOVE_CALM_MIND, MOVE_REST},
            },
        },
    },
    [FRONTIER_FACILITY_PALACE] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_CROBAT,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = 16,
                .nature = NATURE_ADAMANT,
                .evs = {152, 0, 0, 152, 100, 106},
                .moves = {MOVE_CONFUSE_RAY, MOVE_DOUBLE_TEAM, MOVE_TOXIC, MOVE_FLY},
            },
            {
                .species = SPECIES_SLAKING,
                .heldItem = ITEM_SCOPE_LENS,
                .fixedIV = 16,
                .nature = NATURE_HARDY,
                .evs = {152, 152, 0, 106, 100, 0},
                .moves = {MOVE_EARTHQUAKE, MOVE_SWAGGER, MOVE_SHADOW_BALL, MOVE_BRICK_BREAK},
            },
            {
                .species = SPECIES_LAPRAS,
                .heldItem = ITEM_QUICK_CLAW,
                .fixedIV = 16,
                .nature = NATURE_QUIET,
                .evs = {0, 0, 252, 0, 106, 152},
                .moves = {MOVE_ICE_BEAM, MOVE_HORN_DRILL, MOVE_CONFUSE_RAY, MOVE_PROTECT},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_ARCANINE,
                .heldItem = ITEM_WHITE_HERB,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_HASTY,
                .evs = {6, 252, 252, 0, 0, 0},
                .moves = {MOVE_OVERHEAT, MOVE_EXTREME_SPEED, MOVE_ROAR, MOVE_PROTECT},
            },
            {
                .species = SPECIES_SLAKING,
                .heldItem = ITEM_SCOPE_LENS,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_HARDY,
                .evs = {6, 252, 0, 252, 0, 0},
                .moves = {MOVE_HYPER_BEAM, MOVE_EARTHQUAKE, MOVE_SHADOW_BALL, MOVE_YAWN},
            },
            {
                .species = SPECIES_SUICUNE,
                .heldItem = ITEM_KINGS_ROCK,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_HASTY,
                .evs = {252, 0, 252, 6, 0, 0},
                .moves = {MOVE_BLIZZARD, MOVE_SURF, MOVE_BITE, MOVE_CALM_MIND},
            },
        },
    },
    [FRONTIER_FACILITY_ARENA] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_HERACROSS,
                .heldItem = ITEM_SALAC_BERRY,
                .fixedIV = 20,
                .nature = NATURE_JOLLY,
                .evs = {106, 152, 0, 152, 0, 100},
                .moves = {MOVE_MEGAHORN, MOVE_ROCK_TOMB, MOVE_ENDURE, MOVE_REVERSAL},
            },
            {
                .species = SPECIES_UMBREON,
                .heldItem = ITEM_LEFTOVERS,
                .fixedIV = 20,
                .nature = NATURE_CALM,
                .evs = {152, 0, 100, 0, 152, 106},
                .moves = {MOVE_BODY_SLAM, MOVE_CONFUSE_RAY, MOVE_PSYCHIC, MOVE_FAINT_ATTACK},
            },
            {
                .species = SPECIES_SHEDINJA,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = 20,
                .nature = NATURE_ADAMANT,
                .evs = {0, 252, 6, 252, 0, 0},
                .moves = {MOVE_SHADOW_BALL, MOVE_RETURN, MOVE_CONFUSE_RAY, MOVE_AERIAL_ACE},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_UMBREON,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_CALM,
                .evs = {252, 0, 0, 0, 252, 6},
                .moves = {MOVE_DOUBLE_EDGE, MOVE_CONFUSE_RAY, MOVE_REST, MOVE_PSYCHIC},
            },
            {
                .species = SPECIES_GENGAR,
                .heldItem = ITEM_LEFTOVERS,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MODEST,
                .evs = {252, 0, 252, 0, 6, 0},
                .moves = {MOVE_PSYCHIC, MOVE_HYPNOSIS, MOVE_DREAM_EATER, MOVE_DESTINY_BOND},
            },
            {
                .species = SPECIES_BRELOOM,
                .heldItem = ITEM_LUM_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_JOLLY,
                .evs = {6, 252, 0, 252, 0, 0},
                .moves = {MOVE_SPORE, MOVE_FOCUS_PUNCH, MOVE_GIGA_DRAIN, MOVE_HEADBUTT},
            },
        },
    },
    [FRONTIER_FACILITY_FACTORY] =
    {
        // Because Factory's Pokémon are random, this facility's Brain also uses random Pokémon.
        // What is interesting, this team is actually the one Steven uses in the multi tag battle alongside the player.
        {
            {
                .species = SPECIES_METANG,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BRAVE,
                .evs = {0, 252, 252, 0, 6, 0},
                .moves = {MOVE_LIGHT_SCREEN, MOVE_PSYCHIC, MOVE_REFLECT, MOVE_METAL_CLAW},
            },
            {
                .species = SPECIES_SKARMORY,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_IMPISH,
                .evs = {252, 0, 0, 0, 6, 252},
                .moves = {MOVE_TOXIC, MOVE_AERIAL_ACE, MOVE_PROTECT, MOVE_STEEL_WING},
            },
            {
                .species = SPECIES_AGGRON,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_ADAMANT,
                .evs = {0, 252, 0, 0, 252, 6},
                .moves = {MOVE_THUNDERBOLT, MOVE_PROTECT, MOVE_SOLAR_BEAM, MOVE_DRAGON_CLAW},
            },
        },
        {
            {
                .species = SPECIES_METANG,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BRAVE,
                .evs = {0, 252, 252, 0, 6, 0},
                .moves = {MOVE_LIGHT_SCREEN, MOVE_PSYCHIC, MOVE_REFLECT, MOVE_METAL_CLAW},
            },
            {
                .species = SPECIES_SKARMORY,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_IMPISH,
                .evs = {252, 0, 0, 0, 6, 252},
                .moves = {MOVE_TOXIC, MOVE_AERIAL_ACE, MOVE_PROTECT, MOVE_STEEL_WING},
            },
            {
                .species = SPECIES_AGGRON,
                .heldItem = ITEM_SITRUS_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_ADAMANT,
                .evs = {0, 252, 0, 0, 252, 6},
                .moves = {MOVE_THUNDERBOLT, MOVE_PROTECT, MOVE_SOLAR_BEAM, MOVE_DRAGON_CLAW},
            },
        },
    },
    [FRONTIER_FACILITY_PIKE] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_SEVIPER,
                .heldItem = ITEM_QUICK_CLAW,
                .fixedIV = 16,
                .nature = NATURE_BRAVE,
                .evs = {252, 0, 252, 0, 6, 0},
                .moves = {MOVE_SWAGGER, MOVE_CRUNCH, MOVE_POISON_FANG, MOVE_GIGA_DRAIN},
            },
            {
                .species = SPECIES_SHUCKLE,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = 16,
                .nature = NATURE_BOLD,
                .evs = {252, 0, 0, 0, 106, 252},
                .moves = {MOVE_TOXIC, MOVE_SANDSTORM, MOVE_PROTECT, MOVE_REST},
            },
            {
                .species = SPECIES_MILOTIC,
                .heldItem = ITEM_LEFTOVERS,
                .fixedIV = 16,
                .nature = NATURE_MODEST,
                .evs = {152, 0, 100, 0, 152, 106},
                .moves = {MOVE_ICE_BEAM, MOVE_MIRROR_COAT, MOVE_SURF, MOVE_RECOVER},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_SEVIPER,
                .heldItem = ITEM_FOCUS_BAND,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BOLD,
                .evs = {252, 0, 0, 0, 252, 6},
                .moves = {MOVE_SWAGGER, MOVE_CRUNCH, MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN},
            },
            {
                .species = SPECIES_STEELIX,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_BRAVE,
                .evs = {252, 0, 0, 0, 6, 252},
                .moves = {MOVE_EARTHQUAKE, MOVE_ROCK_SLIDE, MOVE_EXPLOSION, MOVE_SCREECH},
            },
            {
                .species = SPECIES_GYARADOS,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_ADAMANT,
                .evs = {252, 6, 0, 0, 0, 252},
                .moves = {MOVE_DRAGON_DANCE, MOVE_RETURN, MOVE_ROAR, MOVE_REST},
            },
        },
    },
    [FRONTIER_FACILITY_PYRAMID] =
    {
        // Silver Symbol.
        {
            {
                .species = SPECIES_REGIROCK,
                .heldItem = ITEM_QUICK_CLAW,
                .fixedIV = 16,
                .nature = NATURE_ADAMANT,
                .evs = {152, 152, 0, 0, 106, 100},
                .moves = {MOVE_EXPLOSION, MOVE_SUPERPOWER, MOVE_EARTHQUAKE, MOVE_ANCIENT_POWER},
            },
            {
                .species = SPECIES_REGISTEEL,
                .heldItem = ITEM_LEFTOVERS,
                .fixedIV = 16,
                .nature = NATURE_ADAMANT,
                .evs = {152, 152, 0, 0, 6, 200},
                .moves = {MOVE_EARTHQUAKE, MOVE_METAL_CLAW, MOVE_TOXIC, MOVE_IRON_DEFENSE},
            },
            {
                .species = SPECIES_REGICE,
                .heldItem = ITEM_CHESTO_BERRY,
                .fixedIV = 16,
                .nature = NATURE_MODEST,
                .evs = {106, 0, 152, 0, 100, 152},
                .moves = {MOVE_ICE_BEAM, MOVE_AMNESIA, MOVE_THUNDER, MOVE_REST},
            },
        },
        // Gold Symbol.
        {
            {
                .species = SPECIES_ARTICUNO,
                .heldItem = ITEM_SCOPE_LENS,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MILD,
                .evs = {6, 0, 252, 252, 0, 0},
                .moves = {MOVE_BLIZZARD, MOVE_WATER_PULSE, MOVE_AERIAL_ACE, MOVE_REFLECT},
            },
            {
                .species = SPECIES_ZAPDOS,
                .heldItem = ITEM_LUM_BERRY,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MILD,
                .evs = {6, 0, 252, 252, 0, 0},
                .moves = {MOVE_THUNDER, MOVE_DETECT, MOVE_DRILL_PECK, MOVE_LIGHT_SCREEN},
            },
            {
                .species = SPECIES_MOLTRES,
                .heldItem = ITEM_BRIGHT_POWDER,
                .fixedIV = MAX_PER_STAT_IVS,
                .nature = NATURE_MILD,
                .evs = {6, 0, 252, 252, 0, 0},
                .moves = {MOVE_FIRE_BLAST, MOVE_HYPER_BEAM, MOVE_AERIAL_ACE, MOVE_SAFEGUARD},
            },
        },
    },
};

static const u8 sBattlePointAwards[][NUM_FRONTIER_FACILITIES][FRONTIER_MODE_COUNT] =
{
    {
        {1, 2, 3, 3}, {1, 1}, {4, 5}, {1}, {3, 4}, {1}, {5}
    },
    {
        {2, 3, 4, 4}, {1, 1}, {4, 5}, {1}, {3, 4}, {1}, {5}
    },
    {
        {3, 4, 5, 5}, {2, 2}, {5, 6}, {1}, {4, 5}, {2}, {6}
    },
    {
        {4, 5, 6, 6}, {2, 2}, {5, 6}, {2}, {4, 5}, {2}, {6}
    },
    {
        {5, 6, 7, 7}, {3, 3}, {6, 7}, {2}, {5, 6}, {2}, {7}
    },
    {
        {6, 7, 8, 8}, {3, 3}, {6, 7}, {2}, {5, 6}, {4}, {7}
    },
    {
        {7, 8, 9, 9}, {4, 4}, {7, 8}, {3}, {6, 7}, {4}, {8}
    },
    {
        {8, 9, 10, 10}, {4, 4}, {7, 8}, {3},{6, 7}, {4}, {8}
    },
    {
        {9, 10, 11, 11}, {5, 5}, {8, 9}, {4}, {7, 8}, {8}, {9}
    },
    {
        {10, 11, 12, 12}, {5, 5}, {8, 9}, {4}, {7, 8}, {8}, {9}
    },
    {
        {11, 12, 13, 13}, {6, 6}, {9, 10}, {5,0}, {8, 9}, {8}, {10}
    },
    {
        {12, 13, 14, 14}, {6, 6}, {9, 10}, {6,0}, {8, 9}, {8}, {10}
    },
    {
        {13, 14, 15, 15}, {7, 7}, {10, 11}, {7}, {9, 10}, {10}, {11}
    },
    {
        {14, 15, 15, 15}, {7, 7}, {10, 11}, {8}, {9, 10}, {10}, {11}
    },
    {
        {15, 15, 15, 15}, {8, 8}, {11, 12}, {9}, {10, 11}, {10}, {12}
    },
    {
        {15, 15, 15, 15}, {8, 8}, {11, 12}, {10}, {10, 11}, {10}, {12}
    },
    {
        {15, 15, 15, 15}, {9, 9}, {12, 13}, {11}, {11, 12}, {12}, {13}
    },
    {
        {15, 15, 15, 15}, {9, 9}, {12, 13}, {12}, {11, 12}, {12}, {13}
    },
    {
        {15, 15, 15, 15}, {10, 10}, {13, 14}, {13}, {12, 13}, {12}, {14}
    },
    {
        {15, 15, 15, 15}, {10, 10}, {13, 14}, {14}, {12, 13}, {12}, {14}
    },
    {
        {15, 15, 15, 15}, {11, 11}, {14, 15}, {15}, {13, 14}, {12}, {15}
    },
    {
        {15, 15, 15, 15}, {11, 11}, {14, 15}, {15}, {13, 14}, {14}, {15}
    },
    {
        {15, 15, 15, 15}, {12, 12}, {15, 15}, {15}, {14, 15}, {14}, {15}
    },
    {
        {15, 15, 15, 15}, {12, 12}, {15, 15}, {15}, {14, 15}, {14}, {15}
    },
    {
        {15, 15, 15, 15}, {13, 13}, {15, 15}, {15}, {15, 15}, {14}, {15}
    },
    {
        {15, 15, 15, 15}, {13, 13}, {15, 15}, {15}, {15, 15}, {15}, {15}
    },
    {
        {15, 15, 15, 15}, {14, 14}, {15, 15}, {15}, {15, 15}, {15}, {15}
    },
    {
        {15, 15, 15, 15}, {14, 14}, {15, 15}, {15}, {15, 15}, {15}, {15}
    },
    {
        {15, 15, 15, 15}, {15, 15}, {15, 15}, {15}, {15, 15}, {15}, {15}
    },
    {
        {15, 15, 15, 15}, {15, 15}, {15, 15}, {15}, {15, 15}, {15}, {15}
    },
};

// Flags to change the conversation when the Frontier Brain is encountered for a battle
// First bit is has battled them before and not won yet, second bit is has battled them and won (obtained a Symbol)
static const u16 sBattledBrainBitFlags[NUM_FRONTIER_FACILITIES][2] =
{
    [FRONTIER_FACILITY_TOWER]   = {1 << 0, 1 << 1},
    [FRONTIER_FACILITY_DOME]    = {1 << 2, 1 << 3},
    [FRONTIER_FACILITY_PALACE]  = {1 << 4, 1 << 5},
    [FRONTIER_FACILITY_ARENA]   = {1 << 6, 1 << 7},
    [FRONTIER_FACILITY_FACTORY] = {1 << 8, 1 << 9},
    [FRONTIER_FACILITY_PIKE]    = {1 << 10, 1 << 11},
    [FRONTIER_FACILITY_PYRAMID] = {1 << 12, 1 << 13},
};

static void (* const sFrontierUtilFuncs[])(void) =
{
    [FRONTIER_UTIL_FUNC_GET_STATUS]            = GetChallengeStatus,
    [FRONTIER_UTIL_FUNC_GET_DATA]              = GetFrontierData,
    [FRONTIER_UTIL_FUNC_SET_DATA]              = SetFrontierData,
    [FRONTIER_UTIL_FUNC_SET_PARTY_ORDER]       = SetSelectedPartyOrder,
    [FRONTIER_UTIL_FUNC_SOFT_RESET]            = DoSoftReset_,
    [FRONTIER_UTIL_FUNC_SET_TRAINERS]          = SetFrontierTrainers,
    [FRONTIER_UTIL_FUNC_SAVE_PARTY]            = SaveSelectedParty,
    [FRONTIER_UTIL_FUNC_RESULTS_WINDOW]        = ShowFacilityResultsWindow,
    [FRONTIER_UTIL_FUNC_CHECK_AIR_TV_SHOW]     = CheckPutFrontierTVShowOnAir,
    [FRONTIER_UTIL_FUNC_GET_BRAIN_STATUS]      = Script_GetFrontierBrainStatus,
    [FRONTIER_UTIL_FUNC_IS_BRAIN]              = IsTrainerFrontierBrain,
    [FRONTIER_UTIL_FUNC_GIVE_BATTLE_POINTS]    = GiveBattlePoints,
    [FRONTIER_UTIL_FUNC_GET_FACILITY_SYMBOLS]  = GetFacilitySymbolCount,
    [FRONTIER_UTIL_FUNC_GIVE_FACILITY_SYMBOL]  = GiveFacilitySymbol,
    [FRONTIER_UTIL_FUNC_CHECK_BATTLE_TYPE]     = CheckBattleTypeFlag,
    [FRONTIER_UTIL_FUNC_CHECK_INELIGIBLE]      = CheckPartyIneligibility,
    [FRONTIER_UTIL_FUNC_CHECK_VISIT_TRAINER]   = ValidateVisitingTrainer,
    [FRONTIER_UTIL_FUNC_INCREMENT_STREAK]      = IncrementWinStreak,
    [FRONTIER_UTIL_FUNC_RESTORE_HELD_ITEMS]    = RestoreHeldItems,
    [FRONTIER_UTIL_FUNC_SAVE_BATTLE]           = SaveRecordBattle,
    [FRONTIER_UTIL_FUNC_BUFFER_TRAINER_NAME]   = BufferFrontierTrainerName,
    [FRONTIER_UTIL_FUNC_RESET_SKETCH_MOVES]    = ResetSketchedMoves,
    [FRONTIER_UTIL_FUNC_SET_BRAIN_OBJECT]      = SetFacilityBrainObjectEvent,
};

static const struct WindowTemplate sFrontierResultsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 28,
    .height = 18,
    .paletteNum = 15,
    .baseBlock = 1
};

static const struct WindowTemplate sLinkContestResultsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 2,
    .width = 26,
    .height = 15,
    .paletteNum = 15,
    .baseBlock = 1
};

static const struct WindowTemplate sRankingHallRecordsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 1,
    .width = 26,
    .height = 17,
    .paletteNum = 15,
    .baseBlock = 1
};

// Second field - whether the character is female.
static const u8 sFrontierBrainObjEventGfx[NUM_FRONTIER_FACILITIES][2] =
{
    [FRONTIER_FACILITY_TOWER]   = {OBJ_EVENT_GFX_ANABEL,  TRUE},
    [FRONTIER_FACILITY_DOME]    = {OBJ_EVENT_GFX_TUCKER,  FALSE},
    [FRONTIER_FACILITY_PALACE]  = {OBJ_EVENT_GFX_SPENSER, FALSE},
    [FRONTIER_FACILITY_ARENA]   = {OBJ_EVENT_GFX_GRETA,   TRUE},
    [FRONTIER_FACILITY_FACTORY] = {OBJ_EVENT_GFX_NOLAND,  FALSE},
    [FRONTIER_FACILITY_PIKE]    = {OBJ_EVENT_GFX_LUCY,    TRUE},
    [FRONTIER_FACILITY_PYRAMID] = {OBJ_EVENT_GFX_BRANDON, FALSE},
};

const u16 gFrontierBannedSpecies[] =
{
    SPECIES_MEW, SPECIES_MEWTWO, SPECIES_HO_OH, SPECIES_LUGIA, SPECIES_CELEBI,
    SPECIES_KYOGRE, SPECIES_GROUDON, SPECIES_RAYQUAZA, SPECIES_JIRACHI, SPECIES_DEOXYS, 0xFFFF
};

static const u8 *const sRecordsWindowChallengeTexts[][2] =
{
    [RANKING_HALL_TOWER_SINGLES] = {gText_BattleTower2,  gText_FacilitySingle},
    [RANKING_HALL_TOWER_DOUBLES] = {gText_BattleTower2,  gText_FacilityDouble},
    [RANKING_HALL_TOWER_MULTIS]  = {gText_BattleTower2,  gText_FacilityMulti},
    [RANKING_HALL_DOME]          = {gText_BattleDome,    gText_FacilitySingle},
    [RANKING_HALL_PALACE]        = {gText_BattlePalace,  gText_FacilitySingle},
    [RANKING_HALL_ARENA]         = {gText_BattleArena,   gText_Facility},
    [RANKING_HALL_FACTORY]       = {gText_BattleFactory, gText_FacilitySingle},
    [RANKING_HALL_PIKE]          = {gText_BattlePike,    gText_Facility},
    [RANKING_HALL_PYRAMID]       = {gText_BattlePyramid, gText_Facility},
    [RANKING_HALL_TOWER_LINK]    = {gText_BattleTower2,  gText_FacilityLink},
};

static const u8 *const sLevelModeText[] =
{
    [FRONTIER_LVL_50]   = gText_RecordsLv50,
    [FRONTIER_LVL_OPEN] = gText_RecordsOpenLevel,
};

static const u8 *const sHallFacilityToRecordsText[] =
{
    [RANKING_HALL_TOWER_SINGLES] = gText_FrontierFacilityWinStreak,
    [RANKING_HALL_TOWER_DOUBLES] = gText_FrontierFacilityWinStreak,
    [RANKING_HALL_TOWER_MULTIS]  = gText_FrontierFacilityWinStreak,
    [RANKING_HALL_DOME]          = gText_FrontierFacilityClearStreak,
    [RANKING_HALL_PALACE]        = gText_FrontierFacilityWinStreak,
    [RANKING_HALL_ARENA]         = gText_FrontierFacilityKOsStreak,
    [RANKING_HALL_FACTORY]       = gText_FrontierFacilityWinStreak,
    [RANKING_HALL_PIKE]          = gText_FrontierFacilityRoomsCleared,
    [RANKING_HALL_PYRAMID]       = gText_FrontierFacilityFloorsCleared,
    [RANKING_HALL_TOWER_LINK]    = gText_FrontierFacilityWinStreak,
};

static const u16 sFrontierBrainTrainerIds[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = TRAINER_ANABEL,
    [FRONTIER_FACILITY_DOME]    = TRAINER_TUCKER,
    [FRONTIER_FACILITY_PALACE]  = TRAINER_SPENSER,
    [FRONTIER_FACILITY_ARENA]   = TRAINER_GRETA,
    [FRONTIER_FACILITY_FACTORY] = TRAINER_NOLAND,
    [FRONTIER_FACILITY_PIKE]    = TRAINER_LUCY,
    [FRONTIER_FACILITY_PYRAMID] = TRAINER_BRANDON,
};

static const u8 *const sFrontierBrainPlayerLostSilverTexts[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = gText_AnabelWonSilver,
    [FRONTIER_FACILITY_DOME]    = gText_TuckerWonSilver,
    [FRONTIER_FACILITY_PALACE]  = gText_SpenserWonSilver,
    [FRONTIER_FACILITY_ARENA]   = gText_GretaWonSilver,
    [FRONTIER_FACILITY_FACTORY] = gText_NolandWonSilver,
    [FRONTIER_FACILITY_PIKE]    = gText_LucyWonSilver,
    [FRONTIER_FACILITY_PYRAMID] = gText_BrandonWonSilver,
};

static const u8 *const sFrontierBrainPlayerWonSilverTexts[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = gText_AnabelDefeatSilver,
    [FRONTIER_FACILITY_DOME]    = gText_TuckerDefeatSilver,
    [FRONTIER_FACILITY_PALACE]  = gText_SpenserDefeatSilver,
    [FRONTIER_FACILITY_ARENA]   = gText_GretaDefeatSilver,
    [FRONTIER_FACILITY_FACTORY] = gText_NolandDefeatSilver,
    [FRONTIER_FACILITY_PIKE]    = gText_LucyDefeatSilver,
    [FRONTIER_FACILITY_PYRAMID] = gText_BrandonDefeatSilver,
};

static const u8 *const sFrontierBrainPlayerLostGoldTexts[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = gText_AnabelWonGold,
    [FRONTIER_FACILITY_DOME]    = gText_TuckerWonGold,
    [FRONTIER_FACILITY_PALACE]  = gText_SpenserWonGold,
    [FRONTIER_FACILITY_ARENA]   = gText_GretaWonGold,
    [FRONTIER_FACILITY_FACTORY] = gText_NolandWonGold,
    [FRONTIER_FACILITY_PIKE]    = gText_LucyWonGold,
    [FRONTIER_FACILITY_PYRAMID] = gText_BrandonWonGold,
};

static const u8 *const sFrontierBrainPlayerWonGoldTexts[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = gText_AnabelDefeatGold,
    [FRONTIER_FACILITY_DOME]    = gText_TuckerDefeatGold,
    [FRONTIER_FACILITY_PALACE]  = gText_SpenserDefeatGold,
    [FRONTIER_FACILITY_ARENA]   = gText_GretaDefeatGold,
    [FRONTIER_FACILITY_FACTORY] = gText_NolandDefeatGold,
    [FRONTIER_FACILITY_PIKE]    = gText_LucyDefeatGold,
    [FRONTIER_FACILITY_PYRAMID] = gText_BrandonDefeatGold,
};

static const u8 *const *const sFrontierBrainPlayerLostTexts[] =
{
    sFrontierBrainPlayerLostSilverTexts,
    sFrontierBrainPlayerLostGoldTexts,
};

static const u8 *const *const sFrontierBrainPlayerWonTexts[] =
{
    sFrontierBrainPlayerWonSilverTexts,
    sFrontierBrainPlayerWonGoldTexts,
};

// code
void CallFrontierUtilFunc(void)
{
    sFrontierUtilFuncs[gSpecialVar_0x8004]();
}

// VAR_TEMP_CHALLENGE_STATUS is used to react to the status in OnFrame map scripts
static void GetChallengeStatus(void)
{
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0xFF);
    switch (gSaveBlock2Ptr->frontier.challengeStatus)
    {
    case 0:
        break;
    case CHALLENGE_STATUS_SAVING:
        FrontierGamblerSetWonOrLost(FALSE);
        VarSet(VAR_TEMP_CHALLENGE_STATUS, gSaveBlock2Ptr->frontier.challengeStatus);
        break;
    case CHALLENGE_STATUS_LOST:
        FrontierGamblerSetWonOrLost(FALSE);
        VarSet(VAR_TEMP_CHALLENGE_STATUS, gSaveBlock2Ptr->frontier.challengeStatus);
        break;
    case CHALLENGE_STATUS_WON:
        FrontierGamblerSetWonOrLost(TRUE);
        VarSet(VAR_TEMP_CHALLENGE_STATUS, gSaveBlock2Ptr->frontier.challengeStatus);
        break;
    case CHALLENGE_STATUS_PAUSED:
        VarSet(VAR_TEMP_CHALLENGE_STATUS, gSaveBlock2Ptr->frontier.challengeStatus);
        break;
    }
}

static void GetFrontierData(void)
{
    u8 facility = VarGet(VAR_FRONTIER_FACILITY);
    u8 hasSymbol = GetPlayerSymbolCountForFacility(facility);
    if (hasSymbol == 2)
        hasSymbol = 1;

    switch (gSpecialVar_0x8005)
    {
    case FRONTIER_DATA_CHALLENGE_STATUS:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.challengeStatus;
        break;
    case FRONTIER_DATA_LVL_MODE:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.lvlMode;
        break;
    case FRONTIER_DATA_BATTLE_NUM:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        break;
    case FRONTIER_DATA_PAUSED:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.challengePaused;
        break;
    case FRONTIER_DATA_BATTLE_OUTCOME:
        gSpecialVar_Result = gBattleOutcome;
        gBattleOutcome = 0;
        break;
    case FRONTIER_DATA_RECORD_DISABLED:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.disableRecordBattle;
        break;
    case FRONTIER_DATA_HEARD_BRAIN_SPEECH:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.battledBrainFlags & sBattledBrainBitFlags[facility][hasSymbol];
        break;
    }
}

static void SetFrontierData(void)
{
    s32 i;
    u8 facility = VarGet(VAR_FRONTIER_FACILITY);
    u8 hasSymbol = GetPlayerSymbolCountForFacility(facility);
    if (hasSymbol == 2)
        hasSymbol = 1;

    switch (gSpecialVar_0x8005)
    {
    case FRONTIER_DATA_CHALLENGE_STATUS:
        gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8006;
        break;
    case FRONTIER_DATA_LVL_MODE:
        gSaveBlock2Ptr->frontier.lvlMode = gSpecialVar_0x8006;
        break;
    case FRONTIER_DATA_BATTLE_NUM:
        gSaveBlock2Ptr->frontier.curChallengeBattleNum = gSpecialVar_0x8006;
        break;
    case FRONTIER_DATA_PAUSED:
        gSaveBlock2Ptr->frontier.challengePaused = gSpecialVar_0x8006;
        break;
    case FRONTIER_DATA_SELECTED_MON_ORDER:
        for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
            gSaveBlock2Ptr->frontier.selectedPartyMons[i] = gSelectedOrderFromParty[i];
        break;
    case FRONTIER_DATA_RECORD_DISABLED:
        gSaveBlock2Ptr->frontier.disableRecordBattle = gSpecialVar_0x8006;
        break;
    case FRONTIER_DATA_HEARD_BRAIN_SPEECH:
        gSaveBlock2Ptr->frontier.battledBrainFlags |= sBattledBrainBitFlags[facility][hasSymbol];
        break;
    }
}

static void SetSelectedPartyOrder(void)
{
    s32 i;

    ClearSelectedPartyOrder();
    for (i = 0; i < gSpecialVar_0x8005; i++)
        gSelectedOrderFromParty[i] = gSaveBlock2Ptr->frontier.selectedPartyMons[i];
    ReducePlayerPartyToSelectedMons();
}

static void DoSoftReset_(void)
{
    DoSoftReset();
}

static void SetFrontierTrainers(void)
{
    gFacilityTrainers = gBattleFrontierTrainers;
}

static void SaveSelectedParty(void)
{
    u8 i;

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        u16 monId = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
        if (monId < PARTY_SIZE)
            gSaveBlock1Ptr->playerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1] = gPlayerParty[i];
    }
}

static void ShowFacilityResultsWindow(void)
{
    if (gSpecialVar_0x8006 >= FRONTIER_MODE_COUNT)
        gSpecialVar_0x8006 = 0;
    switch (gSpecialVar_0x8005)
    {
    case FRONTIER_FACILITY_TOWER:
        ShowTowerResultsWindow(gSpecialVar_0x8006);
        break;
    case FRONTIER_FACILITY_DOME:
        ShowDomeResultsWindow(gSpecialVar_0x8006);
        break;
    case FRONTIER_FACILITY_PALACE:
        ShowPalaceResultsWindow(gSpecialVar_0x8006);
        break;
    case FRONTIER_FACILITY_PIKE:
        ShowPikeResultsWindow();
        break;
    case FRONTIER_FACILITY_FACTORY:
        ShowFactoryResultsWindow(gSpecialVar_0x8006);
        break;
    case FRONTIER_FACILITY_ARENA:
        ShowArenaResultsWindow();
        break;
    case FRONTIER_FACILITY_PYRAMID:
        ShowPyramidResultsWindow();
        break;
    case FACILITY_LINK_CONTEST:
        ShowLinkContestResultsWindow();
        break;
    }
}

static bool8 IsWinStreakActive(u32 challenge)
{
    if (gSaveBlock2Ptr->frontier.winStreakActiveFlags & challenge)
        return TRUE;
    else
        return FALSE;
}

static void PrintAligned(const u8 *str, s32 y)
{
    s32 x = GetStringCenterAlignXOffset(FONT_NORMAL, str, DISPLAY_WIDTH - 16);
    y = (y * 8) + 1;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x, y, TEXT_SKIP_DRAW, NULL);
}

static void PrintHyphens(s32 y)
{
    s32 i;
    u8 text[37];

    for (i = 0; i < (int)ARRAY_COUNT(text) - 1; i++)
        text[i] = CHAR_HYPHEN;
    text[i] = EOS;

    y = (y * 8) + 1;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, text, 4, y, TEXT_SKIP_DRAW, NULL);
}

// Battle Tower records.
static void TowerPrintStreak(const u8 *str, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x1, y, TEXT_SKIP_DRAW, NULL);
    if (num > MAX_STREAK)
        num = MAX_STREAK;
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_WinStreak);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void TowerPrintRecordStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    u16 num = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[battleMode][lvlMode];
    TowerPrintStreak(gText_Record, num, x1, x2, y);
}

static u16 TowerGetWinStreak(u8 battleMode, u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void TowerPrintPrevOrCurrentStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = TowerGetWinStreak(battleMode, lvlMode);
    switch (battleMode)
    {
    default:
    case FRONTIER_MODE_SINGLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_TOWER_SINGLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_TOWER_SINGLES_50);
        break;
    case FRONTIER_MODE_DOUBLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_TOWER_DOUBLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_TOWER_DOUBLES_50);
        break;
    case FRONTIER_MODE_MULTIS:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_TOWER_MULTIS_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_TOWER_MULTIS_50);
        break;
    case FRONTIER_MODE_LINK_MULTIS:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_TOWER_LINK_MULTIS_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_TOWER_LINK_MULTIS_50);
        break;
    }

    if (isCurrent == TRUE)
        TowerPrintStreak(gText_Current, winStreak, x1, x2, y);
    else
        TowerPrintStreak(gText_Prev, winStreak, x1, x2, y);
}

static void ShowTowerResultsWindow(u8 battleMode)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    if (battleMode == FRONTIER_MODE_SINGLES)
        StringExpandPlaceholders(gStringVarBuffer, gText_SingleBattleRoomResults);
    else if (battleMode == FRONTIER_MODE_DOUBLES)
        StringExpandPlaceholders(gStringVarBuffer, gText_DoubleBattleRoomResults);
    else if (battleMode == FRONTIER_MODE_MULTIS)
        StringExpandPlaceholders(gStringVarBuffer, gText_MultiBattleRoomResults);
    else
        StringExpandPlaceholders(gStringVarBuffer, gText_LinkMultiBattleRoomResults);

    PrintAligned(gStringVarBuffer, 2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 16, 49, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 16, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    TowerPrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_50, 72, 132, 49);
    TowerPrintRecordStreak(battleMode, FRONTIER_LVL_50, 72, 132, 65);
    TowerPrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_OPEN, 72, 132, 97);
    TowerPrintRecordStreak(battleMode, FRONTIER_LVL_OPEN, 72, 132, 113);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Dome records.
static u16 DomeGetWinStreak(u8 battleMode, u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void PrintTwoStrings(const u8 *str1, const u8 *str2, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str1, x1, y, TEXT_SKIP_DRAW, NULL);
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, str2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void DomePrintPrevOrCurrentStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = DomeGetWinStreak(battleMode, lvlMode);
    switch (battleMode)
    {
    default:
    case FRONTIER_MODE_SINGLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_DOME_SINGLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_DOME_SINGLES_50);
        break;
    case FRONTIER_MODE_DOUBLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_DOME_DOUBLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_DOME_DOUBLES_50);
        break;
    }

    if (isCurrent == TRUE)
        PrintTwoStrings(gText_Current, gText_ClearStreak, winStreak, x1, x2, y);
    else
        PrintTwoStrings(gText_Prev, gText_ClearStreak, winStreak, x1, x2, y);
}

static void ShowDomeResultsWindow(u8 battleMode)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    if (battleMode == FRONTIER_MODE_SINGLES)
        StringExpandPlaceholders(gStringVarBuffer, gText_SingleBattleTourneyResults);
    else
        StringExpandPlaceholders(gStringVarBuffer, gText_DoubleBattleTourneyResults);

    PrintAligned(gStringVarBuffer, 0);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 8, 33, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 8, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    DomePrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_50, 64, 121, 33);
    PrintTwoStrings(gText_Record, gText_ClearStreak, gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][FRONTIER_LVL_50], 64, 121, 49);
    PrintTwoStrings(gText_Total, gText_Championships, gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][FRONTIER_LVL_50], 64, 112, 65);
    DomePrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_OPEN, 64, 121, 97);
    PrintTwoStrings(gText_Record, gText_ClearStreak, gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][FRONTIER_LVL_OPEN], 64, 121, 113);
    PrintTwoStrings(gText_Total, gText_Championships, gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][FRONTIER_LVL_OPEN], 64, 112, 129);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Palace records.
static void PalacePrintStreak(const u8 *str, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x1, y, TEXT_SKIP_DRAW, NULL);
    if (num > MAX_STREAK)
        num = MAX_STREAK;
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_WinStreak);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void PalacePrintRecordStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    u16 num = gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[battleMode][lvlMode];
    PalacePrintStreak(gText_Record, num, x1, x2, y);
}

static u16 PalaceGetWinStreak(u8 battleMode, u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void PalacePrintPrevOrCurrentStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = PalaceGetWinStreak(battleMode, lvlMode);
    switch (battleMode)
    {
    default:
    case FRONTIER_MODE_SINGLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_PALACE_SINGLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_PALACE_SINGLES_50);
        break;
    case FRONTIER_MODE_DOUBLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_PALACE_DOUBLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_PALACE_DOUBLES_50);
    }

    if (isCurrent == TRUE)
        PalacePrintStreak(gText_Current, winStreak, x1, x2, y);
    else
        PalacePrintStreak(gText_Prev, winStreak, x1, x2, y);
}

static void ShowPalaceResultsWindow(u8 battleMode)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    if (battleMode == FRONTIER_MODE_SINGLES)
        StringExpandPlaceholders(gStringVarBuffer, gText_SingleBattleHallResults);
    else
        StringExpandPlaceholders(gStringVarBuffer, gText_DoubleBattleHallResults);

    PrintAligned(gStringVarBuffer, 2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 16, 49, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 16, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    PalacePrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_50, 72, 131, 49);
    PalacePrintRecordStreak(battleMode, FRONTIER_LVL_50, 72, 131, 65);
    PalacePrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_OPEN, 72, 131, 97);
    PalacePrintRecordStreak(battleMode, FRONTIER_LVL_OPEN, 72, 131, 113);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Pike records.
static u16 PikeGetWinStreak(u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void PikePrintCleared(const u8 *str1, const u8 *str2, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str1, x1, y, TEXT_SKIP_DRAW, NULL);
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, str2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void PikePrintPrevOrCurrentStreak(u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = PikeGetWinStreak(lvlMode);

    if (lvlMode != FRONTIER_LVL_50)
        isCurrent = IsWinStreakActive(STREAK_PIKE_OPEN);
    else
        isCurrent = IsWinStreakActive(STREAK_PIKE_50);

    if (isCurrent == TRUE)
        PrintTwoStrings(gText_Current, gText_RoomsCleared, winStreak, x1, x2, y);
    else
        PrintTwoStrings(gText_Prev, gText_RoomsCleared, winStreak, x1, x2, y);
}

static void ShowPikeResultsWindow(void)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    StringExpandPlaceholders(gStringVarBuffer, gText_BattleChoiceResults);
    PrintAligned(gStringVarBuffer, 0);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 8, 33, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 8, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    PikePrintPrevOrCurrentStreak(FRONTIER_LVL_50, 64, 114, 33);
    PikePrintCleared(gText_Record, gText_RoomsCleared, gSaveBlock2Ptr->frontier.pikeRecordStreaks[FRONTIER_LVL_50], 64, 114, 49);
    PikePrintCleared(gText_Total, gText_TimesCleared, gSaveBlock2Ptr->frontier.pikeTotalStreaks[FRONTIER_LVL_50], 64, 114, 65);
    PikePrintPrevOrCurrentStreak(FRONTIER_LVL_OPEN, 64, 114, 97);
    PikePrintCleared(gText_Record, gText_RoomsCleared, gSaveBlock2Ptr->frontier.pikeRecordStreaks[FRONTIER_LVL_OPEN], 64, 114, 113);
    PikePrintCleared(gText_Total, gText_TimesCleared, gSaveBlock2Ptr->frontier.pikeTotalStreaks[FRONTIER_LVL_OPEN], 64, 114, 129);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Arena records.
static void ArenaPrintStreak(const u8 *str, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x1, y, TEXT_SKIP_DRAW, NULL);
    if (num > MAX_STREAK)
        num = MAX_STREAK;
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_KOsInARow);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void ArenaPrintRecordStreak(u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    u16 num = gSaveBlock2Ptr->frontier.arenaRecordStreaks[lvlMode];
    ArenaPrintStreak(gText_Record, num, x1, x2, y);
}

static u16 ArenaGetWinStreak(u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void ArenaPrintPrevOrCurrentStreak(u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = ArenaGetWinStreak(lvlMode);

    if (lvlMode != FRONTIER_LVL_50)
        isCurrent = IsWinStreakActive(STREAK_ARENA_OPEN);
    else
        isCurrent = IsWinStreakActive(STREAK_ARENA_50);

    if (isCurrent == TRUE)
        ArenaPrintStreak(gText_Current, winStreak, x1, x2, y);
    else
        ArenaPrintStreak(gText_Prev, winStreak, x1, x2, y);
}

static void ShowArenaResultsWindow(void)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    PrintHyphens(10);
    StringExpandPlaceholders(gStringVarBuffer, gText_SetKOTourneyResults);
    PrintAligned(gStringVarBuffer, 2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 16, 49, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 16, 97, TEXT_SKIP_DRAW, NULL);
    ArenaPrintPrevOrCurrentStreak(FRONTIER_LVL_50, 72, 126, 49);
    ArenaPrintRecordStreak(FRONTIER_LVL_50, 72, 126, 65);
    ArenaPrintPrevOrCurrentStreak(FRONTIER_LVL_OPEN, 72, 126, 97);
    ArenaPrintRecordStreak(FRONTIER_LVL_OPEN, 72, 126, 113);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Factory records.
static void FactoryPrintStreak(const u8 *str, u16 num1, u16 num2, u8 x1, u8 x2, u8 x3, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x1, y, TEXT_SKIP_DRAW, NULL);
    if (num1 > MAX_STREAK)
        num1 = MAX_STREAK;
    ConvertIntToDecimalStringN(gStringVar1, num1, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_WinStreak);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);

    ConvertIntToDecimalStringN(gStringVar1, num2, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_TimesVar1);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x3, y, TEXT_SKIP_DRAW, NULL);
}

static void FactoryPrintRecordStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 x3, u8 y)
{
    u16 num1 = gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[battleMode][lvlMode];
    u16 num2 = gSaveBlock2Ptr->frontier.factoryRecordRentsCount[battleMode][lvlMode];
    FactoryPrintStreak(gText_Record, num1, num2, x1, x2, x3, y);
}

static u16 FactoryGetWinStreak(u8 battleMode, u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static u16 FactoryGetRentsCount(u8 battleMode, u8 lvlMode)
{
    u16 rents = gSaveBlock2Ptr->frontier.factoryRentsCount[battleMode][lvlMode];
    if (rents > MAX_STREAK)
        return MAX_STREAK;
    else
        return rents;
}

static void FactoryPrintPrevOrCurrentStreak(u8 battleMode, u8 lvlMode, u8 x1, u8 x2, u8 x3, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = FactoryGetWinStreak(battleMode, lvlMode);
    u16 rents = FactoryGetRentsCount(battleMode, lvlMode);
    switch (battleMode)
    {
    default:
    case FRONTIER_MODE_SINGLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_FACTORY_SINGLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_FACTORY_SINGLES_50);
        break;
    case FRONTIER_MODE_DOUBLES:
        if (lvlMode != FRONTIER_LVL_50)
            isCurrent = IsWinStreakActive(STREAK_FACTORY_DOUBLES_OPEN);
        else
            isCurrent = IsWinStreakActive(STREAK_FACTORY_DOUBLES_50);
        break;
    }

    if (isCurrent == TRUE)
        FactoryPrintStreak(gText_Current, winStreak, rents, x1, x2, x3, y);
    else
        FactoryPrintStreak(gText_Prev, winStreak, rents, x1, x2, x3, y);
}

static void ShowFactoryResultsWindow(u8 battleMode)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    if (battleMode == FRONTIER_MODE_SINGLES)
        StringExpandPlaceholders(gStringVarBuffer, gText_BattleSwapSingleResults);
    else
        StringExpandPlaceholders(gStringVarBuffer, gText_BattleSwapDoubleResults);

    PrintAligned(gStringVarBuffer, 0);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 8, 33, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_RentalSwap, 152, 33, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 8, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    FactoryPrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_50, 8, 64, 158, 49);
    FactoryPrintRecordStreak(battleMode, FRONTIER_LVL_50, 8, 64, 158, 65);
    FactoryPrintPrevOrCurrentStreak(battleMode, FRONTIER_LVL_OPEN, 8, 64, 158, 113);
    FactoryPrintRecordStreak(battleMode, FRONTIER_LVL_OPEN, 8, 64, 158, 129);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Battle Pyramid records.
static void PyramidPrintStreak(const u8 *str, u16 num, u8 x1, u8 x2, u8 y)
{
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x1, y, TEXT_SKIP_DRAW, NULL);
    if (num > MAX_STREAK)
        num = MAX_STREAK;
    ConvertIntToDecimalStringN(gStringVar1, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_FloorsCleared);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x2, y, TEXT_SKIP_DRAW, NULL);
}

static void PyramidPrintRecordStreak(u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    u16 num = gSaveBlock2Ptr->frontier.pyramidRecordStreaks[lvlMode];
    PyramidPrintStreak(gText_Record, num, x1, x2, y);
}

static u16 PyramidGetWinStreak(u8 lvlMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode];
    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static void PyramidPrintPrevOrCurrentStreak(u8 lvlMode, u8 x1, u8 x2, u8 y)
{
    bool8 isCurrent;
    u16 winStreak = PyramidGetWinStreak(lvlMode);

    if (lvlMode != FRONTIER_LVL_50)
        isCurrent = IsWinStreakActive(STREAK_PYRAMID_OPEN);
    else
        isCurrent = IsWinStreakActive(STREAK_PYRAMID_50);

    if (isCurrent == TRUE)
        PyramidPrintStreak(gText_Current, winStreak, x1, x2, y);
    else
        PyramidPrintStreak(gText_Prev, winStreak, x1, x2, y);
}

static void ShowPyramidResultsWindow(void)
{
    gRecordsWindowId = AddWindow(&sFrontierResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    StringExpandPlaceholders(gStringVarBuffer, gText_BattleQuestResults);
    PrintAligned(gStringVarBuffer, 2);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Lv502, 8, 49, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_OpenLv, 8, 97, TEXT_SKIP_DRAW, NULL);
    PrintHyphens(10);
    PyramidPrintPrevOrCurrentStreak(FRONTIER_LVL_50, 64, 111, 49);
    PyramidPrintRecordStreak(FRONTIER_LVL_50, 64, 111, 65);
    PyramidPrintPrevOrCurrentStreak(FRONTIER_LVL_OPEN, 64, 111, 97);
    PyramidPrintRecordStreak(FRONTIER_LVL_OPEN, 64, 111, 113);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

// Link contest records. Why is it in this file?
static void ShowLinkContestResultsWindow(void)
{
    const u8 *str;
    s32 i, j;
    s32 x;

    gRecordsWindowId = AddWindow(&sLinkContestResultsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));

    StringExpandPlaceholders(gStringVarBuffer, gText_LinkContestResults);
    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 208);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x, 1, TEXT_SKIP_DRAW, NULL);

    str = gText_1st;
    x = GetStringRightAlignXOffset(FONT_NORMAL, str, 38) + 50;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x, 25, TEXT_SKIP_DRAW, NULL);

    str = gText_2nd;
    x = GetStringRightAlignXOffset(FONT_NORMAL, str, 38) + 88;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x, 25, TEXT_SKIP_DRAW, NULL);

    str = gText_3rd;
    x = GetStringRightAlignXOffset(FONT_NORMAL, str, 38) + 126;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x, 25, TEXT_SKIP_DRAW, NULL);

    str = gText_4th;
    x = GetStringRightAlignXOffset(FONT_NORMAL, str, 38) + 164;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, str, x, 25, TEXT_SKIP_DRAW, NULL);

    x = 6;
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Cool, x, 41, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Beauty, x, 57, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Cute, x, 73, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Smart, x, 89, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_Tough, x, 105, TEXT_SKIP_DRAW, NULL);

    for (i = 0; i < CONTEST_CATEGORIES_COUNT; i++)
    {
        for (j = 0; j < CONTESTANT_COUNT; j++)
        {
            ConvertIntToDecimalStringN(gStringVarBuffer, gSaveBlock2Ptr->contestLinkResults[i][j], STR_CONV_MODE_RIGHT_ALIGN, 4);
            AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, (j * 38) + 64, (i * 16) + 41, TEXT_SKIP_DRAW, NULL);
        }
    }

    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

static void CheckPutFrontierTVShowOnAir(void)
{
    u8 name[32];
    s32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (facility)
    {
    case FRONTIER_FACILITY_TOWER:
        if (gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.towerRecordWinStreaks[battleMode][lvlMode])
        {
            gSaveBlock2Ptr->frontier.towerRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];
            if (battleMode == FRONTIER_MODE_LINK_MULTIS)
            {
                StringCopy(name, gLinkPlayers[gBattleScripting.multiplayerId ^ 1].name);
                StripExtCtrlCodes(name);
                StringCopy(gSaveBlock2Ptr->frontier.opponentNames[lvlMode], name);
                SetTrainerId(gLinkPlayers[gBattleScripting.multiplayerId ^ 1].trainerId, gSaveBlock2Ptr->frontier.opponentTrainerIds[lvlMode]);
            }
            if (gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                switch (battleMode)
                {
                case FRONTIER_MODE_SINGLES:
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_TOWER_SINGLES);
                    break;
                case FRONTIER_MODE_DOUBLES:
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_TOWER_DOUBLES);
                    break;
                case FRONTIER_MODE_MULTIS:
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_TOWER_MULTIS);
                    break;
                case FRONTIER_MODE_LINK_MULTIS:
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_TOWER_LINK_MULTIS);
                    break;
                }
            }
        }
        break;
    case FRONTIER_FACILITY_DOME:
        if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode])
        {
            gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
            if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                if (battleMode == FRONTIER_MODE_SINGLES)
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_DOME_SINGLES);
                else
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_DOME_DOUBLES);
            }
        }
        break;
    case FRONTIER_FACILITY_PALACE:
        if (gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[battleMode][lvlMode])
        {
            gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode];
            if (gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                if (battleMode == FRONTIER_MODE_SINGLES)
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_PALACE_SINGLES);
                else
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_PALACE_DOUBLES);
            }
        }
        break;
    case FRONTIER_FACILITY_ARENA:
        if (gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode] > gSaveBlock2Ptr->frontier.arenaRecordStreaks[lvlMode])
        {
            gSaveBlock2Ptr->frontier.arenaRecordStreaks[lvlMode] = gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode];
            if (gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode], FRONTIER_SHOW_ARENA);
            }
        }
        break;
    case FRONTIER_FACILITY_FACTORY:
        if (gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[battleMode][lvlMode])
        {
            gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode];
            gSaveBlock2Ptr->frontier.factoryRecordRentsCount[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.factoryRentsCount[battleMode][lvlMode];
            if (gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                if (battleMode == FRONTIER_MODE_SINGLES)
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_FACTORY_SINGLES);
                else
                    TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode], FRONTIER_SHOW_FACTORY_DOUBLES);
            }
        }
        break;
    case FRONTIER_FACILITY_PIKE:
        if (gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode] > gSaveBlock2Ptr->frontier.pikeRecordStreaks[lvlMode])
        {
            gSaveBlock2Ptr->frontier.pikeRecordStreaks[lvlMode] = gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode];
            if (gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode], FRONTIER_SHOW_PIKE);
            }
        }
        break;
    case FRONTIER_FACILITY_PYRAMID:
        if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] > gSaveBlock2Ptr->frontier.pyramidRecordStreaks[lvlMode])
        {
            gSaveBlock2Ptr->frontier.pyramidRecordStreaks[lvlMode] = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode];
            if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] > 1
                && ShouldAirFrontierTVShow())
            {
                TryPutFrontierTVShowOnAir(gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode], FRONTIER_SHOW_PYRAMID);
            }
        }
        break;
    }
}

static void Script_GetFrontierBrainStatus(void)
{
    VarGet(VAR_FRONTIER_FACILITY); // Unused return value.
    gSpecialVar_Result = GetFrontierBrainStatus();
}

u8 GetFrontierBrainStatus(void)
{
    s32 status = FRONTIER_BRAIN_NOT_READY;
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u16 winStreakNoModifier = GetCurrentFacilityWinStreak();
    s32 winStreak = winStreakNoModifier + sFrontierBrainStreakAppearances[facility][3];
    s32 symbolsCount;

    if (battleMode != FRONTIER_MODE_SINGLES)
        return FRONTIER_BRAIN_NOT_READY;

    symbolsCount = GetPlayerSymbolCountForFacility(facility);
    switch (symbolsCount)
    {
    // Missing a symbol
    case 0:
    case 1:
        if (winStreak == sFrontierBrainStreakAppearances[facility][symbolsCount])
            status = symbolsCount + 1; // FRONTIER_BRAIN_SILVER and FRONTIER_BRAIN_GOLD
        break;
    // Already received both symbols
    case 2:
    default:
        // Silver streak is reached
        if (winStreak == sFrontierBrainStreakAppearances[facility][0])
            status = FRONTIER_BRAIN_STREAK;
        // Gold streak is reached
        else if (winStreak == sFrontierBrainStreakAppearances[facility][1])
            status = FRONTIER_BRAIN_STREAK_LONG;
        // Some increment of the gold streak is reached
        else if (winStreak > sFrontierBrainStreakAppearances[facility][1] && (winStreak - sFrontierBrainStreakAppearances[facility][1]) % sFrontierBrainStreakAppearances[facility][2] == 0)
            status = FRONTIER_BRAIN_STREAK_LONG;
        break;
    }

    return status;
}

void CopyFrontierTrainerText(u8 whichText, u16 trainerId)
{
    switch (whichText)
    {
    case FRONTIER_BEFORE_TEXT:
        if (trainerId == TRAINER_EREADER)
            FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.greeting);
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
            CopyFrontierBrainText(FALSE);
        else if (trainerId < FRONTIER_TRAINERS_COUNT)
            FrontierSpeechToString(gFacilityTrainers[trainerId].speechBefore);
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
            FrontierSpeechToString(gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].greeting);
        else
            BufferApprenticeChallengeText(trainerId - TRAINER_RECORD_MIXING_APPRENTICE);
        break;
    case FRONTIER_PLAYER_LOST_TEXT:
        if (trainerId == TRAINER_EREADER)
        {
            FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.farewellPlayerLost);
        }
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
        {
            CopyFrontierBrainText(FALSE);
        }
        else if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            FrontierSpeechToString(gFacilityTrainers[trainerId].speechWin);
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                FrontierSpeechToString(GetRecordedBattleEasyChatSpeech());
            else
                FrontierSpeechToString(gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].speechWon);
        }
        else
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                FrontierSpeechToString(GetRecordedBattleEasyChatSpeech());
            else
                FrontierSpeechToString(gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].speechWon);
        }
        break;
    case FRONTIER_PLAYER_WON_TEXT:
        if (trainerId == TRAINER_EREADER)
        {
            FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.farewellPlayerWon);
        }
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
        {
            CopyFrontierBrainText(TRUE);
        }
        else if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            FrontierSpeechToString(gFacilityTrainers[trainerId].speechLose);
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                FrontierSpeechToString(GetRecordedBattleEasyChatSpeech());
            else
                FrontierSpeechToString(gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].speechLost);
        }
        else
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            {
                trainerId = GetRecordedBattleApprenticeId();
                FrontierSpeechToString(gApprentices[trainerId].speechLost);
            }
            else
            {
                trainerId = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id;
                FrontierSpeechToString(gApprentices[trainerId].speechLost);
            }
        }
        break;
    }
}

void ResetWinStreaks(void)
{
    s32 battleMode, lvlMode;

    gSaveBlock2Ptr->frontier.winStreakActiveFlags = 0;
    for (battleMode = 0; battleMode < FRONTIER_MODE_COUNT; battleMode++)
    {
        for (lvlMode = 0; lvlMode < FRONTIER_LVL_TENT; lvlMode++)
        {
            gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = 0;
            if (battleMode < FRONTIER_MODE_MULTIS)
            {
                gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] = 0;
                gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode] = 0;
                gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] = 0;
            }
            if (battleMode == FRONTIER_MODE_SINGLES)
            {
                gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode] = 0;
                gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode] = 0;
                gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] = 0;
            }
        }
    }
    if (gSaveBlock2Ptr->frontier.challengeStatus != 0)
        gSaveBlock2Ptr->frontier.challengeStatus = CHALLENGE_STATUS_SAVING;
}

u32 GetCurrentFacilityWinStreak(void)
{
    s32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);

    switch (facility)
    {
    case FRONTIER_FACILITY_TOWER:
        return gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];
    case FRONTIER_FACILITY_DOME:
        return gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
    case FRONTIER_FACILITY_PALACE:
        return gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode];
    case FRONTIER_FACILITY_ARENA:
        return gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode];
    case FRONTIER_FACILITY_FACTORY:
        return gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode];
    case FRONTIER_FACILITY_PIKE:
        return gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode];
    case FRONTIER_FACILITY_PYRAMID:
        return gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode];
    default:
        return 0;
    }
}

void ResetFrontierTrainerIds(void)
{
    s32 i;

    for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.trainerIds); i++)
        gSaveBlock2Ptr->frontier.trainerIds[i] = 0xFFFF;
}

static void IsTrainerFrontierBrain(void)
{
    if (gTrainerBattleOpponent_A == TRAINER_FRONTIER_BRAIN)
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

u8 GetPlayerSymbolCountForFacility(u8 facility)
{
    return FlagGet(FLAG_SYS_TOWER_SILVER + facility * 2)
         + FlagGet(FLAG_SYS_TOWER_GOLD + facility * 2);
}

static void GiveBattlePoints(void)
{
    s32 challengeNum = 0;
    s32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 points;

    switch (facility)
    {
    case FRONTIER_FACILITY_TOWER:
        challengeNum = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
        break;
    case FRONTIER_FACILITY_DOME:
        challengeNum = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
        break;
    case FRONTIER_FACILITY_PALACE:
        challengeNum = gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
        break;
    case FRONTIER_FACILITY_ARENA:
        challengeNum = gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
        break;
    case FRONTIER_FACILITY_FACTORY:
        challengeNum = gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
        break;
    case FRONTIER_FACILITY_PIKE:
        challengeNum = gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode] / NUM_PIKE_ROOMS;
        break;
    case FRONTIER_FACILITY_PYRAMID:
        challengeNum = gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
        break;
    }

    if (challengeNum != 0)
        challengeNum--;
    if (challengeNum >= ARRAY_COUNT(sBattlePointAwards))
        challengeNum = ARRAY_COUNT(sBattlePointAwards) - 1;

    points = sBattlePointAwards[challengeNum][facility][battleMode];
    if (gTrainerBattleOpponent_A == TRAINER_FRONTIER_BRAIN)
        points += 10;
    gSaveBlock2Ptr->frontier.battlePoints += points;
    ConvertIntToDecimalStringN(gStringVar1, points, STR_CONV_MODE_LEFT_ALIGN, 2);
    if (gSaveBlock2Ptr->frontier.battlePoints > MAX_BATTLE_FRONTIER_POINTS)
        gSaveBlock2Ptr->frontier.battlePoints = MAX_BATTLE_FRONTIER_POINTS;

    points = gSaveBlock2Ptr->frontier.cardBattlePoints;
    points += sBattlePointAwards[challengeNum][facility][battleMode];
    IncrementDailyBattlePoints(sBattlePointAwards[challengeNum][facility][battleMode]);
    if (gTrainerBattleOpponent_A == TRAINER_FRONTIER_BRAIN)
    {
        points += 10;
        IncrementDailyBattlePoints(10);
    }
    if (points > 0xFFFF)
        points = 0xFFFF;
    gSaveBlock2Ptr->frontier.cardBattlePoints = points;
}

static void GetFacilitySymbolCount(void)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    gSpecialVar_Result = GetPlayerSymbolCountForFacility(facility);
}

static void GiveFacilitySymbol(void)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    if (GetPlayerSymbolCountForFacility(facility) == 0)
        FlagSet(FLAG_SYS_TOWER_SILVER + facility * 2);
    else
        FlagSet(FLAG_SYS_TOWER_GOLD + facility * 2);
}

static void CheckBattleTypeFlag(void)
{
    if (gBattleTypeFlags & gSpecialVar_0x8005)
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

static u8 AppendCaughtBannedMonSpeciesName(u16 species, u8 count, s32 numBannedMonsCaught)
{
    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
    {
        count++;
        switch (count)
        {
        case 1:
        case 3:
        case 5:
        case 7:
        case 9:
        case 11:
            if (numBannedMonsCaught == count)
                StringAppend(gStringVar1, gText_SpaceAndSpace);
            else if (numBannedMonsCaught > count)
                StringAppend(gStringVar1, gText_CommaSpace);
            break;
        case 2:
            if (count == numBannedMonsCaught)
                StringAppend(gStringVar1, gText_SpaceAndSpace);
            else
                StringAppend(gStringVar1, gText_CommaSpace);
            StringAppend(gStringVar1, gText_NewLine);
            break;
        default:
            if (count == numBannedMonsCaught)
                StringAppend(gStringVar1, gText_SpaceAndSpace);
            else
                StringAppend(gStringVar1, gText_CommaSpace);
            StringAppend(gStringVar1, gText_LineBreak);
            break;
        }
        StringAppend(gStringVar1, gSpeciesNames[species]);
    }

    return count;
}

static void AppendIfValid(u16 species, u16 heldItem, u16 hp, u8 lvlMode, u8 monLevel, u16 *speciesArray, u16 *itemsArray, u8 *count)
{
    s32 i = 0;

    if (species == SPECIES_EGG || species == SPECIES_NONE)
        return;

    for (i = 0; gFrontierBannedSpecies[i] != 0xFFFF && gFrontierBannedSpecies[i] != species; i++)
        ;

    if (gFrontierBannedSpecies[i] != 0xFFFF)
        return;
    if (lvlMode == FRONTIER_LVL_50 && monLevel > FRONTIER_MAX_LEVEL_50)
        return;

    for (i = 0; i < *count && speciesArray[i] != species; i++)
        ;
    if (i != *count)
        return;

    if (heldItem != 0)
    {
        for (i = 0; i < *count && itemsArray[i] != heldItem; i++)
            ;
        if (i != *count)
            return;
    }

    speciesArray[*count] = species;
    itemsArray[*count] = heldItem;
    (*count)++;
}

// gSpecialVar_Result is the level mode before and after calls to this function
// gSpecialVar_0x8004 is used to store the return value instead (TRUE if there are insufficient eligible mons)
// The names of ineligible Pokémon that have been caught are also buffered to print
static void CheckPartyIneligibility(void)
{
    u16 speciesArray[PARTY_SIZE];
    u16 itemArray[PARTY_SIZE];
    s32 monId = 0;
    s32 toChoose = 0;
    u8 count = 0;
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 monIdLooper;

    // count is re-used, define for clarity
    #define numEligibleMons count

    switch (battleMode)
    {
    case FRONTIER_MODE_SINGLES:
        toChoose = FRONTIER_PARTY_SIZE;
        break;
    case FRONTIER_MODE_MULTIS:
    case FRONTIER_MODE_LINK_MULTIS:
        toChoose = FRONTIER_MULTI_PARTY_SIZE;
        break;
    case FRONTIER_MODE_DOUBLES:
        if (VarGet(VAR_FRONTIER_FACILITY) == FRONTIER_FACILITY_TOWER)
            toChoose = FRONTIER_DOUBLES_PARTY_SIZE;
        else
            toChoose = FRONTIER_PARTY_SIZE;
        break;
    }

    monIdLooper = 0;
    do
    {
        monId = monIdLooper;
        numEligibleMons = 0;
        do
        {
            u16 species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES_OR_EGG);
            u16 heldItem = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
            u8 level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
            u16 hp = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
            if (VarGet(VAR_FRONTIER_FACILITY) == FRONTIER_FACILITY_PYRAMID)
            {
                if (heldItem == ITEM_NONE)
                    AppendIfValid(species, heldItem, hp, gSpecialVar_Result, level, speciesArray, itemArray, &numEligibleMons);
            }
            else
            {
                AppendIfValid(species, heldItem, hp, gSpecialVar_Result, level, speciesArray, itemArray, &numEligibleMons);
            }
            monId++;
            if (monId >= PARTY_SIZE)
                monId = 0;
        } while (monId != monIdLooper);

        monIdLooper++;
    } while (monIdLooper < PARTY_SIZE && numEligibleMons < toChoose);

    if (numEligibleMons < toChoose)
    {
        s32 i;
        s32 caughtBannedMons = 0;
        s32 species = gFrontierBannedSpecies[0];
        for (i = 0; species != 0xFFFF; i++, species = gFrontierBannedSpecies[i])
        {
            if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                caughtBannedMons++;
        }
        gStringVar1[0] = EOS;
        gSpecialVar_0x8004 = TRUE;
        count = 0;
        for (i = 0; gFrontierBannedSpecies[i] != 0xFFFF; i++)
            count = AppendCaughtBannedMonSpeciesName(gFrontierBannedSpecies[i], count, caughtBannedMons);

        if (count == 0)
        {
            StringAppend(gStringVar1, gText_Space2);
            StringAppend(gStringVar1, gText_Are);
        }
        else
        {
            if (count & 1)
                StringAppend(gStringVar1, gText_LineBreak);
            else
                StringAppend(gStringVar1, gText_Space2);
            StringAppend(gStringVar1, gText_Are2);
        }
    }
    else
    {
        gSpecialVar_0x8004 = FALSE;
        gSaveBlock2Ptr->frontier.lvlMode = gSpecialVar_Result;
    }
    #undef numEligibleMons
}

static void ValidateVisitingTrainer(void)
{
    ValidateEReaderTrainer();
}

static void IncrementWinStreak(void)
{
    s32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    s32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);

    switch (facility)
    {
    case FRONTIER_FACILITY_TOWER:
        if (gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] < MAX_STREAK)
        {
            gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode]++;
            if (battleMode == FRONTIER_MODE_SINGLES)
            {
                SetGameStat(GAME_STAT_BATTLE_TOWER_SINGLES_STREAK, gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode]);
                gSaveBlock2Ptr->frontier.towerSinglesStreak = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];
            }
        }
        break;
    case FRONTIER_FACILITY_DOME:
        if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode]++;
        if (gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode]++;
        break;
    case FRONTIER_FACILITY_PALACE:
        if (gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.palaceWinStreaks[battleMode][lvlMode]++;
        break;
    case FRONTIER_FACILITY_ARENA:
        if (gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.arenaWinStreaks[lvlMode]++;
        break;
    case FRONTIER_FACILITY_FACTORY:
        if (gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode]++;
        break;
    case FRONTIER_FACILITY_PIKE:
        if (gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.pikeWinStreaks[lvlMode]++;
        break;
    case FRONTIER_FACILITY_PYRAMID:
        if (gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode] < MAX_STREAK)
            gSaveBlock2Ptr->frontier.pyramidWinStreaks[lvlMode]++;
        break;
    }
}

static void RestoreHeldItems(void)
{
    u8 i;

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        if (gSaveBlock2Ptr->frontier.selectedPartyMons[i] != 0)
        {
            u16 item = GetMonData(&gSaveBlock1Ptr->playerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], MON_DATA_HELD_ITEM, NULL);
            SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &item);
        }
    }
}

static void SaveRecordBattle(void)
{
    gSpecialVar_Result = MoveRecordedBattleToSaveData();
    gSaveBlock2Ptr->frontier.disableRecordBattle = TRUE;
}

static void BufferFrontierTrainerName(void)
{
    switch (gSpecialVar_0x8005)
    {
    case 0:
        GetFrontierTrainerName(gStringVar1, gTrainerBattleOpponent_A);
        break;
    case 1:
        GetFrontierTrainerName(gStringVar2, gTrainerBattleOpponent_A);
        break;
    }
}

static void ResetSketchedMoves(void)
{
    u8 i, j, k;

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        u16 monId = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
        if (monId < PARTY_SIZE)
        {
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                for (k = 0; k < MAX_MON_MOVES; k++)
                {
                    if (GetMonData(&gSaveBlock1Ptr->playerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], MON_DATA_MOVE1 + k, NULL)
                        == GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + j, NULL))
                        break;
                }
                if (k == MAX_MON_MOVES)
                    SetMonMoveSlot(&gPlayerParty[i], MOVE_SKETCH, j);
            }
            gSaveBlock1Ptr->playerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1] = gPlayerParty[i];
        }
    }
}

static void SetFacilityBrainObjectEvent(void)
{
    SetFrontierBrainObjEventGfx(VarGet(VAR_FRONTIER_FACILITY));
}

// Battle Frontier Ranking Hall records.
static void Print1PRecord(s32 position, s32 x, s32 y, struct RankingHall1P *hallRecord, s32 hallFacilityId)
{
    u8 text[32];
    u16 winStreak;

    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_123Dot[position], x * 8, (8 * (y + 5 * position)) + 1, TEXT_SKIP_DRAW, NULL);
    hallRecord->name[PLAYER_NAME_LENGTH] = EOS;
    if (hallRecord->winStreak)
    {
        TVShowConvertInternationalString(text, hallRecord->name, hallRecord->language);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, text, (x + 2) * 8, (8 * (y + 5 * position)) + 1, TEXT_SKIP_DRAW, NULL);
        winStreak = hallRecord->winStreak;
        if (winStreak > MAX_STREAK)
            winStreak = MAX_STREAK;
        ConvertIntToDecimalStringN(gStringVar2, winStreak, STR_CONV_MODE_RIGHT_ALIGN, 4);
        StringExpandPlaceholders(gStringVarBuffer, sHallFacilityToRecordsText[hallFacilityId]);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NORMAL, sHallFacilityToRecordsText[hallFacilityId], 0xC8), (8 * (y + 5 * position)) + 1, TEXT_SKIP_DRAW, NULL);
    }
}

static void Print2PRecord(s32 position, s32 x, s32 y, struct RankingHall2P *hallRecord)
{
    u8 text[32];
    u16 winStreak;

    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_123Dot[position], x * 8, (8 * (y + 5 * position)) + 1, TEXT_SKIP_DRAW, NULL);
    if (hallRecord->winStreak)
    {
        hallRecord->name1[PLAYER_NAME_LENGTH] = EOS;
        hallRecord->name2[PLAYER_NAME_LENGTH] = EOS;
        TVShowConvertInternationalString(text, hallRecord->name1, hallRecord->language);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, text, (x + 2) * 8, (8 * (y + 5 * position - 1)) + 1, TEXT_SKIP_DRAW, NULL);
        if (IsStringJapanese(hallRecord->name2))
            TVShowConvertInternationalString(text, hallRecord->name2, LANGUAGE_JAPANESE);
        else
            StringCopy(text, hallRecord->name2);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, text, (x + 4) * 8, (8 * (y + 5 * position + 1)) + 1, TEXT_SKIP_DRAW, NULL);

        winStreak = hallRecord->winStreak;
        if (winStreak > MAX_STREAK)
            winStreak = MAX_STREAK;
        ConvertIntToDecimalStringN(gStringVar2, winStreak, STR_CONV_MODE_RIGHT_ALIGN, 4);
        StringExpandPlaceholders(gStringVarBuffer, sHallFacilityToRecordsText[RANKING_HALL_TOWER_LINK]);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NORMAL, sHallFacilityToRecordsText[RANKING_HALL_TOWER_LINK], 0xC8), (8 * (y + 5 * position)) + 1, TEXT_SKIP_DRAW, NULL);
    }
}

static void Fill1PRecords(struct RankingHall1P *dst, s32 hallFacilityId, s32 lvlMode)
{
    s32 i, j;
    struct RankingHall1P record1P[HALL_RECORDS_COUNT + 1];
    struct PlayerHallRecords *playerHallRecords = AllocZeroed(sizeof(struct PlayerHallRecords));
    GetPlayerHallRecords(playerHallRecords);

    for (i = 0; i < HALL_RECORDS_COUNT; i++)
        record1P[i] = gSaveBlock2Ptr->hallRecords1P[hallFacilityId][lvlMode][i];

    record1P[HALL_RECORDS_COUNT] = playerHallRecords->onePlayer[hallFacilityId][lvlMode];

    for (i = 0; i < HALL_RECORDS_COUNT; i++)
    {
        s32 highestWinStreak = 0;
        s32 highestId = 0;
        for (j = 0; j < HALL_RECORDS_COUNT + 1; j++)
        {
            if (record1P[j].winStreak > highestWinStreak)
            {
                highestId = j;
                highestWinStreak = record1P[j].winStreak;
            }
        }
        if (record1P[HALL_RECORDS_COUNT].winStreak >= highestWinStreak)
            highestId = HALL_RECORDS_COUNT;

        dst[i] = record1P[highestId];
        record1P[highestId].winStreak = 0;
    }

    Free(playerHallRecords);
}

static void Fill2PRecords(struct RankingHall2P *dst, s32 lvlMode)
{
    s32 i, j;
    struct RankingHall2P record2P[HALL_RECORDS_COUNT + 1];
    struct PlayerHallRecords *playerHallRecords = AllocZeroed(sizeof(struct PlayerHallRecords));
    GetPlayerHallRecords(playerHallRecords);

    for (i = 0; i < HALL_RECORDS_COUNT; i++)
        record2P[i] = gSaveBlock2Ptr->hallRecords2P[lvlMode][i];

    record2P[HALL_RECORDS_COUNT] = playerHallRecords->twoPlayers[lvlMode];

    for (i = 0; i < HALL_RECORDS_COUNT; i++)
    {
        s32 highestWinStreak = 0;
        s32 highestId = 0;
        for (j = 0; j < HALL_RECORDS_COUNT; j++)
        {
            if (record2P[j].winStreak > highestWinStreak)
            {
                highestId = j;
                highestWinStreak = record2P[j].winStreak;
            }
        }
        if (record2P[HALL_RECORDS_COUNT].winStreak >= highestWinStreak)
            highestId = HALL_RECORDS_COUNT;

        dst[i] = record2P[highestId];
        record2P[highestId].winStreak = 0;
    }

    Free(playerHallRecords);
}

static void PrintHallRecords(s32 hallFacilityId, s32 lvlMode)
{
    s32 i;
    s32 x;
    struct RankingHall1P records1P[HALL_RECORDS_COUNT];
    struct RankingHall2P records2P[HALL_RECORDS_COUNT];

    StringCopy(gStringVar1, sRecordsWindowChallengeTexts[hallFacilityId][0]);
    StringExpandPlaceholders(gStringVarBuffer, sRecordsWindowChallengeTexts[hallFacilityId][1]);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, 0, 1, TEXT_SKIP_DRAW, NULL);
    x = GetStringRightAlignXOffset(FONT_NORMAL, sLevelModeText[lvlMode], DISPLAY_WIDTH - 32);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, sLevelModeText[lvlMode], x, 1, TEXT_SKIP_DRAW, NULL);
    if (hallFacilityId == RANKING_HALL_TOWER_LINK)
    {
        gSaveBlock2Ptr->frontier.opponentNames[0][PLAYER_NAME_LENGTH] = EOS;
        gSaveBlock2Ptr->frontier.opponentNames[1][PLAYER_NAME_LENGTH] = EOS;
        Fill2PRecords(records2P, lvlMode);
        for (i = 0; i < HALL_RECORDS_COUNT; i++)
            Print2PRecord(i, 1, 4, &records2P[i]);
    }
    else
    {
        Fill1PRecords(records1P, hallFacilityId, lvlMode);
        for (i = 0; i < HALL_RECORDS_COUNT; i++)
            Print1PRecord(i, 1, 4, &records1P[i], hallFacilityId);
    }
}

void ShowRankingHallRecordsWindow(void)
{
    gRecordsWindowId = AddWindow(&sRankingHallRecordsWindowTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    PrintHallRecords(gSpecialVar_0x8005, FRONTIER_LVL_50);
    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

void ScrollRankingHallRecordsWindow(void)
{
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    PrintHallRecords(gSpecialVar_0x8005, FRONTIER_LVL_OPEN);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_GFX);
}

void ClearRankingHallRecords(void)
{
    s32 i, j, k;

    // UB: Passing 0 as a pointer instead of a pointer holding a value of 0.
#ifdef UBFIX
    u8 emptyId[TRAINER_ID_LENGTH] = {0};
    #define ZERO emptyId
#else
    #define ZERO 0
#endif

    for (i = 0; i < HALL_FACILITIES_COUNT; i++)
    {
        for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
        {
            for (k = 0; k < HALL_RECORDS_COUNT; k++)
            {
                CopyTrainerId(gSaveBlock2Ptr->hallRecords1P[i][j][k].id, ZERO);
                gSaveBlock2Ptr->hallRecords1P[i][j][k].name[0] = EOS;
                gSaveBlock2Ptr->hallRecords1P[i][j][k].winStreak = 0;
            }
        }
    }

    for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
    {
        for (k = 0; k < HALL_RECORDS_COUNT; k++)
        {
            CopyTrainerId(gSaveBlock2Ptr->hallRecords2P[j][k].id1, ZERO);
            CopyTrainerId(gSaveBlock2Ptr->hallRecords2P[j][k].id2, ZERO);
            gSaveBlock2Ptr->hallRecords2P[j][k].name1[0] = EOS;
            gSaveBlock2Ptr->hallRecords2P[j][k].name2[0] = EOS;
            gSaveBlock2Ptr->hallRecords2P[j][k].winStreak = 0;
        }
    }
}

void SaveGameFrontier(void)
{
    s32 i;
    struct Pokemon *monsParty = AllocZeroed(sizeof(struct Pokemon) * PARTY_SIZE);

    for (i = 0; i < PARTY_SIZE; i++)
        monsParty[i] = gPlayerParty[i];

    i = gPlayerPartyCount;
    LoadPlayerParty();
    SetContinueGameWarpStatusToDynamicWarp();
    TrySavingData(SAVE_LINK);
    ClearContinueGameWarpStatus2();
    gPlayerPartyCount = i;

    for (i = 0; i < PARTY_SIZE; i++)
        gPlayerParty[i] = monsParty[i];

    Free(monsParty);
}

// Frontier Brain functions.
u8 GetFrontierBrainTrainerPicIndex(void)
{
    s32 facility;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        facility = GetRecordedBattleFrontierFacility();
    else
        facility = VarGet(VAR_FRONTIER_FACILITY);

    return gTrainers[sFrontierBrainTrainerIds[facility]].trainerPic;
}

u8 GetFrontierBrainTrainerClass(void)
{
    s32 facility;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        facility = GetRecordedBattleFrontierFacility();
    else
        facility = VarGet(VAR_FRONTIER_FACILITY);

    return gTrainers[sFrontierBrainTrainerIds[facility]].trainerClass;
}

void CopyFrontierBrainTrainerName(u8 *dst)
{
    s32 i;
    s32 facility;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        facility = GetRecordedBattleFrontierFacility();
    else
        facility = VarGet(VAR_FRONTIER_FACILITY);

    for (i = 0; i < PLAYER_NAME_LENGTH; i++)
        dst[i] = gTrainers[sFrontierBrainTrainerIds[facility]].trainerName[i];

    dst[i] = EOS;
}

bool8 IsFrontierBrainFemale(void)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    return sFrontierBrainObjEventGfx[facility][1];
}

void SetFrontierBrainObjEventGfx_2(void)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    VarSet(VAR_OBJ_GFX_ID_0, sFrontierBrainObjEventGfx[facility][0]);
}

#define FRONTIER_BRAIN_OTID 61226

void CreateFrontierBrainPokemon(void)
{
    s32 i, j;
    s32 selectedMonBits;
    s32 monPartyId;
    s32 monLevel = 0;
    u8 friendship;
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetFronterBrainSymbol();

    if (facility == FRONTIER_FACILITY_DOME)
        selectedMonBits = GetDomeTrainerSelectedMons(TrainerIdToDomeTournamentId(TRAINER_FRONTIER_BRAIN));
    else
        selectedMonBits = (1 << FRONTIER_PARTY_SIZE) - 1; // all 3 mons selected

    ZeroEnemyPartyMons();
    monPartyId = 0;
    monLevel = SetFacilityPtrsGetLevel();
    for (i = 0; i < FRONTIER_PARTY_SIZE; selectedMonBits >>= 1, i++)
    {
        if (!(selectedMonBits & 1))
            continue;

        do
        {
            do
            {
                j = Random32(); //should just be one while loop, but that doesn't match
            } while (IsShinyOtIdPersonality(FRONTIER_BRAIN_OTID, j));
        } while (sFrontierBrainsMons[facility][symbol][i].nature != GetNatureFromPersonality(j));
        CreateMon(&gEnemyParty[monPartyId],
                  sFrontierBrainsMons[facility][symbol][i].species,
                  monLevel,
                  sFrontierBrainsMons[facility][symbol][i].fixedIV,
                  TRUE, j,
                  OT_ID_PRESET, FRONTIER_BRAIN_OTID);
        SetMonData(&gEnemyParty[monPartyId], MON_DATA_HELD_ITEM, &sFrontierBrainsMons[facility][symbol][i].heldItem);
        for (j = 0; j < NUM_STATS; j++)
            SetMonData(&gEnemyParty[monPartyId], MON_DATA_HP_EV + j, &sFrontierBrainsMons[facility][symbol][i].evs[j]);
        friendship = MAX_FRIENDSHIP;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            SetMonMoveSlot(&gEnemyParty[monPartyId], sFrontierBrainsMons[facility][symbol][i].moves[j], j);
            if (sFrontierBrainsMons[facility][symbol][i].moves[j] == MOVE_FRUSTRATION)
                friendship = 0;
        }
        SetMonData(&gEnemyParty[monPartyId], MON_DATA_FRIENDSHIP, &friendship);
        CalculateMonStats(&gEnemyParty[monPartyId]);
        monPartyId++;
    }
}

u16 GetFrontierBrainMonSpecies(u8 monId)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetFronterBrainSymbol();

    return sFrontierBrainsMons[facility][symbol][monId].species;
}

void SetFrontierBrainObjEventGfx(u8 facility)
{
    gTrainerBattleOpponent_A = TRAINER_FRONTIER_BRAIN;
    VarSet(VAR_OBJ_GFX_ID_0, sFrontierBrainObjEventGfx[facility][0]);
}

u16 GetFrontierBrainMonMove(u8 monId, u8 moveSlotId)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetFronterBrainSymbol();

    return sFrontierBrainsMons[facility][symbol][monId].moves[moveSlotId];
}

u8 GetFrontierBrainMonNature(u8 monId)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetFronterBrainSymbol();

    return sFrontierBrainsMons[facility][symbol][monId].nature;
}

u8 GetFrontierBrainMonEvs(u8 monId, u8 evStatId)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetFronterBrainSymbol();

    return sFrontierBrainsMons[facility][symbol][monId].evs[evStatId];
}

s32 GetFronterBrainSymbol(void)
{
    s32 facility = VarGet(VAR_FRONTIER_FACILITY);
    s32 symbol = GetPlayerSymbolCountForFacility(facility);

    if (symbol == 2)
    {
        u16 winStreak = GetCurrentFacilityWinStreak();
        if (winStreak + sFrontierBrainStreakAppearances[facility][3] == sFrontierBrainStreakAppearances[facility][0])
            symbol = 0;
        else if (winStreak + sFrontierBrainStreakAppearances[facility][3] == sFrontierBrainStreakAppearances[facility][1])
            symbol = 1;
        else if (winStreak + sFrontierBrainStreakAppearances[facility][3] > sFrontierBrainStreakAppearances[facility][1]
                 && (winStreak + sFrontierBrainStreakAppearances[facility][3] - sFrontierBrainStreakAppearances[facility][1]) % sFrontierBrainStreakAppearances[facility][2] == 0)
            symbol = 1;
    }
    return symbol;
}

// Called for intro speech as well despite the fact that its handled in the map scripts files instead
static void CopyFrontierBrainText(bool8 playerWonText)
{
    s32 facility;
    s32 symbol;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
    {
        facility = GetRecordedBattleFrontierFacility();
        symbol = GetRecordedBattleFronterBrainSymbol();
    }
    else
    {
        facility = VarGet(VAR_FRONTIER_FACILITY);
        symbol = GetFronterBrainSymbol();
    }

    switch (playerWonText)
    {
    case FALSE:
        StringCopy(gStringVarBuffer, sFrontierBrainPlayerLostTexts[symbol][facility]);
        break;
    case TRUE:
        StringCopy(gStringVarBuffer, sFrontierBrainPlayerWonTexts[symbol][facility]);
        break;
    }
}
