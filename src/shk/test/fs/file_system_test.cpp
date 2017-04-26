#include <catch.hpp>

#include <sys/stat.h>

#include "fs/file_system.h"

#include "../in_memory_file_system.h"

namespace shk {
namespace {

void writeFile(FileSystem &fs, nt_string_view path, string_view contents) {
  std::string err;
  CHECK(fs.writeFile(path, contents, &err));
  CHECK(err == "");
}

void symlink(FileSystem &fs, nt_string_view target, nt_string_view source) {
  std::string err;
  CHECK(fs.symlink(target, source, &err));
  CHECK(err == "");
}

}  // anonymous namespace

TEST_CASE("FileSystem") {
  InMemoryFileSystem fs;

  SECTION("DirEntry") {
    DirEntry r(DirEntry::Type::REG, "f");
    CHECK(r.type == DirEntry::Type::REG);
    CHECK(r.name == "f");

    DirEntry d(DirEntry::Type::DIR, "d");
    CHECK(d.type == DirEntry::Type::DIR);
    CHECK(d.name == "d");

    DirEntry r_copy = r;
    CHECK(d < r);
    CHECK(!(r < d));
    CHECK(!(r < r));
    CHECK(!(r < r_copy));
    CHECK(!(d < d));

    CHECK(r_copy == r);
    CHECK(!(r_copy != r));
    CHECK(!(r == d));
    CHECK(r != d);
  }

  SECTION("hashDir") {
    fs.mkdir("d");
    fs.mkdir("e");

    std::string err_1;
    std::string err_2;

    CHECK(fs.hashDir("d", &err_1) == fs.hashDir("e", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");

    fs.mkdir("d/d");
    const auto hash_with_one_dir = fs.hashDir("d", &err_1);
    CHECK(hash_with_one_dir != fs.hashDir("e", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");

    fs.open("d/e", "w");
    const auto hash_with_one_dir_and_one_file = fs.hashDir("d", &err_1);
    CHECK(hash_with_one_dir_and_one_file != hash_with_one_dir);
    CHECK(hash_with_one_dir_and_one_file != fs.hashDir("e", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");

    fs.unlink("d/e");
    CHECK(hash_with_one_dir == fs.hashDir("d", &err_1));
    CHECK(err_1 == "");

    fs.rmdir("d/d");
    CHECK(fs.hashDir("d", &err_1) == fs.hashDir("e", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");

    CHECK(fs.hashDir("nonexisting", &err_1) == std::make_pair(Hash(), false));
    CHECK(err_1 != "");
  }

  SECTION("hashSymlink") {
    symlink(fs, "target", "link_1");
    symlink(fs, "target", "link_2");
    symlink(fs, "target_other", "link_3");

    std::string err_1;
    std::string err_2;

    CHECK(fs.hashSymlink("link_1", &err_1) == fs.hashSymlink("link_2", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");
    CHECK(fs.hashSymlink("link_2", &err_1) != fs.hashSymlink("link_3", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");

    CHECK(fs.hashSymlink("missing", &err_1).second == false);
    CHECK(err_1 != "");
  }

  SECTION("writeFile") {
    writeFile(fs, "abc", "hello");
    CHECK(fs.stat("abc").result == 0);  // Verify file exists
  }

  SECTION("writeFile, readFile") {
    writeFile(fs, "abc", "hello");
    std::string err;
    CHECK(
        fs.readFile("abc", &err) ==
        std::make_pair(std::string("hello"), true));
  }

  SECTION("writeFile, writeFile, readFile") {
    writeFile(fs, "abc", "hello");
    writeFile(fs, "abc", "hello!");
    std::string err;
    CHECK(
        fs.readFile("abc", &err) ==
        std::make_pair(std::string("hello!"), true));
  }

  SECTION("mkdirs") {
    const std::string abc = "abc";

    SECTION("single directory") {
      const auto dirs = mkdirs(fs, abc);
      CHECK(S_ISDIR(fs.stat(abc).metadata.mode));
      CHECK(dirs == std::vector<std::string>({ abc }));
    }

    SECTION("already existing directory") {
      mkdirs(fs, abc);
      const auto dirs = mkdirs(fs, abc);  // Should be ok
      CHECK(S_ISDIR(fs.stat(abc).metadata.mode));
      CHECK(dirs.empty());
    }

    SECTION("over file") {
      fs.open(abc, "w");
      CHECK_THROWS_AS(mkdirs(fs, abc), IoError);
    }

    SECTION("several directories") {
      const std::string dir_path = "abc/def/ghi";
      const std::string file_path = "abc/def/ghi/jkl";
      const auto dirs = mkdirs(fs, dir_path);
      CHECK(dirs == std::vector<std::string>({
          "abc",
          "abc/def",
          "abc/def/ghi" }));
      writeFile(fs, file_path, "hello");
    }
  }
}

}  // namespace shk
