#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "contest.h"
#include "sound.h"
#include "task.h"
#include "constants/battle_anim.h"

static void SoundTask_FireBlast_Step1(u8 taskId);
static void SoundTask_FireBlast_Step2(u8 taskId);
static void SoundTask_LoopSEAdjustPanning_Step(u8 taskId);
static void SoundTask_PlayDoubleCry_Step(u8 taskId);
static void SoundTask_PlayCryWithEcho_Step(u8 taskId);
static void SoundTask_AdjustPanningVar_Step(u8 taskId);

// Loops the specified sound effect and pans from the
// attacker to the target. The second specified sound effect
// is played at the very end. This task is effectively
// hardcoded to the move FIRE_BLAST due to the baked-in
// durations.
// arg 0: looped sound effect
// arg 1: ending sound effect
void SoundTask_FireBlast(u8 taskId)
{
    s8 pan1, pan2, panIncrement;

    gTasks[taskId].data[0] = gBattleAnimArgs[0];
    gTasks[taskId].data[1] = gBattleAnimArgs[1];

    pan1 = BattleAnimAdjustPanning(SOUND_PAN_ATTACKER);
    pan2 = BattleAnimAdjustPanning(SOUND_PAN_TARGET);
    panIncrement = CalculatePanIncrement(pan1, pan2, 2);

    gTasks[taskId].data[2] = pan1;
    gTasks[taskId].data[3] = pan2;
    gTasks[taskId].data[4] = panIncrement;
    gTasks[taskId].data[10] = 10;

    gTasks[taskId].func = SoundTask_FireBlast_Step1;
}

static void SoundTask_FireBlast_Step1(u8 taskId)
{
    s16 pan = gTasks[taskId].data[2];
    s8 panIncrement = gTasks[taskId].data[4];
    if (++gTasks[taskId].data[11] == 111)
    {
        gTasks[taskId].data[10] = 5;
        gTasks[taskId].data[11] = 0;
        gTasks[taskId].func = SoundTask_FireBlast_Step2;
    }
    else
    {
        if (++gTasks[taskId].data[10] == 11)
        {
            gTasks[taskId].data[10] = 0;
            PlaySE12WithPanning(gTasks[taskId].data[0], pan);
        }
        pan += panIncrement;
        gTasks[taskId].data[2] = KeepPanInRange(pan);
    }
}

static void SoundTask_FireBlast_Step2(u8 taskId)
{
    if (++gTasks[taskId].data[10] == 6)
    {
        s8 pan;

        gTasks[taskId].data[10] = 0;
        pan = BattleAnimAdjustPanning(SOUND_PAN_TARGET);
        PlaySE12WithPanning(gTasks[taskId].data[1], pan);
        if (++gTasks[taskId].data[11] == 2)
            DestroyAnimSoundTask(taskId);
    }
}

void SoundTask_LoopSEAdjustPanning(u8 taskId)
{
    u16 songId = gBattleAnimArgs[0];
    s8 sourcePan = gBattleAnimArgs[1];
    s8 targetPan = gBattleAnimArgs[2];
    s8 panIncrement = gBattleAnimArgs[3];
    u8 r10 = gBattleAnimArgs[4];
    u8 r7 = gBattleAnimArgs[5];
    u8 r9 = gBattleAnimArgs[6];

    sourcePan = BattleAnimAdjustPanning(sourcePan);
    targetPan = BattleAnimAdjustPanning(targetPan);
    panIncrement = CalculatePanIncrement(sourcePan, targetPan, panIncrement);

    gTasks[taskId].data[0] = songId;
    gTasks[taskId].data[1] = sourcePan;
    gTasks[taskId].data[2] = targetPan;
    gTasks[taskId].data[3] = panIncrement;
    gTasks[taskId].data[4] = r10;
    gTasks[taskId].data[5] = r7;
    gTasks[taskId].data[6] = r9;
    gTasks[taskId].data[10] = 0;
    gTasks[taskId].data[11] = sourcePan;
    gTasks[taskId].data[12] = r9;

    gTasks[taskId].func = SoundTask_LoopSEAdjustPanning_Step;
    gTasks[taskId].func(taskId);
}

static void SoundTask_LoopSEAdjustPanning_Step(u8 taskId)
{
    s8 unusedVar =  gTasks[taskId].data[3];

    if (gTasks[taskId].data[12]++ == gTasks[taskId].data[6])
    {
        gTasks[taskId].data[12] = 0;
        PlaySE12WithPanning(gTasks[taskId].data[0], gTasks[taskId].data[11]);
        if (--gTasks[taskId].data[4] == 0)
        {
            DestroyAnimSoundTask(taskId);
            return;
        }
    }

    if (gTasks[taskId].data[10]++ == gTasks[taskId].data[5])
    {
        gTasks[taskId].data[10] = 0;
        gTasks[taskId].data[11] += gTasks[taskId].data[3];
        gTasks[taskId].data[11] = KeepPanInRange(gTasks[taskId].data[11]);
    }
}

void SoundTask_PlayCryHighPitch(u8 taskId)
{
    u16 species = 0;
    s8 pan = BattleAnimAdjustPanning(SOUND_PAN_ATTACKER);
    if (IsContest())
    {
        if (gBattleAnimArgs[0] == ANIM_ATTACKER)
            species = gContestResources->moveAnim->species;
    // Destroying the task twice (here and at end of function)
    // results in an incorrect value for gAnimVisualTaskCount
    #ifndef BUGFIX
        else
            DestroyAnimVisualTask(taskId);
    #endif
    }
    else
    {
        u8 battlerId;

        // Get wanted battler.
        if (gBattleAnimArgs[0] == ANIM_ATTACKER)
            battlerId = gBattleAnimAttacker;
        else if (gBattleAnimArgs[0] == ANIM_TARGET)
            battlerId = gBattleAnimTarget;
        else if (gBattleAnimArgs[0] == ANIM_ATK_PARTNER)
            battlerId = BATTLE_PARTNER(gBattleAnimAttacker);
        else
            battlerId = BATTLE_PARTNER(gBattleAnimTarget);

        // Check if battler is visible.
        if ((gBattleAnimArgs[0] == ANIM_TARGET || gBattleAnimArgs[0] == ANIM_DEF_PARTNER) && !IsBattlerSpriteVisible(battlerId))
        {
            DestroyAnimVisualTask(taskId);
            return;
        }

        if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
        else
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
    }

    if (species != SPECIES_NONE)
        PlayCry_ByMode(species, pan, CRY_MODE_HIGH_PITCH);

    DestroyAnimVisualTask(taskId);
}

void SoundTask_PlayDoubleCry(u8 taskId)
{
    u16 species = 0;
    s8 pan = BattleAnimAdjustPanning(SOUND_PAN_ATTACKER);
    if (IsContest())
    {
        if (gBattleAnimArgs[0] == ANIM_ATTACKER)
            species = gContestResources->moveAnim->species;
    // Destroying the task twice (here and at end of function)
    // results in an incorrect value for gAnimVisualTaskCount
    #ifndef BUGFIX
        else
            DestroyAnimVisualTask(taskId);
    #endif
    }
    else
    {
        u8 battlerId;

        // Get wanted battler.
        if (gBattleAnimArgs[0] == ANIM_ATTACKER)
            battlerId = gBattleAnimAttacker;
        else if (gBattleAnimArgs[0] == ANIM_TARGET)
            battlerId = gBattleAnimTarget;
        else if (gBattleAnimArgs[0] == ANIM_ATK_PARTNER)
            battlerId = BATTLE_PARTNER(gBattleAnimAttacker);
        else
            battlerId = BATTLE_PARTNER(gBattleAnimTarget);

        // Check if battler is visible.
        if ((gBattleAnimArgs[0] == ANIM_TARGET || gBattleAnimArgs[0] == ANIM_DEF_PARTNER) && !IsBattlerSpriteVisible(battlerId))
        {
            DestroyAnimVisualTask(taskId);
            return;
        }

        if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
        else
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
    }

    gTasks[taskId].data[0] = gBattleAnimArgs[1];
    gTasks[taskId].data[1] = species;
    gTasks[taskId].data[2] = pan;

    if (species != SPECIES_NONE)
    {
        if (gBattleAnimArgs[1] == DOUBLE_CRY_GROWL)
            PlayCry_ByMode(species, pan, CRY_MODE_GROWL_1);
        else // DOUBLE_CRY_ROAR
            PlayCry_ByMode(species, pan, CRY_MODE_ROAR_1);

        gTasks[taskId].func = SoundTask_PlayDoubleCry_Step;
    }
    else
    {
        DestroyAnimVisualTask(taskId);
    }
}

static void SoundTask_PlayDoubleCry_Step(u8 taskId)
{
    u16 species = gTasks[taskId].data[1];
    s8 pan = gTasks[taskId].data[2];

    if (gTasks[taskId].data[9] < 2)
    {
        gTasks[taskId].data[9]++;
    }
    else
    {
        if (gTasks[taskId].data[0] == DOUBLE_CRY_GROWL)
        {
            if (!IsCryPlaying())
            {
                PlayCry_ByMode(species, pan, CRY_MODE_GROWL_2);
                DestroyAnimVisualTask(taskId);
            }
        }
        else // DOUBLE_CRY_ROAR
        {
            if (!IsCryPlaying())
            {
                PlayCry_ByMode(species, pan, CRY_MODE_ROAR_2);
                DestroyAnimVisualTask(taskId);
            }
        }
    }
}

void SoundTask_WaitForCry(u8 taskId)
{
    if (gTasks[taskId].data[9] < 2)
    {
        gTasks[taskId].data[9]++;
    }
    else
    {
        if (!IsCryPlaying())
            DestroyAnimVisualTask(taskId);
    }
}


#define tSpecies data[1]
#define tPan     data[2]
#define tState   data[9]
#define tLastCry data[10] // If it's not the last cry, don't try to restore the BGM, because another is coming

void SoundTask_PlayCryWithEcho(u8 taskId)
{
    u16 species;
    s8 pan;

    gTasks[taskId].tLastCry = gBattleAnimArgs[0];
    pan = BattleAnimAdjustPanning(SOUND_PAN_ATTACKER);

    if (IsContest())
        species = gContestResources->moveAnim->species;
    else
        species = gAnimBattlerSpecies[gBattleAnimAttacker];

    gTasks[taskId].tSpecies = species;
    gTasks[taskId].tPan = pan;

    if (species != SPECIES_NONE)
        gTasks[taskId].func = SoundTask_PlayCryWithEcho_Step;
    else
        DestroyAnimVisualTask(taskId);
}

static void SoundTask_PlayCryWithEcho_Step(u8 taskId)
{
    u16 species = gTasks[taskId].tSpecies;
    s8 pan = gTasks[taskId].tPan;

    // Note the cases are not in order of execution
    switch (gTasks[taskId].tState)
    {
    case 2:
        PlayCry_DuckNoRestore(species, pan, CRY_MODE_ECHO_START);
        gTasks[taskId].tState++;
        break;
    case 1:
    case 3:
    case 4:
        gTasks[taskId].tState++;
        break;
    case 5:
        if (IsCryPlaying())
            break;
    case 0:
        StopCryAndClearCrySongs();
        gTasks[taskId].tState++;
        break;
    default:
        if (!gTasks[taskId].tLastCry)
            PlayCry_DuckNoRestore(species, pan, CRY_MODE_ECHO_END);
        else
            PlayCry_ByMode(species, pan, CRY_MODE_ECHO_END);

        DestroyAnimVisualTask(taskId);
        break;
    }
}

#undef tSpecies
#undef tPan
#undef tState
#undef tLastCry

void SoundTask_PlaySE1WithPanning(u8 taskId)
{
    u16 songId = gBattleAnimArgs[0];
    s8 pan = BattleAnimAdjustPanning(gBattleAnimArgs[1]);

    PlaySE1WithPanning(songId, pan);
    DestroyAnimVisualTask(taskId);
}

void SoundTask_PlaySE2WithPanning(u8 taskId)
{
    u16 songId = gBattleAnimArgs[0];
    s8 pan = BattleAnimAdjustPanning(gBattleAnimArgs[1]);

    PlaySE2WithPanning(songId, pan);
    DestroyAnimVisualTask(taskId);
}

// Adjusts panning and assigns it to gAnimCustomPanning. Doesnt play sound.
// Used by Confuse Ray and Will-O-Wisp (see uses of gAnimCustomPanning)
void SoundTask_AdjustPanningVar(u8 taskId)
{
    s8 sourcePan = gBattleAnimArgs[0];
    s8 targetPan = gBattleAnimArgs[1];
    s8 panIncrement = gBattleAnimArgs[2];
    s16 r9 = gBattleAnimArgs[3];

    sourcePan = BattleAnimAdjustPanning(sourcePan);
    targetPan = BattleAnimAdjustPanning(targetPan);
    panIncrement = CalculatePanIncrement(sourcePan, targetPan, panIncrement);

    gTasks[taskId].data[1] = sourcePan;
    gTasks[taskId].data[2] = targetPan;
    gTasks[taskId].data[3] = panIncrement;
    gTasks[taskId].data[5] = r9;
    gTasks[taskId].data[10] = 0;
    gTasks[taskId].data[11] = sourcePan;

    gTasks[taskId].func = SoundTask_AdjustPanningVar_Step;
    gTasks[taskId].func(taskId);
}

static void SoundTask_AdjustPanningVar_Step(u8 taskId)
{
    s8 unusedVar =  gTasks[taskId].data[3];

    if (gTasks[taskId].data[10]++ == gTasks[taskId].data[5])
    {
        gTasks[taskId].data[10] = 0;
        gTasks[taskId].data[11] += gTasks[taskId].data[3];
        gTasks[taskId].data[11] = KeepPanInRange(gTasks[taskId].data[11]);
    }

    gAnimCustomPanning = gTasks[taskId].data[11];
    if (gTasks[taskId].data[11] == gTasks[taskId].data[2])
        DestroyAnimVisualTask(taskId);
}
