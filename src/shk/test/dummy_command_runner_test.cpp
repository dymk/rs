#include <catch.hpp>
#include <rapidcheck/catch.h>

#include "dummy_command_runner.h"
#include "generators.h"
#include "in_memory_file_system.h"

namespace shk {

TEST_CASE("DummyCommandRunner") {
  rc::prop("splitCommand of constructCommand should be an identity transformation", []() {
    const auto in_inputs = *gen::pathStringSet();
    const auto in_outputs = *gen::pathStringSet();

    const auto command = DummyCommandRunner::constructCommand(
        std::vector<std::string>(in_inputs.begin(), in_inputs.end()),
        std::vector<std::string>(in_outputs.begin(), in_outputs.end()));

    std::unordered_set<std::string> out_outputs;
    std::unordered_map<std::string, DependencyType> out_inputs;
    std::tie(out_outputs, out_inputs) = detail::splitCommand(command);

    std::unordered_set<std::string> out_input_set;
    for (const auto &dep : out_inputs) {
      RC_ASSERT(dep.second == DependencyType::ALWAYS);
      out_input_set.insert(dep.first);
    }

    RC_ASSERT(out_input_set == in_inputs);
    RC_ASSERT(out_outputs == out_outputs);
  });

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
        "pool",
        [&](CommandRunner::Result &&result) {
          for (size_t i = 0; i < num_cmds; i++) {
            runner.invoke(
                "/bin/echo",
                "pool",
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

      file_system.open(path, "w");  // Create the file
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

      runner.invoke(command, "pool", CommandRunner::noopCallback);
      while (!runner.empty()) {
        runner.runCommands();
      }

      CHECK(file_system.stat(path).result == 0);
    }

    SECTION("fail with missing input") {
      const std::string path = "abc";
      const auto command = DummyCommandRunner::constructCommand({ path }, {});

      auto exit_status = ExitStatus::SUCCESS;
      runner.invoke(command, "pool", [&](CommandRunner::Result &&result) {
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
      runner.invoke(command, "pool", [&](CommandRunner::Result &&result) {
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
    SECTION("empty command") {
      const auto empty_command = DummyCommandRunner::constructCommand({}, {});
      DummyCommandRunner::checkCommand(file_system, empty_command);
    }

    rc::prop("checkCommand after runCommand", []() {
      InMemoryFileSystem file_system;
      DummyCommandRunner runner(file_system);

      // Place inputs in their own folder to make sure that they don't collide
      // with outputs.
      const auto input_path_gen = rc::gen::exec([] {
        return "_in/" + *gen::pathComponent();
      });
      const auto inputs = *rc::gen::container<std::vector<std::string>>(
          input_path_gen);

      // Create input files
      file_system.mkdir("_in");
      for (const auto &input : inputs) {
        file_system.writeFile(
            input,
            "file:" + input);
      }

      const auto outputs = *rc::gen::nonEmpty(
          gen::pathStringWithSingleComponentVector());

      const auto command = DummyCommandRunner::constructCommand(inputs, outputs);

      // The command is not run yet so should not pass
      RC_ASSERT_THROWS(DummyCommandRunner::checkCommand(file_system, command));

      runner.invoke(command, "pool", CommandRunner::noopCallback);
      while (!runner.empty()) {
        runner.runCommands();
      }

      DummyCommandRunner::checkCommand(file_system, command);
    });
  }
}

}  // namespace shk
