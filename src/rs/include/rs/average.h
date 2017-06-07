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

#pragma once

#include <rs/map.h>
#include <rs/pipe.h>
#include <rs/reduce.h>

namespace shk {

template <typename ResultType = double>
auto Average() {
  using Accumulator = std::pair<ResultType, size_t>;
  return BuildPipe(
      Reduce(Accumulator(0, 0), [](Accumulator &&accum, auto &&value) {
        accum.first += value;
        accum.second++;
        return accum;
      }),
      Map([](const Accumulator &accum) {
        if (accum.second == 0) {
          throw std::out_of_range(
              "Cannot compute average value of empty stream");
        } else {
          return accum.first / accum.second;
        }
      }));
}

}  // namespace shk
