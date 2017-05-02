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

#include "io_error.h"

namespace shk {

IoError::IoError() {}

IoError::IoError(const IoError &other)
    : code(other.code),
      _what(other._what ? new std::string(*other._what) : nullptr) {}

IoError &IoError::operator=(const IoError &other) {
  code = other.code;
  _what.reset(other._what ? new std::string(*other._what) : nullptr);
  return *this;
}

bool IoError::operator==(const IoError &other) const {
  if (code != other.code) {
    return false;
  }
  if (!_what && !other._what) {
    return true;
  }
  if (!_what || !other._what) {
    return false;
  }
  return *_what == *other._what;
}

bool IoError::operator!=(const IoError &other) const {
  return !(*this == other);
}

}  // namespace shk
