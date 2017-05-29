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

#include <vector>

#include <rx/sum.h>
#include <rx/iterate.h>

#include "test_util.h"

namespace shk {

TEST_CASE("Sum") {
  auto sum = Sum();

  SECTION("empty") {
    CHECK(GetOne<int>(sum(Iterate(std::vector<int>{}))) == 0);
  }

  SECTION("one value") {
    CHECK(GetOne<int>(sum(Iterate(std::vector<int>{ 10 }))) == 10);
  }

  SECTION("two values") {
    CHECK(GetOne<int>(sum(Iterate(std::vector<int>{ 10, 2 }))) == 12);
  }
}

}  // namespace shk
