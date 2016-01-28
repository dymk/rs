// Copyright 2011 Google Inc. All Rights Reserved.
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

namespace shk {

int toolRecompact(int argc, char *argv[]) {
  if (!ensureBuildDirExists()) {
    return 1;
  }

  try {
    _deps_log.recompact(path);
  } catch (const IoError &error) {
    error("failed recompaction: %s", error.what());
    return 1;
  }

  return 0;
}

}  // namespace shk
