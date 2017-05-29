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

#include <rs/iterate.h>
#include <rs/subscriber.h>

namespace shk {

TEST_CASE("Iterate") {
  SECTION("construct") {
    auto stream = Iterate(std::vector<int>{});
  }

  SECTION("empty container") {
    auto stream = Iterate(std::vector<int>{});

    int done = 0;
    auto sub = stream(MakeSubscriber(
        [](int next) { CHECK(!"should not happen"); },
        [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
        [&done] { done++; }));
    CHECK(done == 1);
    sub.Request(1);
    CHECK(done == 1);
  }

  SECTION("one value") {
    auto stream = Iterate(std::vector<int>{ 1 });

    int done = 0;
    int next = 0;
    auto sub = stream(MakeSubscriber(
        [&next](int val) {
          CHECK(val == 1);
          next++;
        },
        [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
        [&done] {
          done++;
        }));
    CHECK(done == 0);
    CHECK(next == 0);
    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 1);
    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 1);
  }

  SECTION("multiple values, one at a time") {
    auto stream = Iterate(std::vector<int>{ 1, 2 });

    int done = 0;
    int next = 0;
    auto sub = stream(MakeSubscriber(
        [&next](int val) {
          if (next == 0) {
            CHECK(val == 1);
          } else if (next == 1) {
            CHECK(val == 2);
          } else {
            CHECK(!"got too many values");
          }
          next++;
        },
        [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
        [&done, &next] {
          CHECK(done == 0);
          CHECK(next == 2);
          done++;
        }));
    CHECK(done == 0);
    CHECK(next == 0);

    sub.Request(1);
    CHECK(done == 0);
    CHECK(next == 1);

    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 2);

    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 2);
  }

  SECTION("multiple values, all at once") {
    auto stream = Iterate(std::vector<int>{ 1, 2 });

    int done = 0;
    int next = 0;
    auto sub = stream(MakeSubscriber(
        [&next](int val) {
          if (next == 0) {
            CHECK(val == 1);
          } else if (next == 1) {
            CHECK(val == 2);
          } else {
            CHECK(!"got too many values");
          }
          next++;
        },
        [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
        [&done, &next] {
          CHECK(done == 0);
          CHECK(next == 2);
          done++;
        }));
    CHECK(done == 0);
    CHECK(next == 0);

    sub.Request(2);
    CHECK(done == 1);
    CHECK(next == 2);

    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 2);
  }

  SECTION("multiple values, more than all at once") {
    auto stream = Iterate(std::vector<int>{ 1, 2 });

    int done = 0;
    int next = 0;
    auto sub = stream(MakeSubscriber(
        [&next](int val) {
          if (next == 0) {
            CHECK(val == 1);
          } else if (next == 1) {
            CHECK(val == 2);
          } else {
            CHECK(!"got too many values");
          }
          next++;
        },
        [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
        [&done, &next] {
          CHECK(done == 0);
          CHECK(next == 2);
          done++;
        }));
    CHECK(done == 0);
    CHECK(next == 0);

    sub.Request(Subscription::kAll);
    CHECK(done == 1);
    CHECK(next == 2);

    sub.Request(1);
    CHECK(done == 1);
    CHECK(next == 2);
  }

  SECTION("multiple iterations") {
    auto stream = Iterate(std::vector<int>{ 1 });

    for (int i = 0; i < 3; i++) {
      int done = 0;
      int next = 0;
      auto sub = stream(MakeSubscriber(
          [&next](int val) {
            CHECK(next == 0);
            CHECK(val == 1);
            next++;
          },
          [](std::exception_ptr &&error) { CHECK(!"should not happen"); },
          [&done, &next] {
            CHECK(done == 0);
            CHECK(next == 1);
            done++;
          }));

      CHECK(done == 0);
      sub.Request(1);
      CHECK(done == 1);
      sub.Request(1);
    }
  }
}

}  // namespace shk
