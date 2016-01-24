#pragma once

#include <functional>
#include <string>
#include <vector>

#include "exit_status.h"

namespace shk {

enum class UseConsole {
  NO,
  YES,
};

/**
 * A CommandRunner is responsible for invoking build commands, for detecting
 * which files the command read and wrote to, verifying that the command did
 * not do something disallowed, for example access network or leave a daemon
 * process running.
 *
 * It is not responsible for verifying anything that requires knowledge of the
 * whole build graph to check, for example if the command read a file that is
 * an output of another command without declaring that as a dependency.
 */
class CommandRunner {
 public:
  virtual ~CommandRunner() = default;

  struct Result {
    std::vector<std::string> input_files;
    std::vector<std::string> output_files;
    ExitStatus exit_status = ExitStatus::SUCCESS;
    std::string output;
    std::vector<std::string> linting_errors;
  };

  using Callback = std::function<void (Result &&result)>;

  static void noopCallback(Result &&result) {}

  /**
   * Invoke a command. When the command is finished, callback is invoked with
   * the result. It is allowed to call invoke() and empty() from the callback,
   * but it is not allowed to call wait() from there.
   *
   * It is legal to call invoke even from a callback of invoke (ie within a
   * runCommands invocation).
   *
   * The callback is always invokedfrom within a runCommands call.
   */
  virtual void invoke(
      const std::string &command,
      UseConsole use_console,
      const Callback &callback) = 0;

  /**
   * Returns the number of currently running commands.
   */
  virtual size_t size() const = 0;

  bool empty() const { return size() == 0; }

  /**
   * Wait until a command has completed. If there are no commands running
   * right now (if empty()), then the method returns immediately.
   *
   * @return true if the process was interrupted while running the commands.
   */
  virtual bool runCommands() = 0;
};

}  // namespace shk
