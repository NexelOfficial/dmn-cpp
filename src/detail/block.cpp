#include "dmn/detail/block.hpp"

#include <domino/global.h>
#include <domino/pool.h>

using dmn::detail::block_id;

static_assert(sizeof(block_id) == sizeof(BLOCKID));
static_assert(alignof(block_id) == alignof(BLOCKID));