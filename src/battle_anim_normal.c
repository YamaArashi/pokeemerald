#include "global.h"
#include "battle_anim.h"
#include "battle_anim_internal.h"
#include "palette.h"
#include "random.h"
#include "task.h"
#include "trig.h"
#include "constants/rgb.h"

static void AnimConfusionDuck(struct Sprite *);
static void AnimSimplePaletteBlend(struct Sprite *);
static void AnimSimplePaletteBlend_Step(struct Sprite *);
static void AnimComplexPaletteBlend(struct Sprite *);
static void AnimComplexPaletteBlend_Step1(struct Sprite *);
static void AnimComplexPaletteBlend_Step2(struct Sprite *);
static void AnimCirclingSparkle(struct Sprite *);
static void AnimShakeMonOrBattleTerrain(struct Sprite *);
static void AnimShakeMonOrBattleTerrain_Step(struct Sprite *);
static void AnimShakeMonOrBattleTerrain_UpdateCoordOffsetEnabled(void);
static void AnimHitSplatBasic(struct Sprite *);
static void AnimHitSplatPersistent(struct Sprite *);
static void AnimHitSplatHandleInvert(struct Sprite *);
static void AnimHitSplatRandom(struct Sprite *);
static void AnimHitSplatOnMonEdge(struct Sprite *);
static void AnimCrossImpact(struct Sprite *);
static void AnimFlashingHitSplat(struct Sprite *);
static void AnimFlashingHitSplat_Step(struct Sprite *);
static void AnimConfusionDuck_Step(struct Sprite *);
static void BlendColorCycle(u8, u8, u8);
static void AnimTask_BlendColorCycleLoop(u8);
static void BlendColorCycleExclude(u8, u8, u8);
static void AnimTask_BlendColorCycleExcludeLoop(u8);
static void BlendColorCycleByTag(u8, u8, u8);
static void AnimTask_BlendColorCycleByTagLoop(u8);
static void AnimTask_FlashAnimTagWithColor_Step1(u8);
static void AnimTask_FlashAnimTagWithColor_Step2(u8);
static void AnimTask_ShakeBattleTerrain_Step(u8);

static const union AnimCmd sAnim_ConfusionDuck_0[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_FRAME(0, 8, .hFlip = TRUE),
    ANIMCMD_FRAME(8, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_ConfusionDuck_1[] =
{
    ANIMCMD_FRAME(0, 8, .hFlip = TRUE),
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(8, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_ConfusionDuck[] =
{
    sAnim_ConfusionDuck_0,
    sAnim_ConfusionDuck_1,
};

const struct SpriteTemplate gConfusionDuckSpriteTemplate =
{
    .tileTag = ANIM_TAG_DUCK,
    .paletteTag = ANIM_TAG_DUCK,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = sAnims_ConfusionDuck,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimConfusionDuck,
};

const struct SpriteTemplate gSimplePaletteBlendSpriteTemplate =
{
    .tileTag = 0,
    .paletteTag = 0,
    .oam = &gDummyOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimSimplePaletteBlend,
};

const struct SpriteTemplate gComplexPaletteBlendSpriteTemplate =
{
    .tileTag = 0,
    .paletteTag = 0,
    .oam = &gDummyOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimComplexPaletteBlend,
};

static const union AnimCmd sAnim_CirclingSparkle[] =
{
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_FRAME(16, 3),
    ANIMCMD_FRAME(32, 3),
    ANIMCMD_FRAME(48, 3),
    ANIMCMD_FRAME(64, 3),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_CirclingSparkle[] =
{
    sAnim_CirclingSparkle,
};

// Unused
static const struct SpriteTemplate sCirclingSparkleSpriteTemplate =
{
    .tileTag = ANIM_TAG_SPARKLE_4,
    .paletteTag = ANIM_TAG_SPARKLE_4,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_CirclingSparkle,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimCirclingSparkle,
};

const struct SpriteTemplate gShakeMonOrTerrainSpriteTemplate =
{
    .tileTag = 0,
    .paletteTag = 0,
    .oam = &gDummyOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimShakeMonOrBattleTerrain,
};

static const union AffineAnimCmd sAffineAnim_HitSplat_0[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_HitSplat_1[] =
{
    AFFINEANIMCMD_FRAME(0xD8, 0xD8, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_HitSplat_2[] =
{
    AFFINEANIMCMD_FRAME(0xB0, 0xB0, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_HitSplat_3[] =
{
    AFFINEANIMCMD_FRAME(0x80, 0x80, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_HitSplat[] =
{
    sAffineAnim_HitSplat_0,
    sAffineAnim_HitSplat_1,
    sAffineAnim_HitSplat_2,
    sAffineAnim_HitSplat_3,
};

const struct SpriteTemplate gBasicHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatBasic,
};

const struct SpriteTemplate gHandleInvertHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatHandleInvert,
};

const struct SpriteTemplate gWaterHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_WATER_IMPACT,
    .paletteTag = ANIM_TAG_WATER_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatBasic,
};

const struct SpriteTemplate gRandomPosHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatRandom,
};

const struct SpriteTemplate gMonEdgeHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatOnMonEdge,
};

const struct SpriteTemplate gCrossImpactSpriteTemplate =
{
    .tileTag = ANIM_TAG_CROSS_IMPACT,
    .paletteTag = ANIM_TAG_CROSS_IMPACT,
    .oam = &gOamData_AffineOff_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimCrossImpact,
};

const struct SpriteTemplate gFlashingHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjNormal_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimFlashingHitSplat,
};

const struct SpriteTemplate gPersistHitSplatSpriteTemplate =
{
    .tileTag = ANIM_TAG_IMPACT,
    .paletteTag = ANIM_TAG_IMPACT,
    .oam = &gOamData_AffineNormal_ObjBlend_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_HitSplat,
    .callback = AnimHitSplatPersistent,
};

// Moves a spinning duck around the mon's head.
static void AnimConfusionDuck(struct Sprite *sprite)
{
    CMD_ARGS(x, y, waveOffset, wavePeriod, duration);

    sprite->x += cmd->x;
    sprite->y += cmd->y;
    sprite->data[0] = cmd->waveOffset;
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->data[1] = -cmd->wavePeriod;
        sprite->data[4] = 1;
    }
    else
    {
        sprite->data[1] = cmd->wavePeriod;
        sprite->data[4] = 0;
        StartSpriteAnim(sprite, 1);
    }

    sprite->data[3] = cmd->duration;
    sprite->callback = AnimConfusionDuck_Step;
    sprite->callback(sprite);
}

static void AnimConfusionDuck_Step(struct Sprite *sprite)
{
    sprite->x2 = Cos(sprite->data[0], 30);
    sprite->y2 = Sin(sprite->data[0], 10);

    if ((u16)sprite->data[0] < 128)
        sprite->oam.priority = 1;
    else
        sprite->oam.priority = 3;

    sprite->data[0] = (sprite->data[0] + sprite->data[1]) & 0xFF;
    if (++sprite->data[2] == sprite->data[3])
        DestroyAnimSprite(sprite);
}

// Performs a simple color blend on a specified sprite.
static void AnimSimplePaletteBlend(struct Sprite *sprite)
{
    CMD_ARGS(selector, delay, initialBlendY, targetBlendY, color);

    u32 selectedPalettes = UnpackSelectedBattlePalettes(cmd->selector);
    BeginNormalPaletteFade(selectedPalettes, cmd->delay, cmd->initialBlendY, cmd->targetBlendY, cmd->color);
    sprite->invisible = TRUE;
    sprite->callback = AnimSimplePaletteBlend_Step;
}

// Unpacks a bitfield and returns a bitmask of its selected palettes.
// Bits 0-6 of the selector parameter result in the following palettes being selected:
//   0: F_PAL_BG, battle background palettes (BG palettes 1, 2, and 3)
//   1: F_PAL_ATTACKER, gBattleAnimAttacker OBJ palette
//   2: F_PAL_TARGET, gBattleAnimTarget OBJ palette
//   3: F_PAL_ATK_PARTNER, gBattleAnimAttacker partner OBJ palette
//   4: F_PAL_DEF_PARTNER, gBattleAnimTarget partner OBJ palette
//   5: F_PAL_ANIM_1, BG palette 8 (or 14, if in Contest)
//   6: F_PAL_ANIM_2, BG palette 9
u32 UnpackSelectedBattlePalettes(s16 selector)
{
    bool8 battleBackground = selector & 1;
    bool8 attacker = (selector >> 1) & 1;
    bool8 target = (selector >> 2) & 1;
    bool8 attackerPartner = (selector >> 3) & 1;
    bool8 targetPartner = (selector >> 4) & 1;
    bool8 anim1 = (selector >> 5) & 1;
    bool8 anim2 = (selector >> 6) & 1;
    return GetBattlePalettesMask(battleBackground, attacker, target, attackerPartner, targetPartner, anim1, anim2);
}

static void AnimSimplePaletteBlend_Step(struct Sprite *sprite)
{
    if (!gPaletteFade.active)
        DestroyAnimSprite(sprite);
}

#define sTimer           data[0]
#define sDelay           data[1]
#define sNumBlends       data[2]
#define sColor1          data[3]
#define sBlendY1         data[4]
#define sColor2          data[5]
#define sBlendY2         data[6]
#define sPaletteSelector data[7]

static void AnimComplexPaletteBlend(struct Sprite *sprite)
{
    CMD_ARGS(selector, delay, numBlends, color1, blendY1, color2, blendY2);

    u32 selectedPalettes;

    sprite->sTimer = cmd->delay;
    sprite->sDelay = cmd->delay;
    sprite->sNumBlends = cmd->numBlends;
    sprite->sColor1 = cmd->color1;
    sprite->sBlendY1 = cmd->blendY1;
    sprite->sColor2 = cmd->color2;
    sprite->sBlendY2 = cmd->blendY2;
    sprite->sPaletteSelector = cmd->selector;

    selectedPalettes = UnpackSelectedBattlePalettes(sprite->sPaletteSelector);
    BlendPalettes(selectedPalettes, cmd->blendY1, cmd->color1);
    sprite->invisible = TRUE;
    sprite->callback = AnimComplexPaletteBlend_Step1;
}

static void AnimComplexPaletteBlend_Step1(struct Sprite *sprite)
{
    u32 selectedPalettes;

    if (sprite->sTimer > 0)
    {
        sprite->sTimer--;
        return;
    }

    if (gPaletteFade.active)
        return;

    if (sprite->sNumBlends == 0)
    {
        sprite->callback = AnimComplexPaletteBlend_Step2;
        return;
    }

    selectedPalettes = UnpackSelectedBattlePalettes(sprite->sPaletteSelector);
    if (sprite->sDelay & 0x100)
        BlendPalettes(selectedPalettes, sprite->sBlendY1, sprite->sColor1);
    else
        BlendPalettes(selectedPalettes, sprite->sBlendY2, sprite->sColor2);

    sprite->sDelay ^= 0x100;
    sprite->sTimer = sprite->sDelay & 0xFF;
    sprite->sNumBlends--;
}

static void AnimComplexPaletteBlend_Step2(struct Sprite *sprite)
{
    u32 selectedPalettes;

    if (!gPaletteFade.active)
    {
        selectedPalettes = UnpackSelectedBattlePalettes(sprite->sPaletteSelector);
        BlendPalettes(selectedPalettes, 0, 0);
        DestroyAnimSprite(sprite);
    }
}

#undef sTimer
#undef sDelay
#undef sNumBlends
#undef sColor1
#undef sBlendY1
#undef sColor2
#undef sBlendY2
#undef sPaletteSelector

static void AnimCirclingSparkle(struct Sprite *sprite)
{
    CMD_ARGS(x, y);

    sprite->x += cmd->x;
    sprite->y += cmd->y;
    sprite->data[0] = 0;
    sprite->data[1] = 10;
    sprite->data[2] = 8;
    sprite->data[3] = 40;
    sprite->data[4] = 112;
    sprite->data[5] = 0;
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = TranslateSpriteInGrowingCircle;
    sprite->callback(sprite);
}

// Task data for AnimTask_BlendColorCycle, AnimTask_BlendColorCycleExclude, and AnimTask_BlendColorCycleByTag
#define tPalSelector   data[0]  // AnimTask_BlendColorCycle
#define tPalTag        data[0]  // AnimTask_BlendColorCycleByTag
#define tDelay         data[1]
#define tNumBlends     data[2]
#define tInitialBlendY data[3]
#define tTargetBlendY  data[4]
#define tBlendColor    data[5]
#define tRestoreBlend  data[8]
#define tPalSelectorHi data[9]
#define tPalSelectorLo data[10]

// Blends mon/screen to designated color or back alternately tNumBlends times
// Many uses of this task only set a tNumBlends of 2, which has the effect of blending to a color and back once
void AnimTask_BlendColorCycle(u8 taskId)
{
    CMD_ARGS(selector, delay, numBlends, initialBlendY, targetBlendY, color);

    gTasks[taskId].tPalSelector = cmd->selector;
    gTasks[taskId].tDelay = cmd->delay;
    gTasks[taskId].tNumBlends = cmd->numBlends;
    gTasks[taskId].tInitialBlendY = cmd->initialBlendY;
    gTasks[taskId].tTargetBlendY = cmd->targetBlendY;
    gTasks[taskId].tBlendColor = cmd->color;
    gTasks[taskId].tRestoreBlend = FALSE;
    BlendColorCycle(taskId, 0, gTasks[taskId].tTargetBlendY);
    gTasks[taskId].func = AnimTask_BlendColorCycleLoop;
}

static void BlendColorCycle(u8 taskId, u8 startBlendAmount, u8 targetBlendAmount)
{
    u32 selectedPalettes = UnpackSelectedBattlePalettes(gTasks[taskId].tPalSelector);
    BeginNormalPaletteFade(
        selectedPalettes,
        gTasks[taskId].tDelay,
        startBlendAmount,
        targetBlendAmount,
        gTasks[taskId].tBlendColor);

    gTasks[taskId].tNumBlends--;
    gTasks[taskId].tRestoreBlend ^= 1;
}

static void AnimTask_BlendColorCycleLoop(u8 taskId)
{
    u8 startBlendAmount, targetBlendAmount;
    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tNumBlends > 0)
        {
            if (!gTasks[taskId].tRestoreBlend)
            {
                // Blend to designated color
                startBlendAmount = gTasks[taskId].tInitialBlendY;
                targetBlendAmount = gTasks[taskId].tTargetBlendY;
            }
            else
            {
                // Blend back to original color
                startBlendAmount = gTasks[taskId].tTargetBlendY;
                targetBlendAmount = gTasks[taskId].tInitialBlendY;
            }

            if (gTasks[taskId].tNumBlends == 1)
                targetBlendAmount = 0;

            BlendColorCycle(taskId, startBlendAmount, targetBlendAmount);
        }
        else
        {
            DestroyAnimVisualTask(taskId);
        }
    }
}

// See AnimTask_BlendColorCycle. Same, but excludes Attacker and Target
void AnimTask_BlendColorCycleExclude(u8 taskId)
{
    CMD_ARGS(unk0, delay, numBlends, initialBlendY, targetBlendY, color);

    int battler;
    u32 selectedPalettes = 0;

    gTasks[taskId].data[0] = cmd->unk0;
    gTasks[taskId].tDelay = cmd->delay;
    gTasks[taskId].tNumBlends = cmd->numBlends;
    gTasks[taskId].tInitialBlendY = cmd->initialBlendY;
    gTasks[taskId].tTargetBlendY = cmd->targetBlendY;
    gTasks[taskId].tBlendColor = cmd->color;
    gTasks[taskId].tRestoreBlend = 0;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (battler != gBattleAnimAttacker && battler != gBattleAnimTarget)
            selectedPalettes |= 1 << (battler + 16);
    }

    if (cmd->unk0 == 1)
        selectedPalettes |= 0xE;

    gTasks[taskId].tPalSelectorHi = selectedPalettes >> 16;
    gTasks[taskId].tPalSelectorLo = selectedPalettes & 0xFF;
    BlendColorCycleExclude(taskId, 0, gTasks[taskId].tTargetBlendY);
    gTasks[taskId].func = AnimTask_BlendColorCycleExcludeLoop;
}

static void BlendColorCycleExclude(u8 taskId, u8 startBlendAmount, u8 targetBlendAmount)
{
    u32 selectedPalettes = ((u16)gTasks[taskId].tPalSelectorHi << 16) | (u16)gTasks[taskId].tPalSelectorLo;
    BeginNormalPaletteFade(
        selectedPalettes,
        gTasks[taskId].tDelay,
        startBlendAmount,
        targetBlendAmount,
        gTasks[taskId].tBlendColor);

    gTasks[taskId].tNumBlends--;
    gTasks[taskId].tRestoreBlend ^= 1;
}

static void AnimTask_BlendColorCycleExcludeLoop(u8 taskId)
{
    u8 startBlendAmount, targetBlendAmount;
    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tNumBlends > 0)
        {
            if (!gTasks[taskId].tRestoreBlend)
            {
                // Blend to designated color
                startBlendAmount = gTasks[taskId].tInitialBlendY;
                targetBlendAmount = gTasks[taskId].tTargetBlendY;
            }
            else
            {
                // Blend back to original color
                startBlendAmount = gTasks[taskId].tTargetBlendY;
                targetBlendAmount = gTasks[taskId].tInitialBlendY;
            }

            if (gTasks[taskId].tNumBlends == 1)
                targetBlendAmount = 0;

            BlendColorCycleExclude(taskId, startBlendAmount, targetBlendAmount);
        }
        else
        {
            DestroyAnimVisualTask(taskId);
        }
    }
}

// See AnimTask_BlendColorCycle. Same, but selects palette by ANIM_TAG_*
void AnimTask_BlendColorCycleByTag(u8 taskId)
{
    CMD_ARGS(tag, delay, numBlends, initialBlendY, targetBlendY, color);

    gTasks[taskId].tPalTag = cmd->tag;
    gTasks[taskId].tDelay = cmd->delay;
    gTasks[taskId].tNumBlends = cmd->numBlends;
    gTasks[taskId].tInitialBlendY = cmd->initialBlendY;
    gTasks[taskId].tTargetBlendY = cmd->targetBlendY;
    gTasks[taskId].tBlendColor = cmd->color;
    gTasks[taskId].tRestoreBlend = FALSE;

    BlendColorCycleByTag(taskId, 0, gTasks[taskId].tTargetBlendY);
    gTasks[taskId].func = AnimTask_BlendColorCycleByTagLoop;
}

static void BlendColorCycleByTag(u8 taskId, u8 startBlendAmount, u8 targetBlendAmount)
{
    u8 paletteIndex = IndexOfSpritePaletteTag(gTasks[taskId].tPalTag);
    BeginNormalPaletteFade(
        1 << (paletteIndex + 16),
        gTasks[taskId].tDelay,
        startBlendAmount,
        targetBlendAmount,
        gTasks[taskId].tBlendColor);

    gTasks[taskId].tNumBlends--;
    gTasks[taskId].tRestoreBlend ^= 1;
}

static void AnimTask_BlendColorCycleByTagLoop(u8 taskId)
{
    u8 startBlendAmount, targetBlendAmount;
    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tNumBlends > 0)
        {
            if (!gTasks[taskId].tRestoreBlend)
            {
                // Blend to designated color
                startBlendAmount = gTasks[taskId].tInitialBlendY;
                targetBlendAmount = gTasks[taskId].tTargetBlendY;
            }
            else
            {
                // Blend back to original color
                startBlendAmount = gTasks[taskId].tTargetBlendY;
                targetBlendAmount = gTasks[taskId].tInitialBlendY;
            }

            if (gTasks[taskId].tNumBlends == 1)
                targetBlendAmount = 0;

            BlendColorCycleByTag(taskId, startBlendAmount, targetBlendAmount);
        }
        else
        {
            DestroyAnimVisualTask(taskId);
        }
    }
}

#undef tPalSelector
#undef tPalTag
#undef tDelay
#undef tNumBlends
#undef tInitialBlendY
#undef tTargetBlendY
#undef tBlendColor
#undef tRestoreBlend
#undef tPalSelectorHi
#undef tPalSelectorLo

// Flashes the specified anim tag with given color. Used e.g. to flash the particles red in Hyper Beam
#define tTimer     data[0]
#define tDelay     data[1]
#define tNumBlends data[2]
#define tColor1    data[3]
#define tBlendY1   data[4]
#define tColor2    data[5]
#define tBlendY2   data[6]
#define tAnimTag   data[7]
void AnimTask_FlashAnimTagWithColor(u8 taskId)
{
    CMD_ARGS(tag, delay, numBlends, color1, blendY1, color2, blendY2);

    u8 paletteIndex;

    gTasks[taskId].tTimer = cmd->delay;
    gTasks[taskId].tDelay = cmd->delay;
    gTasks[taskId].tNumBlends = cmd->numBlends;
    gTasks[taskId].tColor1 = cmd->color1;
    gTasks[taskId].tBlendY1 = cmd->blendY1;
    gTasks[taskId].tColor2 = cmd->color2;
    gTasks[taskId].tBlendY2 = cmd->blendY2;
    gTasks[taskId].tAnimTag = cmd->tag;

    paletteIndex = IndexOfSpritePaletteTag(cmd->tag);
    BeginNormalPaletteFade(
        1 << (paletteIndex + 16),
        0,
        cmd->blendY1,
        cmd->blendY1,
        cmd->color1);

    gTasks[taskId].func = AnimTask_FlashAnimTagWithColor_Step1;
}

static void AnimTask_FlashAnimTagWithColor_Step1(u8 taskId)
{
    u32 selectedPalettes;

    if (gTasks[taskId].tTimer > 0)
    {
        gTasks[taskId].tTimer--;
        return;
    }

    if (gPaletteFade.active)
        return;

    if (gTasks[taskId].tNumBlends == 0)
    {
        gTasks[taskId].func = AnimTask_FlashAnimTagWithColor_Step2;
        return;
    }

    selectedPalettes = 1 << (IndexOfSpritePaletteTag(gTasks[taskId].tAnimTag) + 16);
    if (gTasks[taskId].tDelay & 0x100)
    {
        BeginNormalPaletteFade(
            selectedPalettes,
            0,
            gTasks[taskId].tBlendY1,
            gTasks[taskId].tBlendY1,
            gTasks[taskId].tColor1);
    }
    else
    {
        BeginNormalPaletteFade(
            selectedPalettes,
            0,
            gTasks[taskId].tBlendY2,
            gTasks[taskId].tBlendY2,
            gTasks[taskId].tColor2);
    }

    gTasks[taskId].tDelay ^= 0x100;
    gTasks[taskId].tTimer = gTasks[taskId].tDelay & 0xFF;
    gTasks[taskId].tNumBlends--;
}

static void AnimTask_FlashAnimTagWithColor_Step2(u8 taskId)
{
    u32 selectedPalettes;

    if (!gPaletteFade.active)
    {
        selectedPalettes = 1 << (IndexOfSpritePaletteTag(gTasks[taskId].tAnimTag) + 16);
        BeginNormalPaletteFade(selectedPalettes, 0, 0, 0, RGB_BLACK);
        DestroyAnimVisualTask(taskId);
    }
}

#undef tTimer
#undef tDelay
#undef tNumBlends
#undef tColor1
#undef tBlendY1
#undef tColor2
#undef tBlendY2
#undef tAnimTag

void AnimTask_InvertScreenColor(u8 taskId)
{
    CMD_ARGS(flagsScenery, flagsAttacker, flagsTarget);

    u32 selectedPalettes = 0;
    u8 attackerBattler = gBattleAnimAttacker;
    u8 targetBattler = gBattleAnimTarget;

    if (cmd->flagsScenery & (1 << 8))
        selectedPalettes = GetBattlePalettesMask(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);

    if (cmd->flagsAttacker & (1 << 8))
        selectedPalettes |= (0x10000 << attackerBattler);

    if (cmd->flagsTarget & (1 << 8))
        selectedPalettes |= (0x10000 << targetBattler);

    InvertPlttBuffer(selectedPalettes);
    DestroyAnimVisualTask(taskId);
}

// Unused
#define tTimer         data[0]
#define tLength        data[1]
#define tFlagsScenery  data[2]
#define tFlagsAttacker data[3]
#define tFlagsTarget   data[4]
#define tColorR        data[5]
#define tColorG        data[6]
#define tColorB        data[7]
void AnimTask_TintPalettes(u8 taskId)
{
    CMD_ARGS(flagsScenery, flagsAttacker, flagsTarget, duration, r, g, b);

    u8 attackerBattler;
    u8 targetBattler;
    u8 paletteIndex;
    u32 selectedPalettes = 0;

    if (gTasks[taskId].tTimer == 0)
    {
        gTasks[taskId].tFlagsScenery = cmd->flagsScenery;
        gTasks[taskId].tFlagsAttacker = cmd->flagsAttacker;
        gTasks[taskId].tFlagsTarget = cmd->flagsTarget;
        gTasks[taskId].tLength = cmd->duration;
        gTasks[taskId].tColorR = cmd->r;
        gTasks[taskId].tColorG = cmd->g;
        gTasks[taskId].tColorB = cmd->b;
    }

    gTasks[taskId].tTimer++;
    attackerBattler = gBattleAnimAttacker;
    targetBattler = gBattleAnimTarget;

    if (gTasks[taskId].tFlagsScenery & (1 << 8))
        selectedPalettes = PALETTES_BG;

    if (gTasks[taskId].tFlagsScenery & 1)
    {
        paletteIndex = IndexOfSpritePaletteTag(gSprites[gHealthboxSpriteIds[attackerBattler]].template->paletteTag);
        selectedPalettes |= (1 << paletteIndex) << 16;
    }

    if (gTasks[taskId].tFlagsAttacker & (1 << 8))
        selectedPalettes |= (1 << attackerBattler) << 16;

    if (gTasks[taskId].tFlagsTarget & (1 << 8))
        selectedPalettes |= (1 << targetBattler) << 16;

    TintPlttBuffer(selectedPalettes, gTasks[taskId].tColorR, gTasks[taskId].tColorG, gTasks[taskId].tColorB);
    if (gTasks[taskId].tTimer == gTasks[taskId].tLength)
    {
        UnfadePlttBuffer(selectedPalettes);
        DestroyAnimVisualTask(taskId);
    }
}
#undef tTimer
#undef tLength
#undef tFlagsScenery
#undef tFlagsAttacker
#undef tFlagsTarget
#undef tColorR
#undef tColorG
#undef tColorB

#define sShakeVelocity data[0]
#define sShakeTimer    data[1]
#define sShakeDuration data[2]
#define sTimer         data[3]
#define sOriginalValue data[4]
#define sType          data[5]
#define sShakePtrLo    data[6]
#define sShakePtrHi    data[7]

static void AnimShakeMonOrBattleTerrain(struct Sprite *sprite)
{
    CMD_ARGS(velocity, shakeDuration, duration, type, battlerSelector);

    sprite->invisible = TRUE;
    sprite->sShakeVelocity = -cmd->velocity;
    sprite->sShakeTimer = cmd->shakeDuration;
    sprite->sShakeDuration = cmd->shakeDuration;
    sprite->sTimer = cmd->duration;

    switch (cmd->type)
    {
    case SHAKE_BG_X:
        StoreSpriteCallbackInData6(sprite, (void *)&gBattle_BG3_X);
        break;
    case SHAKE_BG_Y:
        StoreSpriteCallbackInData6(sprite, (void *)&gBattle_BG3_Y);
        break;
    case SHAKE_MON_X:
        StoreSpriteCallbackInData6(sprite, (void *)&gSpriteCoordOffsetX);
        break;
    default:
        StoreSpriteCallbackInData6(sprite, (void *)&gSpriteCoordOffsetY);
        break;
    }

    sprite->sOriginalValue = *(u16 *)(sprite->sShakePtrLo | (sprite->sShakePtrHi << 16));
    sprite->sType = cmd->type;
    if (sprite->sType == SHAKE_MON_X || sprite->sType == SHAKE_MON_Y)
        AnimShakeMonOrBattleTerrain_UpdateCoordOffsetEnabled();

    sprite->callback = AnimShakeMonOrBattleTerrain_Step;
}

static void AnimShakeMonOrBattleTerrain_Step(struct Sprite *sprite)
{
    u8 i;

    if (sprite->sTimer > 0)
    {
        sprite->sTimer--;
        if (sprite->sShakeTimer > 0)
        {
            sprite->sShakeTimer--;
        }
        else
        {
            sprite->sShakeTimer = sprite->sShakeDuration;
            *(u16 *)(sprite->sShakePtrLo | (sprite->sShakePtrHi << 16)) += sprite->sShakeVelocity;
            sprite->sShakeVelocity = -sprite->sShakeVelocity;
        }
    }
    else
    {
        *(u16 *)(sprite->sShakePtrLo | (sprite->sShakePtrHi << 16)) = sprite->sOriginalValue;
        if (sprite->sType == SHAKE_MON_X || sprite->sType == SHAKE_MON_Y)
        {
            for (i = 0; i < gBattlersCount; i++)
                gSprites[gBattlerSpriteIds[i]].coordOffsetEnabled = FALSE;
        }

        DestroyAnimSprite(sprite);
    }
}

static void AnimShakeMonOrBattleTerrain_UpdateCoordOffsetEnabled(void)
{
    // Matches AnimShakeMonOrBattleTerrain.
    CMD_ARGS(velocity, shakeDuration, duration, type, battlerSelector);

    gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].coordOffsetEnabled = FALSE;
    gSprites[gBattlerSpriteIds[gBattleAnimTarget]].coordOffsetEnabled = FALSE;

    if (cmd->battlerSelector == 2)
    {
        gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].coordOffsetEnabled = TRUE;
        gSprites[gBattlerSpriteIds[gBattleAnimTarget]].coordOffsetEnabled = TRUE;
    }
    else
    {
        if (cmd->battlerSelector == 0)
            gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].coordOffsetEnabled = TRUE;
        else
            gSprites[gBattlerSpriteIds[gBattleAnimTarget]].coordOffsetEnabled = TRUE;
    }
}

#undef sShakeVelocity
#undef sShakeTimer
#undef sShakeDuration
#undef sTimer
#undef sOriginalValue
#undef sType
#undef sShakePtrLo
#undef sShakePtrHi

// Task data for AnimTask_ShakeBattleTerrain
#define tXOffset     data[0]
#define tYOffset     data[1]
#define tNumShakes   data[2]
#define tTimer       data[3]
#define tShakeDelay  data[8]

// Can shake battle terrain back and forth on the X or down and back to original pos on Y (cant shake up from orig pos)
void AnimTask_ShakeBattleTerrain(u8 taskId)
{
    CMD_ARGS(xOffset, yOffset, shakes, delay);

    gTasks[taskId].tXOffset = cmd->xOffset;
    gTasks[taskId].tYOffset = cmd->yOffset;
    gTasks[taskId].tNumShakes = cmd->shakes;
    gTasks[taskId].tTimer = cmd->delay;
    gTasks[taskId].tShakeDelay = cmd->delay;
    gBattle_BG3_X = cmd->xOffset;
    gBattle_BG3_Y = cmd->yOffset;
    gTasks[taskId].func = AnimTask_ShakeBattleTerrain_Step;
    gTasks[taskId].func(taskId);
}

static void AnimTask_ShakeBattleTerrain_Step(u8 taskId)
{
    if (gTasks[taskId].tTimer == 0)
    {
        if (gBattle_BG3_X == gTasks[taskId].tXOffset)
            gBattle_BG3_X = -gTasks[taskId].tXOffset;
        else
            gBattle_BG3_X = gTasks[taskId].tXOffset;

        if (gBattle_BG3_Y == -gTasks[taskId].tYOffset)
            gBattle_BG3_Y = 0;
        else
            gBattle_BG3_Y = -gTasks[taskId].tYOffset;

        gTasks[taskId].tTimer = gTasks[taskId].tShakeDelay;
        if (--gTasks[taskId].tNumShakes == 0)
        {
            gBattle_BG3_X = 0;
            gBattle_BG3_Y = 0;
            DestroyAnimVisualTask(taskId);
        }
    }
    else
    {
        gTasks[taskId].tTimer--;
    }
}

#undef tXOffset
#undef tYOffset
#undef tNumShakes
#undef tTimer
#undef tShakeDelay

static void AnimHitSplatBasic(struct Sprite *sprite)
{
    CMD_ARGS(x, y, relativeTo, animation);

    StartSpriteAffineAnim(sprite, cmd->animation);
    if (cmd->relativeTo == ANIM_ATTACKER)
        InitSpritePosToAnimAttacker(sprite, TRUE);
    else
        InitSpritePosToAnimTarget(sprite, TRUE);

    sprite->callback = RunStoredCallbackWhenAffineAnimEnds;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

// Same as basic hit splat but takes a length of time to persist for.
static void AnimHitSplatPersistent(struct Sprite *sprite)
{
    CMD_ARGS(x, y, relativeTo, animation, duration);

    StartSpriteAffineAnim(sprite, cmd->animation);
    if (cmd->relativeTo == ANIM_ATTACKER)
        InitSpritePosToAnimAttacker(sprite, TRUE);
    else
        InitSpritePosToAnimTarget(sprite, TRUE);

    sprite->data[0] = cmd->duration;
    sprite->callback = RunStoredCallbackWhenAffineAnimEnds;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSpriteAfterTimer);
}

// For paired hit splats whose position is inverted when used by the opponent on the player.
// Used by Twineedle and Spike Cannon
static void AnimHitSplatHandleInvert(struct Sprite *sprite)
{
    // Matches AnimHitSplatBasic.
    CMD_ARGS(x, y, relativeTo, animation);

    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER && !IsContest())
        cmd->y = -cmd->y;

    AnimHitSplatBasic(sprite);
}

static void AnimHitSplatRandom(struct Sprite *sprite)
{
    CMD_ARGS(relativeTo, animation);

    if (cmd->animation == -1)
        cmd->animation = Random2() & 3;

    StartSpriteAffineAnim(sprite, cmd->animation);
    if (cmd->relativeTo == ANIM_ATTACKER)
        InitSpritePosToAnimAttacker(sprite, FALSE);
    else
        InitSpritePosToAnimTarget(sprite, FALSE);

    sprite->x2 += (Random2() % 48) - 24;
    sprite->y2 += (Random2() % 24) - 12;

    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = RunStoredCallbackWhenAffineAnimEnds;
}

static void AnimHitSplatOnMonEdge(struct Sprite *sprite)
{
    CMD_ARGS(relativeTo, x, y, animation);

    sprite->data[0] = GetAnimBattlerSpriteId(cmd->relativeTo);
    sprite->x = gSprites[sprite->data[0]].x + gSprites[sprite->data[0]].x2;
    sprite->y = gSprites[sprite->data[0]].y + gSprites[sprite->data[0]].y2;
    sprite->x2 = cmd->x;
    sprite->y2 = cmd->y;
    StartSpriteAffineAnim(sprite, cmd->animation);
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = RunStoredCallbackWhenAffineAnimEnds;
}

static void AnimCrossImpact(struct Sprite *sprite)
{
    CMD_ARGS(x, y, relativeTo, duration);

    if (cmd->relativeTo == ANIM_ATTACKER)
        InitSpritePosToAnimAttacker(sprite, TRUE);
    else
        InitSpritePosToAnimTarget(sprite, TRUE);

    sprite->data[0] = cmd->duration;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
    sprite->callback = WaitAnimForDuration;
}

static void AnimFlashingHitSplat(struct Sprite *sprite)
{
    CMD_ARGS(x, y, relativeTo, animation);

    StartSpriteAffineAnim(sprite, cmd->animation);
    if (cmd->relativeTo == ANIM_ATTACKER)
        InitSpritePosToAnimAttacker(sprite, TRUE);
    else
        InitSpritePosToAnimTarget(sprite, TRUE);

    sprite->callback = AnimFlashingHitSplat_Step;
}

static void AnimFlashingHitSplat_Step(struct Sprite *sprite)
{
    sprite->invisible ^= 1;
    if (sprite->data[0]++ > 12)
        DestroyAnimSprite(sprite);
}
