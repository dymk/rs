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

#include "manifest/step.h"

namespace shk {

/**
 * A BuildStatus object lives for the duration of one build. It is the mechanism
 * in which the build process reports back what is going on. Its primary use
 * is to be able to show progress indication to the user (that is not something
 * the core build process is concerned with).
 *
 * A BuildStatus is typically created when steps to perform has been planned and
 * counted. The BuildStatus object is destroyed when the build is done. The
 * destructor is a good place to do final reporting.
 */
class BuildStatus {
 public:
  virtual ~BuildStatus() = default;

  virtual void stepStarted(const Step &step) = 0;

  virtual void stepFinished(
      const Step &step,
      bool success,
      const std::string &output) = 0;
};

}  // namespace shk
