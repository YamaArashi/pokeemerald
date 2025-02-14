#include "global.h"
#include "battle_dome.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "frontier_util.h"
#include "battle_message.h"
#include "event_data.h"
#include "overworld.h"
#include "util.h"
#include "malloc.h"
#include "string_util.h"
#include "random.h"
#include "task.h"
#include "main.h"
#include "gpu_regs.h"
#include "text.h"
#include "bg.h"
#include "window.h"
#include "strings.h"
#include "palette.h"
#include "decompress.h"
#include "party_menu.h"
#include "menu.h"
#include "sound.h"
#include "pokemon_icon.h"
#include "data.h"
#include "international_string_util.h"
#include "trainer_pokemon_sprites.h"
#include "scanline_effect.h"
#include "script_pokemon_util.h"
#include "graphics.h"
#include "constants/battle_dome.h"
#include "constants/frontier_util.h"
#include "constants/moves.h"
#include "constants/trainers.h"
#include "constants/abilities.h"
#include "constants/songs.h"
#include "constants/battle_frontier.h"
#include "constants/rgb.h"

#define TAG_BUTTONS 0

// Enough space to hold 2 match info cards worth of trainers and their parties
#define NUM_INFOCARD_SPRITES ((FRONTIER_PARTY_SIZE + 1) * 4)
#define NUM_INFOCARD_TRAINERS 2

// An 'Info Card' is a trainer or match information page that can be viewed on the Tourney Tree
struct TourneyTreeInfoCard
{
    u8 spriteIds[NUM_INFOCARD_SPRITES];
    u8 pos;
    u8 tournamentIds[NUM_INFOCARD_TRAINERS];
};

struct TourneyTreeLineSection
{
    u8 x;
    u8 y;
    u16 tile;
};

#define DOME_TRAINERS gSaveBlock2Ptr->frontier.domeTrainers
#define DOME_MONS     gSaveBlock2Ptr->frontier.domeMonIds

#define tState              data[0]

// Task data for Task_ShowTourneyTree
#define tNotInteractive     data[1]
#define tIsPrevTourneyTree  data[4]

// Task data for Task_ShowTourneyInfoCard
#define tTournamentId       data[1]
#define tMode               data[2]
#define tPrevTaskId         data[3]

enum {
    EFFECTIVENESS_MODE_GOOD,
    EFFECTIVENESS_MODE_BAD,
    EFFECTIVENESS_MODE_AI_VS_AI,
};

// Window IDs for the tourney tree
enum {
    TOURNEYWIN_NAMES_LEFT,
    TOURNEYWIN_NAMES_RIGHT,
    TOURNEYWIN_TITLE,
};

// Window IDs for the trainer (WIN_TRAINER_*) and match (WIN_MATCH_*) info cards.
// All 9 have a duplicate window at WIN + NUM_INFO_CARD_WINDOWS used by the alternate info card
enum {
    WIN_TRAINER_NAME,
    WIN_TRAINER_MON1_NAME,
    WIN_TRAINER_MON2_NAME, // Used implicitly
    WIN_TRAINER_MON3_NAME, // Used implicitly
    WIN_TRAINER_FLAVOR_TEXT = WIN_TRAINER_MON1_NAME + FRONTIER_PARTY_SIZE, // Trainer's potential, battle style, and stat texts
    WIN_MATCH_NUMBER,
    WIN_MATCH_TRAINER_NAME_LEFT,
    WIN_MATCH_TRAINER_NAME_RIGHT,
    WIN_MATCH_WIN_TEXT,
    NUM_INFO_CARD_WINDOWS
};

static u8 GetDomeTrainerMonIvs(u16);
static void SwapDomeTrainers(int, int, u16 *);
static void CalcDomeMonStats(u16, int, int, u8, u8, int *);
static void CreateDomeOpponentMons(u16);
static int SelectOpponentMons_Good(u16, bool8);
static int SelectOpponentMons_Bad(u16, bool8);
static int GetTypeEffectivenessPoints(int, int, int);
static int SelectOpponentMonsFromParty(int *, bool8);
static void Task_ShowTourneyInfoCard(u8);
static void Task_HandleInfoCardInput(u8);
static u8 Task_GetInfoCardInput(u8);
static void SetFacilityTrainerAndMonPtrs(void);
static int TrainerIdToTournamentId(u16);
static u16 TrainerIdOfPlayerOpponent(void);
static void Task_ShowTourneyTree(u8);
static void Task_HandleStaticTourneyTreeInput(u8);
static void CB2_TourneyTree(void);
static void VblankCb_TourneyInfoCard(void);
static void DisplayMatchInfoOnCard(u8, u8);
static void DisplayTrainerInfoOnCard(u8, u8);
static int BufferDomeWinString(u8, u8 *);
static u8 GetDomeBrainTrainerPicId(void);
static u8 GetDomeBrainTrainerClass(void);
static void CopyDomeBrainTrainerName(u8 *);
static void CopyDomeTrainerName(u8 *, u16);
static void HblankCb_TourneyTree(void);
static void VblankCb_TourneyTree(void);
static u8 UpdateTourneyTreeCursor(u8);
static void DecideRoundWinners(u8);
static u8 GetOpposingNPCTournamentIdByRound(u8, u8);
static void DrawTourneyAdvancementLine(u8, u8);
static void SpriteCB_HorizontalScrollArrow(struct Sprite *);
static void SpriteCB_VerticalScrollArrow(struct Sprite *);
static void InitDomeChallenge(void);
static void GetDomeData(void);
static void SetDomeData(void);
static void BufferDomeRoundText(void);
static void BufferDomeOpponentName(void);
static void InitDomeOpponentParty(void);
static void ShowDomeOpponentInfo(void);
static void ShowDomeTourneyTree(void);
static void ShowPreviousDomeTourneyTree(void);
static void SetDomeOpponentId(void);
static void SetDomeOpponentGraphicsId(void);
static void ShowNonInteractiveDomeTourneyTree(void);
static void ResolveDomeRoundWinners(void);
static void SaveDomeChallenge(void);
static void IncrementDomeStreaks(void);
static void ResetSketchedMoves(void);
static void RestoreDomePlayerPartyHeldItems(void);
static void ReduceDomePlayerPartyToSelectedMons(void);
static void GetPlayerSeededBeforeOpponent(void);
static void BufferLastDomeWinnerName(void);
static void InitRandomTourneyTreeResults(void);
static void InitDomeTrainers(void);

EWRAM_DATA u32 gPlayerPartyLostHP = 0; // never read
static EWRAM_DATA u32 sPlayerPartyMaxHP = 0; // never read
static EWRAM_DATA struct TourneyTreeInfoCard *sInfoCard = {0};
static EWRAM_DATA u8 *sTilemapBuffer = NULL;

// Each move has an array of points for different move characteristics which contribute to a tourney trainers listed battle style (see sBattleStyleThresholds)
// All move points are either 1 or 0, so theyre essentially flags saying whether or not the move has that characteristic
static const u8 sBattleStyleMovePoints[MOVES_COUNT][NUM_MOVE_POINT_TYPES] =
{
    [MOVE_NONE]          = {0},
    [MOVE_POUND]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_KARATE_CHOP]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_DOUBLE_SLAP]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_COMET_PUNCH]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_MEGA_PUNCH]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_PAY_DAY]       = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FIRE_PUNCH]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_ICE_PUNCH]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_THUNDER_PUNCH] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SCRATCH]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_VICE_GRIP]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_GUILLOTINE]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_RAZOR_WIND]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SWORDS_DANCE]  = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_POPULAR] = 1},
    [MOVE_CUT]           = {[MOVE_POINTS_DMG] = 1},
    [MOVE_GUST]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_WING_ATTACK]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_WHIRLWIND]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FLY]           = {[MOVE_POINTS_DMG] = 1},
    [MOVE_BIND]          = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SLAM]          = {[MOVE_POINTS_DMG] = 1},
    [MOVE_VINE_WHIP]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_STOMP]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DOUBLE_KICK]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_MEGA_KICK]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_JUMP_KICK]     = {[MOVE_POINTS_DMG] = 1},
    [MOVE_ROLLING_KICK]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SAND_ATTACK]   = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_HEADBUTT]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_HORN_ATTACK]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FURY_ATTACK]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_HORN_DRILL]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_TACKLE]        = {[MOVE_POINTS_DMG] = 1},
    [MOVE_BODY_SLAM]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_WRAP]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_TAKE_DOWN]     = {[MOVE_POINTS_DMG] = 1},
    [MOVE_THRASH]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DOUBLE_EDGE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TAIL_WHIP]     = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_POISON_STING]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_TWINEEDLE]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_PIN_MISSILE]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_LEER]          = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_BITE]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_GROWL]         = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ROAR]          = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SING]          = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_SUPERSONIC]    = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_SONIC_BOOM]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_DISABLE]       = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_ACID]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_EMBER]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FLAMETHROWER]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MIST]          = {0},
    [MOVE_WATER_GUN]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_HYDRO_PUMP]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SURF]          = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_ICE_BEAM]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BLIZZARD]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_PSYBEAM]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BUBBLE_BEAM]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_AURORA_BEAM]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_HYPER_BEAM]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_PECK]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_DRILL_PECK]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SUBMISSION]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_LOW_KICK]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_COUNTER]       = {[MOVE_POINTS_DEF] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_SEISMIC_TOSS]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_STRENGTH]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ABSORB]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_MEGA_DRAIN]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_LEECH_SEED]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STATUS] = 1},
    [MOVE_GROWTH]        = {[MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_RAZOR_LEAF]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SOLAR_BEAM]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_POISON_POWDER] = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_STUN_SPORE]    = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_SLEEP_POWDER]  = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_PETAL_DANCE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_STRING_SHOT]   = {[MOVE_POINTS_STAT_LOWER] = 1},
    [MOVE_DRAGON_RAGE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FIRE_SPIN]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_THUNDER_SHOCK] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_THUNDERBOLT]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_THUNDER_WAVE]  = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_THUNDER]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_ROCK_THROW]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_EARTHQUAKE]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_FISSURE]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LUCK] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_DIG]           = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TOXIC]         = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_CONFUSION]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_PSYCHIC]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_HYPNOSIS]      = {[MOVE_POINTS_COMBO] = 1},
    [MOVE_MEDITATE]      = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_AGILITY]       = {[MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_QUICK_ATTACK]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_RAGE]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TELEPORT]      = {0},
    [MOVE_NIGHT_SHADE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_MIMIC]         = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SCREECH]       = {[MOVE_POINTS_STAT_LOWER] = 1},
    [MOVE_DOUBLE_TEAM]   = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_RECOVER]       = {0},
    [MOVE_HARDEN]        = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_MINIMIZE]      = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_SMOKESCREEN]   = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_DEF] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CONFUSE_RAY]   = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_WITHDRAW]      = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_DEFENSE_CURL]  = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_BARRIER]       = {[MOVE_POINTS_DEF] = 1},
    [MOVE_LIGHT_SCREEN]  = {[MOVE_POINTS_DEF] = 1},
    [MOVE_HAZE]          = {0},
    [MOVE_REFLECT]       = {[MOVE_POINTS_DEF] = 1},
    [MOVE_FOCUS_ENERGY]  = {[MOVE_POINTS_COMBO] = 1},
    [MOVE_BIDE]          = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_METRONOME]     = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_MIRROR_MOVE]   = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_SELF_DESTRUCT] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_EGG_BOMB]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_LICK]          = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SMOG]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SLUDGE]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BONE_CLUB]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FIRE_BLAST]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_WATERFALL]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CLAMP]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SWIFT]         = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SKULL_BASH]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_SPIKE_CANNON]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CONSTRICT]     = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_AMNESIA]       = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_KINESIS]       = {[MOVE_POINTS_STAT_LOWER] = 1},
    [MOVE_SOFT_BOILED]   = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_HI_JUMP_KICK]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_GLARE]         = {[MOVE_POINTS_STAT_LOWER] = 1},
    [MOVE_DREAM_EATER]   = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_RARE] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_POISON_GAS]    = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_BARRAGE]       = {[MOVE_POINTS_DMG] = 1},
    [MOVE_LEECH_LIFE]    = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_LOVELY_KISS]   = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_SKY_ATTACK]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_TRANSFORM]     = {[MOVE_POINTS_RARE] = 1},
    [MOVE_BUBBLE]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DIZZY_PUNCH]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SPORE]         = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FLASH]         = {0},
    [MOVE_PSYWAVE]       = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SPLASH]        = {[MOVE_POINTS_RARE] = 1},
    [MOVE_ACID_ARMOR]    = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_CRABHAMMER]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_EXPLOSION]     = {[MOVE_POINTS_RISKY] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_FURY_SWIPES]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_BONEMERANG]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_REST]          = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1},
    [MOVE_ROCK_SLIDE]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_HYPER_FANG]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SHARPEN]       = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_CONVERSION]    = {[MOVE_POINTS_DEF] = 1},
    [MOVE_TRI_ATTACK]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SUPER_FANG]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SLASH]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SUBSTITUTE]    = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DEF] = 1},
    [MOVE_STRUGGLE]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1}, // Odd that this is assigned qualities
    [MOVE_SKETCH]        = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_LUCK] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_TRIPLE_KICK]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_THIEF]         = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SPIDER_WEB]    = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_MIND_READER]   = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_NIGHTMARE]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FLAME_WHEEL]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SNORE]         = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_CURSE]         = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_FLAIL]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CONVERSION_2]  = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_AEROBLAST]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_COTTON_SPORE]  = {[MOVE_POINTS_STAT_LOWER] = 1},
    [MOVE_REVERSAL]      = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SPITE]         = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_RISKY] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_POWDER_SNOW]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_PROTECT]       = {[MOVE_POINTS_DEF] = 1, [MOVE_POINTS_POPULAR] = 1},
    [MOVE_MACH_PUNCH]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SCARY_FACE]    = {0},
    [MOVE_FAINT_ATTACK]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SWEET_KISS]    = {0},
    [MOVE_BELLY_DRUM]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_SLUDGE_BOMB]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MUD_SLAP]      = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_OCTAZOOKA]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SPIKES]        = {[MOVE_POINTS_COMBO] = 1},
    [MOVE_ZAP_CANNON]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_LUCK] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FORESIGHT]     = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_DESTINY_BOND]  = {[MOVE_POINTS_RISKY] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_PERISH_SONG]   = {[MOVE_POINTS_RISKY] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_ICY_WIND]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DETECT]        = {[MOVE_POINTS_DEF] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_BONE_RUSH]     = {[MOVE_POINTS_DMG] = 1},
    [MOVE_LOCK_ON]       = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_OUTRAGE]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SANDSTORM]     = {0},
    [MOVE_GIGA_DRAIN]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_ENDURE]        = {[MOVE_POINTS_DEF] = 1},
    [MOVE_CHARM]         = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ROLLOUT]       = {[MOVE_POINTS_DMG] = 1},
    [MOVE_FALSE_SWIPE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SWAGGER]       = {[MOVE_POINTS_EFFECT] = 1},
    [MOVE_MILK_DRINK]    = {[MOVE_POINTS_HEAL] = 1},
    [MOVE_SPARK]         = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FURY_CUTTER]   = {[MOVE_POINTS_DMG] = 1},
    [MOVE_STEEL_WING]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MEAN_LOOK]     = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_ATTRACT]       = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SLEEP_TALK]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_HEAL_BELL]     = {[MOVE_POINTS_LOW_PP] = 1},
    [MOVE_RETURN]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_PRESENT]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_FRUSTRATION]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SAFEGUARD]     = {[MOVE_POINTS_DEF] = 1},
    [MOVE_PAIN_SPLIT]    = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SACRED_FIRE]   = {[MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MAGNITUDE]     = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_DYNAMIC_PUNCH] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LUCK] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MEGAHORN]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_DRAGON_BREATH] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BATON_PASS]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_RARE] = 1},
    [MOVE_ENCORE]        = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_PURSUIT]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_RAPID_SPIN]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SWEET_SCENT]   = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_IRON_TAIL]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_METAL_CLAW]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_VITAL_THROW]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_MORNING_SUN]   = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SYNTHESIS]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_MOONLIGHT]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_HIDDEN_POWER]  = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CROSS_CHOP]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_TWISTER]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_RAIN_DANCE]    = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SUNNY_DAY]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_CRUNCH]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MIRROR_COAT]   = {[MOVE_POINTS_DEF] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_PSYCH_UP]      = {[MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_EXTREME_SPEED] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_ANCIENT_POWER] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SHADOW_BALL]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FUTURE_SIGHT]  = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1},
    [MOVE_ROCK_SMASH]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_WHIRLPOOL]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BEAT_UP]       = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FAKE_OUT]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_UPROAR]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_STOCKPILE]     = {[MOVE_POINTS_COMBO] = 1},
    [MOVE_SPIT_UP]       = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_SWALLOW]       = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1},
    [MOVE_HEAT_WAVE]     = {[MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_HAIL]          = {0},
    [MOVE_TORMENT]       = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FLATTER]       = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_WILL_O_WISP]   = {[MOVE_POINTS_STATUS] = 1},
    [MOVE_MEMENTO]       = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FACADE]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FOCUS_PUNCH]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_SMELLING_SALT] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FOLLOW_ME]     = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_NATURE_POWER]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_CHARGE]        = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TAUNT]         = {[MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_HELPING_HAND]  = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TRICK]         = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ROLE_PLAY]     = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_WISH]          = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ASSIST]        = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_INGRAIN]       = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_DEF] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_SUPERPOWER]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_MAGIC_COAT]    = {[MOVE_POINTS_DEF] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_RECYCLE]       = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_REVENGE]       = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_BRICK_BREAK]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_YAWN]          = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_KNOCK_OFF]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_ENDEAVOR]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_ERUPTION]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SKILL_SWAP]    = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_IMPRISON]      = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_REFRESH]       = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_GRUDGE]        = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SNATCH]        = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LUCK] = 1},
    [MOVE_SECRET_POWER]  = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DIVE]          = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ARM_THRUST]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CAMOUFLAGE]    = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TAIL_GLOW]     = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_LUSTER_PURGE]  = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MIST_BALL]     = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_FEATHER_DANCE] = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_TEETER_DANCE]  = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_BLAZE_KICK]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MUD_SPORT]     = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ICE_BALL]      = {[MOVE_POINTS_DMG] = 1},
    [MOVE_NEEDLE_ARM]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SLACK_OFF]     = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_HYPER_VOICE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_POISON_FANG]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_CRUSH_CLAW]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BLAST_BURN]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_HYDRO_CANNON]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_METEOR_MASH]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_ASTONISH]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_WEATHER_BALL]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_AROMATHERAPY]  = {[MOVE_POINTS_LOW_PP] = 1},
    [MOVE_FAKE_TEARS]    = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_AIR_CUTTER]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_OVERHEAT]      = {[MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_ODOR_SLEUTH]   = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_ROCK_TOMB]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SILVER_WIND]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_METAL_SOUND]   = {0},
    [MOVE_GRASS_WHISTLE] = {0},
    [MOVE_TICKLE]        = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_COSMIC_POWER]  = {0},
    [MOVE_WATER_SPOUT]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_SIGNAL_BEAM]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SHADOW_PUNCH]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_EXTRASENSORY]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SKY_UPPERCUT]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SAND_TOMB]     = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_SHEER_COLD]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LUCK] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_MUDDY_WATER]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_BULLET_SEED]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_AERIAL_ACE]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_ICICLE_SPEAR]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_IRON_DEFENSE]  = {[MOVE_POINTS_DEF] = 1},
    [MOVE_BLOCK]         = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_HOWL]          = {0},
    [MOVE_DRAGON_CLAW]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_FRENZY_PLANT]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_BULK_UP]       = {[MOVE_POINTS_COMBO] = 1},
    [MOVE_BOUNCE]        = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_MUD_SHOT]      = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_POISON_TAIL]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_COVET]         = {[MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_VOLT_TACKLE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1},
    [MOVE_MAGICAL_LEAF]  = {[MOVE_POINTS_DMG] = 1},
    [MOVE_WATER_SPORT]   = {[MOVE_POINTS_ACCURATE] = 1},
    [MOVE_CALM_MIND]     = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_LEAF_BLADE]    = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1},
    [MOVE_DRAGON_DANCE]  = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1},
    [MOVE_ROCK_BLAST]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_SHOCK_WAVE]    = {[MOVE_POINTS_DMG] = 1},
    [MOVE_WATER_PULSE]   = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_ACCURATE] = 1, [MOVE_POINTS_EFFECT] = 1},
    [MOVE_DOOM_DESIRE]   = {[MOVE_POINTS_RARE] = 1, [MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1},
    [MOVE_PSYCHO_BOOST]  = {[MOVE_POINTS_DMG] = 1, [MOVE_POINTS_POWERFUL] = 1, [MOVE_POINTS_STRONG] = 1, [MOVE_POINTS_LOW_PP] = 1, [MOVE_POINTS_EFFECT] = 1},
};

// This array is searched in-order to determine what battle style a tourney trainer uses.
// If the sum of the points for the party's moves meets/exceeds all the point totals of an element, then they use that battle style
static const u8 sBattleStyleThresholds[NUM_BATTLE_STYLES - 1][NUM_MOVE_POINT_TYPES] =
{
    [DOME_BATTLE_STYLE_RISKY]           = {[MOVE_POINTS_RISKY] = 1},
    [DOME_BATTLE_STYLE_STALL]           = {[MOVE_POINTS_HEAL] = 2, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DEF] = 2},
    [DOME_BATTLE_STYLE_VARIED]          = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DEF] = 1},
    [DOME_BATTLE_STYLE_COMBO_HIGH]      = {[MOVE_POINTS_COMBO] = 3},
    [DOME_BATTLE_STYLE_RARE_MOVES]      = {[MOVE_POINTS_RARE] = 2},
    [DOME_BATTLE_STYLE_RARE_MOVE]       = {[MOVE_POINTS_RARE] = 1},
    [DOME_BATTLE_STYLE_HP]              = {[MOVE_POINTS_HEAL] = 3},
    [DOME_BATTLE_STYLE_STORE_POWER]     = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_HEAL] = 1},
    [DOME_BATTLE_STYLE_ENFEEBLE_LOW]    = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_STATUS] = 1},
    [DOME_BATTLE_STYLE_LUCK]            = {[MOVE_POINTS_LUCK] = 2},
    [DOME_BATTLE_STYLE_REGAL]           = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_DEF] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1},
    [DOME_BATTLE_STYLE_LOW_PP]          = {[MOVE_POINTS_LOW_PP] = 3},
    [DOME_BATTLE_STYLE_STATUS_ATK]      = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_STATUS] = 1},
    [DOME_BATTLE_STYLE_ENDURE]          = {[MOVE_POINTS_HEAL] = 2, [MOVE_POINTS_DEF] = 2},
    [DOME_BATTLE_STYLE_STATUS]          = {[MOVE_POINTS_STATUS] = 2},
    [DOME_BATTLE_STYLE_STRAIGHTFORWARD] = {[MOVE_POINTS_ACCURATE] = 3, [MOVE_POINTS_STRONG] = 3},
    [DOME_BATTLE_STYLE_AGGRESSIVE]      = {[MOVE_POINTS_STRONG] = 4},
    [DOME_BATTLE_STYLE_DEF]             = {[MOVE_POINTS_DEF] = 3},
    [DOME_BATTLE_STYLE_ENFEEBLE_HIGH]   = {[MOVE_POINTS_STAT_LOWER] = 2, [MOVE_POINTS_STATUS] = 2}, // BUG: This battle style is unobtainable; DOME_BATTLE_STYLE_ENFEEBLE_LOW will always succeed before it
    [DOME_BATTLE_STYLE_POPULAR_POWER]   = {[MOVE_POINTS_POWERFUL] = 3, [MOVE_POINTS_POPULAR] = 3},
    [DOME_BATTLE_STYLE_COMBO_LOW]       = {[MOVE_POINTS_COMBO] = 2},
    [DOME_BATTLE_STYLE_ACCURATE]        = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 3},
    [DOME_BATTLE_STYLE_POWERFUL]        = {[MOVE_POINTS_POWERFUL] = 4},
    [DOME_BATTLE_STYLE_ATK_OVER_DEF]    = {[MOVE_POINTS_DMG] = 7},
    [DOME_BATTLE_STYLE_DEF_OVER_ATK]    = {[MOVE_POINTS_DEF] = 4}, // BUG: This battle style is unobtainable; DOME_BATTLE_STYLE_DEF will always succeed before it
    [DOME_BATTLE_STYLE_POPULAR_STRONG]  = {[MOVE_POINTS_POPULAR] = 2, [MOVE_POINTS_STRONG] = 4},
    [DOME_BATTLE_STYLE_EFFECTS]         = {[MOVE_POINTS_EFFECT] = 4},
    [DOME_BATTLE_STYLE_BALANCED]        = {0}, // If no other thresholds are met, this battle style is used
    [DOME_BATTLE_STYLE_UNUSED1]         = {0}, // Here below is unreachable
    [DOME_BATTLE_STYLE_UNUSED2]         = {0},
    [DOME_BATTLE_STYLE_UNUSED3]         = {0},
  //[DOME_BATTLE_STYLE_UNUSED4]         = {0}, // Excluded here, presumably was meant to be a style just for Dome Ace Tucker
};
static const u8 sUnusedArray[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    0, 2, 253, 0, 0, 0, 0, 0, 253, 0, 0, 0, 0, 0, 253, 0,
    0, 0, 0, 0, 253, 0, 0, 0, 0, 0, 253, 254, 0, 0, 0, 0,
    0, 254, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 254, 0, 0,
    0, 0, 0, 254, 0, 0, 0, 0, 0,
};

// 1st array is for cursor position (sprite id): cursor can be on a trainer info button, a match info button, or the exit/cancel button
// 2nd array is for round count. For some reason this array contains an inaccessible Round 5 which is identical to Round 4
// 3rd array is movement direction (see the MOVE_DIR_* constants in UpdateTourneyTreeCursor)
// The values are sprite IDs for the cursor position to move to, with 0xFF being an invalid move
static const u8 sTourneyTreeCursorMovementMap[DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT + 1][DOME_ROUNDS_COUNT + 1][4]=
{
    [0]  = {{   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}},
    [1]  = {{   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}},
    [2]  = {{   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}},
    [3]  = {{   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}},
    [4]  = {{   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}},
    [5]  = {{   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}},
    [6]  = {{   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}},
    [7]  = {{   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}},
    [8]  = {{  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}},
    [9]  = {{   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}},
    [10] = {{   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}},
    [11] = {{  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}},
    [12] = {{  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}},
    [13] = {{  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}},
    [14] = {{  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}},
    [15] = {{  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}},
    [16] = {{  19,   17,    0,   20}, {  19,   17,    0,   24}, {  19,   17,    0,   24}, {  19,   17,    0,   24}, {  19,   17,    0,   24}},
    [17] = {{  16,   18,    2,   21}, {  16,   18,    2,   24}, {  16,   18,    2,   24}, {  16,   18,    2,   24}, {  16,   18,    2,   24}},
    [18] = {{  17,   19,    4,   22}, {  17,   19,    4,   25}, {  17,   19,    4,   25}, {  17,   19,    4,   25}, {  17,   19,    4,   25}},
    [19] = {{  18,   16,    6,   23}, {  18,   16,    6,   25}, {  18,   16,    6,   25}, {  18,   16,    6,   25}, {  18,   16,    6,   25}},
    [20] = {{  23,   21,   16,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}},
    [21] = {{  20,   22,   17,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}},
    [22] = {{  21,   23,   18,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}},
    [23] = {{  22,   20,   19,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}},
    [24] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  25,   25,   16,   26}, {  25,   25,   16,   28}, {  25,   25,   16,   28}, {  25,   25,   16,   28}},
    [25] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  24,   24,   18,   27}, {  24,   24,   18,   28}, {  24,   24,   18,   28}, {  24,   24,   18,   28}},
    [26] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  27,   27,   24,   20}, {  27,   27,   29,   20}, {  27,   27,   29,   20}, {  27,   27,   29,   20}},
    [27] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  26,   26,   25,   22}, {  26,   26,   29,   22}, {  26,   26,   29,   22}, {  26,   26,   29,   22}},
    [28] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   24,   29}, {0xFF, 0xFF,   24,   30}, {0xFF, 0xFF,   24,   30}},
    [29] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   28,   26}, {0xFF, 0xFF,   30,   26}, {0xFF, 0xFF,   30,   26}},
    [30] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   28,   29}, {0xFF, 0xFF,   28,   29}},
    [31] = {{  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}}, // TOURNEY_TREE_CLOSE_BUTTON
};

static const struct BgTemplate sTourneyTreeBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct BgTemplate sInfoCardBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 20,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 24,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
};

static const struct WindowTemplate sTourneyTreeWindowTemplates[] =
{
    [TOURNEYWIN_NAMES_LEFT] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 3,
        .width = 8,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 16,
    },
    [TOURNEYWIN_NAMES_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 3,
        .width = 8,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 144,
    },
    [TOURNEYWIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 1,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 272,
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sInfoCardWindowTemplates[] =
{
    [WIN_TRAINER_NAME] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 2,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WIN_TRAINER_MON1_NAME] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 53,
    },
    [WIN_TRAINER_MON2_NAME] = {
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = 7,
        .width = 9,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 69,
    },
    [WIN_TRAINER_MON3_NAME] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 96,
    },
    [WIN_TRAINER_FLAVOR_TEXT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 12,
        .width = 26,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 112,
    },
    [WIN_MATCH_NUMBER] = {
        .bg = 0,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 294,
    },
    [WIN_MATCH_TRAINER_NAME_LEFT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 340,
    },
    [WIN_MATCH_TRAINER_NAME_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 356,
    },
    [WIN_MATCH_WIN_TEXT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 372,
    },
    // Duplicate windows used by the alternate info card
    // Same as above but on bg 1 instead of bg 0
    [WIN_TRAINER_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 2,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WIN_TRAINER_MON1_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 16,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 53,
    },
    [WIN_TRAINER_MON2_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 19,
        .tilemapTop = 7,
        .width = 9,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 69,
    },
    [WIN_TRAINER_MON3_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 16,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 96,
    },
    [WIN_TRAINER_FLAVOR_TEXT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 12,
        .width = 26,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 112,
    },
    [WIN_MATCH_NUMBER + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 294,
    },
    [WIN_MATCH_TRAINER_NAME_LEFT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 340,
    },
    [WIN_MATCH_TRAINER_NAME_RIGHT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 20,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 356,
    },
    [WIN_MATCH_WIN_TEXT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 372,
    },
    #ifdef UBFIX
    DUMMY_WIN_TEMPLATE,
    #endif
};

static const struct ScanlineEffectParams sTourneyTreeScanlineEffectParams =
{
    .dmaDest = &REG_BG3CNT,
    .dmaControl = SCANLINE_EFFECT_DMACNT_16BIT,
    .initState = 1,
};

static const struct CompressedSpriteSheet sTourneyTreeButtonsSpriteSheet[] =
{
    {.data = gDomeTourneyTreeButtons_Gfx, .size = 0x0600, .tag = TAG_BUTTONS},
    {},
};

// Unused
static const struct CompressedSpritePalette sTourneyTreeButtonsSpritePal[] =
{
    {.data = gDomeTourneyTreeButtons_Pal, .tag = TAG_BUTTONS},
    {},
};

static const struct OamData sOamData_TourneyTreePokeball =
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
    .affineParam = 0,
};

// For Exit/Cancel buttons
static const struct OamData sOamData_TourneyTreeCloseButton =
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
    .priority = 0,
    .paletteNum = 1,
    .affineParam = 0,
};

static const struct OamData sOamData_VerticalScrollArrow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 2,
    .affineParam = 0,
};

static const struct OamData sOamData_HorizontalScrollArrow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 2,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_TourneyTreePokeballNormal[] =
{
    ANIMCMD_FRAME(20, 1),
    ANIMCMD_END,
};
static const union AnimCmd sSpriteAnim_TourneyTreePokeballSelected[] =
{
    ANIMCMD_FRAME(24, 1),
    ANIMCMD_END,
};

static const union AnimCmd * const sSpriteAnimTable_TourneyTreePokeball[] =
{
    sSpriteAnim_TourneyTreePokeballNormal,
    sSpriteAnim_TourneyTreePokeballSelected,
};

// Sprite template for the pokeballs on the tourney tree that act as buttons to view a trainer/match info card
static const struct SpriteTemplate sTourneyTreePokeballSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreePokeball,
    .anims = sSpriteAnimTable_TourneyTreePokeball,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const union AnimCmd sSpriteAnim_TourneyTreeCancelButtonNormal[] =
{
    ANIMCMD_FRAME(8, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_TourneyTreeCancelButtonSelected[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd * const sSpriteAnimTable_TourneyTreeCancelButton[] =
{
    sSpriteAnim_TourneyTreeCancelButtonNormal,
    sSpriteAnim_TourneyTreeCancelButtonSelected,
};

static const struct SpriteTemplate sCancelButtonSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreeCloseButton,
    .anims = sSpriteAnimTable_TourneyTreeCancelButton,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const union AnimCmd sSpriteAnim_TourneyTreeExitButtonNormal[] =
{
    ANIMCMD_FRAME(40, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_TourneyTreeExitButtonSelected[] =
{
    ANIMCMD_FRAME(32, 1),
    ANIMCMD_END,
 };

static const union AnimCmd * const sSpriteAnimTable_TourneyTreeExitButton[] =
{
    sSpriteAnim_TourneyTreeExitButtonNormal,
    sSpriteAnim_TourneyTreeExitButtonSelected,
};

static const struct SpriteTemplate sExitButtonSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreeCloseButton,
    .anims = sSpriteAnimTable_TourneyTreeExitButton,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const union AnimCmd sSpriteAnim_UpArrow[] =
{
    ANIMCMD_FRAME(18, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_DownArrow[] =
{
    ANIMCMD_FRAME(18, 1, .vFlip = TRUE),
    ANIMCMD_END,
 };

static const union AnimCmd sSpriteAnim_LeftArrow[] =
{
    ANIMCMD_FRAME(16, 1, .hFlip = TRUE),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_RightArrow[] =
{
    ANIMCMD_FRAME(16, 1),
    ANIMCMD_END,
};

static const union AnimCmd * const sSpriteAnimTable_VerticalScrollArrow[] =
{
    sSpriteAnim_UpArrow,
    sSpriteAnim_DownArrow,
};

static const union AnimCmd * const sSpriteAnimTable_HorizontalScrollArrow[] =
{
    sSpriteAnim_LeftArrow,
    sSpriteAnim_RightArrow,
};

static const struct SpriteTemplate sHorizontalScrollArrowSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_HorizontalScrollArrow,
    .anims = sSpriteAnimTable_HorizontalScrollArrow,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_HorizontalScrollArrow
};

static const struct SpriteTemplate sVerticalScrollArrowSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_VerticalScrollArrow,
    .anims = sSpriteAnimTable_VerticalScrollArrow,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_VerticalScrollArrow
};

// Organized by seed starting position, i.e. seed 0 battles seed 8 first
static const u8 sTourneyTreeTrainerIds[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 8, 12, 4, 7, 15, 11, 3, 2, 10, 14, 6, 5, 13, 9, 1};

static void (* const sBattleDomeFunctions[])(void) =
{
    [BATTLE_DOME_FUNC_INIT]                     = InitDomeChallenge,
    [BATTLE_DOME_FUNC_GET_DATA]                 = GetDomeData,
    [BATTLE_DOME_FUNC_SET_DATA]                 = SetDomeData,
    [BATTLE_DOME_FUNC_GET_ROUND_TEXT]           = BufferDomeRoundText,
    [BATTLE_DOME_FUNC_GET_OPPONENT_NAME]        = BufferDomeOpponentName,
    [BATTLE_DOME_FUNC_INIT_OPPONENT_PARTY]      = InitDomeOpponentParty,
    [BATTLE_DOME_FUNC_SHOW_OPPONENT_INFO]       = ShowDomeOpponentInfo,
    [BATTLE_DOME_FUNC_SHOW_TOURNEY_TREE]        = ShowDomeTourneyTree,
    [BATTLE_DOME_FUNC_SHOW_PREV_TOURNEY_TREE]   = ShowPreviousDomeTourneyTree,
    [BATTLE_DOME_FUNC_SET_OPPONENT_ID]          = SetDomeOpponentId,
    [BATTLE_DOME_FUNC_SET_OPPONENT_GFX]         = SetDomeOpponentGraphicsId,
    [BATTLE_DOME_FUNC_SHOW_STATIC_TOURNEY_TREE] = ShowNonInteractiveDomeTourneyTree,
    [BATTLE_DOME_FUNC_RESOLVE_WINNERS]          = ResolveDomeRoundWinners,
    [BATTLE_DOME_FUNC_SAVE]                     = SaveDomeChallenge,
    [BATTLE_DOME_FUNC_INCREMENT_STREAK]         = IncrementDomeStreaks,
    [BATTLE_DOME_FUNC_SET_TRAINERS]             = SetFacilityTrainerAndMonPtrs,
    [BATTLE_DOME_FUNC_RESET_SKETCH]             = ResetSketchedMoves,
    [BATTLE_DOME_FUNC_RESTORE_HELD_ITEMS]       = RestoreDomePlayerPartyHeldItems,
    [BATTLE_DOME_FUNC_REDUCE_PARTY]             = ReduceDomePlayerPartyToSelectedMons,
    [BATTLE_DOME_FUNC_COMPARE_SEEDS]            = GetPlayerSeededBeforeOpponent,
    [BATTLE_DOME_FUNC_GET_WINNER_NAME]          = BufferLastDomeWinnerName,
    [BATTLE_DOME_FUNC_INIT_RESULTS_TREE]        = InitRandomTourneyTreeResults,
    [BATTLE_DOME_FUNC_INIT_TRAINERS]            = InitDomeTrainers,
};

static const u32 sWinStreakFlags[][2] =
{
    {STREAK_DOME_SINGLES_50, STREAK_DOME_SINGLES_OPEN},
    {STREAK_DOME_DOUBLES_50, STREAK_DOME_DOUBLES_OPEN},
};

static const u32 sWinStreakMasks[][2] =
{
    {~(STREAK_DOME_SINGLES_50), ~(STREAK_DOME_SINGLES_OPEN)},
    {~(STREAK_DOME_DOUBLES_50), ~(STREAK_DOME_DOUBLES_OPEN)},
};

// TODO: The below two arrays probably need better names. The one below for example is only true of sIdToOpponentId[i][0]
static const u8 sIdToOpponentId[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    [0]  = { 8,  0,  4,  8},
    [1]  = { 9, 12,  8,  0},
    [2]  = {10,  8, 12,  0},
    [3]  = {11,  4,  0,  8},
    [4]  = {12,  0,  4,  8},
    [5]  = {13, 12,  8,  0},
    [6]  = {14,  8, 12,  0},
    [7]  = {15,  4,  0,  8},
    [8]  = { 0,  0,  4,  8},
    [9]  = { 1, 12,  8,  0},
    [10] = { 2,  8, 12,  0},
    [11] = { 3,  4,  0,  8},
    [12] = { 4,  0,  4,  8},
    [13] = { 5, 12,  8,  0},
    [14] = { 6,  8, 12,  0},
    [15] = { 7,  4,  0,  8},
};

// sTourneyTreeTrainerIds with every other pair swapped
static const u8 sTourneyTreeTrainerOpponentIds[DOME_TOURNAMENT_TRAINERS_COUNT] = { 0, 8, 4, 12, 7, 15, 3, 11, 2, 10, 6, 14, 5, 13, 1, 9 };

// The match number - 1 that a given tournament trainer will participate in for a given round
static const u8 sIdToMatchNumber[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    { 0,  8, 12, 14},
    { 0,  8, 12, 14},
    { 1,  8, 12, 14},
    { 1,  8, 12, 14},
    { 2,  9, 12, 14},
    { 2,  9, 12, 14},
    { 3,  9, 12, 14},
    { 3,  9, 12, 14},
    { 4, 10, 13, 14},
    { 4, 10, 13, 14},
    { 5, 10, 13, 14},
    { 5, 10, 13, 14},
    { 6, 11, 13, 14},
    { 6, 11, 13, 14},
    { 7, 11, 13, 14},
    { 7, 11, 13, 14},
};

static const u8 sLastMatchCardNum[DOME_ROUNDS_COUNT] =
{
    [DOME_ROUND1]    = 23,
    [DOME_ROUND2]    = 27,
    [DOME_SEMIFINAL] = 29,
    [DOME_FINAL]     = 30
};

static const u8 sTrainerAndRoundToLastMatchCardNum[DOME_TOURNAMENT_TRAINERS_COUNT / 2][DOME_ROUNDS_COUNT] =
{
    {16, 24, 28, 30},
    {17, 24, 28, 30},
    {18, 25, 28, 30},
    {19, 25, 28, 30},
    {20, 26, 29, 30},
    {21, 26, 29, 30},
    {22, 27, 29, 30},
    {23, 27, 29, 30},
};

static const u8 sTournamentIdToPairedTrainerIds[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 15, 8, 7, 3, 12, 11, 4, 1, 14, 9, 6, 2, 13, 10, 5};

// The first line of text on a trainers info card. It describes their potential to win, based on their seed in the tournament tree.
// Dome Ace Tucker has their own separate potential text.
static const u8 *const sBattleDomePotentialTexts[DOME_TOURNAMENT_TRAINERS_COUNT + 1] =
{
    BattleDome_Text_Potential1, // Highest potential
    BattleDome_Text_Potential2,
    BattleDome_Text_Potential3,
    BattleDome_Text_Potential4,
    BattleDome_Text_Potential5,
    BattleDome_Text_Potential6,
    BattleDome_Text_Potential7,
    BattleDome_Text_Potential8,
    BattleDome_Text_Potential9,
    BattleDome_Text_Potential10,
    BattleDome_Text_Potential11,
    BattleDome_Text_Potential12,
    BattleDome_Text_Potential13,
    BattleDome_Text_Potential14,
    BattleDome_Text_Potential15,
    BattleDome_Text_Potential16, // Lowest potential
    BattleDome_Text_PotentialDomeAceTucker,
};

// The second line of text on a trainers info card. It gives information about their battle style (dependent on their party's moves).
static const u8 *const sBattleDomeOpponentStyleTexts[NUM_BATTLE_STYLES] =
{
    [DOME_BATTLE_STYLE_RISKY]           = BattleDome_Text_StyleRiskDisaster,
    [DOME_BATTLE_STYLE_STALL]           = BattleDome_Text_StyleEndureLongBattles,
    [DOME_BATTLE_STYLE_VARIED]          = BattleDome_Text_StyleVariesTactics,
    [DOME_BATTLE_STYLE_COMBO_HIGH]      = BattleDome_Text_StyleToughWinningPattern,
    [DOME_BATTLE_STYLE_RARE_MOVES]      = BattleDome_Text_StyleUsesVeryRareMove,   // Seems like the text for these two was swapped
    [DOME_BATTLE_STYLE_RARE_MOVE]       = BattleDome_Text_StyleUsesStartlingMoves, //
    [DOME_BATTLE_STYLE_HP]              = BattleDome_Text_StyleConstantlyWatchesHP,
    [DOME_BATTLE_STYLE_STORE_POWER]     = BattleDome_Text_StyleStoresAndLoosesPower,
    [DOME_BATTLE_STYLE_ENFEEBLE_LOW]    = BattleDome_Text_StyleEnfeeblesFoes,
    [DOME_BATTLE_STYLE_LUCK]            = BattleDome_Text_StylePrefersLuckTactics,
    [DOME_BATTLE_STYLE_REGAL]           = BattleDome_Text_StyleRegalAtmosphere,
    [DOME_BATTLE_STYLE_LOW_PP]          = BattleDome_Text_StylePowerfulLowPPMoves,
    [DOME_BATTLE_STYLE_STATUS_ATK]      = BattleDome_Text_StyleEnfeebleThenAttack,
    [DOME_BATTLE_STYLE_ENDURE]          = BattleDome_Text_StyleBattlesWhileEnduring,
    [DOME_BATTLE_STYLE_STATUS]          = BattleDome_Text_StyleUpsetsFoesEmotionally,
    [DOME_BATTLE_STYLE_STRAIGHTFORWARD] = BattleDome_Text_StyleStrongAndStraightforward,
    [DOME_BATTLE_STYLE_AGGRESSIVE]      = BattleDome_Text_StyleAggressivelyStrongMoves,
    [DOME_BATTLE_STYLE_DEF]             = BattleDome_Text_StyleCleverlyDodgesAttacks,
    [DOME_BATTLE_STYLE_ENFEEBLE_HIGH]   = BattleDome_Text_StyleUsesUpsettingMoves,
    [DOME_BATTLE_STYLE_POPULAR_POWER]   = BattleDome_Text_StyleUsesPopularMoves,
    [DOME_BATTLE_STYLE_COMBO_LOW]       = BattleDome_Text_StyleHasPowerfulComboMoves,
    [DOME_BATTLE_STYLE_ACCURATE]        = BattleDome_Text_StyleUsesHighProbabilityMoves,
    [DOME_BATTLE_STYLE_POWERFUL]        = BattleDome_Text_StyleAggressivelySpectacularMoves,
    [DOME_BATTLE_STYLE_ATK_OVER_DEF]    = BattleDome_Text_StyleEmphasizesOffenseOverDefense,
    [DOME_BATTLE_STYLE_DEF_OVER_ATK]    = BattleDome_Text_StyleEmphasizesDefenseOverOffense,
    [DOME_BATTLE_STYLE_POPULAR_STRONG]  = BattleDome_Text_StyleAttacksQuicklyStrongMoves,
    [DOME_BATTLE_STYLE_EFFECTS]         = BattleDome_Text_StyleUsesAddedEffectMoves,
    [DOME_BATTLE_STYLE_BALANCED]        = BattleDome_Text_StyleUsesBalancedMixOfMoves,
    [DOME_BATTLE_STYLE_UNUSED1]         = BattleDome_Text_StyleSampleMessage1,
    [DOME_BATTLE_STYLE_UNUSED2]         = BattleDome_Text_StyleSampleMessage2,
    [DOME_BATTLE_STYLE_UNUSED3]         = BattleDome_Text_StyleSampleMessage3,
    [DOME_BATTLE_STYLE_UNUSED4]         = BattleDome_Text_StyleSampleMessage4,
};

// The third line of text on a trainers info card. It that gives information about their party's stat spread (based on their Pokémon's effort values and Nature).
static const u8 *const sBattleDomeOpponentStatsTexts[] =
{
    BattleDome_Text_EmphasizesHPAndAtk,      // DOME_TEXT_TWO_GOOD_STATS and DOME_TEXT_HP start here
    BattleDome_Text_EmphasizesHPAndDef,
    BattleDome_Text_EmphasizesHPAndSpeed,
    BattleDome_Text_EmphasizesHPAndSpAtk,
    BattleDome_Text_EmphasizesHPAndSpDef,
    BattleDome_Text_EmphasizesAtkAndDef,     // DOME_TEXT_ATK starts here
    BattleDome_Text_EmphasizesAtkAndSpeed,
    BattleDome_Text_EmphasizesAtkAndSpAtk,
    BattleDome_Text_EmphasizesAtkAndSpDef,
    BattleDome_Text_EmphasizesDefAndSpeed,   // DOME_TEXT_DEF starts here
    BattleDome_Text_EmphasizesDefAndSpAtk,
    BattleDome_Text_EmphasizesDefAndSpDef,
    BattleDome_Text_EmphasizesSpeedAndSpAtk, // DOME_TEXT_SPEED starts here
    BattleDome_Text_EmphasizesSpeedAndSpDef,
    BattleDome_Text_EmphasizesSpAtkAndSpDef, // DOME_TEXT_SPATK starts here
    BattleDome_Text_EmphasizesHP,            // DOME_TEXT_ONE_GOOD_STAT starts here
    BattleDome_Text_EmphasizesAtk,
    BattleDome_Text_EmphasizesDef,
    BattleDome_Text_EmphasizesSpeed,
    BattleDome_Text_EmphasizesSpAtk,
    BattleDome_Text_EmphasizesSpDef,
    BattleDome_Text_NeglectsHPAndAtk,        // DOME_TEXT_TWO_BAD_STATS starts here
    BattleDome_Text_NeglectsHPAndDef,
    BattleDome_Text_NeglectsHPAndSpeed,
    BattleDome_Text_NeglectsHPAndSpAtk,
    BattleDome_Text_NeglectsHPAndSpDef,
    BattleDome_Text_NeglectsAtkAndDef,
    BattleDome_Text_NeglectsAtkAndSpeed,
    BattleDome_Text_NeglectsAtkAndSpAtk,
    BattleDome_Text_NeglectsAtkAndSpDef,
    BattleDome_Text_NeglectsDefAndSpeed,
    BattleDome_Text_NeglectsDefAndSpAtk,
    BattleDome_Text_NeglectsDefAndSpDef,
    BattleDome_Text_NeglectsSpeedAndSpAtk,
    BattleDome_Text_NeglectsSpeedAndSpDef,
    BattleDome_Text_NeglectsSpAtkAndSpDef,
    BattleDome_Text_NeglectsHP,              // DOME_TEXT_ONE_BAD_STAT starts here
    BattleDome_Text_NeglectsAtk,
    BattleDome_Text_NeglectsDef,
    BattleDome_Text_NeglectsSpeed,
    BattleDome_Text_NeglectsSpAtk,
    BattleDome_Text_NeglectsSpDef,
    [DOME_TEXT_WELL_BALANCED] = BattleDome_Text_RaisesMonsWellBalanced,
};

static const u8 sInfoTrainerMonX[FRONTIER_PARTY_SIZE] = {104, 136, 104};
static const u8 sInfoTrainerMonY[FRONTIER_PARTY_SIZE] = { 38,  62,  78};
static const u8 sSpeciesNameTextYCoords[] = {0, 4, 0};

// Offsets within sBattleDomeOpponentStatsTexts for stat combinations
// SPDEF has no offset because by then all stat combinations have been reached, so it has no combination texts
static const u8 sStatTextOffsets[NUM_STATS - 1] =
{
    DOME_TEXT_HP,
    DOME_TEXT_ATK,
    DOME_TEXT_DEF,
    DOME_TEXT_SPEED,
    DOME_TEXT_SPATK
};

static const u8 *const sBattleDomeMatchNumberTexts[DOME_TOURNAMENT_MATCHES_COUNT] =
{
    BattleDome_Text_Round1Match1,
    BattleDome_Text_Round1Match2,
    BattleDome_Text_Round1Match3,
    BattleDome_Text_Round1Match4,
    BattleDome_Text_Round1Match5,
    BattleDome_Text_Round1Match6,
    BattleDome_Text_Round1Match7,
    BattleDome_Text_Round1Match8,
    BattleDome_Text_Round2Match1,
    BattleDome_Text_Round2Match2,
    BattleDome_Text_Round2Match3,
    BattleDome_Text_Round2Match4,
    BattleDome_Text_SemifinalMatch1,
    BattleDome_Text_SemifinalMatch2,
    BattleDome_Text_FinalMatch,
};

static const u8 *const sBattleDomeWinTexts[] =
{
    [DOME_TEXT_NO_WINNER_YET]    = BattleDome_Text_LetTheBattleBegin,
    [DOME_TEXT_WON_USING_MOVE]   = BattleDome_Text_TrainerWonUsingMove,
    [DOME_TEXT_CHAMP_USING_MOVE] = BattleDome_Text_TrainerBecameChamp,
    [DOME_TEXT_WON_ON_FORFEIT]   = BattleDome_Text_TrainerWonByDefault,
    [DOME_TEXT_CHAMP_ON_FORFEIT] = BattleDome_Text_TrainerWonOutrightByDefault,
    [DOME_TEXT_WON_NO_MOVES]     = BattleDome_Text_TrainerWonNoMoves,
    [DOME_TEXT_CHAMP_NO_MOVES]   = BattleDome_Text_TrainerWonOutrightNoMoves,
};

static const u8 sLeftTrainerMonX[FRONTIER_PARTY_SIZE]  = { 96,  96,  96};
static const u8 sLeftTrainerMonY[FRONTIER_PARTY_SIZE]  = { 56,  80, 104};
static const u8 sRightTrainerMonX[FRONTIER_PARTY_SIZE] = {144, 144, 144};
static const u8 sRightTrainerMonY[FRONTIER_PARTY_SIZE] = { 56,  80, 104};

// Duplicate of sTourneyTreeTrainerIds
static const u8 sTourneyTreeTrainerIds2[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 8, 12, 4, 7, 15, 11, 3, 2, 10, 14, 6, 5, 13, 9, 1};

// The number of possible trainers that could be competing in a given match
#define NUM_POSSIBLE_MATCH_TRAINERS(round) (DOME_TOURNAMENT_TRAINERS_COUNT / (1 << (DOME_ROUNDS_COUNT - round - 1)))

// The range of tournament trainers to check as possible participants in a given match
// Given by the offset in sCompetitorRangeByMatch[][0], the number of trainers in sCompetitorRangeByMatch[][1], and the round
static const u8 sCompetitorRangeByMatch[DOME_TOURNAMENT_MATCHES_COUNT][3] =
{
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 0,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 1,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 2,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 3,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 4,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 5,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 6,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 7,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 0,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 1,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 2,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 3,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL) * 0, NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL), DOME_SEMIFINAL},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL) * 1, NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL), DOME_SEMIFINAL},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_FINAL) * 0,     NUM_POSSIBLE_MATCH_TRAINERS(DOME_FINAL),     DOME_FINAL},
};

#define NAME_ROW_HEIGHT 16

// 1st value is the windowId, 2nd value is the y coord
static const u8 sTrainerNamePositions[DOME_TOURNAMENT_TRAINERS_COUNT][2] =
{
    { TOURNEYWIN_NAMES_LEFT,  0 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 7 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 0 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  7 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  3 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 4 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 3 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  4 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  1 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 6 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 1 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  6 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  2 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 5 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 2 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  5 * NAME_ROW_HEIGHT},
};

// Coords for the pokeballs on the tourney tree that act as buttons to view trainer/match info
static const u8 sTourneyTreePokeballCoords[DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT][2] =
{
    { 68,  33}, // Left side trainers
    { 68,  49},
    { 68,  65},
    { 68,  81},
    { 68,  97},
    { 68, 113},
    { 68, 129},
    { 68, 145},
    {172,  33}, // Right side trainers
    {172,  49},
    {172,  65},
    {172,  81},
    {172,  97},
    {172, 113},
    {172, 129},
    {172, 145},
    { 87,  41}, // Left side Round 1 matches
    { 87,  73},
    { 87, 105},
    { 87, 137},
    {153,  41}, // Right side Round 1 matches
    {153,  73},
    {153, 105},
    {153, 137},
    { 95,  57}, // Left side Round 2 matches
    { 95, 121},
    {145,  57}, // Right side Round 2 matches
    {145, 121},
    {103,  89}, // Left side semifinal match
    {137,  89}, // Right side semifinal match
    {120,  89}, // Final match
};

// Tile values from tourney_tree.png for the highlighted lines of the tourney tree.
// These tiles will be used to replace the existing, unhighlighted line tiles on the tourney tree tilemap.
#define LINE_PAL           (6 << 12)
#define LINE_H             (LINE_PAL | 0x21) // Horizontal
#define LINE_CORNER_R      (LINE_PAL | 0x23) // Horizontal into a right-side vertical
#define LINE_CORNER_L      (LINE_PAL | 0x25) // Horizontal into a left-side vertical
#define LINE_V_R           (LINE_PAL | 0x27) // Right-side vertical
#define LINE_V_L           (LINE_PAL | 0x29) // Left-side vertical
#define LINE_H_BOTTOM      (LINE_PAL | 0x2B) // Horizontal on the bottom of the tree
#define LINE_H_LOGO1       (LINE_PAL | 0x2C) // Horizontal, logo behind
#define LINE_H_LOGO2       (LINE_PAL | 0x2D) // Horizontal, logo behind
#define LINE_H_LOGO3       (LINE_PAL | 0x2E) // Horizontal, logo behind
#define LINE_H_LOGO4       (LINE_PAL | 0x2F) // Horizontal, logo behind
#define LINE_V_R_LOGO1     (LINE_PAL | 0x30) // Right-side vertical, logo behind
#define LINE_V_R_LOGO2     (LINE_PAL | 0x31) // Right-side vertical, logo behind
#define LINE_V_R_LOGO3     (LINE_PAL | 0x32) // Right-side vertical, logo behind
#define LINE_V_R_LOGO4     (LINE_PAL | 0x33) // Right-side vertical, logo behind
#define LINE_V_L_LOGO1     (LINE_PAL | 0x35) // Left-side vertical, logo behind
#define LINE_V_L_LOGO2     (LINE_PAL | 0x36) // Left-side vertical, logo behind
#define LINE_V_L_LOGO3     (LINE_PAL | 0x37) // Left-side vertical, logo behind
#define LINE_V_L_LOGO4     (LINE_PAL | 0x38) // Left-side vertical, logo behind
#define LINE_V_R_HALF_LOGO (LINE_PAL | 0x3B) // Right-side vertical, half lit from the top, logo behind
#define LINE_V_L_HALF_LOGO (LINE_PAL | 0x3C) // Left-side vertical, half lit from the top, logo behind
#define LINE_CORNER_R_HALF (LINE_PAL | 0x43) // Lit horizontal, unlit right-side vertical
#define LINE_CORNER_L_HALF (LINE_PAL | 0x45) // Lit horizontal, unlit left-side vertical
#define LINE_V_R_HALF      (LINE_PAL | 0x47) // Right-side vertical, half lit from the top
#define LINE_V_L_HALF      (LINE_PAL | 0x49) // Left-side vertical, half lit from the top

// Each of these line sections define the position of the advancement line on the tourney tree for the victor of that round
// The trainers here are numbered by tourney ID (rank/seed) and ordered according to where they start on the tourney tree
#define LINESECTION_ROUND1_TRAINER1(lastTile) \
    {.tile = LINE_H,        .y =  4, .x =  9}, \
    {.tile = LINE_CORNER_R, .y =  4, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y =  5, .x = 10}, \
    {.tile = lastTile,      .y =  5, .x = 11},

#define LINESECTION_ROUND1_TRAINER9(lastTile) \
    {.tile = LINE_H,   .y =  6, .x =  9}, \
    {.tile = LINE_H,   .y =  6, .x = 10}, \
    {.tile = LINE_V_R, .y =  5, .x = 10}, \
    {.tile = lastTile, .y =  5, .x = 11},

#define LINESECTION_ROUND1_TRAINER13(lastTile) \
    {.tile = LINE_H,        .y =  8, .x =  9}, \
    {.tile = LINE_CORNER_R, .y =  8, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y =  9, .x = 10}, \
    {.tile = lastTile,      .y =  9, .x = 11},

#define LINESECTION_ROUND1_TRAINER5(lastTile) \
    {.tile = LINE_H,   .y = 10, .x =  9}, \
    {.tile = LINE_H,   .y = 10, .x = 10}, \
    {.tile = LINE_V_R, .y =  9, .x = 10}, \
    {.tile = lastTile, .y =  9, .x = 11},

#define LINESECTION_ROUND1_TRAINER8(lastTile) \
    {.tile = LINE_H,        .y = 12, .x =  9}, \
    {.tile = LINE_CORNER_R, .y = 12, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y = 13, .x = 10}, \
    {.tile = lastTile,      .y = 13, .x = 11},

#define LINESECTION_ROUND1_TRAINER16(lastTile) \
    {.tile = LINE_H,   .y = 14, .x =  9}, \
    {.tile = LINE_H,   .y = 14, .x = 10}, \
    {.tile = LINE_V_R, .y = 13, .x = 10}, \
    {.tile = lastTile, .y = 13, .x = 11},

#define LINESECTION_ROUND1_TRAINER12(lastTile) \
    {.tile = LINE_H,        .y = 16, .x =  9}, \
    {.tile = LINE_CORNER_R, .y = 16, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y = 17, .x = 10}, \
    {.tile = lastTile,      .y = 17, .x = 11},

#define LINESECTION_ROUND1_TRAINER4(lastTile) \
    {.tile = LINE_H_BOTTOM, .y = 18, .x =  9}, \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 10}, \
    {.tile = LINE_V_R,      .y = 17, .x = 10}, \
    {.tile = lastTile,      .y = 17, .x = 11},

#define LINESECTION_ROUND1_TRAINER3(lastTile) \
    {.tile = LINE_H,        .y =  4, .x = 20}, \
    {.tile = LINE_CORNER_L, .y =  4, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y =  5, .x = 19}, \
    {.tile = lastTile,      .y =  5, .x = 18},

#define LINESECTION_ROUND1_TRAINER11(lastTile) \
    {.tile = LINE_H,   .y =  6, .x = 20}, \
    {.tile = LINE_H,   .y =  6, .x = 19}, \
    {.tile = LINE_V_L, .y =  5, .x = 19}, \
    {.tile = lastTile, .y =  5, .x = 18},

#define LINESECTION_ROUND1_TRAINER15(lastTile) \
    {.tile = LINE_H,        .y =  8, .x = 20}, \
    {.tile = LINE_CORNER_L, .y =  8, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y =  9, .x = 19}, \
    {.tile = lastTile,      .y =  9, .x = 18},

#define LINESECTION_ROUND1_TRAINER7(lastTile) \
    {.tile = LINE_H,   .y = 10, .x = 20}, \
    {.tile = LINE_H,   .y = 10, .x = 19}, \
    {.tile = LINE_V_L, .y =  9, .x = 19}, \
    {.tile = lastTile, .y =  9, .x = 18},

#define LINESECTION_ROUND1_TRAINER6(lastTile) \
    {.tile = LINE_H,        .y = 12, .x = 20}, \
    {.tile = LINE_CORNER_L, .y = 12, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y = 13, .x = 19}, \
    {.tile = lastTile,      .y = 13, .x = 18},

#define LINESECTION_ROUND1_TRAINER14(lastTile) \
    {.tile = LINE_H,   .y = 14, .x = 20}, \
    {.tile = LINE_H,   .y = 14, .x = 19}, \
    {.tile = LINE_V_L, .y = 13, .x = 19}, \
    {.tile = lastTile, .y = 13, .x = 18},

#define LINESECTION_ROUND1_TRAINER10(lastTile) \
    {.tile = LINE_H,        .y = 16, .x = 20}, \
    {.tile = LINE_CORNER_L, .y = 16, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y = 17, .x = 19}, \
    {.tile = lastTile,      .y = 17, .x = 18},

#define LINESECTION_ROUND1_TRAINER2(lastTile) \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 20}, \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 19}, \
    {.tile = LINE_V_L,      .y = 17, .x = 19}, \
    {.tile = lastTile,      .y = 17, .x = 18},

#define LINESECTION_ROUND2_MATCH1(lastTile) \
    {.tile = LINE_V_R,      .y =  6, .x = 11}, \
    {.tile = LINE_V_R_HALF, .y =  7, .x = 11}, \
    {.tile = lastTile,      .y =  7, .x = 12},

#define LINESECTION_ROUND2_MATCH2(lastTile) \
    {.tile = LINE_V_R, .y =  8, .x = 11}, \
    {.tile = LINE_V_R, .y =  7, .x = 11}, \
    {.tile = lastTile, .y =  7, .x = 12},

#define LINESECTION_ROUND2_MATCH3(lastTile) \
    {.tile = LINE_V_R,      .y = 14, .x = 11}, \
    {.tile = LINE_V_R_HALF, .y = 15, .x = 11}, \
    {.tile = lastTile,      .y = 15, .x = 12},

#define LINESECTION_ROUND2_MATCH4(lastTile) \
    {.tile = LINE_V_R, .y = 16, .x = 11}, \
    {.tile = LINE_V_R, .y = 15, .x = 11}, \
    {.tile = lastTile, .y = 15, .x = 12},

#define LINESECTION_ROUND2_MATCH5(lastTile) \
    {.tile = LINE_V_L,      .y =  6, .x = 18}, \
    {.tile = LINE_V_L_HALF, .y =  7, .x = 18}, \
    {.tile = lastTile,      .y =  7, .x = 17},

#define LINESECTION_ROUND2_MATCH6(lastTile) \
    {.tile = LINE_V_L, .y =  8, .x = 18}, \
    {.tile = LINE_V_L, .y =  7, .x = 18}, \
    {.tile = lastTile, .y =  7, .x = 17},

#define LINESECTION_ROUND2_MATCH7(lastTile) \
    {.tile = LINE_V_L,      .y = 14, .x = 18}, \
    {.tile = LINE_V_L_HALF, .y = 15, .x = 18}, \
    {.tile = lastTile,      .y = 15, .x = 17},

#define LINESECTION_ROUND2_MATCH8(lastTile) \
    {.tile = LINE_V_L, .y = 16, .x = 18}, \
    {.tile = LINE_V_L, .y = 15, .x = 18}, \
    {.tile = lastTile, .y = 15, .x = 17},

#define LINESECTION_SEMIFINAL_TOP_LEFT \
    {.tile = LINE_V_R,           .y =  8, .x = 12}, \
    {.tile = LINE_V_R,           .y =  9, .x = 12}, \
    {.tile = LINE_V_R,           .y = 10, .x = 12}, \
    {.tile = LINE_V_R_HALF_LOGO, .y = 11, .x = 12},

#define LINESECTION_SEMIFINAL_BOTTOM_LEFT \
    {.tile = LINE_V_R_LOGO4, .y = 14, .x = 12}, \
    {.tile = LINE_V_R_LOGO3, .y = 13, .x = 12}, \
    {.tile = LINE_V_R_LOGO2, .y = 12, .x = 12}, \
    {.tile = LINE_V_R_LOGO1, .y = 11, .x = 12},

#define LINESECTION_SEMIFINAL_TOP_RIGHT \
    {.tile = LINE_V_L,           .y =  8, .x = 17}, \
    {.tile = LINE_V_L,           .y =  9, .x = 17}, \
    {.tile = LINE_V_L,           .y = 10, .x = 17}, \
    {.tile = LINE_V_L_HALF_LOGO, .y = 11, .x = 17},

#define LINESECTION_SEMIFINAL_BOTTOM_RIGHT \
    {.tile = LINE_V_L_LOGO4, .y = 14, .x = 17}, \
    {.tile = LINE_V_L_LOGO3, .y = 13, .x = 17}, \
    {.tile = LINE_V_L_LOGO2, .y = 12, .x = 17}, \
    {.tile = LINE_V_L_LOGO1, .y = 11, .x = 17},

#define LINESECTION_FINAL_LEFT \
    {.tile = LINE_H_LOGO1, .y = 11, .x = 13}, \
    {.tile = LINE_H_LOGO2, .y = 11, .x = 14},

#define LINESECTION_FINAL_RIGHT \
    {.tile = LINE_H_LOGO4, .y = 11, .x = 16}, \
    {.tile = LINE_H_LOGO3, .y = 11, .x = 15},


static const struct TourneyTreeLineSection sLineSectionTrainer1Round1[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Round2[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Final[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Round1[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Round2[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Final[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Round1[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Round2[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Final[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Round1[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Round2[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Final[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Round1[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Round2[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Final[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Round1[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Round2[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Final[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Round1[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Round2[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Final[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Round1[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Round2[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Final[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Round1[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Round2[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Final[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Round1[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Round2[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Final[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Round1[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Round2[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Final[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Round1[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Round2[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Final[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Round1[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Round2[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Final[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Round1[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Round2[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Final[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Round1[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Round2[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Final[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Round1[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Round2[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Final[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection *const sTourneyTreeLineSections[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    {sLineSectionTrainer1Round1,  sLineSectionTrainer1Round2,  sLineSectionTrainer1Semifinal,  sLineSectionTrainer1Final},
    {sLineSectionTrainer2Round1,  sLineSectionTrainer2Round2,  sLineSectionTrainer2Semifinal,  sLineSectionTrainer2Final},
    {sLineSectionTrainer3Round1,  sLineSectionTrainer3Round2,  sLineSectionTrainer3Semifinal,  sLineSectionTrainer3Final},
    {sLineSectionTrainer4Round1,  sLineSectionTrainer4Round2,  sLineSectionTrainer4Semifinal,  sLineSectionTrainer4Final},
    {sLineSectionTrainer5Round1,  sLineSectionTrainer5Round2,  sLineSectionTrainer5Semifinal,  sLineSectionTrainer5Final},
    {sLineSectionTrainer6Round1,  sLineSectionTrainer6Round2,  sLineSectionTrainer6Semifinal,  sLineSectionTrainer6Final},
    {sLineSectionTrainer7Round1,  sLineSectionTrainer7Round2,  sLineSectionTrainer7Semifinal,  sLineSectionTrainer7Final},
    {sLineSectionTrainer8Round1,  sLineSectionTrainer8Round2,  sLineSectionTrainer8Semifinal,  sLineSectionTrainer8Final},
    {sLineSectionTrainer9Round1,  sLineSectionTrainer9Round2,  sLineSectionTrainer9Semifinal,  sLineSectionTrainer9Final},
    {sLineSectionTrainer10Round1, sLineSectionTrainer10Round2, sLineSectionTrainer10Semifinal, sLineSectionTrainer10Final},
    {sLineSectionTrainer11Round1, sLineSectionTrainer11Round2, sLineSectionTrainer11Semifinal, sLineSectionTrainer11Final},
    {sLineSectionTrainer12Round1, sLineSectionTrainer12Round2, sLineSectionTrainer12Semifinal, sLineSectionTrainer12Final},
    {sLineSectionTrainer13Round1, sLineSectionTrainer13Round2, sLineSectionTrainer13Semifinal, sLineSectionTrainer13Final},
    {sLineSectionTrainer14Round1, sLineSectionTrainer14Round2, sLineSectionTrainer14Semifinal, sLineSectionTrainer14Final},
    {sLineSectionTrainer15Round1, sLineSectionTrainer15Round2, sLineSectionTrainer15Semifinal, sLineSectionTrainer15Final},
    {sLineSectionTrainer16Round1, sLineSectionTrainer16Round2, sLineSectionTrainer16Semifinal, sLineSectionTrainer16Final},
};

static const u8 sTourneyTreeLineSectionArrayCounts[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    {ARRAY_COUNT(sLineSectionTrainer1Round1),  ARRAY_COUNT(sLineSectionTrainer1Round2),  ARRAY_COUNT(sLineSectionTrainer1Semifinal),  ARRAY_COUNT(sLineSectionTrainer1Final)},
    {ARRAY_COUNT(sLineSectionTrainer2Round1),  ARRAY_COUNT(sLineSectionTrainer2Round2),  ARRAY_COUNT(sLineSectionTrainer2Semifinal),  ARRAY_COUNT(sLineSectionTrainer2Final)},
    {ARRAY_COUNT(sLineSectionTrainer3Round1),  ARRAY_COUNT(sLineSectionTrainer3Round2),  ARRAY_COUNT(sLineSectionTrainer3Semifinal),  ARRAY_COUNT(sLineSectionTrainer3Final)},
    {ARRAY_COUNT(sLineSectionTrainer4Round1),  ARRAY_COUNT(sLineSectionTrainer4Round2),  ARRAY_COUNT(sLineSectionTrainer4Semifinal),  ARRAY_COUNT(sLineSectionTrainer4Final)},
    {ARRAY_COUNT(sLineSectionTrainer5Round1),  ARRAY_COUNT(sLineSectionTrainer5Round2),  ARRAY_COUNT(sLineSectionTrainer5Semifinal),  ARRAY_COUNT(sLineSectionTrainer5Final)},
    {ARRAY_COUNT(sLineSectionTrainer6Round1),  ARRAY_COUNT(sLineSectionTrainer6Round2),  ARRAY_COUNT(sLineSectionTrainer6Semifinal),  ARRAY_COUNT(sLineSectionTrainer6Final)},
    {ARRAY_COUNT(sLineSectionTrainer7Round1),  ARRAY_COUNT(sLineSectionTrainer7Round2),  ARRAY_COUNT(sLineSectionTrainer7Semifinal),  ARRAY_COUNT(sLineSectionTrainer7Final)},
    {ARRAY_COUNT(sLineSectionTrainer8Round1),  ARRAY_COUNT(sLineSectionTrainer8Round2),  ARRAY_COUNT(sLineSectionTrainer8Semifinal),  ARRAY_COUNT(sLineSectionTrainer8Final)},
    {ARRAY_COUNT(sLineSectionTrainer9Round1),  ARRAY_COUNT(sLineSectionTrainer9Round2),  ARRAY_COUNT(sLineSectionTrainer9Semifinal),  ARRAY_COUNT(sLineSectionTrainer9Final)},
    {ARRAY_COUNT(sLineSectionTrainer10Round1), ARRAY_COUNT(sLineSectionTrainer10Round2), ARRAY_COUNT(sLineSectionTrainer10Semifinal), ARRAY_COUNT(sLineSectionTrainer10Final)},
    {ARRAY_COUNT(sLineSectionTrainer11Round1), ARRAY_COUNT(sLineSectionTrainer11Round2), ARRAY_COUNT(sLineSectionTrainer11Semifinal), ARRAY_COUNT(sLineSectionTrainer11Final)},
    {ARRAY_COUNT(sLineSectionTrainer12Round1), ARRAY_COUNT(sLineSectionTrainer12Round2), ARRAY_COUNT(sLineSectionTrainer12Semifinal), ARRAY_COUNT(sLineSectionTrainer12Final)},
    {ARRAY_COUNT(sLineSectionTrainer13Round1), ARRAY_COUNT(sLineSectionTrainer13Round2), ARRAY_COUNT(sLineSectionTrainer13Semifinal), ARRAY_COUNT(sLineSectionTrainer13Final)},
    {ARRAY_COUNT(sLineSectionTrainer14Round1), ARRAY_COUNT(sLineSectionTrainer14Round2), ARRAY_COUNT(sLineSectionTrainer14Semifinal), ARRAY_COUNT(sLineSectionTrainer14Final)},
    {ARRAY_COUNT(sLineSectionTrainer15Round1), ARRAY_COUNT(sLineSectionTrainer15Round2), ARRAY_COUNT(sLineSectionTrainer15Semifinal), ARRAY_COUNT(sLineSectionTrainer15Final)},
    {ARRAY_COUNT(sLineSectionTrainer16Round1), ARRAY_COUNT(sLineSectionTrainer16Round2), ARRAY_COUNT(sLineSectionTrainer16Semifinal), ARRAY_COUNT(sLineSectionTrainer16Final)},
};

void CallBattleDomeFunction(void)
{
    sBattleDomeFunctions[gSpecialVar_0x8004]();
}

static void InitDomeChallenge(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    gSaveBlock2Ptr->frontier.challengeStatus = 0;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    gSaveBlock2Ptr->frontier.disableRecordBattle = FALSE;
    if (!(gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]))
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] = 0;

    SetDynamicWarp(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE);
    gTrainerBattleOpponent_A = 0;
}

static void GetDomeData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case DOME_DATA_WIN_STREAK:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
        break;
    case DOME_DATA_WIN_STREAK_ACTIVE:
        gSpecialVar_Result = ((gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]) != 0);
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSingles50;
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen;
        break;
    case DOME_DATA_HAS_WON_SINGLES_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSingles50;
        break;
    case DOME_DATA_HAS_WON_SINGLES_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen;
        break;
    case DOME_DATA_ATTEMPTED_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedDoublesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedDoubles50;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSingles50;
        }
        break;
    case DOME_DATA_HAS_WON_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonDoublesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonDoubles50;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSingles50;
        }
        break;
    case DOME_DATA_SELECTED_MONS:
        ClearSelectedPartyOrder();
        gSelectedOrderFromParty[0] = gSaveBlock2Ptr->frontier.selectedPartyMons[3];
        gSelectedOrderFromParty[1] = gSaveBlock2Ptr->frontier.selectedPartyMons[3] >> 8;
        break;
    case DOME_DATA_PREV_TOURNEY_TYPE:
        gSpecialVar_Result = (gSaveBlock2Ptr->frontier.domeLvlMode * 2) - 3 + gSaveBlock2Ptr->frontier.domeBattleMode;
        break;
    }
}

static void SetDomeData(void)
{
    u32 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case DOME_DATA_WIN_STREAK:
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] = gSpecialVar_0x8006;
        break;
    case DOME_DATA_WIN_STREAK_ACTIVE:
        if (gSpecialVar_0x8006)
            gSaveBlock2Ptr->frontier.winStreakActiveFlags |= sWinStreakFlags[battleMode][lvlMode];
        else
            gSaveBlock2Ptr->frontier.winStreakActiveFlags &= sWinStreakMasks[battleMode][lvlMode];
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_50:
        gSaveBlock2Ptr->frontier.domeAttemptedSingles50 = gSpecialVar_0x8006;
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_OPEN:
        gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen = gSpecialVar_0x8006;
        break;
    case DOME_DATA_HAS_WON_SINGLES_50:
        gSaveBlock2Ptr->frontier.domeHasWonSingles50 = gSpecialVar_0x8006;
        break;
    case DOME_DATA_HAS_WON_SINGLES_OPEN:
        gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen = gSpecialVar_0x8006;
        break;
    case DOME_DATA_ATTEMPTED_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeAttemptedDoublesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeAttemptedDoubles50 = gSpecialVar_0x8006;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeAttemptedSingles50 = gSpecialVar_0x8006;
        }
        break;
    case DOME_DATA_HAS_WON_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeHasWonDoublesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeHasWonDoubles50 = gSpecialVar_0x8006;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeHasWonSingles50 = gSpecialVar_0x8006;
        }
        break;
    case DOME_DATA_SELECTED_MONS:
        gSaveBlock2Ptr->frontier.selectedPartyMons[3] = T1_READ_16(gSelectedOrderFromParty);
        break;
    }
}

static void InitDomeTrainers(void)
{
    int i, j, k;
    int monLevel;
    int species[FRONTIER_PARTY_SIZE];
    int monTypesBits, monTypesCount;
    int trainerId;
    int monId;
    u16 *rankingScores;
    int *statValues;
    u8 ivs = 0;

    species[0] = 0;
    species[1] = 0;
    species[2] = 0;
    rankingScores = AllocZeroed(sizeof(u16) * DOME_TOURNAMENT_TRAINERS_COUNT);
    statValues = AllocZeroed(sizeof(int) * NUM_STATS);

    gSaveBlock2Ptr->frontier.domeLvlMode = gSaveBlock2Ptr->frontier.lvlMode + 1;
    gSaveBlock2Ptr->frontier.domeBattleMode = VarGet(VAR_FRONTIER_BATTLE_MODE) + 1;
    DOME_TRAINERS[0].trainerId = TRAINER_PLAYER;
    DOME_TRAINERS[0].isEliminated = FALSE;
    DOME_TRAINERS[0].eliminatedAt = 0;
    DOME_TRAINERS[0].forfeited = FALSE;

    // Store the data used to display party information on the player's tourney page
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        DOME_MONS[0][i] = GetMonData(&gPlayerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], MON_DATA_SPECIES, NULL);
        for (j = 0; j < MAX_MON_MOVES; j++)
            gSaveBlock2Ptr->frontier.domePlayerPartyData[i].moves[j] = GetMonData(&gPlayerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], MON_DATA_MOVE1 + j, NULL);
        for (j = 0; j < NUM_STATS; j++)
            gSaveBlock2Ptr->frontier.domePlayerPartyData[i].evs[j] = GetMonData(&gPlayerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], MON_DATA_HP_EV + j, NULL);

        gSaveBlock2Ptr->frontier.domePlayerPartyData[i].nature = GetNature(&gPlayerParty[gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1]);
    }

    // Populate the tourney roster with random frontier trainers (dependent on streak)
    for (i = 1; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        // Choose trainer. First 5/16 trainers are easier than the rest
        if (i > 5)
        {
            do
            {
                trainerId = GetRandomScaledFrontierTrainerId(GetCurrentFacilityWinStreak(), 0);
                for (j = 1; j < i; j++)
                {
                    if (DOME_TRAINERS[j].trainerId == trainerId)
                        break;
                }
            } while (j != i);
            DOME_TRAINERS[i].trainerId = trainerId;
        }
        else
        {
            do
            {
                trainerId = GetRandomScaledFrontierTrainerId(GetCurrentFacilityWinStreak() + 1, 0);
                for (j = 1; j < i; j++)
                {
                    if (DOME_TRAINERS[j].trainerId == trainerId)
                        break;
                }
            } while (j != i);
            DOME_TRAINERS[i].trainerId = trainerId;
        }

        // Choose party
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            do
            {
                monId = GetRandomFrontierMonFromSet(trainerId);
                for (k = 0; k < j; k++)
                {
                    // Make sure the mon is valid.
                    int alreadySelectedMonId = DOME_MONS[i][k];
                    if (alreadySelectedMonId == monId
                        || species[0] == gFacilityTrainerMons[monId].species
                        || species[1] == gFacilityTrainerMons[monId].species
                        || gFacilityTrainerMons[alreadySelectedMonId].itemTableId == gFacilityTrainerMons[monId].itemTableId)
                        break;
                }
            } while (k != j);

            DOME_MONS[i][j] = monId;
            species[j] = gFacilityTrainerMons[monId].species;
        }

        DOME_TRAINERS[i].isEliminated = FALSE;
        DOME_TRAINERS[i].eliminatedAt = 0;
        DOME_TRAINERS[i].forfeited = FALSE;
    }

    // rankingScores is used to determine the seed (ranking) of the trainers
    // rankingScores[0] is for the player, rankingScores[1-15] are for the opponent trainers

    // Calculate player's ranking score
    monTypesBits = 0;
    rankingScores[0] = 0;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        // trainerId var re-used here as index of selected mons
        trainerId = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_ATK, NULL);
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_DEF, NULL);
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_SPATK, NULL);
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_SPDEF, NULL);
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_SPEED, NULL);
        rankingScores[0] += GetMonData(&gPlayerParty[trainerId], MON_DATA_MAX_HP, NULL);
        monTypesBits |= gBitTable[gSpeciesInfo[GetMonData(&gPlayerParty[trainerId], MON_DATA_SPECIES, NULL)].types[0]];
        monTypesBits |= gBitTable[gSpeciesInfo[GetMonData(&gPlayerParty[trainerId], MON_DATA_SPECIES, NULL)].types[1]];
    }

    // Count the number of types in the players party, to factor into the ranking
    for (monTypesCount = 0, j = 0; j < 32; j++)
    {
        if (monTypesBits & 1)
            monTypesCount++;
        monTypesBits >>= 1;
    }

    monLevel = SetFacilityPtrsGetLevel();
    rankingScores[0] += (monTypesCount * monLevel) / 20;

    // Calculate rankingScores for the opponent trainers
    for (i = 1; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        monTypesBits = 0;
        rankingScores[i] = 0;
        ivs = GetDomeTrainerMonIvs(DOME_TRAINERS[i].trainerId);
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            CalcDomeMonStats(gFacilityTrainerMons[DOME_MONS[i][j]].species,
                             monLevel, ivs,
                             gFacilityTrainerMons[DOME_MONS[i][j]].evSpread,
                             gFacilityTrainerMons[DOME_MONS[i][j]].nature,
                             statValues);

            rankingScores[i] += statValues[STAT_ATK];
            rankingScores[i] += statValues[STAT_DEF];
            rankingScores[i] += statValues[STAT_SPATK];
            rankingScores[i] += statValues[STAT_SPDEF];
            rankingScores[i] += statValues[STAT_SPEED];
            rankingScores[i] += statValues[STAT_HP];
            monTypesBits |= gBitTable[gSpeciesInfo[gFacilityTrainerMons[DOME_MONS[i][j]].species].types[0]];
            monTypesBits |= gBitTable[gSpeciesInfo[gFacilityTrainerMons[DOME_MONS[i][j]].species].types[1]];
        }

        for (monTypesCount = 0, j = 0; j < 32; j++)
        {
            if (monTypesBits & 1)
                monTypesCount++;
            monTypesBits >>= 1;
        }
        rankingScores[i] += (monTypesCount * monLevel) / 20;
    }

    // Seed tourney trainers according to their ranking
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT - 1; i++)
    {
        for (j = i + 1; j < DOME_TOURNAMENT_TRAINERS_COUNT; j++)
        {
            if (rankingScores[i] < rankingScores[j])
            {
                SwapDomeTrainers(i, j, rankingScores);
            }
            else
            {
                if (rankingScores[i] == rankingScores[j])
                {
                    if (DOME_TRAINERS[j].trainerId == TRAINER_PLAYER)
                        SwapDomeTrainers(i, j, rankingScores);
                    else if (DOME_TRAINERS[i].trainerId > DOME_TRAINERS[j].trainerId)
                        SwapDomeTrainers(i, j, rankingScores);
                }
            }
        }
    }

    // Add Frontier Brain to the tourney if they should be fought at the end of it
    if (GetFrontierBrainStatus() != FRONTIER_BRAIN_NOT_READY)
    {
        for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
        {
            if (DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
                break;
        }

        if (sTrainerNamePositions[i][0] != TOURNEYWIN_NAMES_LEFT)
        {
            j = 0;
            DOME_TRAINERS[j].trainerId = TRAINER_FRONTIER_BRAIN;
        }
        else
        {
            j = 1;
            DOME_TRAINERS[j].trainerId = TRAINER_FRONTIER_BRAIN;
        }

        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            DOME_MONS[j][i] = GetFrontierBrainMonSpecies(i);
    }

    Free(rankingScores);
    Free(statValues);
}

#define CALC_STAT(base, statIndex)                                                          \
{                                                                                           \
    u8 baseStat = gSpeciesInfo[species].base;                                                 \
    stats[statIndex] = (((2 * baseStat + ivs + evs[statIndex] / 4) * level) / 100) + 5;     \
    stats[statIndex] = (u8) ModifyStatByNature(nature, stats[statIndex], statIndex);        \
}

static void CalcDomeMonStats(u16 species, int level, int ivs, u8 evBits, u8 nature, int *stats)
{
    int i, count;
    u8 bits;
    u16 resultingEvs;
    int evs[NUM_STATS];

    count = 0, bits = evBits;
    for (i = 0; i < NUM_STATS; bits >>= 1, i++)
    {
        if (bits & 1)
            count++;
    }

    resultingEvs = MAX_TOTAL_EVS / count;
    for (i = 0; i < NUM_STATS; bits <<= 1, i++)
    {
        evs[i] = 0;
        if (evBits & bits)
            evs[i] = resultingEvs;
    }

    if (species == SPECIES_SHEDINJA)
    {
        stats[STAT_HP] = 1;
    }
    else
    {
        int n = 2 * gSpeciesInfo[species].baseHP;
        stats[STAT_HP] = (((n + ivs + evs[STAT_HP] / 4) * level) / 100) + level + 10;
    }

    CALC_STAT(baseAttack, STAT_ATK);
    CALC_STAT(baseDefense, STAT_DEF);
    CALC_STAT(baseSpeed, STAT_SPEED);
    CALC_STAT(baseSpAttack, STAT_SPATK);
    CALC_STAT(baseSpDefense, STAT_SPDEF);
}

static void SwapDomeTrainers(int id1, int id2, u16 *statsArray)
{
    int i;
    u16 temp;

    SWAP(statsArray[id1], statsArray[id2], temp);
    SWAP(DOME_TRAINERS[id1].trainerId, DOME_TRAINERS[id2].trainerId, temp);

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        SWAP(DOME_MONS[id1][i], DOME_MONS[id2][i], temp);
}

static void BufferDomeRoundText(void)
{
    StringCopy(gStringVar1, gRoundsStringTable[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
}

static void BufferDomeOpponentName(void)
{
    StringCopy(gStringVar1, gRoundsStringTable[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
    CopyDomeTrainerName(gStringVar2, gTrainerBattleOpponent_A);
}

static void InitDomeOpponentParty(void)
{
    gPlayerPartyLostHP = 0;
    sPlayerPartyMaxHP =  GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP, NULL);
    sPlayerPartyMaxHP += GetMonData(&gPlayerParty[1], MON_DATA_MAX_HP, NULL);
    CalculatePlayerPartyCount();
    CreateDomeOpponentMons(TrainerIdToTournamentId(gTrainerBattleOpponent_A));
}

static void CreateDomeOpponentMon(u8 monPartyId, u16 tournamentTrainerId, u8 tournamentMonId, u32 otId)
{
    int i;
    u8 friendship = MAX_FRIENDSHIP;
    #ifdef BUGFIX
    u8 fixedIv = GetDomeTrainerMonIvs(DOME_TRAINERS[tournamentTrainerId].trainerId);
    #else
    u8 fixedIv = GetDomeTrainerMonIvs(tournamentTrainerId); // BUG: Using the wrong ID. As a result, all Pokémon have ivs of 3.
    #endif
    u8 level = SetFacilityPtrsGetLevel();
    CreateMonWithEVSpreadNatureOTID(&gEnemyParty[monPartyId],
                                         gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].species,
                                         level,
                                         gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].nature,
                                         fixedIv,
                                         gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].evSpread, otId);

    friendship = MAX_FRIENDSHIP;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonMoveSlot(&gEnemyParty[monPartyId],
                       gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].moves[i], i);
        if (gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].moves[i] == MOVE_FRUSTRATION)
            friendship = 0;
    }

    SetMonData(&gEnemyParty[monPartyId], MON_DATA_FRIENDSHIP, &friendship);
    SetMonData(&gEnemyParty[monPartyId], MON_DATA_HELD_ITEM,
               &gBattleFrontierHeldItems[gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][tournamentMonId]].itemTableId]);
}

static void CreateDomeOpponentMons(u16 tournamentTrainerId)
{
    u8 monsCount = 0;
    u32 otId = 0;
    int i, selectedMonBits;

    ZeroEnemyPartyMons();
    selectedMonBits = GetDomeTrainerSelectedMons(tournamentTrainerId);
    otId = Random32();

    if (Random() % 10 > 5)
    {
        // Create mon if it was selected, starting from front
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            if (selectedMonBits & 1)
            {
                CreateDomeOpponentMon(monsCount, tournamentTrainerId, i, otId);
                monsCount++;
            }
            selectedMonBits >>= 1;
        }
    }
    else
    {
        // Create mon if it was selected, starting from back
        for (i = FRONTIER_PARTY_SIZE - 1; i >= 0; i--)
        {
            if (selectedMonBits & (1 << (FRONTIER_PARTY_SIZE - 1)))
            {
                CreateDomeOpponentMon(monsCount, tournamentTrainerId, i, otId);
                monsCount++;
            }
            selectedMonBits <<= 1;
        }
    }
}

// Returns a bitmask representing which 2 of the trainer's 3 Pokémon to select.
// The choice is calculated solely depending on the type effectiveness of their
// movesets against the player's Pokémon.
// There is a 50% chance of either a "good" or "bad" selection mode being used.
// In the good mode movesets are preferred which are more effective against the
// player, and in the bad mode the opposite is true. If all 3 Pokémon tie, the
// other mode will be tried. If they tie again, the Pokémon selection is random.
int GetDomeTrainerSelectedMons(u16 tournamentTrainerId)
{
    int selectedMonBits;
    if (Random() & 1)
    {
        selectedMonBits = SelectOpponentMons_Good(tournamentTrainerId, FALSE);
        if (selectedMonBits == 0)
            selectedMonBits = SelectOpponentMons_Bad(tournamentTrainerId, TRUE);
    }
    else
    {
        selectedMonBits = SelectOpponentMons_Bad(tournamentTrainerId, FALSE);
        if (selectedMonBits == 0)
            selectedMonBits = SelectOpponentMons_Good(tournamentTrainerId, TRUE);
    }

    return selectedMonBits;
}

static int SelectOpponentMons_Good(u16 tournamentTrainerId, bool8 allowRandom)
{
    int i, moveId, playerMonId;
    int partyMovePoints[FRONTIER_PARTY_SIZE];

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        partyMovePoints[i] = 0;
        for (moveId = 0; moveId < MAX_MON_MOVES; moveId++)
        {
            for (playerMonId = 0; playerMonId < FRONTIER_PARTY_SIZE; playerMonId++)
            {
                if (DOME_TRAINERS[tournamentTrainerId].trainerId == TRAINER_FRONTIER_BRAIN)
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(GetFrontierBrainMonMove(i, moveId),
                                            GetMonData(&gPlayerParty[playerMonId], MON_DATA_SPECIES, NULL), EFFECTIVENESS_MODE_GOOD);
                }
                else
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][i]].moves[moveId],
                                            GetMonData(&gPlayerParty[playerMonId], MON_DATA_SPECIES, NULL), EFFECTIVENESS_MODE_GOOD);
                }
            }
        }
    }
    return SelectOpponentMonsFromParty(partyMovePoints, allowRandom);
}

// Identical to function above, but uses EFFECTIVENESS_MODE_BAD
static int SelectOpponentMons_Bad(u16 tournamentTrainerId, bool8 allowRandom)
{
    int i, moveId, playerMonId;
    int partyMovePoints[FRONTIER_PARTY_SIZE];

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        partyMovePoints[i] = 0;
        for (moveId = 0; moveId < MAX_MON_MOVES; moveId++)
        {
            for (playerMonId = 0; playerMonId < FRONTIER_PARTY_SIZE; playerMonId++)
            {
                if (DOME_TRAINERS[tournamentTrainerId].trainerId == TRAINER_FRONTIER_BRAIN)
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(GetFrontierBrainMonMove(i, moveId),
                                            GetMonData(&gPlayerParty[playerMonId], MON_DATA_SPECIES, NULL), EFFECTIVENESS_MODE_BAD);
                }
                else
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(gFacilityTrainerMons[DOME_MONS[tournamentTrainerId][i]].moves[moveId],
                                            GetMonData(&gPlayerParty[playerMonId], MON_DATA_SPECIES, NULL), EFFECTIVENESS_MODE_BAD);
                }
            }
        }
    }
    return SelectOpponentMonsFromParty(partyMovePoints, allowRandom);
}

static int SelectOpponentMonsFromParty(int *partyMovePoints, bool8 allowRandom)
{
    int i, j;
    int selectedMonBits = 0;
    int partyPositions[FRONTIER_PARTY_SIZE];

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        partyPositions[i] = i;

    // All party mons have equal move score totals, choose randomly
    if (partyMovePoints[0] == partyMovePoints[1]
     && partyMovePoints[0] == partyMovePoints[2])
    {
        if (allowRandom)
        {
            i = 0;
            while (i != DOME_BATTLE_PARTY_SIZE)
            {
                u32 rand = Random() & FRONTIER_PARTY_SIZE;
                if (rand != FRONTIER_PARTY_SIZE && !(selectedMonBits & gBitTable[rand]))
                {
                    selectedMonBits |= gBitTable[rand];
                    i++;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
        {
            for (j = i + 1; j < FRONTIER_PARTY_SIZE; j++)
            {
                int temp;

                if (partyMovePoints[i] < partyMovePoints[j])
                {
                    SWAP(partyMovePoints[i], partyMovePoints[j],temp)
                    SWAP(partyPositions[i], partyPositions[j], temp)
                }

                if (partyMovePoints[i] == partyMovePoints[j] && (Random() & 1))
                {
                    SWAP(partyMovePoints[i], partyMovePoints[j],temp)
                    SWAP(partyPositions[i], partyPositions[j], temp)
                }
            }
        }

        for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
        {
            selectedMonBits |= gBitTable[partyPositions[i]];
        }
    }

    return selectedMonBits;
}

#define TYPE_x0     0
#define TYPE_x0_25  5
#define TYPE_x0_50  10
#define TYPE_x1     20
#define TYPE_x2     40
#define TYPE_x4     80

static int GetTypeEffectivenessPoints(int move, int targetSpecies, int mode)
{
    int defType1, defType2, defAbility, moveType;
    int i = 0;
    int typePower = TYPE_x1;

    if (move == MOVE_NONE || move == MOVE_UNAVAILABLE || gBattleMoves[move].power == 0)
        return 0;

    defType1 = gSpeciesInfo[targetSpecies].types[0];
    defType2 = gSpeciesInfo[targetSpecies].types[1];
    defAbility = gSpeciesInfo[targetSpecies].abilities[0];
    moveType = gBattleMoves[move].type;

    if (defAbility == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        // They likely meant to return here, as 8 is the number of points normally used in this mode for moves with no effect.
        // Because there's no return the value instead gets interpreted by the switch, and the number of points becomes 0.
        if (mode == EFFECTIVENESS_MODE_BAD)
        {
            typePower = 8;
        #ifdef BUGFIX
            return typePower;
        #endif
        }
    }
    else
    {
        // Calculate a "type power" value to determine the benefit of using this type move against the target.
        // This value will then be used to get the number of points to assign to the move.
        while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
        {
            if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
            {
                i += 3;
                continue;
            }
            if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
            {
                // BUG: the value of TYPE_x2 does not exist in gTypeEffectiveness, so if defAbility is ABILITY_WONDER_GUARD, the conditional always fails
                #ifndef BUGFIX
                    #define WONDER_GUARD_EFFECTIVENESS TYPE_x2
                #else
                    #define WONDER_GUARD_EFFECTIVENESS TYPE_MUL_SUPER_EFFECTIVE
                #endif
                if (TYPE_EFFECT_DEF_TYPE(i) == defType1)
                    if ((defAbility == ABILITY_WONDER_GUARD && TYPE_EFFECT_MULTIPLIER(i) == WONDER_GUARD_EFFECTIVENESS) || defAbility != ABILITY_WONDER_GUARD)
                        typePower = (typePower * TYPE_EFFECT_MULTIPLIER(i)) / 10;
                if (TYPE_EFFECT_DEF_TYPE(i) == defType2 && defType1 != defType2)
                    if ((defAbility == ABILITY_WONDER_GUARD && TYPE_EFFECT_MULTIPLIER(i) == WONDER_GUARD_EFFECTIVENESS) || defAbility != ABILITY_WONDER_GUARD)
                        typePower = (typePower * TYPE_EFFECT_MULTIPLIER(i)) / 10;
            }
            i += 3;
        }
    }

    switch (mode)
    {
    case EFFECTIVENESS_MODE_GOOD:
        // Weights moves that more effective.
        switch (typePower)
        {
        case TYPE_x0:
        case TYPE_x0_25:
        case TYPE_x0_50:
        default:
            typePower = 0;
            break;
        case TYPE_x1:
            typePower = 2;
            break;
        case TYPE_x2:
            typePower = 4;
            break;
        case TYPE_x4:
            typePower = 8;
            break;
        }
        break;
    case EFFECTIVENESS_MODE_BAD:
        // Weights moves that are less effective.
        // Odd that there's no limit on this being used, even the Frontier Brain could end up using this.
        switch (typePower)
        {
        case TYPE_x0:
            typePower = 8;
            break;
        case TYPE_x0_25:
            typePower = 4;
            break;
        case TYPE_x0_50:
            typePower = 2;
            break;
        default:
        case TYPE_x1:
            typePower = 0;
            break;
        case TYPE_x2:
            typePower = -2;
            break;
        case TYPE_x4:
            typePower = -4;
            break;
        }
        break;
    case EFFECTIVENESS_MODE_AI_VS_AI:
        // Used as part of calculating the winner in a battle between two AIs.
        // Weights moves that are more effective much more strongly in both directions.
        switch (typePower)
        {
        case TYPE_x0:
            typePower = -16;
            break;
        case TYPE_x0_25:
            typePower = -8;
            break;
        case TYPE_x0_50:
        default:
            typePower = 0;
            break;
        case TYPE_x1:
            typePower = 4;
            break;
        case TYPE_x2:
            typePower = 12;
            break;
        case TYPE_x4:
            typePower = 20;
            break;
        }
        break;
    }

    return typePower;
}

// Duplicate of GetFrontierTrainerFixedIvs
// NOTE: In CreateDomeOpponentMon a tournament trainer ID (0-15) is passed instead, resulting in all IVs of 3
//       To fix, see CreateDomeOpponentMon
static u8 GetDomeTrainerMonIvs(u16 trainerId)
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

static int TournamentIdOfOpponent(int roundId, int trainerId)
{
    int i, j, opponentMax;

    // Get trainer's tournament id
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (DOME_TRAINERS[i].trainerId == trainerId)
            break;
    }

    // Get trainer's opponent's tournament id
    if (roundId != DOME_ROUND1)
    {
        if (roundId == DOME_FINAL)
            opponentMax = sIdToOpponentId[i][roundId] + 8;
        else
            opponentMax = sIdToOpponentId[i][roundId] + 4;

        // Get first non-eliminated trainer in range of possible opponents
        for (j = sIdToOpponentId[i][roundId]; j < opponentMax; j++)
        {
            if (sTourneyTreeTrainerOpponentIds[j] != i && !DOME_TRAINERS[sTourneyTreeTrainerOpponentIds[j]].isEliminated)
                break;
        }

        if (j != opponentMax)
            return sTourneyTreeTrainerOpponentIds[j];
        else
            return 0xFF; // Already eliminated
    }
    else
    {
        if (!DOME_TRAINERS[sIdToOpponentId[i][roundId]].isEliminated)
            return sIdToOpponentId[i][roundId];
        else
            return 0xFF; // Already eliminated
    }
}

static void SetDomeOpponentId(void)
{
    gTrainerBattleOpponent_A = TrainerIdOfPlayerOpponent();
}

// While not an issue in-game, this will overflow if called after the player's opponent for the current round has been eliminated
static u16 TrainerIdOfPlayerOpponent(void)
{
    return DOME_TRAINERS[TournamentIdOfOpponent(gSaveBlock2Ptr->frontier.curChallengeBattleNum, TRAINER_PLAYER)].trainerId;
}

static void SetDomeOpponentGraphicsId(void)
{
    SetBattleFacilityTrainerGfxId(gTrainerBattleOpponent_A, 0);
}

static void SaveDomeChallenge(void)
{
    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveGameFrontier();
}

static void IncrementDomeStreaks(void)
{
    u8 lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] < 999)
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode]++;
    if (gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode] < 999)
        gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode]++;

    if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode])
        gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
}

// For showing the opponent info card of the upcoming trainer
static void ShowDomeOpponentInfo(void)
{
    u8 taskId = CreateTask(Task_ShowTourneyInfoCard, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tTournamentId = TrainerIdToTournamentId(TrainerIdOfPlayerOpponent());
    gTasks[taskId].tMode = INFOCARD_NEXT_OPPONENT;
    gTasks[taskId].tPrevTaskId = 0;

    SetMainCallback2(CB2_TourneyTree);
}

// For showing the opponent info card or the match info card
static void Task_ShowTourneyInfoCard(u8 taskId)
{
    int i;
    int tournamentId = gTasks[taskId].tTournamentId;
    int mode = gTasks[taskId].tMode;
    int id = gTasks[taskId].tPrevTaskId;

    switch (gTasks[taskId].tState)
    {
    case 0:
        SetHBlankCallback(NULL);
        SetVBlankCallback(NULL);
        EnableInterrupts(INTR_FLAG_VBLANK);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sInfoCardBgTemplates, ARRAY_COUNT(sInfoCardBgTemplates));
        InitWindows(sInfoCardWindowTemplates);
        DeactivateAllTextPrinters();
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        gBattle_BG3_X = 0;
        gBattle_BG3_Y = 0;
        if (mode == INFOCARD_MATCH)
            gBattle_BG2_X = 0, gBattle_BG2_Y = 0;
        else
            gBattle_BG2_X = 0, gBattle_BG2_Y = DISPLAY_HEIGHT;

        gTasks[taskId].tState++;
        break;
    case 1:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WIN1H, 0);
        SetGpuReg(REG_OFFSET_WIN1V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 4;
        gTasks[taskId].tState++;
        break;
    case 2:
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyInfoCard_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyInfoCard_Tilemap, 0x2000, 0, 1);
        DecompressAndLoadBgGfxUsingHeap(3, gDomeTourneyInfoCardBg_Tilemap, 0x800, 0, 1);
        LoadCompressedSpriteSheet(sTourneyTreeButtonsSpriteSheet);
        LoadCompressedPalette(gDomeTourneyTree_Pal, BG_PLTT_OFFSET, BG_PLTT_SIZE);
        LoadCompressedPalette(gDomeTourneyTreeButtons_Pal, OBJ_PLTT_OFFSET, OBJ_PLTT_SIZE);
        LoadCompressedPalette(gBattleWindowTextPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        if (mode == INFOCARD_MATCH)
            LoadCompressedPalette(gDomeTourneyMatchCardBg_Pal, BG_PLTT_ID(5), PLTT_SIZE_4BPP); // Changes the moving info card bg to orange when in match card mode
        CpuFill32(0, gPlttBufferFaded, PLTT_SIZE);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        gTasks[taskId].tState++;
        break;
    case 3:
        SetVBlankCallback(VblankCb_TourneyInfoCard);
        sInfoCard = AllocZeroed(sizeof(*sInfoCard));
        for (i = 0; i < NUM_INFOCARD_SPRITES; i++)
            sInfoCard->spriteIds[i] = SPRITE_NONE;
        LoadMonIconPalettes();
        i = CreateTask(Task_HandleInfoCardInput, 0);
        gTasks[i].data[0] = 0;
        gTasks[i].data[2] = 0;
        gTasks[i].data[3] = mode;
        gTasks[i].data[4] = id;
        if (mode == INFOCARD_MATCH)
        {
            DisplayMatchInfoOnCard(0, tournamentId);
            sInfoCard->pos = 1;
        }
        else
        {
            DisplayTrainerInfoOnCard(0, tournamentId);
        }
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);
        if (mode != INFOCARD_NEXT_OPPONENT)
        {
            // Scroll up arrow
            id = CreateSprite(&sVerticalScrollArrowSpriteTemplate, 120, 4, 0);
            StartSpriteAnim(&gSprites[id], 0);
            gSprites[id].data[0] = i;

            // Scroll down arrow
            id = CreateSprite(&sVerticalScrollArrowSpriteTemplate, 120, 156, 0);
            StartSpriteAnim(&gSprites[id], 1);
            gSprites[id].data[0] = i;

            // Scroll left arrow
            id = CreateSprite(&sHorizontalScrollArrowSpriteTemplate, 6, 80, 0);
            StartSpriteAnim(&gSprites[id], 0);
            gSprites[id].data[0] = i;
            gSprites[id].data[1] = 0;
            if (mode == INFOCARD_TRAINER)
                gSprites[id].invisible = TRUE;

            // Scroll right arrow
            id = CreateSprite(&sHorizontalScrollArrowSpriteTemplate, 234, 80, 0);
            StartSpriteAnim(&gSprites[id], 1);
            gSprites[id].data[0] = i;
            gSprites[id].data[1] = 1;
        }
        DestroyTask(taskId);
        break;
    }
}

// Note: Card scrolling up means the current card goes down and another one appears from top.
// The same is true for scrolling left.
// That means that the sprite needs to move with the moving card in the opposite scrolling direction.
static void SpriteCB_TrainerIconCardScrollUp(struct Sprite *sprite)
{
    sprite->y += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y >= -32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->y >= 192)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollDown(struct Sprite *sprite)
{
    sprite->y -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y <= 192)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->y <= -32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollLeft(struct Sprite *sprite)
{
    sprite->x += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x >= -32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->x >= DISPLAY_WIDTH + 32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollRight(struct Sprite *sprite)
{
    sprite->x -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x <= DISPLAY_WIDTH + 32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->x <= -32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

#define sMonIconStill data[3]

static void SpriteCB_MonIconDomeInfo(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
}

static void SpriteCB_MonIconCardScrollUp(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->y += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y >= -16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->y >= 176)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollDown(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->y -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y <= 176)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->y <= -16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollLeft(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->x += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x >= -16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->x >= DISPLAY_WIDTH + 16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollRight(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->x -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x <= DISPLAY_WIDTH + 16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->x <= -16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_HorizontalScrollArrow(struct Sprite *sprite)
{
    int taskId1 = sprite->data[0];
    int arrId = gTasks[gTasks[taskId1].data[4]].data[1];
    int tournmanetTrainerId = sTourneyTreeTrainerIds[arrId];
    int roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;

    if (gTasks[taskId1].data[3] == 1)
    {
        if (sprite->data[1])
        {
            if ((DOME_TRAINERS[tournmanetTrainerId].isEliminated
                && sInfoCard->pos - 1 < DOME_TRAINERS[tournmanetTrainerId].eliminatedAt))
            {
                sprite->invisible = FALSE;
            }
            else if (!DOME_TRAINERS[tournmanetTrainerId].isEliminated
                     && sInfoCard->pos - 1 < roundId)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
        else
        {
            if (sInfoCard->pos != 0)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
    }
    else
    {
        if (sprite->data[1])
        {
            if (sInfoCard->pos > 1)
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
            else
            {
                sprite->invisible = FALSE;
            }
        }
        else
        {
            if (sInfoCard->pos != 0)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
    }
}

static void SpriteCB_VerticalScrollArrow(struct Sprite *sprite)
{
    int taskId1 = sprite->data[0];

    if (gTasks[taskId1].data[3] == 1)
    {
        if (sInfoCard->pos != 0)
        {
            if (gTasks[taskId1].data[0] == 2)
                sprite->invisible = TRUE;
        }
        else
        {
            sprite->invisible = FALSE;
        }
    }
    else
    {
        if (sInfoCard->pos != 1)
        {
            if (gTasks[taskId1].data[0] == 2)
                sprite->invisible = TRUE;
        }
        else
        {
            sprite->invisible = FALSE;
        }
    }
}

// Task states for Task_HandleInfoCardInput
#define STATE_FADE_IN      0
#define STATE_WAIT_FADE    1
#define STATE_GET_INPUT    2
#define STATE_REACT_INPUT  3
#define STATE_MOVE_UP      4
#define STATE_MOVE_DOWN    5
#define STATE_MOVE_LEFT    6
#define STATE_MOVE_RIGHT   7
#define STATE_CLOSE_CARD   8

#define tUsingAlternateSlot data[2] // CARD_ALTERNATE_SLOT

static void Task_HandleInfoCardInput(u8 taskId)
{
    int i;
    int windowId = 0;
    int mode = gTasks[taskId].data[3];
    int taskId2 = gTasks[taskId].data[4];
    int trainerTourneyId = 0;
    int matchNo = 0;

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState = STATE_WAIT_FADE;
        }
        break;
    case STATE_WAIT_FADE:
        if (!gPaletteFade.active)
            gTasks[taskId].tState = STATE_GET_INPUT;
        break;
    case STATE_GET_INPUT:
        i = Task_GetInfoCardInput(taskId);
        switch (i)
        {
        case INFOCARD_INPUT_AB:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_CARD;
            break;
        case TRAINERCARD_INPUT_UP ... TRAINERCARD_INPUT_RIGHT:
        case MATCHCARD_INPUT_UP ... MATCHCARD_INPUT_RIGHT:
            gTasks[taskId].data[5] = i;
            if (gTasks[taskId].tUsingAlternateSlot)
                windowId = NUM_INFO_CARD_WINDOWS;
            else
                windowId = 0;

            for (i = windowId; i < windowId + NUM_INFO_CARD_WINDOWS; i++)
            {
                CopyWindowToVram(i, COPYWIN_GFX);
                FillWindowPixelBuffer(i, PIXEL_FILL(0));
            }
            gTasks[taskId].tState = STATE_REACT_INPUT;
            break;
        case INFOCARD_INPUT_NONE:
            break;
        }
        break;
    case STATE_REACT_INPUT:
        i = gTasks[taskId].data[5];
        switch (i)
        {
        case TRAINERCARD_INPUT_UP:
        case MATCHCARD_INPUT_UP:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = DISPLAY_HEIGHT;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (i == TRAINERCARD_INPUT_UP)
            {
                if (sInfoCard->pos == 0)
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId);
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = 0;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId);
                    sInfoCard->pos = 0;
                }
            }
            else // i == MATCHCARD_INPUT_UP
            {
                if (sInfoCard->pos == 0)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    BufferDomeWinString(matchNo, sInfoCard->tournamentIds);
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sInfoCard->tournamentIds[0];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId);
                }
                else if (sInfoCard->pos == 2)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    BufferDomeWinString(matchNo, sInfoCard->tournamentIds);
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sInfoCard->tournamentIds[1];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId);
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    matchNo = gTasks[taskId2].data[1] - 16;
                    DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, matchNo);
                }
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_UP;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_DOWN:
        case MATCHCARD_INPUT_DOWN:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = -DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = -DISPLAY_HEIGHT;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (i == TRAINERCARD_INPUT_DOWN)
            {
                if (sInfoCard->pos == 0)
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId);
                }
                else
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = 0;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId);
                    sInfoCard->pos = 0;
                }
            }
            else // i == MATCHCARD_INPUT_DOWN
            {
                if (sInfoCard->pos == 0)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    BufferDomeWinString(matchNo, sInfoCard->tournamentIds);
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sInfoCard->tournamentIds[0];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId);
                }
                else if (sInfoCard->pos == 2)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    BufferDomeWinString(matchNo, sInfoCard->tournamentIds);
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sInfoCard->tournamentIds[1];
                    DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId);
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = 0;
                    matchNo = gTasks[taskId2].data[1] - 16;
                    DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, matchNo);
                }
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_DOWN;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_LEFT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = DISPLAY_WIDTH + 16;
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = DISPLAY_WIDTH + 16;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 0)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, trainerTourneyId);
            }
            else
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = 0;
                matchNo = sIdToMatchNumber[gTasks[taskId2].data[1]][sInfoCard->pos - 1];
                DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, matchNo);
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_LEFT;
            gTasks[taskId].data[5] = 0;
            break;
        case MATCHCARD_INPUT_LEFT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = DISPLAY_WIDTH + 16;
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = DISPLAY_WIDTH + 16;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 0)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sInfoCard->tournamentIds[0];
                DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, trainerTourneyId);
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                matchNo = gTasks[taskId2].data[1] - 16;
                DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, matchNo);
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_LEFT;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_RIGHT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 1)
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = 0;
            }
            matchNo = sIdToMatchNumber[gTasks[taskId2].data[1]][sInfoCard->pos - 1];
            DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, matchNo);

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_RIGHT;
            gTasks[taskId].data[5] = 0;
            break;
        case MATCHCARD_INPUT_RIGHT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 2)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sInfoCard->tournamentIds[1];
                DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, trainerTourneyId);
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                matchNo = gTasks[taskId2].data[1] - 16;
                DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, matchNo);
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_RIGHT;
            gTasks[taskId].data[5] = 0;
            break;
        }
        break;
    case STATE_MOVE_UP:
        if (++gTasks[taskId].data[5] != 41)
        {
            gBattle_BG0_Y -= 4;
            gBattle_BG1_Y -= 4;
            gBattle_BG2_Y -= 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_DOWN:
        if (++gTasks[taskId].data[5] != 41)
        {
            gBattle_BG0_Y += 4;
            gBattle_BG1_Y += 4;
            gBattle_BG2_Y += 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_LEFT:
        if (++gTasks[taskId].data[5] != 65)
        {
            gBattle_BG0_X -= 4;
            gBattle_BG1_X -= 4;
            gBattle_BG2_X -= 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_RIGHT:
        if (++gTasks[taskId].data[5] != 65)
        {
            gBattle_BG0_X += 4;
            gBattle_BG1_X += 4;
            gBattle_BG2_X += 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_CLOSE_CARD:
        if (!gPaletteFade.active)
        {
            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyTrainerPicSprite(sInfoCard->spriteIds[i]);
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyMonIconSprite(&gSprites[sInfoCard->spriteIds[i]]);
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyTrainerPicSprite(sInfoCard->spriteIds[i]);
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyMonIconSprite(&gSprites[sInfoCard->spriteIds[i]]);
                }
            }

            FreeMonIconPalettes();
            FREE_AND_SET_NULL(sInfoCard);
            FreeAllWindowBuffers();

            if (mode == INFOCARD_NEXT_OPPONENT)
            {
                SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            }
            else
            {
                i = CreateTask(Task_ShowTourneyTree, 0);
                gTasks[i].data[0] = 0;
                gTasks[i].tNotInteractive = FALSE;
                gTasks[i].data[2] = 3;
                gTasks[i].data[3] = gTasks[taskId].data[4];
                gTasks[i].tIsPrevTourneyTree = gTasks[taskId2].data[6];
            }
            DestroyTask(taskId);
        }
        break;
    }
}

// undefine task states for Task_HandleInfoCardInput
#undef STATE_FADE_IN
#undef STATE_WAIT_FADE
#undef STATE_GET_INPUT
#undef STATE_REACT_INPUT
#undef STATE_MOVE_UP
#undef STATE_MOVE_DOWN
#undef STATE_MOVE_LEFT
#undef STATE_MOVE_RIGHT
#undef STATE_CLOSE_CARD

static u8 Task_GetInfoCardInput(u8 taskId)
{
    u8 input = INFOCARD_INPUT_NONE;
    int taskId2 = gTasks[taskId].data[4];
    int position = gTasks[taskId2].data[1];
    u8 tourneyId = sTourneyTreeTrainerIds[position];
    u16 roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;

    if (JOY_NEW(A_BUTTON | B_BUTTON))
        input = INFOCARD_INPUT_AB;

    // Next opponent card cant scroll
    if (gTasks[taskId].data[3] == INFOCARD_NEXT_OPPONENT)
        return input;

    if (gTasks[taskId].data[3] == INFOCARD_TRAINER)
    {
        // For trainer info cards, pos is 0 when on a trainer info card (not viewing that trainer's match progression)
        // Scrolling up/down from a trainer info card goes to other trainer info cards
        if (JOY_NEW(DPAD_UP) && sInfoCard->pos == 0)
        {
            if (position == 0)
                position = DOME_TOURNAMENT_TRAINERS_COUNT - 1;
            else
                position--;
            input = TRAINERCARD_INPUT_UP;
        }
        else if (JOY_NEW(DPAD_DOWN) && sInfoCard->pos == 0)
        {
            if (position == DOME_TOURNAMENT_TRAINERS_COUNT - 1)
                position = 0;
            else
                position++;
            input = TRAINERCARD_INPUT_DOWN;
        }
        // Scrolling left can only be done after scrolling right
        else if (JOY_NEW(DPAD_LEFT) && sInfoCard->pos != 0)
        {
            sInfoCard->pos--;
            input = TRAINERCARD_INPUT_LEFT;
        }
        // Scrolling right from a trainer info card shows their match progression
        else if (JOY_NEW(DPAD_RIGHT))
        {
            // Can only scroll right from a trainer card until the round they were eliminated
            if (DOME_TRAINERS[tourneyId].isEliminated && sInfoCard->pos - 1 < DOME_TRAINERS[tourneyId].eliminatedAt)
            {
                sInfoCard->pos++;
                input = TRAINERCARD_INPUT_RIGHT;
            }
            // otherwise can scroll as far right as the current round allows
            if (!DOME_TRAINERS[tourneyId].isEliminated && sInfoCard->pos - 1 < roundId)
            {
                sInfoCard->pos++;
                input = TRAINERCARD_INPUT_RIGHT;
            }
        }

        if (input == INFOCARD_INPUT_AB)
        {
            if (sInfoCard->pos != 0)
                gTasks[taskId2].data[1] = sTrainerAndRoundToLastMatchCardNum[position / 2][sInfoCard->pos - 1];
            else
                gTasks[taskId2].data[1] = position;
        }
    }
    else // gTasks[taskId].data[3] == INFOCARD_MATCH
    {
        // For match info cards, pos is 1 when on the match card, 0 when on the left trainer, and 1 when on the right trainer
        // Scrolling up/down from a match info card goes to the next/previous match
        if (JOY_NEW(DPAD_UP) && sInfoCard->pos == 1)
        {
            if (position == DOME_TOURNAMENT_TRAINERS_COUNT)
                position = sLastMatchCardNum[roundId];
            else
                position--;
            input = MATCHCARD_INPUT_UP;
        }
        else if (JOY_NEW(DPAD_DOWN) && sInfoCard->pos == 1)
        {
            if (position == sLastMatchCardNum[roundId])
                position = DOME_TOURNAMENT_TRAINERS_COUNT;
            else
                position++;
            input = MATCHCARD_INPUT_DOWN;
        }
        // Scrolling left/right from a match info card shows the trainer info card of the competitors for that match
        else if (JOY_NEW(DPAD_LEFT) && sInfoCard->pos != 0)
        {
            input = MATCHCARD_INPUT_LEFT;
            sInfoCard->pos--;
        }
        else if (JOY_NEW(DPAD_RIGHT) && (sInfoCard->pos == 0 || sInfoCard->pos == 1))
        {
            input = MATCHCARD_INPUT_RIGHT;
            sInfoCard->pos++;
        }

        if (input == INFOCARD_INPUT_AB)
        {
            if (sInfoCard->pos == 0) // On left trainer info card
                gTasks[taskId2].data[1] = sTournamentIdToPairedTrainerIds[sInfoCard->tournamentIds[0]];
            else if (sInfoCard->pos == 2) // On right trainer info card
                gTasks[taskId2].data[1] = sTournamentIdToPairedTrainerIds[sInfoCard->tournamentIds[1]];
            else // On match info card
                gTasks[taskId2].data[1] = position;
        }
    }

    if (input != INFOCARD_INPUT_NONE && input != INFOCARD_INPUT_AB)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId2].data[1] = position;
        gTasks[taskId].tUsingAlternateSlot ^= 1;
    }

    return input;
}

#undef tUsingAlternateSlot

// allocatedArray below needs to be large enough to hold stat totals for each mon, or totals of each type of move points
#define ALLOC_ARRAY_SIZE max(NUM_STATS * FRONTIER_PARTY_SIZE, NUM_MOVE_POINT_TYPES)

static void DisplayTrainerInfoOnCard(u8 flags, u8 trainerTourneyId)
{
    struct TextPrinterTemplate textPrinter;
    int i, j, k;
    int trainerId = 0;
    u8 nature = 0;
    int arrId = 0;
    int windowId = WIN_TRAINER_NAME;
    int x = 0, y = 0;
    u8 palSlot = 0;
    s16 *allocatedArray = AllocZeroed(sizeof(s16) * ALLOC_ARRAY_SIZE);
    trainerId = DOME_TRAINERS[trainerTourneyId].trainerId;

    if (flags & CARD_ALTERNATE_SLOT)
        arrId = 2 * (FRONTIER_PARTY_SIZE + 1), windowId = WIN_TRAINER_NAME + NUM_INFO_CARD_WINDOWS, palSlot = 2;
    if (flags & MOVE_CARD_RIGHT)
        x = DISPLAY_WIDTH + 16;
    if (flags & MOVE_CARD_DOWN)
        y = DISPLAY_HEIGHT;
    if (flags & MOVE_CARD_LEFT)
        x = -(DISPLAY_WIDTH + 16);
    if (flags & MOVE_CARD_UP)
        y = -DISPLAY_HEIGHT;

    // Create trainer pic sprite
    if (trainerId == TRAINER_PLAYER)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(GetDomeBrainTrainerPicId(), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);
    else
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(GetFrontierTrainerFrontSpriteId(trainerId), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[arrId]].invisible = TRUE;

    // Create party mon icons
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerId == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(DOME_MONS[trainerTourneyId][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(DOME_MONS[trainerTourneyId][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].species,
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].invisible = TRUE;
    }

    // Initialize the text printer
    textPrinter.fontId = FONT_SHORT;
    textPrinter.x = 0;
    textPrinter.y = 0;
    textPrinter.currentX = textPrinter.x;
    textPrinter.currentY = textPrinter.y;
    textPrinter.letterSpacing = 2;
    textPrinter.lineSpacing = 0;
    textPrinter.unk = 0;
    textPrinter.fgColor = TEXT_DYNAMIC_COLOR_5;
    textPrinter.bgColor = TEXT_COLOR_TRANSPARENT;
    textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;

    // Get class and trainer name
    i = 0;
    if (trainerId == TRAINER_PLAYER)
        j = gFacilityClassToTrainerClass[FACILITY_CLASS_BRENDAN];
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
        j = GetDomeBrainTrainerClass();
    else
        j = GetFrontierOpponentClass(trainerId);

    for (;gTrainerClassNames[j][i] != EOS; i++)
        gStringVar1[i] = gTrainerClassNames[j][i];
    gStringVar1[i] = CHAR_SPACE;
    gStringVar1[i + 1] = EOS;

    if (trainerId == TRAINER_PLAYER)
    {
        StringAppend(gStringVar1, gSaveBlock2Ptr->playerName);
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CopyDomeBrainTrainerName(gStringVar2);
        StringAppend(gStringVar1, gStringVar2);
    }
    else
    {
        CopyDomeTrainerName(gStringVar2, trainerId);
        StringAppend(gStringVar1, gStringVar2);
    }

    // Print class and trainer name
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, gStringVar1, 0xD0, textPrinter.letterSpacing);
    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId;
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);
    textPrinter.letterSpacing = 0;

    // Print names of the party mons
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        textPrinter.currentY = sSpeciesNameTextYCoords[i];
        if (trainerId == TRAINER_PLAYER)
            textPrinter.currentChar = gSpeciesNames[DOME_MONS[trainerTourneyId][i]];
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
            textPrinter.currentChar = gSpeciesNames[DOME_MONS[trainerTourneyId][i]];
        else
            textPrinter.currentChar = gSpeciesNames[gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].species];

        textPrinter.windowId = WIN_TRAINER_MON1_NAME + i + windowId;
        if (i == 1)
            textPrinter.currentX = 7;
        else
            textPrinter.currentX = 0;

        PutWindowTilemap(WIN_TRAINER_MON1_NAME + i + windowId);
        CopyWindowToVram(WIN_TRAINER_MON1_NAME + i + windowId, COPYWIN_FULL);
        AddTextPrinter(&textPrinter, 0, NULL);
    }

    PutWindowTilemap(windowId + WIN_TRAINER_FLAVOR_TEXT);
    CopyWindowToVram(windowId + WIN_TRAINER_FLAVOR_TEXT, COPYWIN_FULL);

    // Print text about trainers potential in the tourney
    if (trainerId == TRAINER_FRONTIER_BRAIN)
        textPrinter.currentChar = sBattleDomePotentialTexts[DOME_TOURNAMENT_TRAINERS_COUNT];
    else
        textPrinter.currentChar = sBattleDomePotentialTexts[trainerTourneyId];

    textPrinter.fontId = FONT_NORMAL;
    textPrinter.windowId = windowId + WIN_TRAINER_FLAVOR_TEXT;
    textPrinter.currentX = 0;
    textPrinter.y = 4;
    textPrinter.currentY = 4;
    AddTextPrinter(&textPrinter, 0, NULL);

    // Calculate move scores to determine the trainers battle style
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            for (k = 0; k < NUM_MOVE_POINT_TYPES; k++)
            {
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    allocatedArray[k] += sBattleStyleMovePoints[GetFrontierBrainMonMove(i, j)][k];
                else if (trainerId == TRAINER_PLAYER)
                    allocatedArray[k] += sBattleStyleMovePoints[gSaveBlock2Ptr->frontier.domePlayerPartyData[i].moves[j]][k];
                else
                    allocatedArray[k] += sBattleStyleMovePoints[gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].moves[j]][k];
            }
        }
    }

    // Get the battle style the trainer uses
    // Each element of sBattleStyleThresholds is an array of point thresholds for particular move qualities
    // If all the point thresholds in the array are satisfied, the player is considered to be using that battle style
    for (i = 0; i < ARRAY_COUNT(sBattleStyleThresholds); i++)
    {
        int thresholdStatCount = 0;

        for (k = 0, j = 0; j < NUM_MOVE_POINT_TYPES; j++)
        {
            if (sBattleStyleThresholds[i][j] != 0)
            {
                thresholdStatCount++;
                if (allocatedArray[j] != 0 && allocatedArray[j] >= sBattleStyleThresholds[i][j])
                    k++; // number of point thresholds met/exceeded
            }
        }
        if (thresholdStatCount == k)
            break; // All thresholds for battle style met/exceeded, player uses this battle style
    }

    // Print the trainers battle style
    textPrinter.currentChar = sBattleDomeOpponentStyleTexts[i];
    textPrinter.y = 20;
    textPrinter.currentY = 20;
    AddTextPrinter(&textPrinter, 0, NULL);

    for (i = 0; i < ALLOC_ARRAY_SIZE; i++)
        allocatedArray[i] = 0;

    // Calculate EV/nature points for the stat portion of battle style
    if (trainerId == TRAINER_FRONTIER_BRAIN || trainerId == TRAINER_PLAYER)
    {
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            // Add the EVs for this mon
            for (j = 0; j < NUM_STATS; j++)
            {
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    allocatedArray[j] = GetFrontierBrainMonEvs(i, j);
                else
                    allocatedArray[j] = gSaveBlock2Ptr->frontier.domePlayerPartyData[i].evs[j];
            }

            // HP doesnt have a nature modifier, so just add it here
            allocatedArray[NUM_STATS] += allocatedArray[STAT_HP];

            // Add the EVs with the nature modifier for this mon and and track number of negative natures
            for (j = 0; j < NUM_NATURE_STATS; j++)
            {
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    nature = GetFrontierBrainMonNature(i);
                else
                    nature = gSaveBlock2Ptr->frontier.domePlayerPartyData[i].nature;

                if (gNatureStatTable[nature][j] > 0)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 110) / 100;
                }
                else if (gNatureStatTable[nature][j] < 0)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 90) / 100;
                    allocatedArray[j + NUM_STATS + NUM_NATURE_STATS + 2]++;
                }
                else
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
            }
        }
        for (j = 0, i = 0; i < NUM_STATS; i++)
            j += allocatedArray[NUM_STATS + i];
        for (i = 0; i < NUM_STATS; i++)
            allocatedArray[i] = (allocatedArray[NUM_STATS + i] * 100) / j;
    }
    // Same as above but for regular trainers instead of the frontier brain or player
    else
    {
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            int evBits = gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].evSpread;
            for (k = 0, j = 0; j < NUM_STATS; j++)
            {
                allocatedArray[j] = 0;
                if (evBits & 1)
                    k++;
                evBits >>= 1;
            }
            k = MAX_TOTAL_EVS / k;
            evBits = gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].evSpread;
            for (j = 0; j < NUM_STATS; j++)
            {
                if (evBits & 1)
                    allocatedArray[j] = k;
                evBits >>= 1;
            }

            allocatedArray[NUM_STATS] += allocatedArray[STAT_HP];
            for (j = 0; j < NUM_NATURE_STATS; j++)
            {
                nature = gFacilityTrainerMons[DOME_MONS[trainerTourneyId][i]].nature;
                if (gNatureStatTable[nature][j] > 0)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 110) / 100;
                }
                else if (gNatureStatTable[nature][j] < 0)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 90) / 100;
                    allocatedArray[j + NUM_STATS + NUM_NATURE_STATS + 2]++;
                }
                else
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
            }
        }
        for (j = 0, i = 0; i < NUM_STATS; i++)
            j += allocatedArray[i + NUM_STATS];
        for (i = 0; i < NUM_STATS; i++)
            allocatedArray[i] = (allocatedArray[NUM_STATS + i] * 100) / j;
    }

    // Count the number of good/bad stats for the party
    // i is the number of good stats, j is the number of bad stats
    for (i = 0, j = 0, k = 0; k < NUM_STATS; k++)
    {
        // Any stat above 29 EVs is considered good
        if (allocatedArray[k] > 29)
        {
            // If 2 good stats have been found already, choose which to use
            if (i == 2)
            {

                if (allocatedArray[6] < allocatedArray[k])
                {
                    if (allocatedArray[7] < allocatedArray[k])
                    {
                        if (allocatedArray[6] < allocatedArray[7])
                        {
                            allocatedArray[6] = allocatedArray[7];
                            allocatedArray[7] = k;
                        }
                        else
                        {
                            allocatedArray[7] = k;
                        }
                    }
                    else
                    {
                        allocatedArray[6] = allocatedArray[7];
                        allocatedArray[7] = k;
                    }
                }
                else
                {
                    if (allocatedArray[7] < allocatedArray[k])
                        allocatedArray[7] = k;
                }
            }
            else
            {
                allocatedArray[i + 6] = k;
                i++;
            }
        }

        // Any stat with 0 EVs is considered bad
        if (allocatedArray[k] == 0)
        {
            // If 2 bad stats have been found already, choose which to use
            if (j == 2)
            {
                if (allocatedArray[k + 12] >= 2
                    || ((allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[8]] == 0 && allocatedArray[12 + allocatedArray[9]] == 0)
                       )
                    )
                {
                    allocatedArray[8] = allocatedArray[9];
                    allocatedArray[9] = k;
                }
                else if (allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[8]] == 0)
                {
                    allocatedArray[8] = allocatedArray[9];
                    allocatedArray[9] = k;
                }
                else if (allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[9]] == 0)
                {
                    allocatedArray[9] = k;
                }
            }
            else
            {
                allocatedArray[j + 8] = k;
                j++;
            }
        }
    }

    // Get the string ID to display which stats are good/bad
    if (i == 2)
        i = sStatTextOffsets[allocatedArray[6]] + (allocatedArray[7] - (allocatedArray[6] + 1)) + DOME_TEXT_TWO_GOOD_STATS;
    else if (i == 1)
        i = allocatedArray[6] + DOME_TEXT_ONE_GOOD_STAT;
    else if (j == 2)
        i = sStatTextOffsets[allocatedArray[8]] + (allocatedArray[9] - (allocatedArray[8] + 1)) + DOME_TEXT_TWO_BAD_STATS;
    else if (j == 1)
        i = allocatedArray[8] + DOME_TEXT_ONE_BAD_STAT;
    else
        i = DOME_TEXT_WELL_BALANCED;

    // Print the stat text
    textPrinter.currentChar = sBattleDomeOpponentStatsTexts[i];
    textPrinter.y = 36;
    textPrinter.currentY = 36;
    AddTextPrinter(&textPrinter, 0, NULL);
    Free(allocatedArray);
}

static int BufferDomeWinString(u8 matchNum, u8 *tournamentIds)
{
    int i;
    u8 tournamentId;
    int winStringId = 0;
    int count = 0;

    // Get winners name
    for (i = sCompetitorRangeByMatch[matchNum][0]; i < sCompetitorRangeByMatch[matchNum][0] + sCompetitorRangeByMatch[matchNum][1]; i++)
    {
        tournamentId = sTourneyTreeTrainerIds2[i];
        if (!DOME_TRAINERS[tournamentId].isEliminated)
        {
            tournamentIds[count] = tournamentId;
            if (DOME_TRAINERS[tournamentId].trainerId == TRAINER_PLAYER)
                StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
            else if (DOME_TRAINERS[tournamentId].trainerId == TRAINER_FRONTIER_BRAIN)
                CopyDomeBrainTrainerName(gStringVar1);
            else
                CopyDomeTrainerName(gStringVar1, DOME_TRAINERS[tournamentId].trainerId);
            count++;
        }
    }

    // Neither trainer has been eliminated, battle hasn't occurred yet
    if (count == 2)
        return DOME_TEXT_NO_WINNER_YET;

    for (i = sCompetitorRangeByMatch[matchNum][0]; i < sCompetitorRangeByMatch[matchNum][0] + sCompetitorRangeByMatch[matchNum][1]; i++)
    {
        tournamentId = sTourneyTreeTrainerIds2[i];

        if (DOME_TRAINERS[tournamentId].isEliminated
            && DOME_TRAINERS[tournamentId].eliminatedAt >= sCompetitorRangeByMatch[matchNum][2])
        {
            tournamentIds[count] = tournamentId;
            count++;

            if (DOME_TRAINERS[tournamentId].eliminatedAt == sCompetitorRangeByMatch[matchNum][2])
            {
                // Set initial winStringId offset
                StringCopy(gStringVar2, gMoveNames[gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId]]);
                winStringId = DOME_TRAINERS[tournamentId].forfeited * 2; // (DOME_TEXT_WON_USING_MOVE - 1) or (DOME_TEXT_WON_ON_FORFEIT - 1)

                if (gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId] == MOVE_NONE && DOME_TRAINERS[tournamentId].forfeited == FALSE)
                    winStringId = DOME_TEXT_WON_NO_MOVES - 1;
            }
            else
            {
                if (DOME_TRAINERS[tournamentId].trainerId == TRAINER_PLAYER)
                    StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
                else if (DOME_TRAINERS[tournamentId].trainerId == TRAINER_FRONTIER_BRAIN)
                    CopyDomeBrainTrainerName(gStringVar1);
                else
                    CopyDomeTrainerName(gStringVar1, DOME_TRAINERS[tournamentId].trainerId);
            }
        }

        if (count == 2)
            break;
    }

    if (matchNum == DOME_TOURNAMENT_MATCHES_COUNT - 1)
        return winStringId + 2; // use DOME_TEXT_CHAMP_*
    else
        return winStringId + 1; // use DOME_TEXT_WON_*
}

static void DisplayMatchInfoOnCard(u8 flags, u8 matchNo)
{
    struct TextPrinterTemplate textPrinter;
    int tournamentIds[2];
    int trainerIds[2];
    bool32 lost[2];
    int i;
    int winStringId = 0;
    int arrId = 0;
    int windowId = 0;
    int x = 0, y = 0;
    u8 palSlot = 0;

    if (flags & CARD_ALTERNATE_SLOT)
        arrId = 2 * (FRONTIER_PARTY_SIZE + 1), windowId = NUM_INFO_CARD_WINDOWS, palSlot = 2;
    if (flags & MOVE_CARD_RIGHT)
        x = DISPLAY_WIDTH + 16;
    if (flags & MOVE_CARD_DOWN)
        y = DISPLAY_HEIGHT;
    if (flags & MOVE_CARD_LEFT)
        x = -(DISPLAY_WIDTH + 16);
    if (flags & MOVE_CARD_UP)
        y = -DISPLAY_HEIGHT;

    // Copy trainers information to handy arrays.
    winStringId = BufferDomeWinString(matchNo, sInfoCard->tournamentIds);
    for (i = 0; i < NUM_INFOCARD_TRAINERS; i++)
    {
        tournamentIds[i] = sInfoCard->tournamentIds[i];
        trainerIds[i] = DOME_TRAINERS[tournamentIds[i]].trainerId;
        if (DOME_TRAINERS[tournamentIds[i]].eliminatedAt <= sCompetitorRangeByMatch[matchNo][2]
            && DOME_TRAINERS[tournamentIds[i]].isEliminated)
            lost[i] = TRUE;
        else
            lost[i] = FALSE;
    }

    // Draw left trainer sprite.
    if (trainerIds[0] == TRAINER_PLAYER)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);
    else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(GetDomeBrainTrainerPicId(), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);
    else
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(GetFrontierTrainerFrontSpriteId(trainerIds[0]), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[arrId]].invisible = TRUE;
    if (lost[0])
        gSprites[sInfoCard->spriteIds[arrId]].oam.paletteNum = 3;

    // Draw right trainer sprite.
    if (trainerIds[1] == TRAINER_PLAYER)
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);
    else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(GetDomeBrainTrainerPicId(), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);
    else
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(GetFrontierTrainerFrontSpriteId(trainerIds[1]), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[1 + arrId]].invisible = TRUE;
    if (lost[1])
        gSprites[sInfoCard->spriteIds[1 + arrId]].oam.paletteNum = 3;

    // Draw left trainer's Pokémon icons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerIds[0] == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(DOME_MONS[tournamentIds[0]][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(DOME_MONS[tournamentIds[0]][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(gFacilityTrainerMons[DOME_MONS[tournamentIds[0]][i]].species,
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].invisible = TRUE;
        if (lost[0])
        {
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.paletteNum = 3;
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].sMonIconStill = TRUE;
        }
    }

    // Draw right trainer's Pokémon icons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerIds[1] == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(DOME_MONS[tournamentIds[1]][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }
        else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(DOME_MONS[tournamentIds[1]][i],
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(gFacilityTrainerMons[DOME_MONS[tournamentIds[1]][i]].species,
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0, TRUE);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].invisible = TRUE;
        if (lost[1])
        {
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.paletteNum = 3;
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].sMonIconStill = TRUE;
        }
    }

    // Print the win string (or 'Let the battle begin!').
    textPrinter.x = 0;
    textPrinter.y = 2;
    textPrinter.currentX = textPrinter.x;
    textPrinter.currentY = textPrinter.y;
    textPrinter.letterSpacing = 0;
    textPrinter.lineSpacing = 0;
    textPrinter.unk = 0;
    textPrinter.fgColor = TEXT_DYNAMIC_COLOR_5;
    textPrinter.bgColor = TEXT_COLOR_TRANSPARENT;
    textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;
    StringExpandPlaceholders(gStringVarBuffer, sBattleDomeWinTexts[winStringId]);
    textPrinter.currentChar = gStringVarBuffer;
    textPrinter.windowId = windowId + WIN_MATCH_WIN_TEXT;
    textPrinter.fontId = FONT_NORMAL;
    PutWindowTilemap(windowId + WIN_MATCH_WIN_TEXT);
    CopyWindowToVram(windowId + WIN_MATCH_WIN_TEXT, COPYWIN_FULL);
    textPrinter.currentX = 0;
    textPrinter.currentY = textPrinter.y = 0;
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print left trainer's name.
    if (trainerIds[0] == TRAINER_PLAYER)
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        CopyDomeBrainTrainerName(gStringVar1);
    else
        CopyDomeTrainerName(gStringVar1, trainerIds[0]);

    textPrinter.fontId = FONT_SHORT;
    textPrinter.letterSpacing = 2;
    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId + WIN_MATCH_TRAINER_NAME_LEFT;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x40, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_TRAINER_NAME_LEFT);
    CopyWindowToVram(windowId + WIN_MATCH_TRAINER_NAME_LEFT, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print right trainer's name.
    if (trainerIds[1] == TRAINER_PLAYER)
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        CopyDomeBrainTrainerName(gStringVar1);
    else
        CopyDomeTrainerName(gStringVar1, trainerIds[1]);

    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId + WIN_MATCH_TRAINER_NAME_RIGHT;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x40, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_TRAINER_NAME_RIGHT);
    CopyWindowToVram(windowId + WIN_MATCH_TRAINER_NAME_RIGHT, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print match number.
    textPrinter.letterSpacing = 0;
    textPrinter.currentChar = sBattleDomeMatchNumberTexts[matchNo];
    textPrinter.windowId = windowId + WIN_MATCH_NUMBER;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0xA0, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_NUMBER);
    CopyWindowToVram(windowId + WIN_MATCH_NUMBER, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);
}

static void ShowDomeTourneyTree(void)
{
    u8 taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = FALSE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = FALSE;
    SetMainCallback2(CB2_TourneyTree);
}

// To show the results of the last tourney on the computer in the lobby
static void ShowPreviousDomeTourneyTree(void)
{
    u8 taskId;

    SetFacilityTrainerAndMonPtrs();
    gSaveBlock2Ptr->frontier.lvlMode = gSaveBlock2Ptr->frontier.domeLvlMode - 1;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = DOME_FINAL;
    taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = FALSE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = TRUE;
    SetMainCallback2(CB2_TourneyTree);
}

// Task states for Task_HandleTourneyTreeInput
#define STATE_FADE_IN               0
#define STATE_WAIT_FADE             1
#define STATE_GET_INPUT             2
#define STATE_SHOW_INFOCARD_TRAINER 3
#define STATE_SHOW_INFOCARD_MATCH   5
#define STATE_CLOSE_TOURNEY_TREE    7

static void Task_HandleTourneyTreeInput(u8 taskId)
{
    u8 newTaskId = 0;
    int spriteId = gTasks[taskId].data[1];

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState = STATE_WAIT_FADE;
            StartSpriteAnim(&gSprites[spriteId], 1);
        }
        break;
    case STATE_WAIT_FADE:
        if (!gPaletteFade.active)
            gTasks[taskId].tState = STATE_GET_INPUT;
        break;
    case STATE_GET_INPUT:
        switch (UpdateTourneyTreeCursor(taskId))
        {
        case TOURNEY_TREE_SELECTED_CLOSE:
        default:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_TOURNEY_TREE;
            break;
        case TOURNEY_TREE_NO_SELECTION:
            break;
        case TOURNEY_TREE_SELECTED_TRAINER:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_SHOW_INFOCARD_TRAINER;
            break;
        case TOURNEY_TREE_SELECTED_MATCH:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_SHOW_INFOCARD_MATCH;
            break;
        }
        break;
    case STATE_SHOW_INFOCARD_TRAINER:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            newTaskId = CreateTask(Task_ShowTourneyInfoCard, 0);
            gTasks[newTaskId].tState = 0;
            gTasks[newTaskId].tTournamentId = sTourneyTreeTrainerIds[spriteId];
            gTasks[newTaskId].tMode = INFOCARD_TRAINER;
            gTasks[newTaskId].tPrevTaskId = taskId;

            gTasks[taskId].tState = STATE_SHOW_INFOCARD_TRAINER + 1;
            sInfoCard->pos = 0;
        }
        break;
    case STATE_SHOW_INFOCARD_TRAINER + 1:
        break;
    case STATE_SHOW_INFOCARD_MATCH:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            newTaskId = CreateTask(Task_ShowTourneyInfoCard, 0);
            gTasks[newTaskId].tState = 0;
            gTasks[newTaskId].tTournamentId = spriteId - DOME_TOURNAMENT_TRAINERS_COUNT;
            gTasks[newTaskId].tMode = INFOCARD_MATCH;
            gTasks[newTaskId].tPrevTaskId = taskId;

            gTasks[taskId].tState = STATE_SHOW_INFOCARD_MATCH + 1;
        }
        break;
    case STATE_SHOW_INFOCARD_MATCH + 1:
        break;
    case STATE_CLOSE_TOURNEY_TREE:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            DestroyTask(gTasks[taskId].data[7]);
            DestroyTask(taskId);
        }
        break;
    }
}

// undefine task states for Task_HandleTourneyTreeInput
#undef STATE_FADE_IN
#undef STATE_WAIT_FADE
#undef STATE_GET_INPUT
#undef STATE_SHOW_INFOCARD_TRAINER
#undef STATE_SHOW_INFOCARD_MATCH
#undef STATE_CLOSE_TOURNEY_TREE


#define MOVE_DIR_UP    0
#define MOVE_DIR_DOWN  1
#define MOVE_DIR_LEFT  2
#define MOVE_DIR_RIGHT 3
#define MOVE_DIR_NONE  4

// Move the tourney tree cursor
// The 'cursor' is actually just which button sprite is currently doing the 'selected' animation
static u8 UpdateTourneyTreeCursor(u8 taskId)
{
    u8 selection = TOURNEY_TREE_NO_SELECTION;
    int direction = MOVE_DIR_NONE;
    int tourneyTreeCursorSpriteId = gTasks[taskId].data[1];
    int roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;

    if (gMain.newKeys == B_BUTTON || (JOY_NEW(A_BUTTON) && tourneyTreeCursorSpriteId == TOURNEY_TREE_CLOSE_BUTTON))
    {
        PlaySE(SE_SELECT);
        selection = TOURNEY_TREE_SELECTED_CLOSE;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (tourneyTreeCursorSpriteId < DOME_TOURNAMENT_TRAINERS_COUNT)
        {
            PlaySE(SE_SELECT);
            selection = TOURNEY_TREE_SELECTED_TRAINER;
        }
        else
        {
            PlaySE(SE_SELECT);
            selection = TOURNEY_TREE_SELECTED_MATCH;
        }
    }
    else
    {
        if (gMain.newKeys == DPAD_UP && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][0] != 0xFF)
            direction = MOVE_DIR_UP;
        else if (gMain.newKeys == DPAD_DOWN && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][1] != 0xFF)
            direction = MOVE_DIR_DOWN;
        else if (gMain.newKeys == DPAD_LEFT && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][2] != 0xFF)
            direction = MOVE_DIR_LEFT;
        else if (gMain.newKeys == DPAD_RIGHT && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][3] != 0xFF)
            direction = MOVE_DIR_RIGHT;
    }

    if (direction != MOVE_DIR_NONE)
    {
        PlaySE(SE_SELECT);
        StartSpriteAnim(&gSprites[tourneyTreeCursorSpriteId], 0); // Do unselected sprite anim
        tourneyTreeCursorSpriteId = sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][direction];
        StartSpriteAnim(&gSprites[tourneyTreeCursorSpriteId], 1); // Do selected sprite anim
        gTasks[taskId].data[1] = tourneyTreeCursorSpriteId;
    }

    return selection;
}

#undef MOVE_DIR_UP
#undef MOVE_DIR_DOWN
#undef MOVE_DIR_LEFT
#undef MOVE_DIR_RIGHT
#undef MOVE_DIR_NONE

// Shows the results of the just-completed round for the current tourney
static void ShowNonInteractiveDomeTourneyTree(void)
{
    u8 taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = TRUE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = FALSE;
    SetMainCallback2(CB2_TourneyTree);
}

static void ResolveDomeRoundWinners(void)
{
    int i;

    if (gSpecialVar_0x8005 == DOME_PLAYER_WON_MATCH)
    {
        DOME_TRAINERS[TrainerIdToTournamentId(gTrainerBattleOpponent_A)].isEliminated = TRUE;
        DOME_TRAINERS[TrainerIdToTournamentId(gTrainerBattleOpponent_A)].eliminatedAt = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        gSaveBlock2Ptr->frontier.domeWinningMoves[TrainerIdToTournamentId(gTrainerBattleOpponent_A)] = gBattleResults.lastUsedMovePlayer;

        // If the player's match was the final one, no NPC vs NPC matches to decide
        if (gSaveBlock2Ptr->frontier.curChallengeBattleNum < DOME_FINAL)
            DecideRoundWinners(gSaveBlock2Ptr->frontier.curChallengeBattleNum);
    }
    else // DOME_PLAYER_LOST_MATCH or DOME_PLAYER_RETIRED
    {
        DOME_TRAINERS[TrainerIdToTournamentId(TRAINER_PLAYER)].isEliminated = TRUE;
        DOME_TRAINERS[TrainerIdToTournamentId(TRAINER_PLAYER)].eliminatedAt = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        gSaveBlock2Ptr->frontier.domeWinningMoves[TrainerIdToTournamentId(TRAINER_PLAYER)] = gBattleResults.lastUsedMoveOpponent;

        if (gBattleOutcome == B_OUTCOME_FORFEITED || gSpecialVar_0x8005 == DOME_PLAYER_RETIRED)
            DOME_TRAINERS[TrainerIdToTournamentId(TRAINER_PLAYER)].forfeited = TRUE;

        // Player lost, decide remaining outcome of tournament
        for (i = gSaveBlock2Ptr->frontier.curChallengeBattleNum; i < DOME_ROUNDS_COUNT; i++)
            DecideRoundWinners(i);
    }
}

// Decides the winning move of an NPC vs NPC match
static u16 GetWinningMove(int winnerTournamentId, int loserTournamentId, u8 roundId)
{
    int i, j, k;
    int moveScores[MAX_MON_MOVES * FRONTIER_PARTY_SIZE];
    u16 moveIds[MAX_MON_MOVES * FRONTIER_PARTY_SIZE];
    u16 bestScore = 0;
    u16 bestId = 0;
    int movePower = 0;
    SetFacilityPtrsGetLevel();

    // Calc move points of all 4 moves for all 3 Pokémon hitting all 3 target mons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            // TODO: Clean this up, looks like a different data structure (2D array)
            moveScores[i * MAX_MON_MOVES + j] = 0;
            if (DOME_TRAINERS[winnerTournamentId].trainerId == TRAINER_FRONTIER_BRAIN)
                moveIds[i * MAX_MON_MOVES + j] = GetFrontierBrainMonMove(i, j);
            else
                moveIds[i * MAX_MON_MOVES + j] = gFacilityTrainerMons[DOME_MONS[winnerTournamentId][i]].moves[j];

            movePower = gBattleMoves[moveIds[i * MAX_MON_MOVES + j]].power;
            if (movePower == 0)
                movePower = 40;
            else if (movePower == 1)
                movePower = 60;
            else if (moveIds[i * MAX_MON_MOVES + j] == MOVE_SELF_DESTRUCT
                  || moveIds[i * MAX_MON_MOVES + j] == MOVE_EXPLOSION)
                movePower /= 2;

            for (k = 0; k < FRONTIER_PARTY_SIZE; k++)
            {
                u32 var = 0;
                u16 targetSpecies = SPECIES_NONE;
                u16 targetAbility = ABILITY_NONE;
                do
                {
                    var = Random32();
                } while (gFacilityTrainerMons[DOME_MONS[loserTournamentId][k]].nature != GetNatureFromPersonality(var));

                targetSpecies = gFacilityTrainerMons[DOME_MONS[loserTournamentId][k]].species;
                if (var & 1)
                    targetAbility = gSpeciesInfo[targetSpecies].abilities[1];
                else
                    targetAbility = gSpeciesInfo[targetSpecies].abilities[0];

                var = AI_TypeCalc(moveIds[i * MAX_MON_MOVES + j], targetSpecies, targetAbility);
                if (var & MOVE_RESULT_NOT_VERY_EFFECTIVE && var & MOVE_RESULT_SUPER_EFFECTIVE)
                    moveScores[i * MAX_MON_MOVES + j] += movePower;
                else if (var & MOVE_RESULT_NO_EFFECT)
                    moveScores[i * MAX_MON_MOVES + j] += 0;
                else if (var & MOVE_RESULT_SUPER_EFFECTIVE)
                    moveScores[i * MAX_MON_MOVES + j] += movePower * 2;
                else if (var & MOVE_RESULT_NOT_VERY_EFFECTIVE)
                    moveScores[i * MAX_MON_MOVES + j] += movePower / 2;
                else
                    moveScores[i * MAX_MON_MOVES + j] += movePower;
            }

            if (bestScore < moveScores[i * MAX_MON_MOVES + j])
            {
                bestId = i * MAX_MON_MOVES + j;
                bestScore = moveScores[i * MAX_MON_MOVES + j];
            }
            else if (bestScore == moveScores[i * MAX_MON_MOVES + j])
            {
                if (moveIds[bestId] < moveIds[i * MAX_MON_MOVES + j]) // Why not use (Random() & 1) instead of promoting moves with a higher id?
                    bestId = i * MAX_MON_MOVES + j;
            }
        }
    }

    j = bestId;
    do
    {
        for (i = 0; i < roundId - 1; i++)
        {
            if (gSaveBlock2Ptr->frontier.domeWinningMoves[GetOpposingNPCTournamentIdByRound(winnerTournamentId, i)] == moveIds[j])
                break;
        }
        if (i != roundId - 1)
        {
            moveScores[j] = 0;
            bestScore = 0;
            j = 0;
            for (k = 0; k < MAX_MON_MOVES * FRONTIER_PARTY_SIZE; k++)
                j += moveScores[k];
            if (j == 0)
                break;
            j = 0;
            for (k = 0; k < MAX_MON_MOVES * FRONTIER_PARTY_SIZE; k++)
            {
                if (bestScore < moveScores[k])
                {
                    j = k;
                    bestScore = moveScores[k];
                }
                else if (bestScore == moveScores[k] && moveIds[j] < moveIds[k]) // Yes, these conditions are redundant
                {
                    j = k;
                    bestScore = moveScores[k];
                }
            }
        }
    } while (i != roundId - 1);

    if (moveScores[j] == 0)
        j = bestId;

    return moveIds[j];
}

static void Task_ShowTourneyTree(u8 taskId)
{
    int i;
    struct TextPrinterTemplate textPrinter;
    int notInteractive = gTasks[taskId].tNotInteractive;
    int r4 = gTasks[taskId].data[2];

    switch (gTasks[taskId].tState)
    {
    case 0:
        SetHBlankCallback(NULL);
        SetVBlankCallback(NULL);
        EnableInterrupts(INTR_FLAG_HBLANK | INTR_FLAG_VBLANK);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sTourneyTreeBgTemplates, ARRAY_COUNT(sTourneyTreeBgTemplates));
        InitWindows(sTourneyTreeWindowTemplates);
        DeactivateAllTextPrinters();
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0xB00, BG_COORD_SET);
        gTasks[taskId].tState++;
        break;
    case 1:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(88, 96));
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, DISPLAY_HEIGHT - 1));
        SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(144, 152));
        SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(0, DISPLAY_HEIGHT - 1));
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gTasks[taskId].tState++;
        break;
    case 2:
        sTilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);
        LZDecompressWram(gDomeTourneyTree_Tilemap, sTilemapBuffer);
        SetBgTilemapBuffer(1, sTilemapBuffer);
        CopyBgTilemapBufferToVram(1);
        DecompressAndLoadBgGfxUsingHeap(1, gDomeTourneyTree_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyLine_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyLineDown_Tilemap, 0x2000, 0, 1);
        DecompressAndLoadBgGfxUsingHeap(3, gDomeTourneyLineUp_Tilemap, 0x2000, 0, 1);
        LoadCompressedPalette(gDomeTourneyTree_Pal, BG_PLTT_OFFSET, BG_PLTT_SIZE);
        LoadCompressedPalette(gDomeTourneyTreeButtons_Pal, OBJ_PLTT_OFFSET, OBJ_PLTT_SIZE);
        LoadCompressedPalette(gBattleWindowTextPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        CpuFill32(0, gPlttBufferFaded, PLTT_SIZE);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        gTasks[taskId].tState++;
        break;
    case 3:
        LoadCompressedSpriteSheet(sTourneyTreeButtonsSpriteSheet);
        if (notInteractive == FALSE)
        {
            for (i = 0; i < ARRAY_COUNT(sTourneyTreePokeballCoords); i++)
                CreateSprite(&sTourneyTreePokeballSpriteTemplate, sTourneyTreePokeballCoords[i][0], sTourneyTreePokeballCoords[i][1], 0);

            if (gTasks[taskId].tIsPrevTourneyTree)
                CreateSprite(&sExitButtonSpriteTemplate, 218, 12, 0);
            else
                CreateSprite(&sCancelButtonSpriteTemplate, 218, 12, 0);
        }

        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJ_1D_MAP);
        gTasks[taskId].tState++;
        break;
    case 4:
        textPrinter.fontId = FONT_SHORT;
        textPrinter.currentChar = gText_BattleTourney;
        textPrinter.windowId = TOURNEYWIN_TITLE;
        textPrinter.x = 0;
        textPrinter.y = 0;
        textPrinter.letterSpacing = 2;
        textPrinter.lineSpacing = 0;
        textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x70, textPrinter.letterSpacing);
        textPrinter.currentY = 1;
        textPrinter.unk = 0;
        textPrinter.fgColor = TEXT_DYNAMIC_COLOR_5;
        textPrinter.bgColor = TEXT_COLOR_TRANSPARENT;
        textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;
        AddTextPrinter(&textPrinter, 0, NULL);
        for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
        {
            int roundId, var2;

            CopyDomeTrainerName(gDisplayedStringBattle, DOME_TRAINERS[i].trainerId);
            if (notInteractive == TRUE)
            {
                if (DOME_TRAINERS[i].isEliminated)
                {
                    if (DOME_TRAINERS[i].eliminatedAt != DOME_ROUND1)
                    {
                        var2 = DOME_TRAINERS[i].eliminatedAt - 1;
                        DrawTourneyAdvancementLine(i, var2);
                    }
                }
                else if (gSaveBlock2Ptr->frontier.curChallengeBattleNum != DOME_ROUND2)
                {
                    DrawTourneyAdvancementLine(i, gSaveBlock2Ptr->frontier.curChallengeBattleNum - 2);
                }
            }
            else if (notInteractive == FALSE)
            {
                if (DOME_TRAINERS[i].isEliminated)
                {
                    if (DOME_TRAINERS[i].eliminatedAt != DOME_ROUND1)
                    {
                        var2 = DOME_TRAINERS[i].eliminatedAt - 1;
                        DrawTourneyAdvancementLine(i, var2);
                    }
                }
                else if (gSaveBlock2Ptr->frontier.curChallengeBattleNum != DOME_ROUND1)
                {
                    if (gTasks[taskId].tIsPrevTourneyTree)
                        var2 = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
                    else
                        var2 = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;
                    DrawTourneyAdvancementLine(i, var2);
                }
            }

            if (gTasks[taskId].tIsPrevTourneyTree)
                roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            else
                roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;

            if (    ((notInteractive == TRUE && DOME_TRAINERS[i].eliminatedAt < gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1)
                  || (notInteractive == FALSE && DOME_TRAINERS[i].eliminatedAt <= roundId))
                && DOME_TRAINERS[i].isEliminated)
            {
                if (DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
                {
                    textPrinter.fgColor = TEXT_COLOR_LIGHT_GRAY;
                    textPrinter.shadowColor = TEXT_COLOR_RED;
                }
                else
                {
                    textPrinter.fgColor = TEXT_DYNAMIC_COLOR_2;
                    textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;
                }
            }
            else
            {
                if (DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
                {
                    textPrinter.fgColor = TEXT_COLOR_LIGHT_GRAY;
                    textPrinter.shadowColor = TEXT_COLOR_RED;
                }
                else
                {
                    textPrinter.fgColor = TEXT_DYNAMIC_COLOR_5;
                    textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;
                }
            }

            if (sTrainerNamePositions[i][0] == TOURNEYWIN_NAMES_LEFT)
                textPrinter.currentX = GetStringWidthDifference(textPrinter.fontId, gDisplayedStringBattle, 0x3D, textPrinter.letterSpacing);
            else
                textPrinter.currentX = 3;
            textPrinter.currentChar = gDisplayedStringBattle;
            textPrinter.windowId = sTrainerNamePositions[i][0];
            textPrinter.currentY = sTrainerNamePositions[i][1];
            AddTextPrinter(&textPrinter, 0, NULL);
        }
        gTasks[taskId].tState++;
        break;
    case 5:
        PutWindowTilemap(TOURNEYWIN_NAMES_LEFT);
        PutWindowTilemap(TOURNEYWIN_NAMES_RIGHT);
        PutWindowTilemap(TOURNEYWIN_TITLE);
        CopyWindowToVram(TOURNEYWIN_NAMES_LEFT, COPYWIN_FULL);
        CopyWindowToVram(TOURNEYWIN_NAMES_RIGHT, COPYWIN_FULL);
        CopyWindowToVram(TOURNEYWIN_TITLE, COPYWIN_FULL);
        SetHBlankCallback(HblankCb_TourneyTree);
        SetVBlankCallback(VblankCb_TourneyTree);
        if (r4 == 2)
        {
            if (notInteractive == FALSE)
            {
                i = CreateTask(Task_HandleTourneyTreeInput, 0);
                gTasks[i].data[0] = notInteractive;
                gTasks[i].data[1] = notInteractive;
                gTasks[i].data[6] = gTasks[taskId].tIsPrevTourneyTree;
            }
            else
            {
                i = CreateTask(Task_HandleStaticTourneyTreeInput, 0);
                gTasks[i].data[0] = 0;
            }
        }
        else
        {
            i = gTasks[taskId].data[3];
            gTasks[i].tState = 0;
        }
        ScanlineEffect_Clear();

        i = 0;
        while (i < 91)
        {
            gScanlineEffectRegBuffers[0][i] = BGCNT_PRIORITY(2) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            gScanlineEffectRegBuffers[1][i] = BGCNT_PRIORITY(2) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            i++;
        }

        while (i < 160)
        {
            gScanlineEffectRegBuffers[0][i] =  BGCNT_PRIORITY(1) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            gScanlineEffectRegBuffers[1][i] =  BGCNT_PRIORITY(1) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            i++;
        }

        ScanlineEffect_SetParams(sTourneyTreeScanlineEffectParams);
        DestroyTask(taskId);
        break;
    }
}

static void DrawTourneyAdvancementLine(u8 tournamentId, u8 roundId)
{
    int i;
    const struct TourneyTreeLineSection *lineSection = sTourneyTreeLineSections[tournamentId][roundId];

    for (i = 0; i < sTourneyTreeLineSectionArrayCounts[tournamentId][roundId]; i++)
        CopyToBgTilemapBufferRect_ChangePalette(1, &lineSection[i].tile, lineSection[i].x, lineSection[i].y, 1, 1, 17);

    CopyBgTilemapBufferToVram(1);
}

#define STATE_FADE_IN             0
#define STATE_SHOW_RESULTS        1
#define STATE_DELAY               2
#define STATE_WAIT_FOR_INPUT      3
#define STATE_CLOSE_TOURNEY_TREE  4

// The non-interactive tourney tree that's shown when a round is completed
static void Task_HandleStaticTourneyTreeInput(u8 taskId)
{
    int i;
    struct TextPrinterTemplate textPrinter;

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gTasks[taskId].tState = STATE_SHOW_RESULTS;
        break;
    case STATE_SHOW_RESULTS:
        if (!gPaletteFade.active)
        {
            gTasks[taskId].tState = STATE_DELAY;
            gTasks[taskId].data[3] = 64;
            textPrinter.fontId = FONT_SHORT;
            textPrinter.x = 0;
            textPrinter.y = 0;
            textPrinter.letterSpacing = 2;
            textPrinter.lineSpacing = 0;
            textPrinter.unk = 0;
            textPrinter.fgColor = TEXT_DYNAMIC_COLOR_2;
            textPrinter.bgColor = TEXT_COLOR_TRANSPARENT;
            textPrinter.shadowColor = TEXT_DYNAMIC_COLOR_4;

            // Update the advancement lines and gray out eliminated trainer names
            for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
            {
                CopyDomeTrainerName(gDisplayedStringBattle, DOME_TRAINERS[i].trainerId);
                if (DOME_TRAINERS[i].eliminatedAt == gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1
                    && DOME_TRAINERS[i].isEliminated)
                {
                    if (sTrainerNamePositions[i][0] == TOURNEYWIN_NAMES_LEFT)
                        textPrinter.currentX = GetStringWidthDifference(textPrinter.fontId, gDisplayedStringBattle, 0x3D, textPrinter.letterSpacing);
                    else
                        textPrinter.currentX = 3;

                    textPrinter.currentChar = gDisplayedStringBattle;
                    textPrinter.windowId = sTrainerNamePositions[i][0];
                    textPrinter.currentY = sTrainerNamePositions[i][1];
                    AddTextPrinter(&textPrinter, 0, NULL);
                }
                if (!DOME_TRAINERS[i].isEliminated)
                {
                    int roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;
                    DrawTourneyAdvancementLine(i, roundId);
                }
            }
        }
        break;
    case STATE_DELAY:
        if (--gTasks[taskId].data[3] == 0)
            gTasks[taskId].tState = STATE_WAIT_FOR_INPUT;
        break;
    case STATE_WAIT_FOR_INPUT:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_TOURNEY_TREE;
        }
        break;
    case STATE_CLOSE_TOURNEY_TREE:
        if (!gPaletteFade.active)
        {
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            DestroyTask(taskId);
        }
        break;
    }
}

#undef STATE_FADE_IN
#undef STATE_SHOW_RESULTS
#undef STATE_DELAY
#undef STATE_WAIT_FOR_INPUT
#undef STATE_CLOSE_TOURNEY_TREE

static void CB2_TourneyTree(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}

static void VblankCb_TourneyInfoCard(void)
{
    ChangeBgX(3, 0x80, BG_COORD_ADD);
    ChangeBgY(3, 0x80, BG_COORD_SUB);
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
    SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

#define SET_WIN0H_WIN1H(win0H, win1H)                       \
{                                                           \
    *(vu32*)(REG_ADDR_WIN0H) = ((win0H << 16) | (win1H));   \
}

static void HblankCb_TourneyTree(void)
{
    u16 vCount = REG_VCOUNT;

    if (vCount < 42)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 50)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 58)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 75)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(144, 152), WIN_RANGE(88, 96));
    }
    else if (vCount < 82)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 95)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 103)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 119)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(144, 152), WIN_RANGE(88, 96));
    }
    else if (vCount < 127)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 135)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
}

static void VblankCb_TourneyTree(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    ChangeBgY(2, 0x80, BG_COORD_SUB);
    ChangeBgY(3, 0x80, BG_COORD_ADD);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetFacilityTrainerAndMonPtrs(void)
{
    gFacilityTrainerMons = gBattleFrontierMons;
    gFacilityTrainers = gBattleFrontierTrainers;
}

static void ResetSketchedMoves(void)
{
    int i, moveSlot;

    for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
    {
        int playerMonId = gSaveBlock2Ptr->frontier.selectedPartyMons[gSelectedOrderFromParty[i] - 1] - 1;
        int count;

        for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
        {
            count = 0;
            while (count < MAX_MON_MOVES)
            {
                if (GetMonData(&gSaveBlock1Ptr->playerParty[playerMonId], MON_DATA_MOVE1 + count, NULL) == GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + moveSlot, NULL))
                    break;
                count++;
            }
            if (count == MAX_MON_MOVES)
                SetMonMoveSlot(&gPlayerParty[i], MOVE_SKETCH, moveSlot);
        }

        gSaveBlock1Ptr->playerParty[playerMonId] = gPlayerParty[i];
    }
}

static void RestoreDomePlayerPartyHeldItems(void)
{
    int i;

    for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
    {
        int playerMonId = gSaveBlock2Ptr->frontier.selectedPartyMons[gSelectedOrderFromParty[i] - 1] - 1;
        u16 item = GetMonData(&gSaveBlock1Ptr->playerParty[playerMonId], MON_DATA_HELD_ITEM, NULL);
        SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &item);
    }
}

static void ReduceDomePlayerPartyToSelectedMons(void)
{
    ReducePlayerPartyToSelectedMons();
}

static void GetPlayerSeededBeforeOpponent(void)
{
    // A higher tournament ID is a worse seed
    if (TrainerIdToTournamentId(gTrainerBattleOpponent_A) > TrainerIdToTournamentId(TRAINER_PLAYER))
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2;
}

static void BufferLastDomeWinnerName(void)
{
    int i;

    SetFacilityTrainerAndMonPtrs();
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (!DOME_TRAINERS[i].isEliminated)
            break;
    }
    CopyDomeTrainerName(gStringVar1, DOME_TRAINERS[i].trainerId);
}

// For showing the previous tourney results before the player has entered a challenge
static void InitRandomTourneyTreeResults(void)
{
    int i, j, k;
    int monLevel;
    int species[FRONTIER_PARTY_SIZE];
    int monTypesBits;
    int trainerId;
    int monId;
    int zero1;
    int zero2;
    u8 lvlMode;
    u16 *statSums;
    int *statValues;
    u8 ivs = 0;

    species[0] = 0;
    species[1] = 0;
    species[2] = 0;
    if ((gSaveBlock2Ptr->frontier.domeLvlMode != -gSaveBlock2Ptr->frontier.domeBattleMode) && gSaveBlock2Ptr->frontier.challengeStatus != CHALLENGE_STATUS_SAVING)
        return;

    statSums = AllocZeroed(sizeof(u16) * DOME_TOURNAMENT_TRAINERS_COUNT);
    statValues = AllocZeroed(sizeof(int) * NUM_STATS);
    lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_50;
    zero1 = 0;
    zero2 = 0;

    gSaveBlock2Ptr->frontier.domeLvlMode = zero1 + 1;
    gSaveBlock2Ptr->frontier.domeBattleMode = zero2 + 1;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        do
        {
            if (i < 5)
                trainerId = Random() % 10;
            else if (i < 15)
                trainerId = Random() % 20 + 10;
            else
                trainerId = Random() % 10 + 30;

            for (j = 0; j < i; j++)
            {
                if (DOME_TRAINERS[j].trainerId == trainerId)
                    break;
            }
        } while (j != i);

        DOME_TRAINERS[i].trainerId = trainerId;
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            do
            {
                monId = GetRandomFrontierMonFromSet(trainerId);
                for (k = 0; k < j; k++)
                {
                    // Make sure the mon is valid.
                    int alreadySelectedMonId = DOME_MONS[i][k];
                    if (alreadySelectedMonId == monId
                        || species[0] == gFacilityTrainerMons[monId].species
                        || species[1] == gFacilityTrainerMons[monId].species
                        || gFacilityTrainerMons[alreadySelectedMonId].itemTableId == gFacilityTrainerMons[monId].itemTableId)
                        break;
                }
            } while (k != j);

            DOME_MONS[i][j] = monId;
            species[j] = gFacilityTrainerMons[monId].species;
        }
        DOME_TRAINERS[i].isEliminated = FALSE;
        DOME_TRAINERS[i].eliminatedAt = 0;
        DOME_TRAINERS[i].forfeited = FALSE;
    }

    monLevel = FRONTIER_MAX_LEVEL_50;
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        monTypesBits = 0;
        statSums[i] = 0;
        ivs = GetDomeTrainerMonIvs(DOME_TRAINERS[i].trainerId);
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            CalcDomeMonStats(gFacilityTrainerMons[DOME_MONS[i][j]].species,
                             monLevel, ivs,
                             gFacilityTrainerMons[DOME_MONS[i][j]].evSpread,
                             gFacilityTrainerMons[DOME_MONS[i][j]].nature,
                             statValues);

            statSums[i] += statValues[STAT_ATK];
            statSums[i] += statValues[STAT_DEF];
            statSums[i] += statValues[STAT_SPATK];
            statSums[i] += statValues[STAT_SPDEF];
            statSums[i] += statValues[STAT_SPEED];
            statSums[i] += statValues[STAT_HP];
            monTypesBits |= gBitTable[gSpeciesInfo[gFacilityTrainerMons[DOME_MONS[i][j]].species].types[0]];
            monTypesBits |= gBitTable[gSpeciesInfo[gFacilityTrainerMons[DOME_MONS[i][j]].species].types[1]];
        }

        // Because GF hates temporary vars, trainerId acts like monTypesCount here.
        for (trainerId = 0, j = 0; j < 32; j++)
        {
            if (monTypesBits & 1)
                trainerId++;
            monTypesBits >>= 1;
        }
        statSums[i] += (trainerId * monLevel) / 20;
    }

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT - 1; i++)
    {
        for (j = i + 1; j < DOME_TOURNAMENT_TRAINERS_COUNT; j++)
        {
            if (statSums[i] < statSums[j])
            {
                SwapDomeTrainers(i, j, statSums);
            }
            else if (statSums[i] == statSums[j])
            {
                if (DOME_TRAINERS[i].trainerId > DOME_TRAINERS[j].trainerId)
                    SwapDomeTrainers(i, j, statSums);
            }
        }
    }

    Free(statSums);
    Free(statValues);

    for (i = 0; i < DOME_ROUNDS_COUNT; i++)
        DecideRoundWinners(i);

    gSaveBlock2Ptr->frontier.lvlMode = lvlMode;
}

static int TrainerIdToTournamentId(u16 trainerId)
{
    int i;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (DOME_TRAINERS[i].trainerId == trainerId)
            break;
    }

    return i;
}

// The same as the above one, but has global scope.
int TrainerIdToDomeTournamentId(u16 trainerId)
{
    int i;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (DOME_TRAINERS[i].trainerId == trainerId)
            break;
    }

    return i;
}

static u8 GetOpposingNPCTournamentIdByRound(u8 tournamentId, u8 round)
{
    u8 tournamentIds[2];
    BufferDomeWinString(sTrainerAndRoundToLastMatchCardNum[sTournamentIdToPairedTrainerIds[tournamentId] / 2][round] - 16, tournamentIds);
    if (tournamentId == tournamentIds[0])
        return tournamentIds[1];
    else
        return tournamentIds[0];
}

// Determines which trainers won in the NPC vs NPC battles
static void DecideRoundWinners(u8 roundId)
{
    int i;
    int moveSlot, monId1, monId2;
    int tournamentId1, tournamentId2;
    int species;
    int points1 = 0, points2 = 0;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (DOME_TRAINERS[i].isEliminated || DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
            continue;

        tournamentId1 = i;
        tournamentId2 = TournamentIdOfOpponent(roundId, DOME_TRAINERS[tournamentId1].trainerId);
        // Frontier Brain always wins, check tournamentId1.
        if (DOME_TRAINERS[tournamentId1].trainerId == TRAINER_FRONTIER_BRAIN && tournamentId2 != 0xFF)
        {
            DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
            DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
            gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = GetWinningMove(tournamentId1, tournamentId2, roundId);
        }
        // Frontier Brain always wins, check tournamentId2.
        else if (DOME_TRAINERS[tournamentId2].trainerId == TRAINER_FRONTIER_BRAIN && tournamentId1 != 0xFF)
        {
            DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
            DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
            gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = GetWinningMove(tournamentId2, tournamentId1, roundId);
        }
        // Decide which one of two trainers wins!
        else if (tournamentId2 != 0xFF)
        {
            // BUG: points1 and points2 are not cleared at the beginning of the loop resulting in not fair results.
            #ifdef BUGFIX
            points1 = 0;
            points2 = 0;
            #endif

            // Calculate points for both trainers.
            for (monId1 = 0; monId1 < FRONTIER_PARTY_SIZE; monId1++)
            {
                for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
                {
                    for (monId2 = 0; monId2 < FRONTIER_PARTY_SIZE; monId2++)
                    {
                        points1 += GetTypeEffectivenessPoints(gFacilityTrainerMons[DOME_MONS[tournamentId1][monId1]].moves[moveSlot],
                                                gFacilityTrainerMons[DOME_MONS[tournamentId2][monId2]].species, EFFECTIVENESS_MODE_AI_VS_AI);
                    }
                }
                species = gFacilityTrainerMons[DOME_MONS[tournamentId1][monId1]].species;
                points1 += ( gSpeciesInfo[species].baseHP
                           + gSpeciesInfo[species].baseAttack
                           + gSpeciesInfo[species].baseDefense
                           + gSpeciesInfo[species].baseSpeed
                           + gSpeciesInfo[species].baseSpAttack
                           + gSpeciesInfo[species].baseSpDefense) / 10;
            }
            // Random part of the formula.
            points1 += (Random() & 0x1F);
            // Favor trainers with higher id;
            points1 += tournamentId1;

            for (monId1 = 0; monId1 < FRONTIER_PARTY_SIZE; monId1++)
            {
                for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
                {
                    for (monId2 = 0; monId2 < FRONTIER_PARTY_SIZE; monId2++)
                    {
                        points2 += GetTypeEffectivenessPoints(gFacilityTrainerMons[DOME_MONS[tournamentId2][monId1]].moves[moveSlot],
                                                gFacilityTrainerMons[DOME_MONS[tournamentId1][monId2]].species, EFFECTIVENESS_MODE_AI_VS_AI);
                    }
                }
                species = gFacilityTrainerMons[DOME_MONS[tournamentId2][monId1]].species;
                points2 += ( gSpeciesInfo[species].baseHP
                           + gSpeciesInfo[species].baseAttack
                           + gSpeciesInfo[species].baseDefense
                           + gSpeciesInfo[species].baseSpeed
                           + gSpeciesInfo[species].baseSpAttack
                           + gSpeciesInfo[species].baseSpDefense) / 10;
            }
            // Random part of the formula.
            points2 += (Random() & 0x1F);
            // Favor trainers with higher id;
            points2 += tournamentId2;

            if (points1 > points2)
            {
                DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = GetWinningMove(tournamentId1, tournamentId2, roundId);
            }
            else if (points1 < points2)
            {
                DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = GetWinningMove(tournamentId2, tournamentId1, roundId);
            }
            // Points are the same, so we favor the one with the higher id.
            else if (tournamentId1 > tournamentId2)
            {
                DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = GetWinningMove(tournamentId1, tournamentId2, roundId);
            }
            else
            {
                DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = GetWinningMove(tournamentId2, tournamentId1, roundId);
            }
        }
    }
}

static void CopyDomeTrainerName(u8 *str, u16 trainerId)
{
    int i = 0;
    SetFacilityPtrsGetLevel();

    if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CopyDomeBrainTrainerName(str);
    }
    else
    {
        if (trainerId == TRAINER_PLAYER)
        {
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                str[i] = gSaveBlock2Ptr->playerName[i];
        }
        else if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                str[i] = gFacilityTrainers[trainerId].trainerName[i];
        }
        str[i] = EOS;
    }
}

static u8 GetDomeBrainTrainerPicId(void)
{
    return gTrainers[TRAINER_TUCKER].trainerPic;
}

static u8 GetDomeBrainTrainerClass(void)
{
    return gTrainers[TRAINER_TUCKER].trainerClass;
}

static void CopyDomeBrainTrainerName(u8 *str)
{
    int i;

    for (i = 0; i < PLAYER_NAME_LENGTH; i++)
        str[i] = gTrainers[TRAINER_TUCKER].trainerName[i];
    str[i] = EOS;
}
