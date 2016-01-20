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

TEST_CASE("Subprocess") {
  SubprocessSet subprocs;

  // Run a command that fails and emits to stderr.
  SECTION("BadCommandStderr") {
    Subprocess *subproc = subprocs.add("cmd /c ninja_no_such_command");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      // Pretend we discovered that stderr was ready for writing.
      subprocs.doWork();
    }

    CHECK(ExitStatus::FAILURE == subproc->finish());
    CHECK("" != subproc->getOutput());
  }

  // Run a command that does not exist
  SECTION("NoSuchCommand") {
    Subprocess *subproc = subprocs.add("ninja_no_such_command");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      // Pretend we discovered that stderr was ready for writing.
      subprocs.doWork();
    }

    CHECK(ExitStatus::FAILURE == subproc->finish());
    CHECK("" != subproc->getOutput());
#ifdef _WIN32
    CHECK("CreateProcess failed: The system cannot find the file "
          "specified.\n" == subproc->getOutput());
#endif
  }

#ifndef _WIN32

  SECTION("InterruptChild") {
    Subprocess *subproc = subprocs.add("kill -INT $$");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      subprocs.doWork();
    }

    CHECK(ExitStatus::INTERRUPTED == subproc->finish());
  }

  SECTION("InterruptParent") {
    Subprocess *subproc = subprocs.add("kill -INT $PPID ; sleep 1");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      const bool interrupted = subprocs.doWork();
      if (interrupted) {
        return;
      }
    }

    CHECK(!"We should have been interrupted");
  }

  SECTION("InterruptChildWithSigTerm") {
    Subprocess *subproc = subprocs.add("kill -TERM $$");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      subprocs.doWork();
    }

    CHECK(ExitStatus::INTERRUPTED == subproc->finish());
  }

  SECTION("InterruptParentWithSigTerm") {
    Subprocess *subproc = subprocs.add("kill -TERM $PPID ; sleep 1");
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      const bool interrupted = subprocs.doWork();
      if (interrupted) {
        return;
      }
    }

    CHECK(!"We should have been interrupted");
  }

  // A shell command to check if the current process is connected to a terminal.
  // This is different from having stdin/stdout/stderr be a terminal. (For
  // instance consider the command "yes < /dev/null > /dev/null 2>&1".
  // As "ps" will confirm, "yes" could still be connected to a terminal, despite
  // not having any of the standard file descriptors be a terminal.
  static const char kIsConnectedToTerminal[] = "tty < /dev/tty > /dev/null";

  SECTION("Console") {
    // Skip test if we don't have the console ourselves.
    if (isatty(0) && isatty(1) && isatty(2)) {
      // Test that stdin, stdout and stderr are a terminal.
      // Also check that the current process is connected to a terminal.
      Subprocess* subproc =
          subprocs.add(std::string("test -t 0 -a -t 1 -a -t 2 && ") +
                            std::string(kIsConnectedToTerminal),
                        /*use_console=*/true);
      REQUIRE(subproc != NULL);

      while (!subproc->done()) {
        subprocs.doWork();
      }

      CHECK(ExitStatus::SUCCESS == subproc->finish());
    }
  }

  SECTION("NoConsole") {
    Subprocess *subproc =
        subprocs.add(kIsConnectedToTerminal, /*use_console=*/false);
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      subprocs.doWork();
    }

    CHECK(ExitStatus::SUCCESS != subproc->finish());
  }

#endif

  SECTION("SetWithSingle") {
    Subprocess *subproc = subprocs.add(kSimpleCommand);
    REQUIRE(subproc != NULL);

    while (!subproc->done()) {
      subprocs.doWork();
    }
    CHECK(ExitStatus::SUCCESS == subproc->finish());
    CHECK("" != subproc->getOutput());

    CHECK(1u == subprocs.finished().size());
  }

  SECTION("SetWithMulti") {
    Subprocess *processes[3];
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

    for (int i = 0; i < 3; ++i) {
      processes[i] = subprocs.add(kCommands[i]);
      CHECK(processes[i] != NULL);
    }

    CHECK(3u == subprocs.running().size());
    for (int i = 0; i < 3; ++i) {
      CHECK(!processes[i]->done());
      CHECK("" == processes[i]->getOutput());
    }

    while (!processes[0]->done() || !processes[1]->done() ||
           !processes[2]->done()) {
      CHECK(subprocs.running().size() > 0u);
      subprocs.doWork();
    }

    CHECK(0u == subprocs.running().size());
    CHECK(3u == subprocs.finished().size());

    for (int i = 0; i < 3; ++i) {
      CHECK(ExitStatus::SUCCESS == processes[i]->finish());
      CHECK("" != processes[i]->getOutput());
      delete processes[i];
    }
  }

// OS X's process limit is less than 1025 by default
// (|sysctl kern.maxprocperuid| is 709 on 10.7 and 10.8 and less prior to that).
#if !defined(__APPLE__) && !defined(_WIN32)
  SECTION("SetWithLots") {
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

    std::vector<Subprocess *> procs;
    for (size_t i = 0; i < kNumProcs; ++i) {
      Subprocess *subproc = subprocs.add("/bin/echo");
      REQUIRE(subproc != NULL);
      procs.push_back(subproc);
    }
    while (!subprocs.running().empty())
      subprocs.doWork();
    for (size_t i = 0; i < procs.size(); ++i) {
      CHECK(ExitStatus::SUCCESS, procs[i]->finish());
      CHECK("" != procs[i]->getOutput());
    }
    CHECK(kNumProcs == subprocs.finished().size());
  }
#endif  // !__APPLE__ && !_WIN32

  // TODO: this test could work on Windows, just not sure how to simply
  // read stdin.
#ifndef _WIN32
  // Verify that a command that attempts to read stdin correctly thinks
  // that stdin is closed.
  SECTION("ReadStdin") {
    Subprocess *subproc = subprocs.add("cat -");
    while (!subproc->done()) {
      subprocs.doWork();
    }
    CHECK(ExitStatus::SUCCESS == subproc->finish());
    CHECK(1u == subprocs.finished().size());
  }
#endif  // _WIN32
}

}  // namespace shk
