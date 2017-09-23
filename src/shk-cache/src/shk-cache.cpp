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

#include <stdio.h>
#include <thread>

#include <shk-cache/shkcache_generated.h>
#include <shk-cache/shkcache.grpc.fb.h>
#include <util/assert.h>

namespace shk {

int main(int /*argc*/, const char * /*argv*/[]) {
  printf("Hello!\n");

  return 0;
}

}  // namespace shk

int main(int argc, const char *argv[]) {
  return shk::main(argc, argv);
}
