#include <catch.hpp>

#include "cmd/trace_server_handle.h"

namespace shk {

TEST_CASE("TraceServerHandle") {
  SECTION("InvalidCommand") {
    std::string err;
    CHECK(!TraceServerHandle::open("/nonexisting", &err));
    CHECK(err == "posix_spawn() failed");
  }

  SECTION("WrongAcknowledgementMessage") {
    std::string err;
    CHECK(!TraceServerHandle::open("/bin/echo", &err));
    CHECK(err == "did not see expected acknowledgement message");
  }

  SECTION("Success") {
    std::string err;
    CHECK(TraceServerHandle::open("shk-trace-dummy", &err));
  }
}

}  // namespace shk
