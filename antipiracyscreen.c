// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄      
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  Anti piracy screen and V9938 command set demo. Unauthorized copying is encouraged!
//────────────────────────────────────────────────────────────────────────────────────

//=============================================================================
// INCLUDES
//=============================================================================
#include "msxgl.h"
#include "vdp.h"
#include "print.h"
#include "content/flags.h"
#include "content/dopefish.h"
#include "content/logo.h"
#include "draw.h"

//=============================================================================
// VARIABLES & STATE MACHINES
//=============================================================================
enum {
    FLAG_BLACK_WAIT,
    FLAG_BLACK_LOWERS,
    WAIT_A_SEC,
    NOQUARTER_RISES,
    NOQUARTER_STAYS,

};

u8 state = FLAG_BLACK_WAIT;
u16 stateTimer = 0;
u8 frame = 0;
u8 timer = 0;
u8 flagY = 56;

//=============================================================================
// SYSTEM FONT (so the message won't look like garbage in DOS)
//=============================================================================

static u8 g_SystemFont[256 * 8];

void LoadSystemFont()
{
    u8 slot = *((u8*)0xF91F);
    u16 addr = *((u16*)0xF920);

    for(u16 i = 0; i < sizeof(g_SystemFont); ++i)
    {
        g_SystemFont[i] = BIOS_InterSlotRead(slot, addr + i);
    }

    Print_SetFontEx(
        8, 8,
        6, 8,
        1, 255,
        g_SystemFont + 8
    );

    Print_SetMode(PRINT_MODE_BITMAP);
    Print_Initialize();
}


//=============================================================================
// MAIN LOOP
//=============================================================================

//-----------------------------------------------------------------------------
// Program entry point
void main()
{
    VDP_SetMode(VDP_MODE_SCREEN5);
    VDP_EnableVBlank(TRUE);
    VDP_ClearVRAM();
    VDP_EnableSprite(FALSE);

    VDP_SetPalette(Flags_palette);
    VDP_SetBackdropColor(2);

    for(u8 y = 0; y < 88; y++)
    {
        VDP_WriteVRAM(
            Flags + ((u16)y * 112),
            0x8000 + ((u16)y * 128),
            0,
            112
        );
    }

    for(u8 y = 0; y < 72; y++)
    {
        VDP_WriteVRAM(
            Dopefish + ((u16)y * 40),
            0xAC00 + ((u16)y * 128),
            0,
            40
        );
    }

    for(u8 y = 0; y < 8; y++)
    {
        VDP_WriteVRAM(
            Logo + ((u16)y * 16),
            0xD000 + ((u16)y * 128),
            0,
            16
        );
    }

    Draw_FillBox(85, 56, 155, 200, 1, VDP_OP_IMP);
    Draw_Box(89, 61, 90, 200, 9, VDP_OP_IMP);
    Draw_Line(88, 200, 88, 61, 8, VDP_OP_IMP);
    Draw_Line(91, 200, 91, 61, 8, VDP_OP_IMP);

    VDP_CommandHMMM(88, 56, 80, 344, 56, 44);

    // Use the MSX Main-ROM font
    LoadSystemFont();
    Print_SetColor(6, 2);

    Print_DrawTextAt(54, 24, "This is what happens to");
    Print_DrawTextAt(42, 32, "losers who pirate our games");

    while (!Keyboard_IsKeyPressed(KEY_ESC))
    {
        Halt();

        timer++;

        if(timer >=6)
        {
            timer = 0;

            frame++;

            if(frame >= 4)
                frame = 0;

        }

        stateTimer++;

        switch(state)
        {
            case FLAG_BLACK_WAIT:

                VDP_CommandHMMM((u16)frame * 56, 256, 88, 56, 56, 44);

                if(stateTimer >= 240)
                {
                    stateTimer = 0;
                    state = FLAG_BLACK_LOWERS;
                }

                break;


            case FLAG_BLACK_LOWERS:
            {
                u8 oldY = flagY;
                u8 newHeight;

                flagY++;

                if(flagY >= 201)
                    newHeight = 0;
                else if(flagY + 44 > 201)
                    newHeight = 201 - flagY;
                else
                    newHeight = 44;

                if(newHeight > 0)
                    VDP_CommandHMMM((u16)frame * 56, 256, 88, flagY, 56, newHeight);

                if(oldY < 61)
                    VDP_CommandHMMM(80, 344 + (oldY - 56), 88, oldY, 56, 1);
                else
                    VDP_CommandHMMM(80, 349, 88, oldY, 56, 1);

                if(flagY >= 201)
                {
                    stateTimer = 0;
                    state = WAIT_A_SEC;
                }

                break;
            }

            case WAIT_A_SEC:

                if(stateTimer >= 240)
                {
                    stateTimer = 0;
                    flagY = 201;
                    state = NOQUARTER_RISES;
                }

                break;


            case NOQUARTER_RISES:
            {
                u8 oldY = flagY;
                u8 newHeight;

                flagY--;

                if(flagY + 44 > 201)
                    newHeight = 201 - flagY;
                else
                    newHeight = 44;

                VDP_CommandHMMM((u16)frame * 56, 300, 88, flagY, 56, newHeight);

                // Once fully onscreen, erase the bottom row it leaves behind
                if(oldY <= 157)
                    VDP_CommandHMMM(80, 349, 88, oldY + 43, 56, 1);

                if(flagY <= 56)
                {
                    flagY = 56;
                    state = NOQUARTER_STAYS;
                }

                break;
            }

            case NOQUARTER_STAYS:

                VDP_CommandHMMM((u16)frame * 56, 300, 88, 56, 56, 44);

                break;
        }
    }

    BIOS_Exit(0);
}
