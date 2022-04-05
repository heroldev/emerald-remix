#ifndef GUARD_RTC_RESET_H
#define GUARD_RTC_RESET_H

#include "save.h"

enum
{
    SAVE_NORMAL,     // Save full save slot
    SAVE_SAVEBLOCKS, // Save just SaveBlock1 and SaveBlock2
    SAVE_SAVEBLOCK2, // Save just SaveBlock2
};

bool32 ResetRtc_IdentifyFlash(void);
u8 ResetRtc_TrySave(u8 mode);
bool8 ResetRtc_LoadSave(u32);

bool8 VarSet(u16 id, u16 value);

#endif  //GUARD_RTC_RESET_H
