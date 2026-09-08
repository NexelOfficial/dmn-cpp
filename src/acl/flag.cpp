#include "dmn/acl/flag.hpp"

#include <domino/global.h>
#include <domino/acl.h>

using dmn::acl::flag;
using dmn::acl::level;
using dmn::acl::principal_type;

static_assert(static_cast<uint16_t>(principal_type::server) == ACL_FLAG_SERVER);
static_assert(static_cast<uint16_t>(principal_type::person) == ACL_FLAG_PERSON);
static_assert(static_cast<uint16_t>(principal_type::group) == ACL_FLAG_GROUP);

static_assert(static_cast<uint16_t>(level::noaccess) == ACL_LEVEL_NOACCESS);
static_assert(static_cast<uint16_t>(level::depositor) == ACL_LEVEL_DEPOSITOR);
static_assert(static_cast<uint16_t>(level::reader) == ACL_LEVEL_READER);
static_assert(static_cast<uint16_t>(level::author) == ACL_LEVEL_AUTHOR);
static_assert(static_cast<uint16_t>(level::editor) == ACL_LEVEL_EDITOR);
static_assert(static_cast<uint16_t>(level::designer) == ACL_LEVEL_DESIGNER);
static_assert(static_cast<uint16_t>(level::manager) == ACL_LEVEL_MANAGER);

static_assert(static_cast<uint16_t>(flag::author_no_create) == ACL_FLAG_AUTHOR_NOCREATE);
static_assert(static_cast<uint16_t>(flag::no_delete) == ACL_FLAG_NODELETE);
static_assert(static_cast<uint16_t>(flag::create_personal_agent) == ACL_FLAG_CREATE_PRAGENT);
static_assert(static_cast<uint16_t>(flag::create_personal_folder) == ACL_FLAG_CREATE_PRFOLDER);
static_assert(static_cast<uint16_t>(flag::create_folder) == ACL_FLAG_CREATE_FOLDER);
static_assert(static_cast<uint16_t>(flag::create_lotusscript) == ACL_FLAG_CREATE_LOTUSSCRIPT);
static_assert(static_cast<uint16_t>(flag::public_reader) == ACL_FLAG_PUBLICREADER);
static_assert(static_cast<uint16_t>(flag::public_writer) == ACL_FLAG_PUBLICWRITER);
static_assert(static_cast<uint16_t>(flag::monitors_disallowed) == ACL_FLAG_MONITORS_DISALLOWED);
static_assert(static_cast<uint16_t>(flag::no_replicate) == ACL_FLAG_NOREPLICATE);