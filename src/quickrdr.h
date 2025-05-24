#pragma once

#include <stdint.h>
#include <assert.h>

#pragma pack(push, 1)

typedef struct
{
    char magic[4];              // "QRDR"
    uint8_t version;            // version of the file format
    char name[16];              // name of the book (15 char max)
    uint24_t total_size;        // total size of the file
    uint8_t min_extension_byte; // minimum extension byte for the file, 0 for no extension
    uint8_t line_height;        // distance from baseline to baseline
    uint24_t font_glyph_count;  // number of glyphs in the font
    uint16_t font_glyph_size;    // size of each glyph in bytes (including header)
    uint24_t page_count;        // number of pages in the book
    // uint24_t page_offset[page_count];           // offset of each page in the file
    // quickrdr_glyph_t glyphs[font_glyph_count];  // glyphs in the font
    // uint8_t page_data[];                        // page data
} quickrdr_header_t;

static_assert(sizeof(quickrdr_header_t) == 34, "quickrdr_header_t size mismatch");

typedef struct
{
    uint16_t glyph_id;
    uint8_t width;
    uint8_t height;
    uint8_t data[0]; // bitmap data
} quickrdr_glyph_t;

typedef struct
{
    char filename[9]; // must end in "00"
    char name[16];
} quickrdr_book_t;

struct quickrdr_book_handle
{
    quickrdr_header_t header;
    uint8_t var;
    uint24_t current_var;
};
typedef struct quickrdr_book_handle *quickrdr_book_handle_t;

unsigned int quickrdr_list_files(quickrdr_book_t *book_list, unsigned int offset, unsigned int count);
unsigned int quickrdr_count_files(void);

quickrdr_book_handle_t quickrdr_open_book(const char *filename);
void quickrdr_close_book(quickrdr_book_handle_t book);
/**
 * @returns 1 on success, 0 on failure
 */
uint8_t quickrdr_read_glyph(quickrdr_book_handle_t book, uint16_t glyph_id, quickrdr_glyph_t *glyph);
uint24_t quickrdr_get_page_size(quickrdr_book_handle_t book, uint24_t page);
uint24_t quickrdr_read_page(quickrdr_book_handle_t book, uint24_t page, uint8_t *data);
uint8_t quickrdr_next_char(quickrdr_book_handle_t book, uint8_t *data, uint16_t *glyph_id);
void quickrdr_get_book_filename(quickrdr_book_handle_t book, char *filename);

#pragma pack(pop)
