#include <catch.hpp>

#include "fs/file_id.h"

namespace shk {

TEST_CASE("FileId") {
  SECTION("create from Stat") {
    Stat stat;
    stat.metadata.ino = 3;
    stat.metadata.dev = 4;
    const FileId id(stat);
    CHECK(id.ino == 3);
    CHECK(id.dev == 4);
  }

  SECTION("operator==") {
    CHECK(FileId(1, 2) == FileId(1, 2));
    CHECK(!(FileId(1, 3) == FileId(1, 2)));
    CHECK(!(FileId(3, 2) == FileId(1, 2)));
  }

  SECTION("operator!=") {
    CHECK(!(FileId(1, 2) != FileId(1, 2)));
    CHECK(FileId(1, 3) != FileId(1, 2));
    CHECK(FileId(3, 2) != FileId(1, 2));
  }

  SECTION("hash") {
    CHECK(std::hash<FileId>()(FileId(1, 2)) == std::hash<FileId>()(FileId(1, 2)));
    CHECK(std::hash<FileId>()(FileId(1, 2)) != std::hash<FileId>()(FileId(2, 2)));
  }
}

}  // namespace shk
