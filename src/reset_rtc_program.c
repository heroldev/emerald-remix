#include "global.h"
#include "gpu_regs.h"
#include "multiboot.h"
#include "malloc.h"
#include "bg.h"
#include "graphics.h"
#include "main.h"
#include "sprite.h"
#include "task.h"
#include "scanline_effect.h"
#include "window.h"
#include "text.h"
#include "menu.h"
#include "m4a.h"
#include "constants/rgb.h"

struct {
    u8 state;
    u8 curScene;
    u16 timer;
    struct MultiBootParam mb;
} static * sRtcPrgm;

static void RtcPrgm_Main(void);
static void RtcPrgm_GpuSet(void);
static int RtcPrgm_TrySetScene(int);
static void RtcPrgm_SetScene(int);
static void RtcPrgm_HideScene(void);

static const u8 sText_RtcProgram[] = _("RTC Reset");
static const u8 sText_RtcRubySapphire[] = _("Ruby/Sapphire");
static const u8 sText_RtcEmerald[] = _("Emerald");
static const u8 sText_RtcWillBeResetPressA[] = _("The Berry Program on your POKéMON\n"
                                                            "Ruby/Sapphire Game Pak will be updated.\n"
                                                            "{COLOR RED}{SHADOW LIGHT_RED}Press the A Button.");
static const u8 sText_RtcEnsureGBAConnectionMatches[] = _("Please ensure the connection of your\n"
                                                       "Game Boy Advance system matches this.\n"
                                                       "{COLOR RED}{SHADOW LIGHT_RED}YES: Press the A Button.\n"
                                                       "NO: Turn off the power and try again.");
static const u8 sText_RtcTurnOffPowerHoldingStartSelect[] = _("Please turn on the power of POKéMON\n"
                                                           "Ruby/Sapphire while holding START and\n"
                                                           "SELECT simultaneously. Then, ensure\n"
                                                           "the picture above appears.");
static const u8 sText_RtcTransmittingPleaseWait[] = _("Transmitting. Please wait.\n"
                                                   "{COLOR RED}{SHADOW LIGHT_RED}Please do not turn off the power or\n"
                                                   "unplug the Game Boy Advance Game\nLink Cable.");
static const u8 sText_RtcPleaseFollowInstructionsOnScreen[] = _("Please follow the instructions on your\n"
                                                             "POKéMON Ruby/Sapphire screen.");
static const u8 sText_RtcTransmissionFailureTryAgain[] = _("Transmission failure.\n"
                                                        "{COLOR RED}{SHADOW LIGHT_RED}Please try again.");

static const struct BgTemplate sRtcPrgmBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const struct WindowTemplate sRtcPrgmWindowTemplates[] = {
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 4,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 11,
        .width = 28,
        .height = 8,
        .paletteNum = 15,
        .baseBlock = 53
    },
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 8,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 277
    },
    {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 0,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 337
    },
    DUMMY_WIN_TEMPLATE
};

static const u16 sRtcPrgmPalColors[] = {
    RGB_WHITE,       RGB_WHITE,       RGB(12, 12, 12), RGB(26, 26, 25),
    RGB(28, 1, 1),   RGB(31, 23, 14), RGB(4, 19, 1),   RGB(18, 30, 18),
    RGB(6, 10, 25),  RGB(20, 24, 30), RGB_WHITE,       RGB(12, 12, 12),
    RGB(26, 26, 25), RGB_BLACK,       RGB_BLACK,       RGB_BLACK
};

static const u8 sRtcPrgmTextColors[] = {TEXT_DYNAMIC_COLOR_1, TEXT_DYNAMIC_COLOR_2, TEXT_DYNAMIC_COLOR_3};
static const u8 sRtcGameTitleTextColors[] = { TEXT_COLOR_TRANSPARENT, TEXT_DYNAMIC_COLOR_1, TEXT_DYNAMIC_COLOR_4};

enum {
    SCENE_ENSURE_CONNECT,
    SCENE_TURN_OFF_POWER,
    SCENE_TRANSMITTING,
    SCENE_FOLLOW_INSTRUCT,
    SCENE_TRANSMIT_FAILED,
    SCENE_BEGIN,
    SCENE_NONE
};

static const u8 *const sRtcPrgmTexts[] = {
    [SCENE_ENSURE_CONNECT]  = sText_RtcEnsureGBAConnectionMatches,
    [SCENE_TURN_OFF_POWER]  = sText_RtcTurnOffPowerHoldingStartSelect,
    [SCENE_TRANSMITTING]    = sText_RtcTransmittingPleaseWait,
    [SCENE_FOLLOW_INSTRUCT] = sText_RtcPleaseFollowInstructionsOnScreen,
    [SCENE_TRANSMIT_FAILED] = sText_RtcTransmissionFailureTryAgain,
    [SCENE_BEGIN]           = sText_RtcWillBeResetPressA
};

static const struct {
    const u32 *gfx;
    const u32 *tilemap;
    const u16 *palette;
} sRtcPrgmGraphics[] = {
    [SCENE_ENSURE_CONNECT] = {
        gBerryFixGameboy_Gfx,
        gBerryFixGameboy_Tilemap,
        gBerryFixGameboy_Pal
    },
    [SCENE_TURN_OFF_POWER] = {
        gBerryFixGameboyLogo_Gfx,
        gBerryFixGameboyLogo_Tilemap,
        gBerryFixGameboyLogo_Pal
    },
    [SCENE_TRANSMITTING] = {
        gBerryFixGbaTransfer_Gfx,
        gBerryFixGbaTransfer_Tilemap,
        gBerryFixGbaTransfer_Pal
    },
    [SCENE_FOLLOW_INSTRUCT] = {
        gBerryFixGbaTransferHighlight_Gfx,
        gBerryFixGbaTransferHighlight_Tilemap,
        gBerryFixGbaTransferHighlight_Pal
    },
    [SCENE_TRANSMIT_FAILED] = {
        gBerryFixGbaTransferError_Gfx,
        gBerryFixGbaTransferError_Tilemap,
        gBerryFixGbaTransferError_Pal
    },
    [SCENE_BEGIN] = {
        gBerryFixWindow_Gfx,
        gBerryFixWindow_Tilemap,
        gBerryFixWindow_Pal
    },
};

extern const u8 gMultiBootProgram_RtcResetPrgm_Start[0x3BF4];
extern const u8 gMultiBootProgram_RtcResetPrgm_End[];

enum {
    MAINSTATE_INIT,
    MAINSTATE_BEGIN,
    MAINSTATE_CONNECT,
    MAINSTATE_INIT_MULTIBOOT,
    MAINSTATE_MULTIBOOT,
    MAINSTATE_TRANSMIT,
    MAINSTATE_EXIT,
    MAINSTATE_FAILED,
};

void CB2_InitResetRtcProgram(void) {
  
    DisableInterrupts(0xFFFF); // all
    EnableInterrupts(INTR_FLAG_VBLANK);
    m4aSoundVSyncOff();
    SetVBlankCallback(NULL);
    ResetSpriteData();
    ResetTasks();
    ScanlineEffect_Stop();
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    sRtcPrgm = AllocZeroed(sizeof(*sRtcPrgm));
    sRtcPrgm->state = MAINSTATE_INIT;
    sRtcPrgm->curScene = SCENE_NONE;
    SetMainCallback2(RtcPrgm_Main);
    
}

#define TryScene(sceneNum) RtcPrgm_TrySetScene(sceneNum) == (sceneNum)

static void RtcPrgm_Main(void)
{
    switch (sRtcPrgm->state)
    {
        case MAINSTATE_INIT:
            RtcPrgm_GpuSet();
            sRtcPrgm->state = MAINSTATE_BEGIN;
            break;
        case MAINSTATE_BEGIN:
            if (TryScene(SCENE_BEGIN) && (JOY_NEW(A_BUTTON)))
                sRtcPrgm->state = MAINSTATE_CONNECT;
            break;
        case MAINSTATE_CONNECT:
            if (TryScene(SCENE_ENSURE_CONNECT) && (JOY_NEW(A_BUTTON)))
                sRtcPrgm->state = MAINSTATE_INIT_MULTIBOOT;
            break;
        case MAINSTATE_INIT_MULTIBOOT:
            if (TryScene(SCENE_TURN_OFF_POWER))
            {
                sRtcPrgm->mb.masterp = gMultiBootProgram_RtcResetPrgm_Start;
                sRtcPrgm->mb.server_type = 0;
                MultiBootInit(&sRtcPrgm->mb);
                sRtcPrgm->timer = 0;
                sRtcPrgm->state = MAINSTATE_MULTIBOOT;
            }
            break;
        case MAINSTATE_MULTIBOOT:
            MultiBootMain(&sRtcPrgm->mb);
            if (sRtcPrgm->mb.probe_count != 0 || (!(sRtcPrgm->mb.response_bit & 2) || !(sRtcPrgm->mb.client_bit & 2)))
            {
                sRtcPrgm->timer = 0;
            }
            else if (++sRtcPrgm->timer > 180)
            {
                MultiBootStartMaster(&sRtcPrgm->mb,
                                     gMultiBootProgram_RtcResetPrgm_Start + ROM_HEADER_SIZE,
                                     (u32)(gMultiBootProgram_RtcResetPrgm_End - (gMultiBootProgram_RtcResetPrgm_Start + ROM_HEADER_SIZE)),
                                     4,
                                     1);
                sRtcPrgm->state = MAINSTATE_TRANSMIT;
            }
            break;
        case MAINSTATE_TRANSMIT:
            if (TryScene(SCENE_TRANSMITTING))
            {
                MultiBootMain(&sRtcPrgm->mb);

                if (MultiBootCheckComplete(&sRtcPrgm->mb))
                    sRtcPrgm->state = MAINSTATE_EXIT;
                else if (!(sRtcPrgm->mb.client_bit & 2))
                    sRtcPrgm->state = MAINSTATE_FAILED;
            }
            break;
        case MAINSTATE_EXIT:
            if (TryScene(SCENE_FOLLOW_INSTRUCT) && JOY_NEW(A_BUTTON))
                DoSoftReset();
            break;
        case MAINSTATE_FAILED:
            if (TryScene(SCENE_TRANSMIT_FAILED) && JOY_NEW(A_BUTTON))
                sRtcPrgm->state = MAINSTATE_BEGIN;
            break;
    }
}

static void RtcPrgm_GpuSet(void)
{
    s32 width, left;

    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);

    DmaFill32(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill32(3, 0, PLTT, PLTT_SIZE);
    ResetBgsAndClearDma3BusyFlags(0);

    InitBgsFromTemplates(0, sRtcPrgmBgTemplates, ARRAY_COUNT(sRtcPrgmBgTemplates));
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    InitWindows(sRtcPrgmWindowTemplates);
    DeactivateAllTextPrinters();

    DmaCopy32(3, sRtcPrgmPalColors, BG_PLTT + 0x1E0, sizeof(sRtcPrgmPalColors));
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP);
    FillWindowPixelBuffer(2, PIXEL_FILL(0));
    FillWindowPixelBuffer(3, PIXEL_FILL(0));
    FillWindowPixelBuffer(0, PIXEL_FILL(10));

    width = GetStringWidth(FONT_SMALL, sText_RtcEmerald, 0);
    left = (120 - width) / 2;
    AddTextPrinterParameterized3(2, FONT_SMALL, left, 3, sRtcGameTitleTextColors, TEXT_SKIP_DRAW, sText_RtcEmerald);

    width = GetStringWidth(FONT_SMALL, sText_RtcRubySapphire, 0);
    left = (120 - width) / 2 + 120;
    AddTextPrinterParameterized3(2, FONT_SMALL, left, 3, sRtcGameTitleTextColors, TEXT_SKIP_DRAW, sText_RtcRubySapphire);

    width = GetStringWidth(FONT_SMALL, sText_RtcRubySapphire, 0);
    left = (112 - width) / 2;
    AddTextPrinterParameterized3(3, FONT_SMALL, left, 0, sRtcGameTitleTextColors, TEXT_SKIP_DRAW, sText_RtcRubySapphire);

    width = GetStringWidth(FONT_NORMAL, sText_RtcProgram, 0);
    left = (208 - width) / 2;
    AddTextPrinterParameterized3(0, FONT_NORMAL, left, 2, sRtcPrgmTextColors, TEXT_SKIP_DRAW, sText_RtcProgram);

    CopyWindowToVram(2, COPYWIN_GFX);
    CopyWindowToVram(3, COPYWIN_GFX);
    CopyWindowToVram(0, COPYWIN_GFX);
}

static int RtcPrgm_TrySetScene(int scene)
{
    if (sRtcPrgm->curScene == scene)
        return scene;

    if (sRtcPrgm->curScene == SCENE_NONE)
    {
        RtcPrgm_SetScene(scene);
        sRtcPrgm->curScene = scene;
    }
    else
    {
        RtcPrgm_HideScene();
        sRtcPrgm->curScene = SCENE_NONE;
    }
    return sRtcPrgm->curScene;
}

static void RtcPrgm_SetScene(int scene)
{
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
    FillWindowPixelBuffer(1, PIXEL_FILL(10));
    AddTextPrinterParameterized3(1, FONT_NORMAL, 0, 0, sRtcPrgmTextColors, TEXT_SKIP_DRAW, sRtcPrgmTexts[scene]);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);
    switch (scene)
    {
    case SCENE_ENSURE_CONNECT:
    case SCENE_TRANSMITTING:
    case SCENE_FOLLOW_INSTRUCT:
    case SCENE_TRANSMIT_FAILED:
        PutWindowTilemap(2);
        break;
    case SCENE_TURN_OFF_POWER:
        PutWindowTilemap(3);
        break;
    case SCENE_BEGIN:
        PutWindowTilemap(0);
        break;
    }
    CopyBgTilemapBufferToVram(0);
    LZ77UnCompVram(sRtcPrgmGraphics[scene].gfx, (void *)BG_CHAR_ADDR(1));
    LZ77UnCompVram(sRtcPrgmGraphics[scene].tilemap, (void *)BG_SCREEN_ADDR(31));
    CpuCopy32(sRtcPrgmGraphics[scene].palette, (void *)BG_PLTT, 0x100);
    ShowBg(0);
    ShowBg(1);
}

static void RtcPrgm_HideScene(void)
{
    HideBg(0);
    HideBg(1);
}