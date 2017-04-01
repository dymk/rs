#include <catch.hpp>

#include "event_consolidator.h"

namespace shk {
namespace {

bool containsFatalError(const std::vector<EventConsolidator::Event> &events) {
  return std::find_if(events.begin(), events.end(), [](
      const EventConsolidator::Event &event) {
    return event.first == EventType::FatalError;
  }) != events.end();
}

}  // anonymous namespace

TEST_CASE("EventConsolidator") {
  using ET = EventType;

  static constexpr ET all_events[] = {
      ET::Read,
      ET::ReadDirectory,
      ET::Write,
      ET::Create,
      ET::Delete,
      ET::FatalError };

  EventConsolidator ec;

  SECTION("Copyable") {
    ec.event(ET::FatalError, "");
    auto ec2 = ec;

    CHECK(containsFatalError(ec.getConsolidatedEventsAndReset()));
    CHECK(containsFatalError(ec2.getConsolidatedEventsAndReset()));
  }

  SECTION("Assignable") {
    EventConsolidator ec2;
    ec2.event(ET::FatalError, "");
    ec = ec2;

    CHECK(containsFatalError(ec.getConsolidatedEventsAndReset()));
    CHECK(containsFatalError(ec2.getConsolidatedEventsAndReset()));
  }

  SECTION("Reset") {
    for (const auto event : all_events) {
      ec.event(event, "");
      CHECK(!ec.getConsolidatedEventsAndReset().empty());
      CHECK(ec.getConsolidatedEventsAndReset().empty());
    }
  }

  SECTION("MergeDuplicateEvents") {
    for (const auto event : all_events) {
      ec.event(event, "a");
      ec.event(event, "a");
      CHECK(ec.getConsolidatedEventsAndReset().size() == 1);
    }
  }

  SECTION("KeepPathAndEventType") {
    for (const auto event : all_events) {
      ec.event(event, "path");

      auto res = ec.getConsolidatedEventsAndReset();
      REQUIRE(res.size() == 1);
      CHECK(res.front().first == event);
      CHECK(res.front().second == "path");
    }
  }

  SECTION("IgnoreAfterCreate") {
    static constexpr ET events[] = {
        ET::Read,
        ET::ReadDirectory,
        ET::Write };

    for (const auto event : events) {
      ec.event(ET::Create, "path");
      ec.event(event, "path");

      auto res = ec.getConsolidatedEventsAndReset();
      REQUIRE(res.size() == 1);
      CHECK(res.front().first == ET::Create);
      CHECK(res.front().second == "path");
    }
  }

  SECTION("CreateOverridesDelete") {
    ec.event(ET::Delete, "path");
    ec.event(ET::Create, "path");

    auto res = ec.getConsolidatedEventsAndReset();
    REQUIRE(res.size() == 1);
    CHECK(res.front().first == ET::Create);
    CHECK(res.front().second == "path");
  }

  SECTION("CreateAndDeleteOverrideWrite") {
    static constexpr ET events[] = { ET::Create, ET::Delete };

    for (auto event : events) {
      ec.event(ET::Write, "path");
      ec.event(event, "path");

      auto res = ec.getConsolidatedEventsAndReset();
      REQUIRE(res.size() == 1);
      CHECK(res.front().first == event);
      CHECK(res.front().second == "path");
    }
  }

  SECTION("DeleteOverridesWriteAndReadDirectory") {
    static constexpr ET events[] = { ET::Write, ET::ReadDirectory };

    for (auto event : events) {
      ec.event(event, "path");
      ec.event(ET::Delete, "path");

      auto res = ec.getConsolidatedEventsAndReset();
      REQUIRE(res.size() == 1);
      CHECK(res.front().first == ET::Delete);
      CHECK(res.front().second == "path");
    }
  }

  SECTION("CreateWriteDeleteDoNotOverrideRead") {
    static constexpr ET events[] = { ET::Create, ET::Write, ET::Delete };

    for (auto event : events) {
      ec.event(ET::Read, "path");
      ec.event(event, "path");

      auto res = ec.getConsolidatedEventsAndReset();
      REQUIRE(res.size() == 2);
      if (res[0].first != ET::Read) {
        // Crude sort
        swap(res[0], res[1]);
      }

      CHECK(res[0].first == ET::Read);
      CHECK(res[0].second == "path");
      CHECK(res[1].first == event);
      CHECK(res[1].second == "path");
    }
  }

  SECTION("DeleteErasesCreate") {
    ec.event(ET::Create, "path");
    ec.event(ET::Delete, "path");

    CHECK(ec.getConsolidatedEventsAndReset().empty());
  }
}

}  // namespace shk
