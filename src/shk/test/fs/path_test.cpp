#include <catch.hpp>

#include "fs/path.h"

#include "../in_memory_file_system.h"

namespace shk {
namespace {

std::string canonicalizePath(std::string path) throw(PathError) {
  shk::canonicalizePath(&path);
  return path;
}

#ifdef _WIN32
std::string canonicalizePathError(std::string path) {
  try {
    canonicalizePath(path);
    CHECK_THROWS((void)0);
    return "";
  } catch (PathError &error) {
    return error.what();
  }
}
#endif

void checkBasenameSplit(
    const std::string &path,
    const std::string &dirname,
    const std::string &basename) {
  string_view dn, bn;
  std::tie(dn, bn) = basenameSplitPiece(path);
  CHECK(std::string(dn) == dirname);
  CHECK(std::string(bn) == basename);
}

class FailingStatFileSystem : public FileSystem {
 public:
  std::unique_ptr<Stream> open(
      nt_string_view path, const char *mode) throw(IoError) override {
    return _fs.open(path, mode);
  }
  std::unique_ptr<Mmap> mmap(nt_string_view path) throw(IoError) override {
    return _fs.mmap(path);
  }
  Stat stat(nt_string_view path) override {
    Stat stat;
    stat.result = ENOENT;
    return stat;
  }
  Stat lstat(nt_string_view path) override {
    return stat(path);
  }
  void mkdir(nt_string_view path) throw(IoError) override {
    _fs.mkdir(path);
  }
  void rmdir(nt_string_view path) throw(IoError) override {
    _fs.rmdir(path);
  }
  void unlink(nt_string_view path) throw(IoError) override {
    _fs.unlink(path);
  }
  void symlink(
      nt_string_view target,
      nt_string_view source) throw(IoError) override {
    _fs.symlink(target, source);
  }
  void rename(
      nt_string_view old_path,
      nt_string_view new_path) throw(IoError) override {
    _fs.rename(old_path, new_path);
  }
  void truncate(
      nt_string_view path, size_t size) throw(IoError) override {
    _fs.truncate(path, size);
  }
  std::vector<DirEntry> readDir(
      nt_string_view path) throw(IoError) override {
    return _fs.readDir(path);
  }
  std::pair<std::string, bool> readSymlink(
      nt_string_view path, std::string *err) override {
    return _fs.readSymlink(path, err);
  }
  std::pair<std::string, bool> readFile(
      nt_string_view path, std::string *err) override {
    return _fs.readFile(path, err);
  }
  std::pair<Hash, bool> hashFile(
      nt_string_view path, std::string *err) override {
    return _fs.hashFile(path, err);
  }
  std::pair<std::string, bool> mkstemp(
      std::string &&filename_template, std::string *err) override {
    throw _fs.mkstemp(std::move(filename_template), err);
  }

 private:
  InMemoryFileSystem _fs;
};

}  // anonymous namespace

TEST_CASE("Path") {
  SECTION("basenameSplit") {
    checkBasenameSplit("/usr/lib", "/usr", "lib");
    checkBasenameSplit("/usr/", "/", "usr");
    checkBasenameSplit("/usr/////////", "/", "usr");
    checkBasenameSplit("usr", ".", "usr");
    checkBasenameSplit("/", "/", "/");
    checkBasenameSplit("//", "/", "/");
    checkBasenameSplit("/////", "/", "/");
    checkBasenameSplit(".", ".", ".");
    checkBasenameSplit("..", ".", "..");
    checkBasenameSplit("", ".", "");
  }

  SECTION("dirname") {
    // Not thoroughly tested because it's tested as part of basenameSplit
    CHECK(dirname(".") == ".");
    CHECK(dirname("/") == "/");
    CHECK(dirname("hej") == ".");
    CHECK(dirname("hej/a") == "hej");
    CHECK(dirname("/hej/a") == "/hej");
    CHECK(dirname("/hej") == "/");
  }

  SECTION("canonicalizePath") {
    SECTION("Path samples") {
      CHECK("." == canonicalizePath(""));
      CHECK("." == canonicalizePath("."));
      CHECK("." == canonicalizePath("./."));
      CHECK("foo.h" == canonicalizePath("foo.h"));
      CHECK("foo.h" == canonicalizePath("./foo.h"));
      CHECK("foo/bar.h" == canonicalizePath("./foo/./bar.h"));
      CHECK("x/bar.h" == canonicalizePath("./x/foo/../bar.h"));
      CHECK("bar.h" == canonicalizePath("./x/foo/../../bar.h"));
      CHECK("foo/bar" == canonicalizePath("foo//bar"));
      CHECK("bar" == canonicalizePath("foo//.//..///bar"));
      CHECK("../bar.h" == canonicalizePath("./x/../foo/../../bar.h"));
      CHECK("foo" == canonicalizePath("foo/./."));
      CHECK("foo" == canonicalizePath("foo/bar/.."));
      CHECK("foo/.hidden_bar" == canonicalizePath("foo/.hidden_bar"));
      CHECK("/foo" == canonicalizePath("/foo"));
#ifdef _WIN32
      CHECK("//foo" == canonicalizePath("//foo"));
#else
      CHECK("/foo" == canonicalizePath("//foo"));
#endif
      CHECK("/" == canonicalizePath("/"));
      CHECK("/" == canonicalizePath("//"));
      CHECK("/" == canonicalizePath("/////"));
    }

#ifdef _WIN32
    SECTION("Path samples on Windows") {
      CHECK("foo.h" == canonicalizePath(".\\foo.h"));
      CHECK("foo/bar.h" == canonicalizePath(".\\foo\\.\\bar.h"));
      CHECK("x/bar.h" == canonicalizePath(".\\x\\foo\\..\\bar.h"));
      CHECK("bar.h" == canonicalizePath(".\\x\\foo\\..\\..\\bar.h"));
      CHECK("foo/bar" == canonicalizePath("foo\\\\bar"));
      CHECK("bar" == canonicalizePath("foo\\\\.\\\\..\\\\\\bar"));
      CHECK("../bar.h" == canonicalizePath(".\\x\\..\\foo\\..\\..\\bar.h"));
      CHECK("foo" == canonicalizePath("foo\\.\\."));
      CHECK("foo" == canonicalizePath("foo\\bar\\.."));
      CHECK("foo/.hidden_bar" == canonicalizePath("foo\\.hidden_bar"));
      CHECK("/foo" == canonicalizePath("\\foo"));
      CHECK("//foo" == canonicalizePath("\\\\foo"));
      CHECK("" == canonicalizePath("\\"));
      CHECK(canonicalizePath("foo.h") == "foo.h");
      CHECK(canonicalizePath("a\\foo.h") == "a/foo.h");
      CHECK(canonicalizePath("a/bcd/efh\\foo.h") == "a/bcd/efh/foo.h");
      CHECK(canonicalizePath("a\\bcd/efh\\foo.h") == "a/bcd/efh/foo.h");
      CHECK(canonicalizePath("a\\bcd\\efh\\foo.h") == "a/bcd/efh/foo.h");
      CHECK(canonicalizePath("a/bcd/efh/foo.h") == "a/bcd/efh/foo.h");
      CHECK(canonicalizePath("a\\./efh\\foo.h") == "a/efh/foo.h");
      CHECK(canonicalizePath("a\\../efh\\foo.h") == "efh/foo.h");
      CHECK(canonicalizePath("a\\b\\c\\d\\e\\f\\g\\foo.h") == "a/b/c/d/e/f/g/foo.h");
      CHECK(canonicalizePath("a\\b\\c\\..\\..\\..\\g\\foo.h") == "g/foo.h");
      CHECK(canonicalizePath("a\\b/c\\../../..\\g\\foo.h") == "g/foo.h");
      CHECK(canonicalizePath("a\\b/c\\./../..\\g\\foo.h") == "a/g/foo.h");
      CHECK(canonicalizePath("a\\b/c\\./../..\\g/foo.h") == "a/g/foo.h");
      CHECK(canonicalizePath("a\\\\\\foo.h") == "a/foo.h");
      CHECK(canonicalizePath("a/\\\\foo.h") == "a/foo.h");
      CHECK(canonicalizePath("a\\//foo.h") == "a/foo.h");
    }

    SECTION("Windows Slash Tracking") {
      SECTION("Canonicalize Not Exceeding Len") {
        // Make sure searching \/ doesn't go past supplied len.
        char buf[] = "foo/bar\\baz.h\\";  // Last \ past end.
        size_t size = 13;
        canonicalizePath(buf, &size);
        EXPECT_EQ(0, strncmp("foo/bar/baz.h", buf, size));
      }

      SECTION("TooManyComponents") {
        // 64 is OK.
        std::string path =
            "a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a"
            "/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./x.h";
        canonicalizePath(&path);

        // Backslashes version.
        path =
            "a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\."
            "\\a\\.\\a\\.\\a\\.\\a\\.\\"
            "a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\."
            "\\a\\.\\a\\.\\a\\.\\a\\.\\x.h";
        canonicalizePath(&path);

        // 65 is not.
        path =
            "a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/"
            "a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./a/./x.h";
        CHECK(canonicalizePathError(path) == "too many path components");

        // Backslashes version.
        path =
            "a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\."
            "\\a\\.\\a\\.\\a\\.\\a\\.\\"
            "a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\.\\a\\."
            "\\a\\.\\a\\.\\a\\.\\a\\.\\a\\x.h";
        CHECK(canonicalizePathError(path) == "too many path components");
      }
    }
#endif

    SECTION("UpDir") {
      CHECK("../../foo/bar.h" == canonicalizePath("../../foo/bar.h"));
      CHECK("../foo/bar.h" == canonicalizePath("test/../../foo/bar.h"));
    }

    SECTION("AbsolutePath") {
      CHECK("/usr/include/stdio.h" == canonicalizePath("/usr/include/stdio.h"));
    }

    SECTION("NotNullTerminated") {
      std::string path;
      size_t len;

      path = "foo/. bar/.";
      len = strlen("foo/.");  // Canonicalize only the part before the space.
      canonicalizePath(&path[0], &len);
      CHECK(strlen("foo") == len);
      CHECK("foo/. bar/." == path);

      path = "foo/../file bar/.";
      len = strlen("foo/../file");
      canonicalizePath(&path[0], &len);
      CHECK(strlen("file") == len);
      CHECK("file ./file bar/." == path);
    }
  }

  SECTION("operator==, operator!=") {
    InMemoryFileSystem fs;
    fs.mkdir("dir");
    std::string err;
    CHECK(fs.writeFile("f", "", &err));
    CHECK(err == "");
    CHECK(fs.writeFile("dir/f", "", &err));
    CHECK(err == "");

    static const char *kPaths[] = {
        "/",
        "/dir",
        "/f",
        "/dir/f",
        "/dir/../f" };

    for (const char *path1_string : kPaths) {
      for (const char *path2_string : kPaths) {
        Paths paths(fs);
        const auto path1 = paths.get(path1_string);
        const auto path2 = paths.get(path2_string);
        if (path1_string == path2_string) {
          CHECK(path1 == path2);
        } else {
          CHECK(path1 != path2);
        }
      }
    }
  }

  SECTION("original") {
    InMemoryFileSystem fs;
    fs.open("file", "w");
    fs.open("other_file", "w");
    fs.mkdir("dir");
    Paths paths(fs);

    CHECK(paths.get("file").original() == "file");
    CHECK(paths.get("dir/.").original() == "dir/.");
    CHECK(paths.get("dir/../nonexisting").original() == "dir/../nonexisting");
  }

  SECTION("exists") {
    InMemoryFileSystem fs;
    fs.open("file", "w");
    fs.open("other_file", "w");
    fs.mkdir("dir");
    Paths paths(fs);

    CHECK(paths.get("file").exists());
    CHECK(paths.get("dir/.").exists());
    CHECK(!paths.get("dir/../nonexisting").exists());
    CHECK(!paths.get("nonexisting").exists());
  }

  SECTION("fileId") {
    InMemoryFileSystem fs;
    fs.open("file", "w");
    fs.open("other_file", "w");
    fs.mkdir("dir");
    Paths paths(fs);

    const auto file = fs.stat("file").metadata;
    const auto dir = fs.stat("dir").metadata;

    CHECK(paths.get("file").fileId() == FileId(file.ino, file.dev));
    CHECK(paths.get("dir/.").fileId() == FileId(dir.ino, dir.dev));
    CHECK(!paths.get("dir/../nonexisting").fileId());
    CHECK(!paths.get("nonexisting").fileId());
  }

  SECTION("Paths.get") {
    InMemoryFileSystem fs;
    fs.open("file", "w");
    fs.open("other_file", "w");
    fs.mkdir("dir");
    Paths paths(fs);

    CHECK(paths.get("/a").isSame(paths.get("a")));
    CHECK(!paths.get("/a").isSame(paths.get("/b")));
    CHECK(!paths.get("/a").isSame(paths.get("/a/b")));
    CHECK(!paths.get("a").isSame(paths.get("b")));
    CHECK(!paths.get("hey").isSame(paths.get("b")));
    CHECK(!paths.get("hey").isSame(paths.get("there")));
    CHECK(!paths.get("a/hey").isSame(paths.get("a/there")));
    CHECK(!paths.get("hey/a").isSame(paths.get("there/a")));

    CHECK_THROWS_AS(paths.get(""), PathError);
    CHECK(paths.get("/") != paths.get("//"));
    CHECK(paths.get("/").isSame(paths.get("//")));
    CHECK(paths.get("/").isSame(paths.get("///")));

    CHECK(paths.get("/missing") != paths.get("//missing"));
    CHECK(paths.get("/missing").isSame(paths.get("//missing")));

    CHECK(paths.get("/file") != paths.get("//file"));
    CHECK(paths.get("/file").isSame(paths.get("//file")));

    CHECK(paths.get("/dir") == paths.get("/dir"));
    CHECK(paths.get("/dir") != paths.get("//dir"));
    CHECK(paths.get("/dir").isSame(paths.get("//dir")));

    CHECK(paths.get("/dir/file").isSame(paths.get("/dir//file")));
    CHECK(paths.get("/./dir/file").isSame(paths.get("/dir//file")));
    CHECK(paths.get("/dir/file").isSame(paths.get("/dir/file/.")));
    CHECK(paths.get("/./dir/file").isSame(paths.get("/dir/./file/../file")));

    CHECK(!paths.get("/dir/file_").isSame(paths.get("/dir/file")));
    CHECK(!paths.get("/file_").isSame(paths.get("/file")));
    CHECK(!paths.get("/dir").isSame(paths.get("/file")));
    CHECK(!paths.get("/other_file").isSame(paths.get("/file")));

    CHECK(paths.get(".").isSame(paths.get("./")));

    CHECK_THROWS_AS(paths.get("/file/"), PathError);
    CHECK_THROWS_AS(paths.get("/file/blah"), PathError);
    CHECK_THROWS_AS(paths.get("/file//"), PathError);
    CHECK_THROWS_AS(paths.get("/file/./"), PathError);
    CHECK_THROWS_AS(paths.get("/file/./x"), PathError);

    FailingStatFileSystem failing_stat_fs;
    CHECK_THROWS_AS(Paths(failing_stat_fs).get("/"), PathError);
    CHECK_THROWS_AS(Paths(failing_stat_fs).get("."), PathError);
  }

  SECTION("IsSameHash") {
    InMemoryFileSystem fs;
    Paths paths(fs);
    std::unordered_set<Path, Path::IsSameHash, Path::IsSame> set;

    const auto a = paths.get("a/./b");
    const auto b = paths.get("a/b");
    set.insert(a);
    CHECK(set.find(b) != set.end());
    set.insert(b);
    CHECK(set.size() == 1);
  }
}

}  // namespace shk
