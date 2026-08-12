#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "dmn/database.hpp"
#include "dmn/object.hpp"
#include "dmn/error.hpp"
#include "dmn/unid.hpp"
#include "dmn/list.hpp"
#include "dmn/note.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

namespace {
auto get_item_value(const dmn::database& db, dmn::note_id nid) -> std::optional<dmn::object> {
  auto note = db.get_note(nid);
  if (!note) {
    throw std::runtime_error("Failed to open note for item value");
  }

  return note->get<dmn::object>("Subject");
};

auto create_temp_attachment() -> fs::path {
  const auto path = fs::temp_directory_path() / "workflow_addin_note_test_attachment.txt";
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to create temporary attachment file");
  }
  out << "Attachment payload\nLine two\n";
  out.flush();
  return path;
}
}  // namespace

TEST_CASE("note database lifecycle and item operations", "[nsf][note]") {
  auto [db, name] = utils::random_database();
  REQUIRE(db->get_path().find(name) != std::string::npos);
  REQUIRE(db->try_get_handle().has_value());

  auto primary = db->create_note();
  REQUIRE(primary.info<dmn::info::note_id>() == dmn::note_id{});
  REQUIRE(primary.try_get_handle().has_value());
  REQUIRE(primary.get_database().get_path().find(name) != std::string::npos);

  const std::string subject = utils::random_small_string() + "UTF-8: 🐶";
  const std::string huge_text = utils::random_large_string();
  const std::string multiline = "Line 1\nLine 2\r\nLine 3 with punctuation !@#$%^&*()";
  const std::string empty_text = "";
  const std::string replacement_subject = "Updated subject";
  const std::string weird_field_name = "Weird_Field_01";
  const std::string attachment_name = "workflow_addin_test.txt";
  const auto attachment_path = create_temp_attachment();
  const auto now = dmn::time_date::from_time_point(std::chrono::system_clock::now());

  primary.set("Subject", subject);
  primary.set("EmptyText", empty_text);
  primary.set("MultilineText", multiline);
  primary.set("NumericValue", 42.5);
  primary.set("ZeroValue", 0.0);
  primary.set("OneValue", 1.0);
  primary.set("DateValue", now.value());

  dmn::list tags{};
  tags.push_back("alpha");
  tags.push_back("beta");
  tags.push_back("gamma");
  primary.set("Tags", tags);
  primary.set(weird_field_name, std::string{"value"});

  REQUIRE_THROWS_AS(primary.set("HugeText", huge_text), dmn::error);
  REQUIRE_THROWS_AS(primary.compute_with_form(), dmn::error);
  REQUIRE_NOTHROW(primary.embed_element(attachment_name, attachment_path.string()));
  REQUIRE_NOTHROW(primary.embed_element(attachment_path.string()));
  REQUIRE_NOTHROW(primary.save(true));

  REQUIRE(primary.has("Subject"));
  REQUIRE(primary.has("EmptyText"));
  REQUIRE(primary.has("MultilineText"));
  REQUIRE(primary.has("NumericValue"));
  REQUIRE(primary.has("Tags"));
  REQUIRE_FALSE(primary.has("MissingField"));

  REQUIRE(primary.get_type("Subject") == dmn::type::text);
  REQUIRE(primary.get_type("EmptyText") == dmn::type::text);
  REQUIRE(primary.get_type("MultilineText") == dmn::type::text);
  REQUIRE(primary.get_type("NumericValue") == dmn::type::number);
  REQUIRE(primary.get_type("Tags") == dmn::type::text_list);
  REQUIRE(primary.get_type("DateValue") == dmn::type::time);
  REQUIRE(primary.get_type("MissingField") == dmn::type::invalid_or_unknown);

  const auto numeric_text_value = primary.get<std::string>("NumericValue");
  const auto numeric_value = primary.get<double>("NumericValue");
  const auto numeric_int_value = primary.get<int>("NumericValue");
  const auto one_bool_value = primary.get<bool>("OneValue");
  const auto zero_bool_value = primary.get<bool>("ZeroValue");
  const auto tags_value = primary.get<dmn::list>("Tags");
  const auto date_value = primary.get<dmn::time_date>("DateValue");
  const auto raw_numeric_value = primary.get<dmn::object>("NumericValue");

  REQUIRE_FALSE(numeric_text_value.has_value());
  REQUIRE(numeric_value.has_value());
  REQUIRE(date_value.value() == now.value());
  REQUIRE(numeric_value.value() == 42.5);
  REQUIRE(numeric_int_value.has_value());
  REQUIRE(numeric_int_value.value() == 42);
  REQUIRE(one_bool_value.has_value());
  REQUIRE(one_bool_value.value());
  REQUIRE(zero_bool_value.has_value());
  REQUIRE_FALSE(zero_bool_value.value());
  REQUIRE(tags_value.has_value());
  REQUIRE(tags_value.value().size() == 3);
  REQUIRE(raw_numeric_value.has_value());
  REQUIRE(raw_numeric_value->try_as<double>().value() == 42.5);

  const auto subject_item_value = get_item_value(*db, primary.info<dmn::info::note_id>());
  REQUIRE(subject_item_value.has_value());
  REQUIRE(subject_item_value->is<std::string>());
  REQUIRE_FALSE(subject_item_value->empty());

  const auto subject_value = subject_item_value->try_as<std::string>();
  REQUIRE(subject_value.has_value());
  REQUIRE(subject_value.value() == subject);

  const auto all_items = primary.items(std::nullopt);
  REQUIRE(all_items.contains("Subject"));
  REQUIRE(all_items.contains("NumericValue"));
  REQUIRE(all_items.contains("Tags"));
  REQUIRE(all_items.contains(weird_field_name));
  REQUIRE(all_items.at("Subject").is<std::string>());
  REQUIRE(all_items.at("NumericValue").is<double>());
  REQUIRE(all_items.at("Tags").is<dmn::list>());

  const auto filtered_items = primary.items(std::regex{R"(^.*Value$)"});
  REQUIRE(filtered_items.contains("NumericValue"));
  REQUIRE(filtered_items.contains("ZeroValue"));
  REQUIRE(filtered_items.contains("OneValue"));
  REQUIRE(filtered_items.contains("DateValue"));
  REQUIRE_FALSE(filtered_items.contains("Subject"));
  REQUIRE_FALSE(filtered_items.contains("Tags"));

  primary.set("Subject", replacement_subject);
  REQUIRE(primary.get<std::string>("Subject").value() == replacement_subject);

  primary.erase("EmptyText");
  REQUIRE_FALSE(primary.has("EmptyText"));
  REQUIRE(primary.get<std::string>("EmptyText") == std::nullopt);

  REQUIRE_NOTHROW(primary.save(true));

  const auto original_noteid = primary.info<dmn::info::note_id>();
  const auto original_unid = primary.info<dmn::info::unid>();
  const auto string_unid = original_unid.to_string();
  REQUIRE(string_unid.size() == 32);
  const auto parsed_unid = dmn::unid::from_string(string_unid);
  REQUIRE(parsed_unid.has_value());
  REQUIRE(parsed_unid->to_string() == string_unid);
  REQUIRE(parsed_unid == original_unid);

  auto reopened_by_id = db->get_note(original_noteid);
  REQUIRE(reopened_by_id.has_value());
  REQUIRE(reopened_by_id->info<dmn::info::note_id>() == original_noteid);
  REQUIRE(reopened_by_id->info<dmn::info::unid>() == original_unid);
  const auto reopened_subject = reopened_by_id->get<std::string>("Subject");
  REQUIRE(reopened_subject.has_value());
  REQUIRE(reopened_subject.value() == replacement_subject);

  auto reopened_by_unid = db->get_note(original_unid);
  REQUIRE(reopened_by_unid.has_value());
  REQUIRE(reopened_by_unid->info<dmn::info::note_id>() == original_noteid);
  const auto reopened_numeric = reopened_by_unid->get<double>("NumericValue");
  REQUIRE(reopened_numeric.has_value());
  REQUIRE(reopened_numeric.value() == 42.5);

  auto copied = primary.copy_to_database(*db);
  REQUIRE(copied.has_value());
  auto copy = std::move(*copied);
  REQUIRE_FALSE(copy.info<dmn::info::note_id>() == primary.info<dmn::info::note_id>());
  const auto copied_subject = copy.get<std::string>("Subject");
  const auto copied_tags = copy.get<dmn::list>("Tags");
  REQUIRE(copied_subject.has_value());
  REQUIRE(copied_subject.value() == replacement_subject);
  REQUIRE(copied_tags.has_value());
  REQUIRE(copied_tags.value().size() == 3);

  REQUIRE_NOTHROW(primary.remove(true));
  REQUIRE_FALSE(db->get_note(original_noteid).has_value());
  std::error_code ignore_ec;
  fs::remove(attachment_path, ignore_ec);
}