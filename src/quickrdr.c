#include "quickrdr.h"

#include <fileioc.h>
#include <debug.h>

#include <string.h>

#define CHUNK_SIZE 65460

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
    if (book->current_var != offset / CHUNK_SIZE)
    {
        char name[10];
        ti_GetName(name, book->var);
        ti_Close(book->var);
        size_t len = strlen(name);
        name[len - 2] = '0' + (offset / CHUNK_SIZE / 10);
        name[len - 1] = '0' + (offset / CHUNK_SIZE) % 10;
        book->var = ti_Open(name, "r");
        if (book->var == 0)
        {
            dbg_printf("Failed to open book var %s\n", name);
            return EOF;
        }
        book->current_var = offset / CHUNK_SIZE;
    }
    return ti_Seek(offset % CHUNK_SIZE, SEEK_SET, book->var);
}

static size_t book_read(quickrdr_book_handle_t book, void *buf, size_t size)
{
    size_t read = 0;
    while (size)
    {
        size_t this_read = ti_Read(buf + read, 1, size - read, book->var);
        if (!this_read)
        {
            return read;
        }
        read += this_read;
        book_seek(book, book->current_var * CHUNK_SIZE + ti_Tell(book->var));
    }
    return read;
}

quickrdr_book_handle_t quickrdr_open_book(const char *filename)
{
    size_t len = strlen(filename);
    if (len < 3 || strcmp(filename + len - 2, "00") != 0)
    {
        dbg_printf("Filename %s does not end with 00\n", filename);
        return NULL;
    }
    quickrdr_book_handle_t book = calloc(1, sizeof(struct quickrdr_book_handle));
    if (book == NULL)
    {
        dbg_printf("Failed to allocate memory for book handle\n");
        return NULL;
    }
    book->var = ti_Open(filename, "r");
    if (book->var == 0)
    {
        dbg_printf("Failed to open book var %s\n", filename);
        goto err_free;
    }
    ti_Seek(0, SEEK_SET, book->var);
    size_t read = book_read(book, &book->header, sizeof(book->header));
    if (read != sizeof(book->header))
    {
        dbg_printf("Read failed, read %u bytes\n", read);
        goto err_close;
    }
    if (memcmp(book->header.magic, "QRDR", 4) != 0)
    {
        dbg_printf("Invalid magic data\n");
        goto err_close;
    }
    if (book->header.version != 1)
    {
        dbg_printf("Invalid version %u\n", book->header.version);
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

#define dbg(var) dbg_printf("%s = %u\n", #var, var)

static uint24_t book_calculate_glyph_offset(quickrdr_book_handle_t book, uint16_t glyph_id)
{
    if (glyph_id < 256U)
    {
        return sizeof(book->header) + book->header.page_count * sizeof(uint24_t) +
               (glyph_id - 1) * book->header.font_glyph_size;
    }
    else
    {
        // two byte glyphs
        uint8_t extension_byte = glyph_id >> 8;
        dbg(book->header.min_extension_byte);
        dbg(extension_byte);
        return sizeof(book->header) + book->header.page_count * sizeof(uint24_t) +
               (book->header.min_extension_byte - 1) * book->header.font_glyph_size +
               (extension_byte - book->header.min_extension_byte) * 256U * book->header.font_glyph_size +
               (glyph_id & 0xFF) * book->header.font_glyph_size;
    }
}

uint8_t quickrdr_read_glyph(quickrdr_book_handle_t book, uint16_t glyph_id, quickrdr_glyph_t *glyph)
{
    uint24_t offset = book_calculate_glyph_offset(book, glyph_id);
    dbg_printf("Reading glyph %u, offset %u\n", glyph_id, offset);
    if (book_seek(book, offset) == EOF)
    {
        dbg_printf("Failed to seek to glyph %u\n", glyph_id);
        return 0;
    }
    // size_t read = ti_Read(glyph, sizeof(quickrdr_glyph_t) + book->header.font_glyph_size, 1, book->var);
    size_t read = book_read(book, glyph, book->header.font_glyph_size);
    if (read != book->header.font_glyph_size)
    {
        dbg_printf("Failed to read glyph %u, read %u bytes\n", glyph_id, read);
        return 0;
    }
    if (glyph->glyph_id != glyph_id)
    {
        dbg_printf("Glyph ID mismatch, expected %u, got %u\n", glyph_id, glyph->glyph_id);
        return 0;
    }
    return 1;
}

static uint24_t quickrdr_get_page_offset(quickrdr_book_handle_t book, uint24_t page)
{
    if (page >= book->header.page_count)
    {
        dbg_printf("Page %u out of bounds\n", page);
        return 0;
    }
    uint24_t offset = sizeof(quickrdr_header_t) + page * sizeof(uint24_t);
    if (book_seek(book, offset) == EOF)
    {
        dbg_printf("Failed to seek to page %u\n", page);
        return 0;
    }
    uint24_t page_offset;
    size_t read = book_read(book, &page_offset, sizeof(page_offset));
    if (read != sizeof(page_offset))
    {
        dbg_printf("Failed to read page offset for %u\n", page);
        return 0;
    }
    return page_offset;
}

uint24_t quickrdr_get_page_size(quickrdr_book_handle_t book, uint24_t page)
{
    uint24_t page_offset = quickrdr_get_page_offset(book, page);
    if (!page_offset)
    {
        return 0;
    }
    if (page == book->header.page_count - 1)
    {
        dbg_printf("Last page %u, offset %u, total size %u\n", page, page_offset, book->header.total_size);
        return book->header.total_size - page_offset;
    }
    uint24_t next_page_offset = quickrdr_get_page_offset(book, page + 1);
    if (!next_page_offset)
    {
        return 0;
    }
    return next_page_offset - page_offset;
}

uint24_t quickrdr_read_page(quickrdr_book_handle_t book, uint24_t page, uint8_t *data)
{
    uint24_t page_offset = quickrdr_get_page_offset(book, page);
    size_t size = quickrdr_get_page_size(book, page);
    if (size == 0)
    {
        return 0;
    }
    if (page >= book->header.page_count)
    {
        return 0;
    }
    dbg_printf("Reading page %u, offset %u, size %u\n", page, page_offset, size);
    if (book_seek(book, page_offset) == EOF)
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

void quickrdr_get_book_filename(quickrdr_book_handle_t book, char *filename)
{
    if (book == NULL)
    {
        filename[0] = '\0';
        return;
    }
    ti_GetName(filename, book->var);
    size_t len = strlen(filename);
    filename[len - 2] = '0';
    filename[len - 1] = '0';
    filename[len] = '\0';
}
