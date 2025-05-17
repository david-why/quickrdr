#pragma once

#include <stdint.h>

typedef struct
{
    char magic[4]; // "QRDR"
    uint8_t version;
    uint8_t min_extension_byte;
    uint8_t line_height; // distance from baseline to baseline
    uint24_t font_glyphs_count;
    uint16_t font_glyphs_offset;
    uint8_t appvars_count;
    uint16_t appvars_offset;
} quickrdr_header_t;

typedef struct {
    uint16_t glyph_id;
    uint8_t width;
    uint8_t height;
    uint8_t data_size;
    uint8_t data[0]; // bitmap data
} quickrdr_glyph_t;

typedef struct {
    char name[8];
} quickrdr_appvar_t;
