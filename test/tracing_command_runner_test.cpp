#include <catch.hpp>

#include "persistent_file_system.h"
#include "real_command_runner.h"
#include "tracing_command_runner.h"

namespace shk {
namespace {

CommandRunner::Result runCommand(
    CommandRunner &runner,
    const std::string &command) {
  CommandRunner::Result result;

  bool did_finish = false;
  runner.invoke(
      command,
      UseConsole::NO,
      [&](CommandRunner::Result &&result_) {
        result = std::move(result_);
        did_finish = true;
      });

  while (!runner.empty()) {
    // Pretend we discovered that stderr was ready for writing.
    runner.runCommands();
  }

  CHECK(did_finish);

  return result;
}

class FailingMkstempFileSystem : public FileSystem {
 public:
  FailingMkstempFileSystem(Paths &paths)
      : _fs(persistentFileSystem(paths)) {}

  Paths &paths() override {
    return _fs->paths();
  }

  std::unique_ptr<Stream> open(
      const Path &path, const char *mode) throw(IoError) override {
    return _fs->open(path, mode);
  }
  Stat stat(const Path &path) override {
    return _fs->stat(path);
  }
  Stat lstat(const Path &path) override {
    return _fs->lstat(path);
  }
  void mkdir(const Path &path) throw(IoError) override {
    return _fs->mkdir(path);
  }
  void rmdir(const std::string &path) throw(IoError) override {
    return _fs->rmdir(path);
  }
  void unlink(const std::string &path) throw(IoError) override {
    return _fs->unlink(path);
  }
  std::string readFile(const Path &path) throw(IoError) override {
    return _fs->readFile(path);
  }
  std::string mkstemp(std::string &&filename_template) throw(IoError) override {
    throw IoError("Test-induced mkstemp error", 0);
  }

 private:
  std::unique_ptr<FileSystem> _fs;
};

class FailingUnlinkFileSystem : public FileSystem {
 public:
  FailingUnlinkFileSystem(Paths &paths)
      : _fs(persistentFileSystem(paths)) {}

  Paths &paths() override {
    return _fs->paths();
  }

  std::unique_ptr<Stream> open(
      const Path &path, const char *mode) throw(IoError) override {
    return _fs->open(path, mode);
  }
  Stat stat(const Path &path) override {
    return _fs->stat(path);
  }
  Stat lstat(const Path &path) override {
    return _fs->lstat(path);
  }
  void mkdir(const Path &path) throw(IoError) override {
    return _fs->mkdir(path);
  }
  void rmdir(const std::string &path) throw(IoError) override {
    return _fs->rmdir(path);
  }
  void unlink(const std::string &path) throw(IoError) override {
    // Unlink it anyway, because we don't want to leave files around on the
    // file system after the test has finished running.
    _fs->unlink(path);
    throw IoError("Test-induced unlink error", 0);
  }
  std::string readFile(const Path &path) throw(IoError) override {
    return _fs->readFile(path);
  }
  std::string mkstemp(std::string &&filename_template) throw(IoError) override {
    return _fs->mkstemp(std::move(filename_template));
  }

 private:
  std::unique_ptr<FileSystem> _fs;
};

template<typename Container, typename Value>
bool contains(const Container &container, const Value &value) {
  return std::find(
      container.begin(), container.end(), value) != container.end();
}

std::string getWorkingDir() {
  char *wd = getcwd(NULL, 0);
  std::string result = wd;
  free(wd);
  return result;
}

}  // anonymous namespace

TEST_CASE("TracingCommandRunner") {
  Paths paths;
  const auto fs = persistentFileSystem(paths);
  const auto runner = makeTracingCommandRunner(
      *fs,
      makeRealCommandRunner());
  const auto output_path = paths.get(getWorkingDir() + "/shk.test-file");

  SECTION("TrackInputs") {
    const auto result = runCommand(*runner, "/bin/ls /sbin");
    CHECK(contains(result.input_files, paths.get("/sbin")));
    CHECK(contains(result.input_files, paths.get("/bin/ls")));
    CHECK(result.output_files.empty());
  }

  SECTION("TrackOutputs") {
    const auto result = runCommand(
        *runner, "/usr/bin/touch " + output_path.canonicalized());
    CHECK(result.output_files.size() == 1);
    CHECK(contains(result.output_files, output_path));
    fs->unlink(output_path.canonicalized());
  }

  SECTION("TrackRemovedOutputs") {
    const auto result = runCommand(
        *runner,
        "/usr/bin/touch '" + output_path.canonicalized() + "'; /bin/rm '" +
        output_path.canonicalized() + "'");
    CHECK(result.output_files.empty());
  }

  SECTION("TrackMovedOutputs") {
    const auto other_path = paths.get(output_path.canonicalized() + ".b");
    const auto result = runCommand(
        *runner,
        "/usr/bin/touch " + output_path.canonicalized() + " && /bin/mv " +
        output_path.canonicalized() + " " + other_path.canonicalized());
    CHECK(result.output_files.size() == 1);
    // Should have only other_path as an output path; the file at output_path
    // was moved.
    CHECK(contains(result.output_files, other_path));
    fs->unlink(other_path.canonicalized());
  }

  SECTION("HandleTmpFileCreationError") {
    FailingMkstempFileSystem failing_mkstemp(paths);
    const auto runner = makeTracingCommandRunner(
        failing_mkstemp,
        makeRealCommandRunner());

    // Failing to create tmpfile should not make invoke throw
    const auto result = runCommand(*runner, "/bin/echo");

    // But it should make the command fail
    CHECK(result.exit_status == ExitStatus::FAILURE);
  }

  SECTION("HandleTmpFileRemovalError") {
    FailingUnlinkFileSystem failing_unlink(paths);
    const auto runner = makeTracingCommandRunner(
        failing_unlink,
        makeRealCommandRunner());

    const auto result = runCommand(*runner, "/bin/ls /sbin");
    CHECK(contains(result.input_files, paths.get("/bin/ls")));

    // Failing to remove the tempfile should be ignored
  }

  SECTION("abort") {
    CommandRunner::Result result;

    runner->invoke(
        "/bin/echo",
        UseConsole::NO,
        CommandRunner::noopCallback);
  }

  SECTION("size") {
    CommandRunner::Result result;

    runner->invoke(
        "/bin/echo",
        UseConsole::NO,
        CommandRunner::noopCallback);

    CHECK(runner->size() == 1);

    runner->invoke(
        "/bin/echo",
        UseConsole::NO,
        CommandRunner::noopCallback);

    CHECK(runner->size() == 2);

    while (!runner->empty()) {
      // Pretend we discovered that stderr was ready for writing.
      runner->runCommands();
    }
  }
}

}  // namespace shk
