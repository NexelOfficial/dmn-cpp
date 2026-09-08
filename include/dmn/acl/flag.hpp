#pragma once

#include <cstdint>

namespace dmn::acl {

// NOLINTNEXTLINE(performance-enum-size)
enum class principal_type : uint16_t {
  unspecified = 0x0000,
  server = 0x0002,
  person = 0x0020,
  group = 0x0040,
};

enum class level : uint8_t { noaccess, depositor, reader, author, editor, designer, manager };

enum class flag : uint16_t {
  author_no_create = 0x0001,
  no_delete = 0x0004,
  create_personal_agent = 0x0008,
  create_personal_folder = 0x0010,
  create_folder = 0x0080,
  create_lotusscript = 0x0100,
  public_reader = 0x0200,
  public_writer = 0x0400,
  monitors_disallowed = 0x0800,
  no_replicate = 0x1000,
};
}  // namespace dmn::acl