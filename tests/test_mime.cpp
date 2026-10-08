#include <catch2/catch_test_macros.hpp>

#include <dmn/messaging/mime.hpp>
#include <dmn/error.hpp>
#include <dmn/note.hpp>

#include "utils.hpp"

TEST_CASE("invalid headers are rejected", "[nos][mime]") {
  REQUIRE_THROWS_AS(dmn::header({"This key is invalid", ""}), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dmn::header({"Illegal-Key:", ""}), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dmn::header({"UTF-8-🐶", ""}), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dmn::header({"", "This value is invalid\n\n"}), dmn::invalid_argument);
}

TEST_CASE("mime stream supports long lines", "[nos][mime]") {
  auto [db, name] = utils::random_database();
  const auto note = db->create_note();

  const std::string expected(64'000, 'A');

  dmn::omimestream omime{note, "Body"};
  omime << dmn::header{{"Content-Type", "text/plain"}};
  omime << expected;
  omime.finalize(note);

  std::string actual;
  dmn::imimestream imime{note, "Body"};
  REQUIRE_NOTHROW(imime.getline(actual));
  REQUIRE(actual == expected);

  REQUIRE(imime.eof());
  REQUIRE(static_cast<bool>(imime));

  REQUIRE_NOTHROW(imime.getline(actual));
  REQUIRE_FALSE(static_cast<bool>(imime));
  REQUIRE(actual.empty());
}

TEST_CASE("mime object can be written and read", "[nos][mime]") {
  auto [db, name] = utils::random_database();

  const auto note = db->create_note();

  const dmn::header content_type{{"Content-Type", "text/html"}};
  const std::string line_one = "This is line one";
  const std::string line_two = "Here is a line with UTF-8: 🐶";

  dmn::omimestream omime{note, "Body"};
  REQUIRE_NOTHROW(omime << content_type);
  REQUIRE_NOTHROW(omime << line_one << "\r\n");
  REQUIRE_NOTHROW(omime << line_two << "\n");
  REQUIRE_THROWS_AS(omime << content_type, dmn::runtime_error);
  REQUIRE_NOTHROW(omime << "Foo" << " + " << "Bar");
  REQUIRE_NOTHROW(omime.finalize(note));

  dmn::imimestream imime{note, "Body"};
  std::string line;
  std::string word;

  REQUIRE_NOTHROW(imime.getline(line));
  REQUIRE(line == line_one);
  REQUIRE_NOTHROW(imime.getline(line));
  REQUIRE(line == line_two);
  REQUIRE_FALSE(imime.eof());
  REQUIRE_NOTHROW(imime >> word);
  REQUIRE(word == "Foo");
  REQUIRE_NOTHROW(imime >> word);
  REQUIRE(word == "+");
  REQUIRE_NOTHROW(imime >> word);
  REQUIRE(word == "Bar");

  REQUIRE_NOTHROW(imime.getline(line));
  REQUIRE_NOTHROW(imime >> word);
  REQUIRE_FALSE(static_cast<bool>(imime));
  REQUIRE(imime.eof());
  REQUIRE(line.empty());
  REQUIRE(word.empty());
}

TEST_CASE("mime stream preserves whitespace", "[nos][mime]") {
  auto [db, name] = utils::random_database();
  const auto note = db->create_note();

  dmn::omimestream omime{note, "Body"};
  omime << dmn::header{{"Content-Type", "text/plain"}};
  omime << "Hello world\r\nNext line";
  omime.finalize(note);

  std::string word;
  std::string line;

  dmn::imimestream imime{note, "Body"};
  REQUIRE_NOTHROW(imime >> word);
  REQUIRE(word == "Hello");
  REQUIRE_NOTHROW(imime.getline(line));
  REQUIRE(line == " world");

  std::string buffer(4, '\0');
  REQUIRE(imime.read(buffer) == 4);
  REQUIRE(buffer == "Next");

  REQUIRE_NOTHROW(imime.getline(line));
  REQUIRE(line == " line");

  REQUIRE(imime.read(std::span<char>{}) == 0);
  REQUIRE(static_cast<bool>(imime));
}
