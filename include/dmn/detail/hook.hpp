#pragma once

#include <optional>

#include "dmn/detail/uhandle.hpp"

namespace dmn::detail::hook {
/// Install the Domino memory hooks.
void install();

/// Uninstall the Domino memory hooks.
void uninstall();

/// Check whether a memory handle is known to be valid.
///
/// \param handle Handle to check.
/// \param generation Optional generation that the block must come from.
[[nodiscard]] auto is_valid(detail::dhandle_t handle, std::optional<uint32_t> generation = {})
  -> bool;

/// Get the current generation for a memory handle.
[[nodiscard]] auto get_generation(detail::dhandle_t handle) -> uint32_t;
}  // namespace dmn::detail::hook