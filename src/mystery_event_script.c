#include "global.h"
#include "berry.h"
#include "battle_tower.h"
#include "easy_chat.h"
#include "event_data.h"
#include "mail.h"
#include "mystery_event_script.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_size_record.h"
#include "script.h"
#include "strings.h"
#include "string_util.h"
#include "text.h"
#include "util.h"
#include "mystery_event_msg.h"
#include "pokemon_storage_system.h"

extern ScrCmdFunc gMysteryEventScriptCmdTable[];
extern ScrCmdFunc gMysteryEventScriptCmdTableEnd[];

// 0x1 in FireRed, 0x2 in LeafGreen, 0x80 in Ruby, 0x100 in Sapphire
#define VERSION_MASK (1 << 9)

#define mScriptBase data[0]
#define mOffset data[1]
#define mStatus data[2]
#define mValid data[3]

EWRAM_DATA static struct ScriptContext sMysteryEventScriptContext = {0};

static bool32 CheckCompatibility(u16 unk0, u32 unk1, u16 unk2, u32 version)
{
    // 0x1 in English FRLG, 0x2 in English RS, 0x4 in German RS
    if (!(unk0 & 0x1))
        return FALSE;

    // Same as above
    if (!(unk1 & 0x1))
        return FALSE;

    // 0x1 in FRLG, 0x4 in RS
    if (!(unk2 & 0x4))
        return FALSE;

    if (!(version & VERSION_MASK))
        return FALSE;

    return TRUE;
}

static void SetIncompatible(void)
{
    StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventCantBeUsed);
    SetMysteryEventScriptStatus(MEVENT_STATUS_FAILURE);
}

static void InitMysteryEventScript(struct ScriptContext *ctx, u8 *script)
{
    InitScriptContext(ctx, gMysteryEventScriptCmdTable, gMysteryEventScriptCmdTableEnd);
    SetupBytecodeScript(ctx, script);
    ctx->mScriptBase = (u32)script;
    ctx->mOffset = 0;
    ctx->mStatus = MEVENT_STATUS_LOAD_OK;
    ctx->mValid = FALSE;
}

static bool32 RunMysteryEventScriptCommand(struct ScriptContext *ctx)
{
    if (RunScriptCommand(ctx) && ctx->mValid)
        return TRUE;
    else
        return FALSE;
}

void InitMysteryEventScriptContext(u8 *script)
{
    InitMysteryEventScript(&sMysteryEventScriptContext, script);
}

bool32 RunMysteryEventScriptContextCommand(u32 *status)
{
    bool32 ret = RunMysteryEventScriptCommand(&sMysteryEventScriptContext);
    *status = sMysteryEventScriptContext.mStatus;

    return ret;
}

u32 RunMysteryEventScript(u8 *script)
{
    struct ScriptContext *ctx = &sMysteryEventScriptContext;
    InitMysteryEventScript(ctx, script);
    while (RunMysteryEventScriptCommand(ctx));

    return ctx->mStatus;
}

void SetMysteryEventScriptStatus(u32 status)
{
    sMysteryEventScriptContext.mStatus = status;
}

static int CalcRecordMixingGiftChecksum(void)
{
    unsigned int i;
    int sum = 0;
    u8 *data = (u8 *)(&gSaveBlock1Ptr->recordMixingGift.data);

    for (i = 0; i < sizeof(gSaveBlock1Ptr->recordMixingGift.data); i++)
        sum += data[i];

    return sum;
}

static bool32 IsRecordMixingGiftValid(void)
{
    struct RecordMixingGiftData *data = &gSaveBlock1Ptr->recordMixingGift.data;
    int checksum = CalcRecordMixingGiftChecksum();

    if (data->unk0 == 0
        || data->quantity == 0
        || data->itemId == 0
        || checksum == 0
        || checksum != gSaveBlock1Ptr->recordMixingGift.checksum)
        return FALSE;
    else
        return TRUE;
}

static void ClearRecordMixingGift(void)
{
    CpuFill16(0, &gSaveBlock1Ptr->recordMixingGift, sizeof(gSaveBlock1Ptr->recordMixingGift));
}

static void SetRecordMixingGift(u8 unk, u8 quantity, u16 itemId)
{
    if (!unk || !quantity || !itemId)
    {
        ClearRecordMixingGift();
    }
    else
    {
        gSaveBlock1Ptr->recordMixingGift.data.unk0 = unk;
        gSaveBlock1Ptr->recordMixingGift.data.quantity = quantity;
        gSaveBlock1Ptr->recordMixingGift.data.itemId = itemId;
        gSaveBlock1Ptr->recordMixingGift.checksum = CalcRecordMixingGiftChecksum();
    }
}

u16 GetRecordMixingGift(void)
{
    struct RecordMixingGiftData *data = &gSaveBlock1Ptr->recordMixingGift.data;

    if (!IsRecordMixingGiftValid())
    {
        ClearRecordMixingGift();
        return 0;
    }
    else
    {
        u16 itemId = data->itemId;
        data->quantity--;
        if (data->quantity == 0)
            ClearRecordMixingGift();
        else
            gSaveBlock1Ptr->recordMixingGift.checksum = CalcRecordMixingGiftChecksum();

        return itemId;
    }
}

bool8 MEScrCmd_end(struct ScriptContext *ctx)
{
    StopScript(ctx);
    return TRUE;
}

bool8 MEScrCmd_checkcompat(struct ScriptContext *ctx)
{
    u16 unk0;
    u32 unk1;
    u16 unk2;
    u32 version;

    ctx->mOffset = ScriptReadWord(ctx);
    unk0 = ScriptReadHalfword(ctx);
    unk1 = ScriptReadWord(ctx);
    unk2 = ScriptReadHalfword(ctx);
    version = ScriptReadWord(ctx);

    if (CheckCompatibility(unk0, unk1, unk2, version) == TRUE)
        ctx->mValid = TRUE;
    else
        SetIncompatible();

    return TRUE;
}

bool8 MEScrCmd_nop(struct ScriptContext *ctx)
{
    return FALSE;
}

bool8 MEScrCmd_setstatus(struct ScriptContext *ctx)
{
    u8 status = ScriptReadByte(ctx);
    ctx->mStatus = status;
    return FALSE;
}

bool8 MEScrCmd_setmsg(struct ScriptContext *ctx)
{
    u8 status = ScriptReadByte(ctx);
    u8 *str = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    if (status == MEVENT_STATUS_FF || status == ctx->mStatus)
        StringExpandPlaceholders(gStringVarBuffer, str);
    return FALSE;
}

bool8 MEScrCmd_runscript(struct ScriptContext *ctx)
{
    u8 *script = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    RunScriptImmediately(script);
    return FALSE;
}

bool8 MEScrCmd_setenigmaberry(struct ScriptContext *ctx)
{
    u8 *str;
    const u8 *message;
    bool32 haveBerry = IsEnigmaBerryValid();
    u8 *berry = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    StringCopyN(gStringVar1, gSaveBlock1Ptr->enigmaBerry.berry.name, BERRY_NAME_LENGTH + 1);
    SetEnigmaBerry(berry);
    StringCopyN(gStringVar2, gSaveBlock1Ptr->enigmaBerry.berry.name, BERRY_NAME_LENGTH + 1);

    if (!haveBerry)
    {
        str = gStringVarBuffer;
        message = gText_MysteryEventBerry;
    }
    else if (StringCompare(gStringVar1, gStringVar2))
    {
        str = gStringVarBuffer;
        message = gText_MysteryEventBerryTransform;
    }
    else
    {
        str = gStringVarBuffer;
        message = gText_MysteryEventBerryObtained;
    }

    StringExpandPlaceholders(str, message);

    ctx->mStatus = MEVENT_STATUS_SUCCESS;

    if (IsEnigmaBerryValid() == TRUE)
        VarSet(VAR_ENIGMA_BERRY_AVAILABLE, 1);
    else
        ctx->mStatus = MEVENT_STATUS_LOAD_ERROR;

    return FALSE;
}

bool8 MEScrCmd_giveribbon(struct ScriptContext *ctx)
{
    u8 index = ScriptReadByte(ctx);
    u8 ribbonId = ScriptReadByte(ctx);
    GiveGiftRibbonToParty(index, ribbonId);
    StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventSpecialRibbon);
    ctx->mStatus = MEVENT_STATUS_SUCCESS;
    return FALSE;
}

bool8 MEScrCmd_initramscript(struct ScriptContext *ctx)
{
    u8 mapGroup = ScriptReadByte(ctx);
    u8 mapNum = ScriptReadByte(ctx);
    u8 objectId = ScriptReadByte(ctx);
    u8 *script = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    u8 *scriptEnd = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    InitRamScript(script, scriptEnd - script, mapGroup, mapNum, objectId);
    return FALSE;
}

bool8 MEScrCmd_givenationaldex(struct ScriptContext *ctx)
{
    EnableNationalPokedex();
    StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventNationalDex);
    ctx->mStatus = MEVENT_STATUS_SUCCESS;
    return FALSE;
}

bool8 MEScrCmd_addrareword(struct ScriptContext *ctx)
{
    UnlockTrendySaying(ScriptReadByte(ctx));
    StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventRareWord);
    ctx->mStatus = MEVENT_STATUS_SUCCESS;
    return FALSE;
}

bool8 MEScrCmd_setrecordmixinggift(struct ScriptContext *ctx)
{
    u8 unk = ScriptReadByte(ctx);
    u8 quantity = ScriptReadByte(ctx);
    u16 itemId = ScriptReadHalfword(ctx);
    SetRecordMixingGift(unk, quantity, itemId);
    return FALSE;
}

bool8 MEScrCmd_givepokemon(struct ScriptContext *ctx)
{
    struct Mail mail;
    struct Pokemon pokemon;
    u16 species;
    u16 heldItem;
    u32 data = ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase;
    void *pokemonPtr = (void *)data;
    void *mailPtr = (void *)(data + sizeof(struct Pokemon));

    pokemon = *(struct Pokemon *)pokemonPtr;
    species = GetMonData(&pokemon, MON_DATA_SPECIES_OR_EGG);

    if (species == SPECIES_EGG)
        StringCopyN(gStringVar1, gText_EggNickname, POKEMON_NAME_LENGTH + 1);
    else
        StringCopyN(gStringVar1, gText_Pokemon, POKEMON_NAME_LENGTH + 1);

    if (gPlayerPartyCount == PARTY_SIZE)
    {
        StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventFullParty);
        ctx->mStatus = MEVENT_STATUS_FAILURE;
    }
    else
    {
        memcpy(&gPlayerParty[PARTY_SIZE - 1], pokemonPtr, sizeof(struct Pokemon));
        memcpy(&mail, mailPtr, sizeof(struct Mail));

        if (species != SPECIES_EGG)
        {
            u16 pokedexNum = SpeciesToNationalPokedexNum(species);
            GetSetPokedexFlag(pokedexNum, FLAG_SET_SEEN);
            GetSetPokedexFlag(pokedexNum, FLAG_SET_CAUGHT);
        }

        heldItem = GetMonData(&gPlayerParty[PARTY_SIZE - 1], MON_DATA_HELD_ITEM);
        if (ItemIsMail(heldItem))
            GiveMailToMon(&gPlayerParty[PARTY_SIZE - 1], &mail);
        CompactPartySlots();
        CalculatePlayerPartyCount();
        StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventSentOver);
        ctx->mStatus = MEVENT_STATUS_SUCCESS;
    }

    return FALSE;
}

bool8 MEScrCmd_addtrainer(struct ScriptContext *ctx)
{
    u32 data = ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase;
    memcpy(&gSaveBlock2Ptr->frontier.ereaderTrainer, (void *)data, sizeof(gSaveBlock2Ptr->frontier.ereaderTrainer));
    ValidateEReaderTrainer();
    StringExpandPlaceholders(gStringVarBuffer, gText_MysteryEventNewTrainer);
    ctx->mStatus = MEVENT_STATUS_SUCCESS;
    return FALSE;
}

bool8 MEScrCmd_enableresetrtc(struct ScriptContext *ctx)
{
    EnableResetRTC();
    StringExpandPlaceholders(gStringVarBuffer, gText_InGameClockUsable);
    ctx->mStatus = MEVENT_STATUS_SUCCESS;
    return FALSE;
}

bool8 MEScrCmd_checksum(struct ScriptContext *ctx)
{
    int checksum = ScriptReadWord(ctx);
    u8 *data = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    u8 *dataEnd = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    if (checksum != CalcByteArraySum(data, dataEnd - data))
    {
        ctx->mValid = FALSE;
        ctx->mStatus = MEVENT_STATUS_LOAD_ERROR;
    }
    return TRUE;
}

bool8 MEScrCmd_crc(struct ScriptContext *ctx)
{
    int crc = ScriptReadWord(ctx);
    u8 *data = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    u8 *dataEnd = (u8 *)(ScriptReadWord(ctx) - ctx->mOffset + ctx->mScriptBase);
    if (crc != CalcCRC16(data, dataEnd - data))
    {
        ctx->mValid = FALSE;
        ctx->mStatus = MEVENT_STATUS_LOAD_ERROR;
    }
    return TRUE;
}
