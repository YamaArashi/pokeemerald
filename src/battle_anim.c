#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "bg.h"
#include "contest.h"
#include "decompress.h"
#include "dma3.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "main.h"
#include "m4a.h"
#include "palette.h"
#include "pokemon.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "constants/battle_anim.h"

/*
    This file handles the commands for the macros defined in
    battle_anim_script.inc and used in battle_anim_scripts.s
*/

#define ANIM_SPRITE_INDEX_COUNT 8

extern const u16 gMovesWithQuietBGM[];
extern const u8 *const gBattleAnims_Moves[];

static void Cmd_loadspritegfx(void);
static void Cmd_unloadspritegfx(void);
static void Cmd_createsprite(void);
static void Cmd_createvisualtask(void);
static void Cmd_delay(void);
static void Cmd_waitforvisualfinish(void);
static void Cmd_nop(void);
static void Cmd_nop2(void);
static void Cmd_end(void);
static void Cmd_playse(void);
static void Cmd_monbg(void);
static void Cmd_clearmonbg(void);
static void Cmd_setalpha(void);
static void Cmd_blendoff(void);
static void Cmd_call(void);
static void Cmd_return(void);
static void Cmd_setarg(void);
static void Cmd_choosetwoturnanim(void);
static void Cmd_jumpifmoveturn(void);
static void Cmd_goto(void);
static void Cmd_fadetobg(void);
static void Cmd_restorebg(void);
static void Cmd_waitbgfadeout(void);
static void Cmd_waitbgfadein(void);
static void Cmd_changebg(void);
static void Cmd_playsewithpan(void);
static void Cmd_setpan(void);
static void Cmd_panse(void);
static void Cmd_loopsewithpan(void);
static void Cmd_waitplaysewithpan(void);
static void Cmd_setbldcnt(void);
static void Cmd_createsoundtask(void);
static void Cmd_waitsound(void);
static void Cmd_jumpargeq(void);
static void Cmd_monbg_static(void);
static void Cmd_clearmonbg_static(void);
static void Cmd_jumpifcontest(void);
static void Cmd_fadetobgfromset(void);
static void Cmd_panse_adjustnone(void);
static void Cmd_panse_adjustall(void);
static void Cmd_splitbgprio(void);
static void Cmd_splitbgprio_all(void);
static void Cmd_splitbgprio_foes(void);
static void Cmd_invisible(void);
static void Cmd_visible(void);
static void Cmd_teamattack_moveback(void);
static void Cmd_teamattack_movefwd(void);
static void Cmd_stopsound(void);

static void RunAnimScriptCommand(void);
static void Task_UpdateMonBg(u8 taskId);
static void FlipBattlerBgTiles(void);
static void Task_ClearMonBg(u8 taskId);
static void Task_ClearMonBgStatic(u8 taskId);
static void Task_FadeToBg(u8 taskId);
static void Task_PanFromInitialToTarget(u8 taskId);
static void Task_LoopAndPlaySE(u8 taskId);
static void Task_WaitAndPlaySE(u8 taskId);
static void LoadDefaultBg(void);
static void LoadMoveBg(u16 bgId);

EWRAM_DATA static const u8 *sBattleAnimScriptPtr = NULL;
EWRAM_DATA static const u8 *sBattleAnimScriptRetAddr = NULL;
EWRAM_DATA void (*gAnimScriptCallback)(void) = NULL;
EWRAM_DATA static s8 sAnimFramesToWait = 0;
EWRAM_DATA bool8 gAnimScriptActive = FALSE;
EWRAM_DATA u8 gAnimVisualTaskCount = 0;
EWRAM_DATA u8 gAnimSoundTaskCount = 0;
EWRAM_DATA struct DisableStruct *gAnimDisableStructPtr = NULL;
EWRAM_DATA s32 gAnimMoveDmg = 0;
EWRAM_DATA u16 gAnimMovePower = 0;
EWRAM_DATA static u16 sAnimSpriteIndexArray[ANIM_SPRITE_INDEX_COUNT] = {0};
EWRAM_DATA u8 gAnimFriendship = 0;
EWRAM_DATA u16 gWeatherMoveAnim = 0;
EWRAM_DATA s16 gBattleAnimArgs[ANIM_ARGS_COUNT] = {0};
EWRAM_DATA static u16 sSoundAnimFramesToWait = 0;
EWRAM_DATA static u8 sMonAnimTaskIdArray[2] = {0};
EWRAM_DATA u8 gAnimMoveTurn = 0;
EWRAM_DATA static u8 sAnimBackgroundFadeState = 0;
EWRAM_DATA static u16 sAnimMoveIndex = 0; // Set but unused.
EWRAM_DATA u8 gBattleAnimAttacker = 0;
EWRAM_DATA u8 gBattleAnimTarget = 0;
EWRAM_DATA u16 gAnimBattlerSpecies[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gAnimCustomPanning = 0;

#include "data/battle_anim.h"

static void (* const sScriptCmdTable[])(void) =
{
    Cmd_loadspritegfx,        // 0x00
    Cmd_unloadspritegfx,      // 0x01
    Cmd_createsprite,         // 0x02
    Cmd_createvisualtask,     // 0x03
    Cmd_delay,                // 0x04
    Cmd_waitforvisualfinish,  // 0x05
    Cmd_nop,                  // 0x06
    Cmd_nop2,                 // 0x07
    Cmd_end,                  // 0x08
    Cmd_playse,               // 0x09
    Cmd_monbg,                // 0x0A
    Cmd_clearmonbg,           // 0x0B
    Cmd_setalpha,             // 0x0C
    Cmd_blendoff,             // 0x0D
    Cmd_call,                 // 0x0E
    Cmd_return,               // 0x0F
    Cmd_setarg,               // 0x10
    Cmd_choosetwoturnanim,    // 0x11
    Cmd_jumpifmoveturn,       // 0x12
    Cmd_goto,                 // 0x13
    Cmd_fadetobg,             // 0x14
    Cmd_restorebg,            // 0x15
    Cmd_waitbgfadeout,        // 0x16
    Cmd_waitbgfadein,         // 0x17
    Cmd_changebg,             // 0x18
    Cmd_playsewithpan,        // 0x19
    Cmd_setpan,               // 0x1A
    Cmd_panse,                // 0x1B
    Cmd_loopsewithpan,        // 0x1C
    Cmd_waitplaysewithpan,    // 0x1D
    Cmd_setbldcnt,            // 0x1E
    Cmd_createsoundtask,      // 0x1F
    Cmd_waitsound,            // 0x20
    Cmd_jumpargeq,            // 0x21
    Cmd_monbg_static,         // 0x22
    Cmd_clearmonbg_static,    // 0x23
    Cmd_jumpifcontest,        // 0x24
    Cmd_fadetobgfromset,      // 0x25
    Cmd_panse_adjustnone,     // 0x26
    Cmd_panse_adjustall,      // 0x27
    Cmd_splitbgprio,          // 0x28
    Cmd_splitbgprio_all,      // 0x29
    Cmd_splitbgprio_foes,     // 0x2A
    Cmd_invisible,            // 0x2B
    Cmd_visible,              // 0x2C
    Cmd_teamattack_moveback,  // 0x2D
    Cmd_teamattack_movefwd,   // 0x2E
    Cmd_stopsound,            // 0x2F
};

void ClearBattleAnimationVars(void)
{
    s32 i;

    sAnimFramesToWait = 0;
    gAnimScriptActive = FALSE;
    gAnimVisualTaskCount = 0;
    gAnimSoundTaskCount = 0;
    gAnimDisableStructPtr = NULL;
    gAnimMoveDmg = 0;
    gAnimMovePower = 0;
    gAnimFriendship = 0;

    // Clear index array.
    for (i = 0; i < ANIM_SPRITE_INDEX_COUNT; i++)
        sAnimSpriteIndexArray[i] = 0xFFFF;

    // Clear anim args.
    for (i = 0; i < ANIM_ARGS_COUNT; i++)
        gBattleAnimArgs[i] = 0;

    sMonAnimTaskIdArray[0] = TASK_NONE;
    sMonAnimTaskIdArray[1] = TASK_NONE;
    gAnimMoveTurn = 0;
    sAnimBackgroundFadeState = 0;
    sAnimMoveIndex = 0;
    gBattleAnimAttacker = 0;
    gBattleAnimTarget = 0;
    gAnimCustomPanning = 0;
}

void DoMoveAnim(u16 move)
{
    gBattleAnimAttacker = gBattlerAttacker;
    gBattleAnimTarget = gBattlerTarget;
    LaunchBattleAnimation(gBattleAnims_Moves, move, TRUE);
}

void LaunchBattleAnimation(const u8 *const animsTable[], u16 tableId, bool8 isMoveAnim)
{
    s32 i;

    if (!IsContest())
    {
        InitPrioritiesForVisibleBattlers();
        UpdateOamPriorityInAllHealthboxes(0);
        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            if (GetBattlerSide(i) != B_SIDE_PLAYER)
                gAnimBattlerSpecies[i] = GetMonData(&gEnemyParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
            else
                gAnimBattlerSpecies[i] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
        }
    }
    else
    {
        for (i = 0; i < CONTESTANT_COUNT; i++)
            gAnimBattlerSpecies[i] = gContestResources->moveAnim->species;
    }

    if (!isMoveAnim)
        sAnimMoveIndex = 0;
    else
        sAnimMoveIndex = tableId;

    for (i = 0; i < ANIM_ARGS_COUNT; i++)
        gBattleAnimArgs[i] = 0;

    sMonAnimTaskIdArray[0] = TASK_NONE;
    sMonAnimTaskIdArray[1] = TASK_NONE;
    sBattleAnimScriptPtr = animsTable[tableId];
    gAnimScriptActive = TRUE;
    sAnimFramesToWait = 0;
    gAnimScriptCallback = RunAnimScriptCommand;

    for (i = 0; i < ANIM_SPRITE_INDEX_COUNT; i++)
        sAnimSpriteIndexArray[i] = 0xFFFF;

    if (isMoveAnim)
    {
        for (i = 0; gMovesWithQuietBGM[i] != 0xFFFF; i++)
        {
            if (tableId == gMovesWithQuietBGM[i])
            {
                m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 128);
                break;
            }
        }
    }

    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    gBattle_WIN1H = 0;
    gBattle_WIN1V = 0;
}

void DestroyAnimSprite(struct Sprite *sprite)
{
    FreeSpriteOamMatrix(sprite);
    DestroySprite(sprite);
    gAnimVisualTaskCount--;
}

void DestroyAnimVisualTask(u8 taskId)
{
    DestroyTask(taskId);
    gAnimVisualTaskCount--;
}

void DestroyAnimSoundTask(u8 taskId)
{
    DestroyTask(taskId);
    gAnimSoundTaskCount--;
}

static void AddSpriteIndex(u16 index)
{
    s32 i;

    for (i = 0; i < ANIM_SPRITE_INDEX_COUNT; i++)
    {
        if (sAnimSpriteIndexArray[i] == 0xFFFF)
        {
            sAnimSpriteIndexArray[i] = index;
            return;
        }
    }
}

static void ClearSpriteIndex(u16 index)
{
    s32 i;

    for (i = 0; i < ANIM_SPRITE_INDEX_COUNT; i++)
    {
        if (sAnimSpriteIndexArray[i] == index)
        {
            sAnimSpriteIndexArray[i] = 0xFFFF;
            return;
        }
    }
}

static void WaitAnimFrameCount(void)
{
    if (sAnimFramesToWait <= 0)
    {
        gAnimScriptCallback = RunAnimScriptCommand;
        sAnimFramesToWait = 0;
    }
    else
    {
        sAnimFramesToWait--;
    }
}

static void RunAnimScriptCommand(void)
{
    do
    {
        sScriptCmdTable[sBattleAnimScriptPtr[0]]();
    } while (sAnimFramesToWait == 0 && gAnimScriptActive);
}

static void Cmd_loadspritegfx(void)
{
    u16 index;

    sBattleAnimScriptPtr++;
    index = T1_READ_16(sBattleAnimScriptPtr);
    LoadCompressedSpriteSheetUsingHeap(&gBattleAnimPicTable[GET_TRUE_SPRITE_INDEX(index)]);
    LoadCompressedSpritePaletteUsingHeap(&gBattleAnimPaletteTable[GET_TRUE_SPRITE_INDEX(index)]);
    sBattleAnimScriptPtr += 2;
    AddSpriteIndex(GET_TRUE_SPRITE_INDEX(index));
    sAnimFramesToWait = 1;
    gAnimScriptCallback = WaitAnimFrameCount;
}

static void Cmd_unloadspritegfx(void)
{
    u16 index;

    sBattleAnimScriptPtr++;
    index = T1_READ_16(sBattleAnimScriptPtr);
    FreeSpriteTilesByTag(gBattleAnimPicTable[GET_TRUE_SPRITE_INDEX(index)].tag);
    FreeSpritePaletteByTag(gBattleAnimPicTable[GET_TRUE_SPRITE_INDEX(index)].tag);
    sBattleAnimScriptPtr += 2;
    ClearSpriteIndex(GET_TRUE_SPRITE_INDEX(index));
}

static void Cmd_createsprite(void)
{
    s32 i;
    const struct SpriteTemplate *template;
    u8 argVar;
    u8 argsCount;
    s16 subpriority;

    sBattleAnimScriptPtr++;
    template = (const struct SpriteTemplate *)(T2_READ_32(sBattleAnimScriptPtr));
    sBattleAnimScriptPtr += 4;

    argVar = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;

    argsCount = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;
    for (i = 0; i < argsCount; i++)
    {
        gBattleAnimArgs[i] = T1_READ_16(sBattleAnimScriptPtr);
        sBattleAnimScriptPtr += 2;
    }

    if (argVar & ANIMSPRITE_IS_TARGET)
    {
        argVar ^= ANIMSPRITE_IS_TARGET;
        if (argVar >= 64)
            argVar -= 64;
        else
            argVar *= -1;

        subpriority = GetBattlerSpriteSubpriority(gBattleAnimTarget) + (s8)(argVar);
    }
    else
    {
        if (argVar >= 64)
            argVar -= 64;
        else
            argVar *= -1;

        subpriority = GetBattlerSpriteSubpriority(gBattleAnimAttacker) + (s8)(argVar);
    }

    if (subpriority < 3)
        subpriority = 3;

    CreateSpriteAndAnimate(
        template,
        GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2),
        GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET),
        subpriority);
    gAnimVisualTaskCount++;
}

static void Cmd_createvisualtask(void)
{
    TaskFunc taskFunc;
    u8 taskPriority;
    u8 taskId;
    u8 numArgs;
    s32 i;

    sBattleAnimScriptPtr++;

    taskFunc = (TaskFunc)T2_READ_32(sBattleAnimScriptPtr);
    sBattleAnimScriptPtr += 4;

    taskPriority = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;

    numArgs = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;

    for (i = 0; i < numArgs; i++)
    {
        gBattleAnimArgs[i] = T1_READ_16(sBattleAnimScriptPtr);
        sBattleAnimScriptPtr += 2;
    }

    taskId = CreateTask(taskFunc, taskPriority);
    taskFunc(taskId);
    gAnimVisualTaskCount++;
}

static void Cmd_delay(void)
{
    sBattleAnimScriptPtr++;
    sAnimFramesToWait = sBattleAnimScriptPtr[0];
    if (sAnimFramesToWait == 0)
        sAnimFramesToWait = -1;
    sBattleAnimScriptPtr++;
    gAnimScriptCallback = WaitAnimFrameCount;
}

// Wait for visual tasks to finish.
static void Cmd_waitforvisualfinish(void)
{
    if (gAnimVisualTaskCount == 0)
    {
        sBattleAnimScriptPtr++;
        sAnimFramesToWait = 0;
    }
    else
    {
        sAnimFramesToWait = 1;
    }
}

static void Cmd_nop(void)
{
}

static void Cmd_nop2(void)
{
}

static void Cmd_end(void)
{
    s32 i;
    bool32 continuousAnim = FALSE;

    // Keep waiting as long as there are animations to be done.
    if (gAnimVisualTaskCount != 0 || gAnimSoundTaskCount != 0
     || sMonAnimTaskIdArray[0] != TASK_NONE || sMonAnimTaskIdArray[1] != TASK_NONE)
    {
        sSoundAnimFramesToWait = 0;
        sAnimFramesToWait = 1;
        return;
    }

    // Finish the sound effects.
    if (IsSEPlaying())
    {
        if (++sSoundAnimFramesToWait <= 90) // Wait 90 frames, then halt the sound effect.
        {
            sAnimFramesToWait = 1;
            return;
        }
        else
        {
            m4aMPlayStop(&gMPlayInfo_SE1);
            m4aMPlayStop(&gMPlayInfo_SE2);
        }
    }

    // The SE has halted, so set the SE Frame Counter to 0 and continue.
    sSoundAnimFramesToWait = 0;

    for (i = 0; i < ANIM_SPRITE_INDEX_COUNT; i++)
    {
        if (sAnimSpriteIndexArray[i] != 0xFFFF)
        {
            FreeSpriteTilesByTag(gBattleAnimPicTable[sAnimSpriteIndexArray[i]].tag);
            FreeSpritePaletteByTag(gBattleAnimPicTable[sAnimSpriteIndexArray[i]].tag);
            sAnimSpriteIndexArray[i] = 0xFFFF; // set terminator.
        }
    }

    if (!continuousAnim) // May have been used for debug?
    {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 256);
        if (!IsContest())
        {
            InitPrioritiesForVisibleBattlers();
            UpdateOamPriorityInAllHealthboxes(1);
        }
        gAnimScriptActive = FALSE;
    }
}

static void Cmd_playse(void)
{
    sBattleAnimScriptPtr++;
    PlaySE(T1_READ_16(sBattleAnimScriptPtr));
    sBattleAnimScriptPtr += 2;
}

// These two tasks share context and similar task data
// To differentiate them the task data for Task_UpdateMonBg is prefixed t2

// Task data for Task_InitUpdateMonBg
#define tBattlerId  data[0]
#define tInBg2      data[1]
#define tActive     data[2]
#define tIsPartner  data[3]

// Task data for Task_UpdateMonBg
#define t2_SpriteId  data[0]
#define t2_SpriteX   data[1]
#define t2_SpriteY   data[2]
#define t2_BgX       data[3]
#define t2_BgY       data[4]
#define t2_InBg2     data[5]
#define t2_BattlerId data[6]

static void Task_InitUpdateMonBg(u8 taskId)
{
    u8 updateTaskId;

    s16 *data = gTasks[taskId].data;
    u8 battlerSpriteId = gBattlerSpriteIds[tBattlerId];
    gSprites[battlerSpriteId].invisible = TRUE;

    if (!tActive)
    {
        DestroyAnimVisualTask(taskId);
        return;
    }

    updateTaskId = CreateTask(Task_UpdateMonBg, 10);
    gTasks[updateTaskId].t2_SpriteId = battlerSpriteId;
    gTasks[updateTaskId].t2_SpriteX = gSprites[battlerSpriteId].x + gSprites[battlerSpriteId].x2;
    gTasks[updateTaskId].t2_SpriteY = gSprites[battlerSpriteId].y + gSprites[battlerSpriteId].y2;

    if (!tInBg2)
    {
        gTasks[updateTaskId].t2_BgX = gBattle_BG1_X;
        gTasks[updateTaskId].t2_BgY = gBattle_BG1_Y;
    }
    else
    {
        gTasks[updateTaskId].t2_BgX = gBattle_BG2_X;
        gTasks[updateTaskId].t2_BgY = gBattle_BG2_Y;
    }

    gTasks[updateTaskId].t2_InBg2 = tInBg2;
    gTasks[updateTaskId].t2_BattlerId = tBattlerId;
    sMonAnimTaskIdArray[tIsPartner] = updateTaskId;
    DestroyAnimVisualTask(taskId);
}

static void Cmd_monbg(void)
{
    bool8 toBG_2;
    u8 taskId;
    u8 battlerId;
    u8 animBattler;

    sBattleAnimScriptPtr++;

    animBattler = sBattleAnimScriptPtr[0];
    if (animBattler & ANIM_TARGET)
        battlerId = gBattleAnimTarget;
    else
        battlerId = gBattleAnimAttacker;

    // Move designated battler to background
    if (IsBattlerSpriteVisible(battlerId))
    {
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            toBG_2 = FALSE;
        else
            toBG_2 = TRUE;

        MoveBattlerSpriteToBG(battlerId, toBG_2, FALSE);
        taskId = CreateTask(Task_InitUpdateMonBg, 10);
        gAnimVisualTaskCount++;
        gTasks[taskId].tBattlerId = battlerId;
        gTasks[taskId].tInBg2 = toBG_2;
        gTasks[taskId].tActive = TRUE;
        gTasks[taskId].tIsPartner = FALSE;

    }

    // Move battler's partner to background
    battlerId ^= BIT_FLANK;
    if (IsBattlerSpriteVisible(battlerId))
    {
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            toBG_2 = FALSE;
        else
            toBG_2 = TRUE;

        MoveBattlerSpriteToBG(battlerId, toBG_2, FALSE);
        taskId = CreateTask(Task_InitUpdateMonBg, 10);
        gAnimVisualTaskCount++;
        gTasks[taskId].tBattlerId = battlerId;
        gTasks[taskId].tInBg2 = toBG_2;
        gTasks[taskId].tActive = TRUE;
        gTasks[taskId].tIsPartner = TRUE;
    }

    sBattleAnimScriptPtr++;
    sAnimFramesToWait = 1;
    gAnimScriptCallback = WaitAnimFrameCount;
}

bool8 IsBattlerSpriteVisible(u8 battlerId)
{
    if (IsContest())
    {
        if (battlerId == gBattleAnimAttacker)
            return TRUE;
        else
            return FALSE;
    }
    if (!IsBattlerSpritePresent(battlerId))
        return FALSE;
    if (IsContest())
        return TRUE; // This line won't ever be reached.
    if (!gBattleSpritesDataPtr->battlerData[battlerId].invisible || !gSprites[gBattlerSpriteIds[battlerId]].invisible)
        return TRUE;

    return FALSE;
}

void MoveBattlerSpriteToBG(u8 battlerId, bool8 toBG_2, bool8 setSpriteInvisible)
{
    struct BattleAnimBgData animBg;
    u8 battlerSpriteId;

    if (!toBG_2)
    {
        u8 battlerPosition;

        if (IsContest() == TRUE)
        {
            RequestDma3Fill(0, (void *)(BG_SCREEN_ADDR(16)), 0x2000, 1);
            RequestDma3Fill(0xFF, (void *)(BG_SCREEN_ADDR(30)), 0x1000, 0);
        }
        else
        {
            RequestDma3Fill(0, (void *)(BG_SCREEN_ADDR(8)), 0x2000, 1);
            RequestDma3Fill(0xFF, (void *)(BG_SCREEN_ADDR(28)), 0x1000, 0);
        }

        GetBattleAnimBg1Data(&animBg);
        CpuFill16(0, animBg.bgTiles, 0x1000);
        CpuFill16(0xFF, animBg.bgTilemap, 0x800);

        SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 2);
        SetAnimBgAttribute(1, BG_ANIM_SCREEN_SIZE, 1);
        SetAnimBgAttribute(1, BG_ANIM_AREA_OVERFLOW_MODE, 0);

        battlerSpriteId = gBattlerSpriteIds[battlerId];

        gBattle_BG1_X =  -(gSprites[battlerSpriteId].x + gSprites[battlerSpriteId].x2) + 0x20;
        if (IsContest() && IsSpeciesNotUnown(gContestResources->moveAnim->species))
            gBattle_BG1_X--;

        gBattle_BG1_Y =  -(gSprites[battlerSpriteId].y + gSprites[battlerSpriteId].y2) + 0x20;
        if (setSpriteInvisible)
            gSprites[gBattlerSpriteIds[battlerId]].invisible = TRUE;

        SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
        SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);

        LoadPalette(&gPlttBufferUnfaded[OBJ_PLTT_ID(battlerId)], BG_PLTT_ID(animBg.paletteId), PLTT_SIZE_4BPP);
        CpuCopy32(&gPlttBufferUnfaded[OBJ_PLTT_ID(battlerId)], (void *)(BG_PLTT + PLTT_OFFSET_4BPP(animBg.paletteId)), PLTT_SIZE_4BPP);

        if (IsContest())
            battlerPosition = 0;
        else
            battlerPosition = GetBattlerPosition(battlerId);

        DrawBattlerOnBg(1, 0, 0, battlerPosition, animBg.paletteId, animBg.bgTiles, animBg.bgTilemap, animBg.tilesOffset);

        if (IsContest())
            FlipBattlerBgTiles();
    }
    else
    {
        RequestDma3Fill(0, (void *)(BG_SCREEN_ADDR(12)), 0x2000, 1);
        RequestDma3Fill(0, (void *)(BG_SCREEN_ADDR(30)), 0x1000, 1);
        GetBattleAnimBgData(&animBg, 2);
        CpuFill16(0, animBg.bgTiles + 0x1000, 0x1000);
        CpuFill16(0, animBg.bgTilemap + 0x400, 0x800);
        SetAnimBgAttribute(2, BG_ANIM_PRIORITY, 2);
        SetAnimBgAttribute(2, BG_ANIM_SCREEN_SIZE, 1);
        SetAnimBgAttribute(2, BG_ANIM_AREA_OVERFLOW_MODE, 0);

        battlerSpriteId = gBattlerSpriteIds[battlerId];

        gBattle_BG2_X =  -(gSprites[battlerSpriteId].x + gSprites[battlerSpriteId].x2) + 0x20;
        gBattle_BG2_Y =  -(gSprites[battlerSpriteId].y + gSprites[battlerSpriteId].y2) + 0x20;

        if (setSpriteInvisible)
            gSprites[gBattlerSpriteIds[battlerId]].invisible = TRUE;

        SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
        SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);

        LoadPalette(&gPlttBufferUnfaded[OBJ_PLTT_ID(battlerId)], BG_PLTT_ID(9), PLTT_SIZE_4BPP);
        CpuCopy32(&gPlttBufferUnfaded[OBJ_PLTT_ID(battlerId)], (void *)(BG_PLTT + PLTT_OFFSET_4BPP(9)), PLTT_SIZE_4BPP);

        DrawBattlerOnBg(2, 0, 0, GetBattlerPosition(battlerId), animBg.paletteId, animBg.bgTiles + 0x1000, animBg.bgTilemap + 0x400, animBg.tilesOffset);
    }
}

static void FlipBattlerBgTiles(void)
{
    s32 i, j;
    struct BattleAnimBgData animBg;
    u16 *ptr;

    if (IsSpeciesNotUnown(gContestResources->moveAnim->species))
    {
        GetBattleAnimBg1Data(&animBg);
        ptr = animBg.bgTilemap;
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 4; j++)
            {
                u16 temp;
                SWAP(ptr[j + i * 32], ptr[7 - j + i * 32], temp);
            }
        }
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
                ptr[j + i * 32] ^= 0x400;
        }
    }
}

void RelocateBattleBgPal(u16 paletteNum, u16 *dest, u32 offset, bool8 largeScreen)
{
    s32 i, j;
    s32 size;

    if (!largeScreen)
        size = 32;
    else
        size = 64;
    paletteNum <<= 12;
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < 32; j++)
            dest[j + i * 32] = ((dest[j + i * 32] & 0xFFF) | paletteNum) + offset;
    }
}

void ResetBattleAnimBg(bool8 toBG2)
{
    struct BattleAnimBgData animBg;
    GetBattleAnimBg1Data(&animBg);

    if (!toBG2 || IsContest())
    {
        ClearBattleAnimBg(1);
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
    }
    else
    {
        ClearBattleAnimBg(2);
        gBattle_BG2_X = 0;
        gBattle_BG2_Y = 0;
    }
}

static void Task_UpdateMonBg(u8 taskId)
{
    u8 spriteId, battlerId;
    s16 x, y;
    struct BattleAnimBgData animBg;

    spriteId = gTasks[taskId].t2_SpriteId;
    battlerId = gTasks[taskId].t2_BattlerId;
    GetBattleAnimBg1Data(&animBg);
    x = gTasks[taskId].t2_SpriteX - (gSprites[spriteId].x + gSprites[spriteId].x2);
    y = gTasks[taskId].t2_SpriteY - (gSprites[spriteId].y + gSprites[spriteId].y2);

    if (!gTasks[taskId].t2_InBg2)
    {
        gBattle_BG1_X = x + gTasks[taskId].t2_BgX;
        gBattle_BG1_Y = y + gTasks[taskId].t2_BgY;
        CpuCopy32(&gPlttBufferFaded[OBJ_PLTT_ID(battlerId)], &gPlttBufferFaded[BG_PLTT_ID(animBg.paletteId)], PLTT_SIZE_4BPP);
    }
    else
    {
        gBattle_BG2_X = x + gTasks[taskId].t2_BgX;
        gBattle_BG2_Y = y + gTasks[taskId].t2_BgY;
        CpuCopy32(&gPlttBufferFaded[OBJ_PLTT_ID(battlerId)], &gPlttBufferFaded[BG_PLTT_ID(9)], PLTT_SIZE_4BPP);
    }
}

#undef tBattlerId
#undef tInBg2
#undef tActive
#undef tIsPartner

#undef t2_SpriteId
#undef t2_SpriteX
#undef t2_SpriteY
#undef t2_BgX
#undef t2_BgY
#undef t2_InBg2
#undef t2_BattlerId

static void Cmd_clearmonbg(void)
{
    u8 animBattlerId;
    u8 battlerId;
    u8 taskId;

    sBattleAnimScriptPtr++;
    animBattlerId = sBattleAnimScriptPtr[0];

    if (animBattlerId == ANIM_ATTACKER)
        animBattlerId = ANIM_ATK_PARTNER;
    else if (animBattlerId == ANIM_TARGET)
        animBattlerId = ANIM_DEF_PARTNER;

    if (animBattlerId == ANIM_ATTACKER || animBattlerId == ANIM_ATK_PARTNER)
        battlerId = gBattleAnimAttacker;
    else
        battlerId = gBattleAnimTarget;

    if (sMonAnimTaskIdArray[0] != TASK_NONE)
        gSprites[gBattlerSpriteIds[battlerId]].invisible = FALSE;
    if (animBattlerId > 1 && sMonAnimTaskIdArray[1] != TASK_NONE)
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(battlerId)]].invisible = FALSE;
    else
        animBattlerId = 0;

    taskId = CreateTask(Task_ClearMonBg, 5);
    gTasks[taskId].data[0] = animBattlerId;
    gTasks[taskId].data[2] = battlerId;

    sBattleAnimScriptPtr++;
}

static void Task_ClearMonBg(u8 taskId)
{
    gTasks[taskId].data[1]++;
    if (gTasks[taskId].data[1] != 1)
    {
        u8 to_BG2;
        u8 position = GetBattlerPosition(gTasks[taskId].data[2]);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            to_BG2 = FALSE;
        else
            to_BG2 = TRUE;

        if (sMonAnimTaskIdArray[0] != TASK_NONE)
        {
            ResetBattleAnimBg(to_BG2);
            DestroyTask(sMonAnimTaskIdArray[0]);
            sMonAnimTaskIdArray[0] = TASK_NONE;
        }
        if (gTasks[taskId].data[0] > 1)
        {
            ResetBattleAnimBg(to_BG2 ^ 1);
            DestroyTask(sMonAnimTaskIdArray[1]);
            sMonAnimTaskIdArray[1] = TASK_NONE;
        }
        DestroyTask(taskId);
    }
}

// Equivalent to Cmd_monbg but never creates Task_InitUpdateMonBg / Task_UpdateMonBg
static void Cmd_monbg_static(void)
{
    bool8 toBG_2;
    u8 battlerId;
    u8 animBattlerId;

    sBattleAnimScriptPtr++;

    animBattlerId = sBattleAnimScriptPtr[0];

    if (animBattlerId == ANIM_ATTACKER)
        animBattlerId = ANIM_ATK_PARTNER;
    else if (animBattlerId == ANIM_TARGET)
        animBattlerId = ANIM_DEF_PARTNER;

    if (animBattlerId == ANIM_ATTACKER || animBattlerId == ANIM_ATK_PARTNER)
        battlerId = gBattleAnimAttacker;
    else
        battlerId = gBattleAnimTarget;

    if (IsBattlerSpriteVisible(battlerId))
    {
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            toBG_2 = FALSE;
        else
            toBG_2 = TRUE;

        MoveBattlerSpriteToBG(battlerId, toBG_2, FALSE);
    }

    battlerId ^= BIT_FLANK;
    if (animBattlerId > 1 && IsBattlerSpriteVisible(battlerId))
    {
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            toBG_2 = FALSE;
        else
            toBG_2 = TRUE;

        MoveBattlerSpriteToBG(battlerId, toBG_2, FALSE);
    }

    sBattleAnimScriptPtr++;
}

static void Cmd_clearmonbg_static(void)
{
    u8 animBattlerId;
    u8 battlerId;
    u8 taskId;

    sBattleAnimScriptPtr++;
    animBattlerId = sBattleAnimScriptPtr[0];

    if (animBattlerId == ANIM_ATTACKER)
        animBattlerId = ANIM_ATK_PARTNER;
    else if (animBattlerId == ANIM_TARGET)
        animBattlerId = ANIM_DEF_PARTNER;

    if (animBattlerId == ANIM_ATTACKER || animBattlerId == ANIM_ATK_PARTNER)
        battlerId = gBattleAnimAttacker;
    else
        battlerId = gBattleAnimTarget;

    if (IsBattlerSpriteVisible(battlerId))
        gSprites[gBattlerSpriteIds[battlerId]].invisible = FALSE;
    if (animBattlerId > 1 && IsBattlerSpriteVisible(BATTLE_PARTNER(battlerId)))
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(battlerId)]].invisible = FALSE;
    else
        animBattlerId = 0;

    taskId = CreateTask(Task_ClearMonBgStatic, 5);
    gTasks[taskId].data[0] = animBattlerId;
    gTasks[taskId].data[2] = battlerId;

    sBattleAnimScriptPtr++;
}

static void Task_ClearMonBgStatic(u8 taskId)
{
    gTasks[taskId].data[1]++;
    if (gTasks[taskId].data[1] != 1)
    {
        bool8 toBG_2;
        u8 battlerId = gTasks[taskId].data[2];
        u8 position = GetBattlerPosition(battlerId);
        if (position == B_POSITION_OPPONENT_LEFT || position == B_POSITION_PLAYER_RIGHT || IsContest())
            toBG_2 = FALSE;
        else
            toBG_2 = TRUE;

        if (IsBattlerSpriteVisible(battlerId))
            ResetBattleAnimBg(toBG_2);
        if (gTasks[taskId].data[0] > 1 && IsBattlerSpriteVisible(BATTLE_PARTNER(battlerId)))
            ResetBattleAnimBg(toBG_2 ^ 1);

        DestroyTask(taskId);
    }
}

static void Cmd_setalpha(void)
{
    u16 half1, half2;

    sBattleAnimScriptPtr++;
    half1 = *(sBattleAnimScriptPtr++);
    half2 = *(sBattleAnimScriptPtr++) << 8;
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
    SetGpuReg(REG_OFFSET_BLDALPHA, half1 | half2);
}

static void Cmd_setbldcnt(void)
{
    u16 half1, half2;

    sBattleAnimScriptPtr++;
    half1 = *(sBattleAnimScriptPtr++);
    half2 = *(sBattleAnimScriptPtr++) << 8;
    SetGpuReg(REG_OFFSET_BLDCNT, half1 | half2);
}

static void Cmd_blendoff(void)
{
    sBattleAnimScriptPtr++;
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
}

static void Cmd_call(void)
{
    sBattleAnimScriptPtr++;
    sBattleAnimScriptRetAddr = sBattleAnimScriptPtr + 4;
    sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr);
}

static void Cmd_return(void)
{
    sBattleAnimScriptPtr = sBattleAnimScriptRetAddr;
}

static void Cmd_setarg(void)
{
    // Save original address to return to
    // after the T1_READ_16, + 4.
    // They could have equivalently just advanced
    // sBattleAnimScriptPtr by 2 afterwards.
    const u8 *addr = sBattleAnimScriptPtr;
    u16 value;
    u8 argId;

    sBattleAnimScriptPtr++;
    argId = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;
    value = T1_READ_16(sBattleAnimScriptPtr);
    sBattleAnimScriptPtr = addr + 4;
    gBattleAnimArgs[argId] = value;
}

static void Cmd_choosetwoturnanim(void)
{
    sBattleAnimScriptPtr++;
    if (gAnimMoveTurn & 1)
        sBattleAnimScriptPtr += 4;
    sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr);
}

static void Cmd_jumpifmoveturn(void)
{
    u8 toCheck;
    sBattleAnimScriptPtr++;
    toCheck = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;

    if (toCheck == gAnimMoveTurn)
        sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr);
    else
        sBattleAnimScriptPtr += 4;
}

static void Cmd_goto(void)
{
    sBattleAnimScriptPtr++;
    sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr);
}

// Uses of this function that rely on a TRUE return are expecting inBattle to not be ticked as defined in contest behavior.
// As a result, if misused, this function cannot reliably discern between field and contest status and could result in undefined behavior.
bool8 IsContest(void)
{
    if (!gMain.inBattle)
        return TRUE;
    else
        return FALSE;
}

#define tBackgroundId   data[0]
#define tState          data[10]

static void Cmd_fadetobg(void)
{
    u8 backgroundId;
    u8 taskId;

    sBattleAnimScriptPtr++;
    backgroundId = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;
    taskId = CreateTask(Task_FadeToBg, 5);
    gTasks[taskId].tBackgroundId = backgroundId;
    sAnimBackgroundFadeState = 1;
}

static void Cmd_fadetobgfromset(void)
{
    u8 bg1, bg2, bg3;
    u8 taskId;

    sBattleAnimScriptPtr++;
    bg1 = sBattleAnimScriptPtr[0];
    bg2 = sBattleAnimScriptPtr[1];
    bg3 = sBattleAnimScriptPtr[2];
    sBattleAnimScriptPtr += 3;
    taskId = CreateTask(Task_FadeToBg, 5);

    if (IsContest())
        gTasks[taskId].tBackgroundId = bg3;
    else if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
        gTasks[taskId].tBackgroundId = bg2;
    else
        gTasks[taskId].tBackgroundId = bg1;

    sAnimBackgroundFadeState = 1;
}

static void Task_FadeToBg(u8 taskId)
{
    if (gTasks[taskId].tState == 0)
    {
        BeginHardwarePaletteFade(0xE8, 0, 0, 16, 0);
        gTasks[taskId].tState++;
        return;
    }
    if (gPaletteFade.active)
        return;
    if (gTasks[taskId].tState == 1)
    {
        gTasks[taskId].tState++;
        sAnimBackgroundFadeState = 2;
    }
    else if (gTasks[taskId].tState == 2)
    {
        s16 bgId = gTasks[taskId].tBackgroundId;

        if (bgId == -1)
            LoadDefaultBg();
        else
            LoadMoveBg(bgId);

        BeginHardwarePaletteFade(0xE8, 0, 16, 0, 1);
        gTasks[taskId].tState++;
        return;
    }
    if (gPaletteFade.active)
        return;
    if (gTasks[taskId].tState == 3)
    {
        DestroyTask(taskId);
        sAnimBackgroundFadeState = 0;
    }
}

static void LoadMoveBg(u16 bgId)
{
    if (IsContest())
    {
        const u32 *tilemap = gBattleAnimBackgroundTable[bgId].tilemap;
        void *dmaSrc;
        void *dmaDest;

        LZDecompressWram(tilemap, gDecompressionBuffer);
        RelocateBattleBgPal(GetBattleBgPaletteNum(), (void *)gDecompressionBuffer, 0x100, FALSE);
        dmaSrc = gDecompressionBuffer;
        dmaDest = (void *)BG_SCREEN_ADDR(26);
        DmaCopy32(3, dmaSrc, dmaDest, 0x800);
        LZDecompressVram(gBattleAnimBackgroundTable[bgId].image, (void *)BG_SCREEN_ADDR(4));
        LoadCompressedPalette(gBattleAnimBackgroundTable[bgId].palette, BG_PLTT_ID(GetBattleBgPaletteNum()), PLTT_SIZE_4BPP);
    }
    else
    {
        LZDecompressVram(gBattleAnimBackgroundTable[bgId].tilemap, (void *)BG_SCREEN_ADDR(26));
        LZDecompressVram(gBattleAnimBackgroundTable[bgId].image, (void *)BG_CHAR_ADDR(2));
        LoadCompressedPalette(gBattleAnimBackgroundTable[bgId].palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
    }
}

static void LoadDefaultBg(void)
{
    if (IsContest())
        LoadContestBgAfterMoveAnim();
    else
        DrawMainBattleBackground();
}

static void Cmd_restorebg(void)
{
    u8 taskId;

    sBattleAnimScriptPtr++;
    taskId = CreateTask(Task_FadeToBg, 5);
    gTasks[taskId].tBackgroundId = -1;
    sAnimBackgroundFadeState = 1;
}

#undef tBackgroundId
#undef tState

static void Cmd_waitbgfadeout(void)
{
    if (sAnimBackgroundFadeState == 2)
    {
        sBattleAnimScriptPtr++;
        sAnimFramesToWait = 0;
    }
    else
    {
        sAnimFramesToWait = 1;
    }
}

static void Cmd_waitbgfadein(void)
{
    if (sAnimBackgroundFadeState == 0)
    {
        sBattleAnimScriptPtr++;
        sAnimFramesToWait = 0;
    }
    else
    {
        sAnimFramesToWait = 1;
    }
}

static void Cmd_changebg(void)
{
    sBattleAnimScriptPtr++;
    LoadMoveBg(sBattleAnimScriptPtr[0]);
    sBattleAnimScriptPtr++;
}

s8 BattleAnimAdjustPanning(s8 pan)
{
    if (!IsContest() && gBattleSpritesDataPtr->healthBoxesData[gBattleAnimAttacker].statusAnimActive)
    {
        if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
            pan = SOUND_PAN_TARGET;
        else
            pan = SOUND_PAN_ATTACKER;
    }
    else if (IsContest())
    {
        if (gBattleAnimAttacker != gBattleAnimTarget || gBattleAnimAttacker != 2 || pan != SOUND_PAN_TARGET)
            pan *= -1;
    }
    else if (GetBattlerSide(gBattleAnimAttacker) == B_SIDE_PLAYER)
    {
        if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
        {
            if (pan == SOUND_PAN_TARGET)
                pan = SOUND_PAN_ATTACKER;
            else if (pan != SOUND_PAN_ATTACKER)
                pan *= -1;
        }
    }
    else if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_OPPONENT)
    {
        if (pan == SOUND_PAN_ATTACKER)
            pan = SOUND_PAN_TARGET;
    }
    else
    {
        pan *= -1;
    }

    if (pan > SOUND_PAN_TARGET)
        pan = SOUND_PAN_TARGET;
    else if (pan < SOUND_PAN_ATTACKER)
        pan = SOUND_PAN_ATTACKER;

    return pan;
}

s8 BattleAnimAdjustPanning2(s8 pan)
{
    if (!IsContest() && gBattleSpritesDataPtr->healthBoxesData[gBattleAnimAttacker].statusAnimActive)
    {
        if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
            pan = SOUND_PAN_TARGET;
        else
            pan = SOUND_PAN_ATTACKER;
    }
    else
    {
        if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER || IsContest())
            pan = -pan;
    }
    return pan;
}

s16 KeepPanInRange(s16 pan)
{
    if (pan > SOUND_PAN_TARGET)
        pan = SOUND_PAN_TARGET;
    else if (pan < SOUND_PAN_ATTACKER)
        pan = SOUND_PAN_ATTACKER;

    return pan;
}

s16 CalculatePanIncrement(s16 sourcePan, s16 targetPan, s16 incrementPan)
{
    s16 ret;

    if (sourcePan < targetPan)
        ret = abs(incrementPan);
    else if (sourcePan > targetPan)
        ret = -abs(incrementPan);
    else
        ret = 0;

    return ret;
}

static void Cmd_playsewithpan(void)
{
    u16 songId;
    s8 pan;

    sBattleAnimScriptPtr++;
    songId = T1_READ_16(sBattleAnimScriptPtr);
    pan = sBattleAnimScriptPtr[2];
    PlaySE12WithPanning(songId, BattleAnimAdjustPanning(pan));
    sBattleAnimScriptPtr += 3;
}

static void Cmd_setpan(void)
{
    s8 pan;

    sBattleAnimScriptPtr++;
    pan = sBattleAnimScriptPtr[0];
    SE12PanpotControl(BattleAnimAdjustPanning(pan));
    sBattleAnimScriptPtr++;
}

#define tInitialPan     data[0]
#define tTargetPan      data[1]
#define tIncrementPan   data[2]
#define tFramesToWait   data[3]
#define tCurrentPan     data[4]
#define tFrameCounter   data[8]

static void Cmd_panse(void)
{
    u16 songNum;
    s8 currentPanArg, incrementPan, incrementPanArg, currentPan, targetPan;
    u8 framesToWait;
    u8 taskId;

    sBattleAnimScriptPtr++;
    songNum = T1_READ_16(sBattleAnimScriptPtr);
    currentPanArg = sBattleAnimScriptPtr[2];
    incrementPan = sBattleAnimScriptPtr[3]; // targetPan, var is re-used
    incrementPanArg = sBattleAnimScriptPtr[4];
    framesToWait = sBattleAnimScriptPtr[5];

    currentPan = BattleAnimAdjustPanning(currentPanArg);
    targetPan = BattleAnimAdjustPanning(incrementPan);
    incrementPan = CalculatePanIncrement(currentPan, targetPan, incrementPanArg);

    taskId = CreateTask(Task_PanFromInitialToTarget, 1);
    gTasks[taskId].tInitialPan = currentPan;
    gTasks[taskId].tTargetPan = targetPan;
    gTasks[taskId].tIncrementPan = incrementPan;
    gTasks[taskId].tFramesToWait = framesToWait;
    gTasks[taskId].tCurrentPan = currentPan;

    PlaySE12WithPanning(songNum, currentPan);

    gAnimSoundTaskCount++;
    sBattleAnimScriptPtr += 6;
}

void Task_PanFromInitialToTarget(u8 taskId)
{
    bool32 destroyTask = FALSE;
    if (gTasks[taskId].tFrameCounter++ >= gTasks[taskId].tFramesToWait)
    {
        s16 pan;
        s16 initialPanning, targetPanning, currentPan, incrementPan;

        gTasks[taskId].tFrameCounter = 0;
        initialPanning = gTasks[taskId].tInitialPan;
        targetPanning = gTasks[taskId].tTargetPan;
        currentPan = gTasks[taskId].tCurrentPan;
        incrementPan = gTasks[taskId].tIncrementPan;
        pan = currentPan + incrementPan;
        gTasks[taskId].tCurrentPan = pan;

        if (incrementPan == 0) // If we're not incrementing, just cancel the task immediately.
        {
            destroyTask = TRUE;
        }
        else if (initialPanning < targetPanning) // Panning increasing.
        {
            if (pan >= targetPanning) // Target reached.
                destroyTask = TRUE;
        }
        else // Panning decreasing.
        {
            if (pan <= targetPanning) // Target reached.
                destroyTask = TRUE;
        }

        if (destroyTask)
        {
            pan = targetPanning;
            DestroyTask(taskId);
            gAnimSoundTaskCount--;
        }

        SE12PanpotControl(pan);
    }
}

static void Cmd_panse_adjustnone(void)
{
    u16 songId;
    s8 currentPan, targetPan, incrementPan;
    u8 framesToWait;
    u8 taskId;

    sBattleAnimScriptPtr++;
    songId = T1_READ_16(sBattleAnimScriptPtr);
    currentPan = sBattleAnimScriptPtr[2];
    targetPan = sBattleAnimScriptPtr[3];
    incrementPan = sBattleAnimScriptPtr[4];
    framesToWait = sBattleAnimScriptPtr[5];

    taskId = CreateTask(Task_PanFromInitialToTarget, 1);
    gTasks[taskId].tInitialPan = currentPan;
    gTasks[taskId].tTargetPan = targetPan;
    gTasks[taskId].tIncrementPan = incrementPan;
    gTasks[taskId].tFramesToWait = framesToWait;
    gTasks[taskId].tCurrentPan = currentPan;

    PlaySE12WithPanning(songId, currentPan);

    gAnimSoundTaskCount++;
    sBattleAnimScriptPtr += 6;
}

static void Cmd_panse_adjustall(void)
{
    u16 songId;
    s8 targetPanArg, incrementPanArg, currentPanArg, currentPan, targetPan, incrementPan;
    u8 framesToWait;
    u8 taskId;

    sBattleAnimScriptPtr++;
    songId = T1_READ_16(sBattleAnimScriptPtr);
    currentPanArg = sBattleAnimScriptPtr[2];
    targetPanArg = sBattleAnimScriptPtr[3];
    incrementPanArg = sBattleAnimScriptPtr[4];
    framesToWait = sBattleAnimScriptPtr[5];

    currentPan = BattleAnimAdjustPanning2(currentPanArg);
    targetPan = BattleAnimAdjustPanning2(targetPanArg);
    incrementPan = BattleAnimAdjustPanning2(incrementPanArg);

    taskId = CreateTask(Task_PanFromInitialToTarget, 1);
    gTasks[taskId].tInitialPan = currentPan;
    gTasks[taskId].tTargetPan = targetPan;
    gTasks[taskId].tIncrementPan = incrementPan;
    gTasks[taskId].tFramesToWait = framesToWait;
    gTasks[taskId].tCurrentPan = currentPan;

    PlaySE12WithPanning(songId, currentPan);

    gAnimSoundTaskCount++;
    sBattleAnimScriptPtr += 6;
}

#undef tInitialPan
#undef tTargetPan
#undef tIncrementPan
#undef tFramesToWait
#undef tCurrentPan
#undef tFrameCounter

#define tSongId         data[0]
#define tPanning        data[1]
#define tFramesToWait   data[2]
#define tNumberOfPlays  data[3]
#define tFrameCounter   data[8]

static void Cmd_loopsewithpan(void)
{
    u16 songId;
    s8 panningArg, panning;
    u8 framesToWait, numberOfPlays;
    u8 taskId;

    sBattleAnimScriptPtr++;
    songId = T1_READ_16(sBattleAnimScriptPtr);
    panningArg = sBattleAnimScriptPtr[2];
    framesToWait = sBattleAnimScriptPtr[3];
    numberOfPlays = sBattleAnimScriptPtr[4];
    panning = BattleAnimAdjustPanning(panningArg);

    taskId = CreateTask(Task_LoopAndPlaySE, 1);
    gTasks[taskId].tSongId = songId;
    gTasks[taskId].tPanning = panning;
    gTasks[taskId].tFramesToWait = framesToWait;
    gTasks[taskId].tNumberOfPlays = numberOfPlays;
    gTasks[taskId].tFrameCounter = framesToWait;
    gTasks[taskId].func(taskId);

    gAnimSoundTaskCount++;
    sBattleAnimScriptPtr += 5;
}

static void Task_LoopAndPlaySE(u8 taskId)
{
    if (gTasks[taskId].tFrameCounter++ >= gTasks[taskId].tFramesToWait)
    {
        u16 songId;
        s8 panning;
        u8 numberOfPlays;

        gTasks[taskId].tFrameCounter = 0;
        songId = gTasks[taskId].tSongId;
        panning = gTasks[taskId].tPanning;
        numberOfPlays = --gTasks[taskId].tNumberOfPlays;
        PlaySE12WithPanning(songId, panning);
        if (numberOfPlays == 0)
        {
            DestroyTask(taskId);
            gAnimSoundTaskCount--;
        }
    }
}

#undef tSongId
#undef tPanning
#undef tFramesToWait
#undef tNumberOfPlays
#undef tFrameCounter

#define tSongId         data[0]
#define tPanning        data[1]
#define tFramesToWait   data[2]

static void Cmd_waitplaysewithpan(void)
{
    u16 songId;
    s8 panningArg, panning;
    u8 framesToWait;
    u8 taskId;

    sBattleAnimScriptPtr++;
    songId = T1_READ_16(sBattleAnimScriptPtr);
    panningArg = sBattleAnimScriptPtr[2];
    framesToWait = sBattleAnimScriptPtr[3];
    panning = BattleAnimAdjustPanning(panningArg);

    taskId = CreateTask(Task_WaitAndPlaySE, 1);
    gTasks[taskId].tSongId = songId;
    gTasks[taskId].tPanning = panning;
    gTasks[taskId].tFramesToWait = framesToWait;

    gAnimSoundTaskCount++;
    sBattleAnimScriptPtr += 4;
}

static void Task_WaitAndPlaySE(u8 taskId)
{
    if (gTasks[taskId].tFramesToWait-- <= 0)
    {
        PlaySE12WithPanning(gTasks[taskId].tSongId, gTasks[taskId].tPanning);
        DestroyTask(taskId);
        gAnimSoundTaskCount--;
    }
}

#undef tSongId
#undef tPanning
#undef tFramesToWait

static void Cmd_createsoundtask(void)
{
    TaskFunc func;
    u8 numArgs, taskId;
    s32 i;

    sBattleAnimScriptPtr++;
    func = (TaskFunc)T2_READ_32(sBattleAnimScriptPtr);
    sBattleAnimScriptPtr += 4;
    numArgs = sBattleAnimScriptPtr[0];
    sBattleAnimScriptPtr++;
    for (i = 0; i < numArgs; i++)
    {
        gBattleAnimArgs[i] = T1_READ_16(sBattleAnimScriptPtr);
        sBattleAnimScriptPtr += 2;
    }
    taskId = CreateTask(func, 1);
    func(taskId);
    gAnimSoundTaskCount++;
}

static void Cmd_waitsound(void)
{
    if (gAnimSoundTaskCount != 0)
    {
        sSoundAnimFramesToWait = 0;
        sAnimFramesToWait = 1;
    }
    else if (IsSEPlaying())
    {
        if (++sSoundAnimFramesToWait > 90)
        {
            m4aMPlayStop(&gMPlayInfo_SE1);
            m4aMPlayStop(&gMPlayInfo_SE2);
            sSoundAnimFramesToWait = 0;
        }
        else
        {
            sAnimFramesToWait = 1;
        }
    }
    else
    {
        sSoundAnimFramesToWait = 0;
        sBattleAnimScriptPtr++;
        sAnimFramesToWait = 0;
    }
}

static void Cmd_jumpargeq(void)
{
    u8 argId;
    s16 valueToCheck;

    sBattleAnimScriptPtr++;
    argId = sBattleAnimScriptPtr[0];
    valueToCheck = T1_READ_16(sBattleAnimScriptPtr + 1);

    if (valueToCheck == gBattleAnimArgs[argId])
        sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr + 3);
    else
        sBattleAnimScriptPtr += 7;
}

static void Cmd_jumpifcontest(void)
{
    sBattleAnimScriptPtr++;
    if (IsContest())
        sBattleAnimScriptPtr = T2_READ_PTR(sBattleAnimScriptPtr);
    else
        sBattleAnimScriptPtr += 4;
}

static void Cmd_splitbgprio(void)
{
    u8 wantedBattler;
    u8 battlerId;
    u8 battlerPosition;

    wantedBattler = sBattleAnimScriptPtr[1];
    sBattleAnimScriptPtr += 2;

    if (wantedBattler != ANIM_ATTACKER)
        battlerId = gBattleAnimTarget;
    else
        battlerId = gBattleAnimAttacker;

    // Apply only if the given battler is the lead (on left from team's perspective)
    battlerPosition = GetBattlerPosition(battlerId);
    if (!IsContest() && (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_OPPONENT_RIGHT))
    {
        SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 1);
        SetAnimBgAttribute(2, BG_ANIM_PRIORITY, 2);
    }
}

static void Cmd_splitbgprio_all(void)
{
    sBattleAnimScriptPtr++;
    if (!IsContest())
    {
        SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 1);
        SetAnimBgAttribute(2, BG_ANIM_PRIORITY, 2);
    }
}

static void Cmd_splitbgprio_foes(void)
{
    u8 wantedBattler;
    u8 battlerPosition;
    u8 battlerId;

    wantedBattler = sBattleAnimScriptPtr[1];
    sBattleAnimScriptPtr += 2;

    // Apply only if the attacking the opposing side
    if (GetBattlerSide(gBattleAnimAttacker) != GetBattlerSide(gBattleAnimTarget))
    {
        if (wantedBattler != ANIM_ATTACKER)
            battlerId = gBattleAnimTarget;
        else
            battlerId = gBattleAnimAttacker;

        // Apply only if the given battler is the lead (on left from team's perspective)
        battlerPosition = GetBattlerPosition(battlerId);
        if (!IsContest() && (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_OPPONENT_RIGHT))
        {
            SetAnimBgAttribute(1, BG_ANIM_PRIORITY, 1);
            SetAnimBgAttribute(2, BG_ANIM_PRIORITY, 2);
        }
    }
}

static void Cmd_invisible(void)
{
    u8 spriteId;

    spriteId = GetAnimBattlerSpriteId(sBattleAnimScriptPtr[1]);
    if (spriteId != SPRITE_NONE)
        gSprites[spriteId].invisible = TRUE;

    sBattleAnimScriptPtr += 2;
}

static void Cmd_visible(void)
{
    u8 spriteId;

    spriteId = GetAnimBattlerSpriteId(sBattleAnimScriptPtr[1]);
    if (spriteId != SPRITE_NONE)
        gSprites[spriteId].invisible = FALSE;

    sBattleAnimScriptPtr += 2;
}

// Below two commands are never used
static void Cmd_teamattack_moveback(void)
{
    u8 wantedBattler;
    u8 priorityRank;
    u8 spriteId;

    wantedBattler = sBattleAnimScriptPtr[1];
    sBattleAnimScriptPtr += 2;

    // Apply to double battles when attacking own side
    if (!IsContest() && IsDoubleBattle()
     && GetBattlerSide(gBattleAnimAttacker) == GetBattlerSide(gBattleAnimTarget))
    {
        if (wantedBattler == ANIM_ATTACKER)
        {
            priorityRank = GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker);
            spriteId = GetAnimBattlerSpriteId(ANIM_ATTACKER);
        }
        else
        {
            priorityRank = GetBattlerSpriteBGPriorityRank(gBattleAnimTarget);
            spriteId = GetAnimBattlerSpriteId(ANIM_TARGET);
        }
        if (spriteId != SPRITE_NONE)
        {
            gSprites[spriteId].invisible = FALSE;
            if (priorityRank == 2)
                gSprites[spriteId].oam.priority = 3;

            if (priorityRank == 1)
                ResetBattleAnimBg(FALSE);
            else
                ResetBattleAnimBg(TRUE);
        }
    }
}

static void Cmd_teamattack_movefwd(void)
{
    u8 wantedBattler;
    u8 priorityRank;
    u8 spriteId;

    wantedBattler = sBattleAnimScriptPtr[1];
    sBattleAnimScriptPtr += 2;

    // Apply to double battles when attacking own side
    if (!IsContest() && IsDoubleBattle()
     && GetBattlerSide(gBattleAnimAttacker) == GetBattlerSide(gBattleAnimTarget))
    {
        if (wantedBattler == ANIM_ATTACKER)
        {
            priorityRank = GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker);
            spriteId = GetAnimBattlerSpriteId(ANIM_ATTACKER);
        }
        else
        {
            priorityRank = GetBattlerSpriteBGPriorityRank(gBattleAnimTarget);
            spriteId = GetAnimBattlerSpriteId(ANIM_TARGET);
        }

        if (spriteId != SPRITE_NONE && priorityRank == 2)
            gSprites[spriteId].oam.priority = 2;
    }
}

static void Cmd_stopsound(void)
{
    m4aMPlayStop(&gMPlayInfo_SE1);
    m4aMPlayStop(&gMPlayInfo_SE2);
    sBattleAnimScriptPtr++;
}
