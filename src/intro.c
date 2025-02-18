#include "global.h"
#include "main.h"
#include "palette.h"
#include "scanline_effect.h"
#include "task.h"
#include "title_screen.h"
#include "libgcnmultiboot.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "link.h"
#include "multiboot_pokemon_colosseum.h"
#include "load_save.h"
#include "save.h"
#include "new_game.h"
#include "m4a.h"
#include "random.h"
#include "decompress.h"
#include "constants/songs.h"
#include "intro_credits_graphics.h"
#include "trig.h"
#include "intro.h"
#include "graphics.h"
#include "sound.h"
#include "util.h"
#include "title_screen.h"
#include "constants/rgb.h"
#include "constants/battle_anim.h"

/*
    The intro is grouped into the following scenes
    Scene 0. Copyright screen
    Scene 1. GF Logo, pan up over plants, Flygon silhouette goes by
    Scene 2. Player biking on path, joined by Pokémon
    Scene 3. A fight between Groudon/Kyogre ends with Rayquaza

    After this it progresses to the title screen
*/

// Scene 1 main tasks
static void Task_Scene1_Load(u8);
static void Task_Scene1_FadeIn(u8);
static void Task_Scene1_WaterDrops(u8);
static void Task_Scene1_PanUp(u8);
static void Task_Scene1_End(u8);

// Scene 1 supplemental functions
static void IntroResetGpuRegs(void);
static u8 CreateGameFreakLogoSprites(s16, s16, s16);
static void Task_BlendLogoIn(u8);
static void Task_BlendLogoOut(u8);
static void Task_CreateSparkles(u8);
static u8 CreateWaterDrop(s16, s16, u16, u16, u16, u8);
static void SpriteCB_WaterDrop(struct Sprite *sprite);
static void SpriteCB_WaterDrop_Slide(struct Sprite *);
static void SpriteCB_WaterDrop_ReachLeafEnd(struct Sprite *);
static void SpriteCB_WaterDrop_DangleFromLeaf(struct Sprite *);
static void SpriteCB_WaterDrop_Fall(struct Sprite *);
static void SpriteCB_WaterDrop_Ripple(struct Sprite *);
static void SpriteCB_Sparkle(struct Sprite *sprite);
static void SpriteCB_LogoLetter(struct Sprite *sprite);
static void SpriteCB_GameFreakLogo(struct Sprite *sprite);
static void SpriteCB_FlygonSilhouette(struct Sprite *sprite);

// Scene 2 main tasks
static void Task_Scene2_Load(u8);
static void Task_Scene2_CreateSprites(u8);
static void Task_Scene2_BikeRide(u8);
static void Task_Scene2_End(u8);

// Scene 2 supplemental functions
static void SpriteCB_Torchic(struct Sprite *sprite);
static void SpriteCB_Manectric(struct Sprite *sprite);
static void SpriteCB_Volbeat(struct Sprite *sprite);
static void SpriteCB_Flygon(struct Sprite *);
static void SpriteCB_PlayerOnBicycle(struct Sprite *);

// Scene 3 main tasks
static void Task_Scene3_Load(u8);
static void Task_Scene3_SpinPokeball(u8);
static void Task_Scene3_WaitGroudon(u8);
static void Task_Scene3_LoadGroudon(u8);
static void Task_Scene3_InitGroudonBg(u8);
static void Task_Scene3_NarrowWindow(u8);
static void Task_Scene3_EndNarrowWindow(u8);
static void Task_Scene3_StartGroudon(u8);
static void Task_Scene3_Groudon(u8);
static void Task_Scene3_LoadKyogre(u8);
static void Task_Scene3_Kyogre(u8);
static void Task_Scene3_LoadClouds1(u8);
static void Task_Scene3_LoadClouds2(u8);
static void Task_Scene3_InitClouds(u8);
static void Task_Scene3_Clouds(u8);
static void Task_Scene3_LoadLightning(u8);
static void Task_Scene3_Lightning(u8);
static void Task_Scene3_LoadRayquazaAttack(u8);
static void Task_Scene3_Rayquaza(u8);
static void Task_EndIntroMovie(u8);

// Scene 3 supplemental functions
static void CreateGroudonRockSprites(u8);
static void CreateKyogreBubbleSprites_Body(u8);
static void CreateKyogreBubbleSprites_Fins(void);
static void Task_RayquazaAttack(u8);
static void SpriteCB_GroudonRocks(struct Sprite *);
static void SpriteCB_KyogreBubbles(struct Sprite *sprite);
static void SpriteCB_Lightning(struct Sprite *sprite);
static void SpriteCB_RayquazaOrb(struct Sprite *sprite);

static void MainCB2_EndIntro(void);

extern const struct CompressedSpriteSheet gBattleAnimPicTable[];
extern const struct CompressedSpritePalette gBattleAnimPaletteTable[];
extern const struct SpriteTemplate gAncientPowerRockSpriteTemplate;

enum {
    COPYRIGHT_INITIALIZE,
    COPYRIGHT_START_FADE = 140,
    COPYRIGHT_START_INTRO,
};

#define TAG_VOLBEAT   1500
#define TAG_TORCHIC   1501
#define TAG_MANECTRIC 1502
#define TAG_LIGHTNING 1503
#define TAG_BUBBLES   1504
#define TAG_SPARKLE   1505

#define GFXTAG_DROPS_LOGO 2000
#define PALTAG_DROPS      2000
#define PALTAG_LOGO       2001

#define TAG_FLYGON_SILHOUETTE 2002
#define TAG_RAYQUAZA_ORB      2003

#define COLOSSEUM_GAME_CODE 0x65366347 // "Gc6e" in ASCII

// Used by various tasks and sprites
#define tState data[0]
#define sState data[0]

/*
    gIntroFrameCounter is used as a persistent timer throughout the
    intro cinematic. At various points it's used to determine when
    to trigger actions or progress through the cutscene.
    The values for these are defined contiguously below.
*/
#define TIMER_BIG_DROP_START             76
#define TIMER_LOGO_APPEAR               128
#define TIMER_LOGO_LETTERS_COLOR        144
#define TIMER_BIG_DROP_FALLS            251
#define TIMER_LOGO_BLEND_OUT            256
#define TIMER_LOGO_DISAPPEAR            272
#define TIMER_SMALL_DROP_1              368
#define TIMER_SMALL_DROP_2              384
#define TIMER_SPARKLES                  560
#define TIMER_FLYGON_SILHOUETTE_APPEAR  832
#define TIMER_END_PAN_UP                904
#define TIMER_END_SCENE_1              1007
#define TIMER_START_SCENE_2            1026
#define TIMER_MANECTRIC_ENTER          1088
#define TIMER_PLAYER_DRIFT_BACK        1109
#define TIMER_MANECTRIC_RUN_CIRCULAR   1168
#define TIMER_PLAYER_MOVE_FORWARD      1214
#define TIMER_TORCHIC_ENTER            1224
#define TIMER_FLYGON_ENTER             1394
#define TIMER_PLAYER_MOVE_BACKWARD     1398
#define TIMER_PLAYER_HOLD_POSITION     1576
#define TIMER_PLAYER_EXIT              1727
#define TIMER_TORCHIC_SPEED_UP         1735
#define TIMER_TORCHIC_EXIT             1856
#define TIMER_END_SCENE_2              1946
#define TIMER_START_SCENE_3            2068
// timer is reset for scene 3
#define TIMER_POKEBALL_FADE              28
#define TIMER_START_LEGENDARIES          43

static EWRAM_DATA u16 sIntroCharacterGender = 0;
static EWRAM_DATA u16 UNUSED sUnusedVar = 0;
static EWRAM_DATA u16 sFlygonYOffset = 0;

COMMON_DATA u32 gIntroFrameCounter = 0;
COMMON_DATA struct GcmbStruct gMultibootProgramStruct = {0};

static const u16 sIntroDrops_Pal[]            = INCBIN_U16("graphics/intro/scene_1/drops.gbapal");
static const u16 sIntroLogo_Pal[]             = INCBIN_U16("graphics/intro/scene_1/logo.gbapal");
static const u32 sIntroDropsLogo_Gfx[]        = INCBIN_U32("graphics/intro/scene_1/drops_logo.4bpp.lz");
static const u16 sIntro1Bg_Pal[]              = INCBIN_U16("graphics/intro/scene_1/bg.gbapal"); // 16 x 16
static const u32 sIntro1Bg0_Tilemap[]         = INCBIN_U32("graphics/intro/scene_1/bg0_map.bin.lz");
static const u32 sIntro1Bg1_Tilemap[]         = INCBIN_U32("graphics/intro/scene_1/bg1_map.bin.lz");
static const u32 sIntro1Bg2_Tilemap[]         = INCBIN_U32("graphics/intro/scene_1/bg2_map.bin.lz");
static const u32 sIntro1Bg3_Tilemap[]         = INCBIN_U32("graphics/intro/scene_1/bg3_map.bin.lz");
static const u32 sIntro1Bg_Gfx[]              = INCBIN_U32("graphics/intro/scene_1/bg.4bpp.lz");
static const u16 sIntroPokeball_Pal[]         = INCBIN_U16("graphics/intro/scene_3/pokeball.gbapal");
static const u32 sIntroPokeball_Tilemap[]     = INCBIN_U32("graphics/intro/scene_3/pokeball_map.bin.lz");
static const u32 sIntroPokeball_Gfx[]         = INCBIN_U32("graphics/intro/scene_3/pokeball.8bpp.lz");
static const u16 sIntroStreaks_Pal[]          = INCBIN_U16("graphics/intro/scene_3/streaks.gbapal"); // Unused
static const u32 sIntroStreaks_Gfx[]          = INCBIN_U32("graphics/intro/scene_3/streaks.4bpp.lz"); // Unused
static const u32 sIntroStreaks_Tilemap[]      = INCBIN_U32("graphics/intro/scene_3/streaks_map.bin.lz"); // Unused
static const u16 sIntroRayquzaOrb_Pal[]       = INCBIN_U16("graphics/intro/scene_3/rayquaza_orb.gbapal");
static const u16 sIntroMisc_Pal[]             = INCBIN_U16("graphics/intro/scene_3/misc.gbapal"); // Unused
static const u32 sIntroMisc_Gfx[]             = INCBIN_U32("graphics/intro/scene_3/misc.4bpp.lz"); // Rayquza orb, and misc unused gfx
static const u16 sIntroFlygonSilhouette_Pal[] = INCBIN_U16("graphics/intro/scene_1/flygon.gbapal");
static const u32 sIntroLati_Gfx[]             = INCBIN_U32("graphics/intro/scene_1/lati.4bpp.lz"); // Unused
static const u8 sUnusedData[] = {
    0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x01, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x02, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x02, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x02, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x00
};
static const struct CompressedSpriteSheet sSpriteSheet_Sparkle[] =
{
    {gIntroSparkle_Gfx, 0x400, TAG_SPARKLE},
    {},
};
static const struct SpritePalette sSpritePalette_Sparkle[] =
{
    {gIntroLightning_Pal, TAG_SPARKLE}, // Lightning palette re-used
    {},
};
static const struct OamData sOamData_Sparkle =
{
    .y = DISPLAY_HEIGHT,
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
static const union AnimCmd sAnim_Sparkle[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(4, 2),
    ANIMCMD_FRAME(8, 2),
    ANIMCMD_FRAME(12, 2),
    ANIMCMD_FRAME(16, 2),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Sparkle[] =
{
    sAnim_Sparkle,
};
static const struct SpriteTemplate sSpriteTemplate_Sparkle =
{
    .tileTag = TAG_SPARKLE,
    .paletteTag = TAG_SPARKLE,
    .oam = &sOamData_Sparkle,
    .anims = sAnims_Sparkle,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Sparkle,
};
static const u8 sSparkleCoords[][2] =
{
    {124, 40},
    {102, 30},
    { 77, 30},
    { 54, 15},
    {148,  9},
    { 63, 28},
    { 93, 40},
    {148, 32},
    {173, 41},
    { 94, 20},
    {208, 38},
    {},
};
static const struct CompressedSpriteSheet sSpriteSheet_RunningPokemon[] =
{
    {gIntroVolbeat_Gfx, 0x400, TAG_VOLBEAT},
    {gIntroTorchic_Gfx, 0xC00, TAG_TORCHIC},
    {gIntroManectric_Gfx, 0x2000, TAG_MANECTRIC},
    {},
};
static const struct SpritePalette sSpritePalettes_RunningPokemon[] =
{
    {gIntroVolbeat_Pal, TAG_VOLBEAT},
    {gIntroTorchic_Pal, TAG_TORCHIC},
    {gIntroManectric_Pal, TAG_MANECTRIC},
    {},
};
static const struct OamData sOamData_Volbeat =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_Volbeat[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(16, 2),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Volbeat[] =
{
    sAnim_Volbeat,
};
static const struct SpriteTemplate sSpriteTemplate_Volbeat =
{
    .tileTag = TAG_VOLBEAT,
    .paletteTag = TAG_VOLBEAT,
    .oam = &sOamData_Volbeat,
    .anims = sAnims_Volbeat,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Volbeat,
};
static const struct OamData sOamData_Torchic =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_Torchic_Walk[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_Torchic_Run[] =
{
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_FRAME(16, 3),
    ANIMCMD_FRAME(32, 3),
    ANIMCMD_FRAME(16, 3),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_Torchic_Trip[] =
{
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_FRAME(64, 6),
    ANIMCMD_FRAME(80, 0),
    ANIMCMD_END,
};
enum {
    TORCHIC_ANIM_WALK,
    TORCHIC_ANIM_RUN,
    TORCHIC_ANIM_TRIP,
};
static const union AnimCmd *const sAnims_Torchic[] =
{
    [TORCHIC_ANIM_WALK] = sAnim_Torchic_Walk,
    [TORCHIC_ANIM_RUN]  = sAnim_Torchic_Run,
    [TORCHIC_ANIM_TRIP] = sAnim_Torchic_Trip,
};
static const struct SpriteTemplate sSpriteTemplate_Torchic =
{
    .tileTag = TAG_TORCHIC,
    .paletteTag = TAG_TORCHIC,
    .oam = &sOamData_Torchic,
    .anims = sAnims_Torchic,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Torchic,
};
static const struct OamData sOamData_Manectric =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_Manectric[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_FRAME(128, 4),
    ANIMCMD_FRAME(192, 4),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Manectric[] =
{
    sAnim_Manectric,
};
static const struct SpriteTemplate sSpriteTemplate_Manectric =
{
    .tileTag = TAG_MANECTRIC,
    .paletteTag = TAG_MANECTRIC,
    .oam = &sOamData_Manectric,
    .anims = sAnims_Manectric,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Manectric,
};
static const struct CompressedSpriteSheet sSpriteSheet_Lightning[] =
{
    {gIntroLightning_Gfx, 0xC00, TAG_LIGHTNING},
    {},
};
static const struct SpritePalette sSpritePalette_Lightning[] =
{
    {gIntroLightning_Pal, TAG_LIGHTNING},
    {},
};
static const struct OamData sOamData_Lightning =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_Lightning_Top[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(48, 2),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_Lightning_Middle[] =
{
    ANIMCMD_FRAME(16, 2),
    ANIMCMD_FRAME(64, 2),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_Lightning_Bottom[] =
{
    ANIMCMD_FRAME(32, 2),
    ANIMCMD_FRAME(80, 2),
    ANIMCMD_END,
};
static const union AnimCmd *const sAnims_Lightning[] =
{
    sAnim_Lightning_Top,
    sAnim_Lightning_Middle,
    sAnim_Lightning_Bottom,
};
static const struct SpriteTemplate sSpriteTemplate_Lightning =
{
    .tileTag = TAG_LIGHTNING,
    .paletteTag = TAG_LIGHTNING,
    .oam = &sOamData_Lightning,
    .anims = sAnims_Lightning,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Lightning,
};
// x coord, anim number, speed
// Smaller anim numbers are larger rocks, and are given slower speeds
static const s16 sGroudonRockData[][3] =
{
    {104, 0, 0x0C0},
    {142, 3, 0x280},
    { 83, 1, 0x180},
    {155, 0, 0x080},
    { 56, 2, 0x200},
    {174, 1, 0x100},
};
static const struct CompressedSpriteSheet sSpriteSheet_Bubbles[] =
{
    {gIntroBubbles_Gfx, 0x600, TAG_BUBBLES},
    {},
};
static const struct SpritePalette sSpritePalette_Bubbles[] =
{
    {gIntroBubbles_Pal, TAG_BUBBLES},
    {},
};
#define NUM_BUBBLES_IN_SET 6
// x coord, y coord, delay before animation
// Can be produced in two different sets depending on the function called to create the sprites
static const s16 sKyogreBubbleData[NUM_BUBBLES_IN_SET * 2][3] =
{
    // Set 1, for Kyogre's body
    { 66,  64,  1},
    { 96,  96,  8},
    {128,  64,  1},
    {144,  48,  8},
    {160,  72,  1},
    {176,  96,  8},
    // Set 2, for Kyogre's fins
    { 96,  96,  4},
    {112, 104,  8},
    {128,  96,  4},
    { 88,  32,  4},
    {104,  24,  8},
    {120,  32,  4},
};
static const struct OamData sOamData_Bubbles =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_Bubbles[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(24, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_END,
};
static const union AnimCmd *const sAnims_Bubbles[] =
{
    sAnim_Bubbles,
};
static const struct SpriteTemplate sSpriteTemplate_Bubbles =
{
    .tileTag = TAG_BUBBLES,
    .paletteTag = TAG_BUBBLES,
    .oam = &sOamData_Bubbles,
    .anims = sAnims_Bubbles,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_KyogreBubbles,
};
static const struct OamData sOamData_WaterDrop =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
enum {
    DROP_ANIM_UPPER_HALF,
    DROP_ANIM_LOWER_HALF,
    DROP_ANIM_REFLECTION,
    DROP_ANIM_RIPPLE,
};
static const union AnimCmd sAnim_WaterDrop_UpperHalf[] =
{
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_LowerHalf[] =
{
    ANIMCMD_FRAME(24, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_Reflection[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_Ripple[] =
{
    ANIMCMD_FRAME(48, 8),
    ANIMCMD_END,
};
static const union AnimCmd *const sAnims_WaterDrop[] =
{
    [DROP_ANIM_UPPER_HALF] = sAnim_WaterDrop_UpperHalf,
    [DROP_ANIM_LOWER_HALF] = sAnim_WaterDrop_LowerHalf,
    [DROP_ANIM_REFLECTION] = sAnim_WaterDrop_Reflection,
    [DROP_ANIM_RIPPLE]     = sAnim_WaterDrop_Ripple,
};
static const struct SpriteTemplate sSpriteTemplate_WaterDrop =
{
    .tileTag = GFXTAG_DROPS_LOGO,
    .paletteTag = PALTAG_DROPS,
    .oam = &sOamData_WaterDrop,
    .anims = sAnims_WaterDrop,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_WaterDrop,
};
static const union AnimCmd sAnim_PlayerBicycle_Fast[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_FRAME(128, 4),
    ANIMCMD_FRAME(192, 4),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_PlayerBicycle_Slow[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(64, 8),
    ANIMCMD_FRAME(128, 8),
    ANIMCMD_FRAME(192, 8),
    ANIMCMD_JUMP(0),
};
// The below two animations appear to be copied from the Credits version
// of the player graphic, where additional frames are present to show
// the player turning around to look at their rival.
// They go unused here, and if they were used they'd overflow beyond
// the player graphics data.
// The above sAnim_PlayerBicycle_Slow, while valid, is likewise unused
static const union AnimCmd sAnim_PlayerBicycle_LookBack[] =
{
    ANIMCMD_FRAME(256, 4),
    ANIMCMD_FRAME(320, 4),
    ANIMCMD_FRAME(384, 4),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PlayerBicycle_LookForward[] =
{
    ANIMCMD_FRAME(384, 16),
    ANIMCMD_FRAME(320, 16),
    ANIMCMD_FRAME(256, 16),
    ANIMCMD_END,
};
static const union AnimCmd *const sAnims_PlayerBicycle[] =
{
    sAnim_PlayerBicycle_Fast,
    sAnim_PlayerBicycle_Slow,
    sAnim_PlayerBicycle_LookBack,
    sAnim_PlayerBicycle_LookForward,
};
static const struct OamData sOamData_GameFreakLetter =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
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
static const struct OamData sOamData_PresentsLetter =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const struct OamData sOamData_GameFreakLogo =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_BLEND,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_GameFreakLetter_G[] =
{
    ANIMCMD_FRAME(80, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_A[] =
{
    ANIMCMD_FRAME(84, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_M[] =
{
    ANIMCMD_FRAME(88, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_E[] =
{
    ANIMCMD_FRAME(92, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_F[] =
{
    ANIMCMD_FRAME(96, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_R[] =
{
    ANIMCMD_FRAME(100, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_K[] =
{
    ANIMCMD_FRAME(104, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_P[] =
{
    ANIMCMD_FRAME(112, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_R[] =
{
    ANIMCMD_FRAME(113, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_E[] =
{
    ANIMCMD_FRAME(114, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_S[] =
{
    ANIMCMD_FRAME(115, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_N[] =
{
    ANIMCMD_FRAME(116, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_T[] =
{
    ANIMCMD_FRAME(117, 8),
    ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLogo[] =
{
    ANIMCMD_FRAME(128, 8),
    ANIMCMD_END,
};

enum {
    GAMEFREAK_G,
    GAMEFREAK_A,
    GAMEFREAK_M,
    GAMEFREAK_E,
    GAMEFREAK_F,
    GAMEFREAK_R,
    GAMEFREAK_K,
};
enum {
    PRESENTS_P,
    PRESENTS_R,
    PRESENTS_E,
    PRESENTS_S,
    PRESENTS_N,
    PRESENTS_T,
};
static const union AnimCmd *const sAnims_GameFreakLetter[] =
{
    [GAMEFREAK_G] = sAnim_GameFreakLetter_G,
    [GAMEFREAK_A] = sAnim_GameFreakLetter_A,
    [GAMEFREAK_M] = sAnim_GameFreakLetter_M,
    [GAMEFREAK_E] = sAnim_GameFreakLetter_E,
    [GAMEFREAK_F] = sAnim_GameFreakLetter_F,
    [GAMEFREAK_R] = sAnim_GameFreakLetter_R,
    [GAMEFREAK_K] = sAnim_GameFreakLetter_K,
};
static const union AnimCmd *const sAnims_PresentsLetter[] =
{
    [PRESENTS_P] = sAnim_PresentsLetter_P,
    [PRESENTS_R] = sAnim_PresentsLetter_R,
    [PRESENTS_E] = sAnim_PresentsLetter_E,
    [PRESENTS_S] = sAnim_PresentsLetter_S,
    [PRESENTS_N] = sAnim_PresentsLetter_N,
    [PRESENTS_T] = sAnim_PresentsLetter_T,
};
static const union AnimCmd *const sAnims_GameFreakLogo[] =
{
    sAnim_GameFreakLogo,
};
#define NUM_GF_LETTERS 9 // Letters in "Game Freak"
static const s16 sGameFreakLetterData[NUM_GF_LETTERS][2] =
{
    // Letter,   x offset
    {GAMEFREAK_G, -72},
    {GAMEFREAK_A, -56},
    {GAMEFREAK_M, -40},
    {GAMEFREAK_E, -24},
    {GAMEFREAK_F,   8},
    {GAMEFREAK_R,  24},
    {GAMEFREAK_E,  40},
    {GAMEFREAK_A,  56},
    {GAMEFREAK_K,  72},
};
static const s16 sPresentsLetterData[][2] =
{
    // Letter,   x offset
    {PRESENTS_P, -28},
    {PRESENTS_R, -20},
    {PRESENTS_E, -12},
    {PRESENTS_S,  -4},
    {PRESENTS_E,   4},
    {PRESENTS_N,  12},
    {PRESENTS_T,  20},
    {PRESENTS_S,  28},
};
static const union AffineAnimCmd sAffineAnim_GameFreak_Small[] =
{
    AFFINEANIMCMD_FRAME(128, 128, 0, 0),
    AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowAndShrink[] =
{
    AFFINEANIMCMD_FRAME(128, 128, 0, 0),
    AFFINEANIMCMD_FRAME(16, 16, 0, 16),
    AFFINEANIMCMD_FRAME(-16, -16, 0, 8),
    AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowBig[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(8, 8, 0, 48),
    AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowMedium[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(2, 2, 0, 48),
    AFFINEANIMCMD_END,
};
static const union AffineAnimCmd *const sAffineAnims_GameFreak[] =
{
    sAffineAnim_GameFreak_Small,         // Initialize letters while still invisible
    sAffineAnim_GameFreak_GrowAndShrink, // For letters appearing. Logo does this too, but while it's invisible
    sAffineAnim_GameFreak_GrowBig,       // For letters disappearing
    sAffineAnim_GameFreak_GrowMedium,    // For logo disappearing
};
static const u16 sGameFreakLettersMoveSpeed[NUM_GF_LETTERS] =
{
    256, // G
    192, // A
    128, // M
     64, // E
      0, // F
     64, // R
    128, // E
    192, // A
    256  // K
};
static const struct SpriteTemplate sSpriteTemplate_GameFreakLetter =
{
    .tileTag = GFXTAG_DROPS_LOGO,
    .paletteTag = PALTAG_LOGO,
    .oam = &sOamData_GameFreakLetter,
    .anims = sAnims_GameFreakLetter,
    .images = NULL,
    .affineAnims = sAffineAnims_GameFreak,
    .callback = SpriteCB_LogoLetter,
};
// Unused
static const struct SpriteTemplate sSpriteTemplate_PresentsLetter =
{
    .tileTag = GFXTAG_DROPS_LOGO,
    .paletteTag = PALTAG_LOGO,
    .oam = &sOamData_PresentsLetter,
    .anims = sAnims_PresentsLetter,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_LogoLetter,
};
static const struct SpriteTemplate sSpriteTemplate_GameFreakLogo =
{
    .tileTag = GFXTAG_DROPS_LOGO,
    .paletteTag = PALTAG_LOGO,
    .oam = &sOamData_GameFreakLogo,
    .anims = sAnims_GameFreakLogo,
    .images = NULL,
    .affineAnims = sAffineAnims_GameFreak,
    .callback = SpriteCB_GameFreakLogo,
};
static const u8 sGameFreakLetterStartDelays[NUM_GF_LETTERS] =
{
    0,  // G
    23, // A
    23, // M
    49, // E
    62, // F
    36, // R
    36, // E
    10, // A
    10  // K
};
static const struct OamData sOamData_FlygonSilhouette =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_FlygonSilhouette[] =
{
    ANIMCMD_FRAME(0, 10),
    ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_FlygonSilhouette[] =
{
    sAnim_FlygonSilhouette,
};
static const struct SpriteTemplate sSpriteTemplate_FlygonSilhouette =
{
    .tileTag = TAG_FLYGON_SILHOUETTE,
    .paletteTag = TAG_FLYGON_SILHOUETTE,
    .oam = &sOamData_FlygonSilhouette,
    .anims = sAnims_FlygonSilhouette,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_FlygonSilhouette,
};
static const struct CompressedSpriteSheet sSpriteSheet_WaterDropsAndLogo[] =
{
    {sIntroDropsLogo_Gfx, 0x1400, GFXTAG_DROPS_LOGO},
    {},
};
static const struct CompressedSpriteSheet sSpriteSheet_FlygonSilhouette[] =
{
    {gIntroFlygonSilhouette_Gfx, 0x400, TAG_FLYGON_SILHOUETTE},
    {},
};
static const struct SpritePalette sSpritePalettes_Intro1[] =
{
    {sIntroDrops_Pal, PALTAG_DROPS},
    {sIntroLogo_Pal, PALTAG_LOGO},
    {sIntroFlygonSilhouette_Pal, TAG_FLYGON_SILHOUETTE},
    {},
};
static const struct OamData sOamData_RayquazaOrb =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};
static const union AnimCmd sAnim_RayquazaOrb[] =
{
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_END,
};
static const union AnimCmd *const sAnims_RayquazaOrb[] =
{
    sAnim_RayquazaOrb,
};
static const struct SpriteTemplate sSpriteTemplate_RayquazaOrb =
{
    .tileTag = TAG_RAYQUAZA_ORB,
    .paletteTag = TAG_RAYQUAZA_ORB,
    .oam = &sOamData_RayquazaOrb,
    .anims = sAnims_RayquazaOrb,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_RayquazaOrb,
};
static const struct CompressedSpriteSheet sSpriteSheet_RayquazaOrb[] =
{
    {sIntroMisc_Gfx, 0xA00, TAG_RAYQUAZA_ORB},
    {},
};
static const struct SpritePalette sSpritePalette_RayquazaOrb[] =
{
    {sIntroRayquzaOrb_Pal, TAG_RAYQUAZA_ORB},
    {},
};


static void VBlankCB_Intro(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void MainCB2_Intro(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    if (gMain.newKeys != 0 && !gPaletteFade.active)
        SetMainCallback2(MainCB2_EndIntro);
    else if (gIntroFrameCounter != -1)
        gIntroFrameCounter++;
}

static void MainCB2_EndIntro(void)
{
    if (!UpdatePaletteFade())
        SetMainCallback2(CB2_InitTitleScreen);
}

static void LoadCopyrightGraphics(u16 tilesetAddress, u16 tilemapAddress, u16 paletteOffset)
{
    LZ77UnCompVram(gIntroCopyright_Gfx, (void *)(VRAM + tilesetAddress));
    LZ77UnCompVram(gIntroCopyright_Tilemap, (void *)(VRAM + tilemapAddress));
    LoadPalette(gIntroCopyright_Pal, paletteOffset, PLTT_SIZE_4BPP);
}

static void SerialCB_CopyrightScreen(void)
{
    GameCubeMultiBoot_HandleSerialInterrupt(&gMultibootProgramStruct);
}

static u8 SetUpCopyrightScreen(void)
{
    switch (gMain.state)
    {
    case COPYRIGHT_INITIALIZE:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        *(u16 *)PLTT = RGB_WHITE;
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        CpuFill32(0, (void *)OAM, OAM_SIZE);
        CpuFill16(0, (void *)(PLTT + 2), PLTT_SIZE - 2);
        ResetPaletteFade();
        LoadCopyrightGraphics(0, 0x3800, BG_PLTT_ID(0));
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
        SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0)
                                   | BGCNT_CHARBASE(0)
                                   | BGCNT_SCREENBASE(7)
                                   | BGCNT_16COLOR
                                   | BGCNT_TXT256x256);
        EnableInterrupts(INTR_FLAG_VBLANK);
        SetVBlankCallback(VBlankCB_Intro);
        REG_DISPCNT = DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON;
        SetSerialCallback(SerialCB_CopyrightScreen);
        GameCubeMultiBoot_Init(&gMultibootProgramStruct);
    default:
        UpdatePaletteFade();
        gMain.state++;
        GameCubeMultiBoot_Main(&gMultibootProgramStruct);
        break;
    case COPYRIGHT_START_FADE:
        GameCubeMultiBoot_Main(&gMultibootProgramStruct);
        if (gMultibootProgramStruct.gcmb_field_2 != 1)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gMain.state++;
        }
        break;
    case COPYRIGHT_START_INTRO:
        if (UpdatePaletteFade())
            break;
        CreateTask(Task_Scene1_Load, 0);
        SetMainCallback2(MainCB2_Intro);
        if (gMultibootProgramStruct.gcmb_field_2 != 0)
        {
            if (gMultibootProgramStruct.gcmb_field_2 == 2)
            {
                // check the multiboot ROM header game code to see if we already did this
                if (*(u32 *)(EWRAM_START + 0xAC) == COLOSSEUM_GAME_CODE)
                {
                    CpuCopy16(&gMultiBootProgram_PokemonColosseum_Start, (void *)EWRAM_START, sizeof(gMultiBootProgram_PokemonColosseum_Start));
                    *(u32 *)(EWRAM_START + 0xAC) = COLOSSEUM_GAME_CODE;
                }
                GameCubeMultiBoot_ExecuteProgram(&gMultibootProgramStruct);
            }
        }
        else
        {
            GameCubeMultiBoot_Quit();
            SetSerialCallback(SerialCB);
        }
        return 0;
    }

    return 1;
}

void CB2_InitCopyrightScreenAfterBootup(void)
{
    if (!SetUpCopyrightScreen())
    {
        SetSaveBlocksPointers(GetSaveBlocksPointersBaseOffset());
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        LoadGameSave(SAVE_NORMAL);
        if (gSaveFileStatus == SAVE_STATUS_EMPTY || gSaveFileStatus == SAVE_STATUS_CORRUPT)
            Sav2_ClearSetDefault();
        SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound);
        InitHeap(gHeap, HEAP_SIZE);
    }
}

void CB2_InitCopyrightScreenAfterTitleScreen(void)
{
    SetUpCopyrightScreen();
}

#define sBigDropSpriteId data[0]

static void Task_Scene1_Load(u8 taskId)
{
    SetVBlankCallback(NULL);
    sIntroCharacterGender = MOD(Random(), GENDER_COUNT);
    IntroResetGpuRegs();
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 80);
    SetGpuReg(REG_OFFSET_BG1VOFS, 24);
    SetGpuReg(REG_OFFSET_BG0VOFS, 40);
    LZ77UnCompVram(sIntro1Bg_Gfx, (void *)VRAM);
    LZ77UnCompVram(sIntro1Bg0_Tilemap, (void *)(BG_CHAR_ADDR(2)));
    DmaClear16(3, BG_SCREEN_ADDR(17), BG_SCREEN_SIZE);
    LZ77UnCompVram(sIntro1Bg1_Tilemap, (void *)(BG_SCREEN_ADDR(18)));
    DmaClear16(3, BG_SCREEN_ADDR(19), BG_SCREEN_SIZE);
    LZ77UnCompVram(sIntro1Bg2_Tilemap, (void *)(BG_SCREEN_ADDR(20)));
    DmaClear16(3, BG_SCREEN_ADDR(21), BG_SCREEN_SIZE);
    LZ77UnCompVram(sIntro1Bg3_Tilemap, (void *)(BG_SCREEN_ADDR(22)));
    DmaClear16(3, BG_SCREEN_ADDR(23), BG_SCREEN_SIZE);
    LoadPalette(sIntro1Bg_Pal, BG_PLTT_ID(0), sizeof(sIntro1Bg_Pal));
    SetGpuReg(REG_OFFSET_BG3CNT, BGCNT_PRIORITY(3) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(22) | BGCNT_16COLOR | BGCNT_TXT256x512);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(20) | BGCNT_16COLOR | BGCNT_TXT256x512);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(18) | BGCNT_16COLOR | BGCNT_TXT256x512);
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(16) | BGCNT_16COLOR | BGCNT_TXT256x512);
    LoadCompressedSpriteSheet(sSpriteSheet_WaterDropsAndLogo);
    LoadCompressedSpriteSheet(sSpriteSheet_FlygonSilhouette);
    LoadSpritePalettes(sSpritePalettes_Intro1);
    LoadCompressedSpriteSheet(sSpriteSheet_Sparkle);
    LoadSpritePalettes(sSpritePalette_Sparkle);
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(15) + 0], PLTT_SIZEOF(16 - 0));
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(14) + 1], PLTT_SIZEOF(16 - 1) + 1); // Copying an extra half color?
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(13) + 2], PLTT_SIZEOF(16 - 2));
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(12) + 3], PLTT_SIZEOF(16 - 3));
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(11) + 4], PLTT_SIZEOF(16 - 4));
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID(10) + 5], PLTT_SIZEOF(16 - 5));
    CpuCopy16(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferUnfaded[OBJ_PLTT_ID( 9) + 6], PLTT_SIZEOF(16 - 6));
    CreateGameFreakLogoSprites(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 0);
    gTasks[taskId].sBigDropSpriteId = CreateWaterDrop(236, -14, 0x200, 1, 0x78, FALSE);
    gTasks[taskId].func = Task_Scene1_FadeIn;
}

static void Task_Scene1_FadeIn(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    SetVBlankCallback(VBlankCB_Intro);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
    gTasks[taskId].func = Task_Scene1_WaterDrops;
    gIntroFrameCounter = 0;
    m4aSongNumStart(MUS_INTRO);
    ResetSerial();
}

// Task data for Task_Scene1_PanUp
#define tBg2PosHi data[1]
#define tBg2PosLo data[2]
#define tBg1PosHi data[3]
#define tBg1PosLo data[4]
#define tBg3PosHi data[5]
#define tBg3PosLo data[6]

static void Task_Scene1_WaterDrops(u8 taskId)
{
    if (gIntroFrameCounter == TIMER_BIG_DROP_START)
        gSprites[gTasks[taskId].sBigDropSpriteId].sState = 1;

    if (gIntroFrameCounter == TIMER_LOGO_APPEAR)
        CreateTask(Task_BlendLogoIn, 0);

    if (gIntroFrameCounter == TIMER_BIG_DROP_FALLS)
        gSprites[gTasks[taskId].sBigDropSpriteId].sState = 2;

    if (gIntroFrameCounter == TIMER_LOGO_BLEND_OUT)
        CreateTask(Task_BlendLogoOut, 0);

    if (gIntroFrameCounter == TIMER_SMALL_DROP_1)
        CreateWaterDrop(48, 0, 0x400, 5, 0x70, TRUE);

    if (gIntroFrameCounter == TIMER_SMALL_DROP_2)
        CreateWaterDrop(200, 60, 0x400, 9, 0x80, TRUE);

    if (gIntroFrameCounter == TIMER_SPARKLES)
        CreateTask(Task_CreateSparkles, 0);

    if (gIntroFrameCounter > TIMER_SPARKLES)
    {
        gTasks[taskId].tBg2PosHi = 80;
        gTasks[taskId].tBg2PosLo = 0;
        gTasks[taskId].tBg1PosHi = 24;
        gTasks[taskId].tBg1PosLo = 0;
        gTasks[taskId].tBg3PosHi = 40;
        gTasks[taskId].tBg3PosLo = 0;
        gTasks[taskId].func = Task_Scene1_PanUp;
    }
}

#define tDelay       data[1]
#define tTimer       data[2]
#define tTimerSteps  data[3]
#define tNumSparkles data[4]

static void Task_CreateSparkles(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (++tTimer & 1)
        tTimerSteps++;

    switch (tState)
    {
    case 0:
        CreateSprite(&sSpriteTemplate_Sparkle, sSparkleCoords[tNumSparkles][0], sSparkleCoords[tNumSparkles][1] + tTimerSteps, 0);
        tState++;
        tDelay = 12;
        tNumSparkles++;
        break;
    case 1:
        if (--tDelay == 0)
            tState = 0;
        break;
    }

    if (tTimerSteps > 60)
        DestroyTask(taskId);
}

#undef tDelay
#undef tTimer
#undef tTimerSteps
#undef tNumSparkles

#define sTimer data[0]
static void SpriteCB_Sparkle(struct Sprite *sprite)
{
    if (++sprite->sTimer == 12)
        DestroySprite(sprite);
}
#undef sTimer

static void Task_Scene1_PanUp(u8 taskId)
{
    if (gIntroFrameCounter < TIMER_END_PAN_UP)
    {
        s32 offset;

        // Slide bg 2 downward
        offset = (gTasks[taskId].tBg2PosHi << 16) + (u16)gTasks[taskId].tBg2PosLo;
        offset -= 0x6000;
        gTasks[taskId].tBg2PosHi = offset >> 16;
        gTasks[taskId].tBg2PosLo = offset;
        SetGpuReg(REG_OFFSET_BG2VOFS, gTasks[taskId].tBg2PosHi);

        // Slide bg 1 downward
        offset = (gTasks[taskId].tBg1PosHi << 16) + (u16)gTasks[taskId].tBg1PosLo;
        offset -= 0x8000;
        gTasks[taskId].tBg1PosHi = offset >> 16;
        gTasks[taskId].tBg1PosLo = offset;
        SetGpuReg(REG_OFFSET_BG1VOFS, gTasks[taskId].tBg1PosHi);

        // Slide bg 3 downward
        offset = (gTasks[taskId].tBg3PosHi << 16) + (u16)gTasks[taskId].tBg3PosLo;
        offset -= 0xC000;
        gTasks[taskId].tBg3PosHi = offset >> 16;
        gTasks[taskId].tBg3PosLo = offset;
        SetGpuReg(REG_OFFSET_BG0VOFS, gTasks[taskId].tBg3PosHi);

        if (gIntroFrameCounter == TIMER_FLYGON_SILHOUETTE_APPEAR)
        {
            // Show Flygon silhouette
            u8 spriteId = CreateSprite(&sSpriteTemplate_FlygonSilhouette, 120, DISPLAY_HEIGHT, 10);
            gSprites[spriteId].invisible = TRUE;
        }
    }
    else
    {
        if (gIntroFrameCounter > TIMER_END_SCENE_1)
        {
            // Fade to white
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITEALPHA);
            gTasks[taskId].func = Task_Scene1_End;
        }
    }
}

static void Task_Scene1_End(u8 taskId)
{
    if (gIntroFrameCounter > TIMER_START_SCENE_2)
        gTasks[taskId].func = Task_Scene2_Load;
}

static void Task_Scene2_Load(u8 taskId)
{
    IntroResetGpuRegs();
    SetVBlankCallback(NULL);
    ResetSpriteData();
    FreeAllSpritePalettes();
    gIntroCredits_MovingSceneryVBase = 0;
    gIntroCredits_MovingSceneryVOffset = 0;
    sFlygonYOffset = 0;
    LoadIntroPart2Graphics(1);
    gTasks[taskId].func = Task_Scene2_CreateSprites;
}

#define tBgAnimTaskId   data[0]
#define tPlayerSpriteId data[1]
#define tFlygonSpriteId data[2]
#define tFlygonTimer    data[3]

static void Task_Scene2_CreateSprites(u8 taskId)
{
    u8 spriteId;

    // Load sprite sheets
    if (sIntroCharacterGender == MALE)
        LoadCompressedSpriteSheet(gSpriteSheet_IntroBrendan);
    else
        LoadCompressedSpriteSheet(gSpriteSheet_IntroMay);

    LoadCompressedSpriteSheet(gSpriteSheet_IntroBicycle);
    LoadCompressedSpriteSheet(gSpriteSheet_IntroFlygon);

    // Load sprite palettes
    for (spriteId = 0; spriteId < ARRAY_COUNT(sSpriteSheet_RunningPokemon) - 1; spriteId++)
        LoadCompressedSpriteSheet(&sSpriteSheet_RunningPokemon[spriteId]);

    LoadSpritePalettes(gSpritePalettes_IntroPlayerFlygon);
    LoadSpritePalettes(sSpritePalettes_RunningPokemon);

    // Create Pokémon and player sprites
    CreateSprite(&sSpriteTemplate_Manectric, DISPLAY_WIDTH + 32, 128, 0);
    CreateSprite(&sSpriteTemplate_Torchic, DISPLAY_WIDTH + 48, 110, 1);

    if (sIntroCharacterGender == MALE)
        spriteId = CreateIntroBrendanSprite(DISPLAY_WIDTH + 32, 100);
    else
        spriteId = CreateIntroMaySprite(DISPLAY_WIDTH + 32, 100);

    gSprites[spriteId].callback = SpriteCB_PlayerOnBicycle;
    gSprites[spriteId].anims = sAnims_PlayerBicycle;
    gTasks[taskId].tPlayerSpriteId = spriteId;
    CreateSprite(&sSpriteTemplate_Volbeat, DISPLAY_WIDTH + 32, 80, 4);
    spriteId = CreateIntroFlygonSprite(-64, 60);
    gSprites[spriteId].callback = SpriteCB_Flygon;
    gTasks[taskId].tFlygonSpriteId = spriteId;

    // Fade in and start bike ride
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
    SetVBlankCallback(VBlankCB_Intro);
    gTasks[taskId].tBgAnimTaskId = CreateBicycleBgAnimationTask(1, 0x4000, 0x400, 0x10);
    SetIntroPart2BgCnt(1);
    gTasks[taskId].func = Task_Scene2_BikeRide;
}

static void Task_Scene2_BikeRide(u8 taskId)
{
    u16 offset;

    if (gIntroFrameCounter == TIMER_TORCHIC_EXIT)
    {
        // Stop the moving scenery/backgrounds, for when the camera fixes on Torchic
        gIntroCredits_MovingSceneryState = INTROCRED_SCENERY_FROZEN;
        DestroyTask(gTasks[taskId].tBgAnimTaskId);
    }

    if (gIntroFrameCounter > TIMER_END_SCENE_2)
    {
        // Fade out to next scene
        BeginNormalPaletteFade(PALETTES_ALL, 8, 0, 16, RGB_WHITEALPHA);
        gTasks[taskId].func = Task_Scene2_End;
    }

    // Check for updates to player/flygon sprites
    // These states are for SpriteCB_PlayerOnBicycle and SpriteCB_Flygon respectively
    if (gIntroFrameCounter == TIMER_PLAYER_DRIFT_BACK)
        gSprites[gTasks[taskId].tPlayerSpriteId].sState = 1;
    if (gIntroFrameCounter == TIMER_PLAYER_MOVE_FORWARD)
        gSprites[gTasks[taskId].tPlayerSpriteId].sState = 0;
    if (gIntroFrameCounter == TIMER_FLYGON_ENTER)
        gSprites[gTasks[taskId].tFlygonSpriteId].sState = 1;
    if (gIntroFrameCounter == TIMER_PLAYER_MOVE_BACKWARD)
        gSprites[gTasks[taskId].tPlayerSpriteId].sState = 2;
    if (gIntroFrameCounter == TIMER_PLAYER_HOLD_POSITION)
        gSprites[gTasks[taskId].tPlayerSpriteId].sState = 3;
    if (gIntroFrameCounter == TIMER_PLAYER_EXIT)
        gSprites[gTasks[taskId].tPlayerSpriteId].sState = 4;

    // Handle flygon's y movement
    offset = Sin(gTasks[taskId].tFlygonTimer >> 2 & 0x7F, 48);
    sFlygonYOffset = offset;
    if (gTasks[taskId].tFlygonTimer < 512)
        gTasks[taskId].tFlygonTimer++;

    // Alternate colors of the trees
    CycleSceneryPalette(0);
}

static void Task_Scene2_End(u8 taskId)
{
    if (gIntroFrameCounter > TIMER_START_SCENE_3)
        gTasks[taskId].func = Task_Scene3_Load;
}

#define sStateDelay data[1]
#define sNextState  data[2]
#define sCosYIdx    data[3]
#define sSinXIdx    data[4]
#define sSinYIdx    data[5]
#define sFig8Loops  data[6]

enum {
    VOLBEAT_WAIT_ENTER,
    VOLBEAT_ENTER,
    VOLBEAT_ZIP_BACKWARD,
    VOLBEAT_ZIP_DOWN,
    VOLBEAT_ZIP_FORWARD,
    VOLBEAT_INIT_FIGURE_8,
    VOLBEAT_FIGURE_8,
    VOLBEAT_EXIT,
    VOLBEAT_WAIT_STATE
};

static void SpriteCB_Volbeat(struct Sprite *sprite)
{
    sprite->sCosYIdx += 4;
    switch (sprite->sState)
    {
    case VOLBEAT_WAIT_ENTER:
        if (++sprite->sStateDelay < 180)
            break;
        sprite->sState++;
        // fallthrough
    case VOLBEAT_ENTER:
        sprite->x -= 4;
        if (sprite->x == 60)
        {
            sprite->sState = VOLBEAT_WAIT_STATE;
            sprite->sStateDelay = 20;
            sprite->sNextState = VOLBEAT_ZIP_BACKWARD;
        }
        break;
    case VOLBEAT_ZIP_BACKWARD:
        sprite->x += 8;
        sprite->y -= 2;
        if (sprite->x == 124)
        {
            sprite->sState = VOLBEAT_WAIT_STATE;
            sprite->sStateDelay = 20;
            sprite->sNextState = VOLBEAT_ZIP_DOWN;
        }
        break;
    case VOLBEAT_ZIP_DOWN:
        sprite->y += 4;
        if (sprite->y == 80)
        {
            sprite->sState = VOLBEAT_WAIT_STATE;
            sprite->sStateDelay = 10;
            sprite->sNextState = VOLBEAT_ZIP_FORWARD;
        }
        break;
    case VOLBEAT_ZIP_FORWARD:
        sprite->x -= 8;
        sprite->y -= 2;
        if (sprite->x == 60)
        {
            sprite->sState = VOLBEAT_WAIT_STATE;
            sprite->sStateDelay = 10;
            sprite->sNextState = VOLBEAT_INIT_FIGURE_8;
        }
        break;
    case VOLBEAT_INIT_FIGURE_8:
        sprite->x += 60;
        sprite->sSinXIdx = 0xC0;
        sprite->sSinYIdx = 0x80;
        sprite->sFig8Loops = 3;
        sprite->sState++;
        // fallthrough
    case VOLBEAT_FIGURE_8:
        sprite->x2 = Sin((u8)sprite->sSinXIdx, 0x3C);
        sprite->y2 = Sin((u8)sprite->sSinYIdx, 0x14);
        sprite->sSinXIdx += 2;
        sprite->sSinYIdx += 4;
        if ((sprite->sSinXIdx & 0xFF) == 64)
        {
            sprite->hFlip = FALSE;
            if (--sprite->sFig8Loops == 0)
            {
                sprite->x += sprite->x2;
                sprite->x2 = 0;
                sprite->sState++;
            }
        }
        break;
    case VOLBEAT_EXIT:
        sprite->x -= 2;
        sprite->y2 = Sin((u8)sprite->sSinYIdx, 0x14);
        sprite->sSinYIdx += 4;
        if (sprite->x < -16)
            DestroySprite(sprite);
        break;
    case VOLBEAT_WAIT_STATE:
        // Wait for state progression, fly idly until then
        sprite->y2 = Cos((u8)sprite->sCosYIdx, 2);
        if (!--sprite->sStateDelay)
            sprite->sState = sprite->sNextState;
        break;
    }
}

#undef sStateDelay
#undef sNextState
#undef sCosYIdx
#undef sSinXIdx
#undef sSinYIdx
#undef sFig8Loops

#define sMoveTimer data[1]
#define sDelay     data[2]

static void SpriteCB_Torchic(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        if (gIntroFrameCounter == TIMER_TORCHIC_ENTER)
        {
            StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
            sprite->sState++;
        }
        break;
    case 1:
        if (gIntroFrameCounter == TIMER_PLAYER_HOLD_POSITION)
        {
            StartSpriteAnim(sprite, TORCHIC_ANIM_WALK);
            sprite->sState++;
        }
        else
        {
            sprite->sMoveTimer += 64;
            if (sprite->sMoveTimer & 0xFF00)
            {
                sprite->x--;
                sprite->sMoveTimer &= 0xFF;
            }
        }
        break;
    case 2:
        if (gIntroFrameCounter != TIMER_TORCHIC_SPEED_UP)
        {
            sprite->sMoveTimer += 32;
            if (sprite->sMoveTimer & 0xFF00)
            {
                sprite->x++;
                sprite->sMoveTimer &= 0xFF;
            }
        }
        else
        {
            StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
            sprite->sState++;
            sprite->sDelay = 80;
        }
        break;
    case 3:
        if (--sprite->sDelay)
        {
            sprite->sMoveTimer += 64;
            if (sprite->sMoveTimer & 0xFF00)
            {
                sprite->x--;
                sprite->sMoveTimer &= 0xFF;
            }
        }
        else
        {
            StartSpriteAnim(sprite, TORCHIC_ANIM_TRIP);
            sprite->sState++;
        }
        break;
    case 4:
        if (sprite->animEnded)
            sprite->x += 4;

        if (sprite->x > 336)
        {
            StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
            sprite->sState++;
        }
        break;
    case 5:
        if (gIntroFrameCounter >= TIMER_TORCHIC_EXIT)
            sprite->x -= 2;
        break;
    }
}

#undef sMoveTimer
#undef sDelay

#define sSinIdx data[1]
#define sCosIdx data[2]

static void SpriteCB_Manectric(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        if (gIntroFrameCounter == TIMER_MANECTRIC_ENTER)
            sprite->sState++;
        break;
    case 1:
        sprite->x -= 2;
        if (gIntroFrameCounter != TIMER_MANECTRIC_RUN_CIRCULAR)
            break;

        // Initialize circular pattern running
        sprite->y -= 12;
        sprite->sSinIdx = 0x80;
        sprite->sCosIdx = 0;
        sprite->sState++;
        // fallthrough
    case 2:
        if (sprite->x + sprite->x2 <= -32)
        {
            // Manectric is offscreen now, destroy it
            DestroySprite(sprite);
        }
        else
        {
            // Run in circular pattern
            if ((sprite->sSinIdx & 0xFF) < 64)
            {
                sprite->x2 = Sin((u8)sprite->sSinIdx, 16);
            }
            else
            {
                if ((sprite->sSinIdx & 0xFF) == 64)
                    sprite->x -= 48;
                sprite->x2 = Sin((u8)sprite->sSinIdx, 64);
            }
            sprite->sSinIdx++;
            sprite->y2 = Cos((u8)sprite->sCosIdx, 12);
            sprite->sCosIdx++;
        }
        break;
    }
}

#undef sSinIdx
#undef sCosIdx

#define tAlpha        data[0]
#define tZoomDiv      data[1]
#define tZoomDivSpeed data[2]

static void Task_Scene3_Load(u8 taskId)
{
    IntroResetGpuRegs();
    LZ77UnCompVram(sIntroPokeball_Gfx, (void *)VRAM);
    LZ77UnCompVram(sIntroPokeball_Tilemap, (void *)(BG_CHAR_ADDR(1)));
    LoadPalette(sIntroPokeball_Pal, BG_PLTT_ID(0), sizeof(sIntroPokeball_Pal));
    gTasks[taskId].tAlpha = 0;
    gTasks[taskId].tZoomDiv = 0;
    gTasks[taskId].tZoomDivSpeed = 0;
    gTasks[taskId].data[3] = 0;
    PanFadeAndZoomScreen(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 0, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(3) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(8) | BGCNT_256COLOR | BGCNT_AFF256x256);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG2_ON | DISPCNT_OBJ_ON);
    gTasks[taskId].func = Task_Scene3_SpinPokeball;
    gIntroFrameCounter = 0;
    m4aSongNumStart(MUS_INTRO_BATTLE);
}
static void Task_Scene3_SpinPokeball(u8 taskId)
{
    gTasks[taskId].tAlpha += 0x400;

    if (gTasks[taskId].tZoomDiv <= 0x6BF)
    {
        gTasks[taskId].tZoomDiv += gTasks[taskId].tZoomDivSpeed;
        gTasks[taskId].tZoomDivSpeed += 2;
    }
    else
    {
        gTasks[taskId].func = Task_Scene3_WaitGroudon;
    }

    PanFadeAndZoomScreen(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, SAFE_DIV(0x10000, gTasks[taskId].tZoomDiv), gTasks[taskId].tAlpha);

    if (gIntroFrameCounter == TIMER_POKEBALL_FADE)
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITEALPHA);
}

#undef tAlpha
#undef tZoomDiv
#undef tZoomDivSpeed

static void Task_Scene3_WaitGroudon(u8 taskId)
{
    if (gIntroFrameCounter > TIMER_START_LEGENDARIES)
        gTasks[taskId].func = Task_Scene3_LoadGroudon;
}

static void Task_Scene3_LoadGroudon(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        IntroResetGpuRegs();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 8;
        LZDecompressVram(gIntroGroudon_Gfx, (void *)VRAM);
        LZDecompressVram(gIntroGroudon_Tilemap, (void *)(BG_CHAR_ADDR(3)));
        LZDecompressVram(gIntroLegendBg_Gfx, (void *)(BG_CHAR_ADDR(1)));
        LZDecompressVram(gIntroGroudonBg_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
        LoadCompressedSpriteSheetUsingHeap(&gBattleAnimPicTable[GET_TRUE_SPRITE_INDEX(ANIM_TAG_ROCKS)]);
        LoadCompressedSpritePaletteUsingHeap(&gBattleAnimPaletteTable[GET_TRUE_SPRITE_INDEX(ANIM_TAG_ROCKS)]);
        CpuCopy16(gIntro3Bg_Pal, gPlttBufferUnfaded, sizeof(gIntro3Bg_Pal));
        gTasks[taskId].func = Task_Scene3_InitGroudonBg;
    }
}

#define tWinPos  data[0]
#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom    data[3]

static void Task_Scene3_InitGroudonBg(u8 taskId)
{
    SetGpuReg(REG_OFFSET_WIN0H, DISPLAY_WIDTH);
    SetGpuReg(REG_OFFSET_WIN0V, DISPLAY_HEIGHT);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(0)
                               | BGCNT_CHARBASE(0)
                               | BGCNT_SCREENBASE(24)
                               | BGCNT_256COLOR
                               | BGCNT_WRAP
                               | BGCNT_AFF512x512);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1)
                               | BGCNT_CHARBASE(1)
                               | BGCNT_SCREENBASE(28)
                               | BGCNT_16COLOR
                               | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1
                                | DISPCNT_OBJ_1D_MAP
                                | DISPCNT_BG1_ON
                                | DISPCNT_BG2_ON
                                | DISPCNT_OBJ_ON
                                | DISPCNT_WIN0_ON);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
    gTasks[taskId].tWinPos = 0;
    gTasks[taskId].tScreenX = 0xFFA0;
    gTasks[taskId].tScreenY = 0xFF51;
    gTasks[taskId].tZoom = 0x100;
    PanFadeAndZoomScreen(gTasks[taskId].tScreenX, gTasks[taskId].tScreenY, gTasks[taskId].tZoom, 0);
    gTasks[taskId].func = Task_Scene3_NarrowWindow;
}

// Before the Groudon scene starts, the black top/bottom edges of the screen
// come inward for a more 'cinematic' look
#define NARROW_HEIGHT 32
static void Task_Scene3_NarrowWindow(u8 taskId)
{
    if (gTasks[taskId].tWinPos != NARROW_HEIGHT)
    {
        gTasks[taskId].tWinPos += 4;
        SetGpuReg(REG_OFFSET_WIN0V, (gTasks[taskId].tWinPos * 256) - (gTasks[taskId].tWinPos - DISPLAY_HEIGHT));
    }
    else
    {
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(NARROW_HEIGHT, DISPLAY_HEIGHT - NARROW_HEIGHT));
        gTasks[taskId].func = Task_Scene3_EndNarrowWindow;
    }
}
#undef tWinPos
#undef tScreenX
#undef tScreenY
#undef tZoom

static void Task_Scene3_EndNarrowWindow(u8 taskId)
{
    gTasks[taskId].func = Task_Scene3_StartGroudon;
}

static void Task_Scene3_StartGroudon(u8 taskId)
{
    gTasks[taskId].tState = 0;
    gTasks[taskId].func = Task_Scene3_Groudon;
    ScanlineEffect_InitWave(0, DISPLAY_HEIGHT, 4, 4, 1, SCANLINE_EFFECT_REG_BG1HOFS, FALSE);
}

#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom    data[3]
#define tYShake  data[4]
#define tTimer   data[5]
#define tDelay   data[6]
#define tTrigIdx data[6] // Re-used
#define tPalIdx  data[7]

// Treats gIntro3Bg_Pal as u8* and iterates by 1
#define INTRO3_RAW_PTR(palId)(((void *) &gIntro3Bg_Pal) + palId)

static void Task_Scene3_Groudon(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tTimer++;
    if ((tState >= 1 && tState <= 7) && tTimer % 2 == 0)
        tYShake ^= 3;
    PanFadeAndZoomScreen(tScreenX, tScreenY + tYShake, tZoom, 0);
    switch (tState)
    {
    case 0:
        tScreenX += 16;
        if (tScreenX == 160)
        {
            tState++;
            tDelay = 2;
            tPalIdx = 0x1E2;
            CreateGroudonRockSprites(taskId);
        }
        break;
    case 1:
        if (--tDelay == 0)
        {
            tDelay = 2;
            CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
            tPalIdx += 2;
            if (tPalIdx == 0x1EC)
                tState++;
        }
        break;
    case 2:
        if (--tDelay == 0)
        {
            tDelay = 2;
            tState++;
        }
        break;
    case 3:
        if (--tDelay == 0)
        {
            tDelay = 2;
            CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
            tPalIdx -= 2;
            if (tPalIdx == 0x1E0)
            {
                tDelay = 8;
                tState++;
            }
        }
        break;
    case 4:
        if (--tDelay == 0)
        {
            tScreenX = -96;
            tScreenY = 169;
            tDelay = 3;
            tState++;
        }
        break;
    case 5:
        if (--tDelay == 0)
        {
            tScreenX = 80;
            tScreenY = 41;
            tDelay = 16;
            PlayCryInternal(SPECIES_GROUDON, 0, 100, CRY_PRIORITY_NORMAL, CRY_MODE_NORMAL);
            tState++;
        }
        break;
    case 6:
        if (--tDelay == 0)
        {
            tScreenX = 80;
            tScreenY = 40;
            tState++;
        }
        break;
    case 7:
        tScreenX += 4;
        tScreenY += 4;
        tTrigIdx += 0x666;
        tZoom = Sin((tTrigIdx & 0xFF00) >> 8, 64) + 256;
        if (tScreenX == 120)
        {
            BeginNormalPaletteFade(PALETTES_ALL & ~1, 3, 0, 16, RGB_WHITE);
            tZoom = 256;
            tYShake = 0;
            tState++;
        }
        break;
    case 8:
        if (tZoom)
            tZoom -= 8;
        else
            tState++;
        break;
    case 9:
        if (!gPaletteFade.active)
        {
            gTasks[taskId].func = Task_Scene3_LoadKyogre;
            gScanlineEffect.state = 3;
        }
        break;
    }
}

#undef tScreenX
#undef tScreenY
#undef tZoom
#undef tYShake
#undef tTimer
#undef tDelay
#undef tTrigIdx
#undef tPalIdx

#define sRockId data[1]
#define sSpeed  data[2]
#define sTimer  data[3]
#define sTaskId data[4]

static void CreateGroudonRockSprites(u8 taskId)
{
    int i;
    u8 spriteId;

    for (i = 0; i < (int)ARRAY_COUNT(sGroudonRockData); i++)
    {
        spriteId = CreateSprite(&gAncientPowerRockSpriteTemplate, sGroudonRockData[i][0], DISPLAY_HEIGHT, i);
        gSprites[spriteId].callback = SpriteCB_GroudonRocks;
        gSprites[spriteId].oam.priority = 0;
        gSprites[spriteId].sRockId = i;
        gSprites[spriteId].sTaskId = taskId;
        StartSpriteAnim(&gSprites[spriteId], sGroudonRockData[i][1]);
    }
}

static void SpriteCB_GroudonRocks(struct Sprite *sprite)
{
    // Introduce some wobble to the floating
    sprite->sTimer++;
    if (sprite->sTimer % 2 == 0)
        sprite->y2 ^= 3;

    switch(sprite->sState)
    {
    case 0:
        // Rock floats up
        sprite->sSpeed += sGroudonRockData[sprite->sRockId][2];
        sprite->y -= (sprite->sSpeed & 0xFF00) >> 8;
        sprite->sSpeed &= 0xFF;

        // Check if Groudon scene is ending
        if (gTasks[sprite->sTaskId].tState > 7)
            sprite->sState++;
        break;
    case 1:
        // Scene zooms in, move rock offscreen
        if (sprite->x < DISPLAY_WIDTH / 2)
            sprite->x -= 2;
        else
            sprite->x += 2;

        if (sprite->y < DISPLAY_HEIGHT / 2)
            sprite->y -= 2;
        else
            sprite->y += 2;
        break;
    }
}

#undef sRockId
#undef sSpeed
#undef sTimer
#undef sTaskId

#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom    data[3]
#define tDelay   data[6]
#define tTrigIdx data[6] // Re-used
#define tPalIdx  data[7]

static void Task_Scene3_LoadKyogre(u8 taskId)
{
    ResetSpriteData();
    LZDecompressVram(gIntroKyogre_Gfx, (void *)VRAM);
    LZDecompressVram(gIntroKyogre_Tilemap, (void *)(BG_CHAR_ADDR(3)));
    LZDecompressVram(gIntroKyogreBg_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
    LoadCompressedSpriteSheet(sSpriteSheet_Bubbles);
    LoadSpritePalette(sSpritePalette_Bubbles);
    BeginNormalPaletteFade(PALETTES_ALL & ~1, 0, 16, 0, RGB_WHITEALPHA);
    gTasks[taskId].func = Task_Scene3_Kyogre;
    gTasks[taskId].tState = 0;
    gTasks[taskId].tScreenX = 336;
    gTasks[taskId].tScreenY = 80;
    gTasks[taskId].tDelay = 16;
    gTasks[taskId].tZoom = 256;
    PanFadeAndZoomScreen(gTasks[taskId].tScreenX, gTasks[taskId].tScreenY, gTasks[taskId].tZoom, 0);
    ScanlineEffect_InitWave(0, DISPLAY_HEIGHT, 4, 4, 1, SCANLINE_EFFECT_REG_BG1VOFS, FALSE);
}

static void Task_Scene3_Kyogre(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    PanFadeAndZoomScreen(tScreenX, tScreenY, tZoom, 0);

    switch (tState)
    {
    case 0:
        if (--tDelay != 0)
            break;
        tState++;
    case 1:
        tTrigIdx += 4;
        gTasks[taskId].tScreenX = 344 - Sin(tTrigIdx, 0x100);
        gTasks[taskId].tScreenY = 84 - Cos(tTrigIdx, 0x40);
        if (tTrigIdx == 64)
        {
            tDelay = 0x19;
            tPalIdx = 1;
            tState++;
            CreateKyogreBubbleSprites_Body(0);
        }
        break;
    case 2:
        if (--tDelay == 0)
        {
            gTasks[taskId].tScreenX += 256;
            gTasks[taskId].tScreenY -= 258;
            tDelay = 8;
            tState++;
            CreateKyogreBubbleSprites_Body(0);
            CreateKyogreBubbleSprites_Fins();
        }
        break;
    case 3:
        if (--tDelay == 0)
        {
            gTasks[taskId].tScreenX -= 256;
            gTasks[taskId].tScreenY += 258;
            tDelay = 8;
            tState++;
        }
        break;
    case 4:
        if (--tDelay == 0)
        {
            gTasks[taskId].tScreenY -= 252;
            tDelay = 8;
            tState++;
        }
        break;
    case 5:
        if (--tDelay == 0)
        {
            gTasks[taskId].tScreenY += 252;
            if (tPalIdx != 0)
            {
                tDelay = 12;
                tPalIdx--;
                tState = 2;
            }
            else
            {
                tDelay = 1;
                tState++;
                PlayCryInternal(SPECIES_KYOGRE, 0, 120, CRY_PRIORITY_NORMAL, CRY_MODE_NORMAL);
            }
        }
        break;
    case 6:
        if (--tDelay == 0)
        {
            tDelay = 4;
            tPalIdx = 0x1EA;
            tState++;
        }
        break;
    case 7:
        if (--tDelay == 0)
        {
            tDelay = 4;
            CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(2) + 15], PLTT_SIZEOF(1));
            tPalIdx -= 2;
            if (tPalIdx == 0x1E0)
                tState++;
        }
        break;
    case 8:
        if (--tDelay == 0)
        {
            tDelay = 4;
            tPalIdx = 0x1E2;
            tState++;
        }
        break;
    case 9:
        if (--tDelay == 0)
        {
            tDelay = 4;
            CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(2) + 15], PLTT_SIZEOF(1));
            tPalIdx += 2;
            if (tPalIdx == 0x1EE)
            {
                tDelay = 16;
                tState++;
            }
        }
        break;
    case 10:
        if (--tDelay == 0)
        {
            tTrigIdx = 0;
            tState++;
            CreateKyogreBubbleSprites_Body(taskId);
        }
        break;
    case 11:
        tTrigIdx += 4;
        tZoom -= 8;
        gTasks[taskId].tScreenX = Sin(tTrigIdx, 0x3C) + 88;
        if (tTrigIdx == 64)
        {
            BeginNormalPaletteFade(PALETTES_ALL & ~1, 3, 0, 16, RGB_WHITE);
            tState++;
        }
        break;
    case 12:
        tTrigIdx += 4;
        tZoom -= 8;
        gTasks[taskId].tScreenX = Sin(tTrigIdx, 0x14) + 128;
        if (tTrigIdx == 128)
            tState++;
        break;
    case 13:
        if (!gPaletteFade.active)
        {
            gTasks[taskId].func = Task_Scene3_LoadClouds1;
            gScanlineEffect.state = 3;
        }
        break;
    }
}

#undef tScreenX
#undef tScreenY
#undef tZoom
#undef tDelay
#undef tTrigIdx
#undef tPalIdx

#define sSinIdx data[1]
#define sBaseY  data[2]
#define sTaskId data[5]
#define sDelay  data[6]
#define sUnk    data[7] // Never read

// taskId is used inconsistently for these two functions.
// The sprite callback for the bubbles will always read it, unless delay is 0 to
// start (it never is), but the first function is often passed 0 instead of a
// taskId, and the second function doesn't take/assign a taskId at all.
// The only time an actual taskId is given is when it actually needs the
// result of reading it, to zoom in at the end of the scene.

// Creates bubbles at positions spread across Kyogre's body
static void CreateKyogreBubbleSprites_Body(u8 taskId)
{
    int i;
    u8 spriteId;

    for (i = 0; i < NUM_BUBBLES_IN_SET; i++)
    {
        spriteId = CreateSprite(&sSpriteTemplate_Bubbles,
                                sKyogreBubbleData[i][0],
                                sKyogreBubbleData[i][1],
                                i);
        gSprites[spriteId].invisible = TRUE;
        gSprites[spriteId].sTaskId = taskId;
        gSprites[spriteId].sDelay = sKyogreBubbleData[i][2];
        gSprites[spriteId].sUnk = 64;
    }
}

// Creates bubbles at positions around Kyogre's fins, for when it's moving them
static void CreateKyogreBubbleSprites_Fins(void)
{
    int i;
    u8 spriteId;

    for (i = 0; i < NUM_BUBBLES_IN_SET; i++)
    {
        spriteId = CreateSprite(&sSpriteTemplate_Bubbles,
                                sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][0],
                                sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][1],
                                i);
        gSprites[spriteId].invisible = TRUE;
#ifdef BUGFIX
        gSprites[spriteId].sDelay = sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][2];
#else
        gSprites[spriteId].sDelay = sKyogreBubbleData[i][2]; // Using the wrong set of delays here
#endif
        gSprites[spriteId].sUnk = 64;
    }
}

static void SpriteCB_KyogreBubbles(struct Sprite *sprite)
{
    switch(sprite->sState)
    {
    case 0:
        if (sprite->sDelay == 0)
        {
            // Animation has started, float bubbles up
            sprite->sSinIdx = (sprite->sSinIdx + 11) & 0xFF;
            sprite->x2 = Sin(sprite->sSinIdx, 4);
            sprite->sBaseY += 48;
            sprite->y2 = -(sprite->sBaseY >> 8);
            if (sprite->animEnded)
                DestroySprite(sprite);
        }
        else if (--sprite->sDelay == 0)
        {
            // Start bubble animation after delay has finished
            StartSpriteAnim(sprite, 0);
            sprite->invisible = FALSE;
        }

        // Check if Kyogre scene is ending
        // For all but the last bubbles, sTaskId isn't actually set
        if (gTasks[sprite->sTaskId].tState > 11)
            sprite->sState++;
        break;
    case 1:
        // Scene zooms in, move bubbles offscreen
        if (sprite->x < DISPLAY_WIDTH / 2)
            sprite->x -= 3;
        else
            sprite->x += 3;

        if (sprite->y < DISPLAY_HEIGHT / 2)
            sprite->y -= 3;
        else
            sprite->y += 3;

        if ((u16)(sprite->y - 20) > DISPLAY_HEIGHT - 20)
            DestroySprite(sprite);
        break;
    }
}

#undef sSinIdx
#undef sBaseY
#undef sTaskId
#undef sDelay
#undef sUnk

static void Task_Scene3_LoadClouds1(u8 taskId)
{
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0
                               | BLDCNT_TGT1_BG1
                               | BLDCNT_TGT1_BG2
                               | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(31, 31));
    SetGpuReg(REG_OFFSET_BLDY, 31);
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0)
                               | BGCNT_CHARBASE(0)
                               | BGCNT_SCREENBASE(24)
                               | BGCNT_16COLOR
                               | BGCNT_TXT512x256);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(0)
                               | BGCNT_CHARBASE(1)
                               | BGCNT_SCREENBASE(26)
                               | BGCNT_16COLOR
                               | BGCNT_TXT512x256);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2)
                               | BGCNT_CHARBASE(1)
                               | BGCNT_SCREENBASE(28)
                               | BGCNT_16COLOR
                               | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
                                | DISPCNT_OBJ_1D_MAP
                                | DISPCNT_BG0_ON
                                | DISPCNT_BG1_ON
                                | DISPCNT_BG2_ON
                                | DISPCNT_OBJ_ON
                                | DISPCNT_WIN0_ON);
    SetGpuReg(REG_OFFSET_BG0HOFS, 80);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, -80);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    LZDecompressVram(gIntroClouds_Gfx, (void *)VRAM);
    LZDecompressVram(gIntroClouds_Gfx, (void *)(BG_CHAR_ADDR(1)));
    LZDecompressVram(gIntroCloudsSun_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
    gTasks[taskId].func = Task_Scene3_LoadClouds2;
}

static void Task_Scene3_LoadClouds2(u8 taskId)
{
    LZDecompressVram(gIntroCloudsLeft_Tilemap, (void *)(BG_CHAR_ADDR(3)));
    LZDecompressVram(gIntroCloudsRight_Tilemap, (void *)(BG_SCREEN_ADDR(26)));
    gTasks[taskId].func = Task_Scene3_InitClouds;
}

#define tCloudPos data[6]

static void Task_Scene3_InitClouds(u8 taskId)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gTasks[taskId].func = Task_Scene3_Clouds;
    gTasks[taskId].tState = 0;
    gTasks[taskId].tCloudPos = 16;
}

// Clouds coming in from the sides before Rayquaza appears
static void Task_Scene3_Clouds(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    // Left clouds are on BG0, right clouds are on BG1
    SetGpuReg(REG_OFFSET_BG0HOFS, (tCloudPos >> 8));
    SetGpuReg(REG_OFFSET_BG1HOFS, -(tCloudPos >> 8));

    switch (tState)
    {
    case 0:
        // Cloud position is used briefly as a delay, before
        // the scene has faded in from white
        if (--tCloudPos == 0)
        {
            // Start fade in from white, set cloud starting positions
            BeginNormalPaletteFade(PALETTES_ALL & ~1, 0, 16, 0, RGB_WHITEALPHA);
            tCloudPos = 80 << 8;
            tState++;
        }
        break;
    case 1:
        // Start fading out
        if (tCloudPos == 40 << 8)
            BeginNormalPaletteFade(PALETTES_BG & ~1, 3, 0, 16, RGB(9, 10, 10));

        // Move clouds inward toward each other
        if (tCloudPos != 0)
            tCloudPos -= 128;
        else if (!gPaletteFade.active)
            gTasks[taskId].func = Task_Scene3_LoadLightning;
        break;
    }
}
#undef tCloudPos

#define tDelay data[6]
#define tTimer data[7]

static void Task_Scene3_LoadLightning(u8 taskId)
{
    LZDecompressVram(gIntroRayquaza_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
    LZDecompressVram(gIntroRayquazaClouds_Tilemap, (void *)(BG_CHAR_ADDR(3)));
    LZDecompressVram(gIntroRayquaza_Gfx, (void *)(BG_CHAR_ADDR(1)));
    LZDecompressVram(gIntroRayquazaClouds_Gfx, (void *)VRAM);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
                                | DISPCNT_OBJ_1D_MAP
                                | DISPCNT_BG0_ON
                                | DISPCNT_BG2_ON
                                | DISPCNT_OBJ_ON
                                | DISPCNT_WIN0_ON);
    gTasks[taskId].func = Task_Scene3_Lightning;
    gTasks[taskId].tState = 0;
    gTasks[taskId].tDelay = 1;
    gTasks[taskId].tTimer = 0;
    LoadCompressedSpriteSheetUsingHeap(sSpriteSheet_Lightning);
    LoadSpritePalettes(sSpritePalette_Lightning);
}

static void Task_Scene3_Lightning(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 spriteId;

    switch (tState)
    {
    case 0:
        if (--tDelay == 0)
        {
            // Do first lightning bolt
            CreateSprite(&sSpriteTemplate_Lightning, 200, 48, 0);
            spriteId = CreateSprite(&sSpriteTemplate_Lightning, 200, 80, 1);
            StartSpriteAnim(&gSprites[spriteId], 1);
            spriteId = CreateSprite(&sSpriteTemplate_Lightning, 200, 112, 2);
            StartSpriteAnim(&gSprites[spriteId], 2);
            tState++;
            tDelay = 72;
        }
        break;
    case 1:
        if (--tDelay == 0)
        {
            // Do second lightning bolt
            CreateSprite(&sSpriteTemplate_Lightning, 40, 48, 0);
            spriteId = CreateSprite(&sSpriteTemplate_Lightning, 40, 80, 1);
            StartSpriteAnim(&gSprites[spriteId], 1);
            spriteId = CreateSprite(&sSpriteTemplate_Lightning, 40, 112, 2);
            StartSpriteAnim(&gSprites[spriteId], 2);
            tState++;
            tDelay = 48;
        }
        break;
    case 2:
        if (--tDelay == 0)
            gTasks[taskId].func = Task_Scene3_LoadRayquazaAttack;
        break;
    }
}

#define sPalIdx data[1]
#define sDelay data[2]

static void SpriteCB_Lightning(struct Sprite *sprite)
{
    if (sprite->animEnded)
        sprite->invisible = TRUE;

    switch(sprite->sState)
    {
    case 0:
        sprite->sPalIdx = 0x1C2;
        sprite->sState++;
    case 1:
        CpuCopy16(INTRO3_RAW_PTR(sprite->sPalIdx), &gPlttBufferFaded[BG_PLTT_ID(5) + 13], PLTT_SIZEOF(1));
        sprite->sPalIdx += 2;
        if (sprite->sPalIdx != 0x1CE)
            break;
        sprite->sPalIdx = 0x1CC;
        sprite->sDelay = 4;
        sprite->sState++;
    case 2:
        if (--sprite->sDelay == 0)
        {
            sprite->sDelay = 4;
            CpuCopy16(INTRO3_RAW_PTR(sprite->sPalIdx), &gPlttBufferFaded[BG_PLTT_ID(5) + 13], PLTT_SIZEOF(1));
            sprite->sPalIdx -= 2;
            if (sprite->sPalIdx == 0x1C0)
                DestroySprite(sprite);
        }
        break;
    }
}

#undef sPalIdx
#undef sDelay

#define tRayquazaTaskId data[4]

static void Task_Scene3_LoadRayquazaAttack(u8 taskId)
{
    u8 attackTaskId;

    LoadCompressedSpriteSheet(sSpriteSheet_RayquazaOrb);
    LoadSpritePalettes(sSpritePalette_RayquazaOrb);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
                                | DISPCNT_OBJ_1D_MAP
                                | DISPCNT_BG0_ON
                                | DISPCNT_BG2_ON
                                | DISPCNT_OBJ_ON
                                | DISPCNT_WIN0_ON);
    gTasks[taskId].func = Task_Scene3_Rayquaza;
    BeginNormalPaletteFade(PALETTES_BG & ~(0x21), 0, 16, 0, RGB(9, 10, 10));
    gTasks[taskId].tState = 0;
    gTasks[taskId].data[1] = 0xA8;
    gTasks[taskId].data[2] = -0x10;
    gTasks[taskId].data[3] = -0x88;
    gTasks[taskId].data[4] = -0x10;
    attackTaskId = CreateTask(Task_RayquazaAttack, 0);
    gTasks[attackTaskId].tRayquazaTaskId = taskId;
}

static void Task_Scene3_Rayquaza(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tTimer % 2 == 0)
        data[6] ^= 2;

    tTimer++;

    switch(tState)
    {
    case 0:
        if ((tTimer & 1) != 0)
        {
            data[1] -= 2;
            data[2]++;
            data[3] += 2;
            data[4]++;
        }
        if (data[1] == 0x68)
        {
            tState++;
            data[5] = 1;
        }
        break;
    case 1:
        tState++;
        data[5] = 4;
        break;
    case 2:
        data[1] += 4;
        data[2] -= 2;
        data[3] -= 4;
        data[4] -= 2;
        if (!gPaletteFade.active)
        {
            data[5] = 0x8C;
            tState++;
        }
        break;
    case 3:
        if (--data[5] == 0)
            gTasks[taskId].func = Task_EndIntroMovie;
        break;
    }
}
#undef tDelay
#undef tTimer

static void Task_EndIntroMovie(u8 taskId)
{
    DestroyTask(taskId);
    SetMainCallback2(MainCB2_EndIntro);
}

static void Task_RayquazaAttack(u8 taskId)
{
    u8 spriteId;
    s16 *data = gTasks[taskId].data;
    data[2]++;

    switch(tState)
    {
    case 0:
        if ((data[2] & 1) != 0)
        {
            CpuCopy16(INTRO3_RAW_PTR(0x1A2 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 14], PLTT_SIZEOF(1));
            data[1]++;
        }
        if (data[1] == 6)
        {
            tState++;
            data[1] = 0;
            data[3] = 10;
        }
        break;
    case 1:
        if (data[3] == 0)
        {
            if ((data[2] & 1) != 0)
            {
                CpuCopy16(INTRO3_RAW_PTR(0x1A2 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 8], PLTT_SIZEOF(1));
                data[1]++;
            }
            if (data[1] == 6)
            {
                tState++;
                data[3] = 10;
            }
        }
        else
        {
            data[3]--;
        }
        break;
    case 2:
        if (data[3] == 0)
        {
            if ((data[2] & 1) != 0)
            {
                CpuCopy16(INTRO3_RAW_PTR(0x182 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 12], PLTT_SIZEOF(1));
                data[1]++;
            }
            if (data[1] == 6)
            {
                spriteId = CreateSprite(&sSpriteTemplate_RayquazaOrb, 120, 88, 15);
                PlaySE(SE_INTRO_BLAST);
                gSprites[spriteId].invisible = TRUE;
                gSprites[spriteId].data[3] = tRayquazaTaskId;
                tState++;
                data[3] = 16;
            }
        }
        else
        {
            data[3]--;
        }
        break;
    case 3:
        if ((data[2] & 1) != 0)
        {
            if (--data[3] != 0)
            {
                BlendPalette(BG_PLTT_ID(5), 16, data[3], RGB(9, 10, 10));
                CpuCopy16(INTRO3_RAW_PTR(428), &gPlttBufferFaded[BG_PLTT_ID(5) + 14], PLTT_SIZEOF(1));
                CpuCopy16(INTRO3_RAW_PTR(428), &gPlttBufferFaded[BG_PLTT_ID(5) + 8], PLTT_SIZEOF(1));
                CpuCopy16(INTRO3_RAW_PTR(396), &gPlttBufferFaded[BG_PLTT_ID(5) + 12], PLTT_SIZEOF(1));
            }
            else
            {
                tState++;
                data[3] = 53;
            }
        }
        break;
    case 4:
        if (--data[3] == 0)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITE);
            tState++;
        }
        break;
    case 5:
        if (!gPaletteFade.active)
            DestroyTask(taskId);
        break;
    }
}

static void IntroResetGpuRegs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void Task_BlendLogoIn(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
    default:
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND
                                   | BLDCNT_TGT2_BG0
                                   | BLDCNT_TGT2_BG1
                                   | BLDCNT_TGT2_BG2
                                   | BLDCNT_TGT2_BG3
                                   | BLDCNT_TGT2_OBJ
                                   | BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[31]);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend);
        gTasks[taskId].tState++;
        break;
    case 1:
        if (gTasks[taskId].data[1] != 0)
        {
            u8 tmp;

            gTasks[taskId].data[1]--;
            tmp = gTasks[taskId].data[1] / 2;
            SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[tmp]);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[0]);
            gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend) / 4;
            gTasks[taskId].tState++;
        }
        break;
    case 2:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        DestroyTask(taskId);
        break;
    }
}

static void Task_BlendLogoOut(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
    default:
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND
                                   | BLDCNT_TGT2_BG0
                                   | BLDCNT_TGT2_BG1
                                   | BLDCNT_TGT2_BG2
                                   | BLDCNT_TGT2_BG3
                                   | BLDCNT_TGT2_OBJ
                                   | BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[0]);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].tState++;
        break;
    case 1:
        if (gTasks[taskId].data[1] < (int)ARRAY_COUNT(gTitleScreenAlphaBlend) - 2)
        {
            u8 tmp;

            gTasks[taskId].data[1]++;
            tmp = gTasks[taskId].data[1] / 2;
            SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[tmp]);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[31]);
            gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend) / 4;
            gTasks[taskId].tState++;
        }
        break;
    case 2:
        if (gTasks[taskId].data[1] != 0)
        {
            gTasks[taskId].data[1]--;
        }
        else
        {
            SetGpuReg(REG_OFFSET_BLDCNT, 0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
            SetGpuReg(REG_OFFSET_BLDY, 0);
            DestroyTask(taskId);
        }
        break;
    }
}

void PanFadeAndZoomScreen(u16 screenX, u16 screenY, u16 zoom, u16 alpha)
{
    struct BgAffineSrcData src;
    struct BgAffineDstData dest;

    src.texX = 0x8000;
    src.texY = 0x8000;
    src.scrX = screenX;
    src.scrY = screenY;
    src.sx = zoom;
    src.sy = zoom;
    src.alpha = alpha;
    BgAffineSet(&src, &dest, 1);
    SetGpuReg(REG_OFFSET_BG2PA, dest.pa);
    SetGpuReg(REG_OFFSET_BG2PB, dest.pb);
    SetGpuReg(REG_OFFSET_BG2PC, dest.pc);
    SetGpuReg(REG_OFFSET_BG2PD, dest.pd);
    SetGpuReg(REG_OFFSET_BG2X_L, dest.dx);
    SetGpuReg(REG_OFFSET_BG2X_H, dest.dx >> 16);
    SetGpuReg(REG_OFFSET_BG2Y_L, dest.dy);
    SetGpuReg(REG_OFFSET_BG2Y_H, dest.dy >> 16);
}

static void SpriteCB_WaterDrop_Ripple(struct Sprite *sprite)
{
    u8 palNum;

    if (sprite->data[2] >= 192)
    {
        if (sprite->data[3] != 0)
        {
            sprite->data[3]--;
        }
        else
        {
            sprite->invisible = FALSE;
            SetOamMatrix(sprite->data[1], sprite->data[2], 0, 0, sprite->data[2]);
            sprite->data[2] = (sprite->data[2] * 95) / 100;
            palNum = (sprite->data[2] - 192) / 128 + 9;
            if (palNum > 15)
                palNum = 15;
            sprite->oam.paletteNum = palNum;
        }
    }
    else
    {
        DestroySprite(sprite);
    }
}

static void SpriteCB_WaterDropHalf(struct Sprite *sprite)
{
    if (gSprites[sprite->data[7]].data[7] != 0)
    {
        sprite->invisible = TRUE;
        sprite->x += sprite->x2;
        sprite->y += sprite->y2;
        StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
        sprite->data[2] = 1024;
        sprite->data[3] = 8 * (sprite->data[1] & 3);
        sprite->callback = SpriteCB_WaterDrop_Ripple;
        sprite->oam.shape = SPRITE_SHAPE(64x32);
        sprite->oam.size = SPRITE_SIZE(64x32);
        CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
    }
    else
    {
        sprite->x2 = gSprites[sprite->data[7]].x2;
        sprite->y2 = gSprites[sprite->data[7]].y2;
        sprite->x = gSprites[sprite->data[7]].x;
        sprite->y = gSprites[sprite->data[7]].y;
    }
}

static void SpriteCB_WaterDrop(struct Sprite *sprite)
{
    // Wait for sState to be modified by Task_Scene1_WaterDrops
    if (sprite->sState != 0)
        sprite->callback = SpriteCB_WaterDrop_Slide;
}

static void SpriteCB_WaterDrop_Slide(struct Sprite *sprite)
{
    if (sprite->x <= 116)
    {
        sprite->y += sprite->y2;
        sprite->y2 = 0;
        sprite->x += 4;
        sprite->x2 = -4;
        sprite->data[4] = 128;
        sprite->callback = SpriteCB_WaterDrop_ReachLeafEnd;
    }
    else
    {
        u16 data2;
        u16 data3;
        u16 data4;
        s16 sin1;
        s16 sin2;
        s16 sin3;
        s16 sin4;
        s16 var1;
        s16 var2;
        s16 var3;
        s16 var4;
        s16 temp;

        data4 = sprite->data[4];
        sin1 = gSineTable[(u8)data4];
        sin2 = gSineTable[(u8)(data4 + 64)];
        sprite->data[4] += 2;
        sprite->y2 = sin1 / 32;
        sprite->x--;
        if (sprite->x & 1)
            sprite->y++;
        temp = -sin2 / 16;
        data2 = sprite->data[2];
        data3 = sprite->data[3];
        sin3 = gSineTable[(u8)(temp - 16)];
        sin4 = gSineTable[(u8)(temp + 48)];
        var1 = sin4 * data2 / 256;
        var2 = -sin3 * data3 / 256;
        var3 = sin3 * data2 / 256;
        var4 = sin4 * data3 / 256;
        SetOamMatrix(sprite->data[1], data2, 0, 0, data3);
        SetOamMatrix(sprite->data[1] + 1, var1, var3, var2, var4);
        SetOamMatrix(sprite->data[1] + 2, var1, var3, var2 * 2, var4 * 2);
    }
}

static void SpriteCB_WaterDrop_ReachLeafEnd(struct Sprite *sprite)
{
    SetOamMatrix(sprite->data[1], sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
    SetOamMatrix(sprite->data[1] + 1, sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
    SetOamMatrix(sprite->data[1] + 2, sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
    if (sprite->data[4] != MAX_SPRITES)
    {
        u16 sinIdx;
        sprite->data[4] -= 8;
        sinIdx = sprite->data[4];
        sprite->x2 = gSineTable[(u8)(sinIdx + 64)] / 64;
        sprite->y2 = gSineTable[(u8)sinIdx] / 64;
    }
    else
    {
        sprite->data[4] = 0;
        sprite->callback = SpriteCB_WaterDrop_DangleFromLeaf;
    }
}

static void SpriteCB_WaterDrop_DangleFromLeaf(struct Sprite *sprite)
{
    if (sprite->data[0] != 2)
    {
        s16 r2;

        sprite->data[4] += 8;
        r2 = gSineTable[(u8)sprite->data[4]] / 16 + 64;
        sprite->x2 = gSineTable[(u8)(r2 + 64)] / 64;
        sprite->y2 = gSineTable[(u8)r2] / 64;
    }
    else
    {
        sprite->callback = SpriteCB_WaterDrop_Fall;
    }
}

static void SpriteCB_WaterDrop_Fall(struct Sprite *sprite)
{
    if (sprite->y < sprite->data[5])
    {
        sprite->y += 4;
    }
    else
    {
        sprite->data[7] = 1;
        sprite->invisible = TRUE;
        sprite->x += sprite->x2;
        sprite->y += sprite->y2;
        StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
        sprite->data[2] = 1024;
        sprite->data[3] = 8 * (sprite->data[1] & 3);
        sprite->callback = SpriteCB_WaterDrop_Ripple;
        sprite->oam.shape = SPRITE_SHAPE(64x32);
        sprite->oam.size = SPRITE_SIZE(64x32);
        CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
    }
}

// Identical to SpriteCB_WaterDrop_Fall
// Used by the 2nd and 3rd water drops to skip the leaf slide
static void SpriteCB_WaterDropShort(struct Sprite *sprite)
{
    if (sprite->y < sprite->data[5])
    {
        sprite->y += 4;
    }
    else
    {
        sprite->data[7] = 1;
        sprite->invisible = TRUE;
        sprite->x += sprite->x2;
        sprite->y += sprite->y2;
        StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
        sprite->data[2] = 1024;
        sprite->data[3] = 8 * (sprite->data[1] & 3);
        sprite->callback = SpriteCB_WaterDrop_Ripple;
        sprite->oam.shape = SPRITE_SHAPE(64x32);
        sprite->oam.size = SPRITE_SIZE(64x32);
        CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
    }
}

static u8 CreateWaterDrop(s16 x, s16 y, u16 c, u16 d, u16 e, u8 fallImmediately)
{
    u8 spriteId;
    u8 oldSpriteId;

    // Create water drop reflection
    spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
    gSprites[spriteId].data[0] = 0;
    gSprites[spriteId].data[7] = 0;
    gSprites[spriteId].data[1] = d;
    gSprites[spriteId].data[2] = c;
    gSprites[spriteId].data[3] = c;
    gSprites[spriteId].data[5] = e;
    gSprites[spriteId].data[6] = c;
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    gSprites[spriteId].oam.matrixNum = d;
    CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
    StartSpriteAnim(&gSprites[spriteId], DROP_ANIM_REFLECTION);
    if (!fallImmediately)
        gSprites[spriteId].callback = SpriteCB_WaterDrop; // Do full anim, for 1st drop that slides along the leaf
    else
        gSprites[spriteId].callback = SpriteCB_WaterDropShort; // Skip to drop falling into the water, for 2nd and 3rd drops
    oldSpriteId = spriteId;

    // Create water drop upper half
    // Implicitly anim number 0, DROP_ANIM_UPPER_HALF
    spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
    gSprites[spriteId].data[7] = oldSpriteId;
    gSprites[spriteId].data[1] = d + 1;
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    gSprites[spriteId].oam.matrixNum = d + 1;
    CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
    gSprites[spriteId].callback = SpriteCB_WaterDropHalf;

    // Create water drop lower half
    spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
    gSprites[spriteId].data[7] = oldSpriteId;
    gSprites[spriteId].data[1] = d + 2;
    StartSpriteAnim(&gSprites[spriteId], DROP_ANIM_LOWER_HALF);
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    gSprites[spriteId].oam.matrixNum = d + 2;
    CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
    gSprites[spriteId].callback = SpriteCB_WaterDropHalf;

    SetOamMatrix(d, c + 32, 0, 0, c + 32);
    SetOamMatrix(d + 1, c + 32, 0, 0, c + 32);
    SetOamMatrix(d + 2, c + 32, 0, 0, 2 * (c + 32));

    return oldSpriteId;
}

// State is handled by Task_Scene2_BikeRide
static void SpriteCB_PlayerOnBicycle(struct Sprite *sprite)
{
    // Adjust x position
    switch (sprite->sState)
    {
    case 0:
        // Move forwards
        StartSpriteAnimIfDifferent(sprite, 0);
        sprite->x--;
        break;
    case 1:
        // Drift backwards slowly
        StartSpriteAnimIfDifferent(sprite, 0);
        if (gIntroFrameCounter & 7)
            return;
        sprite->x++;
        break;
    case 2:
        // Move backwards
        if (sprite->x <= 120 || gIntroFrameCounter & 7)
            sprite->x++;
        break;
    case 3:
        // Bike in place
        break;
    case 4:
        // Exit to the left
        if (sprite->x > -32)
            sprite->x -= 2;
        break;
    }

    if (gIntroFrameCounter & 7)
        return;

    // Adjust y position
    if (sprite->y2 != 0)
    {
        // Return to neutral after wobble
        sprite->y2 = 0;
    }
    else
    {
        // Random wobble on y axis
        switch (Random() & 3)
        {
        case 0:
            sprite->y2 = -1;
            break;
        case 1:
            sprite->y2 = 1;
            break;
        case 2:
        case 3:
            sprite->y2 = 0;
            break;
        }
    }
}

#define sSinIdx data[1]

// Movement is started by setting state to 1 in Task_Scene2_BikeRide
static void SpriteCB_Flygon(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        break;
    case 1:
        if (sprite->x2 + sprite->x < DISPLAY_WIDTH + 64)
            sprite->x2 += 8;
        else
            sprite->sState = 2;
        break;
    case 2:
        if (sprite->x2 + sprite->x > 120)
            sprite->x2 -= 1;
        else
            sprite->sState = 3;
        break;
    case 3:
        if (sprite->x2 > 0)
            sprite->x2 -= 2;
        break;
    }
    sprite->y2 = Sin((u8)sprite->sSinIdx, 8) - sFlygonYOffset;
    sprite->sSinIdx += 4;
}

#undef sSinIdx

#define sTimer      data[1]
#define sLetterId   data[2]
#define sColorDelay data[3]
#define sLetterX    data[3] // Re-used

#define COLOR_CHANGES 9 // Number of stages for changing the letter color

// For the letters in "Game Freak"
// Also intended for the letters in "Presents", which is never shown
static void SpriteCB_LogoLetter(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        if (sprite->sTimer != 0)
        {
            sprite->sTimer--;
        }
        else
        {
            // Start the grow and shrink appearance animation
            sprite->invisible = FALSE;
            StartSpriteAffineAnim(sprite, 1);
            sprite->sState++;
        }
        break;
    case 1:
        if (gIntroFrameCounter == TIMER_LOGO_LETTERS_COLOR)
        {
            // Initialize color fade
            sprite->sState++;
            sprite->sTimer = COLOR_CHANGES;
            sprite->sColorDelay = 2;
        }
        break;
    case 2:
        // Fade letters to blue
        if (sprite->sColorDelay == 0)
        {
            sprite->sColorDelay = 2;
            if (sprite->sTimer != 0)
            {
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer],      &gPlttBufferFaded[OBJ_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 16], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 4], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 32], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 10], PLTT_SIZEOF(1));
                sprite->sTimer--;
            }
            else
            {
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer],      &gPlttBufferFaded[OBJ_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 16], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 4], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 32], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 10], PLTT_SIZEOF(1));
                sprite->sState++;
            }
        }
        else
        {
            sprite->sColorDelay--;
        }
        break;
    case 3:
        // Fade letters back to white
        if (sprite->sColorDelay != 0)
        {
            sprite->sColorDelay--;
        }
        else
        {
            sprite->sColorDelay = 2;
            if (sprite->sTimer <= COLOR_CHANGES)
            {
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer],      &gPlttBufferFaded[OBJ_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 16], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 4], PLTT_SIZEOF(1));
                CpuCopy16(&gIntroGameFreakTextFade_Pal[sprite->sTimer + 32], &gPlttBufferFaded[OBJ_PLTT_ID(1) + 10], PLTT_SIZEOF(1));
                sprite->sTimer++;
            }
            else
            {
                sprite->sState++;
            }
        }
        break;
    case 4:
        if (gIntroFrameCounter == TIMER_LOGO_DISAPPEAR)
        {
            // Start the grow disappearance animation
            StartSpriteAffineAnim(sprite, 2);
            sprite->oam.objMode = ST_OAM_OBJ_BLEND;
            sprite->sState++;
        }
        break;
    case 5:
        // Spread the letters out as they grow
        sprite->sLetterX += sGameFreakLettersMoveSpeed[sprite->sLetterId];
        sprite->x2 = (sprite->sLetterX & 0xFF00) >> 8;
        if (sprite->sLetterId < 4)
        {
            // Is in first 4 letters, i.e. "Game"
            s16 temp = sprite->x2;
            sprite->x2 = -temp;
        }
        if (sprite->affineAnimEnded)
            DestroySprite(sprite);
        break;
    }
}

static void SpriteCB_GameFreakLogo(struct Sprite *sprite)
{
    switch(sprite->sState)
    {
    case 0:
        if (gIntroFrameCounter == TIMER_LOGO_APPEAR)
        {
            sprite->invisible = FALSE;
            sprite->sState++;
        }
        break;
    case 1:
        if (gIntroFrameCounter == TIMER_LOGO_DISAPPEAR)
        {
            // Start the grow disappearance animation
            StartSpriteAffineAnim(sprite, 3);
            sprite->sState++;
        }
        break;
    case 2:
        if (sprite->affineAnimEnded)
            DestroySprite(sprite);
        break;
    }
}

static u8 CreateGameFreakLogoSprites(s16 x, s16 y, s16 unused)
{
    u16 i;
    u8 spriteId;

    // Create "Game Freak" letters
    for (i = 0; i < NUM_GF_LETTERS; i++)
    {
        spriteId = CreateSprite(&sSpriteTemplate_GameFreakLetter, sGameFreakLetterData[i][1] + x, y - 4, 0);
        gSprites[spriteId].sState = 0;
        gSprites[spriteId].sTimer = sGameFreakLetterStartDelays[i];
        gSprites[spriteId].sLetterId = i;
        gSprites[spriteId].invisible = TRUE;
        gSprites[spriteId].oam.matrixNum = i + 12;
        StartSpriteAnim(&gSprites[spriteId], sGameFreakLetterData[i][0]);
        StartSpriteAffineAnim(&gSprites[spriteId], 0);
    }

    // Create Game Freak logo
    spriteId = CreateSprite(&sSpriteTemplate_GameFreakLogo, 120, y - 6, 0);
    gSprites[spriteId].sState = 0;
    gSprites[spriteId].invisible = TRUE;
    gSprites[spriteId].oam.matrixNum = i + 12;
    StartSpriteAffineAnim(&gSprites[spriteId], 1);

    return spriteId;
}

#undef sTimer
#undef sLetterId
#undef sColorDelay
#undef sLetterX
#undef COLOR_CHANGES

#define sScale   data[1]
#define sRot     data[2]
#define sPos     data[3]
#define sTimer   data[7]

static void SpriteCB_FlygonSilhouette(struct Sprite *sprite)
{
    sprite->sTimer++;

    if (sprite->sState != 0)
    {
        s16 sin;
        s16 cos;

        s16 a, b, c, d;
        // Determines rotation of the sprite
        sin = gSineTable[(u8)sprite->sRot];
        cos = gSineTable[(u8)(sprite->sRot + 64)];
        // Converts rotation and scale into the OAM matrix
        d = Q_8_8_TO_INT( cos * sprite->sScale);
        c = Q_8_8_TO_INT(-sin * sprite->sScale);
        b = Q_8_8_TO_INT( sin * sprite->sScale);
        a = Q_8_8_TO_INT( cos * sprite->sScale);

        SetOamMatrix(1, a, b, c, d);
    }

    switch (sprite->sState)
    {
    case 0:
    default:
        sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
        sprite->oam.matrixNum = 1;
        CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_DOUBLE);
        sprite->invisible = FALSE;
        sprite->sState = 1;
        sprite->sScale = 128;
        sprite->sRot = 0;
        sprite->sPos = 0;
        break;
    case 1:
        sprite->x2 = -Sin((u8)sprite->sPos, 140);
        sprite->y2 = -Sin((u8)sprite->sPos, 120);
        sprite->sScale += 7;
        sprite->sPos += 3;
        if (sprite->x + sprite->x2 <= -16)
        {
            sprite->oam.priority = 3;
            sprite->sState++;
            sprite->x = 20;
            sprite->y = 40;
            sprite->sScale = 512;
            sprite->sRot = 0;
            sprite->sPos = 16;
        }
        break;
    case 2:
        sprite->x2 = Sin((u8)sprite->sPos, 34);
        sprite->y2 = -Cos((u8)sprite->sPos, 60);
        sprite->sScale += 2;
        if (sprite->sTimer % 5 == 0)
            sprite->sPos++;
        break;
    }
}

#undef sScale
#undef sRot
#undef sPos
#undef sTimer

static void SpriteCB_RayquazaOrb(struct Sprite *sprite)
{
    u16 foo;
    switch (sprite->sState)
    {
    case 0:
    default:
        sprite->invisible = FALSE;
        sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
        sprite->oam.matrixNum = 18;
        CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x64), SPRITE_SIZE(64x64), ST_OAM_AFFINE_DOUBLE);
        sprite->data[1] = 0;
        sprite->sState = 1;
        //fall through
    case 1:
        sprite->data[7]++;
        if (sprite->data[7] & 1)
        {
            sprite->invisible = TRUE;
        }
        else
        {
            sprite->invisible = FALSE;
            if (sprite->data[1] < 64)
                sprite->data[1]++;
        }
        foo = 256 - gSineTable[(u8)sprite->data[1]] / 2;
        SetOamMatrix(18, foo, 0, 0, foo);
        break;
    }
}
