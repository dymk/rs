#include "in_memory_invocation_log.h"

namespace shk {
namespace {

std::vector<std::pair<std::string, Fingerprint>> processPaths(
    FileSystem &fs,
    const Clock &clock,
    std::unordered_map<std::string, DependencyType> &&dependencies) {
  std::vector<std::pair<std::string, Fingerprint>> files;
  for (auto &&dep : dependencies) {
    auto &&path = std::move(dep.first);
    const auto fingerprint = takeFingerprint(fs, clock(), path);
    if (dep.second == DependencyType::ALWAYS || !fingerprint.stat.isDir()) {
      files.emplace_back(std::move(path), fingerprint);
    }
  }
  return files;
}

std::vector<std::pair<std::string, Fingerprint>> processPaths(
    FileSystem &fs,
    const Clock &clock,
    std::unordered_set<std::string> &&paths) {
  std::vector<std::pair<std::string, Fingerprint>> files;
  for (auto &&path : paths) {
    files.emplace_back(std::move(path), takeFingerprint(fs, clock(), path));
  }
  return files;
}

}  // anonymous namespace

InMemoryInvocationLog::InMemoryInvocationLog(
    FileSystem &file_system, const Clock &clock)
    : _fs(file_system), _clock(clock) {}

void InMemoryInvocationLog::createdDirectory(const std::string &path) throw(IoError) {
  _created_directories.insert(path);
}

void InMemoryInvocationLog::removedDirectory(const std::string &path) throw(IoError) {
  _created_directories.erase(path);
}

void InMemoryInvocationLog::ranCommand(
    const Hash &build_step_hash,
    std::unordered_set<std::string> &&output_files,
    std::unordered_map<std::string, DependencyType> &&input_files) throw(IoError) {
  _entries[build_step_hash] = {
      processPaths(_fs, _clock, std::move(output_files)),
      processPaths(_fs, _clock, std::move(input_files)) };
}

void InMemoryInvocationLog::cleanedCommand(
    const Hash &build_step_hash) throw(IoError) {
  _entries.erase(build_step_hash);
}

Invocations InMemoryInvocationLog::invocations(Paths &paths) const {
  Invocations result;

  for (const auto &dir : _created_directories) {
    const auto path = paths.get(dir);
    path.fileId().each([&](const FileId file_id) {
      result.created_directories.emplace(file_id, path);
    });
  }

  for (const auto &log_entry : _entries) {
    Invocations::Entry entry;
    const auto files = [&](
        const std::vector<std::pair<std::string, Fingerprint>> &files) {
      std::vector<std::pair<Path, Fingerprint>> result;
      for (const auto &file : files) {
        result.emplace_back(paths.get(file.first), file.second);
      }
      return result;
    };
    entry.output_files = files(log_entry.second.output_files);
    entry.input_files = files(log_entry.second.input_files);
    result.entries[log_entry.first] = entry;
  }

  return result;
}

}  // namespace shk
