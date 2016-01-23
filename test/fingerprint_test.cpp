#include <catch.hpp>

#include <sys/stat.h>

#include "fingerprint.h"

#include "in_memory_file_system.h"

namespace shk {

TEST_CASE("Fingerprint") {
  time_t now = 321;
  InMemoryFileSystem fs([&now] { return now; });
  const std::string initial_contents = "initial_contents";
  writeFile(fs, "a", initial_contents);
  fs.mkdir("dir");

  SECTION("Stat") {
    Fingerprint::Stat a;
    a.size = 1;
    a.ino = 2;
    a.mode = 3;
    a.mtime = 4;
    a.ctime = 5;
    Fingerprint::Stat b = a;

    SECTION("equal") {
      CHECK(a == b);
      CHECK(!(a != b));
    }

    SECTION("size") {
      b.size++;
      CHECK(a != b);
      CHECK(!(a == b));
    }

    SECTION("ino") {
      b.ino++;
      CHECK(a != b);
      CHECK(!(a == b));
    }

    SECTION("mode") {
      b.mode++;
      CHECK(a != b);
      CHECK(!(a == b));
    }

    SECTION("mtime") {
      b.mtime++;
      CHECK(a != b);
      CHECK(!(a == b));
    }

    SECTION("ctime") {
      b.ctime++;
      CHECK(a != b);
      CHECK(!(a == b));
    }
  }

  SECTION("takeFingerprint") {
    SECTION("regular file") {
      const auto fp = takeFingerprint(fs, [] { return 12345; }, "a");

      CHECK(fp.stat.size == initial_contents.size());
      CHECK(fp.stat.ino == fs.stat("a").metadata.ino);
      CHECK(S_ISREG(fp.stat.mode));
      CHECK(fp.stat.mtime == now);
      CHECK(fp.stat.ctime == now);
      CHECK(fp.timestamp == 12345);
      CHECK(fp.hash == fs.hashFile("a"));
      CHECK(fp.stat.couldAccess());
    }

    SECTION("missing file") {
      const auto fp = takeFingerprint(fs, [] { return 12345; }, "b");

      CHECK(fp.stat.size == 0);
      CHECK(fp.stat.ino == 0);
      CHECK(fp.stat.mode == 0);
      CHECK(fp.stat.mtime == 0);
      CHECK(fp.stat.ctime == 0);
      CHECK(fp.timestamp == 12345);
      Hash zero;
      std::fill(zero.data.begin(), zero.data.end(), 0);
      CHECK(fp.hash == zero);
      CHECK(!fp.stat.couldAccess());
    }

    SECTION("directory") {
      const auto fp = takeFingerprint(fs, [] { return 12345; }, "dir");

      CHECK(fp.stat.size == 0);
      CHECK(fp.stat.ino == fs.stat("dir").metadata.ino);
      CHECK(S_ISDIR(fp.stat.mode));
      CHECK(fp.stat.mtime == now);
      CHECK(fp.stat.ctime == now);
      CHECK(fp.timestamp == 12345);
      CHECK(fp.hash == fs.hashDir("dir"));
      CHECK(fp.stat.couldAccess());
    }
  }
}

}  // namespace shk
