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

#include <stdexcept>
#include <string>

namespace shk {

class PathError : public std::runtime_error {
 public:
  template <typename string_type>
  explicit PathError(const string_type &what, const std::string &path)
      : runtime_error(what),
        _what(what),
        _path(path) {}

  virtual const char *what() const throw() {
    return _what.c_str();
  }

  const std::string &path() const {
    return _path;
  }

 private:
  const std::string _what;
  const std::string _path;
};

}  // namespace shk
