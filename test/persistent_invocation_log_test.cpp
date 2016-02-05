#include <catch.hpp>

#include "path.h"
#include "persistent_invocation_log.h"

#include "in_memory_file_system.h"
#include "in_memory_invocation_log.h"

namespace shk {

inline bool operator==(
    const InvocationLog::Entry &a,
    const InvocationLog::Entry &b) {
  return
      std::tie(a.output_files, a.input_files) ==
      std::tie(b.output_files, b.input_files);
}

namespace {

void checkEmpty(const InvocationLogParseResult &empty) {
  CHECK(empty.invocations.entries.empty());
  CHECK(empty.invocations.created_directories.empty());
  CHECK(empty.warning.empty());
  CHECK(!empty.needs_recompaction);
  CHECK(empty.path_ids.empty());
  CHECK(empty.entry_count == 0);
}

template<typename Callback>
void roundtrip(Callback &&callback) {
  InMemoryFileSystem fs;
  Paths paths(fs);
  InMemoryInvocationLog in_memory_log;
  const auto persistent_log = openPersistentInvocationLog(
      fs,
      "file",
      PathIds(),
      0);
  callback(*persistent_log);
  callback(in_memory_log);
  const auto result = parsePersistentInvocationLog(paths, fs, "file");
  auto &invocations = result.invocations;

  CHECK(result.warning == "");

  std::unordered_set<std::string> created_directories;
  for (const auto &dir : invocations.created_directories) {
    created_directories.insert(dir.original());
  }
  CHECK(in_memory_log.createdDirectories() == created_directories);

  std::unordered_map<Hash, InvocationLog::Entry> entries;
  for (const auto &entry : invocations.entries) {
    InvocationLog::Entry log_entry;
    const auto files = [&](
        const std::vector<std::pair<Path, Fingerprint>> &files) {
      std::vector<std::pair<std::string, Fingerprint>> result;
      for (const auto &file : files) {
        result.emplace_back(file.first.original(), file.second);
      }
      return result;
    };
    log_entry.output_files = files(entry.second.output_files);
    log_entry.input_files = files(entry.second.input_files);
    entries[entry.first] = log_entry;
  }
  CHECK(in_memory_log.entries() == entries);
}

void writeFileWithHeader(
    FileSystem &fs,
    const std::string &file,
    uint32_t version) {
  const auto stream = fs.open(file, "w");
  const std::string kFileSignature = "invocations:";
  stream->write(
        reinterpret_cast<const uint8_t *>(kFileSignature.data()),
        kFileSignature.size(),
        1);
  stream->write(
        reinterpret_cast<const uint8_t *>(&version),
        sizeof(version),
        1);
}

}  // anonymous namespace

TEST_CASE("PersistentInvocationLog") {
  InMemoryFileSystem fs;
  Paths paths(fs);
  fs.writeFile("empty", "");

  Hash hash_0;
  std::fill(hash_0.data.begin(), hash_0.data.end(), 0);
  Hash hash_1;
  std::fill(hash_0.data.begin(), hash_0.data.end(), 1);
  Fingerprint fp_0;
  std::fill(fp_0.hash.data.begin(), fp_0.hash.data.end(), 0);
  fp_0.timestamp = 1;
  Fingerprint fp_1;
  std::fill(fp_1.hash.data.begin(), fp_1.hash.data.end(), 0);
  fp_1.timestamp = 2;

  SECTION("Parsing") {
    CHECK_THROWS_AS(
        parsePersistentInvocationLog(paths, fs, "missing"), IoError);
    CHECK_THROWS_AS(
        parsePersistentInvocationLog(paths, fs, "empty"), ParseError);

    writeFileWithHeader(fs, "invalid_header", 3);
    CHECK_THROWS_AS(
        parsePersistentInvocationLog(paths, fs, "invalid_header"), ParseError);

    writeFileWithHeader(fs, "just_header", 1);
    checkEmpty(parsePersistentInvocationLog(paths, fs, "just_header"));
  }

  SECTION("Roundtrip") {
    SECTION("Empty") {
      roundtrip([](InvocationLog &log) {
      });
    }

    SECTION("CreatedDirectory") {
      roundtrip([](InvocationLog &log) {
        log.createdDirectory("dir");
      });
    }

    SECTION("CreatedThenDeletedDirectory") {
      roundtrip([](InvocationLog &log) {
        log.createdDirectory("dir");
        log.removedDirectory("dir");
      });
    }

    SECTION("InvocationNoFiles") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("InvocationSingleInputFile") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        entry.input_files.emplace_back("hi", fp_0);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("InvocationTwoInputFiles") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        entry.input_files.emplace_back("hi", fp_0);
        entry.input_files.emplace_back("duh", fp_1);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("InvocationSingleOutputFile") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        entry.output_files.emplace_back("hi", fp_0);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("InvocationTwoOutputFiles") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        entry.output_files.emplace_back("aah", fp_0);
        entry.output_files.emplace_back("hi", fp_1);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("InvocationInputAndOutputFiles") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        entry.input_files.emplace_back("aah", fp_0);
        entry.output_files.emplace_back("hi", fp_1);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("OverwrittenInvocation") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        log.ranCommand(hash_0, entry);
        entry.output_files.emplace_back("hi", fp_0);
        log.ranCommand(hash_0, entry);
      });
    }

    SECTION("DeletedMissingInvocation") {
      roundtrip([&](InvocationLog &log) {
        log.cleanedCommand(hash_0);
      });
    }

    SECTION("DeletedInvocation") {
      roundtrip([&](InvocationLog &log) {
        InvocationLog::Entry entry;
        log.ranCommand(hash_0, entry);
        log.cleanedCommand(hash_0);
      });
    }

    SECTION("MixAndMatch") {
      roundtrip([&](InvocationLog &log) {
        log.createdDirectory("dir");
        log.createdDirectory("dir_2");
        log.removedDirectory("dir");

        InvocationLog::Entry entry;
        entry.input_files.emplace_back("aah", fp_0);
        entry.output_files.emplace_back("hi", fp_1);
        log.ranCommand(hash_0, entry);
        log.cleanedCommand(hash_1);
        log.ranCommand(hash_1, InvocationLog::Entry());
        log.cleanedCommand(hash_0);
      });
    }

    SECTION("EntryCount") {
    }

    SECTION("PathIds") {
    }

    SECTION("WarnOnTruncatedInput") {
    }

    SECTION("RequestRecompaction") {
    }
  }

  SECTION("Recompaction") {
    // * Recompacting
  }
}

}  // namespace shk
