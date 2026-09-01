#include "dmn/design/font.hpp"

#include <domino/global.h>
#include <domino/fontid.h>

using dmn::design::font;

static_assert(static_cast<int>(font::style::bold) == ISBOLD);
static_assert(static_cast<int>(font::style::italic) == ISITALIC);
static_assert(static_cast<int>(font::style::underline) == ISUNDERLINE);
static_assert(static_cast<int>(font::style::strikethrough) == ISSTRIKEOUT);

auto font::get_font_id() const -> id {
  auto font = FontSetColor(FontSetFaceID(FontSetSize(NULLFONTID, size_), FONT_FACE_SWISS), color_);
  for (auto sl : styles_) {
    if (sl != style::none) {
      font |= (static_cast<int>(sl) << FONT_STYLE_SHIFT);
    }
  }
  return {font};
}