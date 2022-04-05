#include "global.h"
#include "main.h"
#include "reset_rtc.h"

bool32 ResetRtc_IdentifyFlash(void)
{
    gFlashIdentIsValid = TRUE;
    if (!IdentifyFlash())
    {
        SetFlashTimerIntr(0, &((IntrFunc *)gIntrFuncPointers)[9]);
        return TRUE;
    }
    gFlashIdentIsValid = FALSE;
    return FALSE;
}

// Unused
static void ResetRtc_ReadFlash(u16 sectorNum, ptrdiff_t offset, void * dest, size_t size)
{
    ReadFlash(sectorNum, offset, dest, size);
}

static u8 ResetRtc_WriteSaveSectorOrSlot(u16 sectorId, const struct SaveBlockChunk * chunks)
{
    return WriteSaveSectorOrSlot(sectorId, chunks);
}

static u8 ResetRtc_TryLoadSaveSlot(u16 sectorId, const struct SaveBlockChunk * chunks)
{
    return TryLoadSaveSlot(sectorId, chunks);
}

static u32 * ResetRtc_GetDamagedSaveSectors(void)
{
    return &gDamagedSaveSectors;
}

static s32 ResetRtc_Save(u8 mode)
{
    u8 i;
    switch (mode)
    {
    case SAVE_NORMAL:
    default:
        ResetRtc_WriteSaveSectorOrSlot(FULL_SAVE_SLOT, gSaveBlockChunks);
        break;
    case SAVE_SAVEBLOCKS:
        for (i = SECTOR_ID_SAVEBLOCK2; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
            ResetRtc_WriteSaveSectorOrSlot(i, gSaveBlockChunks);
        break;
    case SAVE_SAVEBLOCK2:
        ResetRtc_WriteSaveSectorOrSlot(SECTOR_ID_SAVEBLOCK2, gSaveBlockChunks);
        break;
    }

    return 0;
}

u8 ResetRtc_TrySave(u8 mode)
{
    ResetRtc_Save(mode);
    if (*ResetRtc_GetDamagedSaveSectors() == 0)
        return SAVE_STATUS_OK;
    return SAVE_STATUS_ERROR;
}

u8 ResetRtc_LoadSave(u32 unused)
{
    return ResetRtc_TryLoadSaveSlot(FULL_SAVE_SLOT, gSaveBlockChunks);
}

bool8 VarSet(u16 id, u16 value)
{
    u16 *ptr = GetVarPointer(id);
    if (!ptr)
        return FALSE;
    *ptr = value;
    return TRUE;
}
