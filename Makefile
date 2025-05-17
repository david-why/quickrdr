# ----------------------------
# Makefile Options
# ----------------------------

NAME = QUICKRDR
ICON = icon.png
DESCRIPTION = "E-Book reader"
COMPRESSED = NO
ARCHIVED = YES

HAS_PRINTF = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
