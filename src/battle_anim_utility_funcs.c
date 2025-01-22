#include "global.h"
#include "battle_anim.h"
#include "contest.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "malloc.h"
#include "palette.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "util.h"
#include "constants/rgb.h"
#include "constants/songs.h"

struct AnimStatsChangeData
{
    u8 battler1;
    u8 battler2;
    bool8 hidBattler2;
    s16 data[8];
    u16 species;
};

static EWRAM_DATA struct AnimStatsChangeData *sAnimStatsChangeData = {0};

static void StartBlendAnimSpriteColor(u8, u32);
static void AnimTask_BlendSpriteColor_Step2(u8);
static void AnimTask_HardwarePaletteFade_Step(u8);
static void AnimTask_TraceMonBlended_Step(u8);
static void AnimMonTrace(struct Sprite *);
static void AnimTask_DrawFallingWhiteLinesOnAttacker_Step(u8);
static void StatsChangeAnimation_Step1(u8);
static void StatsChangeAnimation_Step2(u8);
static void StatsChangeAnimation_Step3(u8);
static void AnimTask_Flash_Step(u8);
static void SetPalettesToColor(u32, u16);
static void AnimTask_UpdateSlidingBg(u8);
static void UpdateMonScrollingBgMask(u8);
static void AnimTask_WaitAndRestoreVisibility(u8);

static const u16 sCurseLinesPalette[] = { RGB_WHITE };

// These belong in battle_intro.c, but putting them there causes 2 bytes of alignment padding
// between the two .rodata segments. Perhaps battle_intro.c actually belongs in this file, too.
const u8 gBattleAnimBgCntSet[] = {REG_OFFSET_BG0CNT, REG_OFFSET_BG1CNT, REG_OFFSET_BG2CNT, REG_OFFSET_BG3CNT};
const u8 gBattleAnimBgCntGet[] = {REG_OFFSET_BG0CNT, REG_OFFSET_BG1CNT, REG_OFFSET_BG2CNT, REG_OFFSET_BG3CNT};

void AnimTask_BlendBattleAnimPal(u8 taskId)
{
    u32 selectedPalettes = UnpackSelectedBattlePalettes(gBattleAnimArgs[0]);
    selectedPalettes |= GetBattleMonSpritePalettesMask((gBattleAnimArgs[0] >>  7) & 1,
                                    (gBattleAnimArgs[0] >>  8) & 1,
                                    (gBattleAnimArgs[0] >>  9) & 1,
                                    (gBattleAnimArgs[0] >> 10) & 1);
    StartBlendAnimSpriteColor(taskId, selectedPalettes);
}

void AnimTask_BlendBattleAnimPalExclude(u8 taskId)
{
    u8 battler;
    u32 selectedPalettes;
    u8 animBattlers[2];

    animBattlers[1] = 0xFF;
    selectedPalettes = UnpackSelectedBattlePalettes(F_PAL_BG);
    switch (gBattleAnimArgs[0])
    {
    case 2:
        selectedPalettes = 0;
        // fall through
    case ANIM_ATTACKER:
#ifdef UBFIX
    default:
#endif
        animBattlers[0] = gBattleAnimAttacker;
        break;
    case 3:
        selectedPalettes = 0;
        // fall through
    case ANIM_TARGET:
        animBattlers[0] = gBattleAnimTarget;
        break;
    case 4:
        animBattlers[0] = gBattleAnimAttacker;
        animBattlers[1] = gBattleAnimTarget;
        break;
    case 5:
        animBattlers[0] = 0xFF;
        break;
    case 6:
        selectedPalettes = 0;
        animBattlers[0] = BATTLE_PARTNER(gBattleAnimAttacker);
        break;
    case 7:
        selectedPalettes = 0;
        animBattlers[0] = BATTLE_PARTNER(gBattleAnimTarget);
        break;
    }

    for (battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        if (battler != animBattlers[0] && battler != animBattlers[1] && IsBattlerSpriteVisible(battler))
            selectedPalettes |= 0x10000 << GetSpritePalIdxByBattler(battler);
    }

    StartBlendAnimSpriteColor(taskId, selectedPalettes);
}

void AnimTask_SetCamouflageBlend(u8 taskId)
{
    u32 selectedPalettes = UnpackSelectedBattlePalettes(gBattleAnimArgs[0]);
    switch (gBattleEnvironment)
    {
    case BATTLE_ENVIRONMENT_GRASS:
        gBattleAnimArgs[4] = RGB(12, 24, 2);
        break;
    case BATTLE_ENVIRONMENT_LONG_GRASS:
        gBattleAnimArgs[4] = RGB(0, 15, 2);
        break;
    case BATTLE_ENVIRONMENT_SAND:
        gBattleAnimArgs[4] = RGB(30, 24, 11);
        break;
    case BATTLE_ENVIRONMENT_UNDERWATER:
        gBattleAnimArgs[4] = RGB(0, 0, 18);
        break;
    case BATTLE_ENVIRONMENT_WATER:
        gBattleAnimArgs[4] = RGB(11, 22, 31);
        break;
    case BATTLE_ENVIRONMENT_POND:
        gBattleAnimArgs[4] = RGB(11, 22, 31);
        break;
    case BATTLE_ENVIRONMENT_MOUNTAIN:
        gBattleAnimArgs[4] = RGB(22, 16, 10);
        break;
    case BATTLE_ENVIRONMENT_CAVE:
        gBattleAnimArgs[4] = RGB(14, 9, 3);
        break;
    case BATTLE_ENVIRONMENT_BUILDING:
        gBattleAnimArgs[4] = RGB_WHITE;
        break;
    case BATTLE_ENVIRONMENT_PLAIN:
        gBattleAnimArgs[4] = RGB_WHITE;
        break;
    }

    StartBlendAnimSpriteColor(taskId, selectedPalettes);
}

void AnimTask_BlendParticle(u8 taskId)
{
    u8 paletteIndex = IndexOfSpritePaletteTag(gBattleAnimArgs[0]);
    u32 selectedPalettes = 1 << (paletteIndex + 16);
    StartBlendAnimSpriteColor(taskId, selectedPalettes);
}

void StartBlendAnimSpriteColor(u8 taskId, u32 selectedPalettes)
{
    gTasks[taskId].data[0] = selectedPalettes;
    gTasks[taskId].data[1] = selectedPalettes >> 16;
    gTasks[taskId].data[2] = gBattleAnimArgs[1];
    gTasks[taskId].data[3] = gBattleAnimArgs[2];
    gTasks[taskId].data[4] = gBattleAnimArgs[3];
    gTasks[taskId].data[5] = gBattleAnimArgs[4];
    gTasks[taskId].data[10] = gBattleAnimArgs[2];
    gTasks[taskId].func = AnimTask_BlendSpriteColor_Step2;
    gTasks[taskId].func(taskId);
}

static void AnimTask_BlendSpriteColor_Step2(u8 taskId)
{
    u32 selectedPalettes;
    u16 singlePaletteOffset = 0;

    if (gTasks[taskId].data[9] == gTasks[taskId].data[2])
    {
        gTasks[taskId].data[9] = 0;
        selectedPalettes = gTasks[taskId].data[0] | (gTasks[taskId].data[1] << 16);
        while (selectedPalettes != 0)
        {
            if (selectedPalettes & 1)
                BlendPalette(singlePaletteOffset, 16, gTasks[taskId].data[10], gTasks[taskId].data[5]);
            singlePaletteOffset += 16;
            selectedPalettes >>= 1;
        }

        if (gTasks[taskId].data[10] < gTasks[taskId].data[4])
            gTasks[taskId].data[10]++;
        else if (gTasks[taskId].data[10] > gTasks[taskId].data[4])
            gTasks[taskId].data[10]--;
        else
            DestroyAnimVisualTask(taskId);
    }
    else
    {
        gTasks[taskId].data[9]++;
    }
}

void AnimTask_HardwarePaletteFade(u8 taskId)
{
    BeginHardwarePaletteFade(
        gBattleAnimArgs[0],
        gBattleAnimArgs[1],
        gBattleAnimArgs[2],
        gBattleAnimArgs[3],
        gBattleAnimArgs[4]);

    gTasks[taskId].func = AnimTask_HardwarePaletteFade_Step;
}

static void AnimTask_HardwarePaletteFade_Step(u8 taskId)
{
    if (!gPaletteFade.active)
        DestroyAnimVisualTask(taskId);
}

// Used to leave blended traces of a mon, usually to imply speed as in Agility or Aerial Ace
void AnimTask_TraceMonBlended(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    task->data[0] = gBattleAnimArgs[0];
    task->data[1] = 0;
    task->data[2] = gBattleAnimArgs[1];
    task->data[3] = gBattleAnimArgs[2];
    task->data[4] = gBattleAnimArgs[3];
    task->data[5] = 0;
    task->func = AnimTask_TraceMonBlended_Step;
}

static void AnimTask_TraceMonBlended_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (task->data[4])
    {
        if (task->data[1])
        {
            task->data[1]--;
        }
        else
        {
            task->data[6] = CloneBattlerSpriteWithBlend(task->data[0]);
            if (task->data[6] >= 0)
            {
                gSprites[task->data[6]].oam.priority = task->data[0] ? 1 : 2;
                gSprites[task->data[6]].data[0] = task->data[3];
                gSprites[task->data[6]].data[1] = taskId;
                gSprites[task->data[6]].data[2] = 5;
                gSprites[task->data[6]].callback = AnimMonTrace;
                task->data[5]++;
            }

            task->data[4]--;
            task->data[1] = task->data[2];
        }
    }
    else if (task->data[5] == 0)
    {
        DestroyAnimVisualTask(taskId);
    }
}

static void AnimMonTrace(struct Sprite *sprite)
{
    if (sprite->data[0])
    {
        sprite->data[0]--;
    }
    else
    {
        gTasks[sprite->data[1]].data[sprite->data[2]]--;
        DestroySpriteWithActiveSheet(sprite);
    }
}

// Only used by Curse for non-Ghost mons
void AnimTask_DrawFallingWhiteLinesOnAttacker(u8 taskId)
{
    u16 species;
    int spriteId, newSpriteId;
    u16 var0;
    u16 bg1Cnt;
    struct BattleAnimBgData animBgData;

    var0 = 0;
    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR
                              | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR
                               | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 12));
    bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
    ((struct BgCnt *)&bg1Cnt)->priority = 0;
    ((struct BgCnt *)&bg1Cnt)->screenSize = 0;
    SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);

    if (!IsContest())
    {
        ((struct BgCnt *)&bg1Cnt)->charBaseBlock = 1;
        SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);
    }

    if (IsDoubleBattle() && !IsContest())
    {
        if (GetBattlerPosition(gBattleAnimAttacker) == B_POSITION_OPPONENT_RIGHT
         || GetBattlerPosition(gBattleAnimAttacker) == B_POSITION_PLAYER_LEFT)
        {
            if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)) == TRUE)
            {
                gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority -= 1;
                ((struct BgCnt *)&bg1Cnt)->priority = 1;
                SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);
                var0 = 1;
            }
        }
    }

    if (IsContest())
    {
        species = gContestResources->moveAnim->species;
    }
    else
    {
        if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattleAnimAttacker]], MON_DATA_SPECIES);
        else
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gBattleAnimAttacker]], MON_DATA_SPECIES);
    }

    spriteId = GetAnimBattlerSpriteId(ANIM_ATTACKER);
    newSpriteId = CreateInvisibleSpriteCopy(gBattleAnimAttacker, spriteId, species);
    GetBattleAnimBg1Data(&animBgData);
    AnimLoadCompressedBgTilemapHandleContest(&animBgData, gBattleAnimMaskTilemap_Curse, FALSE);
    AnimLoadCompressedBgGfx(animBgData.bgId, gBattleAnimMaskImage_Curse, animBgData.tilesOffset);
    LoadPalette(sCurseLinesPalette, BG_PLTT_ID(animBgData.paletteId) + 1, PLTT_SIZEOF(1));

    gBattle_BG1_X = -gSprites[spriteId].x + 32;
    gBattle_BG1_Y = -gSprites[spriteId].y + 32;
    gTasks[taskId].data[0] = newSpriteId;
    gTasks[taskId].data[6] = var0;
    gTasks[taskId].func = AnimTask_DrawFallingWhiteLinesOnAttacker_Step;
}

static void AnimTask_DrawFallingWhiteLinesOnAttacker_Step(u8 taskId)
{
    struct BattleAnimBgData animBgData;
    struct Sprite *sprite;
    u16 bg1Cnt;

    gTasks[taskId].data[10] += 4;
    gBattle_BG1_Y -= 4;
    if (gTasks[taskId].data[10] == 64)
    {
        gTasks[taskId].data[10] = 0;
        gBattle_BG1_Y += 64;
        if (++gTasks[taskId].data[11] == 4)
        {
            ResetBattleAnimBg(FALSE);
            gBattle_WIN0H = 0;
            gBattle_WIN0V = 0;
            SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR
                                      | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL  | WINOUT_WIN01_OBJ  | WINOUT_WIN01_CLR
                                       | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
            if (!IsContest())
            {
                bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
                ((struct BgCnt *)&bg1Cnt)->charBaseBlock = 0;
                SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);
            }

            SetGpuReg(REG_OFFSET_DISPCNT, GetGpuReg(REG_OFFSET_DISPCNT) ^ DISPCNT_OBJWIN_ON);
            SetGpuReg(REG_OFFSET_BLDCNT, 0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
            sprite = &gSprites[GetAnimBattlerSpriteId(0)]; // unused
            sprite = &gSprites[gTasks[taskId].data[0]];
            DestroySprite(sprite);

            GetBattleAnimBg1Data(&animBgData);
            ClearBattleAnimBg(animBgData.bgId);
            if (gTasks[taskId].data[6] == 1)
                gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority++;

            gBattle_BG1_Y = 0;
            DestroyAnimVisualTask(taskId);
        }
    }
}

// Defines for data array in sAnimStatsChangeData
#define aDecrease         data[0]
#define aAnimStatId       data[1]
#define aIsTarget         data[2]
#define aMultipleBattlers data[3] // Always false. Changes for multiple battlers are instead run sequentially.
#define aSharply          data[4]

// Task data defines for InitStatsChangeAnimation
#define tAnimSpriteId1    data[0]
#define tVelocity         data[1]
#define tMultipleBattlers data[2]
#define tAnimSpriteId2    data[3]
#define tTargetBlend      data[4]
#define tWaitTime         data[5]
#define tHidBattler2      data[6]
#define tBattler2SpriteId data[7]
#define tWaitTimer        data[10]
#define tFadeTimer        data[11]
#define tBlend            data[12]
#define tState            data[15]

void InitStatsChangeAnimation(u8 taskId)
{
    u8 i;

    sAnimStatsChangeData = AllocZeroed(sizeof(struct AnimStatsChangeData));
    for (i = 0; i < ARRAY_COUNT(sAnimStatsChangeData->data); i++)
        sAnimStatsChangeData->data[i] = gBattleAnimArgs[i];

    gTasks[taskId].func = StatsChangeAnimation_Step1;
}

static void StatsChangeAnimation_Step1(u8 taskId)
{
    if (!sAnimStatsChangeData->aIsTarget)
        sAnimStatsChangeData->battler1 = gBattleAnimAttacker;
    else
        sAnimStatsChangeData->battler1 = gBattleAnimTarget;

    sAnimStatsChangeData->battler2 = BATTLE_PARTNER(sAnimStatsChangeData->battler1);
    if (IsContest() || (sAnimStatsChangeData->aMultipleBattlers && !IsBattlerSpriteVisible(sAnimStatsChangeData->battler2)))
        sAnimStatsChangeData->aMultipleBattlers = FALSE;

    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL | WININ_WIN1_ALL);
    SetGpuReg(REG_OFFSET_WINOUT, (WINOUT_WIN01_ALL & ~WINOUT_WIN01_BG1) | WINOUT_WINOBJ_ALL);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 0);
    SetAnimBgAttribute(1, BG_ANIM_SCREEN_SIZE, 0);
    if (!IsContest())
        SetAnimBgAttribute(1, BG_ANIM_CHAR_BASE_BLOCK, 1);

    if (IsDoubleBattle() && !sAnimStatsChangeData->aMultipleBattlers)
    {
        if (GetBattlerPosition(sAnimStatsChangeData->battler1) == B_POSITION_OPPONENT_RIGHT
         || GetBattlerPosition(sAnimStatsChangeData->battler1) == B_POSITION_PLAYER_LEFT)
        {
            if (IsBattlerSpriteVisible(sAnimStatsChangeData->battler2) == TRUE)
            {
                // Push the battler not being animated back so it doesn't receive the animation.
                gSprites[gBattlerSpriteIds[sAnimStatsChangeData->battler2]].oam.priority--;
                SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 1);
                sAnimStatsChangeData->hidBattler2 = TRUE;
            }
        }
    }

    if (IsContest())
    {
        sAnimStatsChangeData->species = gContestResources->moveAnim->species;
    }
    else
    {
        if (GetBattlerSide(sAnimStatsChangeData->battler1) != B_SIDE_PLAYER)
            sAnimStatsChangeData->species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[sAnimStatsChangeData->battler1]], MON_DATA_SPECIES);
        else
            sAnimStatsChangeData->species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[sAnimStatsChangeData->battler1]], MON_DATA_SPECIES);
    }

    gTasks[taskId].func = StatsChangeAnimation_Step2;
}

static void StatsChangeAnimation_Step2(u8 taskId)
{
    struct BattleAnimBgData animBgData;
    u8 spriteId, spriteId2;
    u8 battlerSpriteId;

    spriteId2 = 0;
    battlerSpriteId = gBattlerSpriteIds[sAnimStatsChangeData->battler1];
    spriteId = CreateInvisibleSpriteCopy(sAnimStatsChangeData->battler1, battlerSpriteId, sAnimStatsChangeData->species);
    if (sAnimStatsChangeData->aMultipleBattlers)
    {
        battlerSpriteId = gBattlerSpriteIds[sAnimStatsChangeData->battler2];
        spriteId2 = CreateInvisibleSpriteCopy(sAnimStatsChangeData->battler2, battlerSpriteId, sAnimStatsChangeData->species);
    }

    GetBattleAnimBg1Data(&animBgData);
    if (!sAnimStatsChangeData->aDecrease)
        AnimLoadCompressedBgTilemapHandleContest(&animBgData, gStatAnim_Increase_Tilemap, FALSE);
    else
        AnimLoadCompressedBgTilemapHandleContest(&animBgData, gStatAnim_Decrease_Tilemap, FALSE);

    AnimLoadCompressedBgGfx(animBgData.bgId, gStatAnim_Gfx, animBgData.tilesOffset);
    switch (sAnimStatsChangeData->aAnimStatId)
    {
    case STAT_ANIM_PAL_ATK:
        LoadCompressedPalette(gStatAnim_Attack_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_DEF:
        LoadCompressedPalette(gStatAnim_Defense_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_ACC:
        LoadCompressedPalette(gStatAnim_Accuracy_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_SPEED:
        LoadCompressedPalette(gStatAnim_Speed_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_EVASION:
        LoadCompressedPalette(gStatAnim_Evasion_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_SPATK:
        LoadCompressedPalette(gStatAnim_SpAttack_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    case STAT_ANIM_PAL_SPDEF:
        LoadCompressedPalette(gStatAnim_SpDefense_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    default:
 // case STAT_ANIM_PAL_MULTIPLE:
        LoadCompressedPalette(gStatAnim_Multiple_Pal, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);
        break;
    }

    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;

    if (sAnimStatsChangeData->aDecrease == TRUE)
    {
        gBattle_BG1_X = 64;
        gTasks[taskId].tVelocity = -3;
    }
    else
    {
        gTasks[taskId].tVelocity = 3;
    }

    if (!sAnimStatsChangeData->aSharply)
    {
        gTasks[taskId].tTargetBlend = 10;
        gTasks[taskId].tWaitTime = 20;
    }
    else
    {
        gTasks[taskId].tTargetBlend = 13;
        gTasks[taskId].tWaitTime = 30;
    }

    gTasks[taskId].tAnimSpriteId1 = spriteId;
    gTasks[taskId].tMultipleBattlers = sAnimStatsChangeData->aMultipleBattlers;
    gTasks[taskId].tAnimSpriteId2 = spriteId2;
    gTasks[taskId].tHidBattler2 = sAnimStatsChangeData->hidBattler2;
    gTasks[taskId].tBattler2SpriteId = gBattlerSpriteIds[sAnimStatsChangeData->battler2];
    gTasks[taskId].func = StatsChangeAnimation_Step3;

    if (!sAnimStatsChangeData->aDecrease)
        PlaySE12WithPanning(SE_M_STAT_INCREASE, BattleAnimAdjustPanning2(SOUND_PAN_ATTACKER));
    else
        PlaySE12WithPanning(SE_M_STAT_DECREASE, BattleAnimAdjustPanning2(SOUND_PAN_ATTACKER));
}

static void StatsChangeAnimation_Step3(u8 taskId)
{
    gBattle_BG1_Y += gTasks[taskId].tVelocity;

    switch (gTasks[taskId].tState)
    {
    case 0:
        // Fade in
        if (gTasks[taskId].tFadeTimer++ > 0)
        {
            gTasks[taskId].tFadeTimer = 0;
            gTasks[taskId].tBlend++;
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].tBlend, 16 - gTasks[taskId].tBlend));
            if (gTasks[taskId].tBlend == gTasks[taskId].tTargetBlend)
                gTasks[taskId].tState++;
        }
        break;
    case 1:
        // Wait
        if (++gTasks[taskId].tWaitTimer == gTasks[taskId].tWaitTime)
            gTasks[taskId].tState++;
        break;
    case 2:
        // Fade out
        if (gTasks[taskId].tFadeTimer++ > 0)
        {
            gTasks[taskId].tFadeTimer = 0;
            gTasks[taskId].tBlend--;
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].tBlend, 16 - gTasks[taskId].tBlend));
            if (gTasks[taskId].tBlend == 0)
            {
                ResetBattleAnimBg(FALSE);
                gTasks[taskId].tState++;
            }
        }
        break;
    case 3:
        // Reset
        gBattle_WIN0H = 0;
        gBattle_WIN0V = 0;
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL | WININ_WIN1_ALL);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_ALL | WINOUT_WINOBJ_ALL);

        if (!IsContest())
            SetAnimBgAttribute(1, BG_ANIM_CHAR_BASE_BLOCK, 0);

        SetGpuReg(REG_OFFSET_DISPCNT, GetGpuReg(REG_OFFSET_DISPCNT) ^ DISPCNT_OBJWIN_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);

        DestroySprite(&gSprites[gTasks[taskId].tAnimSpriteId1]);
        if (gTasks[taskId].tMultipleBattlers)
            DestroySprite(&gSprites[gTasks[taskId].tAnimSpriteId2]);

        // Restore battler 2's priority
        if (gTasks[taskId].tHidBattler2 == TRUE)
            gSprites[gTasks[taskId].tBattler2SpriteId].oam.priority++;

        FREE_AND_SET_NULL(sAnimStatsChangeData);
        DestroyAnimVisualTask(taskId);
        break;
    }
}

#undef aDecrease
#undef aAnimStatId
#undef aIsTarget
#undef aMultipleBattlers
#undef aSharply
#undef tAnimSpriteId1
#undef tVelocity
#undef tMultipleBattlers
#undef tAnimSpriteId2
#undef tTargetBlend
#undef tWaitTime
#undef tHidBattler2
#undef tBattler2SpriteId
#undef tWaitTimer
#undef tFadeTimer
#undef tBlend
#undef tState

void AnimTask_Flash(u8 taskId)
{
    u32 selectedPalettes = GetBattleMonSpritePalettesMask(1, 1, 1, 1);
    SetPalettesToColor(selectedPalettes, RGB_BLACK);
    gTasks[taskId].data[14] = selectedPalettes >> 16;

    selectedPalettes = GetBattlePalettesMask(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE) & 0xFFFF;
    SetPalettesToColor(selectedPalettes, RGB_WHITEALPHA);
    gTasks[taskId].data[15] = selectedPalettes;

    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].func = AnimTask_Flash_Step;
}

static void AnimTask_Flash_Step(u8 taskId)
{
    u16 i;
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        if (++task->data[1] > 6)
        {
            task->data[1] = 0;
            task->data[2] = 16;
            task->data[0]++;
        }
        break;
    case 1:
        if (++task->data[1] > 1)
        {
            task->data[1] = 0;
            task->data[2]--;

            for (i = 0; i < 16; i++)
            {
                if ((task->data[15] >> i) & 1)
                    BlendPalette(BG_PLTT_ID(i), 16, task->data[2], 0xFFFF);

                if ((task->data[14] >> i) & 1)
                    BlendPalette(OBJ_PLTT_ID(i), 16, task->data[2], 0);
            }

            if (task->data[2] == 0)
                task->data[0]++;
        }
        break;
    case 2:
        DestroyAnimVisualTask(taskId);
        break;
    }
}

static void SetPalettesToColor(u32 selectedPalettes, u16 color)
{
    u16 i;

    for (i = 0; i < 32; i++)
    {
        if (selectedPalettes & 1)
        {
            u16 curOffset = PLTT_ID(i);
            u16 paletteOffset = curOffset;
            while (curOffset < paletteOffset + 16)
            {
                gPlttBufferFaded[curOffset] = color;
                curOffset++;
            }
        }

        selectedPalettes >>= 1;
    }
}

void AnimTask_BlendNonAttackerPalettes(u8 taskId)
{
    u32 battler;
    int j;
    u32 selectedPalettes = 0;

    for (battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        if (gBattleAnimAttacker != battler)
            selectedPalettes |= 1 << (battler + 16);
    }

    for (j = 5; j != 0; j--)
        gBattleAnimArgs[j] = gBattleAnimArgs[j - 1];

    StartBlendAnimSpriteColor(taskId, selectedPalettes);
}

void AnimTask_StartSlidingBg(u8 taskId)
{
    u8 newTaskId;

    UpdateAnimBg3ScreenSize(FALSE);
    newTaskId = CreateTask(AnimTask_UpdateSlidingBg, 5);
    if (gBattleAnimArgs[2] && GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        gBattleAnimArgs[0] = -gBattleAnimArgs[0];
        gBattleAnimArgs[1] = -gBattleAnimArgs[1];
    }

    gTasks[newTaskId].data[1] = gBattleAnimArgs[0];
    gTasks[newTaskId].data[2] = gBattleAnimArgs[1];
    gTasks[newTaskId].data[3] = gBattleAnimArgs[3];
    gTasks[newTaskId].data[0]++;
    DestroyAnimVisualTask(taskId);
}

static void AnimTask_UpdateSlidingBg(u8 taskId)
{
    gTasks[taskId].data[10] += gTasks[taskId].data[1];
    gTasks[taskId].data[11] += gTasks[taskId].data[2];
    gBattle_BG3_X += gTasks[taskId].data[10] >> 8;
    gBattle_BG3_Y += gTasks[taskId].data[11] >> 8;
    gTasks[taskId].data[10] &= 0xFF;
    gTasks[taskId].data[11] &= 0xFF;

    if (gBattleAnimArgs[7] == gTasks[taskId].data[3])
    {
        gBattle_BG3_X = 0;
        gBattle_BG3_Y = 0;
        UpdateAnimBg3ScreenSize(TRUE);
        DestroyTask(taskId);
    }
}

void AnimTask_GetAttackerSide(u8 taskId)
{
    gBattleAnimArgs[ARG_RET_ID] = GetBattlerSide(gBattleAnimAttacker);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_GetTargetSide(u8 taskId)
{
    gBattleAnimArgs[ARG_RET_ID] = GetBattlerSide(gBattleAnimTarget);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_GetTargetIsAttackerPartner(u8 taskId)
{
    gBattleAnimArgs[ARG_RET_ID] = BATTLE_PARTNER(gBattleAnimAttacker) == gBattleAnimTarget;
    DestroyAnimVisualTask(taskId);
}

// For hiding or subsequently revealing all other battlers
void AnimTask_SetAllNonAttackersInvisiblity(u8 taskId)
{
    u16 battler;

    for (battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        if (battler != gBattleAnimAttacker && IsBattlerSpriteVisible(battler))
            gSprites[gBattlerSpriteIds[battler]].invisible = gBattleAnimArgs[0];
    }

    DestroyAnimVisualTask(taskId);
}

void StartMonScrollingBgMask(u8 taskId, int UNUSED unused, u16 scrollSpeed, u8 battler, bool8 includePartner, u8 numFadeSteps, u8 fadeStepDelay, u8 duration, const u32 *gfx, const u32 *tilemap, const u32 *palette)
{
    u16 species;
    u8 spriteId, spriteId2;
    u16 bg1Cnt;
    struct BattleAnimBgData animBgData;
    u8 battler2;

    spriteId2 = 0;
    battler2 = BATTLE_PARTNER(battler);

    if (IsContest() || (includePartner && !IsBattlerSpriteVisible(battler2)))
        includePartner = FALSE;

    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR
                              | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ  | WINOUT_WIN01_CLR
                               | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
    ((vBgCnt *)&bg1Cnt)->priority = 0;
    ((vBgCnt *)&bg1Cnt)->screenSize = 0;
    ((vBgCnt *)&bg1Cnt)->areaOverflowMode = 1;
    if (!IsContest())
    {
        ((vBgCnt *)&bg1Cnt)->charBaseBlock = 1;
    }

    SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);

    if (IsContest())
    {
        species = gContestResources->moveAnim->species;
    }
    else
    {
        if (GetBattlerSide(battler) != B_SIDE_PLAYER)
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battler]], MON_DATA_SPECIES);
        else
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battler]], MON_DATA_SPECIES);
    }

    spriteId = CreateInvisibleSpriteCopy(battler, gBattlerSpriteIds[battler], species);
    if (includePartner)
        spriteId2 = CreateInvisibleSpriteCopy(battler2, gBattlerSpriteIds[battler2], species);

    GetBattleAnimBg1Data(&animBgData);
    AnimLoadCompressedBgTilemapHandleContest(&animBgData, tilemap, FALSE);
    AnimLoadCompressedBgGfx(animBgData.bgId, gfx, animBgData.tilesOffset);
    LoadCompressedPalette(palette, BG_PLTT_ID(animBgData.paletteId), PLTT_SIZE_4BPP);

    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gTasks[taskId].data[1] = scrollSpeed;
    gTasks[taskId].data[4] = numFadeSteps;
    gTasks[taskId].data[5] = duration;
    gTasks[taskId].data[6] = fadeStepDelay;
    gTasks[taskId].data[0] = spriteId;
    gTasks[taskId].data[2] = includePartner;
    gTasks[taskId].data[3] = spriteId2;
    gTasks[taskId].func = UpdateMonScrollingBgMask;
}

static void UpdateMonScrollingBgMask(u8 taskId)
{
    gTasks[taskId].data[13] += gTasks[taskId].data[1] < 0 ? -gTasks[taskId].data[1] : gTasks[taskId].data[1];
    if (gTasks[taskId].data[1] < 0)
        gBattle_BG1_Y -= gTasks[taskId].data[13] >> 8;
    else
        gBattle_BG1_Y += gTasks[taskId].data[13] >> 8;

    gTasks[taskId].data[13] &= 0xFF;
    switch (gTasks[taskId].data[15])
    {
    case 0:
        if (gTasks[taskId].data[11]++ >= gTasks[taskId].data[6])
        {
            gTasks[taskId].data[11] = 0;
            gTasks[taskId].data[12]++;
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].data[12], 16 - gTasks[taskId].data[12]));
            if (gTasks[taskId].data[12] == gTasks[taskId].data[4])
                gTasks[taskId].data[15]++;
        }
        break;
    case 1:
        if (++gTasks[taskId].data[10] == gTasks[taskId].data[5])
            gTasks[taskId].data[15]++;
        break;
    case 2:
        if (gTasks[taskId].data[11]++ >= gTasks[taskId].data[6])
        {
            gTasks[taskId].data[11] = 0;
            gTasks[taskId].data[12]--;
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].data[12], 16 - gTasks[taskId].data[12]));
            if (gTasks[taskId].data[12] == 0)
            {
                ResetBattleAnimBg(FALSE);
                gBattle_WIN0H = 0;
                gBattle_WIN0V = 0;
                SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR
                                          | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
                SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL  | WINOUT_WIN01_OBJ  | WINOUT_WIN01_CLR
                                           | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
                if (!IsContest())
                {
                    u16 bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
                    ((vBgCnt *)&bg1Cnt)->charBaseBlock = 0;
                    SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);
                }

                SetGpuReg(REG_OFFSET_DISPCNT, GetGpuReg(REG_OFFSET_DISPCNT) ^ DISPCNT_OBJWIN_ON);
                SetGpuReg(REG_OFFSET_BLDCNT, 0);
                SetGpuReg(REG_OFFSET_BLDALPHA, 0);
                DestroySprite(&gSprites[gTasks[taskId].data[0]]);
                if (gTasks[taskId].data[2])
                    DestroySprite(&gSprites[gTasks[taskId].data[3]]);

                DestroyAnimVisualTask(taskId);
            }
        }
        break;
    }
}

void AnimTask_GetBattleEnvironment(u8 taskId)
{
    gBattleAnimArgs[0] = gBattleEnvironment;
    DestroyAnimVisualTask(taskId);
}

void AnimTask_AllocBackupPalBuffer(u8 taskId)
{
    gMonSpritesGfxPtr->buffer = AllocZeroed(MON_PIC_SIZE * MAX_MON_PIC_FRAMES);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_FreeBackupPalBuffer(u8 taskId)
{
    FREE_AND_SET_NULL(gMonSpritesGfxPtr->buffer);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_CopyPalUnfadedToBackup(u8 taskId)
{
    u32 selectedPalettes;
    int paletteIndex = 0;

    if (gBattleAnimArgs[0] == 0)
    {
        selectedPalettes = GetBattlePalettesMask(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        while ((selectedPalettes & 1) == 0)
        {
            selectedPalettes >>= 1;
            paletteIndex++;
        }
    }
    else if (gBattleAnimArgs[0] == 1)
    {
        paletteIndex = gBattleAnimAttacker + 16;
    }
    else if (gBattleAnimArgs[0] == 2)
    {
        paletteIndex = gBattleAnimTarget + 16;
    }

    memcpy(&gMonSpritesGfxPtr->buffer[gBattleAnimArgs[1] * 16], &gPlttBufferUnfaded[PLTT_ID(paletteIndex)], PLTT_SIZE_4BPP);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_CopyPalUnfadedFromBackup(u8 taskId)
{
    u32 selectedPalettes;
    int paletteIndex = 0;

    if (gBattleAnimArgs[0] == 0)
    {
        selectedPalettes = GetBattlePalettesMask(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        while ((selectedPalettes & 1) == 0)
        {
            selectedPalettes >>= 1;
            paletteIndex++;
        }
    }
    else if (gBattleAnimArgs[0] == 1)
    {
        paletteIndex = gBattleAnimAttacker + 16;
    }
    else if (gBattleAnimArgs[0] == 2)
    {
        paletteIndex = gBattleAnimTarget + 16;
    }

    memcpy(&gPlttBufferUnfaded[PLTT_ID(paletteIndex)], &gMonSpritesGfxPtr->buffer[gBattleAnimArgs[1] * 16], PLTT_SIZE_4BPP);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_CopyPalFadedToUnfaded(u8 taskId)
{
    u32 selectedPalettes;
    int paletteIndex = 0;

    if (gBattleAnimArgs[0] == 0)
    {
        selectedPalettes = GetBattlePalettesMask(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        while ((selectedPalettes & 1) == 0)
        {
            selectedPalettes >>= 1;
            paletteIndex++;
        }
    }
    else if (gBattleAnimArgs[0] == 1)
    {
        paletteIndex = gBattleAnimAttacker + 16;
    }
    else if (gBattleAnimArgs[0] == 2)
    {
        paletteIndex = gBattleAnimTarget + 16;
    }

    memcpy(&gPlttBufferUnfaded[PLTT_ID(paletteIndex)], &gPlttBufferFaded[PLTT_ID(paletteIndex)], PLTT_SIZE_4BPP);
    DestroyAnimVisualTask(taskId);
}

void AnimTask_IsContest(u8 taskId)
{
    if (IsContest())
        gBattleAnimArgs[ARG_RET_ID] = TRUE;
    else
        gBattleAnimArgs[ARG_RET_ID] = FALSE;

    DestroyAnimVisualTask(taskId);
}

void AnimTask_SetAnimAttackerAndTargetForEffectTgt(u8 taskId)
{
    gBattleAnimAttacker = gBattlerTarget;
    gBattleAnimTarget = gEffectBattler;
    DestroyAnimVisualTask(taskId);
}

void AnimTask_IsTargetSameSide(u8 taskId)
{
    if (GetBattlerSide(gBattleAnimAttacker) == GetBattlerSide(gBattleAnimTarget))
        gBattleAnimArgs[ARG_RET_ID] = TRUE;
    else
        gBattleAnimArgs[ARG_RET_ID] = FALSE;

    DestroyAnimVisualTask(taskId);
}

void AnimTask_SetAnimTargetToBattlerTarget(u8 taskId)
{
    gBattleAnimTarget = gBattlerTarget;
    DestroyAnimVisualTask(taskId);
}

void AnimTask_SetAnimAttackerAndTargetForEffectAtk(u8 taskId)
{
    gBattleAnimAttacker = gBattlerAttacker;
    gBattleAnimTarget = gEffectBattler;
    DestroyAnimVisualTask(taskId);
}

void AnimTask_SetAttackerInvisibleWaitForSignal(u8 taskId)
{
    if (IsContest())
    {
        DestroyAnimVisualTask(taskId);
    }
    else
    {
        gTasks[taskId].data[0] = gBattleSpritesDataPtr->battlerData[gBattleAnimAttacker].invisible;
        gBattleSpritesDataPtr->battlerData[gBattleAnimAttacker].invisible = TRUE;
        gTasks[taskId].func = AnimTask_WaitAndRestoreVisibility;
        gAnimVisualTaskCount--;
    }
}

static void AnimTask_WaitAndRestoreVisibility(u8 taskId)
{
    if (gBattleAnimArgs[7] == 0x1000)
    {
        gBattleSpritesDataPtr->battlerData[gBattleAnimAttacker].invisible = (u8)gTasks[taskId].data[0] & 1;
        DestroyTask(taskId);
    }
}
