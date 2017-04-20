#include "cmd/command_runner.h"

#include <list>
#include <string>
#include <vector>

#include "fs/file_system.h"

namespace shk {
namespace detail {

/**
 * Split a command back into its input and output paths.
 *
 * This is exposed for testing purposes.
 */
std::pair<
    std::vector<std::string>,
    std::vector<std::string>> splitCommand(
        const std::string &command);

/**
 * "Run" a given command. This reads the command's input files and writes to its
 * output files in a way that can be checked by
 * DummyCommandRunner::checkCommand.
 *
 * This is exposed for testing purposes.
 */
CommandRunner::Result runCommand(
    FileSystem &file_system,
    const std::string &command);

}  // namespace detail

class DummyCommandRunner : public CommandRunner {
 public:
  DummyCommandRunner(FileSystem &file_system);

  void invoke(
      nt_string_view command,
      Step step,
      const Callback &callback) override;

  size_t size() const override;

  bool canRunMore() const override;

  bool runCommands() override;

  int getCommandsRun() const;

  static std::string constructCommand(
      const std::vector<std::string> &inputs,
      const std::vector<std::string> &outputs);

  /**
   * Verify that a command has run by looking at the file system and see that
   * the output files of the given are there and have the right contents.
   *
   * Throws an exception when the check fails.
   */
  static void checkCommand(
      FileSystem &file_system, const std::string &command)
          throw(IoError, std::runtime_error);

 private:
  int _commands_run = 0;
  FileSystem &_file_system;
  std::list<std::pair<std::string, Callback>> _enqueued_commands;
};

}  // namespace shk
