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

#include "cmd/pooled_command_runner.h"

#include <deque>

namespace shk {
namespace {

class PooledCommandRunner : public CommandRunner {
 public:
  PooledCommandRunner(
      const std::unordered_map<std::string, int> &pools,
      std::unique_ptr<CommandRunner> &&inner)
      : _pools(pools),
        _inner(std::move(inner)) {}

  virtual void invoke(
      nt_string_view command,
      Step step,
      const Callback &callback) override {
    std::string pool(step.poolName());
    if (canRunNow(pool)) {
      invokeNow(command, step, pool, callback);
    } else {
      delay(command, step, pool, callback);
    }
  }

  virtual size_t size() const override {
    return _inner->size() + _delayed_commands_count;
  }

  virtual bool canRunMore() const override {
    return _inner->canRunMore();
  }

  virtual bool runCommands() override {
    return _inner->runCommands();
  }

 private:
  struct Command {
    std::string command;
    Step step;
    Callback callback;
  };

  void delay(
      nt_string_view command,
      Step step,
      const std::string &pool,
      const Callback &callback) {
    _delayed_commands_count++;
    _delayed_commands[pool].push_front(
        Command{ std::string(command), step, callback });
  }

  void invokeDelayedJob(const std::string &pool) {
    auto &commands = _delayed_commands[pool];
    if (!commands.empty()) {
      _delayed_commands_count--;
      const auto &command = commands.back();
      invokeNow(command.command, command.step, pool, command.callback);
      commands.pop_back();
    }
  }

  void invokeNow(
      nt_string_view command,
      Step step,
      const std::string &pool,
      const Callback &callback) {
    if (!pool.empty()) {
      _pools[pool]--;
    }
    _inner->invoke(command, step, [this, pool, callback](Result &&result) {
      if (_pools[pool]++ == 0) {
        // Pool was empty. Try to schedule a delayed job
        invokeDelayedJob(pool);
      }
      callback(std::move(result));
    });
  }

  bool canRunNow(const std::string &pool) const {
    if (pool.empty()) return true;
    auto it = _pools.find(pool);
    if (it == _pools.end()) {
      // Undeclared pools have depth 0. This might or might not be desired
      // behaviour but for now it is like this, and this can't just return
      // true to change that, because then _pools could get negative values.
      return false;
    } else {
      return it->second != 0;
    }
  }

  // Map from pool name to the number of spots left in the pool
  std::unordered_map<std::string, int> _pools;

  std::unique_ptr<CommandRunner> _inner;

  // Map from pool name to a list of commands that have been delayed because the
  // pool was full.
  std::unordered_map<std::string, std::deque<Command>> _delayed_commands;

  size_t _delayed_commands_count = 0;
};

}  // anonymous namespace

std::unique_ptr<CommandRunner> makePooledCommandRunner(
    const std::unordered_map<std::string, int> &pools,
    std::unique_ptr<CommandRunner> &&inner) {
  return std::unique_ptr<CommandRunner>(
      new PooledCommandRunner(pools, std::move(inner)));
}

}  // namespace shk
