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

#include <rx/empty.h>
#include <rx/subscriber.h>

namespace shk {

TEST_CASE("Empty") {
  SECTION("construct") {
    auto empty = Empty();
  }

  SECTION("subscribe") {
    auto empty = Empty();

    bool complete = false;
    {
      auto subscription = empty(MakeSubscriber(
          [](int next) { CHECK(!"should not happen"); },
          [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
          [&complete] { complete = true; }));
      CHECK(complete);

      complete = false;
      subscription.Request(0);
      subscription.Request(1);
      subscription.Request(Subscription::kAll);
      CHECK(!complete);
    }  // Destroy subscription

    CHECK(!complete);
  }
}

}  // namespace shk
