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
    state_test,
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

static quickrdr_state_t state = state_main;
// state_main
static quickrdr_option_t menu_option = option_open;
// state_book_list
#define book_list_perpage 6
static quickrdr_book_t book_list_entries[book_list_perpage];
static unsigned int book_list_page;
static uint8_t book_list_count;
static uint8_t book_list_loaded;

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

static inline uint8_t getcsc()
{
    uint8_t key;
    while ((key = os_GetCSC()) == 0)
    {
    }
    return key;
}

// SINGLE LINE ONLY!!!
static void show_alert(const char *message)
{
    unsigned int width = gfx_GetStringWidth(message);
    if (width > 320)
    {
        width = 320;
    }
    unsigned int x = (320 - width) / 2;
    unsigned int boxX = x < 8 ? x : x - 8;
    gfx_BlitScreen();
    gfx_SetColor(COLOR_BAR_BG);
    gfx_FillRectangle(boxX, 100, 320 - boxX - boxX, 40);
    gfx_SetTextFGColor(COLOR_BAR_TEXT);
    gfx_SetTextBGColor(COLOR_BAR_BG);
    gfx_PrintStringXY(message, x, 116);
    gfx_SwapDraw();
    getcsc();
}

static void try_continue_book(void)
{
    void *search_pos = NULL;
    char *detected_name = ti_DetectVar(&search_pos, "QKRDR", OS_TYPE_APPVAR);
    if (detected_name == NULL)
    {
        show_alert("No book found");
        return;
    }
}

static int step(void)
{
    uint8_t key = os_GetCSC();
    if (state == state_main)
    {
        if (key == sk_Clear)
        {
            return 0;
        }
        else if (key == sk_Up)
        {
            if (menu_option == 0)
            {
                menu_option = option_count;
            }
            menu_option--;
        }
        else if (key == sk_Down)
        {
            if (++menu_option >= option_count)
            {
                menu_option = 0;
            }
        }
        else if (key == sk_Enter)
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
                book_list_page = 0;
                book_list_count = 0;
                book_list_loaded = 0;
            }
        }
        else if (key == sk_Vars)
        {
            state = state_test;
        }
    }
    else if (state == state_book_list)
    {
        if (!book_list_loaded)
        {
            book_list_count = quickrdr_list_files(book_list_entries, book_list_page * book_list_perpage, book_list_perpage);
        }
    }
    else if (state == state_test)
    {
        if (key == sk_Clear)
        {
            state = state_main;
        }
    }
    else if (state == state_settings || state == state_about)
    {
        if (key == sk_Clear)
        {
            state = state_main;
        }
    }
    return 1;
}

static void draw(void)
{
    static char buf[32];
    // top & bottom bar
    gfx_SetColor(COLOR_BAR_BG);
    gfx_FillRectangle_NoClip(0, 0, 320, 240);
    // main menu
    gfx_SetColor(COLOR_MAIN_BG);
    gfx_FillRectangle_NoClip(0, 24, 320, 196);
    // top & bottom bar text
    gfx_SetTextFGColor(COLOR_BAR_TEXT);
    gfx_SetTextBGColor(COLOR_BAR_BG);
    if (state == state_main)
    {
        // top bar text
        gfx_PrintStringXY("QUICKRDR", 8, 8);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Navigate [ENTER] Select [CLEAR] Quit", 8, 226);
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
        // top bar text
        gfx_PrintStringXY("QUICKRDR: Open Book", 8, 8);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Item [\x11\x10] Page [ENTER] Open [CLEAR] Back", 8, 226);
    }
    else if (state == state_test)
    {
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 16; j++)
            {
                gfx_SetTextXY(j * 8, i * 8);
                gfx_PrintChar(i * 16 + j);
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