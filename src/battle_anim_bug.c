#include "global.h"
#include "battle_anim.h"
#include "battle_anim_internal.h"
#include "gpu_regs.h"
#include "trig.h"
#include "constants/rgb.h"

static void AnimMegahornHorn(struct Sprite *);
static void AnimLeechLifeNeedle(struct Sprite *);
static void AnimTranslateWebThread(struct Sprite *);
static void AnimTranslateWebThread_Step(struct Sprite *);
static void AnimStringWrap(struct Sprite *);
static void AnimStringWrap_Step(struct Sprite *);
static void AnimSpiderWeb(struct Sprite *);
static void AnimSpiderWeb_Step(struct Sprite *);
static void AnimSpiderWeb_End(struct Sprite *);
static void AnimTranslateStinger(struct Sprite *);
static void AnimMissileArc(struct Sprite *);
static void AnimMissileArc_Step(struct Sprite *);
static void AnimTailGlowOrb(struct Sprite *);

static const union AffineAnimCmd sAffineAnim_MegahornHorn_0[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 30, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_MegahornHorn_1[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -99, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_MegahornHorn_2[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 94, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_MegahornHorn[] =
{
    sAffineAnim_MegahornHorn_0,
    sAffineAnim_MegahornHorn_1,
    sAffineAnim_MegahornHorn_2,
};

const struct SpriteTemplate gMegahornHornSpriteTemplate =
{
    .tileTag = ANIM_TAG_HORN_HIT_2,
    .paletteTag = ANIM_TAG_HORN_HIT_2,
    .oam = &gOamData_AffineDouble_ObjNormal_32x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_MegahornHorn,
    .callback = AnimMegahornHorn,
};

static const union AffineAnimCmd sAffineAnim_LeechLifeNeedle_0[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -33, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_LeechLifeNeedle_1[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, 96, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_LeechLifeNeedle_2[] =
{
    AFFINEANIMCMD_FRAME(0x0, 0x0, -96, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_LeechLifeNeedle[] =
{
    sAffineAnim_LeechLifeNeedle_0,
    sAffineAnim_LeechLifeNeedle_1,
    sAffineAnim_LeechLifeNeedle_2,
};

const struct SpriteTemplate gLeechLifeNeedleSpriteTemplate =
{
    .tileTag = ANIM_TAG_NEEDLE,
    .paletteTag = ANIM_TAG_NEEDLE,
    .oam = &gOamData_AffineNormal_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_LeechLifeNeedle,
    .callback = AnimLeechLifeNeedle,
};

const struct SpriteTemplate gWebThreadSpriteTemplate =
{
    .tileTag = ANIM_TAG_WEB_THREAD,
    .paletteTag = ANIM_TAG_WEB_THREAD,
    .oam = &gOamData_AffineOff_ObjNormal_8x8,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimTranslateWebThread,
};

const struct SpriteTemplate gStringWrapSpriteTemplate =
{
    .tileTag = ANIM_TAG_STRING,
    .paletteTag = ANIM_TAG_STRING,
    .oam = &gOamData_AffineOff_ObjNormal_64x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimStringWrap,
};

static const union AffineAnimCmd sAffineAnim_SpiderWeb[] =
{
    AFFINEANIMCMD_FRAME(0x10, 0x10, 0, 0),
    AFFINEANIMCMD_FRAME(0x6, 0x6, 0, 1),
    AFFINEANIMCMD_JUMP(1),
};

static const union AffineAnimCmd *const sAffineAnims_SpiderWeb[] =
{
    sAffineAnim_SpiderWeb,
};

const struct SpriteTemplate gSpiderWebSpriteTemplate =
{
    .tileTag = ANIM_TAG_SPIDER_WEB,
    .paletteTag = ANIM_TAG_SPIDER_WEB,
    .oam = &gOamData_AffineDouble_ObjBlend_64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_SpiderWeb,
    .callback = AnimSpiderWeb,
};

const struct SpriteTemplate gLinearStingerSpriteTemplate =
{
    .tileTag = ANIM_TAG_NEEDLE,
    .paletteTag = ANIM_TAG_NEEDLE,
    .oam = &gOamData_AffineNormal_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimTranslateStinger,
};

const struct SpriteTemplate gPinMissileSpriteTemplate =
{
    .tileTag = ANIM_TAG_NEEDLE,
    .paletteTag = ANIM_TAG_NEEDLE,
    .oam = &gOamData_AffineNormal_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimMissileArc,
};

const struct SpriteTemplate gIcicleSpearSpriteTemplate =
{
    .tileTag = ANIM_TAG_ICICLE_SPEAR,
    .paletteTag = ANIM_TAG_ICICLE_SPEAR,
    .oam = &gOamData_AffineNormal_ObjNormal_32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AnimMissileArc,
};

static const union AffineAnimCmd sAffineAnim_TailGlowOrb[] =
{
    AFFINEANIMCMD_FRAME(0x10, 0x10, 0, 0),
    AFFINEANIMCMD_FRAME(0x8, 0x8, 0, 18),
    AFFINEANIMCMD_LOOP(0),
    AFFINEANIMCMD_FRAME(0xFFFB, 0xFFFB, 0, 8),
    AFFINEANIMCMD_FRAME(0x5, 0x5, 0, 8),
    AFFINEANIMCMD_LOOP(5),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_TailGlowOrb[] =
{
    sAffineAnim_TailGlowOrb,
};

const struct SpriteTemplate gTailGlowOrbSpriteTemplate =
{
    .tileTag = ANIM_TAG_CIRCLE_OF_LIGHT,
    .paletteTag = ANIM_TAG_CIRCLE_OF_LIGHT,
    .oam = &gOamData_AffineNormal_ObjBlend_64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_TailGlowOrb,
    .callback = AnimTailGlowOrb,
};

static void AnimMegahornHorn(struct Sprite *sprite)
{
    CMD_ARGS(x1, y1, x2, y2, duration);

    if (IsContest())
    {
        StartSpriteAffineAnim(sprite, 2);
        cmd->x2 = -cmd->x2;
        cmd->x1 = -cmd->x1;
    }
    else if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
    {
        StartSpriteAffineAnim(sprite, 1);
        cmd->y1 = -cmd->y1;
        cmd->x2 = -cmd->x2;
        cmd->y2 = -cmd->y2;
        cmd->x1 = -cmd->x1;
    }

    sprite->x = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_X_2) + cmd->x1;
    sprite->y = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + cmd->y1;
    sprite->data[0] = cmd->duration;

    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + cmd->x2;
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + cmd->y2;

    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

static void AnimLeechLifeNeedle(struct Sprite *sprite)
{
    CMD_ARGS(x, y, duration);

    if (IsContest())
    {
        cmd->x = -cmd->x;
        StartSpriteAffineAnim(sprite, 2);
    }
    else if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
    {
        cmd->y = -cmd->y;
        cmd->x = -cmd->x;
    }

    sprite->x = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_X_2) + cmd->x;
    sprite->y = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + cmd->y;
    sprite->data[0] = cmd->duration;

    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET);

    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

// Creates a single web thread that travels from attacker to target.
// Used by MOVE_STRING_SHOT and MOVE_SPIDER_WEB in their first move phase.
// arg 2: controls the left-to-right movement
static void AnimTranslateWebThread(struct Sprite *sprite)
{
    CMD_ARGS(x, y, unk2, amplitude, targetsBoth);

    if (IsContest())
        cmd->unk2 /= 2;

    InitSpritePosToAnimAttacker(sprite, TRUE);
    sprite->data[0] = cmd->unk2;
    sprite->data[1] = sprite->x;
    sprite->data[3] = sprite->y;

    if (!cmd->targetsBoth)
    {
        sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
        sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET);
    }
    else
    {
        SetAverageBattlerPositions(gBattleAnimTarget, TRUE, &sprite->data[2], &sprite->data[4]);
    }

    InitAnimLinearTranslationWithSpeed(sprite);
    sprite->data[5] = cmd->amplitude;
    sprite->callback = AnimTranslateWebThread_Step;
}

static void AnimTranslateWebThread_Step(struct Sprite *sprite)
{
    if (AnimTranslateLinear(sprite))
    {
        DestroyAnimSprite(sprite);
        return;
    }

    sprite->x2 += Sin(sprite->data[6], sprite->data[5]);
    sprite->data[6] = (sprite->data[6] + 13) & 0xFF;
}

// Second stage of String Shot
static void AnimStringWrap(struct Sprite *sprite)
{
    CMD_ARGS(x, y);

    SetAverageBattlerPositions(gBattleAnimTarget, FALSE, &sprite->x, &sprite->y);
    if (GetBattlerSide(gBattleAnimAttacker))
        sprite->x -= cmd->x;
    else
        sprite->x += cmd->x;

    sprite->y += cmd->y;
    if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
        sprite->y += 8;

    sprite->callback = AnimStringWrap_Step;
}

static void AnimStringWrap_Step(struct Sprite *sprite)
{
    if (++sprite->data[0] == 3)
    {
        sprite->data[0] = 0;
        sprite->invisible ^= 1;
    }

    if (++sprite->data[1] == 51)
    {
        DestroyAnimSprite(sprite);
    }
}

static void AnimSpiderWeb(struct Sprite *sprite)
{
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 0));

    sprite->data[0] = 16;
    sprite->callback = AnimSpiderWeb_Step;
}

static void AnimSpiderWeb_Step(struct Sprite *sprite)
{
    if (sprite->data[2] < 20)
    {
        sprite->data[2]++;
    }
    else if (sprite->data[1]++ & 1)
    {
        sprite->data[0]--;
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(sprite->data[0], 16 - sprite->data[0]));

        if (sprite->data[0] == 0)
        {
            sprite->invisible = TRUE;
            sprite->callback = AnimSpiderWeb_End;
        }
    }
}

static void AnimSpiderWeb_End(struct Sprite *sprite)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DestroyAnimSprite(sprite);
}

// Translates a stinger sprite linearly to a destination location. The sprite is
// initially rotated so that it appears to be traveling in a straight line.
static void AnimTranslateStinger(struct Sprite *sprite)
{
    CMD_ARGS(initialX, intialY, targetX, targetY, duration);

    s16 lVarX, lVarY;
    u16 rot;

    if (IsContest())
    {
        cmd->targetX = -cmd->targetX;
    }
    else
    {
        if (GetBattlerSide(gBattleAnimAttacker))
        {
            cmd->targetX = -cmd->targetX;
            cmd->intialY = -cmd->intialY;
            cmd->targetY = -cmd->targetY;
        }
    }

    if (!IsContest() && GetBattlerSide(gBattleAnimAttacker) == GetBattlerSide(gBattleAnimTarget))
    {
        if (GetBattlerPosition(gBattleAnimTarget) == B_POSITION_PLAYER_LEFT
         || GetBattlerPosition(gBattleAnimTarget) == B_POSITION_OPPONENT_LEFT)
        {
            cmd->targetX *= -1;
            cmd->initialX *= -1;
        }
    }

    InitSpritePosToAnimAttacker(sprite, TRUE);

    lVarX = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + cmd->targetX;
    lVarY = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + cmd->targetY;
    rot = ArcTan2Neg(lVarX - sprite->x, lVarY - sprite->y);
    rot += 0xC000;
    TrySetSpriteRotScale(sprite, FALSE, 0x100, 0x100, rot);

    sprite->data[0] = cmd->duration;
    sprite->data[2] = lVarX;
    sprite->data[4] = lVarY;

    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

// Rotates sprite and moves it in an arc, so that it appears like a missle or arrow traveling.
static void AnimMissileArc(struct Sprite *sprite)
{
    CMD_ARGS(initialX, intialY, targetX, targetY, duration, waveAmplitude);

    InitSpritePosToAnimAttacker(sprite, TRUE);

    if (GetBattlerSide(gBattleAnimAttacker))
        cmd->targetX = -cmd->targetX;

    sprite->data[0] = cmd->duration;
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + cmd->targetX;
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + cmd->targetY;
    sprite->data[5] = cmd->waveAmplitude;
    InitAnimArcTranslation(sprite);

    sprite->callback = AnimMissileArc_Step;
    sprite->invisible = TRUE;
}

static void AnimMissileArc_Step(struct Sprite *sprite)
{
    sprite->invisible = FALSE;

    if (TranslateAnimHorizontalArc(sprite))
    {
        DestroyAnimSprite(sprite);
    }
    else
    {
        s16 tempData[8];
        s16 *data = sprite->data;
        u16 x1 = sprite->x;
        s16 x2 = sprite->x2;
        u16 y1 = sprite->y;
        s16 y2 = sprite->y2;
        int i;

        for (i = 0; i < 8; i++)
            tempData[i] = data[i];

        x2 += x1;
        y2 += y1;

        if (!TranslateAnimHorizontalArc(sprite))
        {
            u16 rotation = ArcTan2Neg(sprite->x + sprite->x2 - x2,
                                  sprite->y + sprite->y2 - y2);
            rotation += 0xC000;
            TrySetSpriteRotScale(sprite, FALSE, 0x100, 0x100, rotation);

            for (i = 0; i < 8; i++)
                data[i] = tempData[i];
        }
    }
}

static void AnimTailGlowOrb(struct Sprite *sprite)
{
    CMD_ARGS(relativeTo);

    if (cmd->relativeTo == ANIM_ATTACKER)
    {
        sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
        sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET) + 18;
    }
    else
    {
        sprite->x = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
        sprite->y = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + 18;
    }

    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = RunStoredCallbackWhenAffineAnimEnds;
}
