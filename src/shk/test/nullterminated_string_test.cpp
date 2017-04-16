#include <catch.hpp>

#include "nullterminated_string.h"

namespace shk {

TEST_CASE("NullterminatedString") {
  SECTION("string_view") {
    SECTION("empty nullptr") {
      auto str = NullterminatedString(string_view(nullptr, 0));
      CHECK(str.c_str()[0] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("empty") {
      auto str = NullterminatedString(string_view(""));
      CHECK(str.c_str()[0] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("short") {
      auto str = NullterminatedString(string_view("a"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("just below limit") {
      auto str = BasicNullterminatedString<2>(string_view("a"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("just above limit") {
      auto str = BasicNullterminatedString<2>(string_view("ab"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 'b');
      CHECK(str.c_str()[2] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) !=
          static_cast<const void *>(&str));
    }

    SECTION("well above limit") {
      auto str = BasicNullterminatedString<2>(string_view("hello world!"));
      CHECK(std::string(str.c_str()) == "hello world!");
      CHECK(
          static_cast<const void *>(str.c_str()) !=
          static_cast<const void *>(&str));
    }
  }

  SECTION("nt_string_view null-terminated") {
    SECTION("empty") {
      auto str = NullterminatedString(nt_string_view(""));
      CHECK(str.c_str()[0] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(""));
    }

    SECTION("short") {
      auto str = NullterminatedString(nt_string_view("a"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>("a"));
    }

    SECTION("just below limit") {
      auto str = BasicNullterminatedString<2>(nt_string_view("a"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>("a"));
    }

    SECTION("just above limit") {
      auto str = BasicNullterminatedString<2>(nt_string_view("ab"));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 'b');
      CHECK(str.c_str()[2] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>("ab"));
    }

    SECTION("well above limit") {
      auto str = BasicNullterminatedString<2>(nt_string_view("hello world!"));
      CHECK(std::string(str.c_str()) == "hello world!");
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>("hello world!"));
    }
  }

  SECTION("nt_string_view non-null-terminated") {
    SECTION("empty") {
      auto str = NullterminatedString(nt_string_view("_", 0));
      CHECK(str.c_str()[0] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("short") {
      auto str = NullterminatedString(nt_string_view("a_", 1));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("just below limit") {
      auto str = BasicNullterminatedString<2>(nt_string_view("a_", 1));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) ==
          static_cast<const void *>(&str));
    }

    SECTION("just above limit") {
      auto str = BasicNullterminatedString<2>(nt_string_view("ab_", 2));
      CHECK(str.c_str()[0] == 'a');
      CHECK(str.c_str()[1] == 'b');
      CHECK(str.c_str()[2] == 0);
      CHECK(
          static_cast<const void *>(str.c_str()) !=
          static_cast<const void *>(&str));
    }

    SECTION("well above limit") {
      auto str = BasicNullterminatedString<2>(
          nt_string_view("hello world!_", 12));
      CHECK(std::string(str.c_str()) == "hello world!");
      CHECK(
          static_cast<const void *>(str.c_str()) !=
          static_cast<const void *>(&str));
    }
  }
}

}  // namespace shk
