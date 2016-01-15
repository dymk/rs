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

struct Rule;

/**
 * An interface for a scope for variable (e.g. "$foo") lookups.
 */
struct Env {
  virtual ~Env() {}
  virtual std::string lookupVariable(const std::string& var) = 0;
};

/**
 * A tokenized string that contains variable references.
 * Can be evaluated relative to an Env.
 */
struct EvalString {
  std::string Evaluate(Env* env) const;

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
  enum TokenType { RAW, SPECIAL };
  using TokenList = std::vector<std::pair<std::string, TokenType>>;
  TokenList _parsed;
};

/**
 * An invokable build command and associated metadata (description, etc.).
 *
 * Rules are created and manipulated by the manifest parser only. After parsing
 * is complete, Rules are all const and should not be modified. This is
 * important for thread safety (and sanity in general).
 */
struct Rule {
  explicit Rule(const std::string &name) : name_(name) {}

  const std::string& name() const { return name_; }

  using Bindings = std::map<std::string, EvalString>;
  void addBinding(const std::string &key, const EvalString &val);

  static bool isReservedBinding(const std::string &var);

  const EvalString* getBinding(const std::string &key) const;

 private:
  // Allow the parsers to reach into this object and fill out its fields.
  friend struct ManifestParser;

  std::string name_;
  std::map<std::string, EvalString> _bindings;
};

/**
 * An Env which contains a mapping of variables to values
 * as well as a pointer to a parent scope.
 *
 * BindingEnvs are created and manipulated by the manifest parser only. After
 * parsing is complete, BindingEnvs are all const and should not be modified.
 * This is important for thread safety (and sanity in general).
 */
struct BindingEnv : public Env {
  BindingEnv() : _parent(NULL) {}
  explicit BindingEnv(BindingEnv* parent) : _parent(parent) {}

  virtual ~BindingEnv() {}
  virtual std::string lookupVariable(const std::string& var);

  void addRule(const Rule* rule);
  const Rule* lookupRule(const std::string& rule_name) const;
  const Rule* lookupRuleCurrentScope(const std::string& rule_name) const;
  const std::map<std::string, const Rule*> &getRules() const;

  void addBinding(const std::string& key, const std::string& val);

  /**
   * This is tricky.  Edges want lookup scope to go in this order:
   * 1) value set on edge itself (_edge->_env)
   * 2) value set on rule, with expansion in the edge's scope
   * 3) value set on enclosing scope of edge (_edge->_env->_parent)
   * This function takes as parameters the necessary info to do (2).
   */
  std::string lookupWithFallback(
      const std::string &var,
      const EvalString *eval,
      Env *env) const;

private:
  std::map<std::string, std::string> _bindings;
  std::map<std::string, const Rule*> _rules;
  BindingEnv *_parent;
};
