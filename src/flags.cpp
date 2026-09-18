#include "dmn/flags.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>

using dmn::item_flag;

static_assert(static_cast<uint16_t>(item_flag::sign) == ITEM_SIGN);
static_assert(static_cast<uint16_t>(item_flag::seal) == ITEM_SEAL);
static_assert(static_cast<uint16_t>(item_flag::summary) == ITEM_SUMMARY);
static_assert(static_cast<uint16_t>(item_flag::readwriters) == ITEM_READWRITERS);
static_assert(static_cast<uint16_t>(item_flag::names) == ITEM_NAMES);
static_assert(static_cast<uint16_t>(item_flag::placeholder) == ITEM_PLACEHOLDER);
static_assert(static_cast<uint16_t>(item_flag::protect) == ITEM_PROTECTED);
static_assert(static_cast<uint16_t>(item_flag::readers) == ITEM_READERS);