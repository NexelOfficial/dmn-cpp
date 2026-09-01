#include "dmn/design/color.hpp"

#include <domino/global.h>
#include <domino/colorid.h>

using dmn::design::color;

static_assert(static_cast<uint16_t>(color::black) == NOTES_COLOR_BLACK);
static_assert(static_cast<uint16_t>(color::white) == NOTES_COLOR_WHITE);
static_assert(static_cast<uint16_t>(color::red) == NOTES_COLOR_RED);
static_assert(static_cast<uint16_t>(color::green) == NOTES_COLOR_GREEN);
static_assert(static_cast<uint16_t>(color::blue) == NOTES_COLOR_BLUE);
static_assert(static_cast<uint16_t>(color::magenta) == NOTES_COLOR_MAGENTA);
static_assert(static_cast<uint16_t>(color::yellow) == NOTES_COLOR_YELLOW);
static_assert(static_cast<uint16_t>(color::cyan) == NOTES_COLOR_CYAN);
static_assert(static_cast<uint16_t>(color::dark_red) == NOTES_COLOR_DKRED);
static_assert(static_cast<uint16_t>(color::dark_green) == NOTES_COLOR_DKGREEN);
static_assert(static_cast<uint16_t>(color::dark_blue) == NOTES_COLOR_DKBLUE);
static_assert(static_cast<uint16_t>(color::dark_magenta) == NOTES_COLOR_DKMAGENTA);
static_assert(static_cast<uint16_t>(color::dark_yellow) == NOTES_COLOR_DKYELLOW);
static_assert(static_cast<uint16_t>(color::dark_cyan) == NOTES_COLOR_DKCYAN);
static_assert(static_cast<uint16_t>(color::gray) == NOTES_COLOR_GRAY);
static_assert(static_cast<uint16_t>(color::light_gray) == NOTES_COLOR_LTGRAY);