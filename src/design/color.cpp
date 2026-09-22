#include "dmn/design/color.hpp"

#include <domino/global.h>
#include <domino/colorid.h>

#include <utility>

using dmn::design::color;

static_assert(std::to_underlying(color::black) == NOTES_COLOR_BLACK);
static_assert(std::to_underlying(color::white) == NOTES_COLOR_WHITE);
static_assert(std::to_underlying(color::red) == NOTES_COLOR_RED);
static_assert(std::to_underlying(color::green) == NOTES_COLOR_GREEN);
static_assert(std::to_underlying(color::blue) == NOTES_COLOR_BLUE);
static_assert(std::to_underlying(color::magenta) == NOTES_COLOR_MAGENTA);
static_assert(std::to_underlying(color::yellow) == NOTES_COLOR_YELLOW);
static_assert(std::to_underlying(color::cyan) == NOTES_COLOR_CYAN);
static_assert(std::to_underlying(color::dark_red) == NOTES_COLOR_DKRED);
static_assert(std::to_underlying(color::dark_green) == NOTES_COLOR_DKGREEN);
static_assert(std::to_underlying(color::dark_blue) == NOTES_COLOR_DKBLUE);
static_assert(std::to_underlying(color::dark_magenta) == NOTES_COLOR_DKMAGENTA);
static_assert(std::to_underlying(color::dark_yellow) == NOTES_COLOR_DKYELLOW);
static_assert(std::to_underlying(color::dark_cyan) == NOTES_COLOR_DKCYAN);
static_assert(std::to_underlying(color::gray) == NOTES_COLOR_GRAY);
static_assert(std::to_underlying(color::light_gray) == NOTES_COLOR_LTGRAY);