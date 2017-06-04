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

#include <rs/flat_map.h>
#include <rs/iterate.h>

namespace shk {

template <typename Container>
auto Concat(Container &&container) {
  auto flat_map = FlatMap([](auto &&publisher) { return publisher; });
  return flat_map(Iterate(std::forward<Container>(container)));
}

}  // namespace shk
