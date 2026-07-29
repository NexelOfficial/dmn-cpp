#pragma once

#include <cstdint>
#include <optional>
#include <array>
#include <string>

struct UNIVERSALNOTEID_tag;
struct ORIGINATORID_tag;

namespace dmn {
struct unid {
  std::array<uint32_t, 2> file;
  std::array<uint32_t, 2> note;

  /// Convert a hexadecimal UNID string to a unid instance.
  ///
  /// \param str UNID string to convert.
  /// \return Instance of `dmn::unid`, if available.
  [[nodiscard]] static auto from_string(std::string_view str) -> std::optional<unid>;

  /// Convert this UNID to its hexadecimal string representation.
  ///
  /// \return Uppercase hexadecimal UNID string.
  [[nodiscard]] auto to_string() const -> std::string;

  /// Cast to raw UNIVERSALNOTEID structure.
  [[nodiscard]] auto as_raw_unid() noexcept -> UNIVERSALNOTEID_tag*;

  auto operator==(unid other) const noexcept -> bool {
    return file == other.file && note == other.note;
  }
};

struct oid {
  dmn::unid universalid;
  uint32_t sequence;
  std::array<uint32_t, 2> sequence_time;

  auto operator==(oid other) const noexcept -> bool {
    return universalid == other.universalid && sequence == other.sequence &&
           sequence_time == other.sequence_time;
  }
};

struct note_id {
#ifdef W32
  using value_t = unsigned long;
#else
  using value_t = unsigned int;
#endif

  value_t value;

  constexpr note_id(value_t id) noexcept : value(id) {};
  constexpr note_id() noexcept : value(0) {};

  auto operator==(note_id other) const noexcept -> bool { return value == other.value; }

  /// Get pointer to underlying id value.
  constexpr auto data() noexcept -> value_t* { return &value; }
};
}  // namespace dmn

// NOLINTBEGIN
template <>
struct std::hash<dmn::unid> {
  auto operator()(const dmn::unid& value) const noexcept -> size_t {
    return std::hash<uint32_t>{}(value.file[0]) ^ (std::hash<uint32_t>{}(value.file[1]) << 1) ^
           (std::hash<uint32_t>{}(value.note[0]) << 2) ^
           (std::hash<uint32_t>{}(value.note[1]) << 3);
  }
};

template <>
struct std::hash<dmn::oid> {
  auto operator()(const dmn::oid& value) const noexcept -> size_t {
    return std::hash<dmn::unid>{}(value.universalid) ^
           (std::hash<uint32_t>{}(value.sequence) << 1) ^
           (std::hash<uint32_t>{}(value.sequence_time[0]) << 2) ^
           (std::hash<uint32_t>{}(value.sequence_time[1]) << 3);
  }
};

template <>
struct std::hash<dmn::note_id> {
  auto operator()(const dmn::note_id& value) const noexcept -> size_t {
    return std::hash<dmn::note_id::value_t>{}(value.value);
  }
};
// NOLINTEND