#include <catch.hpp>

#include <fcntl.h>
#include <unistd.h>

#include "fs/cleaning_file_system.h"

#include "../in_memory_file_system.h"

namespace shk {

TEST_CASE("CleaningFileSystem") {
  const std::string abc = "abc";

  InMemoryFileSystem inner_fs;
  inner_fs.writeFile("f", "contents");
  inner_fs.mkdir("dir");

  CleaningFileSystem fs(inner_fs);

  SECTION("mmap") {
    fs.mkdir("dir");
    CHECK_THROWS_AS(fs.mmap("nonexisting"), IoError);
    CHECK_THROWS_AS(fs.mmap("dir"), IoError);
    CHECK_THROWS_AS(fs.mmap("dir/nonexisting"), IoError);
    CHECK_THROWS_AS(fs.mmap("nonexisting/nonexisting"), IoError);
    CHECK(fs.mmap("f")->memory().asString() == "contents");
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("open") {
    CHECK(fs.open("f", "r"));
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("stat") {
    CHECK(fs.stat("f").result == ENOENT);
    CHECK(fs.lstat("f").result == ENOENT);
    CHECK(fs.stat("nonexisting").result == ENOENT);
    CHECK(fs.lstat("nonexisting").result == ENOENT);
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("mkdir") {
    fs.mkdir(abc);
    CHECK(inner_fs.stat(abc).result == ENOENT);
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("rmdir") {
    fs.rmdir("dir");
    CHECK(inner_fs.stat("dir").result == ENOENT);
  }

  SECTION("getRemovedCount") {
    CHECK(fs.getRemovedCount() == 0);
    SECTION("rmdir") {
      fs.rmdir("dir");
      CHECK(fs.getRemovedCount() == 1);
    }
    SECTION("unlink") {
      fs.unlink("f");
      CHECK(fs.getRemovedCount() == 1);
    }
    SECTION("both") {
      fs.rmdir("dir");
      fs.unlink("f");
      CHECK(fs.getRemovedCount() == 2);
    }
  }

  SECTION("unlink") {
    fs.unlink("f");
    CHECK(inner_fs.stat("f").result == ENOENT);
  }

  SECTION("symlink") {
    fs.symlink("target", "link");
    CHECK(inner_fs.lstat("link").result != ENOENT);
  }

  SECTION("rename") {
    fs.rename("f", "g");
    CHECK(inner_fs.stat("f").result == ENOENT);
    CHECK(inner_fs.stat("g").result != ENOENT);
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("truncate") {
    fs.truncate("f", 1);
    CHECK(inner_fs.readFile("f") == "c");
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("readDir") {
    CHECK(inner_fs.readDir(".") == fs.readDir("."));
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("readSymlink") {
    inner_fs.symlink("target", "link");
    CHECK(fs.readSymlink("link") == "target");
  }

  SECTION("readFile") {
    CHECK(fs.readFile("f") == "contents");
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("hashFile") {
    CHECK(fs.hashFile("f") == inner_fs.hashFile("f"));
    CHECK(fs.getRemovedCount() == 0);
  }

  SECTION("mkstemp") {
    auto tmp_file = fs.mkstemp("test.XXXXXXXX");
    CHECK(!tmp_file.empty());
    CHECK(inner_fs.stat(tmp_file).result != ENOENT);
    CHECK(fs.getRemovedCount() == 0);
  }
}

}  // namespace shk
