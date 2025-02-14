#include "global.h"
#include "battle_tower.h"
#include "apprentice.h"
#include "event_data.h"
#include "battle_setup.h"
#include "overworld.h"
#include "random.h"
#include "text.h"
#include "main.h"
#include "international_string_util.h"
#include "battle.h"
#include "frontier_util.h"
#include "strings.h"
#include "recorded_battle.h"
#include "easy_chat.h"
#include "gym_leader_rematch.h"
#include "battle_transition.h"
#include "trainer_see.h"
#include "new_game.h"
#include "string_util.h"
#include "data.h"
#include "link.h"
#include "field_message_box.h"
#include "tv.h"
#include "battle_factory.h"
#include "constants/apprentice.h"
#include "constants/battle_dome.h"
#include "constants/battle_frontier.h"
#include "constants/battle_frontier_mons.h"
#include "constants/battle_tent.h"
#include "constants/battle_tent_mons.h"
#include "constants/battle_tent_trainers.h"
#include "constants/battle_tower.h"
#include "constants/frontier_util.h"
#include "constants/items.h"
#include "constants/trainers.h"
#include "constants/event_objects.h"
#include "constants/moves.h"

extern const u8 MossdeepCity_SpaceCenter_2F_EventScript_MaxieTrainer[];
extern const u8 MossdeepCity_SpaceCenter_2F_EventScript_TabithaTrainer[];

// EWRAM vars.
EWRAM_DATA const struct BattleFrontierTrainer *gFacilityTrainers = NULL;
EWRAM_DATA const struct FacilityMon *gFacilityTrainerMons = NULL;

// IWRAM common
COMMON_DATA u16 gFrontierTempParty[MAX_FRONTIER_PARTY_SIZE] = {0};

// This file's functions.
static void InitTowerChallenge(void);
static void GetTowerData(void);
static void SetTowerData(void);
static void SetNextFacilityOpponent(void);
static void SetTowerBattleWon(void);
static void AwardBattleTowerRibbons(void);
static void SaveTowerChallenge(void);
static void GetOpponentIntroSpeech(void);
static void BattleTowerNop1(void);
static void BattleTowerNop2(void);
static void LoadMultiPartnerCandidatesData(void);
static void ShowPartnerCandidateMessage(void);
static void LoadLinkMultiOpponentsData(void);
static void TowerTryCloseLink(void);
static void SetMultiPartnerGfx(void);
static void SetTowerInterviewData(void);
static void ValidateBattleTowerRecordChecksums(void);
static void SaveCurrentWinStreak(void);
static void ValidateApprenticesChecksums(void);
static void SetNextBattleTentOpponent(void);
static void CopyEReaderTrainerFarewellMessage(void);
static void ClearBattleTowerRecord(struct EmeraldBattleTowerRecord *record);
static void FillTrainerParty(u16 trainerId, u8 firstMonId, u8 monCount);
static void FillTentTrainerParty_(u16 trainerId, u8 firstMonId, u8 monCount);
static void FillFactoryFrontierTrainerParty(u16 trainerId, u8 firstMonId);
static void FillFactoryTentTrainerParty(u16 trainerId, u8 firstMonId);
static u8 GetFrontierTrainerFixedIvs(u16 trainerId);
static void FillPartnerParty(u16 trainerId);
static void SetEReaderTrainerChecksum(struct BattleTowerEReaderTrainer *ereaderTrainer);
static u8 SetTentPtrsGetLevel(void);

const u16 gBattleFrontierHeldItems[] =
{
    [BATTLE_FRONTIER_ITEM_NONE]           = ITEM_NONE,
    [BATTLE_FRONTIER_ITEM_KINGS_ROCK]     = ITEM_KINGS_ROCK,
    [BATTLE_FRONTIER_ITEM_SITRUS_BERRY]   = ITEM_SITRUS_BERRY,
    [BATTLE_FRONTIER_ITEM_ORAN_BERRY]     = ITEM_ORAN_BERRY,
    [BATTLE_FRONTIER_ITEM_CHESTO_BERRY]   = ITEM_CHESTO_BERRY,
    [BATTLE_FRONTIER_ITEM_HARD_STONE]     = ITEM_HARD_STONE,
    [BATTLE_FRONTIER_ITEM_FOCUS_BAND]     = ITEM_FOCUS_BAND,
    [BATTLE_FRONTIER_ITEM_PERSIM_BERRY]   = ITEM_PERSIM_BERRY,
    [BATTLE_FRONTIER_ITEM_MIRACLE_SEED]   = ITEM_MIRACLE_SEED,
    [BATTLE_FRONTIER_ITEM_BERRY_JUICE]    = ITEM_BERRY_JUICE,
    [BATTLE_FRONTIER_ITEM_MACHO_BRACE]    = ITEM_MACHO_BRACE,
    [BATTLE_FRONTIER_ITEM_SILVER_POWDER]  = ITEM_SILVER_POWDER,
    [BATTLE_FRONTIER_ITEM_CHERI_BERRY]    = ITEM_CHERI_BERRY,
    [BATTLE_FRONTIER_ITEM_BLACK_GLASSES]  = ITEM_BLACK_GLASSES,
    [BATTLE_FRONTIER_ITEM_BLACK_BELT]     = ITEM_BLACK_BELT,
    [BATTLE_FRONTIER_ITEM_SOUL_DEW]       = ITEM_SOUL_DEW,
    [BATTLE_FRONTIER_ITEM_CHOICE_BAND]    = ITEM_CHOICE_BAND,
    [BATTLE_FRONTIER_ITEM_MAGNET]         = ITEM_MAGNET,
    [BATTLE_FRONTIER_ITEM_SILK_SCARF]     = ITEM_SILK_SCARF,
    [BATTLE_FRONTIER_ITEM_WHITE_HERB]     = ITEM_WHITE_HERB,
    [BATTLE_FRONTIER_ITEM_DEEP_SEA_SCALE] = ITEM_DEEP_SEA_SCALE,
    [BATTLE_FRONTIER_ITEM_DEEP_SEA_TOOTH] = ITEM_DEEP_SEA_TOOTH,
    [BATTLE_FRONTIER_ITEM_MYSTIC_WATER]   = ITEM_MYSTIC_WATER,
    [BATTLE_FRONTIER_ITEM_SHARP_BEAK]     = ITEM_SHARP_BEAK,
    [BATTLE_FRONTIER_ITEM_QUICK_CLAW]     = ITEM_QUICK_CLAW,
    [BATTLE_FRONTIER_ITEM_LEFTOVERS]      = ITEM_LEFTOVERS,
    [BATTLE_FRONTIER_ITEM_RAWST_BERRY]    = ITEM_RAWST_BERRY,
    [BATTLE_FRONTIER_ITEM_LIGHT_BALL]     = ITEM_LIGHT_BALL,
    [BATTLE_FRONTIER_ITEM_POISON_BARB]    = ITEM_POISON_BARB,
    [BATTLE_FRONTIER_ITEM_NEVER_MELT_ICE] = ITEM_NEVER_MELT_ICE,
    [BATTLE_FRONTIER_ITEM_ASPEAR_BERRY]   = ITEM_ASPEAR_BERRY,
    [BATTLE_FRONTIER_ITEM_SPELL_TAG]      = ITEM_SPELL_TAG,
    [BATTLE_FRONTIER_ITEM_BRIGHT_POWDER]  = ITEM_BRIGHT_POWDER,
    [BATTLE_FRONTIER_ITEM_LEPPA_BERRY]    = ITEM_LEPPA_BERRY,
    [BATTLE_FRONTIER_ITEM_SCOPE_LENS]     = ITEM_SCOPE_LENS,
    [BATTLE_FRONTIER_ITEM_TWISTED_SPOON]  = ITEM_TWISTED_SPOON,
    [BATTLE_FRONTIER_ITEM_METAL_COAT]     = ITEM_METAL_COAT,
    [BATTLE_FRONTIER_ITEM_MENTAL_HERB]    = ITEM_MENTAL_HERB,
    [BATTLE_FRONTIER_ITEM_CHARCOAL]       = ITEM_CHARCOAL,
    [BATTLE_FRONTIER_ITEM_PECHA_BERRY]    = ITEM_PECHA_BERRY,
    [BATTLE_FRONTIER_ITEM_SOFT_SAND]      = ITEM_SOFT_SAND,
    [BATTLE_FRONTIER_ITEM_LUM_BERRY]      = ITEM_LUM_BERRY,
    [BATTLE_FRONTIER_ITEM_DRAGON_SCALE]   = ITEM_DRAGON_SCALE,
    [BATTLE_FRONTIER_ITEM_DRAGON_FANG]    = ITEM_DRAGON_FANG,
    [BATTLE_FRONTIER_ITEM_IAPAPA_BERRY]   = ITEM_IAPAPA_BERRY,
    [BATTLE_FRONTIER_ITEM_WIKI_BERRY]     = ITEM_WIKI_BERRY,
    [BATTLE_FRONTIER_ITEM_SEA_INCENSE]    = ITEM_SEA_INCENSE,
    [BATTLE_FRONTIER_ITEM_SHELL_BELL]     = ITEM_SHELL_BELL,
    [BATTLE_FRONTIER_ITEM_SALAC_BERRY]    = ITEM_SALAC_BERRY,
    [BATTLE_FRONTIER_ITEM_LANSAT_BERRY]   = ITEM_LANSAT_BERRY,
    [BATTLE_FRONTIER_ITEM_APICOT_BERRY]   = ITEM_APICOT_BERRY,
    [BATTLE_FRONTIER_ITEM_STARF_BERRY]    = ITEM_STARF_BERRY,
    [BATTLE_FRONTIER_ITEM_LIECHI_BERRY]   = ITEM_LIECHI_BERRY,
    [BATTLE_FRONTIER_ITEM_STICK]          = ITEM_STICK,
    [BATTLE_FRONTIER_ITEM_LAX_INCENSE]    = ITEM_LAX_INCENSE,
    [BATTLE_FRONTIER_ITEM_AGUAV_BERRY]    = ITEM_AGUAV_BERRY,
    [BATTLE_FRONTIER_ITEM_FIGY_BERRY]     = ITEM_FIGY_BERRY,
    [BATTLE_FRONTIER_ITEM_THICK_CLUB]     = ITEM_THICK_CLUB,
    [BATTLE_FRONTIER_ITEM_MAGO_BERRY]     = ITEM_MAGO_BERRY,
    [BATTLE_FRONTIER_ITEM_METAL_POWDER]   = ITEM_METAL_POWDER,
    [BATTLE_FRONTIER_ITEM_PETAYA_BERRY]   = ITEM_PETAYA_BERRY,
    [BATTLE_FRONTIER_ITEM_LUCKY_PUNCH]    = ITEM_LUCKY_PUNCH,
    [BATTLE_FRONTIER_ITEM_GANLON_BERRY]   = ITEM_GANLON_BERRY,
};

#include "data/battle_frontier/battle_frontier_trainer_mons.h"
#include "data/battle_frontier/battle_frontier_trainers.h"
#include "data/battle_frontier/battle_frontier_mons.h"

const u8 gTowerMaleFacilityClasses[30] =
{
    FACILITY_CLASS_RUIN_MANIAC,
    FACILITY_CLASS_TUBER_M,
    FACILITY_CLASS_COOLTRAINER_M,
    FACILITY_CLASS_RICH_BOY,
    FACILITY_CLASS_POKEMANIAC,
    FACILITY_CLASS_SWIMMER_M,
    FACILITY_CLASS_BLACK_BELT,
    FACILITY_CLASS_GUITARIST,
    FACILITY_CLASS_KINDLER,
    FACILITY_CLASS_CAMPER,
    FACILITY_CLASS_BUG_MANIAC,
    FACILITY_CLASS_PSYCHIC_M,
    FACILITY_CLASS_GENTLEMAN,
    FACILITY_CLASS_SCHOOL_KID_M,
    FACILITY_CLASS_POKEFAN_M,
    FACILITY_CLASS_EXPERT_M,
    FACILITY_CLASS_YOUNGSTER,
    FACILITY_CLASS_FISHERMAN,
    FACILITY_CLASS_CYCLING_TRIATHLETE_M,
    FACILITY_CLASS_RUNNING_TRIATHLETE_M,
    FACILITY_CLASS_SWIMMING_TRIATHLETE_M,
    FACILITY_CLASS_DRAGON_TAMER,
    FACILITY_CLASS_BIRD_KEEPER,
    FACILITY_CLASS_NINJA_BOY,
    FACILITY_CLASS_SAILOR,
    FACILITY_CLASS_COLLECTOR,
    FACILITY_CLASS_PKMN_BREEDER_M,
    FACILITY_CLASS_PKMN_RANGER_M,
    FACILITY_CLASS_BUG_CATCHER,
    FACILITY_CLASS_HIKER
};

const u8 gTowerFemaleFacilityClasses[20] =
{
    FACILITY_CLASS_AROMA_LADY,
    FACILITY_CLASS_TUBER_F,
    FACILITY_CLASS_COOLTRAINER_F,
    FACILITY_CLASS_HEX_MANIAC,
    FACILITY_CLASS_LADY,
    FACILITY_CLASS_BEAUTY,
    FACILITY_CLASS_PSYCHIC_F,
    FACILITY_CLASS_SCHOOL_KID_F,
    FACILITY_CLASS_POKEFAN_F,
    FACILITY_CLASS_EXPERT_F,
    FACILITY_CLASS_CYCLING_TRIATHLETE_F,
    FACILITY_CLASS_RUNNING_TRIATHLETE_F,
    FACILITY_CLASS_SWIMMING_TRIATHLETE_F,
    FACILITY_CLASS_BATTLE_GIRL,
    FACILITY_CLASS_PARASOL_LADY,
    FACILITY_CLASS_SWIMMER_F,
    FACILITY_CLASS_PICNICKER,
    FACILITY_CLASS_PKMN_BREEDER_F,
    FACILITY_CLASS_PKMN_RANGER_F,
    FACILITY_CLASS_LASS
};

const u8 gTowerMaleTrainerGfxIds[30] =
{
    OBJ_EVENT_GFX_HIKER,
    OBJ_EVENT_GFX_TUBER_M,
    OBJ_EVENT_GFX_MAN_3,
    OBJ_EVENT_GFX_RICH_BOY,
    OBJ_EVENT_GFX_MANIAC,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M,
    OBJ_EVENT_GFX_BLACK_BELT,
    OBJ_EVENT_GFX_MAN_5,
    OBJ_EVENT_GFX_MAN_5,
    OBJ_EVENT_GFX_CAMPER,
    OBJ_EVENT_GFX_MANIAC,
    OBJ_EVENT_GFX_PSYCHIC_M,
    OBJ_EVENT_GFX_GENTLEMAN,
    OBJ_EVENT_GFX_SCHOOL_KID_M,
    OBJ_EVENT_GFX_POKEFAN_M,
    OBJ_EVENT_GFX_EXPERT_M,
    OBJ_EVENT_GFX_YOUNGSTER,
    OBJ_EVENT_GFX_FISHERMAN,
    OBJ_EVENT_GFX_CYCLING_TRIATHLETE_M,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M,
    OBJ_EVENT_GFX_MAN_3,
    OBJ_EVENT_GFX_MAN_5,
    OBJ_EVENT_GFX_NINJA_BOY,
    OBJ_EVENT_GFX_SAILOR,
    OBJ_EVENT_GFX_MANIAC,
    OBJ_EVENT_GFX_MAN_4,
    OBJ_EVENT_GFX_CAMPER,
    OBJ_EVENT_GFX_BUG_CATCHER,
    OBJ_EVENT_GFX_HIKER
};

const u8 gTowerFemaleTrainerGfxIds[20] =
{
    OBJ_EVENT_GFX_WOMAN_2,
    OBJ_EVENT_GFX_TUBER_F,
    OBJ_EVENT_GFX_WOMAN_5,
    OBJ_EVENT_GFX_HEX_MANIAC,
    OBJ_EVENT_GFX_WOMAN_2,
    OBJ_EVENT_GFX_BEAUTY,
    OBJ_EVENT_GFX_LASS,
    OBJ_EVENT_GFX_GIRL_3,
    OBJ_EVENT_GFX_POKEFAN_F,
    OBJ_EVENT_GFX_EXPERT_F,
    OBJ_EVENT_GFX_CYCLING_TRIATHLETE_F,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F,
    OBJ_EVENT_GFX_GIRL_3,
    OBJ_EVENT_GFX_WOMAN_5,
    OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F,
    OBJ_EVENT_GFX_PICNICKER,
    OBJ_EVENT_GFX_WOMAN_2,
    OBJ_EVENT_GFX_PICNICKER,
    OBJ_EVENT_GFX_LASS
};

// Excludes the unused RS_FACILITY_CLASS_BOARDER_1 and _2
static const u8 sRubyFacilityClassToEmerald[RS_FACILITY_CLASSES_COUNT - 2][2] =
{
    {RS_FACILITY_CLASS_AQUA_LEADER_ARCHIE, FACILITY_CLASS_AQUA_LEADER_ARCHIE},
    {RS_FACILITY_CLASS_AQUA_GRUNT_M, FACILITY_CLASS_AQUA_GRUNT_M},
    {RS_FACILITY_CLASS_AQUA_GRUNT_F, FACILITY_CLASS_AQUA_GRUNT_F},
    {RS_FACILITY_CLASS_AROMA_LADY, FACILITY_CLASS_AROMA_LADY},
    {RS_FACILITY_CLASS_RUIN_MANIAC, FACILITY_CLASS_RUIN_MANIAC},
    {RS_FACILITY_CLASS_INTERVIEWER, FACILITY_CLASS_INTERVIEWER},
    {RS_FACILITY_CLASS_TUBER_F, FACILITY_CLASS_TUBER_F},
    {RS_FACILITY_CLASS_TUBER_M, FACILITY_CLASS_TUBER_M},
    {RS_FACILITY_CLASS_COOLTRAINER_M, FACILITY_CLASS_COOLTRAINER_M},
    {RS_FACILITY_CLASS_COOLTRAINER_F, FACILITY_CLASS_COOLTRAINER_F},
    {RS_FACILITY_CLASS_HEX_MANIAC, FACILITY_CLASS_HEX_MANIAC},
    {RS_FACILITY_CLASS_LADY, FACILITY_CLASS_LADY},
    {RS_FACILITY_CLASS_BEAUTY, FACILITY_CLASS_BEAUTY},
    {RS_FACILITY_CLASS_RICH_BOY, FACILITY_CLASS_RICH_BOY},
    {RS_FACILITY_CLASS_POKEMANIAC, FACILITY_CLASS_POKEMANIAC},
    {RS_FACILITY_CLASS_SWIMMER_M, FACILITY_CLASS_SWIMMER_M},
    {RS_FACILITY_CLASS_BLACK_BELT, FACILITY_CLASS_BLACK_BELT},
    {RS_FACILITY_CLASS_GUITARIST, FACILITY_CLASS_GUITARIST},
    {RS_FACILITY_CLASS_KINDLER, FACILITY_CLASS_KINDLER},
    {RS_FACILITY_CLASS_CAMPER, FACILITY_CLASS_CAMPER},
    {RS_FACILITY_CLASS_BUG_MANIAC, FACILITY_CLASS_BUG_MANIAC},
    {RS_FACILITY_CLASS_PSYCHIC_M, FACILITY_CLASS_PSYCHIC_M},
    {RS_FACILITY_CLASS_PSYCHIC_F, FACILITY_CLASS_PSYCHIC_F},
    {RS_FACILITY_CLASS_GENTLEMAN, FACILITY_CLASS_GENTLEMAN},
    {RS_FACILITY_CLASS_ELITE_FOUR_M, FACILITY_CLASS_ELITE_FOUR_SIDNEY},
    {RS_FACILITY_CLASS_ELITE_FOUR_F, FACILITY_CLASS_ELITE_FOUR_PHOEBE},
    {RS_FACILITY_CLASS_LEADER_F, FACILITY_CLASS_LEADER_ROXANNE},
    {RS_FACILITY_CLASS_LEADER_M, FACILITY_CLASS_LEADER_BRAWLY},
    {RS_FACILITY_CLASS_LEADER_MF, FACILITY_CLASS_LEADER_TATE_AND_LIZA},
    {RS_FACILITY_CLASS_SCHOOL_KID_M, FACILITY_CLASS_SCHOOL_KID_M},
    {RS_FACILITY_CLASS_SCHOOL_KID_F, FACILITY_CLASS_SCHOOL_KID_F},
    {RS_FACILITY_CLASS_SR_AND_JR, FACILITY_CLASS_SR_AND_JR},
    {RS_FACILITY_CLASS_POKEFAN_M, FACILITY_CLASS_POKEFAN_M},
    {RS_FACILITY_CLASS_POKEFAN_F, FACILITY_CLASS_POKEFAN_F},
    {RS_FACILITY_CLASS_EXPERT_M, FACILITY_CLASS_EXPERT_M},
    {RS_FACILITY_CLASS_EXPERT_F, FACILITY_CLASS_EXPERT_F},
    {RS_FACILITY_CLASS_YOUNGSTER, FACILITY_CLASS_YOUNGSTER},
    {RS_FACILITY_CLASS_CHAMPION, FACILITY_CLASS_CHAMPION_WALLACE},
    {RS_FACILITY_CLASS_FISHERMAN, FACILITY_CLASS_FISHERMAN},
    {RS_FACILITY_CLASS_CYCLING_TRIATHLETE_M, FACILITY_CLASS_CYCLING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_CYCLING_TRIATHLETE_F, FACILITY_CLASS_CYCLING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_RUNNING_TRIATHLETE_M, FACILITY_CLASS_RUNNING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_RUNNING_TRIATHLETE_F, FACILITY_CLASS_RUNNING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_M, FACILITY_CLASS_SWIMMING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_F, FACILITY_CLASS_SWIMMING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_DRAGON_TAMER, FACILITY_CLASS_DRAGON_TAMER},
    {RS_FACILITY_CLASS_BIRD_KEEPER, FACILITY_CLASS_BIRD_KEEPER},
    {RS_FACILITY_CLASS_NINJA_BOY, FACILITY_CLASS_NINJA_BOY},
    {RS_FACILITY_CLASS_BATTLE_GIRL, FACILITY_CLASS_BATTLE_GIRL},
    {RS_FACILITY_CLASS_PARASOL_LADY, FACILITY_CLASS_PARASOL_LADY},
    {RS_FACILITY_CLASS_SWIMMER_F, FACILITY_CLASS_SWIMMER_F},
    {RS_FACILITY_CLASS_PICNICKER, FACILITY_CLASS_PICNICKER},
    {RS_FACILITY_CLASS_TWINS, FACILITY_CLASS_TWINS},
    {RS_FACILITY_CLASS_SAILOR, FACILITY_CLASS_SAILOR},
    {RS_FACILITY_CLASS_COLLECTOR, FACILITY_CLASS_COLLECTOR},
    {RS_FACILITY_CLASS_WALLY, FACILITY_CLASS_WALLY},
    {RS_FACILITY_CLASS_BRENDAN_1, FACILITY_CLASS_BRENDAN},
    {RS_FACILITY_CLASS_BRENDAN_2, FACILITY_CLASS_BRENDAN_2},
    {RS_FACILITY_CLASS_BRENDAN_3, FACILITY_CLASS_BRENDAN_3},
    {RS_FACILITY_CLASS_MAY_1, FACILITY_CLASS_MAY},
    {RS_FACILITY_CLASS_MAY_2, FACILITY_CLASS_MAY_2},
    {RS_FACILITY_CLASS_MAY_3, FACILITY_CLASS_MAY_3},
    {RS_FACILITY_CLASS_PKMN_BREEDER_M, FACILITY_CLASS_PKMN_BREEDER_M},
    {RS_FACILITY_CLASS_PKMN_BREEDER_F, FACILITY_CLASS_PKMN_BREEDER_F},
    {RS_FACILITY_CLASS_PKMN_RANGER_M, FACILITY_CLASS_PKMN_RANGER_M},
    {RS_FACILITY_CLASS_PKMN_RANGER_F, FACILITY_CLASS_PKMN_RANGER_F},
    {RS_FACILITY_CLASS_MAGMA_LEADER, FACILITY_CLASS_MAGMA_LEADER_MAXIE},
    {RS_FACILITY_CLASS_MAGMA_GRUNT_M, FACILITY_CLASS_MAGMA_GRUNT_M},
    {RS_FACILITY_CLASS_MAGMA_GRUNT_F, FACILITY_CLASS_MAGMA_GRUNT_F},
    {RS_FACILITY_CLASS_LASS, FACILITY_CLASS_LASS},
    {RS_FACILITY_CLASS_BUG_CATCHER, FACILITY_CLASS_BUG_CATCHER},
    {RS_FACILITY_CLASS_HIKER, FACILITY_CLASS_HIKER},
    {RS_FACILITY_CLASS_YOUNG_COUPLE, FACILITY_CLASS_YOUNG_COUPLE},
    {RS_FACILITY_CLASS_OLD_COUPLE, FACILITY_CLASS_OLD_COUPLE},
    {RS_FACILITY_CLASS_SIS_AND_BRO, FACILITY_CLASS_SIS_AND_BRO},
};

#define PARTNER_TEXTS(name)                                             \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Intro,      \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Mon1,       \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Mon2Ask,    \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Accept,     \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Reject

static const u8 *const sPartnerApprenticeTexts1[] =
{
    PARTNER_TEXTS(Apprentice1)
};

static const u8 *const sPartnerApprenticeTexts2[] =
{
    PARTNER_TEXTS(Apprentice2)
};

static const u8 *const sPartnerApprenticeTexts3[] =
{
    PARTNER_TEXTS(Apprentice3)
};

static const u8 *const sPartnerApprenticeTexts4[] =
{
    PARTNER_TEXTS(Apprentice4)
};

static const u8 *const sPartnerApprenticeTexts5[] =
{
    PARTNER_TEXTS(Apprentice5)
};

static const u8 *const sPartnerApprenticeTexts6[] =
{
     PARTNER_TEXTS(Apprentice6)
};

static const u8 *const sPartnerApprenticeTexts7[] =
{
     PARTNER_TEXTS(Apprentice7)
};

static const u8 *const sPartnerApprenticeTexts8[] =
{
     PARTNER_TEXTS(Apprentice8)
};

static const u8 *const sPartnerApprenticeTexts9[] =
{
     PARTNER_TEXTS(Apprentice9)
};

static const u8 *const sPartnerApprenticeTexts10[] =
{
     PARTNER_TEXTS(Apprentice10)
};

static const u8 *const sPartnerApprenticeTexts11[] =
{
     PARTNER_TEXTS(Apprentice11)
};

static const u8 *const sPartnerApprenticeTexts12[] =
{
     PARTNER_TEXTS(Apprentice12)
};

static const u8 *const sPartnerApprenticeTexts13[] =
{
     PARTNER_TEXTS(Apprentice13)
};

static const u8 *const sPartnerApprenticeTexts14[] =
{
     PARTNER_TEXTS(Apprentice14)
};

static const u8 *const sPartnerApprenticeTexts15[] =
{
     PARTNER_TEXTS(Apprentice15)
};

static const u8 *const sPartnerApprenticeTexts16[] =
{
     PARTNER_TEXTS(Apprentice16)
};

static const u8 *const sPartnerTextsLass[] =
{
    PARTNER_TEXTS(Lass)
};

static const u8 *const sPartnerTextsYoungster[] =
{
    PARTNER_TEXTS(Youngster)
};

static const u8 *const sPartnerTextsHiker[] =
{
    PARTNER_TEXTS(Hiker)
};

static const u8 *const sPartnerTextsBeauty[] =
{
    PARTNER_TEXTS(Beauty)
};

static const u8 *const sPartnerTextsFisherman[] =
{
    PARTNER_TEXTS(Fisherman)
};

static const u8 *const sPartnerTextsLady[] =
{
    PARTNER_TEXTS(Lady)
};

static const u8 *const sPartnerTextsCyclingTriathleteF[] =
{
    PARTNER_TEXTS(CyclingTriathleteF)
};

static const u8 *const sPartnerTextsBugCatcher[] =
{
    PARTNER_TEXTS(BugCatcher)
};

static const u8 *const sPartnerTextsSchoolKidM[] =
{
    PARTNER_TEXTS(SchoolKidM)
};

static const u8 *const sPartnerTextsRichBoy[] =
{
    PARTNER_TEXTS(RichBoy)
};

static const u8 *const sPartnerTextsBlackBelt[] =
{
    PARTNER_TEXTS(BlackBelt)
};

static const u8 *const sPartnerTextsTuberF[] =
{
    PARTNER_TEXTS(TuberF)
};

static const u8 *const sPartnerTextsHexManiac[] =
{
    PARTNER_TEXTS(HexManiac)
};

static const u8 *const sPartnerTextsPkmnBreederM[] =
{
    PARTNER_TEXTS(PkmnBreederM)
};

static const u8 *const sPartnerTextsRunningTriathleteF[] =
{
    PARTNER_TEXTS(RunningTriathleteF)
};

static const u8 *const sPartnerTextsRunningTriathleteM[] =
{
    PARTNER_TEXTS(RunningTriathleteM)
};

static const u8 *const sPartnerTextsBattleGirl[] =
{
    PARTNER_TEXTS(BattleGirl)
};

static const u8 *const sPartnerTextsCyclingTriathleteM[] =
{
    PARTNER_TEXTS(CyclingTriathleteM)
};

static const u8 *const sPartnerTextsTuberM[] =
{
    PARTNER_TEXTS(TuberM)
};

static const u8 *const sPartnerTextsGuitarist[] =
{
    PARTNER_TEXTS(Guitarist)
};

static const u8 *const sPartnerTextsGentleman[] =
{
    PARTNER_TEXTS(Gentleman)
};

static const u8 *const sPartnerTextsPokefanM[] =
{
    PARTNER_TEXTS(PokefanM)
};

static const u8 *const sPartnerTextsExpertM[] =
{
    PARTNER_TEXTS(ExpertM)
};

static const u8 *const sPartnerTextsExpertF[] =
{
    PARTNER_TEXTS(ExpertF)
};

static const u8 *const sPartnerTextsDragonTamer[] =
{
    PARTNER_TEXTS(DragonTamer)
};

static const u8 *const sPartnerTextsBirdKeeper[] =
{
    PARTNER_TEXTS(BirdKeeper)
};

static const u8 *const sPartnerTextsNinjaBoy[] =
{
    PARTNER_TEXTS(NinjaBoy)
};

static const u8 *const sPartnerTextsParasolLady[] =
{
    PARTNER_TEXTS(ParasolLady)
};

static const u8 *const sPartnerTextsBugManiac[] =
{
    PARTNER_TEXTS(BugManiac)
};

static const u8 *const sPartnerTextsSailor[] =
{
    PARTNER_TEXTS(Sailor)
};

static const u8 *const sPartnerTextsCollector[] =
{
    PARTNER_TEXTS(Collector)
};

static const u8 *const sPartnerTextsPkmnRangerM[] =
{
    PARTNER_TEXTS(PkmnRangerM)
};

static const u8 *const sPartnerTextsPkmnRangerF[] =
{
    PARTNER_TEXTS(PkmnRangerF)
};

static const u8 *const sPartnerTextsAromaLady[] =
{
    PARTNER_TEXTS(AromaLady)
};

static const u8 *const sPartnerTextsRuinManiac[] =
{
    PARTNER_TEXTS(RuinManiac)
};

static const u8 *const sPartnerTextsCoolTrainerM[] =
{
    PARTNER_TEXTS(CoolTrainerM)
};

static const u8 *const sPartnerTextsCoolTrainerF[] =
{
    PARTNER_TEXTS(CoolTrainerF)
};

static const u8 *const sPartnerTextsPokemaniac[] =
{
    PARTNER_TEXTS(Pokemaniac)
};

static const u8 *const sPartnerTextsKindler[] =
{
    PARTNER_TEXTS(Kindler)
};

static const u8 *const sPartnerTextsCamper[] =
{
    PARTNER_TEXTS(Camper)
};

static const u8 *const sPartnerTextsPicnicker[] =
{
    PARTNER_TEXTS(Picnicker)
};

static const u8 *const sPartnerTextsPsychicM[] =
{
    PARTNER_TEXTS(PsychicM)
};

static const u8 *const sPartnerTextsPsychicF[] =
{
    PARTNER_TEXTS(PsychicF)
};

static const u8 *const sPartnerTextsSchoolKidF[] =
{
    PARTNER_TEXTS(SchoolKidF)
};

static const u8 *const sPartnerTextsPkmnBreederF[] =
{
    PARTNER_TEXTS(PkmnBreederF)
};

static const u8 *const sPartnerTextsPokefanF[] =
{
    PARTNER_TEXTS(PokefanF)
};

static const u8 *const sPartnerTextsSwimmerF[] =
{
    PARTNER_TEXTS(SwimmerF)
};

static const u8 *const sPartnerTextsSwimmingTriathleteM[] =
{
    PARTNER_TEXTS(SwimmingTriathleteM)
};

static const u8 *const sPartnerTextsSwimmingTriathleteF[] =
{
    PARTNER_TEXTS(SwimmingTriathleteF)
};

static const u8 *const sPartnerTextsSwimmerM[] =
{
    PARTNER_TEXTS(SwimmerM)
};

struct
{
    u32 facilityClass;
    const u8 *const *strings;
} static const sPartnerTrainerTextTables[] =
{
    {FACILITY_CLASS_LASS,                  sPartnerTextsLass},
    {FACILITY_CLASS_YOUNGSTER,             sPartnerTextsYoungster},
    {FACILITY_CLASS_HIKER,                 sPartnerTextsHiker},
    {FACILITY_CLASS_BEAUTY,                sPartnerTextsBeauty},
    {FACILITY_CLASS_FISHERMAN,             sPartnerTextsFisherman},
    {FACILITY_CLASS_LADY,                  sPartnerTextsLady},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_F,  sPartnerTextsCyclingTriathleteF},
    {FACILITY_CLASS_BUG_CATCHER,           sPartnerTextsBugCatcher},
    {FACILITY_CLASS_SCHOOL_KID_M,          sPartnerTextsSchoolKidM},
    {FACILITY_CLASS_RICH_BOY,              sPartnerTextsRichBoy},
    {FACILITY_CLASS_BLACK_BELT,            sPartnerTextsBlackBelt},
    {FACILITY_CLASS_TUBER_F,               sPartnerTextsTuberF},
    {FACILITY_CLASS_HEX_MANIAC,            sPartnerTextsHexManiac},
    {FACILITY_CLASS_PKMN_BREEDER_M,        sPartnerTextsPkmnBreederM},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_F,  sPartnerTextsRunningTriathleteF},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_M,  sPartnerTextsRunningTriathleteM},
    {FACILITY_CLASS_BATTLE_GIRL,           sPartnerTextsBattleGirl},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_M,  sPartnerTextsCyclingTriathleteM},
    {FACILITY_CLASS_TUBER_M,               sPartnerTextsTuberM},
    {FACILITY_CLASS_GUITARIST,             sPartnerTextsGuitarist},
    {FACILITY_CLASS_GENTLEMAN,             sPartnerTextsGentleman},
    {FACILITY_CLASS_POKEFAN_M,             sPartnerTextsPokefanM},
    {FACILITY_CLASS_EXPERT_M,              sPartnerTextsExpertM},
    {FACILITY_CLASS_EXPERT_F,              sPartnerTextsExpertF},
    {FACILITY_CLASS_DRAGON_TAMER,          sPartnerTextsDragonTamer},
    {FACILITY_CLASS_BIRD_KEEPER,           sPartnerTextsBirdKeeper},
    {FACILITY_CLASS_NINJA_BOY,             sPartnerTextsNinjaBoy},
    {FACILITY_CLASS_PARASOL_LADY,          sPartnerTextsParasolLady},
    {FACILITY_CLASS_BUG_MANIAC,            sPartnerTextsBugManiac},
    {FACILITY_CLASS_SAILOR,                sPartnerTextsSailor},
    {FACILITY_CLASS_COLLECTOR,             sPartnerTextsCollector},
    {FACILITY_CLASS_PKMN_RANGER_M,         sPartnerTextsPkmnRangerM},
    {FACILITY_CLASS_PKMN_RANGER_F,         sPartnerTextsPkmnRangerF},
    {FACILITY_CLASS_AROMA_LADY,            sPartnerTextsAromaLady},
    {FACILITY_CLASS_RUIN_MANIAC,           sPartnerTextsRuinManiac},
    {FACILITY_CLASS_COOLTRAINER_M,         sPartnerTextsCoolTrainerM},
    {FACILITY_CLASS_COOLTRAINER_F,         sPartnerTextsCoolTrainerF},
    {FACILITY_CLASS_POKEMANIAC,            sPartnerTextsPokemaniac},
    {FACILITY_CLASS_KINDLER,               sPartnerTextsKindler},
    {FACILITY_CLASS_CAMPER,                sPartnerTextsCamper},
    {FACILITY_CLASS_PICNICKER,             sPartnerTextsPicnicker},
    {FACILITY_CLASS_PSYCHIC_M,             sPartnerTextsPsychicM},
    {FACILITY_CLASS_PSYCHIC_F,             sPartnerTextsPsychicF},
    {FACILITY_CLASS_SCHOOL_KID_F,          sPartnerTextsSchoolKidF},
    {FACILITY_CLASS_PKMN_BREEDER_F,        sPartnerTextsPkmnBreederF},
    {FACILITY_CLASS_POKEFAN_F,             sPartnerTextsPokefanF},
    {FACILITY_CLASS_SWIMMER_F,             sPartnerTextsSwimmerF},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_M, sPartnerTextsSwimmingTriathleteM},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_F, sPartnerTextsSwimmingTriathleteF},
    {FACILITY_CLASS_SWIMMER_M,             sPartnerTextsSwimmerM}
};

static const u8 *const *const sPartnerApprenticeTextTables[NUM_APPRENTICES] =
{
    sPartnerApprenticeTexts1,
    sPartnerApprenticeTexts2,
    sPartnerApprenticeTexts3,
    sPartnerApprenticeTexts4,
    sPartnerApprenticeTexts5,
    sPartnerApprenticeTexts6,
    sPartnerApprenticeTexts7,
    sPartnerApprenticeTexts8,
    sPartnerApprenticeTexts9,
    sPartnerApprenticeTexts10,
    sPartnerApprenticeTexts11,
    sPartnerApprenticeTexts12,
    sPartnerApprenticeTexts13,
    sPartnerApprenticeTexts14,
    sPartnerApprenticeTexts15,
    sPartnerApprenticeTexts16
};

struct
{
    u16 species;
    u8 fixedIV;
    u8 level;
    u8 nature;
    u8 evs[NUM_STATS];
    u16 moves[MAX_MON_MOVES];
} static const sStevenMons[MULTI_PARTY_SIZE] =
{
    {
        .species = SPECIES_METANG,
        .fixedIV = MAX_PER_STAT_IVS,
        .level = 42,
        .nature = NATURE_BRAVE,
        .evs = {0, 252, 252, 0, 6, 0},
        .moves = {MOVE_LIGHT_SCREEN, MOVE_PSYCHIC, MOVE_REFLECT, MOVE_METAL_CLAW}
    },
    {
        .species = SPECIES_SKARMORY,
        .fixedIV = MAX_PER_STAT_IVS,
        .level = 43,
        .nature = NATURE_IMPISH,
        .evs = {252, 0, 0, 0, 6, 252},
        .moves = {MOVE_TOXIC, MOVE_AERIAL_ACE, MOVE_PROTECT, MOVE_STEEL_WING}
    },
    {
        .species = SPECIES_AGGRON,
        .fixedIV = MAX_PER_STAT_IVS,
        .level = 44,
        .nature = NATURE_ADAMANT,
        .evs = {0, 252, 0, 0, 252, 6},
        .moves = {MOVE_THUNDER, MOVE_PROTECT, MOVE_SOLAR_BEAM, MOVE_DRAGON_CLAW}
    }
};

#include "data/battle_frontier/battle_tent.h"

static void (* const sBattleTowerFuncs[])(void) =
{
    [BATTLE_TOWER_FUNC_INIT]                = InitTowerChallenge,
    [BATTLE_TOWER_FUNC_GET_DATA]            = GetTowerData,
    [BATTLE_TOWER_FUNC_SET_DATA]            = SetTowerData,
    [BATTLE_TOWER_FUNC_SET_OPPONENT]        = SetNextFacilityOpponent,
    [BATTLE_TOWER_FUNC_SET_BATTLE_WON]      = SetTowerBattleWon,
    [BATTLE_TOWER_FUNC_GIVE_RIBBONS]        = AwardBattleTowerRibbons,
    [BATTLE_TOWER_FUNC_SAVE]                = SaveTowerChallenge,
    [BATTLE_TOWER_FUNC_GET_OPPONENT_INTRO]  = GetOpponentIntroSpeech,
    [BATTLE_TOWER_FUNC_NOP]                 = BattleTowerNop1,
    [BATTLE_TOWER_FUNC_NOP2]                = BattleTowerNop2,
    [BATTLE_TOWER_FUNC_LOAD_PARTNERS]       = LoadMultiPartnerCandidatesData,
    [BATTLE_TOWER_FUNC_PARTNER_MSG]         = ShowPartnerCandidateMessage,
    [BATTLE_TOWER_FUNC_LOAD_LINK_OPPONENTS] = LoadLinkMultiOpponentsData,
    [BATTLE_TOWER_FUNC_TRY_CLOSE_LINK]      = TowerTryCloseLink,
    [BATTLE_TOWER_FUNC_SET_PARTNER_GFX]     = SetMultiPartnerGfx,
    [BATTLE_TOWER_FUNC_SET_INTERVIEW_DATA]  = SetTowerInterviewData,
};

static const u32 sWinStreakFlags[][2] =
{
    {STREAK_TOWER_SINGLES_50,     STREAK_TOWER_SINGLES_OPEN},
    {STREAK_TOWER_DOUBLES_50,     STREAK_TOWER_DOUBLES_OPEN},
    {STREAK_TOWER_MULTIS_50,      STREAK_TOWER_MULTIS_OPEN},
    {STREAK_TOWER_LINK_MULTIS_50, STREAK_TOWER_LINK_MULTIS_OPEN},
};

static const u32 sWinStreakMasks[][2] =
{
    {~(STREAK_TOWER_SINGLES_50),     ~(STREAK_TOWER_SINGLES_OPEN)},
    {~(STREAK_TOWER_DOUBLES_50),     ~(STREAK_TOWER_DOUBLES_OPEN)},
    {~(STREAK_TOWER_MULTIS_50),      ~(STREAK_TOWER_MULTIS_OPEN)},
    {~(STREAK_TOWER_LINK_MULTIS_50), ~(STREAK_TOWER_LINK_MULTIS_OPEN)},
};

// The challenge number at which an Apprentice can appear, depending on how many of their questions were answered
static const u8 sApprenticeChallengeThreshold[MAX_APPRENTICE_QUESTIONS] =
{
    1, 2, 3, 4, 5, 8, 9, 10, 11, 12
};

// Unclear why this was duplicated
static const u8 sBattleTowerPartySizes2[] =
{
    [FRONTIER_MODE_SINGLES]     = FRONTIER_PARTY_SIZE,
    [FRONTIER_MODE_DOUBLES]     = FRONTIER_DOUBLES_PARTY_SIZE,
    [FRONTIER_MODE_MULTIS]      = FRONTIER_MULTI_PARTY_SIZE,
    [FRONTIER_MODE_LINK_MULTIS] = FRONTIER_MULTI_PARTY_SIZE,
};

// Trainer ID ranges for possible frontier trainers to encounter on particular challenges
// Trainers are scaled by difficulty, so higher trainer IDs have better teams
static const u16 sFrontierTrainerIdRanges[][2] =
{
    {FRONTIER_TRAINER_BRADY,   FRONTIER_TRAINER_JILL},   //   0 -  99
    {FRONTIER_TRAINER_TREVIN,  FRONTIER_TRAINER_CHLOE},  //  80 - 119
    {FRONTIER_TRAINER_ERIK,    FRONTIER_TRAINER_SOFIA},  // 100 - 139
    {FRONTIER_TRAINER_NORTON,  FRONTIER_TRAINER_JAZLYN}, // 120 - 159
    {FRONTIER_TRAINER_BRADEN,  FRONTIER_TRAINER_ALISON}, // 140 - 179
    {FRONTIER_TRAINER_ZACHERY, FRONTIER_TRAINER_LAMAR},  // 160 - 199
    {FRONTIER_TRAINER_HANK,    FRONTIER_TRAINER_TESS},   // 180 - 219
    {FRONTIER_TRAINER_JAXON,   FRONTIER_TRAINER_GRETEL}, // 200 - 299
};

static const u16 sFrontierTrainerIdRangesHard[][2] =
{
    {FRONTIER_TRAINER_ERIK,    FRONTIER_TRAINER_CHLOE},  // 100 - 119
    {FRONTIER_TRAINER_NORTON,  FRONTIER_TRAINER_SOFIA},  // 120 - 139
    {FRONTIER_TRAINER_BRADEN,  FRONTIER_TRAINER_JAZLYN}, // 140 - 159
    {FRONTIER_TRAINER_ZACHERY, FRONTIER_TRAINER_ALISON}, // 160 - 179
    {FRONTIER_TRAINER_HANK,    FRONTIER_TRAINER_LAMAR},  // 180 - 199
    {FRONTIER_TRAINER_JAXON,   FRONTIER_TRAINER_TESS},   // 200 - 219
    {FRONTIER_TRAINER_LEON,    FRONTIER_TRAINER_RAUL},   // 220 - 239
    {FRONTIER_TRAINER_JAXON,   FRONTIER_TRAINER_GRETEL}, // 200 - 299
};

// Unknown, unused data
static const u16 sUnused[] = { 179, 141, 200, 183 };

static const u8 sBattleTowerPartySizes[FRONTIER_MODE_COUNT] =
{
    [FRONTIER_MODE_SINGLES]     = FRONTIER_PARTY_SIZE,
    [FRONTIER_MODE_DOUBLES]     = FRONTIER_DOUBLES_PARTY_SIZE,
    [FRONTIER_MODE_MULTIS]      = FRONTIER_MULTI_PARTY_SIZE,
    [FRONTIER_MODE_LINK_MULTIS] = FRONTIER_MULTI_PARTY_SIZE,
};

static const u16 sRecordTrainerSpeechWon[] =
{
    EC_WORD_YAY, EC_WORD_YAY, EC_WORD_EXCL_EXCL, EC_WORD_I_VE, EC_WORD_WON, EC_WORD_EXCL_EXCL
};

static const u16 sRecordTrainerSpeechLost[] =
{
    EC_WORD_TOO, EC_WORD_BAD, EC_WORD_ELLIPSIS, EC_WORD_WE, EC_WORD_LOST, EC_WORD_ELLIPSIS
};

// code
void CallBattleTowerFunc(void)
{
    sBattleTowerFuncs[gSpecialVar_0x8004]();
}

static void InitTowerChallenge(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    gSaveBlock2Ptr->frontier.challengeStatus = CHALLENGE_STATUS_SAVING;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    gSaveBlock2Ptr->frontier.disableRecordBattle = FALSE;
    ResetFrontierTrainerIds();
    if (!(gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]))
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = 0;

    ValidateBattleTowerRecordChecksums();
    SetDynamicWarp(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE);
    gTrainerBattleOpponent_A = 0;
}

static void GetTowerData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case 0:
        break;
    case TOWER_DATA_WIN_STREAK:
        gSpecialVar_Result = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);
        break;
    case TOWER_DATA_WIN_STREAK_ACTIVE:
        gSpecialVar_Result = ((gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]) != 0);
        break;
    case TOWER_DATA_LVL_MODE:
        gSaveBlock2Ptr->frontier.towerLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        break;
    }
}

static void SetTowerData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case 0:
        break;
    case TOWER_DATA_WIN_STREAK:
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = gSpecialVar_0x8006;
        break;
    case TOWER_DATA_WIN_STREAK_ACTIVE:
        if (gSpecialVar_0x8006)
            gSaveBlock2Ptr->frontier.winStreakActiveFlags |= sWinStreakFlags[battleMode][lvlMode];
        else
            gSaveBlock2Ptr->frontier.winStreakActiveFlags &= sWinStreakMasks[battleMode][lvlMode];
        break;
    case TOWER_DATA_LVL_MODE:
        gSaveBlock2Ptr->frontier.towerLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        break;
    }
}

static void SetTowerBattleWon(void)
{
    if (gTrainerBattleOpponent_A == TRAINER_EREADER)
        ClearEReaderTrainer(&gSaveBlock2Ptr->frontier.ereaderTrainer);

    // towerNumWins is never read outside this conditional
    if (gSaveBlock2Ptr->frontier.towerNumWins < MAX_STREAK)
        gSaveBlock2Ptr->frontier.towerNumWins++;

    gSaveBlock2Ptr->frontier.curChallengeBattleNum++;
    SaveCurrentWinStreak();
    gSpecialVar_Result = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
}

static bool8 ChooseSpecialBattleTowerTrainer(void)
{
    s32 i, j, validMons;
    s32 trainerIds[9];
    s32 idsCount = 0;
    s32 winStreak = 0;
    u8 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    if (VarGet(VAR_FRONTIER_FACILITY) != FRONTIER_FACILITY_TOWER)
        return FALSE;

    winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[i]);
        u32 recordHasData = 0;
        u32 checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            recordHasData |= record[j];
            checksum += record[j];
        }
        validMons = 0;
        for (j = 0; j < MAX_FRONTIER_PARTY_SIZE; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].party[j].species != SPECIES_NONE
                && gSaveBlock2Ptr->frontier.towerRecords[i].party[j].level <= GetFrontierEnemyMonLevel(lvlMode))
                validMons++;
        }

        if (validMons >= sBattleTowerPartySizes2[battleMode]
            && gSaveBlock2Ptr->frontier.towerRecords[i].winStreak == winStreak
            && gSaveBlock2Ptr->frontier.towerRecords[i].lvlMode == lvlMode
            && recordHasData
            && gSaveBlock2Ptr->frontier.towerRecords[i].checksum == checksum)
        {
            trainerIds[idsCount] = i + TRAINER_RECORD_MIXING_FRIEND;
            idsCount++;
        }
    }

    if (battleMode == FRONTIER_MODE_SINGLES)
    {
        ValidateApprenticesChecksums();
        for (i = 0; i < APPRENTICE_COUNT; i++)
        {
            if (gSaveBlock2Ptr->apprentices[i].lvlMode != 0
                && sApprenticeChallengeThreshold[gSaveBlock2Ptr->apprentices[i].numQuestions] == winStreak
                && gSaveBlock2Ptr->apprentices[i].lvlMode - 1 == lvlMode)
            {
                trainerIds[idsCount] = i + TRAINER_RECORD_MIXING_APPRENTICE;
                idsCount++;
            }
        }
    }

    if (idsCount != 0)
    {
        gTrainerBattleOpponent_A = trainerIds[Random() % idsCount];
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void SetNextFacilityOpponent(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    if (lvlMode == FRONTIER_LVL_TENT)
    {
        SetNextBattleTentOpponent();
    }
    else
    {
        u16 id;
        u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
        u16 winStreak = GetCurrentFacilityWinStreak();
        u32 challengeNum = winStreak / FRONTIER_STAGES_PER_CHALLENGE;
        SetFacilityPtrsGetLevel();

        if (battleMode == FRONTIER_MODE_MULTIS || battleMode == FRONTIER_MODE_LINK_MULTIS)
        {
            id = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            gTrainerBattleOpponent_A = gSaveBlock2Ptr->frontier.trainerIds[id * 2];
            gTrainerBattleOpponent_B = gSaveBlock2Ptr->frontier.trainerIds[id * 2 + 1];
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_B, 1);
        }
        else if (ChooseSpecialBattleTowerTrainer())
        {
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
            gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = gTrainerBattleOpponent_A;
        }
        else
        {
            s32 i;
            while (1)
            {
                id = GetRandomScaledFrontierTrainerId(challengeNum, gSaveBlock2Ptr->frontier.curChallengeBattleNum);

                // Ensure trainer wasn't previously fought in this challenge.
                for (i = 0; i < gSaveBlock2Ptr->frontier.curChallengeBattleNum; i++)
                {
                    if (gSaveBlock2Ptr->frontier.trainerIds[i] == id)
                        break;
                }
                if (i == gSaveBlock2Ptr->frontier.curChallengeBattleNum)
                    break;
            }

            gTrainerBattleOpponent_A = id;
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
            if (gSaveBlock2Ptr->frontier.curChallengeBattleNum + 1 < FRONTIER_STAGES_PER_CHALLENGE)
                gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = gTrainerBattleOpponent_A;
        }
    }
}

u16 GetRandomScaledFrontierTrainerId(u8 challengeNum, u8 battleNum)
{
    u16 trainerId;

    if (challengeNum <= 7)
    {
        if (battleNum == FRONTIER_STAGES_PER_CHALLENGE - 1)
        {
            // The last battle in each challenge has a jump in difficulty, pulls from a table with higher ranges
            trainerId = (sFrontierTrainerIdRangesHard[challengeNum][1] - sFrontierTrainerIdRangesHard[challengeNum][0]) + 1;
            trainerId = sFrontierTrainerIdRangesHard[challengeNum][0] + (Random() % trainerId);
        }
        else
        {
            trainerId = (sFrontierTrainerIdRanges[challengeNum][1] - sFrontierTrainerIdRanges[challengeNum][0]) + 1;
            trainerId = sFrontierTrainerIdRanges[challengeNum][0] + (Random() % trainerId);
        }
    }
    else
    {
        // After challenge 7, trainer IDs always come from the last, hardest range, which is the same for both trainer ID tables
        trainerId = (sFrontierTrainerIdRanges[7][1] - sFrontierTrainerIdRanges[7][0]) + 1;
        trainerId = sFrontierTrainerIdRanges[7][0] + (Random() % trainerId);
    }

    return trainerId;
}

static void UNUSED GetRandomScaledFrontierTrainerIdRange(u8 challengeNum, u8 battleNum, u16 *trainerIdPtr, u8 *rangePtr)
{
    u16 trainerId, range;

    if (challengeNum <= 7)
    {
        if (battleNum == FRONTIER_STAGES_PER_CHALLENGE - 1)
        {
            // The last battle in each challenge has a jump in difficulty, pulls from a table with higher ranges
            range = (sFrontierTrainerIdRangesHard[challengeNum][1] - sFrontierTrainerIdRangesHard[challengeNum][0]) + 1;
            trainerId = sFrontierTrainerIdRangesHard[challengeNum][0];
        }
        else
        {
            range = (sFrontierTrainerIdRanges[challengeNum][1] - sFrontierTrainerIdRanges[challengeNum][0]) + 1;
            trainerId = sFrontierTrainerIdRanges[challengeNum][0];
        }
    }
    else
    {
        // After challenge 7, trainer IDs always come from the last, hardest range, which is the same for both trainer ID tables
        range = (sFrontierTrainerIdRanges[7][1] - sFrontierTrainerIdRanges[7][0]) + 1;
        trainerId = sFrontierTrainerIdRanges[7][0];
    }

    *trainerIdPtr = trainerId;
    *rangePtr = range;
}

void SetBattleFacilityTrainerGfxId(u16 trainerId, u8 tempVarId)
{
    u32 i;
    u8 facilityClass;
    u8 trainerObjectGfxId;

    SetFacilityPtrsGetLevel();
    if (trainerId == TRAINER_EREADER)
    {
        facilityClass = gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass;
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        SetFrontierBrainObjEventGfx_2();
        return;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        facilityClass = gFacilityTrainers[trainerId].facilityClass;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        facilityClass = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;
    }
    else
    {
        facilityClass = gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass;
    }

    // Search male classes.
    for (i = 0; i < ARRAY_COUNT(gTowerMaleFacilityClasses); i++)
    {
        if (gTowerMaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerMaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerMaleTrainerGfxIds[i];
        switch (tempVarId)
        {
        case 0:
        default:
            VarSet(VAR_OBJ_GFX_ID_0, trainerObjectGfxId);
            return;
        case 1:
            VarSet(VAR_OBJ_GFX_ID_1, trainerObjectGfxId);
            return;
        case 15:
            VarSet(VAR_OBJ_GFX_ID_E, trainerObjectGfxId);
            return;
        }
    }

    // Search female classes.
    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses); i++)
    {
        if (gTowerFemaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerFemaleTrainerGfxIds[i];
        switch (tempVarId)
        {
        case 0:
        default:
            VarSet(VAR_OBJ_GFX_ID_0, trainerObjectGfxId);
            return;
        case 1:
            VarSet(VAR_OBJ_GFX_ID_1, trainerObjectGfxId);
            return;
        case 15:
            VarSet(VAR_OBJ_GFX_ID_E, trainerObjectGfxId);
            return;
        }
    }

    switch (tempVarId)
    {
    case 0:
    default:
        VarSet(VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_BOY_1);
        return;
    case 1:
        VarSet(VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_BOY_1);
        return;
    case 15:
        VarSet(VAR_OBJ_GFX_ID_E, OBJ_EVENT_GFX_BOY_1);
        return;
    }
}

void SetEReaderTrainerGfxId(void)
{
    SetBattleFacilityTrainerGfxId(TRAINER_EREADER, 0);
}

u8 GetBattleFacilityTrainerGfxId(u16 trainerId)
{
    u32 i;
    u8 facilityClass;
    u8 trainerObjectGfxId;

    SetFacilityPtrsGetLevel();
    if (trainerId == TRAINER_EREADER)
    {
        facilityClass = gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        facilityClass = gFacilityTrainers[trainerId].facilityClass;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        facilityClass = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;
    }
    else
    {
        facilityClass = gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass;
    }

    // Search male classes.
    for (i = 0; i < ARRAY_COUNT(gTowerMaleFacilityClasses); i++)
    {
        if (gTowerMaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerMaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerMaleTrainerGfxIds[i];
        return trainerObjectGfxId;
    }

    // Search female classes.
    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses); i++)
    {
        if (gTowerFemaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerFemaleTrainerGfxIds[i];
        return trainerObjectGfxId;
    }
    else
    {
        return OBJ_EVENT_GFX_BOY_1;
    }
}

void PutNewBattleTowerRecord(struct EmeraldBattleTowerRecord *newRecordEm)
{
    u16 slotValues[6];
    u16 slotIds[6];
    s32 i, j, k;
    s32 slotsCount = 0;
    struct EmeraldBattleTowerRecord *newRecord = newRecordEm; // Needed to match.

    // Find a record slot of the same player and replace it.
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        k = 0;
        for (j = 0; j < TRAINER_ID_LENGTH; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].trainerId[j] != newRecord->trainerId[j])
                break;
        }
        if (j == TRAINER_ID_LENGTH)
        {
            for (k = 0; k < PLAYER_NAME_LENGTH; k++)
            {
            // Incorrect index being used
            #ifdef BUGFIX
                #define INDEX k
            #else
                #define INDEX j
            #endif
                if (gSaveBlock2Ptr->frontier.towerRecords[i].name[INDEX] != newRecord->name[INDEX])
                    break;
                if (newRecord->name[INDEX] == EOS)
                {
                    k = PLAYER_NAME_LENGTH;
                    break;
                }
            }
        }

        if (k == PLAYER_NAME_LENGTH)
            break;
    }
    if (i < BATTLE_TOWER_RECORD_COUNT)
    {
        gSaveBlock2Ptr->frontier.towerRecords[i] = *newRecord;
        return;
    }

    // Find an empty record slot.
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak == 0)
            break;
    }
    if (i < BATTLE_TOWER_RECORD_COUNT)
    {
        gSaveBlock2Ptr->frontier.towerRecords[i] = *newRecord;
        return;
    }

    // Find possible slots to replace the record.
    slotValues[0] = gSaveBlock2Ptr->frontier.towerRecords[0].winStreak;
    slotIds[0] = 0;
    slotsCount++;

    for (i = 1; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        for (j = 0; j < slotsCount; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak < slotValues[j])
            {
                j = 0;
                slotsCount = 1;
                slotValues[0] = gSaveBlock2Ptr->frontier.towerRecords[i].winStreak;
                slotIds[0] = i;
                break;
            }
            else if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak > slotValues[j])
            {
                break;
            }
        }

        if (j == slotsCount)
        {
            slotValues[slotsCount] = gSaveBlock2Ptr->frontier.towerRecords[i].winStreak;
            slotIds[slotsCount] = i;
            slotsCount++;
        }
    }

    i = Random() % slotsCount;
    gSaveBlock2Ptr->frontier.towerRecords[slotIds[i]] = *newRecord;
}

u8 GetFrontierTrainerFrontSpriteId(u16 trainerId)
{
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_EREADER)
    {
        return gFacilityClassToPicIndex[gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass];
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        return GetFrontierBrainTrainerPicIndex();
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        return gFacilityClassToPicIndex[gFacilityTrainers[trainerId].facilityClass];
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            return gFacilityClassToPicIndex[GetRecordedBattleRecordMixFriendClass()];
        else
            return gFacilityClassToPicIndex[gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass];
    }
    else
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            return gFacilityClassToPicIndex[gApprentices[GetRecordedBattleApprenticeId()].facilityClass];
        else
            return gFacilityClassToPicIndex[gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass];
    }
}

u8 GetFrontierOpponentClass(u16 trainerId)
{
    u8 trainerClass = 0;
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_EREADER)
    {
        trainerClass = gFacilityClassToTrainerClass[gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass];
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        return GetFrontierBrainTrainerClass();
    }
    else if (trainerId == TRAINER_STEVEN_PARTNER)
    {
        trainerClass = gTrainers[TRAINER_STEVEN].trainerClass;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        trainerClass = gFacilityClassToTrainerClass[gFacilityTrainers[trainerId].facilityClass];
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            trainerClass = gFacilityClassToTrainerClass[GetRecordedBattleRecordMixFriendClass()];
        }
        else
        {
            trainerClass = gFacilityClassToTrainerClass[gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass];
        }
    }
    else
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            trainerClass = gFacilityClassToTrainerClass[gApprentices[GetRecordedBattleApprenticeId()].facilityClass];
        }
        else
        {
            trainerClass = gFacilityClassToTrainerClass[gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass];
        }
    }

    return trainerClass;
}

static u8 GetFrontierTrainerFacilityClass(u16 trainerId)
{
    u8 facilityClass;
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_EREADER)
    {
        facilityClass = gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        facilityClass = gFacilityTrainers[trainerId].facilityClass;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            facilityClass = GetRecordedBattleRecordMixFriendClass();
        else
            facilityClass = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;
    }
    else
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            facilityClass = gApprentices[GetRecordedBattleApprenticeId()].facilityClass;
        else
            facilityClass = gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass;
    }

    return facilityClass;
}

void GetFrontierTrainerName(u8 *dst, u16 trainerId)
{
    s32 i = 0;
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_EREADER)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH; i++)
            dst[i] = gSaveBlock2Ptr->frontier.ereaderTrainer.name[i];
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CopyFrontierBrainTrainerName(dst);
        return;
    }
    else if (trainerId == TRAINER_STEVEN_PARTNER)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH; i++)
            dst[i] = gTrainers[TRAINER_STEVEN].trainerName[i];
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH; i++)
            dst[i] = gFacilityTrainers[trainerId].trainerName[i];
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            GetRecordedBattleRecordMixFriendName(dst);
            return;
        }
        else
        {
            struct EmeraldBattleTowerRecord *record = &gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND];
            TVShowConvertInternationalString(dst, record->name, record->language);
            return;
        }
    }
    else
    {
        u8 id, language;

        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            id = GetRecordedBattleApprenticeId();
            language = GetRecordedBattleApprenticeLanguage();
        }
        else
        {
            struct Apprentice *apprentice = &gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE];
            id = apprentice->id;
            language = apprentice->language;
        }
        TVShowConvertInternationalString(dst, GetApprenticeNameInLanguage(id, language), language);
        return;
    }

    dst[i] = EOS;
}

static bool8 IsFrontierTrainerFemale(u16 trainerId)
{
    u32 i;
    u8 facilityClass;

    SetFacilityPtrsGetLevel();
    if (trainerId == TRAINER_EREADER)
    {
        facilityClass = gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass;
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        return IsFrontierBrainFemale();
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        facilityClass = gFacilityTrainers[trainerId].facilityClass;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        facilityClass = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;
    }
    else
    {
        facilityClass = gApprentices[gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id].facilityClass;
    }

    // Search female classes.
    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses); i++)
    {
        if (gTowerFemaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
        return TRUE;
    else
        return FALSE;
}

void FillFrontierTrainerParty(u8 monsCount)
{
    ZeroEnemyPartyMons();
    FillTrainerParty(gTrainerBattleOpponent_A, 0, monsCount);
}

void FillFrontierTrainersParties(u8 monsCount)
{
    ZeroEnemyPartyMons();
    FillTrainerParty(gTrainerBattleOpponent_A, 0, monsCount);
    FillTrainerParty(gTrainerBattleOpponent_B, 3, monsCount);
}

static void FillTentTrainerParty(u8 monsCount)
{
    ZeroEnemyPartyMons();
    FillTentTrainerParty_(gTrainerBattleOpponent_A, 0, monsCount);
}

static void FillTrainerParty(u16 trainerId, u8 firstMonId, u8 monCount)
{
    s32 i, j;
    u16 chosenMonIndices[MAX_FRONTIER_PARTY_SIZE];
    u8 friendship = MAX_FRIENDSHIP;
    u8 level = SetFacilityPtrsGetLevel();
    u8 fixedIV = 0;
    u8 bfMonCount;
    const u16 *monSet = NULL;
    u32 otID = 0;

    if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        // Normal battle frontier trainer.
        fixedIV = GetFrontierTrainerFixedIvs(trainerId);
        monSet = gFacilityTrainers[gTrainerBattleOpponent_A].monSet;
    }
    else if (trainerId == TRAINER_EREADER)
    {
        for (i = firstMonId; i < firstMonId + FRONTIER_PARTY_SIZE; i++)
            CreateBattleTowerMon(&gEnemyParty[i], &gSaveBlock2Ptr->frontier.ereaderTrainer.party[i - firstMonId]);
        return;
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CreateFrontierBrainPokemon();
        return;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        // Record mixed player.
        for (j = 0, i = firstMonId; i < firstMonId + monCount; j++, i++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[j].species != SPECIES_NONE
                && gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[j].level <= level)
            {
                CreateBattleTowerMon_HandleLevel(&gEnemyParty[i], &gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[j], FALSE);
            }
        }
        return;
    }
    else
    {
        // Apprentice.
        for (i = firstMonId; i < firstMonId + FRONTIER_PARTY_SIZE; i++)
            CreateApprenticeMon(&gEnemyParty[i], &gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE], i - firstMonId);
        return;
    }

    // Regular battle frontier trainer.
    // Attempt to fill the trainer's party with random Pokémon until 3 have been
    // successfully chosen. The trainer's party may not have duplicate Pokémon species
    // or duplicate held items.
    for (bfMonCount = 0; monSet[bfMonCount] != 0xFFFF; bfMonCount++)
        ;
    i = 0;
    otID = Random32();
    while (i != monCount)
    {
        u16 monId = monSet[Random() % bfMonCount];

        // "High tier" Pokémon are only allowed on open level mode
        // 20 is not a possible value for level here
        if ((level == FRONTIER_MAX_LEVEL_50 || level == 20) && monId > FRONTIER_MONS_HIGH_TIER)
            continue;

        // Ensure this Pokémon species isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gEnemyParty[j], MON_DATA_SPECIES, NULL) == gFacilityTrainerMons[monId].species)
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this Pokemon's held item isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gEnemyParty[j], MON_DATA_HELD_ITEM, NULL) != ITEM_NONE
             && GetMonData(&gEnemyParty[j], MON_DATA_HELD_ITEM, NULL) == gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId])
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this exact Pokémon index isn't a duplicate. This check doesn't seem necessary
        // because the species and held items were already checked directly above.
        for (j = 0; j < i; j++)
        {
            if (chosenMonIndices[j] == monId)
                break;
        }
        if (j != i)
            continue;

        chosenMonIndices[i] = monId;

        // Place the chosen Pokémon into the trainer's party.
        CreateMonWithEVSpreadNatureOTID(&gEnemyParty[i + firstMonId],
                                             gFacilityTrainerMons[monId].species,
                                             level,
                                             gFacilityTrainerMons[monId].nature,
                                             fixedIV,
                                             gFacilityTrainerMons[monId].evSpread,
                                             otID);

        friendship = MAX_FRIENDSHIP;
        // Give the chosen Pokémon its specified moves.
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            SetMonMoveSlot(&gEnemyParty[i + firstMonId], gFacilityTrainerMons[monId].moves[j], j);
            if (gFacilityTrainerMons[monId].moves[j] == MOVE_FRUSTRATION)
                friendship = 0;  // Frustration is more powerful the lower the pokemon's friendship is.
        }

        SetMonData(&gEnemyParty[i + firstMonId], MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&gEnemyParty[i + firstMonId], MON_DATA_HELD_ITEM, &gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId]);

        // The Pokémon was successfully added to the trainer's party, so it's safe to move on to
        // the next party slot.
        i++;
    }
}

// Probably an early draft before the 'CreateApprenticeMon' was written.
static void UNUSED Unused_CreateApprenticeMons(u16 trainerId, u8 firstMonId)
{
    s32 i, j;
    u8 friendship = MAX_FRIENDSHIP;
    u8 level = 0;
    u8 fixedIV = 0;
    struct Apprentice *apprentice = &gSaveBlock2Ptr->apprentices[0];

    if (apprentice->numQuestions < 5)
        fixedIV = 6;
    else
        fixedIV = 9;

    if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_50)
        level = FRONTIER_MAX_LEVEL_OPEN;
    else
        level = FRONTIER_MAX_LEVEL_50;

    for (i = 0; i != FRONTIER_PARTY_SIZE; i++)
    {
        CreateMonWithEVSpread(&gEnemyParty[firstMonId + i], apprentice->party[i].species, level, fixedIV, 8);
        friendship = MAX_FRIENDSHIP;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            if (apprentice->party[i].moves[j] == MOVE_FRUSTRATION)
                friendship = 0;
        }
        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_HELD_ITEM, &apprentice->party[i].item);
    }
}

u16 GetRandomFrontierMonFromSet(u16 trainerId)
{
    u8 level = SetFacilityPtrsGetLevel();
    const u16 *monSet = gFacilityTrainers[trainerId].monSet;
    u8 numMons = 0;
    u32 monId = monSet[numMons];

    while (monId != 0xFFFF)
    {
        numMons++;
        monId = monSet[numMons];
        if (monId == 0xFFFF)
            break;
    }

    do
    {
        // "High tier" Pokémon are only allowed on open level mode
        // 20 is not a possible value for level here
        monId = monSet[Random() % numMons];
    } while((level == FRONTIER_MAX_LEVEL_50 || level == 20) && monId > FRONTIER_MONS_HIGH_TIER);

    return monId;
}

static void FillFactoryTrainerParty(void)
{
    ZeroEnemyPartyMons();
    if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_TENT)
        FillFactoryFrontierTrainerParty(gTrainerBattleOpponent_A, 0);
    else
        FillFactoryTentTrainerParty(gTrainerBattleOpponent_A, 0);
}

static void FillFactoryFrontierTrainerParty(u16 trainerId, u8 firstMonId)
{
    u8 i, j;
    u8 friendship;
    u8 level;
    u8 fixedIV;
    u32 otID;

    if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
    // By mistake Battle Tower's Level 50 challenge number is used to determine the IVs for Battle Factory.
    #ifdef BUGFIX
        u8 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
        u8 challengeNum = gSaveBlock2Ptr->frontier.factoryWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
    #else
        u8 UNUSED lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
        u8 challengeNum = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][FRONTIER_LVL_50] / FRONTIER_STAGES_PER_CHALLENGE;
    #endif
        if (gSaveBlock2Ptr->frontier.curChallengeBattleNum < FRONTIER_STAGES_PER_CHALLENGE - 1)
            fixedIV = GetFactoryMonFixedIV(challengeNum, FALSE);
        else
            fixedIV = GetFactoryMonFixedIV(challengeNum, TRUE); // Last trainer in challenge uses higher IVs
    }
    else if (trainerId == TRAINER_EREADER)
    {
        for (i = firstMonId; i < firstMonId + FRONTIER_PARTY_SIZE; i++)
            CreateBattleTowerMon(&gEnemyParty[i], &gSaveBlock2Ptr->frontier.ereaderTrainer.party[i - firstMonId]);
        return;
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        FillFactoryBrainParty();
        return;
    }
    else
    {
        fixedIV = MAX_PER_STAT_IVS;
    }

    level = SetFacilityPtrsGetLevel();
    otID = T1_READ_32(gSaveBlock2Ptr->playerTrainerId);
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        u16 monId = gFrontierTempParty[i];
        CreateMonWithEVSpreadNatureOTID(&gEnemyParty[firstMonId + i],
                                             gFacilityTrainerMons[monId].species,
                                             level,
                                             gFacilityTrainerMons[monId].nature,
                                             fixedIV,
                                             gFacilityTrainerMons[monId].evSpread,
                                             otID);

        friendship = 0;
        for (j = 0; j < MAX_MON_MOVES; j++)
            SetMonMoveAvoidReturn(&gEnemyParty[firstMonId + i], gFacilityTrainerMons[monId].moves[j], j);

        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_HELD_ITEM, &gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId]);
    }
}

static void FillFactoryTentTrainerParty(u16 trainerId, u8 firstMonId)
{
    u8 i, j;
    u8 friendship;
    u8 level = TENT_MIN_LEVEL;
    u8 fixedIV = 0;
    u32 otID = T1_READ_32(gSaveBlock2Ptr->playerTrainerId);

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        u16 monId = gFrontierTempParty[i];
        CreateMonWithEVSpreadNatureOTID(&gEnemyParty[firstMonId + i],
                                             gFacilityTrainerMons[monId].species,
                                             level,
                                             gFacilityTrainerMons[monId].nature,
                                             fixedIV,
                                             gFacilityTrainerMons[monId].evSpread,
                                             otID);

        friendship = 0;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            SetMonMoveAvoidReturn(&gEnemyParty[firstMonId + i], gFacilityTrainerMons[monId].moves[j], j);
            if (gFacilityTrainerMons[monId].moves[j] == MOVE_FRUSTRATION)
                friendship = 0;
        }

        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&gEnemyParty[firstMonId + i], MON_DATA_HELD_ITEM, &gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId]);
    }
}

void FrontierSpeechToString(const u16 *words)
{
    ConvertEasyChatWordsToString(gStringVarBuffer, words, 3, 2);
    if (GetStringWidth(FONT_NORMAL, gStringVarBuffer, -1) > 204u)
    {
        s32 i = 0;

        ConvertEasyChatWordsToString(gStringVarBuffer, words, 2, 3);
        while (gStringVarBuffer[i++] != CHAR_NEWLINE)
            ;
        while (gStringVarBuffer[i] != CHAR_NEWLINE)
            i++;

        gStringVarBuffer[i] = CHAR_PROMPT_SCROLL;
    }
}

static void GetOpponentIntroSpeech(void)
{
    u16 trainerId;
    SetFacilityPtrsGetLevel();

    if (gSpecialVar_0x8005)
        trainerId = gTrainerBattleOpponent_B;
    else
        trainerId = gTrainerBattleOpponent_A;

    if (trainerId == TRAINER_EREADER)
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.greeting);
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
        FrontierSpeechToString(gFacilityTrainers[trainerId].speechBefore);
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].greeting);
    else
        BufferApprenticeChallengeText(trainerId - TRAINER_RECORD_MIXING_APPRENTICE);
}

static void HandleSpecialTrainerBattleEnd(void)
{
    s32 i;

    RecordedBattle_SaveBattleOutcome();
    switch (gBattleScripting.specialTrainerBattleType)
    {
    case SPECIAL_BATTLE_TOWER:
    case SPECIAL_BATTLE_DOME:
    case SPECIAL_BATTLE_PALACE:
    case SPECIAL_BATTLE_ARENA:
    case SPECIAL_BATTLE_FACTORY:
    case SPECIAL_BATTLE_PIKE_SINGLE:
    case SPECIAL_BATTLE_PIKE_DOUBLE:
    case SPECIAL_BATTLE_PYRAMID:
        if (gSaveBlock2Ptr->frontier.battlesCount < 0xFFFFFF)
        {
            gSaveBlock2Ptr->frontier.battlesCount++;
            if (gSaveBlock2Ptr->frontier.battlesCount % 20 == 0)
                UpdateGymLeaderRematch();
        }
        else
        {
            gSaveBlock2Ptr->frontier.battlesCount = 0xFFFFFF;
        }
        break;
    case SPECIAL_BATTLE_SECRET_BASE:
        for (i = 0; i < PARTY_SIZE; i++)
        {
            u16 itemBefore = GetMonData(&gSaveBlock1Ptr->playerParty[i], MON_DATA_HELD_ITEM);
            SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &itemBefore);
        }
        break;
    case SPECIAL_BATTLE_EREADER:
        CopyEReaderTrainerFarewellMessage();
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static void Task_StartBattleAfterTransition(u8 taskId)
{
    if (IsBattleTransitionDone() == TRUE)
    {
        gMain.savedCallback = HandleSpecialTrainerBattleEnd;
        SetMainCallback2(CB2_InitBattle);
        DestroyTask(taskId);
    }
}

void DoSpecialTrainerBattle(void)
{
    s32 i;

    gBattleScripting.specialTrainerBattleType = gSpecialVar_0x8004;
    switch (gSpecialVar_0x8004)
    {
    case SPECIAL_BATTLE_TOWER:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        switch (VarGet(VAR_FRONTIER_BATTLE_MODE))
        {
        case FRONTIER_MODE_SINGLES:
            FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
            break;
        case FRONTIER_MODE_DOUBLES:
            FillFrontierTrainerParty(FRONTIER_DOUBLES_PARTY_SIZE);
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
            break;
        case FRONTIER_MODE_MULTIS:
            FillFrontierTrainersParties(FRONTIER_MULTI_PARTY_SIZE);
            gPartnerTrainerId = gSaveBlock2Ptr->frontier.trainerIds[17];
            FillPartnerParty(gPartnerTrainerId);
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS;
            break;
        case FRONTIER_MODE_LINK_MULTIS:
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI;
            FillFrontierTrainersParties(FRONTIER_MULTI_PARTY_SIZE);
            break;
        }
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_TOWER));
        break;
    case SPECIAL_BATTLE_SECRET_BASE:
        for (i = 0; i < PARTY_SIZE; i++)
        {
            u16 itemBefore = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);
            SetMonData(&gSaveBlock1Ptr->playerParty[i], MON_DATA_HELD_ITEM, &itemBefore);
        }
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_SECRET_BASE));
        break;
    case SPECIAL_BATTLE_EREADER:
        ZeroEnemyPartyMons();
        for (i = 0; i < (int)ARRAY_COUNT(gSaveBlock2Ptr->frontier.ereaderTrainer.party); i++)
            CreateBattleTowerMon(&gEnemyParty[i], &gSaveBlock2Ptr->frontier.ereaderTrainer.party[i]);
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_EREADER_TRAINER;
        gTrainerBattleOpponent_A = 0;
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_E_READER));
        break;
    case SPECIAL_BATTLE_DOME:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOME;
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        if (gTrainerBattleOpponent_A == TRAINER_FRONTIER_BRAIN)
            FillFrontierTrainerParty(DOME_BATTLE_PARTY_SIZE);
        CreateTask(Task_StartBattleAfterTransition, 1);
        CreateTask_PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_DOME));
        break;
    case SPECIAL_BATTLE_PALACE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_PALACE;
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_TENT)
            FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        else
            FillTentTrainerParty(FRONTIER_PARTY_SIZE);
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PALACE));
        break;
    case SPECIAL_BATTLE_ARENA:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_ARENA;
        if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_TENT)
            FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        else
            FillTentTrainerParty(FRONTIER_PARTY_SIZE);
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_ARENA));
        break;
    case SPECIAL_BATTLE_FACTORY:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FACTORY;
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        FillFactoryTrainerParty();
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_FACTORY));
        break;
    case SPECIAL_BATTLE_PIKE_SINGLE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PIKE));
        break;
    case SPECIAL_BATTLE_PYRAMID:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_PYRAMID;
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PYRAMID));
        break;
    case SPECIAL_BATTLE_PIKE_DOUBLE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS;
        FillFrontierTrainersParties(1);
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PIKE));
        break;
    case SPECIAL_BATTLE_STEVEN:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
        FillPartnerParty(TRAINER_STEVEN_PARTNER);
        gApproachingTrainerId = 0;
        BattleSetup_ConfigureTrainerBattle(MossdeepCity_SpaceCenter_2F_EventScript_MaxieTrainer + 1);
        gApproachingTrainerId = 1;
        BattleSetup_ConfigureTrainerBattle(MossdeepCity_SpaceCenter_2F_EventScript_TabithaTrainer + 1);
        gPartnerTrainerId = TRAINER_STEVEN_PARTNER;
        CreateTask(Task_StartBattleAfterTransition, 1);
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(B_TRANSITION_MAGMA);
        break;
    }
}

static void SaveCurrentWinStreak(void)
{
    u8 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u16 winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);

    if (gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] < winStreak)
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = winStreak;
}

static void SaveBattleTowerRecord(void)
{
    s32 i;
    u8 lvlMode, battleMode, class;
    struct EmeraldBattleTowerRecord *playerRecord = &gSaveBlock2Ptr->frontier.towerPlayer;

    ClearBattleTowerRecord(playerRecord);
    lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (gSaveBlock2Ptr->playerGender != MALE)
    {
        class = gTowerFemaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] +
                                       gSaveBlock2Ptr->playerTrainerId[1] +
                                       gSaveBlock2Ptr->playerTrainerId[2] +
                                       gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerFemaleFacilityClasses)];
    }
    else
    {
        class = gTowerMaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] +
                                     gSaveBlock2Ptr->playerTrainerId[1] +
                                     gSaveBlock2Ptr->playerTrainerId[2] +
                                     gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerMaleFacilityClasses)];
    }
    playerRecord->lvlMode = lvlMode;
    playerRecord->facilityClass = class;
    CopyTrainerId(playerRecord->trainerId, gSaveBlock2Ptr->playerTrainerId);
    StringCopy_PlayerName(playerRecord->name, gSaveBlock2Ptr->playerName);
    playerRecord->winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
    {
        playerRecord->greeting[i] = gSaveBlock1Ptr->easyChatBattleStart[i];
        playerRecord->speechWon[i] = gSaveBlock1Ptr->easyChatBattleWon[i];
        playerRecord->speechLost[i] = gSaveBlock1Ptr->easyChatBattleLost[i];
    }

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        if (gSaveBlock2Ptr->frontier.selectedPartyMons[i] != 0)
            ConvertPokemonToBattleTowerPokemon(&gPlayerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], &playerRecord->party[i]);
    }

    playerRecord->language = gGameLanguage;
    CalcEmeraldBattleTowerChecksum(&gSaveBlock2Ptr->frontier.towerPlayer);
    SaveCurrentWinStreak();
}

static void SaveTowerChallenge(void)
{
    u16 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u16 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 challengeNum = (signed)(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE);

    if (gSpecialVar_0x8005 == 0 && (challengeNum > 1 || gSaveBlock2Ptr->frontier.curChallengeBattleNum != 0))
        SaveBattleTowerRecord();

    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveGameFrontier();
}

static void BattleTowerNop1(void)
{

}

static void BattleTowerNop2(void)
{

}

static void GetApprenticeMultiPartnerParty(u16 trainerId)
{
    s32 i, count;
    u32 validSpecies[MULTI_PARTY_SIZE];
    u16 species1 = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES, NULL);
    u16 species2 = GetMonData(&gPlayerParty[1], MON_DATA_SPECIES, NULL);

    count = 0;
    for (i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        u16 apprenticeSpecies = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].party[i].species;
        if (apprenticeSpecies != species1 && apprenticeSpecies != species2)
        {
            validSpecies[count] = i;
            count++;
        }
    }

    gFrontierTempParty[0] = validSpecies[Random() % count];
    do
    {
        gFrontierTempParty[1] = validSpecies[Random() % count];
    } while (gFrontierTempParty[0] == gFrontierTempParty[1]);
}

static void GetRecordMixFriendMultiPartnerParty(u16 trainerId)
{
    s32 i, count;
    u32 validSpecies[3];
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u16 species1 = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES, NULL);
    u16 species2 = GetMonData(&gPlayerParty[1], MON_DATA_SPECIES, NULL);

    count = 0;
    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        if (gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[i].species != species1
            && gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[i].species != species2
            && gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[i].level <= GetFrontierEnemyMonLevel(lvlMode)
            && gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[i].species != SPECIES_NONE)
        {
            validSpecies[count] = i;
            count++;
        }
    }

    gFrontierTempParty[2] = validSpecies[Random() % count];
    do
    {
        gFrontierTempParty[3] = validSpecies[Random() % count];
    } while (gFrontierTempParty[2] == gFrontierTempParty[3]);
}

static void LoadMultiPartnerCandidatesData(void)
{
    s32 i, j, k;
    u32 spArray[5];
    s32 r10;
    u16 trainerId;
    u16 monId;
    u32 lvlMode, battleMode;
    s32 challengeNum;
    u32 species1, species2;
    u32 UNUSED level;
    struct ObjectEventTemplate *objEventTemplates;

    objEventTemplates = gSaveBlock1Ptr->objectEventTemplates;
    lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    challengeNum = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
    species1 = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES, NULL);
    species2 = GetMonData(&gPlayerParty[1], MON_DATA_SPECIES, NULL);
    level = SetFacilityPtrsGetLevel();

    j = 0;
    do
    {
        do
        {
            trainerId = GetRandomScaledFrontierTrainerId(challengeNum, 0);
            for (i = 0; i < j; i++)
            {
                if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
                    break;
                if (gFacilityTrainers[gSaveBlock2Ptr->frontier.trainerIds[i]].facilityClass == gFacilityTrainers[trainerId].facilityClass)
                    break;
            }
        } while (i != j);
        gSaveBlock2Ptr->frontier.trainerIds[j] = trainerId;
        j++;
    } while (j < 6);

    r10 = 8;
    for (i = 0; i < 6; i++)
    {
        trainerId = gSaveBlock2Ptr->frontier.trainerIds[i];
        objEventTemplates[i + 1].graphicsId = GetBattleFacilityTrainerGfxId(trainerId);
        for (j = 0; j < 2; j++)
        {
            while (1)
            {
                monId = GetRandomFrontierMonFromSet(trainerId);
                if (j % 2 != 0 && gFacilityTrainerMons[gSaveBlock2Ptr->frontier.trainerIds[r10 - 1]].itemTableId == gFacilityTrainerMons[monId].itemTableId)
                    continue;

                for (k = 8; k < r10; k++)
                {
                    if (gFacilityTrainerMons[gSaveBlock2Ptr->frontier.trainerIds[k]].species == gFacilityTrainerMons[monId].species)
                        break;
                    if (species1 == gFacilityTrainerMons[monId].species)
                        break;
                    if (species2 == gFacilityTrainerMons[monId].species)
                        break;
                }
                if (k == r10)
                    break;
            }

            gSaveBlock2Ptr->frontier.trainerIds[r10] = monId;
            r10++;
        }
    }

    r10 = 0;
    ValidateApprenticesChecksums();
    for (i = 0; i < APPRENTICE_COUNT; i++)
    {
        if (gSaveBlock2Ptr->apprentices[i].lvlMode != 0
            && sApprenticeChallengeThreshold[gSaveBlock2Ptr->apprentices[i].numQuestions] / FRONTIER_STAGES_PER_CHALLENGE <= challengeNum
            && gSaveBlock2Ptr->apprentices[i].lvlMode - 1 == lvlMode)
        {
            k = 0;
            for (j = 0; j < MULTI_PARTY_SIZE; j++)
            {
                if (species1 != gSaveBlock2Ptr->apprentices[i].party[j].species
                    && species2 != gSaveBlock2Ptr->apprentices[i].party[j].species)
                {
                    k++;
                }
            }
            if (k > 2)
            {
                spArray[r10] = i + TRAINER_RECORD_MIXING_APPRENTICE;
                r10++;
            }
        }
    }
    if (r10 != 0)
    {
        gSaveBlock2Ptr->frontier.trainerIds[6] = spArray[Random() % r10];
        objEventTemplates[7].graphicsId = GetBattleFacilityTrainerGfxId(gSaveBlock2Ptr->frontier.trainerIds[6]);
        FlagClear(FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_1);
        GetApprenticeMultiPartnerParty(gSaveBlock2Ptr->frontier.trainerIds[6]);
    }

    r10 = 0;
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[i]);
        u32 recordHasData = 0;
        u32 checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            recordHasData |= record[j];
            checksum += record[j];
        }

        if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak / FRONTIER_STAGES_PER_CHALLENGE <= challengeNum
            && gSaveBlock2Ptr->frontier.towerRecords[i].lvlMode == lvlMode
            && recordHasData
            && gSaveBlock2Ptr->frontier.towerRecords[i].checksum == checksum)
        {
            k = 0;
            for (j = 0; j < MAX_FRONTIER_PARTY_SIZE; j++)
            {
                if (species1 != gSaveBlock2Ptr->frontier.towerRecords[i].party[j].species
                    && species2 != gSaveBlock2Ptr->frontier.towerRecords[i].party[j].species
                    && gSaveBlock2Ptr->frontier.towerRecords[i].party[j].level <= GetFrontierEnemyMonLevel(lvlMode)
                    && gSaveBlock2Ptr->frontier.towerRecords[i].party[j].species != SPECIES_NONE)
                {
                    k++;
                }
            }
            if (k > 1)
            {
                spArray[r10] = i + TRAINER_RECORD_MIXING_FRIEND;
                r10++;
            }
        }
    }
    if (r10 != 0)
    {
        gSaveBlock2Ptr->frontier.trainerIds[7] = spArray[Random() % r10];
        objEventTemplates[8].graphicsId = GetBattleFacilityTrainerGfxId(gSaveBlock2Ptr->frontier.trainerIds[7]);
        FlagClear(FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_2);
        GetRecordMixFriendMultiPartnerParty(gSaveBlock2Ptr->frontier.trainerIds[7]);
    }
}

static void GetPotentialPartnerMoveAndSpecies(u16 trainerId, u16 monId)
{
    u16 move = 0;
    u16 species = 0;
    SetFacilityPtrsGetLevel();

    if (trainerId != TRAINER_EREADER)
    {
        if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            move = gFacilityTrainerMons[monId].moves[0];
            species = gFacilityTrainerMons[monId].species;
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            move = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[gFrontierTempParty[gSpecialVar_0x8005 + 1]].moves[0];
            species = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].party[gFrontierTempParty[gSpecialVar_0x8005 + 1]].species;
        }
        else
        {
            s32 i;

            move = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].party[gFrontierTempParty[gSpecialVar_0x8005 - 1]].moves[0];
            species = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].party[gFrontierTempParty[gSpecialVar_0x8005 - 1]].species;
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                gStringVar3[i] = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].playerName[i];
            gStringVar3[i] = EOS;
            ConvertInternationalString(gStringVar3, gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].language);
        }
    }

    StringCopy(gStringVar1, gMoveNames[move]);
    StringCopy(gStringVar2, gSpeciesNames[species]);
}

// For multi battles in the Battle Tower, the player may choose a partner by talking to them
// These partners can be an NPC or a former/record-mixed Apprentice
// When talked to, their response consists of:
// PARTNER_MSGID_INTRO - A greeting
// PARTNER_MSGID_MON1 - Naming one Pokémon on their team, and a move it has
// PARTNER_MSGID_MON2_ASK - Naming a second Pokémon on their team, a move it has, and asking if they'd like to be their partner
// PARTNER_MSGID_ACCEPT - If the player agrees to be their partner
// PARTNER_MSGID_REJECT - If the player declines to be their partner
static void ShowPartnerCandidateMessage(void)
{
    s32 i, j, partnerId;
    s32 monId;
    s32 UNUSED level = SetFacilityPtrsGetLevel();
    u16 winStreak = GetCurrentFacilityWinStreak();
    s32 challengeNum = winStreak / FRONTIER_STAGES_PER_CHALLENGE;
    s32 k = gSpecialVar_LastTalked - 2;
    s32 trainerId = gSaveBlock2Ptr->frontier.trainerIds[k];

    for (partnerId = 0; partnerId < ARRAY_COUNT(sPartnerTrainerTextTables); partnerId++)
    {
        if (sPartnerTrainerTextTables[partnerId].facilityClass == GetFrontierTrainerFacilityClass(trainerId))
            break;
    }

    switch (gSpecialVar_0x8005)
    {
    case PARTNER_MSGID_INTRO:
        if (trainerId == TRAINER_EREADER)
            return;
        if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            GetFrontierTrainerName(gStringVar1, trainerId);
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            GetFrontierTrainerName(gStringVar1, trainerId);
        }
        else
        {
            s32 i;
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                gStringVar1[i] = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].playerName[i];
            gStringVar1[i] = EOS;
            ConvertInternationalString(gStringVar1, gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].language);
            ConvertIntToDecimalStringN(gStringVar2, gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].number, STR_CONV_MODE_LEFT_ALIGN, 3);
            GetFrontierTrainerName(gStringVar3, trainerId);
        }
        break;
    case PARTNER_MSGID_MON1:
        monId = gSaveBlock2Ptr->frontier.trainerIds[8 + k * 2];
        GetPotentialPartnerMoveAndSpecies(trainerId, monId);
        break;
    case PARTNER_MSGID_MON2_ASK:
        monId = gSaveBlock2Ptr->frontier.trainerIds[9 + k * 2];
        GetPotentialPartnerMoveAndSpecies(trainerId, monId);
        break;
    case PARTNER_MSGID_ACCEPT:
        gPartnerTrainerId = trainerId;
        if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            gSaveBlock2Ptr->frontier.trainerIds[18] = gSaveBlock2Ptr->frontier.trainerIds[8 + k * 2];
            gSaveBlock2Ptr->frontier.trainerIds[19] = gSaveBlock2Ptr->frontier.trainerIds[9 + k * 2];
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            gSaveBlock2Ptr->frontier.trainerIds[18] = gFrontierTempParty[2];
            gSaveBlock2Ptr->frontier.trainerIds[19] = gFrontierTempParty[3];
        }
        else
        {
            gSaveBlock2Ptr->frontier.trainerIds[18] = gFrontierTempParty[0];
            gSaveBlock2Ptr->frontier.trainerIds[19] = gFrontierTempParty[1];
        }
        for (k = 0; k < FRONTIER_STAGES_PER_CHALLENGE * 2; k++)
        {
            while (1)
            {
                i = GetRandomScaledFrontierTrainerId(challengeNum, k / 2);
                if (gPartnerTrainerId == i)
                    continue;

                for (j = 0; j < k; j++)
                {
                    if (gSaveBlock2Ptr->frontier.trainerIds[j] == i)
                        break;
                }
                if (j == k)
                    break;
            }
            gSaveBlock2Ptr->frontier.trainerIds[k] = i;
        }
        gSaveBlock2Ptr->frontier.trainerIds[17] = trainerId;
        break;
    case PARTNER_MSGID_REJECT:
        break;
    }

    if (trainerId == TRAINER_EREADER)
        return;

    // First check is redundant, only needs to make sure it's not an Apprentice
    if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        ShowFieldMessage(sPartnerTrainerTextTables[partnerId].strings[gSpecialVar_0x8005]);
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        ShowFieldMessage(sPartnerTrainerTextTables[partnerId].strings[gSpecialVar_0x8005]);
    }
    // Trainer is a former/record-mixed Apprentice, do Apprentice message
    else
    {
        u8 apprenticeId = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id;
        ShowFieldMessage(sPartnerApprenticeTextTables[apprenticeId][gSpecialVar_0x8005]);
    }
}

static void LoadLinkMultiOpponentsData(void)
{
    s32 challengeNum;
    s32 i, j;
    s32 trainerId = 0;
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u32 battleNum = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    GetMultiplayerId(); // Yet another pointless function call.

    switch (gSpecialVar_Result)
    {
    case 0:
        if (battleMode == FRONTIER_MODE_LINK_MULTIS)
        {
            challengeNum = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
            if (IsLinkTaskFinished())
            {
                SendBlock(BitmaskAllOtherLinkPlayers(), &challengeNum, sizeof(challengeNum));
                gSpecialVar_Result = 1;
            }
        }
        else
        {
            gSpecialVar_Result = 6;
        }
        break;
    case 1:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            ResetBlockReceivedFlags();
            if (gBlockRecvBuffer[0][0] > gBlockRecvBuffer[1][0])
                challengeNum = gBlockRecvBuffer[0][0];
            else
                challengeNum = gBlockRecvBuffer[1][0];
            for (i = 0; i < FRONTIER_STAGES_PER_CHALLENGE * 2; i++)
            {
                do
                {
                    trainerId = GetRandomScaledFrontierTrainerId(challengeNum, i / 2);
                    for (j = 0; j < i; j++)
                    {
                        if (gSaveBlock2Ptr->frontier.trainerIds[j] == trainerId)
                            break;
                    }
                } while (i != j);
                if (i == j) // This condition is always true, because of the loop above.
                    gSaveBlock2Ptr->frontier.trainerIds[i] = trainerId;
            }
            gSpecialVar_Result = 2;
        }
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), &gSaveBlock2Ptr->frontier.trainerIds, sizeof(gSaveBlock2Ptr->frontier.trainerIds));
            gSpecialVar_Result = 3;
        }
        break;
    case 3:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            ResetBlockReceivedFlags();
            memcpy(&gSaveBlock2Ptr->frontier.trainerIds, gBlockRecvBuffer, sizeof(gSaveBlock2Ptr->frontier.trainerIds));
            gTrainerBattleOpponent_A = gSaveBlock2Ptr->frontier.trainerIds[battleNum * 2];
            gTrainerBattleOpponent_B = gSaveBlock2Ptr->frontier.trainerIds[battleNum * 2 + 1];
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
            SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_B, 1);
            if (gReceivedRemoteLinkPlayers && gWirelessCommType == 0)
                gSpecialVar_Result = 4;
            else
                gSpecialVar_Result = 6;
        }
        break;
    case 4:
        SetCloseLinkCallback();
        gSpecialVar_Result = 5;
        break;
    case 5:
        if (gReceivedRemoteLinkPlayers == 0)
        {
            gSpecialVar_Result = 6;
        }
        break;
    case 6:
        return;
    }
}

static void TowerTryCloseLink(void)
{
    if (gWirelessCommType != 0)
        SetCloseLinkCallback();
}

static void SetMultiPartnerGfx(void)
{
    // 0xF below means use VAR_OBJ_GFX_ID_E
    SetBattleFacilityTrainerGfxId(gSaveBlock2Ptr->frontier.trainerIds[17], 0xF);
}

static void SetTowerInterviewData(void)
{
    s32 i;
    u8 text[32];

    if (VarGet(VAR_FRONTIER_BATTLE_MODE) != FRONTIER_MODE_SINGLES)
        return;

    GetFrontierTrainerName(text, gTrainerBattleOpponent_A);
    StripExtCtrlCodes(text);
    StringCopy(gSaveBlock2Ptr->frontier.towerInterview.opponentName, text);
    GetBattleTowerTrainerLanguage(&gSaveBlock2Ptr->frontier.towerInterview.opponentLanguage, gTrainerBattleOpponent_A);
    gSaveBlock2Ptr->frontier.towerInterview.opponentSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[1]], MON_DATA_SPECIES, NULL);
    gSaveBlock2Ptr->frontier.towerInterview.playerSpecies = GetMonData(&gPlayerParty[gBattlerPartyIndexes[0]], MON_DATA_SPECIES, NULL);
    for (i = 0; i < POKEMON_NAME_LENGTH + 1; i++)
        gSaveBlock2Ptr->frontier.towerInterview.opponentMonNickname[i] = gBattleMons[0].nickname[i];
    gSaveBlock2Ptr->frontier.towerBattleOutcome = gBattleOutcome;
}

static void ValidateBattleTowerRecordChecksums(void)
{
    s32 i, j;
    u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerPlayer);
    u32 checksum = 0;

    for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
    {
        checksum += record[j];
    }
    if (gSaveBlock2Ptr->frontier.towerPlayer.checksum != checksum)
        ClearBattleTowerRecord(&gSaveBlock2Ptr->frontier.towerPlayer);

    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[i]);
        checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            checksum += record[j];
        }
        if (gSaveBlock2Ptr->frontier.towerRecords[i].checksum != checksum)
            ClearBattleTowerRecord(&gSaveBlock2Ptr->frontier.towerRecords[i]);
    }
}

void CalcEmeraldBattleTowerChecksum(struct EmeraldBattleTowerRecord *record)
{
    u32 i;

    record->checksum = 0;
    for (i = 0; i < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        record->checksum += ((u32 *)record)[i];
}

void CalcRubyBattleTowerChecksum(struct RSBattleTowerRecord *record)
{
    u32 i;

    record->checksum = 0;
    for (i = 0; i < (sizeof(struct RSBattleTowerRecord) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        record->checksum += ((u32 *)record)[i];
}

static void ClearBattleTowerRecord(struct EmeraldBattleTowerRecord *record)
{
    u32 i;

    for (i = 0; i < sizeof(struct EmeraldBattleTowerRecord) / 4; i++)
        ((u32 *)record)[i] = 0;
}

u16 GetCurrentBattleTowerWinStreak(u8 lvlMode, u8 battleMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];

    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static u8 GetMonCountForBattleMode(u8 battleMode)
{
    u8 partySizes[ARRAY_COUNT(sBattleTowerPartySizes)];
    memcpy(partySizes, sBattleTowerPartySizes, sizeof(sBattleTowerPartySizes));

    if (battleMode < ARRAY_COUNT(sBattleTowerPartySizes))
        return partySizes[battleMode];
    else
        return FRONTIER_PARTY_SIZE;
}

struct RibbonCounter
{
    u8 partyIndex;
    u8 count;
};

static void AwardBattleTowerRibbons(void)
{
    s32 i;
    u32 partyIndex;
#ifdef BUGFIX
    struct RibbonCounter ribbons[MAX_FRONTIER_PARTY_SIZE];
#else
    struct RibbonCounter ribbons[3]; // BUG: 4 Pokémon can receive ribbons in a double battle mode.
#endif
    u8 ribbonType = 0;
    u8 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u8 monCount = GetMonCountForBattleMode(battleMode);

    if (lvlMode != FRONTIER_LVL_50)
        ribbonType = MON_DATA_VICTORY_RIBBON;
    else
        ribbonType = MON_DATA_WINNING_RIBBON;

    gSpecialVar_Result = FALSE;

    if (GetCurrentBattleTowerWinStreak(lvlMode, battleMode) > 55)
    {
        for (i = 0; i < monCount; i++)
        {
            partyIndex = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
            ribbons[i].partyIndex = partyIndex;
            ribbons[i].count = 0;
            if (!GetMonData(&gSaveBlock1Ptr->playerParty[partyIndex], ribbonType))
            {
                gSpecialVar_Result = TRUE;
                SetMonData(&gSaveBlock1Ptr->playerParty[partyIndex], ribbonType, &gSpecialVar_Result);
                ribbons[i].count = GetRibbonCount(&gSaveBlock1Ptr->playerParty[partyIndex]);
            }
        }
    }

    if (gSpecialVar_Result)
    {
        IncrementGameStat(GAME_STAT_RECEIVED_RIBBONS);
        for (i = 1; i < monCount; i++)
        {
            if (ribbons[i].count > ribbons[0].count)
            {
                struct RibbonCounter prevBest = ribbons[0];
                ribbons[0] = ribbons[i];
                ribbons[i] = prevBest;
            }
        }
        if (ribbons[0].count > NUM_CUTIES_RIBBONS)
        {
            TryPutSpotTheCutiesOnAir(&gSaveBlock1Ptr->playerParty[ribbons[0].partyIndex], ribbonType);
        }
    }
}

// This is a leftover debugging function that is used to populate the E-Reader
// trainer with the player's current data.
static void UNUSED FillEReaderTrainerWithPlayerData(void)
{
    struct BattleTowerEReaderTrainer *ereaderTrainer = &gSaveBlock2Ptr->frontier.ereaderTrainer;
    s32 i, j;

    if (gSaveBlock2Ptr->playerGender != MALE)
    {
        ereaderTrainer->facilityClass = gTowerFemaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] + gSaveBlock2Ptr->playerTrainerId[1]
                                                        + gSaveBlock2Ptr->playerTrainerId[2] + gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerFemaleFacilityClasses)];
    }
    else
    {
        ereaderTrainer->facilityClass = gTowerMaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] + gSaveBlock2Ptr->playerTrainerId[1]
                                                        + gSaveBlock2Ptr->playerTrainerId[2] + gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerMaleFacilityClasses)];
    }

    CopyTrainerId(ereaderTrainer->trainerId, gSaveBlock2Ptr->playerTrainerId);
    StringCopy_PlayerName(ereaderTrainer->name, gSaveBlock2Ptr->playerName);

    ereaderTrainer->winStreak = 1;

    j = 7;
    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
    {
        ereaderTrainer->greeting[i] = gSaveBlock1Ptr->easyChatBattleStart[i];
        ereaderTrainer->farewellPlayerLost[i] = j;
        ereaderTrainer->farewellPlayerWon[i] = j + 6;
        j++;
    }

    for (i = 0; i < (int)ARRAY_COUNT(ereaderTrainer->party); i++)
        ConvertPokemonToBattleTowerPokemon(&gPlayerParty[i], &ereaderTrainer->party[i]);

    SetEReaderTrainerChecksum(ereaderTrainer);
}

u8 GetEreaderTrainerFrontSpriteId(void)
{
    return gFacilityClassToPicIndex[gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass];
}

u8 GetEreaderTrainerClassId(void)
{
    return gFacilityClassToTrainerClass[gSaveBlock2Ptr->frontier.ereaderTrainer.facilityClass];
}

void GetEreaderTrainerName(u8 *dst)
{
    s32 i;

    for (i = 0; i < 5; i++)
        dst[i] = gSaveBlock2Ptr->frontier.ereaderTrainer.name[i];

    dst[i] = EOS;
}

// Checks if the saved E-Reader trainer is valid.
void ValidateEReaderTrainer(void)
{
    u32 i;
    u32 checksum;
    struct BattleTowerEReaderTrainer *ereaderTrainer;

    gSpecialVar_Result = FALSE;
    ereaderTrainer = &gSaveBlock2Ptr->frontier.ereaderTrainer;

    checksum = 0;
    for (i = 0; i < (sizeof(struct BattleTowerEReaderTrainer) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        checksum |= ((u32 *)ereaderTrainer)[i];

    if (checksum == 0)
    {
        gSpecialVar_Result = TRUE;
        return;
    }

    checksum = 0;
    for (i = 0; i < (sizeof(struct BattleTowerEReaderTrainer) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        checksum += ((u32 *)ereaderTrainer)[i];

    if (gSaveBlock2Ptr->frontier.ereaderTrainer.checksum != checksum)
    {
        ClearEReaderTrainer(&gSaveBlock2Ptr->frontier.ereaderTrainer);
        gSpecialVar_Result = TRUE;
    }
}

static void SetEReaderTrainerChecksum(struct BattleTowerEReaderTrainer *ereaderTrainer)
{
    s32 i;

    ereaderTrainer->checksum = 0;
    for (i = 0; i < (sizeof(struct BattleTowerEReaderTrainer) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        ereaderTrainer->checksum += ((u32 *)ereaderTrainer)[i];
}

void ClearEReaderTrainer(struct BattleTowerEReaderTrainer *ereaderTrainer)
{
    u32 i;

    for (i = 0; i < (sizeof(struct BattleTowerEReaderTrainer)) / 4; i++)
        ((u32 *)ereaderTrainer)[i] = 0;
}

void CopyEReaderTrainerGreeting(void)
{
    FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.greeting);
}

static void CopyEReaderTrainerFarewellMessage(void)
{
    if (gBattleOutcome == B_OUTCOME_DREW)
        gStringVarBuffer[0] = EOS;
    else if (gBattleOutcome == B_OUTCOME_WON)
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.farewellPlayerWon);
    else
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.farewellPlayerLost);
}

void TryHideBattleTowerReporter(void)
{
    if (gSaveBlock2Ptr->frontier.challengeStatus == CHALLENGE_STATUS_SAVING)
        HideBattleTowerReporter();
    if (FlagGet(FLAG_CANCEL_BATTLE_ROOM_CHALLENGE) == TRUE)
    {
        HideBattleTowerReporter();
        FlagClear(FLAG_CANCEL_BATTLE_ROOM_CHALLENGE);
    }
}

#define STEVEN_OTID 61226

static void FillPartnerParty(u16 trainerId)
{
    s32 i, j;
    u32 ivs, level;
    u32 friendship;
    u16 monId;
    u32 otID;
    u8 trainerName[PLAYER_NAME_LENGTH + 1];
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_STEVEN_PARTNER)
    {
        for (i = 0; i < MULTI_PARTY_SIZE; i++)
        {
            do
            {
                j = Random32();
            } while (IsShinyOtIdPersonality(STEVEN_OTID, j) || sStevenMons[i].nature != GetNatureFromPersonality(j));
            CreateMon(&gPlayerParty[MULTI_PARTY_SIZE + i],
                      sStevenMons[i].species,
                      sStevenMons[i].level,
                      sStevenMons[i].fixedIV,
                      TRUE,
                      #ifdef BUGFIX
                      j,
                      #else
                      i, // BUG: personality was stored in the 'j' variable. As a result, Steven's Pokémon do not have the intended natures.
                      #endif
                      OT_ID_PRESET, STEVEN_OTID);
            for (j = 0; j < PARTY_SIZE; j++)
                SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_HP_EV + j, &sStevenMons[i].evs[j]);
            for (j = 0; j < MAX_MON_MOVES; j++)
                SetMonMoveSlot(&gPlayerParty[MULTI_PARTY_SIZE + i], sStevenMons[i].moves[j], j);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_NAME, gTrainers[TRAINER_STEVEN].trainerName);
            j = MALE;
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_GENDER, &j);
            CalculateMonStats(&gPlayerParty[MULTI_PARTY_SIZE + i]);
        }
    }
    else if (trainerId == TRAINER_EREADER)
    {
        // Scrapped, lol.
        trainerName[0] = gGameLanguage;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        level = SetFacilityPtrsGetLevel();
        ivs = GetFrontierTrainerFixedIvs(trainerId);
        otID = Random32();
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            monId = gSaveBlock2Ptr->frontier.trainerIds[i + 18];
            CreateMonWithEVSpreadNatureOTID(&gPlayerParty[MULTI_PARTY_SIZE + i],
                                                 gFacilityTrainerMons[monId].species,
                                                 level,
                                                 gFacilityTrainerMons[monId].nature,
                                                 ivs,
                                                 gFacilityTrainerMons[monId].evSpread,
                                                 otID);
            friendship = MAX_FRIENDSHIP;
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                SetMonMoveSlot(&gPlayerParty[MULTI_PARTY_SIZE + i], gFacilityTrainerMons[monId].moves[j], j);
                if (gFacilityTrainerMons[monId].moves[j] == MOVE_FRUSTRATION)
                    friendship = 0;
            }
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_FRIENDSHIP, &friendship);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_HELD_ITEM, &gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId]);
            for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
                trainerName[j] = gFacilityTrainers[trainerId].trainerName[j];
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_NAME, &trainerName);
            j = IsFrontierTrainerFemale(trainerId);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_GENDER, &j);
        }
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        trainerId -= TRAINER_RECORD_MIXING_FRIEND;
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            struct EmeraldBattleTowerRecord *record = &gSaveBlock2Ptr->frontier.towerRecords[trainerId];
            struct BattleTowerPokemon monData = record->party[gSaveBlock2Ptr->frontier.trainerIds[18 + i]];
            StringCopy(trainerName, record->name);
            if (record->language == LANGUAGE_JAPANESE)
            {
                if (monData.nickname[0] != EXT_CTRL_CODE_BEGIN || monData.nickname[1] != EXT_CTRL_CODE_JPN)
                {
                    monData.nickname[5] = EOS;
                    ConvertInternationalString(monData.nickname, LANGUAGE_JAPANESE);
                }
            }
            else
            {
                if (monData.nickname[0] == EXT_CTRL_CODE_BEGIN && monData.nickname[1] == EXT_CTRL_CODE_JPN)
                    trainerName[5] = EOS;
            }
            CreateBattleTowerMon_HandleLevel(&gPlayerParty[MULTI_PARTY_SIZE + i], &monData, TRUE);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_NAME, trainerName);
            j = IsFrontierTrainerFemale(trainerId + TRAINER_RECORD_MIXING_FRIEND);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_GENDER, &j);
        }
    }
    else
    {
        trainerId -= TRAINER_RECORD_MIXING_APPRENTICE;
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            CreateApprenticeMon(&gPlayerParty[MULTI_PARTY_SIZE + i], &gSaveBlock2Ptr->apprentices[trainerId], gSaveBlock2Ptr->frontier.trainerIds[18 + i]);
            j = IsFrontierTrainerFemale(trainerId + TRAINER_RECORD_MIXING_APPRENTICE);
            SetMonData(&gPlayerParty[MULTI_PARTY_SIZE + i], MON_DATA_OT_GENDER, &j);
        }
    }
}

bool32 RubyBattleTowerRecordToEmerald(struct RSBattleTowerRecord *src, struct EmeraldBattleTowerRecord *dst)
{
    s32 i, validMons = 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (src->party[i].species)
            validMons++;
    }

    if (validMons != FRONTIER_PARTY_SIZE)
    {
        memset(dst, 0, sizeof(*dst));
        return FALSE;
    }
    else
    {
        dst->lvlMode = src->lvlMode;
        dst->winStreak = src->winStreak;
        // UB: Reading outside the array. sRubyFacilityClassToEmerald has less than FACILITY_CLASSES_COUNT entries.
        #ifdef UBFIX
        for (i = 0; i < ARRAY_COUNT(sRubyFacilityClassToEmerald); i++)
        #else
        for (i = 0; i < FACILITY_CLASSES_COUNT; i++)
        #endif
        {
            if (sRubyFacilityClassToEmerald[i][0] == src->facilityClass)
                break;
        }
        if (i != FACILITY_CLASSES_COUNT)
            dst->facilityClass = sRubyFacilityClassToEmerald[i][1];
        else
            dst->facilityClass = FACILITY_CLASS_YOUNGSTER;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            dst->name[i] = src->name[i];
        for (i = 0; i < TRAINER_ID_LENGTH; i++)
            dst->trainerId[i] = src->trainerId[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->greeting[i] = src->greeting[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->speechWon[i] = sRecordTrainerSpeechWon[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->speechLost[i] = sRecordTrainerSpeechLost[i];
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            dst->party[i] = src->party[i];

        CpuFill32(0, &dst->party[FRONTIER_PARTY_SIZE], sizeof(dst->party[FRONTIER_PARTY_SIZE]));
        CalcEmeraldBattleTowerChecksum(dst);
        return TRUE;
    }
}

bool32 EmeraldBattleTowerRecordToRuby(struct EmeraldBattleTowerRecord *src, struct RSBattleTowerRecord *dst)
{
    s32 i, validMons = 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (src->party[i].species)
            validMons++;
    }

    if (validMons != FRONTIER_PARTY_SIZE)
    {
        memset(dst, 0, sizeof(*dst));
        return FALSE;
    }
    else
    {
        dst->lvlMode = src->lvlMode;
        dst->winStreak = src->winStreak;
        // UB: Reading outside the array. sRubyFacilityClassToEmerald has less than FACILITY_CLASSES_COUNT entries.
        #ifdef UBFIX
        for (i = 0; i < ARRAY_COUNT(sRubyFacilityClassToEmerald); i++)
        #else
        for (i = 0; i < FACILITY_CLASSES_COUNT; i++)
        #endif
        {
            if (sRubyFacilityClassToEmerald[i][1] == src->facilityClass)
                break;
        }
        if (i != FACILITY_CLASSES_COUNT)
            dst->facilityClass = sRubyFacilityClassToEmerald[i][0];
        else
            dst->facilityClass = RS_FACILITY_CLASS_YOUNGSTER;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            dst->name[i] = src->name[i];
        for (i = 0; i < TRAINER_ID_LENGTH; i++)
            dst->trainerId[i] = src->trainerId[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->greeting[i] = src->greeting[i];
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            dst->party[i] = src->party[i];

        CalcRubyBattleTowerChecksum(dst);
        return TRUE;
    }
}

void CalcApprenticeChecksum(struct Apprentice *apprentice)
{
    s32 i;

    apprentice->checksum = 0;
    for (i = 0; i < offsetof(struct Apprentice, checksum) / sizeof(u32); i++)
        apprentice->checksum += ((u32 *)apprentice)[i];
}

static void ClearApprentice(struct Apprentice *apprentice)
{
    s32 i;

    for (i = 0; i < sizeof(struct Apprentice) / sizeof(u32); i++)
        ((u32 *)apprentice)[i] = 0;
    ResetApprenticeStruct(apprentice);
}

static void ValidateApprenticesChecksums(void)
{
    s32 i, j;

    for (i = 0; i < APPRENTICE_COUNT; i++)
    {
        u32 *data = (u32 *) &gSaveBlock2Ptr->apprentices[i];
        u32 checksum = 0;
        for (j = 0; j < offsetof(struct Apprentice, checksum) / sizeof(u32); j++)
            checksum += data[j];
        if (gSaveBlock2Ptr->apprentices[i].checksum != checksum)
            ClearApprentice(&gSaveBlock2Ptr->apprentices[i]);
    }
}

void GetBattleTowerTrainerLanguage(u8 *dst, u16 trainerId)
{
    if (trainerId == TRAINER_EREADER)
    {
        *dst = gGameLanguage;
    }
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
    {
        *dst = gGameLanguage;
    }
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            *dst = GetRecordedBattleRecordMixFriendLanguage();
        else
            *dst = gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].language;
    }
    else
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            *dst = GetRecordedBattleApprenticeLanguage();
        else
            *dst = gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }
}

u8 SetFacilityPtrsGetLevel(void)
{
    if (gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_TENT)
    {
        return SetTentPtrsGetLevel();
    }
    else
    {
        gFacilityTrainers = gBattleFrontierTrainers;
        gFacilityTrainerMons = gBattleFrontierMons;
        return GetFrontierEnemyMonLevel(gSaveBlock2Ptr->frontier.lvlMode);
    }
}

u8 GetFrontierEnemyMonLevel(u8 lvlMode)
{
    u8 level;

    switch (lvlMode)
    {
    default:
    case FRONTIER_LVL_50:
        level = FRONTIER_MAX_LEVEL_50;
        break;
    case FRONTIER_LVL_OPEN:
        level = GetHighestLevelInPlayerParty();
        if (level < FRONTIER_MIN_LEVEL_OPEN)
            level = FRONTIER_MIN_LEVEL_OPEN;
        break;
    }

    return level;
}

s32 GetHighestLevelInPlayerParty(void)
{
    s32 highestLevel = 0;
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL)
            && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG, NULL) != SPECIES_EGG)
        {
            s32 level = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL, NULL);
            if (level > highestLevel)
                highestLevel = level;
        }
    }

    return highestLevel;
}

// Frontier Trainer parties are roughly scaled in difficulty with higher trainer IDs, so scale IVs as well
// Duplicated in Battle Dome as GetDomeTrainerMonIvs
static u8 GetFrontierTrainerFixedIvs(u16 trainerId)
{
    u8 fixedIv;

    if (trainerId <= FRONTIER_TRAINER_JILL)         // 0 - 99
        fixedIv = 3;
    else if (trainerId <= FRONTIER_TRAINER_CHLOE)   // 100 - 119
        fixedIv = 6;
    else if (trainerId <= FRONTIER_TRAINER_SOFIA)   // 120 - 139
        fixedIv = 9;
    else if (trainerId <= FRONTIER_TRAINER_JAZLYN)  // 140 - 159
        fixedIv = 12;
    else if (trainerId <= FRONTIER_TRAINER_ALISON)  // 160 - 179
        fixedIv = 15;
    else if (trainerId <= FRONTIER_TRAINER_LAMAR)   // 180 - 199
        fixedIv = 18;
    else if (trainerId <= FRONTIER_TRAINER_TESS)    // 200 - 219
        fixedIv = 21;
    else                                            // 220+ (- 299)
        fixedIv = MAX_PER_STAT_IVS;

    return fixedIv;
}

static u16 GetBattleTentTrainerId(void)
{
    u32 facility = VarGet(VAR_FRONTIER_FACILITY);

    if (facility == FRONTIER_FACILITY_PALACE)       // Verdanturf Tent; uses Palace mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_ARENA)   // Fallarbor Tent; uses Arena mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_FACTORY) // Slateport Tent; uses Factory mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_TOWER)
        return 0;
    else
        return 0;
}

static u8 SetTentPtrsGetLevel(void)
{
    u8 level = TENT_MIN_LEVEL;
    u32 facility = VarGet(VAR_FRONTIER_FACILITY);

    if (facility == FRONTIER_FACILITY_FACTORY)
    {
        gFacilityTrainers = gSlateportBattleTentTrainers;
        gFacilityTrainerMons = gSlateportBattleTentMons;
    }
    else if (facility == FRONTIER_FACILITY_PALACE)
    {
        gFacilityTrainers = gVerdanturfBattleTentTrainers;
        gFacilityTrainerMons = gVerdanturfBattleTentMons;
    }
    else if (facility == FRONTIER_FACILITY_ARENA)
    {
        gFacilityTrainers = gFallarborBattleTentTrainers;
        gFacilityTrainerMons = gFallarborBattleTentMons;
    }
    else
    {
        gFacilityTrainers = gBattleFrontierTrainers;
        gFacilityTrainerMons = gBattleFrontierMons;
    }

    level = GetHighestLevelInPlayerParty();
    if (level < TENT_MIN_LEVEL)
        level = TENT_MIN_LEVEL;

    return level;
}

static void SetNextBattleTentOpponent(void)
{
    s32 i;
    u16 trainerId;

    do
    {
        trainerId = GetBattleTentTrainerId();
        for (i = 0; i < gSaveBlock2Ptr->frontier.curChallengeBattleNum; i++)
        {
            if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
                break;
        }
    } while (i != gSaveBlock2Ptr->frontier.curChallengeBattleNum);

    gTrainerBattleOpponent_A = trainerId;
    SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
    if (gSaveBlock2Ptr->frontier.curChallengeBattleNum + 1 < TENT_STAGES_PER_CHALLENGE)
       gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = gTrainerBattleOpponent_A;
}

static void FillTentTrainerParty_(u16 trainerId, u8 firstMonId, u8 monCount)
{
    s32 i, j;
    u16 chosenMonIndices[MAX_FRONTIER_PARTY_SIZE];
    u8 friendship;
    u8 level = SetTentPtrsGetLevel();
    u8 fixedIV = 0;
    u8 bfMonCount;
    const u16 *monSet = NULL;
    u32 otID = 0;
    u16 monId;

    monSet = gFacilityTrainers[gTrainerBattleOpponent_A].monSet;

    bfMonCount = 0;
    monId = monSet[bfMonCount];
    while (monId != 0xFFFF)
    {
        bfMonCount++;
        monId = monSet[bfMonCount];
        if (monId == 0xFFFF)
            break;
    }

    i = 0;
    otID = Random32();
    while (i != monCount)
    {
        u16 monId = monSet[Random() % bfMonCount];

        // Ensure this Pokémon species isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gEnemyParty[j], MON_DATA_SPECIES, NULL) == gFacilityTrainerMons[monId].species)
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this Pokemon's held item isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gEnemyParty[j], MON_DATA_HELD_ITEM, NULL) != ITEM_NONE
             && GetMonData(&gEnemyParty[j], MON_DATA_HELD_ITEM, NULL) == gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId])
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this exact Pokémon index isn't a duplicate. This check doesn't seem necessary
        // because the species and held items were already checked directly above.
        for (j = 0; j < i; j++)
        {
            if (chosenMonIndices[j] == monId)
                break;
        }
        if (j != i)
            continue;

        chosenMonIndices[i] = monId;

        // Place the chosen Pokémon into the trainer's party.
        CreateMonWithEVSpreadNatureOTID(&gEnemyParty[i + firstMonId],
                                             gFacilityTrainerMons[monId].species,
                                             level,
                                             gFacilityTrainerMons[monId].nature,
                                             fixedIV,
                                             gFacilityTrainerMons[monId].evSpread,
                                             otID);

        friendship = MAX_FRIENDSHIP;
        // Give the chosen Pokémon its specified moves.
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            SetMonMoveSlot(&gEnemyParty[i + firstMonId], gFacilityTrainerMons[monId].moves[j], j);
            if (gFacilityTrainerMons[monId].moves[j] == MOVE_FRUSTRATION)
                friendship = 0;  // Frustration is more powerful the lower the pokemon's friendship is.
        }

        SetMonData(&gEnemyParty[i + firstMonId], MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&gEnemyParty[i + firstMonId], MON_DATA_HELD_ITEM, &gBattleFrontierHeldItems[gFacilityTrainerMons[monId].itemTableId]);

        // The Pokémon was successfully added to the trainer's party, so it's safe to move on to
        // the next party slot.
        i++;
    }
}

u8 FacilityClassToGraphicsId(u8 facilityClass)
{
    u8 trainerObjectGfxId;
    u8 i;

    // Search male classes.
    for (i = 0; i < ARRAY_COUNT(gTowerMaleFacilityClasses); i++)
    {
        if (gTowerMaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerMaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerMaleTrainerGfxIds[i];
        return trainerObjectGfxId;
    }

    // Search female classes.
    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses); i++)
    {
        if (gTowerFemaleFacilityClasses[i] == facilityClass)
            break;
    }
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
    {
        trainerObjectGfxId = gTowerFemaleTrainerGfxIds[i];
        return trainerObjectGfxId;
    }
    else
    {
        return OBJ_EVENT_GFX_BOY_1;
    }
}

bool32 ValidateBattleTowerRecord(u8 recordId) // unused
{
    s32 i;
    u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[recordId]);
    u32 checksum = 0;
    u32 hasData = 0;
    for (i = 0; i < offsetof(struct EmeraldBattleTowerRecord, checksum) / sizeof(u32); i++)
    {
        checksum += record[i];
        hasData |= record[i];
    }

    if (checksum == 0 && hasData == 0)
    {
        return FALSE;
    }
    else if (gSaveBlock2Ptr->frontier.towerRecords[recordId].checksum != checksum)
    {
        ClearBattleTowerRecord(&gSaveBlock2Ptr->frontier.towerRecords[recordId]);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void TrySetLinkBattleTowerEnemyPartyLevel(void)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
    {
        s32 i;
        u8 enemyLevel = SetFacilityPtrsGetLevel();

        for (i = 0; i < PARTY_SIZE; i++)
        {
            u32 species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, NULL);
            if (species)
            {
                SetMonData(&gEnemyParty[i], MON_DATA_EXP, &gExperienceTables[gSpeciesInfo[species].growthRate][enemyLevel]);
                CalculateMonStats(&gEnemyParty[i]);
            }
        }
    }
}
