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

#include <string>

namespace shk {

struct CmdlineOptions {
  enum class Result {
    SUCCESS,
    VERSION,
    HELP
  };

  std::string tracefile;
  std::string command;
  Result result = Result::SUCCESS;
  bool suicide_when_orphaned = false;
  bool server = false;
  std::string replay;
  std::string capture;
  bool json = false;

  static CmdlineOptions parse(int argc, char *argv[]);
};

}