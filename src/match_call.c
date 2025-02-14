#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_setup.h"
#include "bg.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "main.h"
#include "match_call.h"
#include "menu.h"
#include "new_game.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "region_map.h"
#include "rtc.h"
#include "script.h"
#include "script_movement.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "wild_encounter.h"
#include "window.h"
#include "constants/abilities.h"
#include "constants/battle_frontier.h"
#include "constants/event_objects.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/trainers.h"

// In this file only the values normally associated with Battle Pike and Factory are swapped.
// Note that this is *not* a bug, because they are properly swapped consistently in this file.
// There would only be an issue if anything in this file interacted with something expecting
// the usual value order, or vice versa.
#define MATCH_CALL_FACTORY  FRONTIER_FACILITY_PIKE
#define MATCH_CALL_PIKE     FRONTIER_FACILITY_FACTORY

// Each match call message has variables that can be populated randomly or
// dependent on the trainer. The below are IDs for how to populate the vars
// in a given message.
// Each message may have up to 3 vars in it
enum {
    STR_TRAINER_NAME,
    STR_MAP_NAME,
    STR_SPECIES_IN_ROUTE,
    STR_SPECIES_IN_PARTY,
    STR_FACILITY_NAME,
    STR_FRONTIER_STREAK,
    STR_NONE = -1,
};
#define STRS_NORMAL_MSG      {STR_TRAINER_NAME, STR_NONE,             STR_NONE}
#define STRS_WILD_BATTLE     {STR_TRAINER_NAME, STR_SPECIES_IN_ROUTE, STR_NONE}
#define STRS_BATTLE_NEGATIVE {STR_TRAINER_NAME, STR_NONE,             STR_NONE}
#define STRS_BATTLE_POSITIVE {STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE}
#define STRS_BATTLE_REQUEST  {STR_TRAINER_NAME, STR_MAP_NAME,         STR_NONE}
#define STRS_FRONTIER        {STR_TRAINER_NAME, STR_FACILITY_NAME,    STR_FRONTIER_STREAK}

#define NUM_STRVARS_IN_MSG 3

// Topic IDs for sMatchCallGeneralTopics
enum {
    GEN_TOPIC_PERSONAL = 1,
    GEN_TOPIC_STREAK,
    GEN_TOPIC_STREAK_RECORD,
    GEN_TOPIC_B_DOME,
    GEN_TOPIC_B_PIKE,
    GEN_TOPIC_B_PYRAMID,
};

// Topic IDs for sMatchCallBattleTopics
enum {
    B_TOPIC_WILD = 1,
    B_TOPIC_NEGATIVE,
    B_TOPIC_POSITIVE,
};

// Each trainer has a text id for 1 of each of the 3 battle topics
// The msgId is the index into the respective topic's message table
// For all but 2 trainers this index is the same for each topic
#define BATTLE_TEXT_IDS(msgId) {TEXT_ID(B_TOPIC_WILD, (msgId)), TEXT_ID(B_TOPIC_NEGATIVE, (msgId)), TEXT_ID(B_TOPIC_POSITIVE, (msgId))}

// Topic IDs for sMatchCallBattleRequestTopics
enum {
    REQ_TOPIC_SAME_ROUTE = 1,
    REQ_TOPIC_DIFF_ROUTE,
};

struct MatchCallState
{
    u32 minutes;
    u16 trainerId;
    u8 stepCounter;
    bool8 triggeredFromScript;
};

struct MatchCallTrainerTextInfo
{
    u16 trainerId;
    u16 unused;
    u16 battleTopicTextIds[3];
    u16 generalTextId;
    u8 battleFrontierRecordStreakTextIndex;
    u16 sameRouteMatchCallTextId;
    u16 differentRouteMatchCallTextId;
};

struct MatchCallText
{
    const u8 *text;
    s8 stringVarFuncIds[NUM_STRVARS_IN_MSG];
};

struct MultiTrainerMatchCallText
{
    u16 trainerId;
    const u8 *text;
};

struct BattleFrontierStreakInfo
{
    u16 facilityId;
    u16 streak;
};

static EWRAM_DATA struct MatchCallState sMatchCallState = {0};
static EWRAM_DATA struct BattleFrontierStreakInfo sBattleFrontierStreakInfo = {0};

static u32 GetCurrentTotalMinutes(struct Time *);
static u32 GetNumRegisteredTrainers(void);
static u32 GetActiveMatchCallTrainerId(u32);
static int GetTrainerMatchCallId(int);
static u16 GetRematchTrainerLocation(int);
static bool32 TrainerIsEligibleForRematch(int);
static void StartMatchCall(void);
static void ExecuteMatchCall(u8);
static void DrawMatchCallTextBoxBorder_Internal(u32, u32, u32);
static void Task_SpinPokenavIcon(u8);
static void InitMatchCallTextPrinter(int, const u8 *);
static bool32 RunMatchCallTextPrinter(int);
static const struct MatchCallText *GetSameRouteMatchCallText(int, u8 *);
static const struct MatchCallText *GetDifferentRouteMatchCallText(int, u8 *);
static const struct MatchCallText *GetBattleMatchCallText(int, u8 *);
static const struct MatchCallText *GetGeneralMatchCallText(int, u8 *);
static bool32 ShouldTrainerRequestBattle(int);
static void BuildMatchCallString(int, const struct MatchCallText *, u8 *);
static u16 GetFrontierStreakInfo(u16, u32 *);
static void PopulateMatchCallStringVars(int, const s8 *);
static void PopulateMatchCallStringVar(int, int, u8 *);
static bool32 MatchCall_LoadGfx(u8);
static bool32 MatchCall_DrawWindow(u8);
static bool32 MatchCall_ReadyIntro(u8);
static bool32 MatchCall_SlideWindowIn(u8);
static bool32 MatchCall_PrintIntro(u8);
static bool32 MatchCall_PrintMessage(u8);
static bool32 MatchCall_SlideWindowOut(u8);
static bool32 MatchCall_EndCall(u8);
static void PopulateTrainerName(int, u8 *);
static void PopulateMapName(int, u8 *);
static void PopulateSpeciesFromTrainerLocation(int, u8 *);
static void PopulateSpeciesFromTrainerParty(int, u8 *);
static void PopulateBattleFrontierFacilityName(int, u8 *);
static void PopulateBattleFrontierStreak(int, u8 *);

#define TEXT_ID(topic, id) (((topic) << 8) | ((id) & 0xFF))

static const struct MatchCallTrainerTextInfo sMatchCallTrainers[] =
{
    {
        .trainerId = TRAINER_ROSE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 3),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_ANDRES_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(12),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 62),
        .battleFrontierRecordStreakTextIndex = 12,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 12),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 12),
    },
    {
        .trainerId = TRAINER_DUSTY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(12),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 4),
        .battleFrontierRecordStreakTextIndex = 12,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 12),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 12),
    },
    {
        .trainerId = TRAINER_LOLA_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(2),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 5),
        .battleFrontierRecordStreakTextIndex = 2,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 2),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 2),
    },
    {
        .trainerId = TRAINER_RICKY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 6),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_LILA_AND_ROY_1,
        .unused = 4,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 61),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_CRISTIN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(10),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 64),
        .battleFrontierRecordStreakTextIndex = 10,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 10),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 10),
    },
    {
        .trainerId = TRAINER_BROOKE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 8),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_WILTON_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(6),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 7),
        .battleFrontierRecordStreakTextIndex = 6,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 6),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 6),
    },
    {
        .trainerId = TRAINER_VALERIE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 9),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_CINDY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 10),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_THALIA_1,
        .unused = 0,
        // Thalia and Sawyer are the only ones who use different msg ids for their battle topics
        .battleTopicTextIds = { TEXT_ID(B_TOPIC_WILD, 8), TEXT_ID(B_TOPIC_NEGATIVE, 10), TEXT_ID(B_TOPIC_POSITIVE, 10) },
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 14),
        .battleFrontierRecordStreakTextIndex = 10,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 10),
    },
    {
        .trainerId = TRAINER_JESSICA_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(10),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 11),
        .battleFrontierRecordStreakTextIndex = 10,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 10),
    },
    {
        .trainerId = TRAINER_WINSTON_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(4),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 12),
        .battleFrontierRecordStreakTextIndex = 4,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 4),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 4),
    },
    {
        .trainerId = TRAINER_STEVE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(7),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 13),
        .battleFrontierRecordStreakTextIndex = 7,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 7),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 7),
    },
    {
        .trainerId = TRAINER_TONY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 15),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_NOB_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 16),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_KOJI_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 59),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_FERNANDO_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(6),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 17),
        .battleFrontierRecordStreakTextIndex = 6,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 6),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 6),
    },
    {
        .trainerId = TRAINER_DALTON_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(4),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 18),
        .battleFrontierRecordStreakTextIndex = 4,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 4),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 4),
    },
    {
        .trainerId = TRAINER_BERNIE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(11),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 19),
        .battleFrontierRecordStreakTextIndex = 11,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 11),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 11),
    },
    {
        .trainerId = TRAINER_ETHAN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 20),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_JOHN_AND_JAY_1,
        .unused = 3,
        .battleTopicTextIds = BATTLE_TEXT_IDS(12),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 60),
        .battleFrontierRecordStreakTextIndex = 12,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 12),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 12),
    },
    {
        .trainerId = TRAINER_JEFFREY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(7),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 21),
        .battleFrontierRecordStreakTextIndex = 7,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 7),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 7),
    },
    {
        .trainerId = TRAINER_CAMERON_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(4),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 22),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 4),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 4),
    },
    {
        .trainerId = TRAINER_JACKI_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 23),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_WALTER_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(12),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 24),
        .battleFrontierRecordStreakTextIndex = 12,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 12),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 12),
    },
    {
        .trainerId = TRAINER_KAREN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(2),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 26),
        .battleFrontierRecordStreakTextIndex = 2,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 2),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 2),
    },
    {
        .trainerId = TRAINER_JERRY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 25),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_ANNA_AND_MEG_1,
        .unused = 6,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 27),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_ISABEL_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(14),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 29),
        .battleFrontierRecordStreakTextIndex = 14,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 14),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 14),
    },
    {
        .trainerId = TRAINER_MIGUEL_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(11),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 28),
        .battleFrontierRecordStreakTextIndex = 11,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 11),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 11),
    },
    {
        .trainerId = TRAINER_TIMOTHY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(12),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 30),
        .battleFrontierRecordStreakTextIndex = 12,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 12),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 12),
    },
    {
        .trainerId = TRAINER_SHELBY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(13),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 31),
        .battleFrontierRecordStreakTextIndex = 13,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 13),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 13),
    },
    {
        .trainerId = TRAINER_CALVIN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 32),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_ELLIOT_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 33),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_ISAIAH_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 38),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_MARIA_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 37),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_ABIGAIL_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 35),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_DYLAN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 36),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_KATELYN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 40),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_BENJAMIN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 34),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_PABLO_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 39),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_NICOLAS_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(4),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 41),
        .battleFrontierRecordStreakTextIndex = 4,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 4),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 4),
    },
    {
        .trainerId = TRAINER_ROBERT_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(6),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 42),
        .battleFrontierRecordStreakTextIndex = 6,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 6),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 6),
    },
    {
        .trainerId = TRAINER_LAO_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 43),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_CYNDY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 44),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_MADELINE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 45),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_JENNY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 46),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_DIANA_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(2),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 47),
        .battleFrontierRecordStreakTextIndex = 2,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 2),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 2),
    },
    {
        .trainerId = TRAINER_AMY_AND_LIV_1,
        .unused = 2,
        .battleTopicTextIds = BATTLE_TEXT_IDS(2),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 48),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 2),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 2),
    },
    {
        .trainerId = TRAINER_ERNEST_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 49),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_CORY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 63),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_EDWIN_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(7),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 50),
        .battleFrontierRecordStreakTextIndex = 7,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 7),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 7),
    },
    {
        .trainerId = TRAINER_LYDIA_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 52),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_ISAAC_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 51),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_GABRIELLE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(8),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 2),
        .battleFrontierRecordStreakTextIndex = 8,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 8),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 8),
    },
    {
        .trainerId = TRAINER_CATHERINE_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 54),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
    {
        .trainerId = TRAINER_JACKSON_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(5),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 53),
        .battleFrontierRecordStreakTextIndex = 5,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 5),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 5),
    },
    {
        .trainerId = TRAINER_HALEY_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(2),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 55),
        .battleFrontierRecordStreakTextIndex = 2,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 2),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 2),
    },
    {
        .trainerId = TRAINER_JAMES_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(1),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 56),
        .battleFrontierRecordStreakTextIndex = 1,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 1),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 1),
    },
    {
        .trainerId = TRAINER_TRENT_1,
        .unused = 0,
        .battleTopicTextIds = BATTLE_TEXT_IDS(3),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 57),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_SAWYER_1,
        .unused = 0,
        // Thalia and Sawyer are the only ones who use different msg ids for their battle topics
        .battleTopicTextIds = { TEXT_ID(B_TOPIC_WILD, 15), TEXT_ID(B_TOPIC_NEGATIVE, 3), TEXT_ID(B_TOPIC_POSITIVE, 3) },
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 1),
        .battleFrontierRecordStreakTextIndex = 3,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 3),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 3),
    },
    {
        .trainerId = TRAINER_KIRA_AND_DAN_1,
        .unused = 1,
        .battleTopicTextIds = BATTLE_TEXT_IDS(9),
        .generalTextId = TEXT_ID(GEN_TOPIC_PERSONAL, 58),
        .battleFrontierRecordStreakTextIndex = 9,
        .sameRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_SAME_ROUTE, 9),
        .differentRouteMatchCallTextId = TEXT_ID(REQ_TOPIC_DIFF_ROUTE, 9),
    },
};

static const struct MatchCallText sMatchCallWildBattleTexts[] =
{
    { .text = MatchCall_WildBattleText1,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText2,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText3,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText4,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText5,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText6,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText7,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText8,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText9,  .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText10, .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText11, .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText12, .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText13, .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText14, .stringVarFuncIds = STRS_WILD_BATTLE },
    { .text = MatchCall_WildBattleText15, .stringVarFuncIds = STRS_WILD_BATTLE },
};

static const struct MatchCallText sMatchCallNegativeBattleTexts[] =
{
    { .text = MatchCall_NegativeBattleText1,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText2,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText3,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText4,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText5,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText6,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText7,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText8,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText9,  .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText10, .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText11, .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText12, .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText13, .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
    { .text = MatchCall_NegativeBattleText14, .stringVarFuncIds = STRS_BATTLE_NEGATIVE },
};

static const struct MatchCallText sMatchCallPositiveBattleTexts[] =
{
    { .text = MatchCall_PositiveBattleText1,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText2,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText3,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText4,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText5,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText6,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText7,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText8,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText9,  .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText10, .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText11, .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText12, .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText13, .stringVarFuncIds = STRS_BATTLE_POSITIVE },
    { .text = MatchCall_PositiveBattleText14, .stringVarFuncIds = STRS_BATTLE_POSITIVE },
};

static const struct MatchCallText sMatchCallSameRouteBattleRequestTexts[] =
{
    { .text = MatchCall_SameRouteBattleRequestText1,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText2,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText3,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText4,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText5,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText6,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText7,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText8,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText9,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText10, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText11, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText12, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText13, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_SameRouteBattleRequestText14, .stringVarFuncIds = STRS_BATTLE_REQUEST },
};

static const struct MatchCallText sMatchCallDifferentRouteBattleRequestTexts[] =
{
    { .text = MatchCall_DifferentRouteBattleRequestText1,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText2,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText3,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText4,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText5,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText6,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText7,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText8,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText9,  .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText10, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText11, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText12, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText13, .stringVarFuncIds = STRS_BATTLE_REQUEST },
    { .text = MatchCall_DifferentRouteBattleRequestText14, .stringVarFuncIds = STRS_BATTLE_REQUEST },
};

static const struct MatchCallText sMatchCallPersonalizedTexts[] =
{
    { .text = MatchCall_PersonalizedText1,  .stringVarFuncIds = { STR_TRAINER_NAME, STR_MAP_NAME, STR_NONE } },
    { .text = MatchCall_PersonalizedText2,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText3,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText4,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText5,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText6,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText7,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText8,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText9,  .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText10, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText11, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText12, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText13, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_ROUTE, STR_NONE } },
    { .text = MatchCall_PersonalizedText14, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText15, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText16, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText17, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText18, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText19, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText20, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText21, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText22, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText23, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText24, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText25, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText26, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText27, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText28, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText29, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText30, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText31, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText32, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText33, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText34, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText35, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText36, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText37, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText38, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText39, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText40, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText41, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText42, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText43, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText44, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText45, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText46, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText47, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText48, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText49, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText50, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText51, .stringVarFuncIds = { STR_TRAINER_NAME, STR_MAP_NAME, STR_NONE } },
    { .text = MatchCall_PersonalizedText52, .stringVarFuncIds = { STR_TRAINER_NAME, STR_SPECIES_IN_PARTY, STR_NONE } },
    { .text = MatchCall_PersonalizedText53, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText54, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText55, .stringVarFuncIds = { STR_TRAINER_NAME, STR_MAP_NAME, STR_NONE } },
    { .text = MatchCall_PersonalizedText56, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText57, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText58, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText59, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText60, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText61, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText62, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText63, .stringVarFuncIds = STRS_NORMAL_MSG },
    { .text = MatchCall_PersonalizedText64, .stringVarFuncIds = STRS_NORMAL_MSG },
};

static const struct MatchCallText sMatchCallBattleFrontierStreakTexts[] =
{
    { .text = MatchCall_BattleFrontierStreakText1,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText2,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText3,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText4,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText5,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText6,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText7,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText8,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText9,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText10, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText11, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText12, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText13, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierStreakText14, .stringVarFuncIds = STRS_FRONTIER },
};

static const struct MatchCallText sMatchCallBattleFrontierRecordStreakTexts[] =
{
    { .text = MatchCall_BattleFrontierRecordStreakText1,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText2,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText3,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText4,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText5,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText6,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText7,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText8,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText9,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText10, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText11, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText12, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText13, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleFrontierRecordStreakText14, .stringVarFuncIds = STRS_FRONTIER },
};

static const struct MatchCallText sMatchCallBattleDomeTexts[] =
{
    { .text = MatchCall_BattleDomeText1,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText2,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText3,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText4,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText5,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText6,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText7,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText8,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText9,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText10, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText11, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText12, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText13, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattleDomeText14, .stringVarFuncIds = STRS_FRONTIER },
};

static const struct MatchCallText sMatchCallBattlePikeTexts[] =
{
    { .text = MatchCall_BattlePikeText1,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText2,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText3,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText4,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText5,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText6,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText7,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText8,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText9,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText10, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText11, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText12, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText13, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePikeText14, .stringVarFuncIds = STRS_FRONTIER },
};

static const struct MatchCallText sMatchCallBattlePyramidTexts[] =
{
    { .text = MatchCall_BattlePyramidText1,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText2,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText3,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText4,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText5,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText6,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText7,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText8,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText9,  .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText10, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText11, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText12, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText13, .stringVarFuncIds = STRS_FRONTIER },
    { .text = MatchCall_BattlePyramidText14, .stringVarFuncIds = STRS_FRONTIER },
};

static const struct MatchCallText *const sMatchCallBattleTopics[] =
{
    [B_TOPIC_WILD - 1]     = sMatchCallWildBattleTexts,
    [B_TOPIC_NEGATIVE - 1] = sMatchCallNegativeBattleTexts,
    [B_TOPIC_POSITIVE - 1] = sMatchCallPositiveBattleTexts,
};

static const struct MatchCallText *const sMatchCallBattleRequestTopics[] =
{
    [REQ_TOPIC_SAME_ROUTE - 1] = sMatchCallSameRouteBattleRequestTexts,
    [REQ_TOPIC_DIFF_ROUTE - 1] = sMatchCallDifferentRouteBattleRequestTexts,
};

static const struct MatchCallText *const sMatchCallGeneralTopics[] =
{
    [GEN_TOPIC_PERSONAL - 1]      = sMatchCallPersonalizedTexts,
    [GEN_TOPIC_STREAK - 1]        = sMatchCallBattleFrontierStreakTexts,
    [GEN_TOPIC_STREAK_RECORD - 1] = sMatchCallBattleFrontierRecordStreakTexts,
    [GEN_TOPIC_B_DOME - 1]        = sMatchCallBattleDomeTexts,
    [GEN_TOPIC_B_PIKE - 1]        = sMatchCallBattlePikeTexts,
    [GEN_TOPIC_B_PYRAMID - 1]     = sMatchCallBattlePyramidTexts,
};

extern const u8 gBirchDexRatingText_AreYouCurious[];
extern const u8 gBirchDexRatingText_SoYouveSeenAndCaught[];
extern const u8 gBirchDexRatingText_OnANationwideBasis[];

void InitMatchCallCounters(void)
{
    RtcCalcLocalTime();
    sMatchCallState.minutes = GetCurrentTotalMinutes(&gLocalTime) + 10;
    sMatchCallState.stepCounter = 0;
}

static u32 GetCurrentTotalMinutes(struct Time *time)
{
    return time->days * 24 * 60 + time->hours * 60 + time->minutes;
}

static bool32 UpdateMatchCallMinutesCounter(void)
{
    int curMinutes;
    RtcCalcLocalTime();
    curMinutes = GetCurrentTotalMinutes(&gLocalTime);
    if (sMatchCallState.minutes > curMinutes || curMinutes - sMatchCallState.minutes > 9)
    {
        sMatchCallState.minutes = curMinutes;
        return TRUE;
    }

    return FALSE;
}

static bool32 CheckMatchCallChance(void)
{
    int callChance = 1;
    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG) && GetMonAbility(&gPlayerParty[0]) == ABILITY_LIGHTNING_ROD)
        callChance = 2;

    if (Random() % 10 < callChance * 3)
        return TRUE;
    else
        return FALSE;
}

static bool32 MapAllowsMatchCall(void)
{
    if (!Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) || gMapHeader.regionMapSectionId == MAPSEC_SAFARI_ZONE)
        return FALSE;

    if (gMapHeader.regionMapSectionId == MAPSEC_SOOTOPOLIS_CITY
     && FlagGet(FLAG_HIDE_SOOTOPOLIS_CITY_RAYQUAZA) == TRUE
     && FlagGet(FLAG_NEVER_SET_0x0DC) == FALSE)
        return FALSE;

    if (gMapHeader.regionMapSectionId == MAPSEC_MT_CHIMNEY
     && FlagGet(FLAG_MET_ARCHIE_METEOR_FALLS) == TRUE
     && FlagGet(FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY) == FALSE)
        return FALSE;

    return TRUE;
}

static bool32 UpdateMatchCallStepCounter(void)
{
    if (++sMatchCallState.stepCounter >= 10)
    {
        sMatchCallState.stepCounter = 0;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static bool32 SelectMatchCallTrainer(void)
{
    u32 matchCallId;
    u32 numRegistered = GetNumRegisteredTrainers();
    if (numRegistered == 0)
        return FALSE;

    sMatchCallState.trainerId = GetActiveMatchCallTrainerId(Random() % numRegistered);
    sMatchCallState.triggeredFromScript = FALSE;
    if (sMatchCallState.trainerId == REMATCH_TABLE_ENTRIES)
        return FALSE;

    matchCallId = GetTrainerMatchCallId(sMatchCallState.trainerId);
    if (GetRematchTrainerLocation(matchCallId) == gMapHeader.regionMapSectionId && !TrainerIsEligibleForRematch(matchCallId))
        return FALSE;

    return TRUE;
}

// Ignores registrable non-trainer NPCs, and special trainers like Wally and the gym leaders.
static u32 GetNumRegisteredTrainers(void)
{
    u32 i, count;
    for (i = 0, count = 0; i < REMATCH_SPECIAL_TRAINER_START; i++)
    {
        if (FlagGet(TRAINER_REGISTERED_FLAGS_START + i))
            count++;
    }

    return count;
}

static u32 GetActiveMatchCallTrainerId(u32 activeMatchCallId)
{
    u32 i;
    for (i = 0; i < REMATCH_SPECIAL_TRAINER_START; i++)
    {
        if (FlagGet(TRAINER_REGISTERED_FLAGS_START + i))
        {
            if (!activeMatchCallId)
                return gRematchTable[i].trainerIds[0];

            activeMatchCallId--;
        }
    }

    return REMATCH_TABLE_ENTRIES;
}

/*
    From the function calls below, a call can only be triggered...
    - If the player has match call
    - Every 10th step
    - Every 10 minutes
    - 1/3 of the time (or 2/3 of the time, if the lead party Pokémon has Lightning Rod)
    - If in a valid outdoor map (not Safari Zone, not underwater, not Mt Chimney with Team Magma, not Sootopolis with legendaries)
    - If an eligible trainer to call the player is selected
*/
bool32 TryStartMatchCall(void)
{
    if (FlagGet(FLAG_HAS_MATCH_CALL)
        && UpdateMatchCallStepCounter()
        && UpdateMatchCallMinutesCounter()
        && CheckMatchCallChance()
        && MapAllowsMatchCall()
        && SelectMatchCallTrainer())
    {
        StartMatchCall();
        return TRUE;
    }

    return FALSE;
}

void StartMatchCallFromScript(const u8 *message)
{
    sMatchCallState.triggeredFromScript = TRUE;
    StartMatchCall();
}

bool32 IsMatchCallTaskActive(void)
{
    return FuncIsActiveTask(ExecuteMatchCall);
}

static void StartMatchCall(void)
{
    if (!sMatchCallState.triggeredFromScript)
    {
        LockPlayerFieldControls();
        FreezeObjectEvents();
        PlayerFreeze();
        StopPlayerAvatar();
    }

    PlaySE(SE_POKENAV_CALL);
    CreateTask(ExecuteMatchCall, 1);
}

static const u16 sMatchCallWindow_Pal[] = INCBIN_U16("graphics/pokenav/match_call/window.gbapal");
static const u8 sMatchCallWindow_Gfx[] = INCBIN_U8("graphics/pokenav/match_call/window.4bpp");
static const u16 sPokenavIcon_Pal[] = INCBIN_U16("graphics/pokenav/match_call/nav_icon.gbapal");
static const u32 sPokenavIcon_Gfx[] = INCBIN_U32("graphics/pokenav/match_call/nav_icon.4bpp.lz");

static const u8 sText_PokenavCallEllipsis[] = _("………………\p");

#define tState      data[0]
#define tWindowId   data[2]
#define tIconTaskId data[5]

static bool32 (*const sMatchCallTaskFuncs[])(u8) =
{
    MatchCall_LoadGfx,
    MatchCall_DrawWindow,
    MatchCall_ReadyIntro,
    MatchCall_SlideWindowIn,
    MatchCall_PrintIntro,
    MatchCall_PrintMessage,
    MatchCall_SlideWindowOut,
    MatchCall_EndCall,
};

static void ExecuteMatchCall(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (sMatchCallTaskFuncs[tState](taskId))
    {
        tState++;
        data[1] = 0; // Never read
        if ((u16)tState > 7)
            DestroyTask(taskId);
    }
}

static const struct WindowTemplate sMatchCallTextWindow =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 28,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x200
};

#define TILE_MC_WINDOW    0x270
#define TILE_POKENAV_ICON 0x279

static bool32 MatchCall_LoadGfx(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    tWindowId = AddWindow(&sMatchCallTextWindow);
    if (tWindowId == WINDOW_NONE)
    {
        DestroyTask(taskId);
        return FALSE;
    }

    if (LoadBgTiles(0, sMatchCallWindow_Gfx, sizeof(sMatchCallWindow_Gfx), TILE_MC_WINDOW) == 0xFFFF)
    {
        RemoveWindow(tWindowId);
        DestroyTask(taskId);
        return FALSE;
    }

    if (!DecompressAndCopyTileDataToVram(0, sPokenavIcon_Gfx, 0, TILE_POKENAV_ICON, 0))
    {
        RemoveWindow(tWindowId);
        DestroyTask(taskId);
        return FALSE;
    }

    FillWindowPixelBuffer(tWindowId, PIXEL_FILL(8));
    LoadPalette(sMatchCallWindow_Pal, BG_PLTT_ID(14), sizeof(sMatchCallWindow_Pal));
    LoadPalette(sPokenavIcon_Pal, BG_PLTT_ID(15), sizeof(sPokenavIcon_Pal));
    ChangeBgY(0, -0x2000, BG_COORD_SET);
    return TRUE;
}

static bool32 MatchCall_DrawWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (FreeTempTileDataBuffersIfPossible())
        return FALSE;

    PutWindowTilemap(tWindowId);
    DrawMatchCallTextBoxBorder_Internal(tWindowId, TILE_MC_WINDOW, 14);
    WriteSequenceToBgTilemapBuffer(0, (0xF << 12) | TILE_POKENAV_ICON, 1, 15, 4, 4, 17, 1);
    tIconTaskId = CreateTask(Task_SpinPokenavIcon, 10);
    CopyWindowToVram(tWindowId, COPYWIN_GFX);
    CopyBgTilemapBufferToVram(0);
    return TRUE;
}

static bool32 MatchCall_ReadyIntro(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!IsDma3ManagerBusyWithBgCopy())
    {
        // Note that "..." is not printed yet, just readied
        InitMatchCallTextPrinter(tWindowId, sText_PokenavCallEllipsis);
        return TRUE;
    }

    return FALSE;
}

static bool32 MatchCall_SlideWindowIn(u8 taskId)
{
    if (ChangeBgY(0, 0x600, BG_COORD_ADD) >= 0)
    {
        ChangeBgY(0, 0, BG_COORD_SET);
        return TRUE;
    }

    return FALSE;
}

static bool32 MatchCall_PrintIntro(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!RunMatchCallTextPrinter(tWindowId))
    {
        FillWindowPixelBuffer(tWindowId, PIXEL_FILL(8));

        // Ready the message
        if (!sMatchCallState.triggeredFromScript)
            SelectMatchCallMessage(sMatchCallState.trainerId, gStringVarBuffer);
        InitMatchCallTextPrinter(tWindowId, gStringVarBuffer);
        return TRUE;
    }

    return FALSE;
}

static bool32 MatchCall_PrintMessage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!RunMatchCallTextPrinter(tWindowId) && !IsSEPlaying() && JOY_NEW(A_BUTTON | B_BUTTON))
    {
        FillWindowPixelBuffer(tWindowId, PIXEL_FILL(8));
        CopyWindowToVram(tWindowId, COPYWIN_GFX);
        PlaySE(SE_POKENAV_HANG_UP);
        return TRUE;
    }

    return FALSE;
}

static bool32 MatchCall_SlideWindowOut(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (ChangeBgY(0, 0x600, BG_COORD_SUB) <= -0x2000)
    {
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 14, 30, 6);
        DestroyTask(tIconTaskId);
        RemoveWindow(tWindowId);
        CopyBgTilemapBufferToVram(0);
        return TRUE;
    }

    return FALSE;
}

static bool32 MatchCall_EndCall(u8 taskId)
{
    u8 playerObjectId;
    if (!IsDma3ManagerBusyWithBgCopy() && !IsSEPlaying())
    {
        ChangeBgY(0, 0, BG_COORD_SET);
        if (!sMatchCallState.triggeredFromScript)
        {
            LoadMessageBoxAndBorderGfx();
            playerObjectId = GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0);
            ObjectEventClearHeldMovementIfFinished(&gObjectEvents[playerObjectId]);
            ScriptMovement_UnfreezeObjectEvents();
            UnfreezeObjectEvents();
            UnlockPlayerFieldControls();
        }

        return TRUE;
    }

    return FALSE;
}

static void DrawMatchCallTextBoxBorder_Internal(u32 windowId, u32 tileOffset, u32 paletteId)
{
    int bg, x, y, width, height;
    int tileNum;

    bg = GetWindowAttribute(windowId, WINDOW_BG);
    x = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    y = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    height = GetWindowAttribute(windowId, WINDOW_HEIGHT);
    tileNum = tileOffset + GetBgAttribute(bg, BG_ATTR_BASETILE);

    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 0), x - 1, y - 1, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 1), x, y - 1, width, 1);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 2), x + width, y - 1, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 3), x - 1, y, 1, height);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 4), x + width, y, 1, height);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 5), x - 1, y + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 6), x, y + height, width, 1);
    FillBgTilemapBufferRect_Palette0(bg, ((paletteId << 12) & 0xF000) | (tileNum + 7), x + width, y + height, 1, 1);
}

static void InitMatchCallTextPrinter(int windowId, const u8 *str)
{
    struct TextPrinterTemplate printerTemplate;
    printerTemplate.currentChar = str;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = FONT_NORMAL;
    printerTemplate.x = 32;
    printerTemplate.y = 1;
    printerTemplate.currentX = 32;
    printerTemplate.currentY = 1;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = TEXT_DYNAMIC_COLOR_1;
    printerTemplate.bgColor = TEXT_COLOR_BLUE;
    printerTemplate.shadowColor = TEXT_DYNAMIC_COLOR_5;
    gTextFlags.useAlternateDownArrow = FALSE;

    AddTextPrinter(&printerTemplate, GetPlayerTextSpeedDelay(), NULL);
}

static bool32 RunMatchCallTextPrinter(int windowId)
{
    if (JOY_HELD(A_BUTTON))
        gTextFlags.canABSpeedUpPrint = TRUE;
    else
        gTextFlags.canABSpeedUpPrint = FALSE;

    RunTextPrinters();
    return IsTextPrinterActive(windowId);
}

#define tTimer     data[0]
#define tSpinStage data[1]
#define tTileNum   data[2]

static void Task_SpinPokenavIcon(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (++tTimer > 8)
    {
        tTimer = 0;
        if (++tSpinStage > 7)
            tSpinStage = 0;

        tTileNum = (tSpinStage * 16) + TILE_POKENAV_ICON;
        WriteSequenceToBgTilemapBuffer(0, tTileNum | ~0xFFF, 1, 15, 4, 4, 17, 1);
        CopyBgTilemapBufferToVram(0);
    }
}

#undef tTimer
#undef tSpinStage
#undef tTileNum

static bool32 TrainerIsEligibleForRematch(int matchCallId)
{
    return gSaveBlock1Ptr->trainerRematches[matchCallId] > 0;
}

static u16 GetRematchTrainerLocation(int matchCallId)
{
    const struct MapHeader *mapHeader = Overworld_GetMapHeaderByGroupAndId(gRematchTable[matchCallId].mapGroup, gRematchTable[matchCallId].mapNum);
    return mapHeader->regionMapSectionId;
}

static u32 GetNumRematchTrainersFought(void)
{
    u32 i, count;
    for (i = 0, count = 0; i < REMATCH_SPECIAL_TRAINER_START; i++)
    {
        if (HasTrainerBeenFought(gRematchTable[i].trainerIds[0]))
            count++;
    }

    return count;
}

// Look through the rematch table for trainers that have been defeated once before.
// Return the index into the rematch table of the nth defeated trainer,
// or REMATCH_TABLE_ENTRIES if fewer than n rematch trainers have been defeated.
static u32 GetNthRematchTrainerFought(int n)
{
    u32 i, count;

    for (i = 0, count = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        if (HasTrainerBeenFought(gRematchTable[i].trainerIds[0]))
        {
            if (count == n)
                return i;

            count++;
        }
    }

    return REMATCH_TABLE_ENTRIES;
}

bool32 SelectMatchCallMessage(int trainerId, u8 *str)
{
    u32 matchCallId;
    const struct MatchCallText *matchCallText;
    bool32 newRematchRequest = FALSE;

    matchCallId = GetTrainerMatchCallId(trainerId);
    sBattleFrontierStreakInfo.facilityId = 0;

    // If the player is on the same route as the trainer
    // and they can be rematched, they will always request a battle
    if (TrainerIsEligibleForRematch(matchCallId)
     && GetRematchTrainerLocation(matchCallId) == gMapHeader.regionMapSectionId)
    {
        matchCallText = GetSameRouteMatchCallText(matchCallId, str);
    }
    // If the player is not on the same route as the trainer
    // and they can be rematched, there is a random chance for
    // the trainer to request a battle
    else if (ShouldTrainerRequestBattle(matchCallId))
    {
        matchCallText = GetDifferentRouteMatchCallText(matchCallId, str);
        newRematchRequest = TRUE;
        UpdateRematchIfDefeated(matchCallId);
    }
    else if (Random() % 3)
    {
        // Message talking about a battle the NPC had
        matchCallText = GetBattleMatchCallText(matchCallId, str);
    }
    else
    {
        // Message talking about something else
        matchCallText = GetGeneralMatchCallText(matchCallId, str);
    }

    BuildMatchCallString(matchCallId, matchCallText, str);
    return newRematchRequest;
}

static int GetTrainerMatchCallId(int trainerId)
{
    int i = 0;
    while (1)
    {
        if (sMatchCallTrainers[i].trainerId == trainerId)
            return i;
        else
            i++;
    }
}

static const struct MatchCallText *GetSameRouteMatchCallText(int matchCallId, u8 *str)
{
    u16 textId = sMatchCallTrainers[matchCallId].sameRouteMatchCallTextId;
    int mask = 0xFF;
    u32 topic = (textId >> 8) - 1;
    u32 id = (textId & mask) - 1;
    return &sMatchCallBattleRequestTopics[topic][id];
}

static const struct MatchCallText *GetDifferentRouteMatchCallText(int matchCallId, u8 *str)
{
    u16 textId = sMatchCallTrainers[matchCallId].differentRouteMatchCallTextId;
    int mask = 0xFF;
    u32 topic = (textId >> 8) - 1;
    u32 id = (textId & mask) - 1;
    return &sMatchCallBattleRequestTopics[topic][id];
}

static const struct MatchCallText *GetBattleMatchCallText(int matchCallId, u8 *str)
{
    int mask;
    u32 textId, topic, id;

    topic = Random() % 3;
    textId = sMatchCallTrainers[matchCallId].battleTopicTextIds[topic];
    if (!textId)
        SpriteCallbackDummy(NULL); // leftover debugging ???

    mask = 0xFF;
    topic = (textId >> 8) - 1;
    id = (textId & mask) - 1;
    return &sMatchCallBattleTopics[topic][id];
}

static const struct MatchCallText *GetGeneralMatchCallText(int matchCallId, u8 *str)
{
    int i;
    int count;
    u32 topic, id;
    u16 rand;

    rand = Random();
    if (!(rand & 1))
    {
        // Count the number of facilities with a win streak
        for (count = 0, i = 0; i < NUM_FRONTIER_FACILITIES; i++)
        {
            if (GetFrontierStreakInfo(i, &topic) > 1)
                count++;
        }

        if (count)
        {
            // At least one facility with a win streak
            // Randomly choose one to have a call about
            count = Random() % count;
            for (i = 0; i < NUM_FRONTIER_FACILITIES; i++)
            {
                sBattleFrontierStreakInfo.streak = GetFrontierStreakInfo(i, &topic);
                if (sBattleFrontierStreakInfo.streak < 2)
                    continue;

                if (!count)
                    break;

                count--;
            }

            sBattleFrontierStreakInfo.facilityId = i;
            id = sMatchCallTrainers[matchCallId].battleFrontierRecordStreakTextIndex - 1;
            return &sMatchCallGeneralTopics[topic][id];
        }
    }

    topic = (sMatchCallTrainers[matchCallId].generalTextId >> 8) - 1;
    id = (sMatchCallTrainers[matchCallId].generalTextId & 0xFF) - 1;
    return &sMatchCallGeneralTopics[topic][id];
}

static void BuildMatchCallString(int matchCallId, const struct MatchCallText *matchCallText, u8 *str)
{
    PopulateMatchCallStringVars(matchCallId, matchCallText->stringVarFuncIds);
    StringExpandPlaceholders(str, matchCallText->text);
}

static u8 *const sMatchCallTextStringVars[] = { gStringVar1, gStringVar2, gStringVar3 };

static void PopulateMatchCallStringVars(int matchCallId, const s8 *stringVarFuncIds)
{
    int i;
    for (i = 0; i < NUM_STRVARS_IN_MSG; i++)
    {
        if (stringVarFuncIds[i] >= 0)
            PopulateMatchCallStringVar(matchCallId, stringVarFuncIds[i], sMatchCallTextStringVars[i]);
    }
}

static void (*const sPopulateMatchCallStringVarFuncs[])(int, u8 *) =
{
    [STR_TRAINER_NAME]     = PopulateTrainerName,
    [STR_MAP_NAME]         = PopulateMapName,
    [STR_SPECIES_IN_ROUTE] = PopulateSpeciesFromTrainerLocation,
    [STR_SPECIES_IN_PARTY] = PopulateSpeciesFromTrainerParty,
    [STR_FACILITY_NAME]    = PopulateBattleFrontierFacilityName,
    [STR_FRONTIER_STREAK]  = PopulateBattleFrontierStreak,
};

static void PopulateMatchCallStringVar(int matchCallId, int funcId, u8 *destStr)
{
    sPopulateMatchCallStringVarFuncs[funcId](matchCallId, destStr);
}

static const struct MultiTrainerMatchCallText sMultiTrainerMatchCallTexts[] =
{
    { .trainerId = TRAINER_KIRA_AND_DAN_1, .text = gText_Kira },
    { .trainerId = TRAINER_AMY_AND_LIV_1,  .text = gText_Amy },
    { .trainerId = TRAINER_JOHN_AND_JAY_1, .text = gText_John },
    { .trainerId = TRAINER_LILA_AND_ROY_1, .text = gText_Roy },
    { .trainerId = TRAINER_GABBY_AND_TY_1, .text = gText_Gabby },
    { .trainerId = TRAINER_ANNA_AND_MEG_1, .text = gText_Anna },
};

static void PopulateTrainerName(int matchCallId, u8 *destStr)
{
    u32 i;
    u16 trainerId = sMatchCallTrainers[matchCallId].trainerId;
    for (i = 0; i < ARRAY_COUNT(sMultiTrainerMatchCallTexts); i++)
    {
        if (sMultiTrainerMatchCallTexts[i].trainerId == trainerId)
        {
            StringCopy(destStr, sMultiTrainerMatchCallTexts[i].text);
            return;
        }
    }

    StringCopy(destStr, gTrainers[trainerId].trainerName);
}

static void PopulateMapName(int matchCallId, u8 *destStr)
{
    GetMapName(destStr, GetRematchTrainerLocation(matchCallId), 0);
}

static u8 GetLandEncounterSlot(void)
{
    int rand = Random() % 100;
    if (rand < 20)
        return 0;
    else if (rand >= 20 && rand < 40)
        return 1;
    else if (rand >= 40 && rand < 50)
        return 2;
    else if (rand >= 50 && rand < 60)
        return 3;
    else if (rand >= 60 && rand < 70)
        return 4;
    else if (rand >= 70 && rand < 80)
        return 5;
    else if (rand >= 80 && rand < 85)
        return 6;
    else if (rand >= 85 && rand < 90)
        return 7;
    else if (rand >= 90 && rand < 94)
        return 8;
    else if (rand >= 94 && rand < 98)
        return 9;
    else if (rand >= 98 && rand < 99)
        return 10;
    else
        return 11;
}

static u8 GetWaterEncounterSlot(void)
{
    int rand = Random() % 100;
    if (rand < 60)
        return 0;
    else if (rand >= 60 && rand < 90)
        return 1;
    else if (rand >= 90 && rand < 95)
        return 2;
    else if (rand >= 95 && rand < 99)
        return 3;
    else
        return 4;
}

static void PopulateSpeciesFromTrainerLocation(int matchCallId, u8 *destStr)
{
    u16 species[2];
    int numSpecies;
    u8 slot;
    int i = 0;

    if (gWildMonHeaders[i].mapGroup != MAP_GROUP(UNDEFINED)) // ??? This check is nonsense.
    {
        while (gWildMonHeaders[i].mapGroup != MAP_GROUP(UNDEFINED))
        {
            if (gWildMonHeaders[i].mapGroup == gRematchTable[matchCallId].mapGroup
             && gWildMonHeaders[i].mapNum == gRematchTable[matchCallId].mapNum)
                break;

            i++;
        }

        if (gWildMonHeaders[i].mapGroup != MAP_GROUP(UNDEFINED))
        {
            numSpecies = 0;
            if (gWildMonHeaders[i].landMonsInfo)
            {
                slot = GetLandEncounterSlot();
                species[numSpecies] = gWildMonHeaders[i].landMonsInfo->wildPokemon[slot].species;
                numSpecies++;
            }

            if (gWildMonHeaders[i].waterMonsInfo)
            {
                slot = GetWaterEncounterSlot();
                species[numSpecies] = gWildMonHeaders[i].waterMonsInfo->wildPokemon[slot].species;
                numSpecies++;
            }

            if (numSpecies)
            {
                StringCopy(destStr, gSpeciesNames[species[Random() % numSpecies]]);
                return;
            }
        }
    }

    destStr[0] = EOS;
}

static void PopulateSpeciesFromTrainerParty(int matchCallId, u8 *destStr)
{
    u16 trainerId;
    union TrainerMonPtr party;
    u8 monId;
    const u8 *speciesName;

    trainerId = GetLastBeatenRematchTrainerId(sMatchCallTrainers[matchCallId].trainerId);
    party = gTrainers[trainerId].party;
    monId = Random() % gTrainers[trainerId].partySize;

    switch (gTrainers[trainerId].partyFlags)
    {
    case 0:
    default:
        speciesName = gSpeciesNames[party.NoItemDefaultMoves[monId].species];
        break;
    case F_TRAINER_PARTY_CUSTOM_MOVESET:
        speciesName = gSpeciesNames[party.NoItemCustomMoves[monId].species];
        break;
    case F_TRAINER_PARTY_HELD_ITEM:
        speciesName = gSpeciesNames[party.ItemDefaultMoves[monId].species];
        break;
    case F_TRAINER_PARTY_CUSTOM_MOVESET | F_TRAINER_PARTY_HELD_ITEM:
        speciesName = gSpeciesNames[party.ItemCustomMoves[monId].species];
        break;
    }

    StringCopy(destStr, speciesName);
}

static const u8 *const sBattleFrontierFacilityNames[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = gText_BattleTower2,
    [FRONTIER_FACILITY_DOME]    = gText_BattleDome,
    [FRONTIER_FACILITY_PALACE]  = gText_BattlePalace,
    [FRONTIER_FACILITY_ARENA]   = gText_BattleArena,
    [MATCH_CALL_PIKE]           = gText_BattlePike,
    [MATCH_CALL_FACTORY]        = gText_BattleFactory,
    [FRONTIER_FACILITY_PYRAMID] = gText_BattlePyramid,
};

static void PopulateBattleFrontierFacilityName(int matchCallId, u8 *destStr)
{
    StringCopy(destStr, sBattleFrontierFacilityNames[sBattleFrontierStreakInfo.facilityId]);
}

static void PopulateBattleFrontierStreak(int matchCallId, u8 *destStr)
{
    int i = 0;
    int streak = sBattleFrontierStreakInfo.streak;
    while (streak != 0)
    {
        streak /= 10;
        i++;
    }

    ConvertIntToDecimalStringN(destStr, sBattleFrontierStreakInfo.streak, STR_CONV_MODE_LEFT_ALIGN, i);
}

static const u16 sBadgeFlags[NUM_BADGES] =
{
    FLAG_BADGE01_GET,
    FLAG_BADGE02_GET,
    FLAG_BADGE03_GET,
    FLAG_BADGE04_GET,
    FLAG_BADGE05_GET,
    FLAG_BADGE06_GET,
    FLAG_BADGE07_GET,
    FLAG_BADGE08_GET,
};

static int GetNumOwnedBadges(void)
{
    u32 i;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (!FlagGet(sBadgeFlags[i]))
            break;
    }

    return i;
}

// Whether or not a trainer calling the player from a different route should request a battle
static bool32 ShouldTrainerRequestBattle(int matchCallId)
{
    int dayCount;
    int otId;
    u16 dewfordRand;
    int numRematchTrainersFought;
    int max, rand, n;

    if (GetNumOwnedBadges() < 5)
        return FALSE;

    dayCount = RtcGetLocalDayCount();
    otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId) & 0xFFFF;

    dewfordRand = gSaveBlock1Ptr->dewfordTrends[0].rand;
    numRematchTrainersFought = GetNumRematchTrainersFought();
    max = (numRematchTrainersFought * 13) / 10;
    rand = ((dayCount ^ dewfordRand) + (dewfordRand ^ GetGameStat(GAME_STAT_TRAINER_BATTLES))) ^ otId;
    n = rand % max;
    if (n < numRematchTrainersFought)
    {
        if (GetNthRematchTrainerFought(n) == matchCallId)
            return TRUE;
    }

    return FALSE;
}

static u16 GetFrontierStreakInfo(u16 facilityId, u32 *topicTextId)
{
    int i;
    int j;
    u16 streak = 0;

    switch (facilityId)
    {
    case FRONTIER_FACILITY_DOME:
        for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.domeRecordWinStreaks); i++)
        {
            for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            {
                if (streak < gSaveBlock2Ptr->frontier.domeRecordWinStreaks[i][j])
                    streak = gSaveBlock2Ptr->frontier.domeRecordWinStreaks[i][j];
            }
        }
        *topicTextId = GEN_TOPIC_B_DOME - 1;
        break;
    case MATCH_CALL_PIKE:
        for (i = 0; i < FRONTIER_LVL_MODE_COUNT; i++)
        {
            if (streak < gSaveBlock2Ptr->frontier.pikeRecordStreaks[i])
                streak = gSaveBlock2Ptr->frontier.pikeRecordStreaks[i];
        }
        *topicTextId = GEN_TOPIC_B_PIKE - 1;
        break;
    case FRONTIER_FACILITY_TOWER:
        for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.towerRecordWinStreaks); i++)
        {
            for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            {
                if (streak < gSaveBlock2Ptr->frontier.towerRecordWinStreaks[i][j])
                    streak = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[i][j];
            }
        }
        *topicTextId = GEN_TOPIC_STREAK_RECORD - 1;
        break;
    case FRONTIER_FACILITY_PALACE:
        for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.palaceRecordWinStreaks); i++)
        {
            for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            {
                if (streak < gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[i][j])
                    streak = gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[i][j];
            }
        }
        *topicTextId = GEN_TOPIC_STREAK_RECORD - 1;
        break;
    case MATCH_CALL_FACTORY:
        for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.factoryRecordWinStreaks); i++)
        {
            for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            {
                if (streak < gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[i][j])
                    streak = gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[i][j];
            }
        }
        *topicTextId = GEN_TOPIC_STREAK_RECORD - 1;
        break;
    case FRONTIER_FACILITY_ARENA:
        for (i = 0; i < FRONTIER_LVL_MODE_COUNT; i++)
        {
            if (streak < gSaveBlock2Ptr->frontier.arenaRecordStreaks[i])
                streak = gSaveBlock2Ptr->frontier.arenaRecordStreaks[i];
        }
        *topicTextId = GEN_TOPIC_STREAK_RECORD - 1;
        break;
    case FRONTIER_FACILITY_PYRAMID:
        for (i = 0; i < FRONTIER_LVL_MODE_COUNT; i++)
        {
            if (streak < gSaveBlock2Ptr->frontier.pyramidRecordStreaks[i])
                streak = gSaveBlock2Ptr->frontier.pyramidRecordStreaks[i];
        }
        *topicTextId = GEN_TOPIC_B_PYRAMID - 1;
        break;
    }

    return streak;
}

static u8 GetPokedexRatingLevel(u16 numSeen)
{
    if (numSeen < 10)
        return 0;
    if (numSeen < 20)
        return 1;
    if (numSeen < 30)
        return 2;
    if (numSeen < 40)
        return 3;
    if (numSeen < 50)
        return 4;
    if (numSeen < 60)
        return 5;
    if (numSeen < 70)
        return 6;
    if (numSeen < 80)
        return 7;
    if (numSeen < 90)
        return 8;
    if (numSeen < 100)
        return 9;
    if (numSeen < 110)
        return 10;
    if (numSeen < 120)
        return 11;
    if (numSeen < 130)
        return 12;
    if (numSeen < 140)
        return 13;
    if (numSeen < 150)
        return 14;
    if (numSeen < 160)
        return 15;
    if (numSeen < 170)
        return 16;
    if (numSeen < 180)
        return 17;
    if (numSeen < 190)
        return 18;
    if (numSeen < 200)
        return 19;

    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_DEOXYS), FLAG_GET_CAUGHT))
        numSeen--;
    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_JIRACHI), FLAG_GET_CAUGHT))
        numSeen--;

    if (numSeen < 200)
        return 19;
    else
        return 20;
}

static const u8 *const sBirchDexRatingTexts[] =
{
    gBirchDexRatingText_LessThan10,
    gBirchDexRatingText_LessThan20,
    gBirchDexRatingText_LessThan30,
    gBirchDexRatingText_LessThan40,
    gBirchDexRatingText_LessThan50,
    gBirchDexRatingText_LessThan60,
    gBirchDexRatingText_LessThan70,
    gBirchDexRatingText_LessThan80,
    gBirchDexRatingText_LessThan90,
    gBirchDexRatingText_LessThan100,
    gBirchDexRatingText_LessThan110,
    gBirchDexRatingText_LessThan120,
    gBirchDexRatingText_LessThan130,
    gBirchDexRatingText_LessThan140,
    gBirchDexRatingText_LessThan150,
    gBirchDexRatingText_LessThan160,
    gBirchDexRatingText_LessThan170,
    gBirchDexRatingText_LessThan180,
    gBirchDexRatingText_LessThan190,
    gBirchDexRatingText_LessThan200,
    gBirchDexRatingText_DexCompleted,
};

void BufferPokedexRatingForMatchCall(u8 *destStr)
{
    int numSeen, numCaught;
    u8 *str;
    u8 dexRatingLevel;

    u8 *buffer = Alloc(sizeof(gStringVarBuffer));
    if (!buffer)
    {
        destStr[0] = EOS;
        return;
    }

    numSeen = GetHoennPokedexCount(FLAG_GET_SEEN);
    numCaught = GetHoennPokedexCount(FLAG_GET_CAUGHT);
    ConvertIntToDecimalStringN(gStringVar1, numSeen, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, numCaught, STR_CONV_MODE_LEFT_ALIGN, 3);
    dexRatingLevel = GetPokedexRatingLevel(numCaught);
    str = StringCopy(buffer, gBirchDexRatingText_AreYouCurious);
    *(str++) = CHAR_PROMPT_CLEAR;
    str = StringCopy(str, gBirchDexRatingText_SoYouveSeenAndCaught);
    *(str++) = CHAR_PROMPT_CLEAR;
    StringCopy(str, sBirchDexRatingTexts[dexRatingLevel]);
    str = StringExpandPlaceholders(destStr, buffer);

    if (IsNationalPokedexEnabled())
    {
        *(str++) = CHAR_PROMPT_CLEAR;
        numSeen = GetNationalPokedexCount(FLAG_GET_SEEN);
        numCaught = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        ConvertIntToDecimalStringN(gStringVar1, numSeen, STR_CONV_MODE_LEFT_ALIGN, 3);
        ConvertIntToDecimalStringN(gStringVar2, numCaught, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(str, gBirchDexRatingText_OnANationwideBasis);
    }

    Free(buffer);
}

void LoadMatchCallWindowGfx(u32 windowId, u32 destOffset, u32 paletteId)
{
    u8 bg = GetWindowAttribute(windowId, WINDOW_BG);
    LoadBgTiles(bg, sMatchCallWindow_Gfx, 0x100, destOffset);
    LoadPalette(sMatchCallWindow_Pal, BG_PLTT_ID(paletteId), sizeof(sMatchCallWindow_Pal));
}

void DrawMatchCallTextBoxBorder(u32 windowId, u32 tileOffset, u32 paletteId)
{
    DrawMatchCallTextBoxBorder_Internal(windowId, tileOffset, paletteId);
}
