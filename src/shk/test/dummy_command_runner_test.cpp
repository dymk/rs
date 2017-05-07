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

#include <catch.hpp>

#include "dummy_command_runner.h"
#include "in_memory_file_system.h"
#include "manifest/step_builder.h"

namespace shk {
namespace {

void checkSplitConstructCommandIdentity(
    const std::vector<std::string> &in_inputs,
    const std::vector<std::string> &in_outputs) {
  const auto command = DummyCommandRunner::constructCommand(
      std::vector<std::string>(in_inputs.begin(), in_inputs.end()),
      std::vector<std::string>(in_outputs.begin(), in_outputs.end()));

  std::vector<std::string> out_outputs;
  std::vector<std::string> out_inputs;
  std::tie(out_outputs, out_inputs) = detail::splitCommand(command);

  CHECK(in_inputs == out_inputs);
  CHECK(in_outputs == out_outputs);
}

void checkRunCommand(
    const std::vector<std::string> &inputs,
    const std::vector<std::string> &outputs) {
  flatbuffers::FlatBufferBuilder builder;
  auto step = StepBuilder().build(builder);

  InMemoryFileSystem file_system;
  DummyCommandRunner runner(file_system);

  // Create input files
  std::string err;
  for (const auto &input : inputs) {
    CHECK(file_system.writeFile(
        input,
        "file:" + input) == IoError::success());
  }

  const auto command = DummyCommandRunner::constructCommand(
      inputs, outputs);

  if (outputs.empty()) {
    DummyCommandRunner::checkCommand(file_system, command);
  } else {
    // The command is not run yet so should not pass
    CHECK_THROWS(DummyCommandRunner::checkCommand(file_system, command));
  }

  runner.invoke(command, step, CommandRunner::noopCallback);
  while (!runner.empty()) {
    runner.runCommands();
  }

  DummyCommandRunner::checkCommand(file_system, command);
}

}  // anonymous namespace

TEST_CASE("DummyCommandRunner") {
  flatbuffers::FlatBufferBuilder builder;
  auto step = StepBuilder().build(builder);

  SECTION("splitCommand of constructCommand") {
    checkSplitConstructCommandIdentity({}, {});
    checkSplitConstructCommandIdentity({ "in" }, {});
    checkSplitConstructCommandIdentity({}, { "out" });
    checkSplitConstructCommandIdentity({ "in" }, { "out" });
    checkSplitConstructCommandIdentity({ "in", "1" }, { "out", "2" });
  }

  InMemoryFileSystem file_system;
  DummyCommandRunner runner(file_system);

  SECTION("initially empty") {
    CHECK(runner.empty());
  }

  SECTION("InvokeFromCallback") {
    // Push a lot of commands within the callback to increase the likelihood
    // of a crash in case the command runner uses a vector or something else
    // equally bad.
    const size_t num_cmds = 50;
    size_t done = 0;
    runner.invoke(
        "/bin/echo",
        step,
        [&](CommandRunner::Result &&result) {
          for (size_t i = 0; i < num_cmds; i++) {
            runner.invoke(
                "/bin/echo",
                step,
                [&](CommandRunner::Result &&result) {
                  done++;
                });
          }
        });

    while (!runner.empty()) {
      runner.runCommands();
    }

    CHECK(num_cmds == done);
  }

  SECTION("runCommands when empty") {
    runner.runCommands();
  }

  SECTION("popCommandsRun") {
    CHECK(runner.popCommandsRun() == std::vector<std::string>{});
    runner.runCommands();
    CHECK(runner.popCommandsRun() == std::vector<std::string>{});

    const auto command = DummyCommandRunner::constructCommand({}, { "abc" });
    runner.invoke(command, step, CommandRunner::noopCallback);
    while (!runner.empty()) {
      runner.runCommands();
    }

    CHECK(runner.popCommandsRun() == std::vector<std::string>{ command });
    CHECK(runner.popCommandsRun() == std::vector<std::string>{});
  }

  SECTION("runCommand") {
    SECTION("empty command should do nothing") {
      const auto empty_file_system = file_system;
      const auto empty_command = DummyCommandRunner::constructCommand({}, {});
      const auto result = detail::runCommand(file_system, empty_command);

      CHECK(result.exit_status == ExitStatus::SUCCESS);
      CHECK(empty_file_system == file_system);
    }

    SECTION("command should read input files") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({ path }, {});

      // Should fail because it should try to read a missing file
      const auto result = detail::runCommand(file_system, command);
      CHECK(result.exit_status != ExitStatus::SUCCESS);

      // Create the file
      CHECK(file_system.open(path, "w").second == IoError::success());
      // Should now not fail anymore
      const auto second_result = detail::runCommand(file_system, command);
      CHECK(second_result.exit_status == ExitStatus::SUCCESS);
    }

    SECTION("command should write output files") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({}, { path });

      const auto result = detail::runCommand(file_system, command);
      CHECK(result.exit_status == ExitStatus::SUCCESS);

      CHECK(file_system.stat(path).result == 0);  // Output file should have been created
    }
  }

  SECTION("invoke") {
    // These are small sanity checks for this function. It is more thoroughly
    // tested by the checkCommand property based test.

    SECTION("create output file") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({}, { path });

      runner.invoke(command, step, CommandRunner::noopCallback);
      while (!runner.empty()) {
        runner.runCommands();
      }

      CHECK(file_system.stat(path).result == 0);
    }

    SECTION("fail with missing input") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({ path }, {});

      auto exit_status = ExitStatus::SUCCESS;
      runner.invoke(command, step, [&](CommandRunner::Result &&result) {
        exit_status = result.exit_status;
      });
      while (!runner.empty()) {
        runner.runCommands();
      }

      CHECK(exit_status != ExitStatus::SUCCESS);
    }

    SECTION("do not count finished but not yet reaped commands in size()") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({ path }, {});

      bool invoked = false;
      runner.invoke(command, step, [&](CommandRunner::Result &&result) {
        CHECK(runner.empty());
        invoked = true;
      });
      while (!runner.empty()) {
        runner.runCommands();
      }

      CHECK(invoked);
    }
  }

  SECTION("canRunMore") {
    CHECK(runner.canRunMore());
  }

  SECTION("checkCommand") {
    checkRunCommand({}, {});
    checkRunCommand({ "in" }, {});
    checkRunCommand({}, { "out" });
    checkRunCommand({ "in" }, { "out" });
    checkRunCommand({ "in", "1" }, { "out", "2" });
  }
}

}  // namespace shk
