#include "quickrdr.h"

#include <fileioc.h>

#include <string.h>

char *find_next_appvar(void **search_pos)
{
    char *detected_name;
    while (detected_name = ti_DetectVar(search_pos, NULL, OS_TYPE_APPVAR))
    {
        static char buf[4];
        uint8_t var = ti_OpenVar(detected_name, "r", OS_TYPE_APPVAR);
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
        strncpy(book_list[i].name, detected_name, sizeof(book_list[i].name));
    }
    return count;
}
