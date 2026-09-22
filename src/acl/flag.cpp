#include "dmn/acl/flag.hpp"

#include <domino/global.h>
#include <domino/acl.h>

#include <utility>

using dmn::acl::flag;
using dmn::acl::level;
using dmn::acl::principal_type;

static_assert(std::to_underlying(principal_type::server) == ACL_FLAG_SERVER);
static_assert(std::to_underlying(principal_type::person) == ACL_FLAG_PERSON);
static_assert(std::to_underlying(principal_type::group) == ACL_FLAG_GROUP);

static_assert(std::to_underlying(level::noaccess) == ACL_LEVEL_NOACCESS);
static_assert(std::to_underlying(level::depositor) == ACL_LEVEL_DEPOSITOR);
static_assert(std::to_underlying(level::reader) == ACL_LEVEL_READER);
static_assert(std::to_underlying(level::author) == ACL_LEVEL_AUTHOR);
static_assert(std::to_underlying(level::editor) == ACL_LEVEL_EDITOR);
static_assert(std::to_underlying(level::designer) == ACL_LEVEL_DESIGNER);
static_assert(std::to_underlying(level::manager) == ACL_LEVEL_MANAGER);

static_assert(std::to_underlying(flag::author_no_create) == ACL_FLAG_AUTHOR_NOCREATE);
static_assert(std::to_underlying(flag::no_delete) == ACL_FLAG_NODELETE);
static_assert(std::to_underlying(flag::create_personal_agent) == ACL_FLAG_CREATE_PRAGENT);
static_assert(std::to_underlying(flag::create_personal_folder) == ACL_FLAG_CREATE_PRFOLDER);
static_assert(std::to_underlying(flag::create_folder) == ACL_FLAG_CREATE_FOLDER);
static_assert(std::to_underlying(flag::create_lotusscript) == ACL_FLAG_CREATE_LOTUSSCRIPT);
static_assert(std::to_underlying(flag::public_reader) == ACL_FLAG_PUBLICREADER);
static_assert(std::to_underlying(flag::public_writer) == ACL_FLAG_PUBLICWRITER);
static_assert(std::to_underlying(flag::monitors_disallowed) == ACL_FLAG_MONITORS_DISALLOWED);
static_assert(std::to_underlying(flag::no_replicate) == ACL_FLAG_NOREPLICATE);