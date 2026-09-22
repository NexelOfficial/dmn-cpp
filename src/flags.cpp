#include "dmn/flags.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <utility>

using dmn::item_flag;

static_assert(std::to_underlying(item_flag::sign) == ITEM_SIGN);
static_assert(std::to_underlying(item_flag::seal) == ITEM_SEAL);
static_assert(std::to_underlying(item_flag::summary) == ITEM_SUMMARY);
static_assert(std::to_underlying(item_flag::readwriters) == ITEM_READWRITERS);
static_assert(std::to_underlying(item_flag::names) == ITEM_NAMES);
static_assert(std::to_underlying(item_flag::placeholder) == ITEM_PLACEHOLDER);
static_assert(std::to_underlying(item_flag::protect) == ITEM_PROTECTED);
static_assert(std::to_underlying(item_flag::readers) == ITEM_READERS);