#include "global.h"
#include "battle_anim.h"
#include "battle_anim_internal.h"
#include "contest.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "palette.h"
#include "scanline_effect.h"
#include "trig.h"
#include "util.h"
#include "constants/rgb.h"

static void AnimUnusedBagSteal(struct Sprite *);
static void AnimUnusedBagSteal_Step(struct Sprite *);
static void AnimBite(struct Sprite *);
static void AnimTearDrop(struct Sprite *);
static void AnimClawSlash(struct Sprite *);
static void AnimTask_AttackerFadeToInvisible_Step(u8);
static void AnimTask_AttackerFadeFromInvisible_Step(u8);
static void AnimBite_Step1(struct Sprite *);
static void AnimBite_Step2(struct Sprite *);
static void AnimTearDrop_Step(struct Sprite *);
static void AnimTask_MoveAttackerMementoShadow_Step(u8);
static void AnimTask_MoveTargetMementoShadow_Step(u8);
static void DoMementoShadowEffect(struct Task *);
static void SetAllBattlersSpritePriority(u8);
static void AnimTask_MetallicShine_Step(u8);

// Unused
static const struct SpriteTemplate sUnusedBagStealSpriteTemplate =
{
    .tileTag = ANIM_TAG_TIED_BAG,
    .paletteTag = ANIM_TAG_TIED_BAG,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimUnusedBagSteal,
};

static const union AffineAnimCmd sAffineAnim_Bite_0[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_1[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 32, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_2[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 64, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_3[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 96, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_4[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -128, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_5[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -96, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_6[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -64, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_Bite_7[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -32, 1),
    AFFINEANIMCMD_END,
};

const union AffineAnimCmd *const gAffineAnims_Bite[] =
{
    sAffineAnim_Bite_0,
    sAffineAnim_Bite_1,
    sAffineAnim_Bite_2,
    sAffineAnim_Bite_3,
    sAffineAnim_Bite_4,
    sAffineAnim_Bite_5,
    sAffineAnim_Bite_6,
    sAffineAnim_Bite_7,
};

const struct SpriteTemplate gSharpTeethSpriteTemplate =
{
    .tileTag = ANIM_TAG_SHARP_TEETH,
    .paletteTag = ANIM_TAG_SHARP_TEETH,
    .oam = &gOamData_AffineNormal_ObjBlend_64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gAffineAnims_Bite,
    .callback = AnimBite,
};

const struct SpriteTemplate gClampJawSpriteTemplate =
{
    .tileTag = ANIM_TAG_CLAMP,
    .paletteTag = ANIM_TAG_CLAMP,
    .oam = &gOamData_AffineNormal_ObjBlend_64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gAffineAnims_Bite,
    .callback = AnimBite,
};

static const union AffineAnimCmd sAffineAnim_TearDrop_0[] =
{
    AFFINEANIMCMD_FRAME(0xC0, 0xC0, 80, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -2, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_TearDrop_1[] =
{
    AFFINEANIMCMD_FRAME(0xC0, 0xC0, -80, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 2, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_TearDrop[] =
{
    sAffineAnim_TearDrop_0,
    sAffineAnim_TearDrop_1,
};

const struct SpriteTemplate gTearDropSpriteTemplate =
{
    .tileTag = ANIM_TAG_SMALL_BUBBLES,
    .paletteTag = ANIM_TAG_SMALL_BUBBLES,
    .oam = &gOamData_AffineNormal_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_TearDrop,
    .callback = AnimTearDrop,
};

static const union AnimCmd sAnim_ClawSlash_0[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_ClawSlash_1[] =
{
    ANIMCMD_FRAME(0, 4, .hFlip = TRUE),
    ANIMCMD_FRAME(16, 4, .hFlip = TRUE),
    ANIMCMD_FRAME(32, 4, .hFlip = TRUE),
    ANIMCMD_FRAME(48, 4, .hFlip = TRUE),
    ANIMCMD_FRAME(64, 4, .hFlip = TRUE),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_ClawSlash[] =
{
    sAnim_ClawSlash_0,
    sAnim_ClawSlash_1,
};

const struct SpriteTemplate gClawSlashSpriteTemplate =
{
    .tileTag = ANIM_TAG_CLAW_SLASH,
    .paletteTag = ANIM_TAG_CLAW_SLASH,
    .oam = &gOamData_AffineOff_ObjNormal_32x32,
    .anims = sAnims_ClawSlash,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimClawSlash,
};

void AnimTask_AttackerFadeToInvisible(u8 taskId)
{
    CMD_ARGS(stepDelay);

    int battler;
    gTasks[taskId].data[0] = cmd->stepDelay;
    battler = gBattleAnimAttacker;
    gTasks[taskId].data[1] = 16;
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 0));
    if (GetBattlerSpriteBGPriorityRank(battler) == 1)
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG1);
    else
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);

    gTasks[taskId].func = AnimTask_AttackerFadeToInvisible_Step;
}

static void AnimTask_AttackerFadeToInvisible_Step(u8 taskId)
{
    u8 blendA = gTasks[taskId].data[1] >> 8;
    u8 blendB = gTasks[taskId].data[1];
    if (gTasks[taskId].data[2] == (u8)gTasks[taskId].data[0])
    {
        blendA++;
        blendB--;
        gTasks[taskId].data[1] = BLDALPHA_BLEND(blendB, blendA);
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].data[1]);
        gTasks[taskId].data[2] = 0;
        if (blendA == 16)
        {
            gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].invisible = TRUE;
            DestroyAnimVisualTask(taskId);
        }
    }
    else
    {
        gTasks[taskId].data[2]++;
    }
}

void AnimTask_AttackerFadeFromInvisible(u8 taskId)
{
    CMD_ARGS(stepDelay);

    gTasks[taskId].data[0] = cmd->stepDelay;
    gTasks[taskId].data[1] = BLDALPHA_BLEND(0, 16);
    gTasks[taskId].func = AnimTask_AttackerFadeFromInvisible_Step;
    SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].data[1]);
}

static void AnimTask_AttackerFadeFromInvisible_Step(u8 taskId)
{
    u8 blendA = gTasks[taskId].data[1] >> 8;
    u8 blendB = gTasks[taskId].data[1];
    if (gTasks[taskId].data[2] == (u8)gTasks[taskId].data[0])
    {
        blendA--;
        blendB++;
        gTasks[taskId].data[1] = (blendA << 8) | blendB;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].data[1]);
        gTasks[taskId].data[2] = 0;
        if (blendA == 0)
        {
            SetGpuReg(REG_OFFSET_BLDCNT, 0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
            DestroyAnimVisualTask(taskId);
        }
    }
    else
    {
        gTasks[taskId].data[2]++;
    }
}

void AnimTask_InitAttackerFadeFromInvisible(u8 taskId)
{
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    if (GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) == 1)
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG1);
    else
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);

    DestroyAnimVisualTask(taskId);
}

static void AnimUnusedBagSteal(struct Sprite *sprite)
{
    sprite->data[1] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->data[3] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    sprite->data[0] = 0x7E;
    InitSpriteDataForLinearTranslation(sprite);
    sprite->data[3] = -sprite->data[1];
    sprite->data[4] = -sprite->data[2];
    sprite->data[6] = 0xFFD8;
    sprite->callback = AnimUnusedBagSteal_Step;
    sprite->callback(sprite);
}

static void AnimUnusedBagSteal_Step(struct Sprite *sprite)
{
    sprite->data[3] += sprite->data[1];
    sprite->data[4] += sprite->data[2];
    sprite->x2 = sprite->data[3] >> 8;
    sprite->y2 = sprite->data[4] >> 8;
    if (sprite->data[7] == 0)
    {
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        sprite->x2 = sprite->data[3] >> 8;
        sprite->y2 = sprite->data[4] >> 8;
        sprite->data[0]--;
    }

    sprite->y2 += Sin(sprite->data[5], sprite->data[6]);
    sprite->data[5] = (sprite->data[5] + 3) & 0xFF;
    if (sprite->data[5] > 0x7F)
    {
        sprite->data[5] = 0;
        sprite->data[6] += 20;
        sprite->data[7]++;
    }

    if (--sprite->data[0] == 0)
        DestroyAnimSprite(sprite);
}

// Move sprite inward for Bite/Crunch and Clamp
static void AnimBite(struct Sprite *sprite)
{
    CMD_ARGS(x, y, animation, xVelocity, yVelocity, halfDuration);

    sprite->x += cmd->x;
    sprite->y += cmd->y;
    StartSpriteAffineAnim(sprite, cmd->animation);
    sprite->data[0] = cmd->xVelocity;
    sprite->data[1] = cmd->yVelocity;
    sprite->data[2] = cmd->halfDuration;
    sprite->callback = AnimBite_Step1;
}

static void AnimBite_Step1(struct Sprite *sprite)
{
    sprite->data[4] += sprite->data[0];
    sprite->data[5] += sprite->data[1];
    sprite->x2 = sprite->data[4] >> 8;
    sprite->y2 = sprite->data[5] >> 8;
    if (++sprite->data[3] == sprite->data[2])
        sprite->callback = AnimBite_Step2;
}

static void AnimBite_Step2(struct Sprite *sprite)
{
    sprite->data[4] -= sprite->data[0];
    sprite->data[5] -= sprite->data[1];
    sprite->x2 = sprite->data[4] >> 8;
    sprite->y2 = sprite->data[5] >> 8;
    if (--sprite->data[3] == 0)
        DestroySpriteAndMatrix(sprite);
}

// Launches a tear drop away from the battler. Used by Fake Tears
static void AnimTearDrop(struct Sprite *sprite)
{
    CMD_ARGS(relativeTo, type);

    u8 battler;
    s8 xOffset;

    if (cmd->relativeTo == ANIM_ATTACKER)
        battler = gBattleAnimAttacker;
    else
        battler = gBattleAnimTarget;

    xOffset = 20;
    sprite->oam.tileNum += 4;

    switch (cmd->type)
    {
    case 0:
        sprite->x = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_RIGHT) - 8;
        sprite->y = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_TOP) + 8;
        break;
    case 1:
        sprite->x = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_RIGHT) - 14;
        sprite->y = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_TOP) + 16;
        break;
    case 2:
        sprite->x = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_LEFT) + 8;
        sprite->y = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_TOP) + 8;
        StartSpriteAffineAnim(sprite, 1);
        xOffset = -20;
        break;
    case 3:
        sprite->x = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_LEFT) + 14;
        sprite->y = GetBattlerSpriteCoordAttr(battler, BATTLER_COORD_ATTR_TOP) + 16;
        StartSpriteAffineAnim(sprite, 1);
        xOffset = -20;
        break;
    }

    sprite->data[0] = 32;
    sprite->data[2] = sprite->x + xOffset;
    sprite->data[4] = sprite->y + 12;
    sprite->data[5] = -12;

    InitAnimArcTranslation(sprite);
    sprite->callback = AnimTearDrop_Step;
}

static void AnimTearDrop_Step(struct Sprite *sprite)
{
    if (TranslateAnimHorizontalArc(sprite))
        DestroySpriteAndMatrix(sprite);
}

void AnimTask_MoveAttackerMementoShadow(u8 taskId)
{
    struct ScanlineEffectParams scanlineParams;
    struct BattleAnimBgData animBg;
    u16 i;
    u8 pos;
    int var0;
    struct Task *task = &gTasks[taskId];

    task->data[7] = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y) + 31;
    task->data[6] = GetBattlerSpriteCoordAttr(gBattleAnimAttacker, BATTLER_COORD_ATTR_TOP) - 7;
    task->data[5] = task->data[7];
    task->data[4] = task->data[6];
    task->data[13] = (task->data[7] - task->data[6]) << 8;

    pos = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X);
    task->data[14] = pos - 32;
    task->data[15] = pos + 32;

    if (GetBattlerSide(gBattleAnimAttacker) == B_SIDE_PLAYER)
        task->data[8] = -12;
    else
        task->data[8] = -64;

    task->data[3] = GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker);
    if (task->data[3] == 1)
    {
        GetBattleAnimBg1Data(&animBg);
        task->data[10] = gBattle_BG1_Y;
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG1);
        FillPalette(RGB_BLACK, BG_PLTT_ID(animBg.paletteId), PLTT_SIZE_4BPP);
        scanlineParams.dmaDest = &REG_BG1VOFS;
        var0 = WINOUT_WIN01_BG1;
        if (!IsContest())
            gBattle_BG2_X += DISPLAY_WIDTH;
    }
    else
    {
        task->data[10] = gBattle_BG2_Y;
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);
        FillPalette(RGB_BLACK, BG_PLTT_ID(9), PLTT_SIZE_4BPP);
        scanlineParams.dmaDest = &REG_BG2VOFS;
        var0 = WINOUT_WIN01_BG2;
        if (!IsContest())
            gBattle_BG1_X += DISPLAY_WIDTH;
    }

    scanlineParams.dmaControl = SCANLINE_EFFECT_DMACNT_16BIT;
    scanlineParams.initState = 1;
    scanlineParams.unused9 = 0;
    task->data[11] = 0;
    task->data[12] = 16;
    task->data[0] = 0;
    task->data[1] = 0;
    task->data[2] = 0;
    SetAllBattlersSpritePriority(3);
    for (i = 0; i < 112; i++)
    {
        gScanlineEffectRegBuffers[0][i] = task->data[10];
        gScanlineEffectRegBuffers[1][i] = task->data[10];
    }

    ScanlineEffect_SetParams(scanlineParams);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | (var0 ^ (WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR)));
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
    gBattle_WIN0H = (task->data[14] << 8) | task->data[15];
    gBattle_WIN0V = DISPLAY_HEIGHT;

    task->func = AnimTask_MoveAttackerMementoShadow_Step;
}

static void AnimTask_MoveAttackerMementoShadow_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        if (++task->data[1] > 1)
        {
            task->data[1] = 0;
            if (++task->data[2] & 1)
            {
                if (task->data[11] != 12)
                    task->data[11]++;
            }
            else
            {
                if (task->data[12] != 8)
                    task->data[12]--;
            }

            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(task->data[11], task->data[12]));

            if (task->data[11] == 12 && task->data[12] == 8)
                task->data[0]++;
        }
        break;
    case 1:
        task->data[4] -= 8;
        DoMementoShadowEffect(task);

        if (task->data[4] < task->data[8])
            task->data[0]++;
        break;
    case 2:
        task->data[4] -= 8;
        DoMementoShadowEffect(task);
        task->data[14] += 4;
        task->data[15] -= 4;

        if (task->data[14] >= task->data[15])
            task->data[14] = task->data[15];

        gBattle_WIN0H = (task->data[14] << 8) | task->data[15];

        if (task->data[14] == task->data[15])
            task->data[0]++;
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

void AnimTask_MoveTargetMementoShadow(u8 taskId)
{
    struct BattleAnimBgData animBg;
    struct ScanlineEffectParams scanlineParams;
    u8 x;
    u16 i;
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        if (IsContest() == TRUE)
        {
            gBattle_WIN0H = 0;
            gBattle_WIN0V = 0;
            SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
            DestroyAnimVisualTask(taskId);
        }
        else
        {
            task->data[3] = GetBattlerSpriteBGPriorityRank(gBattleAnimTarget);
            if (task->data[3] == 1)
            {
                SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG1);
                gBattle_BG2_X += DISPLAY_WIDTH;
            }
            else
            {
                SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);
                gBattle_BG1_X += DISPLAY_WIDTH;
            }

            task->data[0]++;
        }
        break;
    case 1:
        if (task->data[3] == 1)
        {
            GetBattleAnimBg1Data(&animBg);
            task->data[10] = gBattle_BG1_Y;
            FillPalette(RGB_BLACK, BG_PLTT_ID(animBg.paletteId), PLTT_SIZE_4BPP);
        }
        else
        {
            task->data[10] = gBattle_BG2_Y;
            FillPalette(RGB_BLACK, BG_PLTT_ID(9), PLTT_SIZE_4BPP);
        }

        SetAllBattlersSpritePriority(3);
        task->data[0]++;
        break;
    case 2:
        task->data[7] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y) + 31;
        task->data[6] = GetBattlerSpriteCoordAttr(gBattleAnimTarget, BATTLER_COORD_ATTR_TOP) - 7;
        task->data[13] = (task->data[7] - task->data[6]) << 8;
        x = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X);
        task->data[14] = x - 4;
        task->data[15] = x + 4;

        if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
            task->data[8] = -12;
        else
            task->data[8] = -64;

        task->data[4] = task->data[8];
        task->data[5] = task->data[8];
        task->data[11] = 12;
        task->data[12] = 8;
        task->data[0]++;
        break;
    case 3:
        if (task->data[3] == 1)
            scanlineParams.dmaDest = &REG_BG1VOFS;
        else
            scanlineParams.dmaDest = &REG_BG2VOFS;

        for (i = 0; i < 112; i++)
        {
            gScanlineEffectRegBuffers[0][i] = task->data[10] + (159 - i);
            gScanlineEffectRegBuffers[1][i] = task->data[10] + (159 - i);
        }

        scanlineParams.dmaControl = SCANLINE_EFFECT_DMACNT_16BIT;
        scanlineParams.initState = 1;
        scanlineParams.unused9 = 0;
        ScanlineEffect_SetParams(scanlineParams);
        task->data[0]++;
        break;
    case 4:
        if (task->data[3] == 1)
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG0 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        else
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);

        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
        gBattle_WIN0H = (task->data[14] << 8) | task->data[15];
        gBattle_WIN0V = DISPLAY_HEIGHT;

        task->data[0] = 0;
        task->data[1] = 0;
        task->data[2] = 0;
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12, 8));
        task->func = AnimTask_MoveTargetMementoShadow_Step;
        break;
    }
}

static void AnimTask_MoveTargetMementoShadow_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        task->data[5] += 8;
        if (task->data[5] >=  task->data[7])
            task->data[5] = task->data[7];

        DoMementoShadowEffect(task);
        if (task->data[5] == task->data[7])
            task->data[0]++;
        break;
    case 1:
        if (task->data[15] - task->data[14] < 0x40)
        {
            task->data[14] -= 4;
            task->data[15] += 4;
        }
        else
        {
            task->data[1] = 1;
        }

        gBattle_WIN0H = (task->data[14] << 8) | task->data[15];
        task->data[4] += 8;
        if (task->data[4] >= task->data[6])
            task->data[4] = task->data[6];

        DoMementoShadowEffect(task);
        if (task->data[4] == task->data[6] && task->data[1])
        {
            task->data[1] = 0;
            task->data[0]++;
        }
        break;
    case 2:
        if (++task->data[1] > 1)
        {
            task->data[1] = 0;
            task->data[2]++;
            if (task->data[2] & 1)
            {
                if (task->data[11])
                    task->data[11]--;
            }
            else
            {
                if (task->data[12] < 16)
                    task->data[12]++;
            }

            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(task->data[11], task->data[12]));
            if (task->data[11] == 0 && task->data[12] == 16)
                task->data[0]++;
        }
        break;
    case 3:
        gScanlineEffect.state = 3;
        task->data[0]++;
        break;
    case 4:
        gBattle_WIN0H = 0;
        gBattle_WIN0V = 0;
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        DestroyAnimVisualTask(taskId);
        break;
    }
}

static void DoMementoShadowEffect(struct Task *task)
{
    int var0, var1;
    s16 var2;
    s16 i;
    int var4;

    var2 = task->data[5] - task->data[4];
    if (var2 != 0)
    {
        var0 = task->data[13] / var2;
        var1 = task->data[6] << 8;

        for (i = 0; i < task->data[4]; i++)
        {
            gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][i] = task->data[10] - (i - 159);
        }

        for (i = task->data[4]; i <= task->data[5]; i++)
        {
            if (i >= 0)
            {
                s16 var3 = (var1 >> 8) - i;
                gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][i] = var3 + task->data[10];
            }

            var1 += var0;
        }

        var4 = task->data[10] - (i - 159);
        for (i = i; i < task->data[7]; i++)
        {
            if (i >= 0)
            {
                gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][i] = var4;
                var4--;
            }
        }
    }
    else
    {
        var4 = task->data[10] + 159;
        for (i = 0; i < 112; i++)
        {
            gScanlineEffectRegBuffers[0][i] = var4;
            gScanlineEffectRegBuffers[1][i] = var4;
            var4--;
        }
    }
}

static void SetAllBattlersSpritePriority(u8 priority)
{
    u16 i;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        u8 spriteId = GetAnimBattlerSpriteId(i);
        if (spriteId != SPRITE_NONE)
            gSprites[spriteId].oam.priority = priority;
    }
}

void AnimTask_InitMementoShadow(u8 taskId)
{
    u8 toBG2 = GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) ^ 1 ? 1 : 0;
    MoveBattlerSpriteToBG(gBattleAnimAttacker, toBG2, TRUE);
    gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].invisible = FALSE;

    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
    {
        MoveBattlerSpriteToBG(BATTLE_PARTNER(gBattleAnimAttacker), toBG2 ^ 1, TRUE);
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].invisible = FALSE;
    }

    DestroyAnimVisualTask(taskId);
}

void AnimTask_MementoHandleBg(u8 taskId)
{
    bool8 toBG2 = GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) ^ 1 ? TRUE : FALSE;
    ResetBattleAnimBg(toBG2);

    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
        ResetBattleAnimBg(toBG2 ^ 1);

    DestroyAnimVisualTask(taskId);
}

// Animates a deep slash from a claw. Used by Metal Claw, Dragon Claw, and Crush Claw
static void AnimClawSlash(struct Sprite *sprite)
{
    CMD_ARGS(x, y, animation);

    sprite->x += cmd->x;
    sprite->y += cmd->y;
    StartSpriteAnim(sprite, cmd->animation);
    sprite->callback = RunStoredCallbackWhenAnimEnds;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

// Makes the attacker metallic and shining.
// Used by MOVE_HARDEN and MOVE_IRON_DEFENSE.
// Custom color argument is used in MOVE_POISON_TAIL to make the mon turn purplish/pinkish as if became cloaked in poison.
void AnimTask_MetallicShine(u8 taskId)
{
    CMD_ARGS(permanent, useColor, color);

    u16 species;
    u8 spriteId;
    u8 newSpriteId;
    u16 paletteNum;
    struct BattleAnimBgData animBg;
    bool32 priorityChanged = FALSE;

    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG0 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG1);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 12));
    SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 0);
    SetAnimBgAttribute(1, BG_ANIM_SCREEN_SIZE, 0);
    if (!IsContest())
        SetAnimBgAttribute(1, BG_ANIM_CHAR_BASE_BLOCK, 1);

    if (IsDoubleBattle() && !IsContest())
    {
        if (GetBattlerPosition(gBattleAnimAttacker) == B_POSITION_OPPONENT_RIGHT || GetBattlerPosition(gBattleAnimAttacker) == B_POSITION_PLAYER_LEFT)
        {
            if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)) == TRUE)
            {
                gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority--;
                SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 1);
                priorityChanged = TRUE;
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

    GetBattleAnimBg1Data(&animBg);
    AnimLoadCompressedBgTilemap(animBg.bgId, gMetalShineTilemap);
    AnimLoadCompressedBgGfx(animBg.bgId, gMetalShineGfx, animBg.tilesOffset);
    LoadCompressedPalette(gMetalShinePalette, BG_PLTT_ID(animBg.paletteId), PLTT_SIZE_4BPP);

    gBattle_BG1_X = -gSprites[spriteId].x + 96;
    gBattle_BG1_Y = -gSprites[spriteId].y + 32;
    paletteNum = 16 + gSprites[spriteId].oam.paletteNum;

    if (cmd->useColor == 0)
        SetGrayscaleOrOriginalPalette(paletteNum, FALSE);
    else
        BlendPalette(BG_PLTT_ID(paletteNum), 16, 11, cmd->color);

    gTasks[taskId].data[0] = newSpriteId;
    gTasks[taskId].data[1] = cmd->permanent;
    gTasks[taskId].data[2] = cmd->useColor;
    gTasks[taskId].data[3] = cmd->color;
    gTasks[taskId].data[6] = priorityChanged;
    gTasks[taskId].func = AnimTask_MetallicShine_Step;
}

static void AnimTask_MetallicShine_Step(u8 taskId)
{
    struct BattleAnimBgData animBg;
    u16 paletteNum;
    u8 spriteId;

    gTasks[taskId].data[10] += 4;
    gBattle_BG1_X -= 4;
    if (gTasks[taskId].data[10] == 128)
    {
        gTasks[taskId].data[10] = 0;
        gBattle_BG1_X += 128;
        gTasks[taskId].data[11]++;
        if (gTasks[taskId].data[11] == 2)
        {
            spriteId = GetAnimBattlerSpriteId(ANIM_ATTACKER);
            paletteNum = 16 + gSprites[spriteId].oam.paletteNum;
            if (gTasks[taskId].data[1] == 0)
                SetGrayscaleOrOriginalPalette(paletteNum, TRUE);

            DestroySprite(&gSprites[gTasks[taskId].data[0]]);
            GetBattleAnimBg1Data(&animBg);
            ClearBattleAnimBg(animBg.bgId);
            if (gTasks[taskId].data[6] == 1)
                gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority++;
        }
        else if (gTasks[taskId].data[11] == 3)
        {
            gBattle_WIN0H = 0;
            gBattle_WIN0V = 0;
            SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR | WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
            if (!IsContest())
                SetAnimBgAttribute(1, BG_ANIM_CHAR_BASE_BLOCK, 0);

            SetGpuReg(REG_OFFSET_DISPCNT, GetGpuReg(REG_OFFSET_DISPCNT) ^ DISPCNT_OBJWIN_ON);
            SetGpuReg(REG_OFFSET_BLDCNT, 0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
            DestroyAnimVisualTask(taskId);
        }
    }
}

// Changes battler's palette to either grayscale or original.
void AnimTask_SetGrayscaleOrOriginalPal(u8 taskId)
{
    CMD_ARGS(battler, mode);

    u8 spriteId;
    u8 battler;
    bool8 calcSpriteId = FALSE;
    u8 position = B_POSITION_PLAYER_LEFT;

    switch (cmd->battler)
    {
    case ANIM_ATTACKER:
    case ANIM_TARGET:
    case ANIM_ATK_PARTNER:
    case ANIM_DEF_PARTNER:
        spriteId = GetAnimBattlerSpriteId(cmd->battler);
        break;
    case ANIM_PLAYER_LEFT:
        position = B_POSITION_PLAYER_LEFT;
        calcSpriteId = TRUE;
        break;
    case ANIM_PLAYER_RIGHT:
        position = B_POSITION_PLAYER_RIGHT;
        calcSpriteId = TRUE;
        break;
    case ANIM_OPPONENT_LEFT:
        position = B_POSITION_OPPONENT_LEFT;
        calcSpriteId = TRUE;
        break;
    case ANIM_OPPONENT_RIGHT:
        position = B_POSITION_OPPONENT_RIGHT;
        calcSpriteId = TRUE;
        break;
    default:
        spriteId = SPRITE_NONE;
        break;
    }

    if (calcSpriteId)
    {
        battler = GetBattlerAtPosition(position);
        if (IsBattlerSpriteVisible(battler))
            spriteId = gBattlerSpriteIds[battler];
        else
            spriteId = SPRITE_NONE;
    }

    if (spriteId != SPRITE_NONE)
        SetGrayscaleOrOriginalPalette(gSprites[spriteId].oam.paletteNum + 16, cmd->mode);

    DestroyAnimVisualTask(taskId);
}

void GetIsDoomDesireHitTurn(u8 taskId)
{
    if (gAnimMoveTurn < 2)
        gBattleAnimArgs[ARG_RET_ID] = FALSE;

    if (gAnimMoveTurn == 2)
        gBattleAnimArgs[ARG_RET_ID] = TRUE;

    DestroyAnimVisualTask(taskId);
}
