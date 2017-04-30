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

#include "recompact.h"

#include "log/persistent_invocation_log.h"
#include "util.h"

namespace shk {

int toolRecompact(int argc, char *argv[], const ToolParams &params) {
  try {
    recompactPersistentInvocationLog(
        params.file_system,
        params.clock,
        params.invocations,
        params.invocation_log_path);
  } catch (const IoError &err) {
    error("failed recompaction: %s", err.what());
    return 1;
  }

  return 0;
}

}  // namespace shk
