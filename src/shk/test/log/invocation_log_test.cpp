// Copyright 2017 Per Grön. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <catch.hpp>

#include "log/invocation_log.h"

#include "../in_memory_file_system.h"
#include "../in_memory_invocation_log.h"

namespace shk {

TEST_CASE("InvocationLog") {
  InMemoryFileSystem fs;
  std::string err;
  CHECK(fs.writeFile("a", "hello!", &err));
  CHECK(err == "");

  time_t now = 234;
  const auto clock = [&]{ return now; };
  InMemoryInvocationLog log(fs, clock);

  SECTION("FingerprintFiles") {
    SECTION("Empty") {
      CHECK(log.fingerprintFiles({}) == std::vector<Fingerprint>{});
    }

    SECTION("SingleFile") {
      CHECK(
          log.fingerprintFiles({ "a" }) ==
          std::vector<Fingerprint>({ takeFingerprint(fs, now, "a").first }));
    }

    SECTION("MultipleFiles") {
      CHECK(
          log.fingerprintFiles({ "a", "a", "missing" }) ==
          std::vector<Fingerprint>({
              takeFingerprint(fs, now, "a").first,
              takeFingerprint(fs, now, "a").first,
              takeFingerprint(fs, now, "missing").first }));
    }
  }
}

}  // namespace shk
