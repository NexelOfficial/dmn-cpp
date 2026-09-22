#include "dmn/design/font.hpp"

#include <domino/global.h>
#include <domino/fontid.h>

#include <utility>

using dmn::design::font;

static_assert(std::to_underlying(font::style::bold) == ISBOLD);
static_assert(std::to_underlying(font::style::italic) == ISITALIC);
static_assert(std::to_underlying(font::style::underline) == ISUNDERLINE);
static_assert(std::to_underlying(font::style::strikethrough) == ISSTRIKEOUT);

auto font::get_font_id() const -> id {
  auto font = FontSetColor(FontSetFaceID(FontSetSize(NULLFONTID, size_), FONT_FACE_SWISS), color_);
  for (auto sl : styles_) {
    if (sl != style::none) {
      font |= (std::to_underlying(sl) << FONT_STYLE_SHIFT);
    }
  }
  return {font};
}