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

#pragma once

#include <map>
#include <string>
#include <vector>

#include "string_piece.h"

namespace shk {

class Env;

/**
 * A tokenized string that contains variable references.
 * Can be evaluated relative to an Env.
 *
 * Created by the lexer and used in the manifest parser.
 */
class EvalString {
 public:
  std::string evaluate(Env &env) const;

  void clear() { _parsed.clear(); }
  bool empty() const { return _parsed.empty(); }

  void addText(StringPiece text);
  void addSpecial(StringPiece text);

  /**
   * Construct a human-readable representation of the parsed state,
   * for use in tests.
   */
  std::string serialize() const;

 private:
  enum class TokenType { RAW, SPECIAL };
  using TokenList = std::vector<std::pair<std::string, TokenType>>;
  TokenList _parsed;
};

}  // namespace shk
