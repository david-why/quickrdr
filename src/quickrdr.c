#include "quickrdr.h"

#include <fileioc.h>

#include <string.h>

struct quickrdr_book_handle
{
    quickrdr_header_t header;
    uint8_t var;
    uint24_t current_var;
};

static char *find_next_appvar(void **search_pos)
{
    char *detected_name;
    while ((detected_name = ti_Detect(search_pos, NULL)))
    {
        char buf[4] = {0};
        uint8_t var = ti_Open(detected_name, "r");
        size_t read = ti_Read(buf, 4, 1, var);
        ti_Close(var);
        if (read && memcmp(buf, "QRDR", 4) == 0)
        {
            return detected_name;
        }
    }
    return NULL;
}

unsigned int quickrdr_list_files(quickrdr_book_t *book_list, unsigned int offset, unsigned int count)
{
    void *search_pos = NULL;
    while (offset--)
    {
        char *detected_name = find_next_appvar(&search_pos);
        if (detected_name == NULL)
        {
            return 0;
        }
    }
    for (unsigned int i = 0; i < count; i++)
    {
        char *detected_name = find_next_appvar(&search_pos);
        if (detected_name == NULL)
        {
            return i;
        }
        uint8_t var = ti_Open(detected_name, "r");
        if (var != 0)
        {
            ti_Seek(offsetof(quickrdr_header_t, name), SEEK_SET, var);
            ti_Read(book_list[i].name, sizeof(book_list[i].name), 1, var);
            ti_Close(var);
        }
        else
        {
            strncpy(book_list[i].name, detected_name, sizeof(book_list[i].name));
        }
        strncpy(book_list[i].filename, detected_name, sizeof(book_list[i].filename));
    }
    return count;
}

unsigned int quickrdr_count_files(void)
{
    void *search_pos = NULL;
    unsigned int count = 0;
    while (find_next_appvar(&search_pos))
    {
        count++;
    }
    return count;
}

// book opening, reading, etc.

static int book_seek(quickrdr_book_handle_t book, uint24_t offset)
{
    if (book->current_var != offset / 65536)
    {
        char name[10];
        ti_GetName(name, book->var);
        ti_Close(book->var);
        size_t len = strlen(name);
        name[len - 2] = '0' + (offset / 65536 / 10);
        name[len - 1] = '0' + (offset / 65536) % 10;
        book->var = ti_Open(name, "r");
        if (book->var == 0)
        {
            return EOF;
        }
        book->current_var = offset / 65536;
    }
    return ti_Seek(offset % 65536, SEEK_CUR, book->var);
}

static size_t book_read(quickrdr_book_handle_t book, void *buf, size_t size)
{
    size_t read = 0;
    while (size)
    {
        size_t this_read = ti_Read(buf + read, 1, size, book->var);
        if (!this_read)
        {
            return read;
        }
        read += this_read;
        book_seek(book, book->current_var * 65536 + ti_Tell(book->var));
    }
    return read;
}

quickrdr_book_handle_t quickrdr_open_book(const char *filename)
{
    size_t len = strlen(filename);
    if (len < 3 || strcmp(filename + len - 2, "00") != 0)
    {
        return NULL;
    }
    quickrdr_book_handle_t book = malloc(sizeof(struct quickrdr_book_handle));
    if (book == NULL)
    {
        return NULL;
    }
    book->var = ti_Open(filename, "r");
    if (book->var == 0)
    {
        goto err_free;
    }
    ti_Seek(0, SEEK_SET, book->var);
    size_t read = book_read(book, &book->header, sizeof(book->header));
    if (read != sizeof(book->header))
    {
        goto err_close;
    }
    if (memcmp(book->header.magic, "QRDR", 4) != 0)
    {
        goto err_close;
    }
    if (book->header.version != 1)
    {
        goto err_close;
    }
    book->current_var = 0;

    return book;

err_close:
    ti_Close(book->var);
err_free:
    free(book);
    return NULL;
}

void quickrdr_close_book(quickrdr_book_handle_t book)
{
    if (book == NULL)
    {
        return;
    }
    ti_Close(book->var);
    free(book);
}

static uint24_t book_calculate_glyph_offset(quickrdr_book_handle_t book, uint16_t glyph_id)
{
    if (glyph_id < 256U)
    {
        return sizeof(book->header) + book->header.page_count * sizeof(uint24_t) + glyph_id * book->header.font_glyph_size;
    }
    else
    {
        // two byte glyphs
        uint8_t extension_byte = glyph_id >> 8;
        return sizeof(book->header) + book->header.page_count * sizeof(uint24_t) + (book->header.min_extension_byte + extension_byte) * 256U * book->header.font_glyph_size + (glyph_id & 0xFF) * book->header.font_glyph_size;
    }
}

uint8_t quickrdr_read_glyph(quickrdr_book_handle_t book, uint16_t glyph_id, quickrdr_glyph_t *glyph)
{
    uint24_t offset = book_calculate_glyph_offset(book, glyph_id);
    if (book_seek(book, offset) == EOF)
    {
        return 0;
    }
    // size_t read = ti_Read(glyph, sizeof(quickrdr_glyph_t) + book->header.font_glyph_size, 1, book->var);
    size_t read = book_read(book, glyph, sizeof(quickrdr_glyph_t) + book->header.font_glyph_size);
    if (read != sizeof(quickrdr_glyph_t) + book->header.font_glyph_size)
    {
        return 0;
    }
    if (glyph->glyph_id != glyph_id)
    {
        return 0;
    }
    return 1;
}

uint24_t quickrdr_get_page_size(quickrdr_book_handle_t book, uint24_t page)
{
    if (page >= book->header.page_count)
    {
        return 0;
    }
    uint24_t offset = sizeof(quickrdr_header_t) + page * sizeof(uint24_t);
    if (book_seek(book, offset) == EOF)
    {
        return 0;
    }
    uint24_t page_offset;
    // size_t read = ti_Read(&page_offset, sizeof(page_offset), 1, book->var);
    size_t read = book_read(book, &page_offset, sizeof(page_offset));
    if (read != sizeof(page_offset))
    {
        return 0;
    }
    if (page == book->header.page_count - 1)
    {
        return book->header.total_size - page_offset;
    }
    uint24_t next_page_offset;
    // read = ti_Read(&next_page_offset, sizeof(next_page_offset), 1, book->var);
    read = book_read(book, &next_page_offset, sizeof(next_page_offset));
    if (read != sizeof(next_page_offset))
    {
        return 0;
    }
    uint24_t size = next_page_offset - page_offset;
    if (size > book->header.line_height)
    {
        size = book->header.line_height;
    }
    if (size == 0)
    {
        return 0;
    }
    return size;
}

uint24_t quickrdr_read_page(quickrdr_book_handle_t book, uint24_t page, uint8_t *data)
{
    size_t size = quickrdr_get_page_size(book, page);
    if (size == 0)
    {
        return 0;
    }
    if (page >= book->header.page_count)
    {
        return 0;
    }
    uint24_t offset = sizeof(quickrdr_header_t) + book->header.page_count * sizeof(uint24_t) + page * book->header.line_height;
    if (book_seek(book, offset) == EOF)
    {
        return 0;
    }
    size_t read = book_read(book, data, size);
    if (read != size)
    {
        return 0;
    }
    return size;
}

uint8_t quickrdr_next_char(quickrdr_book_handle_t book, uint8_t *data, uint16_t *glyph_id)
{
    uint8_t byte = *data++;
    if (book->header.min_extension_byte && byte >= book->header.min_extension_byte)
    {
        *glyph_id = (byte << 8) | *data++;
        return 2;
    }
    else
    {
        *glyph_id = byte;
        return 1;
    }
}
