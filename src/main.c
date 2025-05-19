#include <ti/getkey.h>
#include <ti/getcsc.h>
#include <sys/lcd.h>
#include <sys/rtc.h>
#include <sys/power.h>

#include <fileioc.h>
#include <graphx.h>
#include <debug.h>

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

#define getbit(array, index) ((array)[(index) / 8] & (1 << (7 - ((index) % 8))))

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
static unsigned int book_list_total_count;
static uint8_t book_list_count;
static uint8_t book_list_loaded;
static uint8_t book_list_chosen;
// state_reading
static quickrdr_book_handle_t reading_book;
static uint24_t reading_page;
static uint8_t *reading_page_data;
static uint24_t reading_page_size;
// static quickrdr_glyph_t *reading_glyphs; // array
static uint8_t partial_redraw = 1;

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

typedef struct
{
    char filename[10];
    uint24_t page;
} quickrdr_save_t;

static void try_continue_book(void)
{
    uint8_t var = ti_Open("QKRDSAVE", "r");
    if (var == 0)
    {
        show_alert("No book found");
        return;
    }
    quickrdr_save_t save;
    if (!ti_Read(&save, sizeof(save), 1, var))
    {
        show_alert("Failed to read save");
        ti_Close(var);
        return;
    }
    ti_Close(var);
    reading_book = quickrdr_open_book(save.filename);
    if (reading_book == NULL)
    {
        show_alert("Failed to open book");
        state = state_main;
        return;
    }
    state = state_reading;
    reading_page = save.page;
    reading_page_data = NULL;
}

static int step(void)
{
    uint8_t key = os_GetCSC();
    partial_redraw = 0;
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
            }
        }
        else if (key == sk_Vars)
        {
            state = state_test;
        }
        else if (key == sk_Prgm)
        {
            // make a test book
            quickrdr_header_t book_header;
            memcpy(book_header.magic, "QRDR", sizeof(book_header.magic));
            book_header.version = 1;
            strcpy(book_header.name, "Test Book");
            book_header.total_size = sizeof(book_header) + 6 + sizeof(quickrdr_glyph_t) + 8 + 9;
            book_header.min_extension_byte = 0;
            book_header.line_height = 16;
            book_header.font_glyph_count = 1;
            book_header.font_glyph_size = sizeof(quickrdr_glyph_t) + 8;
            book_header.page_count = 2;
            uint24_t page_offset = sizeof(book_header) + 6 + sizeof(quickrdr_glyph_t) + 8;
            uint24_t page_offset_2 = sizeof(book_header) + 6 + sizeof(quickrdr_glyph_t) + 8 + 5;
            quickrdr_glyph_t *glyph = malloc(sizeof(quickrdr_glyph_t) + 8);
            glyph->glyph_id = 1;
            glyph->width = 8;
            glyph->height = 8;
            glyph->data[0] = 0b10000000;
            glyph->data[1] = 0b01000000;
            glyph->data[2] = 0b00100000;
            glyph->data[3] = 0b00010000;
            glyph->data[4] = 0b00001000;
            glyph->data[5] = 0b00000100;
            glyph->data[6] = 0b00000010;
            glyph->data[7] = 0b00000001;
            char *filename = "TESTBK00";
            uint8_t page_data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
            uint8_t var = ti_Open(filename, "w");
            if (var == 0)
            {
                show_alert("Failed to create book");
            }
            else
            {
                ti_Write(&book_header, sizeof(book_header), 1, var);
                ti_Write(&page_offset, sizeof(page_offset), 1, var);
                ti_Write(&page_offset_2, sizeof(page_offset), 1, var);
                ti_Write(glyph, sizeof(quickrdr_glyph_t) + 8, 1, var);
                ti_Write(page_data, sizeof(page_data), 1, var);
                ti_SetArchiveStatus(1, var);
                ti_Close(var);
                show_alert("Book created");
            }
            free(glyph);
        }
    }
    else if (state == state_book_list)
    {
        if (key == sk_Clear)
        {
            state = state_main;
            book_list_loaded = 0;
            book_list_chosen = 0;
            book_list_page = 0;
            return 1;
        }
        else if (key == sk_Enter)
        {
            if (book_list_count != 0)
            {
                reading_page = 0;
                state = state_reading;
            }
        }
        else if (key == sk_Left)
        {
            if (book_list_page > 0)
            {
                book_list_page--;
                book_list_loaded = false;
            }
            return 1;
        }
        else if (key == sk_Right)
        {
            if (book_list_page * book_list_perpage + book_list_count < book_list_total_count)
            {
                book_list_page++;
                book_list_loaded = false;
            }
            return 1;
        }
        else if (key == sk_Up)
        {
            if (book_list_chosen == 0)
            {
                book_list_chosen = book_list_count;
            }
            book_list_chosen--;
        }
        else if (key == sk_Down)
        {
            if (++book_list_chosen >= book_list_count)
            {
                book_list_chosen = 0;
            }
        }
        if (!book_list_loaded)
        {
            book_list_count = quickrdr_list_files(book_list_entries, book_list_page * book_list_perpage, book_list_perpage);
            if (book_list_count == 0 && book_list_page != 0)
            {
                book_list_page--;
            }
            else
            {
                book_list_loaded = true;
            }
        }
    }
    else if (state == state_reading)
    {
        if (key == sk_Clear)
        {
            state = state_main;
        }
        if (reading_book == NULL)
        {
            reading_book = quickrdr_open_book(book_list_entries[book_list_chosen].filename);
            if (reading_book == NULL)
            {
                show_alert("Failed to open book");
                state = state_main;
                return 1;
            }
        }
        if (key == sk_Left)
        {
            if (reading_page > 0)
            {
                reading_page--;
                if (reading_page_data != NULL)
                {
                    free(reading_page_data);
                    reading_page_data = NULL;
                }
                partial_redraw = 1;
                return 1;
            }
        }
        else if (key == sk_Right)
        {
            if (reading_page < reading_book->header.page_count - 1)
            {
                reading_page++;
                if (reading_page_data != NULL)
                {
                    free(reading_page_data);
                    reading_page_data = NULL;
                }
                partial_redraw = 1;
                return 1;
            }
        }
        if (reading_page_data == NULL)
        {
            reading_page_size = quickrdr_get_page_size(reading_book, reading_page, NULL);
            dbg_printf("Page size: %u\n", reading_page_size);
            reading_page_data = malloc(reading_page_size);
            if (reading_page_data == NULL)
            {
                show_alert("Failed to allocate memory");
                state = state_main;
                return 1;
            }
            quickrdr_read_page(reading_book, reading_page, reading_page_data);
            dbg_printf("Page data[0]: %u\n", reading_page_data[0]);
            uint8_t var = ti_Open("QKRDSAVE", "w");
            if (var != 0)
            {
                quickrdr_save_t save;
                quickrdr_get_book_filename(reading_book, save.filename);
                save.page = reading_page;
                ti_Write(&save, sizeof(save), 1, var);
                ti_SetArchiveStatus(1, var);
                ti_Close(var);
            }
        }
        else
        {
            partial_redraw = 1;
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
    if (!partial_redraw)
    {
        // top & bottom bar
        gfx_SetColor(COLOR_BAR_BG);
        gfx_FillRectangle_NoClip(0, 0, 320, 240);
        // main menu
        gfx_SetColor(COLOR_MAIN_BG);
        gfx_FillRectangle_NoClip(0, 24, 320, 196);
    }
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
        sprintf(buf, "%u-%u of %u", book_list_page * book_list_perpage + 1, book_list_page * book_list_perpage + book_list_count, book_list_total_count);
        unsigned int width = gfx_GetStringWidth(buf);
        gfx_PrintStringXY(buf, 320 - width - 8, 8);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Item [\x11\x10] Page [ENTER] Open [CLEAR] Back", 8, 226);
        if (!book_list_loaded)
        {
            gfx_SetTextFGColor(COLOR_MAIN_TEXT);
            gfx_SetTextBGColor(COLOR_MAIN_BG);
            gfx_PrintStringXY("Loading...", 8, 36);
        }
        else if (book_list_count == 0)
        {
            gfx_SetTextFGColor(COLOR_MAIN_TEXT);
            gfx_SetTextBGColor(COLOR_MAIN_BG);
            gfx_PrintStringXY("No books found", 8, 36);
        }
        else
        {
            for (int i = 0; i < book_list_count; i++)
            {
                gfx_SetColor(book_list_chosen == i ? COLOR_HIGHLIGHT_BG : COLOR_MAIN_BG);
                gfx_FillRectangle_NoClip(0, 30 + i * 32, 320, 20);
                gfx_SetTextFGColor(book_list_chosen == i ? COLOR_HIGHLIGHT_TEXT : COLOR_MAIN_TEXT);
                gfx_SetTextBGColor(book_list_chosen == i ? COLOR_HIGHLIGHT_BG : COLOR_MAIN_BG);
                gfx_PrintStringXY(book_list_entries[i].name, 24, 36 + i * 32);
                gfx_PrintString(" (");
                gfx_PrintString(book_list_entries[i].filename);
                gfx_PrintString(")");
                if (book_list_chosen == i)
                {
                    gfx_PrintStringXY(">", 8, 36 + i * 32);
                }
            }
        }
    }
    else if (state == state_reading)
    {
        // top bar text
        gfx_PrintStringXY(reading_book->header.name, 8, 8);
        if (reading_page_data != NULL)
        {
            sprintf(buf, "Page %u/%u", reading_page + 1, reading_book->header.page_count);
            unsigned int width = gfx_GetStringWidth(buf);
            gfx_PrintStringXY(buf, 320 - width - 8, 8);
            // bottom bar text
            gfx_PrintStringXY("[\x11\x10] Page [CLEAR] Back", 8, 226);
            gfx_SetTextFGColor(COLOR_MAIN_TEXT);
            gfx_SetTextBGColor(COLOR_MAIN_BG);
            gfx_SetTextXY(8, 32);
            uint8_t *data = reading_page_data;
            quickrdr_glyph_t *glyph = malloc(reading_book->header.font_glyph_size);
            if (glyph == NULL)
            {
                show_alert("Failed to allocate glyph");
                state = state_main;
                return;
            }
            unsigned int curX = 8, curY = 32;
            while (data < reading_page_data + reading_page_size)
            {
                uint16_t glyph_id;
                data += quickrdr_next_char(reading_book, data, &glyph_id);
                if (!quickrdr_read_glyph(reading_book, glyph_id, glyph))
                {
                    show_alert("Failed to read glyph");
                    state = state_main;
                    free(glyph);
                    return;
                }
                gfx_SetColor(COLOR_MAIN_TEXT);
                for (int i = 0; i < glyph->height; i++)
                {
                    for (int j = 0; j < glyph->width; j++)
                    {
                        if (getbit(glyph->data, i * glyph->width + j))
                        {
                            gfx_SetPixel(curX + j, curY + i);
                        }
                    }
                }
                curX += glyph->width;
                if (curX + glyph->width > 320)
                {
                    curX = 8;
                    curY += reading_book->header.line_height;
                }
            }
            free(glyph);
        }
        else
        {
            strcpy(buf, "Loading...");
            unsigned int width = gfx_GetStringWidth(buf);
            gfx_PrintStringXY(buf, 320 - width - 8, 8);
        }
    }
    else if (state == state_settings)
    {
        // top bar text
        gfx_PrintStringXY("QUICKRDR: Settings", 8, 8);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Item [ENTER] Select [CLEAR] Back", 8, 226);
    }
    else if (state == state_about)
    {
        // top bar text
        gfx_PrintStringXY("QUICKRDR: About", 8, 8);
        // bottom bar text
        gfx_PrintStringXY("[\x1e\x1f] Item [ENTER] Select [CLEAR] Back", 8, 226);
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

    // load the total count
    book_list_total_count = quickrdr_count_files();

    while (step())
    {
        dbg_printf("State: %u, partial redraw: %u\n", state, partial_redraw);
        if (partial_redraw)
        {
            gfx_BlitScreen();
        }
        draw();
        gfx_SwapDraw();
    }

    gfx_End();
    return 0;
}