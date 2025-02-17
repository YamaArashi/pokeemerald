#include "global.h"
#include "battle_anim.h"
#include "battle_anim_internal.h"
#include "scanline_effect.h"
#include "task.h"
#include "trig.h"
#include "constants/rgb.h"

static void AnimOutrageFlame(struct Sprite *);
static void AnimDragonRageFirePlume(struct Sprite *);
static void AnimDragonFireToTarget(struct Sprite *);
static void AnimDragonDanceOrb(struct Sprite *);
static void AnimDragonDanceOrb_Step(struct Sprite *);
static void AnimOverheatFlame(struct Sprite *);
static void AnimOverheatFlame_Step(struct Sprite *);
static void AnimTask_DragonDanceWaver_Step(u8);
static void UpdateDragonDanceScanlineEffect(struct Task *);

EWRAM_DATA static u16 sUnusedOverheatData[7] = {0};

static const union AnimCmd sAnim_OutrageOverheatFire_0[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_OutrageOverheatFire[] =
{
    sAnim_OutrageOverheatFire_0,
};

const struct SpriteTemplate gOutrageFlameSpriteTemplate =
{
    .tileTag = ANIM_TAG_SMALL_EMBER,
    .paletteTag = ANIM_TAG_SMALL_EMBER,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_OutrageOverheatFire,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimOutrageFlame,
};

static const union AnimCmd sAnim_DragonBreathFire_0[] =
{
    ANIMCMD_FRAME(16, 3),
    ANIMCMD_FRAME(32, 3),
    ANIMCMD_FRAME(48, 3),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_DragonBreathFire_1[] =
{
    ANIMCMD_FRAME(16, 3, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_FRAME(32, 3, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_FRAME(48, 3, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_DragonBreathFire[] =
{
    sAnim_DragonBreathFire_0,
    sAnim_DragonBreathFire_1,
};

static const union AffineAnimCmd sAffineAnim_DragonBreathFire_0[] =
{
    AFFINEANIMCMD_FRAME(0x50, 0x50, 127, 0),
    AFFINEANIMCMD_FRAME(0xD, 0xD, 0, 100),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_DragonBreathFire_1[] =
{
    AFFINEANIMCMD_FRAME(0x50, 0x50, 0, 0),
    AFFINEANIMCMD_FRAME(0xD, 0xD, 0, 100),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_DragonBreathFire[] =
{
    sAffineAnim_DragonBreathFire_0,
    sAffineAnim_DragonBreathFire_1,
};

const struct SpriteTemplate gDragonBreathFireSpriteTemplate =
{
    .tileTag = ANIM_TAG_SMALL_EMBER,
    .paletteTag = ANIM_TAG_SMALL_EMBER,
    .oam = &gOamData_AffineDouble_ObjNormal_32x32,
    .anims = sAnims_DragonBreathFire,
    .images = NULL,
    .affineAnims = sAffineAnims_DragonBreathFire,
    .callback = AnimDragonFireToTarget,
};

static const union AnimCmd sAnim_DragonRageFirePlume[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_FRAME(64, 5),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_DragonRageFirePlume[] =
{
    sAnim_DragonRageFirePlume,
};

const struct SpriteTemplate gDragonRageFirePlumeSpriteTemplate =
{
    .tileTag = ANIM_TAG_FIRE_PLUME,
    .paletteTag = ANIM_TAG_FIRE_PLUME,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_DragonRageFirePlume,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimDragonRageFirePlume,
};

static const union AnimCmd sAnim_DragonRageFire[] =
{
    ANIMCMD_FRAME(16, 3),
    ANIMCMD_FRAME(32, 3),
    ANIMCMD_FRAME(48, 3),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_DragonRageFire[] =
{
    sAnim_DragonRageFire,
    sAnim_DragonRageFire,
};

static const union AffineAnimCmd sAffineAnim_DragonRageFire_0[] =
{
    AFFINEANIMCMD_FRAME(0x64, 0x64, 127, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_DragonRageFire_1[] =
{
    AFFINEANIMCMD_FRAME(0x64, 0x64, 0, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_DragonRageFire[] =
{
    sAffineAnim_DragonRageFire_0,
    sAffineAnim_DragonRageFire_1,
};

const struct SpriteTemplate gDragonRageFireSpitSpriteTemplate =
{
    .tileTag = ANIM_TAG_SMALL_EMBER,
    .paletteTag = ANIM_TAG_SMALL_EMBER,
    .oam = &gOamData_AffineDouble_ObjNormal_32x32,
    .anims = sAnims_DragonRageFire,
    .images = NULL,
    .affineAnims = sAffineAnims_DragonRageFire,
    .callback = AnimDragonFireToTarget,
};

const struct SpriteTemplate gDragonDanceOrbSpriteTemplate =
{
    .tileTag = ANIM_TAG_HOLLOW_ORB,
    .paletteTag = ANIM_TAG_HOLLOW_ORB,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimDragonDanceOrb,
};

const struct SpriteTemplate gOverheatFlameSpriteTemplate =
{
    .tileTag = ANIM_TAG_SMALL_EMBER,
    .paletteTag = ANIM_TAG_SMALL_EMBER,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_OutrageOverheatFire,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimOverheatFlame,
};

static void AnimOutrageFlame(struct Sprite *sprite)
{
    CMD_ARGS(x, y, duration, xVelocity, yVelocity, flickerDuration);

    sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->x -= cmd->x;
        cmd->xVelocity = -cmd->xVelocity;
        cmd->yVelocity = -cmd->yVelocity;
    }
    else
    {
        sprite->x += cmd->x;
    }

    sprite->y += cmd->y;
    sprite->data[0] = cmd->duration;
    sprite->data[1] = cmd->xVelocity;
    sprite->data[3] = cmd->yVelocity;
    sprite->data[5] = cmd->flickerDuration;
    sprite->invisible = TRUE;
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = TranslateSpriteLinearAndFlicker;
}

static void StartDragonFireTranslation(struct Sprite *sprite)
{
    CMD_ARGS(initialX, initialY, targetX, targetY, duration);

    SetSpriteCoordsToAnimAttackerCoords(sprite);
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->x -= cmd->initialY;
        sprite->y += cmd->initialY;
        sprite->data[2] -= cmd->targetX;
        sprite->data[4] += cmd->targetY;
    }
    else
    {
        sprite->x += cmd->initialX;
        sprite->y += cmd->initialY;
        sprite->data[2] += cmd->targetX;
        sprite->data[4] += cmd->targetY;
        StartSpriteAnim(sprite, 1);
    }

    sprite->data[0] = cmd->duration;
    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
}

static void AnimDragonRageFirePlume(struct Sprite *sprite)
{
    CMD_ARGS(relativeTo, x, y);

    if (cmd->relativeTo == ANIM_ATTACKER)
    {
        sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X);
        sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y);
    }
    else
    {
        sprite->x = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X);
        sprite->y = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y);
    }

    SetAnimSpriteInitialXOffset(sprite, cmd->x);
    sprite->y += cmd->y;
    sprite->callback = RunStoredCallbackWhenAnimEnds;
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
}

// For Dragon Breath and Dragon Rage
static void AnimDragonFireToTarget(struct Sprite *sprite)
{
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        StartSpriteAffineAnim(sprite, 1);

    StartDragonFireTranslation(sprite);
}

static void AnimDragonDanceOrb(struct Sprite *sprite)
{
    CMD_ARGS(angle);

    u16 r5;
    u16 r0;
    sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    sprite->data[4] = 0;
    sprite->data[5] = 1;
    sprite->data[6] = cmd->angle;
    r5 = GetBattlerSpriteCoordAttr(gBattlerAttacker, BATTLER_COORD_ATTR_HEIGHT);
    r0 = GetBattlerSpriteCoordAttr(gBattlerAttacker, BATTLER_COORD_ATTR_WIDTH);
    if (r5 > r0)
        sprite->data[7] = r5 / 2;
    else
        sprite->data[7] = r0 / 2;
    sprite->x2 = Cos(sprite->data[6], sprite->data[7]);
    sprite->y2 = Sin(sprite->data[6], sprite->data[7]);
    sprite->callback = AnimDragonDanceOrb_Step;
}

static void AnimDragonDanceOrb_Step(struct Sprite *sprite)
{
    switch (sprite->data[0])
    {
    case 0:
        sprite->data[6] = (sprite->data[6] - sprite->data[5]) & 0xFF;
        sprite->x2 = Cos(sprite->data[6], sprite->data[7]);
        sprite->y2 = Sin(sprite->data[6], sprite->data[7]);
        if (++sprite->data[4] > 5)
        {
            sprite->data[4] = 0;
            if (sprite->data[5] <= 15 && ++sprite->data[5] > 15)
                sprite->data[5] = 16;
        }
        if (++sprite->data[3] > 0x3C)
        {
            sprite->data[3] = 0;
            sprite->data[0]++;
        }
        break;
    case 1:
        sprite->data[6] = (sprite->data[6] - sprite->data[5]) & 0xFF;
        if (sprite->data[7] <= 0x95 && (sprite->data[7] += 8) > 0x95)
            sprite->data[7] = 0x96;
        sprite->x2 = Cos(sprite->data[6], sprite->data[7]);
        sprite->y2 = Sin(sprite->data[6], sprite->data[7]);
        if (++sprite->data[4] > 5)
        {
            sprite->data[4] = 0;
            if (sprite->data[5] <= 15 && ++sprite->data[5] > 15)
                sprite->data[5] = 16;
        }
        if (++sprite->data[3] > 20)
            DestroyAnimSprite(sprite);
        break;
    }
}

// Wavers the attacker back and forth. Progressing vertical wave of scanline shifts
// Used by Dragon Dance
void AnimTask_DragonDanceWaver(u8 taskId)
{
    struct ScanlineEffectParams scanlineParams;
    struct Task *task = &gTasks[taskId];
    u16 i;
    u8 y;
    if (GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) == 1)
    {
        scanlineParams.dmaDest = &REG_BG1HOFS;
        task->data[2] = gBattle_BG1_X;
    }
    else
    {
        scanlineParams.dmaDest = &REG_BG2HOFS;
        task->data[2] = gBattle_BG2_X;
    }

    scanlineParams.dmaControl = SCANLINE_EFFECT_DMACNT_16BIT;
    scanlineParams.initState = 1;
    scanlineParams.unused9 = 0;
    y = GetBattlerYCoordWithElevation(gBattleAnimAttacker);
    task->data[3] = y - 32;
    task->data[4] = y + 32;
    if (task->data[3] < 0)
        task->data[3] = 0;

    for (i = task->data[3]; i <= task->data[4]; i++)
    {
        gScanlineEffectRegBuffers[0][i] = task->data[2];
        gScanlineEffectRegBuffers[1][i] = task->data[2];
    }

    ScanlineEffect_SetParams(scanlineParams);
    task->func = AnimTask_DragonDanceWaver_Step;
}

static void AnimTask_DragonDanceWaver_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    switch (task->data[0])
    {
    case 0:
        if (++task->data[7] > 1)
        {
            task->data[7] = 0;
            if (++task->data[6] == 3)
                task->data[0]++;
        }
        UpdateDragonDanceScanlineEffect(task);
        break;
    case 1:
        if (++task->data[1] > 0x3C)
            task->data[0]++;
        UpdateDragonDanceScanlineEffect(task);
        break;
    case 2:
        if (++task->data[7] > 1)
        {
            task->data[7] = 0;
            if (--task->data[6] == 0)
                task->data[0]++;
        }
        UpdateDragonDanceScanlineEffect(task);
        break;
    case 3:
        gScanlineEffect.state = 3;
        task->data[0]++;
        break;
    case 4:
        DestroyAnimVisualTask(taskId);
        break;
    }
}

static void UpdateDragonDanceScanlineEffect(struct Task *task)
{
    u16 sineIndex = task->data[5];
    u16 i;
    for (i = task->data[3]; i <= task->data[4]; i++)
    {
        gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][i] = ((gSineTable[sineIndex] * task->data[6]) >> 7) + task->data[2];
        sineIndex = (sineIndex + 8) & 0xFF;
    }

    task->data[5] = (task->data[5] + 9) & 0xFF;
}

static void AnimOverheatFlame(struct Sprite *sprite)
{
    CMD_ARGS(speed, unk1, unk2, duration, y);

    int i;
    int yAmplitude = (cmd->unk2 * 3) / 5;
    sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET) + cmd->y;
    sprite->data[1] = Cos(cmd->unk1, cmd->unk2);
    sprite->data[2] = Sin(cmd->unk1, yAmplitude);
    sprite->x += sprite->data[1] * cmd->speed;
    sprite->y += sprite->data[2] * cmd->speed;
    sprite->data[3] = cmd->duration;
    sprite->callback = AnimOverheatFlame_Step;
    for (i = 0; i < 7; i++)
        sUnusedOverheatData[i] = sprite->data[i];
}

static void AnimOverheatFlame_Step(struct Sprite *sprite)
{
    sprite->data[4] += sprite->data[1];
    sprite->data[5] += sprite->data[2];
    sprite->x2 = sprite->data[4] / 10;
    sprite->y2 = sprite->data[5] / 10;
    if (++sprite->data[0] > sprite->data[3])
        DestroyAnimSprite(sprite);
}
