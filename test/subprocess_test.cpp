// Copyright 2012 Google Inc. All Rights Reserved.
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

#include "subprocess.h"

#include <string>

#ifndef _WIN32
// SetWithLots need setrlimit.
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace shk {

#ifdef _WIN32
const char* kSimpleCommand = "cmd /c dir \\";
#else
const char* kSimpleCommand = "ls /";
#endif

struct CommandResult {
  ExitStatus exit_status = ExitStatus::SUCCESS;
  std::string output;
};

CommandResult runCommand(
    const std::string &command,
    UseConsole use_console = UseConsole::NO) {
  CommandResult result;
  SubprocessSet subprocs;

  bool did_finish = false;
  subprocs.invoke(
      command,
      use_console,
      [&](ExitStatus status, std::string &&output) {
        result.exit_status = status;
        result.output = std::move(output);
        did_finish = true;
      });

  while (!subprocs.empty()) {
    // Pretend we discovered that stderr was ready for writing.
    subprocs.runCommands();
  }

  CHECK(did_finish);

  return result;
}

void verifyInterrupted(const std::string &command) {
  SubprocessSet subprocs;
  subprocs.invoke(
      command,
      UseConsole::NO,
      [](ExitStatus status, std::string &&output) {
      });

  while (!subprocs.empty()) {
    const bool interrupted = subprocs.runCommands();
    if (interrupted) {
      return;
    }
  }

  CHECK(!"We should have been interrupted");
}

TEST_CASE("Subprocess") {
  // Run a command that fails and emits to stderr.
  SECTION("BadCommandStderr") {
    const auto result = runCommand("cmd /c ninja_no_such_command");
    CHECK(result.exit_status == ExitStatus::FAILURE);
    CHECK(result.output != "");
  }

  // Run a command that does not exist
  SECTION("NoSuchCommand") {
    const auto result = runCommand("ninja_no_such_command");
    CHECK(result.exit_status == ExitStatus::FAILURE);
    CHECK(result.output != "");
#ifdef _WIN32
    CHECK("CreateProcess failed: The system cannot find the file "
          "specified.\n" == result.output);
#endif
  }

#ifndef _WIN32

  SECTION("InterruptChild") {
    const auto result = runCommand("kill -INT $$");
    CHECK(result.exit_status == ExitStatus::INTERRUPTED);
  }

  SECTION("InterruptParent") {
    verifyInterrupted("kill -INT $PPID ; sleep 1");
  }

  SECTION("InterruptChildWithSigTerm") {
    const auto result = runCommand("kill -TERM $$");
    CHECK(result.exit_status == ExitStatus::INTERRUPTED);
  }

  SECTION("InterruptParentWithSigTerm") {
    verifyInterrupted("kill -TERM $PPID ; sleep 1");
  }

  // A shell command to check if the current process is connected to a terminal.
  // This is different from having stdin/stdout/stderr be a terminal. (For
  // instance consider the command "yes < /dev/null > /dev/null 2>&1".
  // As "ps" will confirm, "yes" could still be connected to a terminal, despite
  // not having any of the standard file descriptors be a terminal.
  const std::string kIsConnectedToTerminal = "tty < /dev/tty > /dev/null";

  SECTION("Console") {
    // Skip test if we don't have the console ourselves.
    if (isatty(0) && isatty(1) && isatty(2)) {
      // Test that stdin, stdout and stderr are a terminal.
      // Also check that the current process is connected to a terminal.
      const auto result = runCommand(
          "test -t 0 -a -t 1 -a -t 2 && " + kIsConnectedToTerminal,
          UseConsole::YES);
      CHECK(result.exit_status == ExitStatus::SUCCESS);
    }
  }

  SECTION("NoConsole") {
    const auto result = runCommand(kIsConnectedToTerminal);
    CHECK(result.exit_status != ExitStatus::SUCCESS);
  }

#endif

  SECTION("SetWithSingle") {
    const auto result = runCommand(kSimpleCommand);
    CHECK(result.exit_status == ExitStatus::SUCCESS);
    CHECK(result.output != "");
  }

  SECTION("SetWithMulti") {
    SubprocessSet subprocs;

    const char* kCommands[3] = {
      kSimpleCommand,
#ifdef _WIN32
      "cmd /c echo hi",
      "cmd /c time /t",
#else
      "whoami",
      "pwd",
#endif
    };

    int finished_processes = 0;
    bool processes_done[3];
    for (int i = 0; i < 3; ++i) {
      processes_done[i] = 0;
    }

    for (int i = 0; i < 3; ++i) {
      subprocs.invoke(
          kCommands[i],
          UseConsole::NO,
          [i, &processes_done, &finished_processes](
              ExitStatus status, std::string &&output) {
            CHECK(status == ExitStatus::SUCCESS);
            CHECK("" != output);
            processes_done[i] = true;
            finished_processes++;
          });
    }

    CHECK(3u == subprocs.size());
    for (int i = 0; i < 3; ++i) {
      CHECK(!processes_done[i]);
    }

    while (!processes_done[0] || !processes_done[1] || !processes_done[2]) {
      CHECK(subprocs.size() > 0u);
      subprocs.runCommands();
    }

    CHECK(0u == subprocs.size());
    CHECK(3 == finished_processes);
  }

// OS X's process limit is less than 1025 by default
// (|sysctl kern.maxprocperuid| is 709 on 10.7 and 10.8 and less prior to that).
#if !defined(__APPLE__) && !defined(_WIN32)
  SECTION("SetWithLots") {
    SubprocessSet subprocs;

    // Arbitrary big number; needs to be over 1024 to confirm we're no longer
    // hostage to pselect.
    const unsigned kNumProcs = 1025;

    // Make sure [ulimit -n] isn't going to stop us from working.
    rlimit rlim;
    CHECK(0 == getrlimit(RLIMIT_NOFILE, &rlim));
    if (rlim.rlim_cur < kNumProcs) {
      printf("Raise [ulimit -n] well above %u (currently %lu) to make this test go\n", kNumProcs, rlim.rlim_cur);
      return;
    }

    int num_procs_finished = 0;
    for (size_t i = 0; i < kNumProcs; ++i) {
      subprocs.invoke(
          "/bin/echo",
          UseConsole::NO,
          [&](ExitStatus status, std::string &&output) {
            CHECK(ExitStatus::SUCCESS == status);
            CHECK("" != output);
            num_procs_finished++;
          });
    }
    while (!subprocs.empty()) {
      subprocs.runCommands();
    }
    CHECK(num_procs_finished == kNumProcs);
  }
#endif  // !__APPLE__ && !_WIN32

  // TODO: this test could work on Windows, just not sure how to simply
  // read stdin.
#ifndef _WIN32
  // Verify that a command that attempts to read stdin correctly thinks
  // that stdin is closed.
  SECTION("ReadStdin") {
    const auto result = runCommand("cat -");
    CHECK(result.exit_status == ExitStatus::SUCCESS);
  }
#endif  // _WIN32
}
}  // namespace shk
