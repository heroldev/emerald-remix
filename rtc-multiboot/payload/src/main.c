#include "global.h"
#include "main.h"
#include "rtc.h"
#include "rtc_reset.h"

#define ROM_HEADER_MAGIC 0x96
#define ROM_GAME_TITLE_LEN 15

// Task states for Task_ResetRtcScreen
enum {
    STATE_INIT,
    STATE_CHECK_SAVE,
    STATE_START_SET_TIME,
    STATE_WAIT_SET_TIME,
    STATE_SAVE,
    STATE_WAIT_EXIT,
    STATE_EXIT,
};

// Return values for ValidateRomHeader
enum
{
    UPDATE_SAPPHIRE = 2,
    UPDATE_RUBY,
    NO_UPDATE_SAPPHIRE,
    NO_UPDATE_RUBY,
    INVALID
};

static s32 sInitialWaitTimer;
IntrFunc gIntrTable[16];
u16 gHeldKeys;
u16 gNewKeys;
u8 gIntrVector[0x100];
u32 gUpdateSuccessful;
u32 gUnusedVar;
u32 gUnusedBuffer[25];
u32 gMainCallbackState;
u32 gGameVersion;

static EWRAM_DATA u8 sSharedMem[0x8000] = {};

extern void IntrMain(void);

static void ReadKeys(void);
static void IntrDummy(void);
static void SerialIntr(void);
static void BerryFix(u32 *, void *, void *);

static const char sBerryFixGameCode[] = "AGBJ";

const IntrFunc gIntrFuncPointers[] = {
    IntrDummy,
    SerialIntr,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
    NULL,
    NULL,
    NULL,
};

// Language character code, followed by version number
static const char sVersionData[][2] = {
    {'J', 1},
    {'E', 2},
    {'D', 1},
    {'F', 1},
    {'I', 1},
    {'S', 1}
};
static const char sRubyTitleAndCode[ROM_GAME_TITLE_LEN] = "POKEMON RUBYAXV";
static const char sSapphireTitleAndCode[ROM_GAME_TITLE_LEN] = "POKEMON SAPPAXP";
static const char sEmeraldTitleAndCode[ROM_GAME_TITLE_LEN] = "POKEMON EMERBPE";
static const u16 sDebugPals[20] = {
    RGB_BLACK,
    RGB_RED,
    RGB_GREEN,
    RGB_BLUE
};

void AgbMain(void)
{
    RegisterRamReset(RESET_IWRAM | RESET_PALETTE | RESET_VRAM | RESET_OAM);
    DmaCopy32(3, gIntrFuncPointers, gIntrTable, sizeof(gIntrFuncPointers));
    DmaCopy32(3, IntrMain, gIntrVector, sizeof(gIntrVector));
    INTR_VECTOR = gIntrVector;
    REG_IE = INTR_FLAG_VBLANK;
    if (*RomHeaderMagic == ROM_HEADER_MAGIC && *(u32 *)RomHeaderGameCode == *(u32 *)sBerryFixGameCode)
        REG_IE |= INTR_FLAG_GAMEPAK;
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IME = INTR_FLAG_VBLANK;
    MessageBox_Load();
    gMainCallbackState = STATE_INIT;
    gUnusedVar = 0;
    for (;;)
    {
        VBlankIntrWait();
        ReadKeys(); //what
        BerryFix(&gMainCallbackState, gUnusedBuffer, sSharedMem); // TaskMainResetRtc call goes here
    }
}

static void SerialIntr(void)
{}

static void IntrDummy(void)
{}

static void ReadKeys(void)
{
    u16 keyInput = REG_KEYINPUT ^ KEYS_MASK;
    gNewKeys = keyInput & ~gHeldKeys;
    gHeldKeys = keyInput;
}

// Unused
static void fill_palette(const u8 * src, u16 * dest, u8 value)
{
    s32 i;
    for (i = 0; src[i] != 0; i++)
        dest[i] = src[i] | value << 12;
}

static bool32 CheckSameString(const char * a, const char * b, size_t size)
{
    s32 i;
    for (i = 0; i < size; i++)
    {
        if (a[i] != b[i])
            return FALSE;
    }
    return TRUE;
}

static s32 ValidateGameVersion(void)
{
    char languageCode = RomHeaderGameCode[3];
    s32 softwareVersion = *RomHeaderSoftwareVersion;
    s32 shouldUpdate = -1;
    s32 i;

    // Check rom header data to see if games of this
    // language and revision need the berry fix.
    for (i = 0; i < ARRAY_COUNT(sVersionData); i++)
    {
        if (languageCode == sVersionData[i][0])
        {
            if (softwareVersion >= sVersionData[i][1])
                shouldUpdate = FALSE;
            else
                shouldUpdate = TRUE;
            break;
        }
    }
    if (shouldUpdate != -1)
    {
        // A valid language/revision was found, check game title
        // and code to see if it's Ruby or Sapphire

        if (CheckSameString(RomHeaderGameTitle, sRubyTitleAndCode, ROM_GAME_TITLE_LEN) == TRUE)
        {
            if (shouldUpdate == FALSE)
            {
                return NO_UPDATE_RUBY;
            }
            else
            {
                gGameVersion = VERSION_RUBY;
                return UPDATE_RUBY;
            }
        }
        else if (CheckSameString(RomHeaderGameTitle, sSapphireTitleAndCode, ROM_GAME_TITLE_LEN) == TRUE)
        {
            if (shouldUpdate == FALSE)
            {
                return NO_UPDATE_SAPPHIRE;
            }
            else
            {
                gGameVersion = VERSION_SAPPHIRE;
                return UPDATE_SAPPHIRE;
            }
        }
    }
    return INVALID;
}

static s32 ValidateRomHeader(void)
{
    if (RomHeaderMakerCode[0] == '0' && RomHeaderMakerCode[1] == '1' && *RomHeaderMagic == ROM_HEADER_MAGIC)
        return ValidateGameVersion();
    else
        return INVALID;
}

static void BerryFix(u32 * state, void * unused1, void * unused2)
{
    u8 year;
    switch (*state)
    {
    //this is where the states will go to control the RTC screens
    }
}
