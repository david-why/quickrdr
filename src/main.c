#include <ti/getkey.h>
#include <ti/getcsc.h>
#include <sys/lcd.h>
#include <sys/rtc.h>
#include <sys/power.h>

#include <fileioc.h>
#include <graphx.h>

#include "gfx/gfx.h"
#include "quickrdr.h"

#include <string.h>

#define COLOR_OFF_WHITE 0
#define COLOR_BLACK 1
#define COLOR_WHITE 2
#define COLOR_MEDIUM_GRAY 3
#define COLOR_MEDIUM_BLUE 4
#define COLOR_DARK_GRAY 5
#define COLOR_ACCENT_BLUE 6

#define COLOR_BAR_BG COLOR_MEDIUM_BLUE
#define COLOR_BAR_TEXT COLOR_WHITE
#define COLOR_MAIN_BG COLOR_OFF_WHITE
#define COLOR_MAIN_TEXT COLOR_DARK_GRAY
#define COLOR_HIGHLIGHT_BG COLOR_ACCENT_BLUE
#define COLOR_HIGHLIGHT_TEXT COLOR_WHITE

typedef enum
{
    state_main = 0,
    state_reading,
    state_settings,
    state_about,
    state_book_list,
    state_count
} quickrdr_state_t;

typedef enum
{
    option_open = 0,
    option_continue,
    option_settings,
    option_about,
    option_quit,
    option_count
} quickrdr_option_t;

typedef struct
{
    char name[9];
} quickrdr_book_t;

static quickrdr_state_t state = state_main;
// state_main
static quickrdr_option_t menu_option = option_open;
// state_book_list
static quickrdr_book_t book_list_entries[6];
static int book_list_page;

static void get_time(char *output)
{
    uint8_t hours, minutes, seconds;
    boot_GetTime(&seconds, &minutes, &hours);
    if (hours < 10)
    {
        output[0] = '0';
        output[1] = '0' + hours;
    }
    else
    {
        output[0] = '0' + (hours / 10);
        output[1] = '0' + (hours % 10);
    }
    if (minutes < 10)
    {
        output[3] = '0';
        output[4] = '0' + minutes;
    }
    else
    {
        output[3] = '0' + (minutes / 10);
        output[4] = '0' + (minutes % 10);
    }
    if (seconds < 10)
    {
        output[6] = '0';
        output[7] = '0' + seconds;
    }
    else
    {
        output[6] = '0' + (seconds / 10);
        output[7] = '0' + (seconds % 10);
    }
    output[2] = ':';
    output[5] = ':';
    output[8] = '\0';
}

static void try_continue_book(void) {}

static int step(void)
{
    uint8_t key = os_GetCSC();
    if (state == state_main)
    {
        if (key == sk_Clear)
        {
            return 0;
        }
        if (key == sk_Up)
        {
            if (menu_option == 0)
            {
                menu_option = option_count;
            }
            menu_option--;
        }
        if (key == sk_Down)
        {
            if (++menu_option >= option_count)
            {
                menu_option = 0;
            }
        }
        if (key == sk_Enter)
        {
            if (menu_option == option_continue)
            {
                try_continue_book();
            }
            else if (menu_option == option_quit)
            {
                return 0;
            }
            else
            {
                static quickrdr_state_t option_to_state[] = {
                    [option_open] = state_book_list,
                    [option_settings] = state_settings,
                    [option_about] = state_about,
                };
                state = option_to_state[menu_option];
            }
        }
    }
    else if (state == state_book_list)
    {
    }
    return 1;
}

static void draw(void)
{
    static char buf[32];
    if (state == state_main)
    {
        // top & bottom bar
        gfx_SetColor(COLOR_BAR_BG);
        gfx_FillRectangle_NoClip(0, 0, 320, 240);
        // top bar text
        gfx_SetTextFGColor(COLOR_BAR_TEXT);
        gfx_SetTextBGColor(COLOR_BAR_BG);
        gfx_PrintStringXY("QUICKRDR", 8, 8);
        get_time(buf);
        gfx_GetStringWidth(buf);
        gfx_SetTextXY(320 - gfx_GetStringWidth(buf) - 8, 8);
        gfx_PrintString(buf);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Navigate [ENTER] Select [CLEAR] Quit", 8, 226);
        // main menu
        gfx_SetColor(COLOR_MAIN_BG);
        gfx_FillRectangle_NoClip(0, 24, 320, 196);
        static const char *menu_items[] = {
            [option_open] = "Open Book",
            [option_continue] = "Continue Reading",
            [option_settings] = "Settings",
            [option_about] = "About",
            [option_quit] = "Quit",
        };
        for (quickrdr_option_t i = 0; i < option_count; i++)
        {
            gfx_SetColor(menu_option == i ? COLOR_HIGHLIGHT_BG : COLOR_MAIN_BG);
            gfx_FillRectangle_NoClip(0, 30 + i * 32, 320, 20);
            gfx_SetTextFGColor(menu_option == i ? COLOR_HIGHLIGHT_TEXT : COLOR_MAIN_TEXT);
            gfx_SetTextBGColor(menu_option == i ? COLOR_HIGHLIGHT_BG : COLOR_MAIN_BG);
            gfx_PrintStringXY(menu_items[i], 24, 36 + i * 32);
            if (menu_option == i)
            {
                gfx_PrintStringXY(">", 8, 36 + i * 32);
            }
        }
    }
    else if (state == state_book_list)
    {
        gfx_ZeroScreen();
        gfx_SetTextScale(2, 2);
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                gfx_SetTextXY(x * 16, y * 16);
                gfx_PrintChar(y * 16 + x);
            }
        }
    }
}

int main()
{
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetPalette(quickrdr_palette, sizeof(quickrdr_palette), 0);
    gfx_ZeroScreen();

    while (step())
    {
        draw();
        gfx_SwapDraw();
    }

    gfx_End();
    return 0;
}