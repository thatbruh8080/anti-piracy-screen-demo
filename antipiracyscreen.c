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
    FLAG_BLACK_WAIT,     // Black flag gets shown
    FLAG_BLACK_LOWERS,   // Then it lowers
    WAIT_A_SEC,          // Suspens-
    NOQUARTER_RISES,     // The no quarter flag rises
    NOQUARTER_STAYS,     // And it stays there. You know what that means.....

};

u8 state = FLAG_BLACK_WAIT;
u16 stateTimer = 0;
u8 frame = 0;
u8 timer = 0;
u8 flagY = 56;

//=============================================================================
// SYSTEM FONT (so the message won't look like garbage in DOS)
//=============================================================================

// RAM copy of the MSX system font
static u8 g_SystemFont[256 * 8];

void LoadSystemFont()
{
    // System variables pointing to whatever Main ROM font your MSX uses
    u8 slot = *((u8*)0xF91F);
    u16 addr = *((u16*)0xF920);

    // Copy all 256 8-byte characters from the ROM slot into normal RAM
    for(u16 i = 0; i < sizeof(g_SystemFont); ++i)
    {
        g_SystemFont[i] = BIOS_InterSlotRead(slot, addr + i);
    }

    // Characters are stored as 8x8 bitmaps but printed 6 pixels wide
    Print_SetFontEx(
        8, 8,
        6, 8,
        1, 255,
        g_SystemFont + 8
    );

    // We're gonna DRAW TEXT ANYWHERE WE WANT ON THE BITMAP WOO
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
    // Allat video setup, pretty much self explanatory
    VDP_SetMode(VDP_MODE_SCREEN5);
    VDP_EnableVBlank(TRUE);
    VDP_ClearVRAM();
    VDP_EnableSprite(FALSE);

    // Sets palette seen in the bottom of content/flags.h
    VDP_SetPalette(Flags_palette);
    VDP_SetBackdropColor(2);

    // Writes the flag frames into offscreen VRAM
    for(u8 y = 0; y < 88; y++)
    {
        VDP_WriteVRAM(
            Flags + ((u16)y * 112),
            0x8000 + ((u16)y * 128),
            0,
            112
        );
    }

    // Writes some stupid green fish into offscreen VRAM
    for(u8 y = 0; y < 72; y++)
    {
        VDP_WriteVRAM(
            Dopefish + ((u16)y * 40),
            0xAC00 + ((u16)y * 128),
            0,
            40
        );
    }

    // Writes the MSXgl logo into offscreen VRAM
    for(u8 y = 0; y < 8; y++)
    {
        VDP_WriteVRAM(
            Logo + ((u16)y * 16),
            0xD000 + ((u16)y * 128),
            0,
            16
        );
    }

    // Draws the pole
    Draw_FillBox(85, 56, 155, 200, 1, VDP_OP_IMP);
    Draw_Box(89, 61, 90, 200, 9, VDP_OP_IMP);
    Draw_Line(88, 200, 88, 61, 8, VDP_OP_IMP);
    Draw_Line(91, 200, 91, 61, 8, VDP_OP_IMP);

    // Backs up the background beneath the flag so it can be restored as the flag moves
    VDP_CommandHMMM(88, 56, 80, 344, 56, 44);

    // Use the MSX Main-ROM font
    LoadSystemFont();
    // Sets the print foreground and background color to palette indices 6 and 2, check any .h on the content folder for the palette
    Print_SetColor(6, 2);

    // Prints insulting message
    Print_DrawTextAt(54, 24, "This is what happens to");
    Print_DrawTextAt(42, 32, "losers who pirate our games");

    // Main main loop so to speak, while the user hasn't pressed escape yet
    while (!Keyboard_IsKeyPressed(KEY_ESC))
    {
        // Halts the Z80 until an interrupt is accepted; normally this is the VBlank interrupt
        Halt();

        timer++;

        // Animates the flag every 6 VBlanks
        if(timer >=6)
        {
            timer = 0;

            frame++;

            // Loops back after the fourth frame
            if(frame >= 4)
                frame = 0;

        }

        // Counts the number of VBlanks
        stateTimer++;

        switch(state)
        {
            case FLAG_BLACK_WAIT:

                // Copies the current flag frame from page 1 to page 0.
                // The selected animation frame changes every 6 VBlanks.
                VDP_CommandHMMM((u16)frame * 56, 256, 88, 56, 56, 44);

                // Keep waving for 240 VBlanks before the flag lowers, roughly 4 seconds in 60 Hz
                if(stateTimer >= 240)
                {
                    stateTimer = 0;
                    state = FLAG_BLACK_LOWERS;
                }

                break;


            case FLAG_BLACK_LOWERS:
            {
                // Remembers where the flag was last frame
                u8 oldY = flagY;
                // Number of flag rows currently visible on screen
                u8 newHeight;

                // The flag shall lower by one pixel
                flagY++;

                // Clips the flag as it goes down the screen
                if(flagY >= 201)
                    newHeight = 0;
                else if(flagY + 44 > 201)
                    newHeight = 201 - flagY;
                else
                    newHeight = 44;

                // Draws whatever part of the flag is still on screen
                if(newHeight > 0)
                    VDP_CommandHMMM((u16)frame * 56, 256, 88, flagY, 56, newHeight);

                // Restores the row the flag just moved away from
                if(oldY < 61)
                    VDP_CommandHMMM(80, 344 + (oldY - 56), 88, oldY, 56, 1);
                else
                    VDP_CommandHMMM(80, 349, 88, oldY, 56, 1);

                // Once the whole thing disappears to the bottom, something bad's about to happen
                if(flagY >= 201)
                {
                    stateTimer = 0;
                    state = WAIT_A_SEC;
                }

                break;
            }

            case WAIT_A_SEC:

                // Waits for 240 VBlanks, about 4 seconds in 60 Hz
                if(stateTimer >= 240)
                {
                    stateTimer = 0;
                    flagY = 201;
                    state = NOQUARTER_RISES;
                }

                break;


            case NOQUARTER_RISES:
            {
                // Remembers where the flag was last frame
                u8 oldY = flagY;
                // Number of flag rows currently visible on screen
                u8 newHeight;

                // The no quarter flag shall rise a pixel per frame like a good flag
                flagY--;

                // Clips the flag while it comes up from below
                if(flagY + 44 > 201)
                    newHeight = 201 - flagY;
                else
                    newHeight = 44;
                
                // Copies the visible part of the no quarter flag onto the screen
                VDP_CommandHMMM((u16)frame * 56, 300, 88, flagY, 56, newHeight);

                // Once fully onscreen, erase the bottom row it leaves behind
                if(oldY <= 157)
                    VDP_CommandHMMM(80, 349, 88, oldY + 43, 56, 1);

                // Stops flag from breaking the laws of physics and flying too close to the sun
                if(flagY <= 56)
                {
                    flagY = 56;
                    state = NOQUARTER_STAYS;
                }

                break;
            }

            case NOQUARTER_STAYS:

                // Supa boring, keeps waving the red flag on top for eternity unless the user presses esc. Either resets or quits to whatever operating system used to launch it
                // While that happens DEVELOPERS ASSEMBLE! EN GARDE RAAAAAAA- (offscreen cuz too violent to program)
                VDP_CommandHMMM((u16)frame * 56, 300, 88, 56, 56, 44);

                break;
        }
    }
    // Calls the BIOS to exit the program
    BIOS_Exit(0);
}
