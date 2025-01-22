#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_message.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "evolution_scene.h"
#include "evolution_graphics.h"
#include "gpu_regs.h"
#include "link.h"
#include "link_rfu.h"
#include "m4a.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "trade.h"
#include "util.h"
#include "constants/battle_string_ids.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "constants/items.h"

extern struct Evolution gEvolutionTable[][EVOS_PER_MON];

struct EvoInfo
{
    u8 preEvoSpriteId;
    u8 postEvoSpriteId;
    u8 evoTaskId;
    u8 delayTimer;
    u16 savedPalette[48];
};

static EWRAM_DATA struct EvoInfo *sEvoStructPtr = NULL;
static EWRAM_DATA u16 *sBgAnimPal = NULL;

COMMON_DATA void (*gCB2_AfterEvolution)(void) = NULL;

#define sEvoCursorPos           gBattleCommunication[1] // when learning a new move
#define sEvoGraphicsTaskId      gBattleCommunication[2]

static void Task_EvolutionScene(u8 taskId);
static void Task_TradeEvolutionScene(u8 taskId);
static void CB2_EvolutionSceneUpdate(void);
static void CB2_TradeEvolutionSceneUpdate(void);
static void EvoDummyFunc(void);
static void VBlankCB_EvolutionScene(void);
static void VBlankCB_TradeEvolutionScene(void);
static void EvoScene_DoMonAnimAndCry(u8 monSpriteId, u16 speciesId);
static bool32 EvoScene_IsMonAnimFinished(u8 monSpriteId);
static void StartBgAnimation(bool8 isLink);
static void StopBgAnimation(void);
static void Task_AnimateBg(u8 taskId);
static void RestoreBgAfterAnim(void);

static const u16 sUnusedPal1[] = INCBIN_U16("graphics/evolution_scene/unused_1.gbapal");
static const u32 sBgAnim_Gfx[] = INCBIN_U32("graphics/evolution_scene/bg.4bpp.lz");
static const u32 sBgAnim_Inner_Tilemap[] = INCBIN_U32("graphics/evolution_scene/bg_inner.bin.lz");
static const u32 sBgAnim_Outer_Tilemap[] = INCBIN_U32("graphics/evolution_scene/bg_outer.bin.lz");
static const u16 sBgAnim_Intro_Pal[] = INCBIN_U16("graphics/evolution_scene/bg_anim_intro.gbapal");
static const u16 sUnusedPal2[] = INCBIN_U16("graphics/evolution_scene/unused_2.gbapal");
static const u16 sUnusedPal3[]  = INCBIN_U16("graphics/evolution_scene/unused_3.gbapal");
static const u16 sUnusedPal4[] = INCBIN_U16("graphics/evolution_scene/unused_4.gbapal");
static const u16 sBgAnim_Pal[] = INCBIN_U16("graphics/evolution_scene/bg_anim.gbapal");

static const u8 sText_ShedinjaJapaneseName[] = _("ヌケニン");

// The below table is used by Task_UpdateBgPalette to control the speed at which the bg color updates.
// The first two values are indexes into sBgAnim_PalIndexes (indirectly, via sBgAnimPal), and are
// the start and end of the range of colors in sBgAnim_PalIndexes it will move through incrementally
// before starting over. It will repeat this cycle x number of times, where x = the 3rd value,
// delaying each increment by y, where y = the 4th value.
// Once it has cycled x number of times, it will move to the next array in this table.
static const u8 sBgAnim_PaletteControl[][4] =
{
    {  0, 12, 1, 6 },
    { 13, 36, 5, 2 },
    { 13, 24, 1, 2 },
    { 37, 49, 1, 6 },
};

// Indexes into sBgAnim_Pal, 0 is black, transitioning to a bright light blue (172, 213, 255) at 13
static const u8 sBgAnim_PalIndexes[][16] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  0,  0 },
    {  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  0,  0 },
    {  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0, 11,  0,  0 },
    {  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,  0,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,  0,  0 },
    {  0,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 12,  0,  0 },
    {  0,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 12, 11,  0,  0 },
    {  0,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 12, 11, 10,  0,  0 },
    {  0,  5,  6,  7,  8,  9, 10, 11, 12, 13, 12, 11, 10,  9,  0,  0 },
    {  0,  6,  7,  8,  9, 10, 11, 12, 13, 12, 11, 10,  9,  8,  0,  0 },
    {  0,  7,  8,  9, 10, 11, 12, 13, 12, 11, 10,  9,  8,  7,  0,  0 },
    {  0,  8,  9, 10, 11, 12, 13, 12, 11, 10,  9,  8,  7,  6,  0,  0 },
    {  0,  9, 10, 11, 12, 13, 12, 11, 10,  9,  8,  7,  6,  5,  0,  0 },
    {  0, 10, 11, 12, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  0,  0 },
    {  0, 11, 12, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  0,  0 },
    {  0, 12, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  0,  0 },
    {  0, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0 },
    {  0, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  2,  0,  0 },
    {  0, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  2,  3,  0,  0 },
    {  0, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  2,  3,  4,  0,  0 },
    {  0,  9,  8,  7,  6,  5,  4,  3,  2,  1,  2,  3,  4,  5,  0,  0 },
    {  0,  8,  7,  6,  5,  4,  3,  2,  1,  2,  3,  4,  5,  6,  0,  0 },
    {  0,  7,  6,  5,  4,  3,  2,  1,  2,  3,  4,  5,  6,  7,  0,  0 },
    {  0,  6,  5,  4,  3,  2,  1,  2,  3,  4,  5,  6,  7,  8,  0,  0 },
    {  0,  5,  4,  3,  2,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0 },
    {  0,  4,  3,  2,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  0,  0 },
    {  0,  3,  2,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  0,  0 },
    {  0,  2,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,  0,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,  0,  0 },
    {  0, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0 },
    {  0, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0 },
    {  0, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0 },
    {  0,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0 },
    {  0,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

static void CB2_BeginEvolutionScene(void)
{
    UpdatePaletteFade();
    RunTasks();
}

#define tState              data[0]
#define tPreEvoSpecies      data[1]
#define tPostEvoSpecies     data[2]
#define tCanStop            data[3]
#define tBits               data[3]
#define tLearnsFirstMove    data[4]
#define tLearnMoveState     data[6]
#define tLearnMoveYesState  data[7]
#define tLearnMoveNoState   data[8]
#define tEvoWasStopped      data[9]
#define tPartyId            data[10]

#define TASK_BIT_CAN_STOP       (1 << 0)
#define TASK_BIT_LEARN_MOVE     (1 << 7)

static void Task_BeginEvolutionScene(u8 taskId)
{
    struct Pokemon *mon = NULL;
    switch (gTasks[taskId].tState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        gTasks[taskId].tState++;
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            u16 postEvoSpecies;
            bool8 canStopEvo;
            u8 partyId;

            mon = &gPlayerParty[gTasks[taskId].tPartyId];
            postEvoSpecies = gTasks[taskId].tPostEvoSpecies;
            canStopEvo = gTasks[taskId].tCanStop;
            partyId = gTasks[taskId].tPartyId;

            DestroyTask(taskId);
            EvolutionScene(mon, postEvoSpecies, canStopEvo, partyId);
        }
        break;
    }
}

void BeginEvolutionScene(struct Pokemon *mon, u16 postEvoSpecies, bool8 canStopEvo, u8 partyId)
{
    u8 taskId = CreateTask(Task_BeginEvolutionScene, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tPostEvoSpecies = postEvoSpecies;
    gTasks[taskId].tCanStop = canStopEvo;
    gTasks[taskId].tPartyId = partyId;
    SetMainCallback2(CB2_BeginEvolutionScene);
}

void EvolutionScene(struct Pokemon *mon, u16 postEvoSpecies, bool8 canStopEvo, u8 partyId)
{
    u8 name[POKEMON_NAME_BUFFER_SIZE];
    u16 currSpecies;
    u32 trainerId, personality;
    const struct CompressedSpritePalette* pokePal;
    u8 id;

    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);
    CpuFill32(0, (void *)(VRAM), VRAM_SIZE);

    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WIN1H, 0);
    SetGpuReg(REG_OFFSET_WIN1V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);

    ResetPaletteFade();

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 256;
    gBattle_BG3_Y = 0;

    gBattleEnvironment = BATTLE_ENVIRONMENT_PLAIN;

    InitBattleBgsVideo();
    LoadBattleTextboxAndBackground();
    ResetSpriteData();
    ScanlineEffect_Stop();
    ResetTasks();
    FreeAllSpritePalettes();

    gReservedSpritePaletteCount = 4;

    sEvoStructPtr = AllocZeroed(sizeof(struct EvoInfo));
    AllocateMonSpritesGfx();

    GetMonData(mon, MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar1, name);
    StringCopy(gStringVar2, gSpeciesNames[postEvoSpecies]);

    // preEvo sprite
    currSpecies = GetMonData(mon, MON_DATA_SPECIES);
    trainerId = GetMonData(mon, MON_DATA_OT_ID);
    personality = GetMonData(mon, MON_DATA_PERSONALITY);
    DecompressPicFromTable_2(&gMonFrontPicTable[currSpecies],
                             gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                             currSpecies);
    pokePal = GetMonSpritePalStructFromOtIdPersonality(currSpecies, trainerId, personality);
    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(1), PLTT_SIZE_4BPP);

    SetMultiuseSpriteTemplateToPokemon(currSpecies, B_POSITION_OPPONENT_LEFT);
    gMultiuseSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->preEvoSpriteId = id = CreateSprite(&gMultiuseSpriteTemplate, 120, 64, 30);

    gSprites[id].callback = SpriteCallbackDummy_2;
    gSprites[id].oam.paletteNum = 1;
    gSprites[id].invisible = TRUE;

    // postEvo sprite
    DecompressPicFromTable_2(&gMonFrontPicTable[postEvoSpecies],
                             gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_RIGHT],
                             postEvoSpecies);
    pokePal = GetMonSpritePalStructFromOtIdPersonality(postEvoSpecies, trainerId, personality);
    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);

    SetMultiuseSpriteTemplateToPokemon(postEvoSpecies, B_POSITION_OPPONENT_RIGHT);
    gMultiuseSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->postEvoSpriteId = id = CreateSprite(&gMultiuseSpriteTemplate, 120, 64, 30);
    gSprites[id].callback = SpriteCallbackDummy_2;
    gSprites[id].oam.paletteNum = 2;
    gSprites[id].invisible = TRUE;

    LoadEvoSparkleSpriteAndPal();

    sEvoStructPtr->evoTaskId = id = CreateTask(Task_EvolutionScene, 0);
    gTasks[id].tState = 0;
    gTasks[id].tPreEvoSpecies = currSpecies;
    gTasks[id].tPostEvoSpecies = postEvoSpecies;
    gTasks[id].tCanStop = canStopEvo;
    gTasks[id].tLearnsFirstMove = TRUE;
    gTasks[id].tEvoWasStopped = FALSE;
    gTasks[id].tPartyId = partyId;

    memcpy(&sEvoStructPtr->savedPalette, &gPlttBufferUnfaded[BG_PLTT_ID(2)], sizeof(sEvoStructPtr->savedPalette));

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);

    SetHBlankCallback(EvoDummyFunc);
    SetVBlankCallback(VBlankCB_EvolutionScene);
    m4aMPlayAllStop();
    SetMainCallback2(CB2_EvolutionSceneUpdate);
}

static void CB2_EvolutionSceneLoadGraphics(void)
{
    u8 id;
    const struct CompressedSpritePalette* pokePal;
    u16 postEvoSpecies;
    u32 trainerId, personality;
    struct Pokemon *mon = &gPlayerParty[gTasks[sEvoStructPtr->evoTaskId].tPartyId];

    postEvoSpecies = gTasks[sEvoStructPtr->evoTaskId].tPostEvoSpecies;
    trainerId = GetMonData(mon, MON_DATA_OT_ID);
    personality = GetMonData(mon, MON_DATA_PERSONALITY);

    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);
    CpuFill32(0, (void *)(VRAM), VRAM_SIZE);

    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WIN1H, 0);
    SetGpuReg(REG_OFFSET_WIN1V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);

    ResetPaletteFade();

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 256;
    gBattle_BG3_Y = 0;

    gBattleEnvironment = BATTLE_ENVIRONMENT_PLAIN;

    InitBattleBgsVideo();
    LoadBattleTextboxAndBackground();
    ResetSpriteData();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = 4;

    DecompressPicFromTable_2(&gMonFrontPicTable[postEvoSpecies],
                             gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_RIGHT],
                             postEvoSpecies);
    pokePal = GetMonSpritePalStructFromOtIdPersonality(postEvoSpecies, trainerId, personality);

    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);

    SetMultiuseSpriteTemplateToPokemon(postEvoSpecies, B_POSITION_OPPONENT_RIGHT);
    gMultiuseSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->postEvoSpriteId = id = CreateSprite(&gMultiuseSpriteTemplate, 120, 64, 30);

    gSprites[id].callback = SpriteCallbackDummy_2;
    gSprites[id].oam.paletteNum = 2;

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);

    SetHBlankCallback(EvoDummyFunc);
    SetVBlankCallback(VBlankCB_EvolutionScene);
    SetMainCallback2(CB2_EvolutionSceneUpdate);

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static void CB2_TradeEvolutionSceneLoadGraphics(void)
{
    struct Pokemon *mon = &gPlayerParty[gTasks[sEvoStructPtr->evoTaskId].tPartyId];
    u16 postEvoSpecies = gTasks[sEvoStructPtr->evoTaskId].tPostEvoSpecies;

    switch (gMain.state)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        SetHBlankCallback(NULL);
        SetVBlankCallback(NULL);
        ResetSpriteData();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 4;
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        gBattle_BG2_X = 0;
        gBattle_BG2_Y = 0;
        gBattle_BG3_X = 256;
        gBattle_BG3_Y = 0;
        gMain.state++;
        break;
    case 1:
        ResetPaletteFade();
        SetHBlankCallback(EvoDummyFunc);
        SetVBlankCallback(VBlankCB_TradeEvolutionScene);
        gMain.state++;
        break;
    case 2:
        LoadTradeAnimGfx();
        gMain.state++;
        break;
    case 3:
        FillBgTilemapBufferRect(1, 0, 0, 0, 0x20, 0x20, 0x11);
        CopyBgTilemapBufferToVram(1);
        gMain.state++;
        break;
    case 4:
        {
            const struct CompressedSpritePalette* pokePal;
            u32 trainerId = GetMonData(mon, MON_DATA_OT_ID);
            u32 personality = GetMonData(mon, MON_DATA_PERSONALITY);
            DecompressPicFromTable_2(&gMonFrontPicTable[postEvoSpecies],
                                     gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_RIGHT],
                                     postEvoSpecies);
            pokePal = GetMonSpritePalStructFromOtIdPersonality(postEvoSpecies, trainerId, personality);
            LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);
            gMain.state++;
        }
        break;
    case 5:
        {
            u8 id;

            SetMultiuseSpriteTemplateToPokemon(postEvoSpecies, B_POSITION_OPPONENT_LEFT);
            gMultiuseSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
            sEvoStructPtr->postEvoSpriteId = id = CreateSprite(&gMultiuseSpriteTemplate, 120, 64, 30);

            gSprites[id].callback = SpriteCallbackDummy_2;
            gSprites[id].oam.paletteNum = 2;
            gMain.state++;
            LinkTradeDrawWindow();
        }
        break;
    case 6:
        if (gWirelessCommType)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
        }
        BlendPalettes(PALETTES_ALL, 0x10, RGB_BLACK);
        gMain.state++;
        break;
    case 7:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        InitTradeSequenceBgGpuRegs();
        ShowBg(0);
        ShowBg(1);
        SetMainCallback2(CB2_TradeEvolutionSceneUpdate);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG0_ON | DISPCNT_BG1_ON | DISPCNT_OBJ_1D_MAP);
        break;
    }
}

void TradeEvolutionScene(struct Pokemon *mon, u16 postEvoSpecies, u8 preEvoSpriteId, u8 partyId)
{
    u8 name[POKEMON_NAME_BUFFER_SIZE];
    u16 currSpecies;
    u32 trainerId, personality;
    const struct CompressedSpritePalette* pokePal;
    u8 id;

    GetMonData(mon, MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar1, name);
    StringCopy(gStringVar2, gSpeciesNames[postEvoSpecies]);

    gAffineAnimsDisabled = TRUE;

    // preEvo sprite
    currSpecies = GetMonData(mon, MON_DATA_SPECIES);
    personality = GetMonData(mon, MON_DATA_PERSONALITY);
    trainerId = GetMonData(mon, MON_DATA_OT_ID);

    sEvoStructPtr = AllocZeroed(sizeof(struct EvoInfo));
    sEvoStructPtr->preEvoSpriteId = preEvoSpriteId;

    DecompressPicFromTable_2(&gMonFrontPicTable[postEvoSpecies],
                            gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
                            postEvoSpecies);

    pokePal = GetMonSpritePalStructFromOtIdPersonality(postEvoSpecies, trainerId, personality);
    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);

    SetMultiuseSpriteTemplateToPokemon(postEvoSpecies, B_POSITION_OPPONENT_LEFT);
    gMultiuseSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->postEvoSpriteId = id = CreateSprite(&gMultiuseSpriteTemplate, 120, 64, 30);

    gSprites[id].callback = SpriteCallbackDummy_2;
    gSprites[id].oam.paletteNum = 2;
    gSprites[id].invisible = TRUE;

    LoadEvoSparkleSpriteAndPal();

    sEvoStructPtr->evoTaskId = id = CreateTask(Task_TradeEvolutionScene, 0);
    gTasks[id].tState = 0;
    gTasks[id].tPreEvoSpecies = currSpecies;
    gTasks[id].tPostEvoSpecies = postEvoSpecies;
    gTasks[id].tLearnsFirstMove = TRUE;
    gTasks[id].tEvoWasStopped = FALSE;
    gTasks[id].tPartyId = partyId;

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 256;
    gBattle_BG3_Y = 0;

    gTextFlags.useAlternateDownArrow = TRUE;

    SetVBlankCallback(VBlankCB_TradeEvolutionScene);
    SetMainCallback2(CB2_TradeEvolutionSceneUpdate);
}

static void CB2_EvolutionSceneUpdate(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}

static void CB2_TradeEvolutionSceneUpdate(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}

static void CreateShedinja(u16 preEvoSpecies, struct Pokemon *mon)
{
    u32 data = 0;
    if (gEvolutionTable[preEvoSpecies][0].method == EVO_LEVEL_NINJASK && gPlayerPartyCount < PARTY_SIZE)
    {
        s32 i;
        struct Pokemon *shedinja = &gPlayerParty[gPlayerPartyCount];

        CopyMon(&gPlayerParty[gPlayerPartyCount], mon, sizeof(struct Pokemon));
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_SPECIES, &gEvolutionTable[preEvoSpecies][1].targetSpecies);
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_NICKNAME, gSpeciesNames[gEvolutionTable[preEvoSpecies][1].targetSpecies]);
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_HELD_ITEM, &data);
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_MARKINGS, &data);
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_ENCRYPT_SEPARATOR, &data);

        for (i = MON_DATA_COOL_RIBBON; i < MON_DATA_COOL_RIBBON + CONTEST_CATEGORIES_COUNT; i++)
            SetMonData(&gPlayerParty[gPlayerPartyCount], i, &data);
        for (i = MON_DATA_CHAMPION_RIBBON; i <= MON_DATA_UNUSED_RIBBONS; i++)
            SetMonData(&gPlayerParty[gPlayerPartyCount], i, &data);

        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_STATUS, &data);
        data = MAIL_NONE;
        SetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_MAIL, &data);

        CalculateMonStats(&gPlayerParty[gPlayerPartyCount]);
        CalculatePlayerPartyCount();

        GetSetPokedexFlag(SpeciesToNationalPokedexNum(gEvolutionTable[preEvoSpecies][1].targetSpecies), FLAG_SET_SEEN);
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(gEvolutionTable[preEvoSpecies][1].targetSpecies), FLAG_SET_CAUGHT);

        if (GetMonData(shedinja, MON_DATA_SPECIES) == SPECIES_SHEDINJA
            && GetMonData(shedinja, MON_DATA_LANGUAGE) == LANGUAGE_JAPANESE
            && GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NINJASK)
                SetMonData(shedinja, MON_DATA_NICKNAME, sText_ShedinjaJapaneseName);
    }
}

// States for the main switch in Task_EvolutionScene
enum {
    EVOSTATE_FADE_IN,
    EVOSTATE_INTRO_MSG,
    EVOSTATE_INTRO_MON_ANIM,
    EVOSTATE_INTRO_SOUND,
    EVOSTATE_START_MUSIC,
    EVOSTATE_START_BG_AND_SPARKLE_SPIRAL,
    EVOSTATE_SPARKLE_ARC,
    EVOSTATE_CYCLE_MON_SPRITE,
    EVOSTATE_WAIT_CYCLE_MON_SPRITE,
    EVOSTATE_SPARKLE_CIRCLE,
    EVOSTATE_SPARKLE_SPRAY,
    EVOSTATE_EVO_SOUND,
    EVOSTATE_RESTORE_SCREEN,
    EVOSTATE_EVO_MON_ANIM,
    EVOSTATE_SET_MON_EVOLVED,
    EVOSTATE_TRY_LEARN_MOVE,
    EVOSTATE_END,
    EVOSTATE_CANCEL,
    EVOSTATE_CANCEL_MON_ANIM,
    EVOSTATE_CANCEL_MSG,
    EVOSTATE_LEARNED_MOVE,
    EVOSTATE_TRY_LEARN_ANOTHER_MOVE,
    EVOSTATE_REPLACE_MOVE,
};

// States for the switch in EVOSTATE_REPLACE_MOVE
enum {
    MVSTATE_INTRO_MSG_1,
    MVSTATE_INTRO_MSG_2,
    MVSTATE_INTRO_MSG_3,
    MVSTATE_PRINT_YES_NO,
    MVSTATE_HANDLE_YES_NO,
    MVSTATE_SHOW_MOVE_SELECT,
    MVSTATE_HANDLE_MOVE_SELECT,
    MVSTATE_FORGET_MSG_1,
    MVSTATE_FORGET_MSG_2,
    MVSTATE_LEARNED_MOVE,
    MVSTATE_ASK_CANCEL,
    MVSTATE_CANCEL,
    MVSTATE_RETRY_AFTER_HM,
};

// Task data from CycleEvolutionMonSprite
#define tEvoStopped data[8]

static void Task_EvolutionScene(u8 taskId)
{
    u32 var;
    struct Pokemon *mon = &gPlayerParty[gTasks[taskId].tPartyId];

    // check if B Button was held, so the evolution gets stopped
    if (gMain.heldKeys == B_BUTTON
        && gTasks[taskId].tState == EVOSTATE_WAIT_CYCLE_MON_SPRITE
        && gTasks[sEvoGraphicsTaskId].isActive
        && gTasks[taskId].tBits & TASK_BIT_CAN_STOP)
    {
        gTasks[taskId].tState = EVOSTATE_CANCEL;
        gTasks[sEvoGraphicsTaskId].tEvoStopped = TRUE;
        StopBgAnimation();
        return;
    }

    switch (gTasks[taskId].tState)
    {
    case EVOSTATE_FADE_IN:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gSprites[sEvoStructPtr->preEvoSpriteId].invisible = FALSE;
        gTasks[taskId].tState++;
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        break;
    case EVOSTATE_INTRO_MSG:
        if (!gPaletteFade.active)
        {
            StringExpandPlaceholders(gStringVar4, gText_PkmnIsEvolving);
            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_INTRO_MON_ANIM:
        if (!IsTextPrinterActive(0))
        {
            EvoScene_DoMonAnimAndCry(sEvoStructPtr->preEvoSpriteId, gTasks[taskId].tPreEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_INTRO_SOUND:
        if (EvoScene_IsMonAnimFinished(sEvoStructPtr->preEvoSpriteId))
        {
            PlaySE(MUS_EVOLUTION_INTRO);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_START_MUSIC:
        if (!IsSEPlaying())
        {
            // Start music, fade background to black
            PlayNewMapMusic(MUS_EVOLUTION);
            gTasks[taskId].tState++;
            BeginNormalPaletteFade(0x1C, 4, 0, 0x10, RGB_BLACK);
        }
        break;
    case EVOSTATE_START_BG_AND_SPARKLE_SPIRAL:
        if (!gPaletteFade.active)
        {
            StartBgAnimation(FALSE);
            sEvoGraphicsTaskId = EvolutionSparkles_SpiralUpward(17);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SPARKLE_ARC:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            gTasks[taskId].tState++;
            sEvoStructPtr->delayTimer = 1;
            sEvoGraphicsTaskId = EvolutionSparkles_ArcDown();
        }
        break;
    case EVOSTATE_CYCLE_MON_SPRITE: // launch task that flashes pre evo with post evo sprites
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = CycleEvolutionMonSprite(sEvoStructPtr->preEvoSpriteId, sEvoStructPtr->postEvoSpriteId);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_WAIT_CYCLE_MON_SPRITE:
        if (--sEvoStructPtr->delayTimer == 0)
        {
            sEvoStructPtr->delayTimer = 3;
            if (!gTasks[sEvoGraphicsTaskId].isActive)
                gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SPARKLE_CIRCLE:
        sEvoGraphicsTaskId = EvolutionSparkles_CircleInward();
        gTasks[taskId].tState++;
        break;
    case EVOSTATE_SPARKLE_SPRAY:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = EvolutionSparkles_SprayAndFlash(gTasks[taskId].tPostEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_EVO_SOUND:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            PlaySE(SE_EXP);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_RESTORE_SCREEN: // stop music, return screen to pre-fade state
        if (IsSEPlaying())
        {
            m4aMPlayAllStop();
            memcpy(&gPlttBufferUnfaded[BG_PLTT_ID(2)], sEvoStructPtr->savedPalette, sizeof(sEvoStructPtr->savedPalette));
            RestoreBgAfterAnim();
            BeginNormalPaletteFade(0x1C, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_EVO_MON_ANIM:
        if (!gPaletteFade.active)
        {
            EvoScene_DoMonAnimAndCry(sEvoStructPtr->postEvoSpriteId, gTasks[taskId].tPostEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SET_MON_EVOLVED:
        if (IsCryFinished())
        {
            StringExpandPlaceholders(gStringVar4, gText_CongratsPkmnEvolved);
            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            PlayBGM(MUS_EVOLVED);
            gTasks[taskId].tState++;
            SetMonData(mon, MON_DATA_SPECIES, (void *)(&gTasks[taskId].tPostEvoSpecies));
            CalculateMonStats(mon);
            EvolutionRenameMon(mon, gTasks[taskId].tPreEvoSpecies, gTasks[taskId].tPostEvoSpecies);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_SEEN);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_CAUGHT);
            IncrementGameStat(GAME_STAT_EVOLVED_POKEMON);
        }
        break;
    case EVOSTATE_TRY_LEARN_MOVE:
        if (!IsTextPrinterActive(0))
        {
            var = MonTryLearningNewMove(mon, gTasks[taskId].tLearnsFirstMove);
            if (var != MOVE_NONE && !gTasks[taskId].tEvoWasStopped)
            {
                u8 nickname[POKEMON_NAME_BUFFER_SIZE];
                if (!(gTasks[taskId].tBits & TASK_BIT_LEARN_MOVE))
                {
                    StopMapMusic();
                    Overworld_PlaySpecialMapMusic();
                }

                gTasks[taskId].tBits |= TASK_BIT_LEARN_MOVE;
                gTasks[taskId].tLearnsFirstMove = FALSE;
                gTasks[taskId].tLearnMoveState = MVSTATE_INTRO_MSG_1;
                GetMonData(mon, MON_DATA_NICKNAME, nickname);
                StringCopy_Nickname(gBattleTextBuff1, nickname);

                if (var == MON_HAS_MAX_MOVES)
                    gTasks[taskId].tState = EVOSTATE_REPLACE_MOVE;
                else if (var == MON_ALREADY_KNOWS_MOVE)
                    break;
                else
                    gTasks[taskId].tState = EVOSTATE_LEARNED_MOVE;
            }
            else // no move to learn, or evolution was canceled
            {
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                gTasks[taskId].tState++;
            }
        }
        break;
    case EVOSTATE_END:
        if (!gPaletteFade.active)
        {
            if (!(gTasks[taskId].tBits & TASK_BIT_LEARN_MOVE))
            {
                StopMapMusic();
                Overworld_PlaySpecialMapMusic();
            }
            if (!gTasks[taskId].tEvoWasStopped)
                CreateShedinja(gTasks[taskId].tPreEvoSpecies, mon);

            DestroyTask(taskId);
            FreeMonSpritesGfx();
            FREE_AND_SET_NULL(sEvoStructPtr);
            FreeAllWindowBuffers();
            SetMainCallback2(gCB2_AfterEvolution);
        }
        break;
    case EVOSTATE_CANCEL:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            m4aMPlayAllStop();
            BeginNormalPaletteFade(0x6001C, 0, 0x10, 0, RGB_WHITE);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_CANCEL_MON_ANIM:
        if (!gPaletteFade.active)
        {
            EvoScene_DoMonAnimAndCry(sEvoStructPtr->preEvoSpriteId, gTasks[taskId].tPreEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_CANCEL_MSG:
        if (EvoScene_IsMonAnimFinished(sEvoStructPtr->preEvoSpriteId))
        {
            if (gTasks[taskId].tEvoWasStopped) // FRLG auto cancellation
                StringExpandPlaceholders(gStringVar4, gText_EllipsisQuestionMark);
            else
                StringExpandPlaceholders(gStringVar4, gText_PkmnStoppedEvolving);

            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            gTasks[taskId].tEvoWasStopped = TRUE;
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
        }
        break;
    case EVOSTATE_LEARNED_MOVE:
        if (!IsTextPrinterActive(0) && !IsSEPlaying())
        {
            BufferMoveToLearnIntoBattleTextBuff2();
            PlayFanfare(MUS_LEVEL_UP);
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNLEARNEDMOVE - BATTLESTRINGS_TABLE_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnsFirstMove = 0x40; // re-used as a counter
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_TRY_LEARN_ANOTHER_MOVE:
        if (!IsTextPrinterActive(0) && !IsSEPlaying() && --gTasks[taskId].tLearnsFirstMove == 0)
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
        break;
    case EVOSTATE_REPLACE_MOVE:
        switch (gTasks[taskId].tLearnMoveState)
        {
        case MVSTATE_INTRO_MSG_1:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "{mon} is trying to learn {move}"
                BufferMoveToLearnIntoBattleTextBuff2();
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE1 - BATTLESTRINGS_TABLE_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_INTRO_MSG_2:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "But, {mon} can't learn more than four moves"
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE2 - BATTLESTRINGS_TABLE_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_INTRO_MSG_3:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "Delete a move to make room for {move}?"
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE3 - BATTLESTRINGS_TABLE_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveYesState = MVSTATE_SHOW_MOVE_SELECT;
                gTasks[taskId].tLearnMoveNoState = MVSTATE_ASK_CANCEL;
                gTasks[taskId].tLearnMoveState++;
            }
        case MVSTATE_PRINT_YES_NO:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                HandleBattleWindow(YESNOBOX_X_Y, 0);
                BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
                gTasks[taskId].tLearnMoveState++;
                sEvoCursorPos = 0;
                BattleCreateYesNoCursorAt(0);
            }
            break;
        case MVSTATE_HANDLE_YES_NO:
            // This Yes/No is used for both the initial "delete move?" prompt
            // and for the "stop learning move?" prompt
            // What Yes/No do next is determined by tLearnMoveYesState / tLearnMoveNoState
            if (JOY_NEW(DPAD_UP) && sEvoCursorPos != 0)
            {
                // Moved onto YES
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(sEvoCursorPos);
                sEvoCursorPos = 0;
                BattleCreateYesNoCursorAt(0);
            }
            if (JOY_NEW(DPAD_DOWN) && sEvoCursorPos == 0)
            {
                // Moved onto NO
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(sEvoCursorPos);
                sEvoCursorPos = 1;
                BattleCreateYesNoCursorAt(1);
            }
            if (JOY_NEW(A_BUTTON))
            {
                HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
                PlaySE(SE_SELECT);

                if (sEvoCursorPos != 0)
                {
                    // NO
                    gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveNoState;
                }
                else
                {
                    // YES
                    gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveYesState;
                    if (gTasks[taskId].tLearnMoveState == MVSTATE_SHOW_MOVE_SELECT)
                        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                }
            }
            if (JOY_NEW(B_BUTTON))
            {
                // Equivalent to selecting NO
                HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
                PlaySE(SE_SELECT);
                gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveNoState;
            }
            break;
        case MVSTATE_SHOW_MOVE_SELECT:
            if (!gPaletteFade.active)
            {
                FreeAllWindowBuffers();
                ShowSelectMovePokemonSummaryScreen(gPlayerParty, gTasks[taskId].tPartyId,
                            gPlayerPartyCount - 1, CB2_EvolutionSceneLoadGraphics,
                            gMoveToLearn);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_HANDLE_MOVE_SELECT:
            if (!gPaletteFade.active && gMain.callback2 == CB2_EvolutionSceneUpdate)
            {
                var = GetMoveSlotToReplace();
                if (var == MAX_MON_MOVES)
                {
                    // Didn't select move slot
                    gTasks[taskId].tLearnMoveState = MVSTATE_ASK_CANCEL;
                }
                else
                {
                    // Selected move to forget
                    u16 move = GetMonData(mon, var + MON_DATA_MOVE1);
                    if (IsHMMove2(move))
                    {
                        // Can't forget HMs
                        BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_HMMOVESCANTBEFORGOTTEN - BATTLESTRINGS_TABLE_START]);
                        BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                        gTasks[taskId].tLearnMoveState = MVSTATE_RETRY_AFTER_HM;
                    }
                    else
                    {
                        // Forget move
                        PREPARE_MOVE_BUFFER(gBattleTextBuff2, move)

                        RemoveMonPPBonus(mon, var);
                        SetMonMoveSlot(mon, gMoveToLearn, var);
                        gTasks[taskId].tLearnMoveState++;
                    }
                }
            }
            break;
        case MVSTATE_FORGET_MSG_1:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_123POOF - BATTLESTRINGS_TABLE_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnMoveState++;
            break;
        case MVSTATE_FORGET_MSG_2:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNFORGOTMOVE - BATTLESTRINGS_TABLE_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_LEARNED_MOVE:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_ANDELLIPSIS - BATTLESTRINGS_TABLE_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tState = EVOSTATE_LEARNED_MOVE;
            }
            break;
        case MVSTATE_ASK_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_STOPLEARNINGMOVE - BATTLESTRINGS_TABLE_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnMoveYesState = MVSTATE_CANCEL;
            gTasks[taskId].tLearnMoveNoState = MVSTATE_INTRO_MSG_1;
            gTasks[taskId].tLearnMoveState = MVSTATE_PRINT_YES_NO;
            break;
        case MVSTATE_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_DIDNOTLEARNMOVE - BATTLESTRINGS_TABLE_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
            break;
        case MVSTATE_RETRY_AFTER_HM:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
                gTasks[taskId].tLearnMoveState = MVSTATE_SHOW_MOVE_SELECT;
            break;
        }
        break;
    }
}

// States for the main switch in Task_TradeEvolutionScene
enum {
    T_EVOSTATE_INTRO_MSG,
    T_EVOSTATE_INTRO_CRY,
    T_EVOSTATE_INTRO_SOUND,
    T_EVOSTATE_START_MUSIC,
    T_EVOSTATE_START_BG_AND_SPARKLE_SPIRAL,
    T_EVOSTATE_SPARKLE_ARC,
    T_EVOSTATE_CYCLE_MON_SPRITE,
    T_EVOSTATE_WAIT_CYCLE_MON_SPRITE,
    T_EVOSTATE_SPARKLE_CIRCLE,
    T_EVOSTATE_SPARKLE_SPRAY,
    T_EVOSTATE_EVO_SOUND,
    T_EVOSTATE_EVO_MON_ANIM,
    T_EVOSTATE_SET_MON_EVOLVED,
    T_EVOSTATE_TRY_LEARN_MOVE,
    T_EVOSTATE_END,
    T_EVOSTATE_CANCEL,
    T_EVOSTATE_CANCEL_MON_ANIM,
    T_EVOSTATE_CANCEL_MSG,
    T_EVOSTATE_LEARNED_MOVE,
    T_EVOSTATE_TRY_LEARN_ANOTHER_MOVE,
    T_EVOSTATE_REPLACE_MOVE,
};

// States for the switch in T_EVOSTATE_REPLACE_MOVE
enum {
    T_MVSTATE_INTRO_MSG_1,
    T_MVSTATE_INTRO_MSG_2,
    T_MVSTATE_INTRO_MSG_3,
    T_MVSTATE_PRINT_YES_NO,
    T_MVSTATE_HANDLE_YES_NO,
    T_MVSTATE_SHOW_MOVE_SELECT,
    T_MVSTATE_HANDLE_MOVE_SELECT,
    T_MVSTATE_FORGET_MSG,
    T_MVSTATE_LEARNED_MOVE,
    T_MVSTATE_ASK_CANCEL,
    T_MVSTATE_CANCEL,
    T_MVSTATE_RETRY_AFTER_HM,
};

// Compare to Task_EvolutionScene, very similar
static void Task_TradeEvolutionScene(u8 taskId)
{
    u32 var = 0;
    struct Pokemon *mon = &gPlayerParty[gTasks[taskId].tPartyId];

    switch (gTasks[taskId].tState)
    {
    case T_EVOSTATE_INTRO_MSG:
        StringExpandPlaceholders(gStringVar4, gText_PkmnIsEvolving);
        DrawTextOnTradeWindow(0, gStringVar4, 1);
        gTasks[taskId].tState++;
        break;
    case T_EVOSTATE_INTRO_CRY:
        if (!IsTextPrinterActive(0))
        {
            PlayCry_Normal(gTasks[taskId].tPreEvoSpecies, 0);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_INTRO_SOUND:
        if (IsCryFinished())
        {
            m4aSongNumStop(MUS_EVOLUTION);
            PlaySE(MUS_EVOLUTION_INTRO);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_START_MUSIC:
        if (!IsSEPlaying())
        {
            PlayBGM(MUS_EVOLUTION);
            gTasks[taskId].tState++;
            BeginNormalPaletteFade(0x1C, 4, 0, 0x10, RGB_BLACK);
        }
        break;
    case T_EVOSTATE_START_BG_AND_SPARKLE_SPIRAL:
        if (!gPaletteFade.active)
        {
            StartBgAnimation(TRUE);
            var = gSprites[sEvoStructPtr->preEvoSpriteId].oam.paletteNum + 16;
            sEvoGraphicsTaskId = EvolutionSparkles_SpiralUpward(var);
            gTasks[taskId].tState++;
            SetGpuReg(REG_OFFSET_BG3CNT, BGCNT_PRIORITY(3) | BGCNT_SCREENBASE(6));
        }
        break;
    case T_EVOSTATE_SPARKLE_ARC:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            gTasks[taskId].tState++;
            sEvoStructPtr->delayTimer = 1;
            sEvoGraphicsTaskId = EvolutionSparkles_ArcDown();
        }
        break;
    case T_EVOSTATE_CYCLE_MON_SPRITE:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = CycleEvolutionMonSprite(sEvoStructPtr->preEvoSpriteId, sEvoStructPtr->postEvoSpriteId);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_WAIT_CYCLE_MON_SPRITE:
        if (--sEvoStructPtr->delayTimer == 0)
        {
            sEvoStructPtr->delayTimer = 3;
            if (!gTasks[sEvoGraphicsTaskId].isActive)
                gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_SPARKLE_CIRCLE:
        sEvoGraphicsTaskId = EvolutionSparkles_CircleInward();
        gTasks[taskId].tState++;
        break;
    case T_EVOSTATE_SPARKLE_SPRAY:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = EvolutionSparkles_SprayAndFlash_Trade(gTasks[taskId].tPostEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_EVO_SOUND:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            PlaySE(SE_EXP);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_EVO_MON_ANIM:
        if (IsSEPlaying())
        {
            // Restore bg, do mon anim/cry
            Free(sBgAnimPal);
            EvoScene_DoMonAnimAndCry(sEvoStructPtr->postEvoSpriteId, gTasks[taskId].tPostEvoSpecies);
            memcpy(&gPlttBufferUnfaded[BG_PLTT_ID(2)], sEvoStructPtr->savedPalette, sizeof(sEvoStructPtr->savedPalette));
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_SET_MON_EVOLVED:
        if (IsCryFinished())
        {
            StringExpandPlaceholders(gStringVar4, gText_CongratsPkmnEvolved);
            DrawTextOnTradeWindow(0, gStringVar4, 1);
            PlayFanfare(MUS_EVOLVED);
            gTasks[taskId].tState++;
            SetMonData(mon, MON_DATA_SPECIES, (&gTasks[taskId].tPostEvoSpecies));
            CalculateMonStats(mon);
            EvolutionRenameMon(mon, gTasks[taskId].tPreEvoSpecies, gTasks[taskId].tPostEvoSpecies);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_SEEN);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_CAUGHT);
            IncrementGameStat(GAME_STAT_EVOLVED_POKEMON);
        }
        break;
    case T_EVOSTATE_TRY_LEARN_MOVE:
        if (!IsTextPrinterActive(0) && IsFanfareTaskInactive() == TRUE)
        {
            var = MonTryLearningNewMove(mon, gTasks[taskId].tLearnsFirstMove);
            if (var != MOVE_NONE && !gTasks[taskId].tEvoWasStopped)
            {
                u8 nickname[POKEMON_NAME_BUFFER_SIZE];
                gTasks[taskId].tBits |= TASK_BIT_LEARN_MOVE;
                gTasks[taskId].tLearnsFirstMove = FALSE;
                gTasks[taskId].tLearnMoveState = 0;
                GetMonData(mon, MON_DATA_NICKNAME, nickname);
                StringCopy_Nickname(gBattleTextBuff1, nickname);

                if (var == MON_HAS_MAX_MOVES)
                    gTasks[taskId].tState = T_EVOSTATE_REPLACE_MOVE;
                else if (var == MON_ALREADY_KNOWS_MOVE)
                    break;
                else
                    gTasks[taskId].tState = T_EVOSTATE_LEARNED_MOVE;
            }
            else
            {
                PlayBGM(MUS_EVOLUTION);
                DrawTextOnTradeWindow(0, gText_CommunicationStandby5, 1);
                gTasks[taskId].tState++;
            }
        }
        break;
    case T_EVOSTATE_END:
        if (!IsTextPrinterActive(0))
        {
            DestroyTask(taskId);
            FREE_AND_SET_NULL(sEvoStructPtr);
            gTextFlags.useAlternateDownArrow = FALSE;
            SetMainCallback2(gCB2_AfterEvolution);
        }
        break;
    case T_EVOSTATE_CANCEL:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            m4aMPlayAllStop();
            BeginNormalPaletteFade((1 << (gSprites[sEvoStructPtr->preEvoSpriteId].oam.paletteNum + 16)) | (0x4001C), 0, 0x10, 0, RGB_WHITE);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_CANCEL_MON_ANIM:
        if (!gPaletteFade.active)
        {
            EvoScene_DoMonAnimAndCry(sEvoStructPtr->preEvoSpriteId, gTasks[taskId].tPreEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_CANCEL_MSG:
        if (EvoScene_IsMonAnimFinished(sEvoStructPtr->preEvoSpriteId))
        {
            StringExpandPlaceholders(gStringVar4, gText_EllipsisQuestionMark);
            DrawTextOnTradeWindow(0, gStringVar4, 1);
            gTasks[taskId].tEvoWasStopped = TRUE;
            gTasks[taskId].tState = T_EVOSTATE_TRY_LEARN_MOVE;
        }
        break;
    case T_EVOSTATE_LEARNED_MOVE:
        if (!IsTextPrinterActive(0) && !IsSEPlaying())
        {
            BufferMoveToLearnIntoBattleTextBuff2();
            PlayFanfare(MUS_LEVEL_UP);
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNLEARNEDMOVE - BATTLESTRINGS_TABLE_START]);
            DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
            gTasks[taskId].tLearnsFirstMove = 0x40; // re-used as a counter
            gTasks[taskId].tState++;
        }
        break;
    case T_EVOSTATE_TRY_LEARN_ANOTHER_MOVE:
        if (!IsTextPrinterActive(0) && IsFanfareTaskInactive() == TRUE && --gTasks[taskId].tLearnsFirstMove == 0)
            gTasks[taskId].tState = T_EVOSTATE_TRY_LEARN_MOVE;
        break;
    case T_EVOSTATE_REPLACE_MOVE:
        switch (gTasks[taskId].tLearnMoveState)
        {
        case T_MVSTATE_INTRO_MSG_1:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "{mon} is trying to learn {move}"
                BufferMoveToLearnIntoBattleTextBuff2();
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE1 - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case T_MVSTATE_INTRO_MSG_2:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "But, {mon} can't learn more than four moves"
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE2 - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case T_MVSTATE_INTRO_MSG_3:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                // "Delete a move to make room for {move}?"
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE3 - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveYesState = T_MVSTATE_SHOW_MOVE_SELECT;
                gTasks[taskId].tLearnMoveNoState = T_MVSTATE_ASK_CANCEL;
                gTasks[taskId].tLearnMoveState++;
            }
        case T_MVSTATE_PRINT_YES_NO:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                LoadUserWindowBorderGfx(0, 0xA8, BG_PLTT_ID(14));
                CreateYesNoMenu(&gTradeEvolutionSceneYesNoWindowTemplate, 0xA8, 0xE, 0);
                sEvoCursorPos = 0;
                gTasks[taskId].tLearnMoveState++;
                sEvoCursorPos = 0;
            }
            break;
        case T_MVSTATE_HANDLE_YES_NO:
            switch (Menu_ProcessInputNoWrapClearOnChoose())
            {
            case 0: // YES
                sEvoCursorPos = 0;
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_EMPTYSTRING3 - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveYesState;
                if (gTasks[taskId].tLearnMoveState == T_MVSTATE_SHOW_MOVE_SELECT)
                    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                break;
            case 1: // NO
            case MENU_B_PRESSED:
                sEvoCursorPos = 1;
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_EMPTYSTRING3 - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveNoState;
                break;
            }
            break;
        case T_MVSTATE_SHOW_MOVE_SELECT:
            if (!gPaletteFade.active)
            {
                if (gWirelessCommType)
                    DestroyWirelessStatusIndicatorSprite();

                Free(GetBgTilemapBuffer(3));
                Free(GetBgTilemapBuffer(1));
                Free(GetBgTilemapBuffer(0));
                FreeAllWindowBuffers();

                ShowSelectMovePokemonSummaryScreen(gPlayerParty, gTasks[taskId].tPartyId,
                            gPlayerPartyCount - 1, CB2_TradeEvolutionSceneLoadGraphics,
                            gMoveToLearn);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case T_MVSTATE_HANDLE_MOVE_SELECT:
            if (!gPaletteFade.active && gMain.callback2 == CB2_TradeEvolutionSceneUpdate)
            {
                var = GetMoveSlotToReplace();
                if (var == MAX_MON_MOVES)
                {
                    // Didn't select move slot
                    gTasks[taskId].tLearnMoveState = T_MVSTATE_ASK_CANCEL;
                }
                else
                {
                    // Selected move to forget
                    u16 move = GetMonData(mon, var + MON_DATA_MOVE1);
                    if (IsHMMove2(move))
                    {
                        // Can't forget HMs
                        BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_HMMOVESCANTBEFORGOTTEN - BATTLESTRINGS_TABLE_START]);
                        DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                        gTasks[taskId].tLearnMoveState = T_MVSTATE_RETRY_AFTER_HM;
                    }
                    else
                    {
                        // Forget move
                        PREPARE_MOVE_BUFFER(gBattleTextBuff2, move)

                        RemoveMonPPBonus(mon, var);
                        SetMonMoveSlot(mon, gMoveToLearn, var);
                        BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_123POOF - BATTLESTRINGS_TABLE_START]);
                        DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                        gTasks[taskId].tLearnMoveState++;
                    }
                }
            }
            break;
        case T_MVSTATE_FORGET_MSG:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNFORGOTMOVE - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case T_MVSTATE_LEARNED_MOVE:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_ANDELLIPSIS - BATTLESTRINGS_TABLE_START]);
                DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
                gTasks[taskId].tState = T_EVOSTATE_LEARNED_MOVE;
            }
            break;
        case T_MVSTATE_ASK_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_STOPLEARNINGMOVE - BATTLESTRINGS_TABLE_START]);
            DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
            gTasks[taskId].tLearnMoveYesState = T_MVSTATE_CANCEL;
            gTasks[taskId].tLearnMoveNoState = T_MVSTATE_INTRO_MSG_1;
            gTasks[taskId].tLearnMoveState = T_MVSTATE_PRINT_YES_NO;
            break;
        case T_MVSTATE_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_DIDNOTLEARNMOVE - BATTLESTRINGS_TABLE_START]);
            DrawTextOnTradeWindow(0, gDisplayedStringBattle, 1);
            gTasks[taskId].tState = T_EVOSTATE_TRY_LEARN_MOVE;
            break;
        case T_MVSTATE_RETRY_AFTER_HM:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
                gTasks[taskId].tLearnMoveState = T_MVSTATE_SHOW_MOVE_SELECT;
            break;
        }
        break;
    }
}

#undef tState
#undef tPreEvoSpecies
#undef tPostEvoSpecies
#undef tCanStop
#undef tBits
#undef tLearnsFirstMove
#undef tLearnMoveState
#undef tLearnMoveYesState
#undef tLearnMoveNoState
#undef tEvoWasStopped
#undef tPartyId

static void EvoDummyFunc(void)
{
}

static void VBlankCB_EvolutionScene(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
    SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
    SetGpuReg(REG_OFFSET_BG3HOFS, gBattle_BG3_X);
    SetGpuReg(REG_OFFSET_BG3VOFS, gBattle_BG3_Y);

    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void VBlankCB_TradeEvolutionScene(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
    SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
    SetGpuReg(REG_OFFSET_BG3HOFS, gBattle_BG3_X);
    SetGpuReg(REG_OFFSET_BG3VOFS, gBattle_BG3_Y);

    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

#define tCycleTimer   data[0]
#define tPalStage     data[1]
#define tControlStage data[2]
#define tNumCycles    data[3]
#define tStartTimer   data[5]
#define tPaused       data[6]

// See comments above sBgAnim_PaletteControl
#define START_PAL sBgAnim_PaletteControl[tControlStage][0]
#define END_PAL   sBgAnim_PaletteControl[tControlStage][1]
#define CYCLES    sBgAnim_PaletteControl[tControlStage][2]
#define DELAY     sBgAnim_PaletteControl[tControlStage][3]

// Cycles the background through a set range of palettes in a series
// of stages, each stage having a different palette range and timing
static void Task_UpdateBgPalette(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tPaused)
        return;
    if (tStartTimer++ < 20)
        return;

    if (tCycleTimer++ > DELAY)
    {
        if (END_PAL == tPalStage)
        {
            // Reached final palette in current stage, completed a 'cycle'
            // If this is the final cycle for this stage, move to the next stage
            tNumCycles++;
            if (tNumCycles == CYCLES)
            {
                tNumCycles = 0;
                tControlStage++;
            }
            tPalStage = START_PAL;
        }
        else
        {
            // Haven't reached final palette in current stage, load the current palette
            LoadPalette(&sBgAnimPal[tPalStage * 16], BG_PLTT_ID(10), PLTT_SIZE_4BPP);
            tCycleTimer = 0;
            tPalStage++;
        }
    }

    if (tControlStage == (int)ARRAY_COUNT(sBgAnim_PaletteControl[0]))
        DestroyTask(taskId);
}

#undef tCycleTimer
#undef tPalStage
#undef tControlStage
#undef tNumCycles
#undef tStartTimer
#undef START_PAL
#undef END_PAL
#undef CYCLES
#undef DELAY

#define tIsLink data[2]

static void CreateBgAnimTask(bool8 isLink)
{
    u8 taskId = CreateTask(Task_AnimateBg, 7);

    if (!isLink)
        gTasks[taskId].tIsLink = FALSE;
    else
        gTasks[taskId].tIsLink = TRUE;
}

static void Task_AnimateBg(u8 taskId)
{
    u16 *outer_X, *outer_Y;

    u16 *inner_X = &gBattle_BG1_X;
    u16 *inner_Y = &gBattle_BG1_Y;

    if (!gTasks[taskId].tIsLink)
    {
        outer_X = &gBattle_BG2_X;
        outer_Y = &gBattle_BG2_Y;
    }
    else
    {
        outer_X = &gBattle_BG3_X;
        outer_Y = &gBattle_BG3_Y;
    }

    gTasks[taskId].data[0] = (gTasks[taskId].data[0] + 5) & 0xFF;
    gTasks[taskId].data[1] = (gTasks[taskId].data[0] + 0x80) & 0xFF;

    *inner_X = Cos(gTasks[taskId].data[0], 4) + 8;
    *inner_Y = Sin(gTasks[taskId].data[0], 4) + 16;

    *outer_X = Cos(gTasks[taskId].data[1], 4) + 8;
    *outer_Y = Sin(gTasks[taskId].data[1], 4) + 16;

    if (!FuncIsActiveTask(Task_UpdateBgPalette))
    {
        DestroyTask(taskId);

        *inner_X = 0;
        *inner_Y = 0;

        *outer_X = 256;
        *outer_Y = 0;
    }
}

#undef tIsLink

static void InitMovingBgPalette(u16 *palette)
{
    s32 i, j;

    for (i = 0; i < (int)ARRAY_COUNT(sBgAnim_PalIndexes); i++)
    {
        for (j = 0; j < 16; j++)
        {
            palette[i * 16 + j] = sBgAnim_Pal[sBgAnim_PalIndexes[i][j]];
        }
    }
}

static void StartBgAnimation(bool8 isLink)
{
    u8 innerBgId, outerBgId;

    sBgAnimPal = AllocZeroed(0x640);
    InitMovingBgPalette(sBgAnimPal);

    if (!isLink)
        innerBgId = 1, outerBgId = 2;
    else
        innerBgId = 1, outerBgId = 3;

    LoadPalette(sBgAnim_Intro_Pal, BG_PLTT_ID(10), PLTT_SIZE_4BPP);

    DecompressAndLoadBgGfxUsingHeap(1, sBgAnim_Gfx, FALSE, 0, 0);
    CopyToBgTilemapBuffer(innerBgId, sBgAnim_Inner_Tilemap, 0, 0);
    CopyToBgTilemapBuffer(outerBgId, sBgAnim_Outer_Tilemap, 0, 0);
    CopyBgTilemapBufferToVram(innerBgId);
    CopyBgTilemapBufferToVram(outerBgId);

    if (!isLink)
    {
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG2);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 8));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG2_ON | DISPCNT_BG1_ON | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP);

        SetBgAttribute(innerBgId, BG_ATTR_PRIORITY, 2);
        SetBgAttribute(outerBgId, BG_ATTR_PRIORITY, 2);

        ShowBg(1);
        ShowBg(2);
    }
    else
    {
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 8));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG3_ON | DISPCNT_BG1_ON | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP);
    }

    CreateTask(Task_UpdateBgPalette, 5);
    CreateBgAnimTask(isLink);
}

static void UNUSED PauseBgPaletteAnim(void)
{
    u8 taskId = FindTaskIdByFunc(Task_UpdateBgPalette);

    if (taskId != TASK_NONE)
        gTasks[taskId].tPaused = TRUE;

    FillPalette(RGB_BLACK, BG_PLTT_ID(10), PLTT_SIZE_4BPP);
}

#undef tPaused

static void StopBgAnimation(void)
{
    u8 taskId;

    if ((taskId = FindTaskIdByFunc(Task_UpdateBgPalette)) != TASK_NONE)
        DestroyTask(taskId);
    if ((taskId = FindTaskIdByFunc(Task_AnimateBg)) != TASK_NONE)
        DestroyTask(taskId);

    FillPalette(RGB_BLACK, BG_PLTT_ID(10), PLTT_SIZE_4BPP);
    RestoreBgAfterAnim();
}

static void RestoreBgAfterAnim(void)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    SetBgAttribute(1, BG_ATTR_PRIORITY, GetBattleBgTemplateData(1, 5));
    SetBgAttribute(2, BG_ATTR_PRIORITY, GetBattleBgTemplateData(2, 5));
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG3_ON | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP);
    Free(sBgAnimPal);
}

static void EvoScene_DoMonAnimAndCry(u8 monSpriteId, u16 speciesId)
{
    DoMonFrontSpriteAnimation(&gSprites[monSpriteId], speciesId, FALSE, 0);
}

static bool32 EvoScene_IsMonAnimFinished(u8 monSpriteId)
{
    if (gSprites[monSpriteId].callback == SpriteCallbackDummy)
        return TRUE;

    return FALSE;
}
