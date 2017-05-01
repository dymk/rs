// Copyright 2017 Per Grön. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <catch.hpp>

#include <fcntl.h>
#include <unistd.h>

#include "fs/dry_run_file_system.h"

#include "../in_memory_file_system.h"

namespace shk {

TEST_CASE("DryRunFileSystem") {
  const std::string abc = "abc";

  InMemoryFileSystem inner_fs;
  CHECK(inner_fs.writeFile("f", "contents") == IoError::success());
  CHECK(inner_fs.mkdir("dir") == IoError::success());

  const auto fs = dryRunFileSystem(inner_fs);

  SECTION("mmap") {
    CHECK(fs->mkdir("dir") == IoError::success());
    CHECK_THROWS_AS(fs->mmap("nonexisting"), IoError);
    CHECK_THROWS_AS(fs->mmap("dir"), IoError);
    CHECK_THROWS_AS(fs->mmap("dir/nonexisting"), IoError);
    CHECK_THROWS_AS(fs->mmap("nonexisting/nonexisting"), IoError);
    CHECK(fs->mmap("f")->memory() == "contents");
  }

  SECTION("open") {
    // Not implemented
    CHECK_THROWS_AS(fs->open("f", "r"), IoError);
  }

  SECTION("stat") {
    CHECK(fs->stat("f").timestamps.mtime == inner_fs.stat("f").timestamps.mtime);
    CHECK(fs->lstat("f").timestamps.mtime == inner_fs.lstat("f").timestamps.mtime);
  }

  SECTION("mkdir") {
    CHECK(fs->mkdir(abc) == IoError::success());
    CHECK(inner_fs.stat(abc).result == ENOENT);
  }

  SECTION("rmdir") {
    CHECK(fs->rmdir("dir") == IoError::success());
    CHECK(inner_fs.stat("dir").result != ENOENT);
  }

  SECTION("unlink") {
    CHECK(fs->unlink("f") == IoError::success());
    CHECK(inner_fs.stat("f").result != ENOENT);
  }

  SECTION("symlink") {
    CHECK(fs->symlink("target", "link") == IoError::success());
    CHECK(inner_fs.stat("link").result == ENOENT);
  }

  SECTION("unlink") {
    CHECK(fs->rename("f", "g") == IoError::success());
    CHECK(inner_fs.stat("f").result != ENOENT);
    CHECK(inner_fs.stat("g").result == ENOENT);
  }

  SECTION("truncate") {
    CHECK(fs->truncate("f", 1) == IoError::success());
    CHECK(
        inner_fs.readFile("f") ==
        std::make_pair(std::string("contents"), IoError::success()));
  }

  SECTION("readDir") {
    auto inner = inner_fs.readDir(".");
    auto outer = fs->readDir(".");
    CHECK(inner == outer);
    CHECK(inner.second == IoError::success());
    CHECK(outer.second == IoError::success());
  }

  SECTION("readSymlink") {
    CHECK(inner_fs.symlink("target", "link") == IoError::success());
    CHECK(
        fs->readSymlink("link") ==
        std::make_pair(std::string("target"), IoError::success()));
  }

  SECTION("readFile") {
    CHECK(
        fs->readFile("f") ==
        std::make_pair(std::string("contents"), IoError::success()));
  }

  SECTION("hashFile") {
    std::string err_1;
    std::string err_2;
    CHECK(fs->hashFile("f", &err_1) == inner_fs.hashFile("f", &err_2));
    CHECK(err_1 == "");
    CHECK(err_2 == "");
  }

  SECTION("mkstemp") {
    std::string err;
    CHECK(
        fs->mkstemp("test.XXXXXXXX", &err) ==
        std::make_pair(std::string(""), true));
    CHECK(err == "");
  }
}

}  // namespace shk
