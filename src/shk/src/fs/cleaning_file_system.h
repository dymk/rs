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

#pragma once

#include "fs/file_system.h"

namespace shk {

/**
 * Used when cleaning.
 *
 * File system that acts like a normal file system, with some differences:
 *
 * 1. It counts the number of removed files, for reporting purposes.
 * 2. It lies about file stats, to ensure that everything is treated as dirty.
 * 3. It doesn't create directories.
 */
class CleaningFileSystem : public FileSystem {
 public:
  CleaningFileSystem(FileSystem &inner_file_system);

  int getRemovedCount() const;

  std::unique_ptr<Stream> open(
      nt_string_view path, const char *mode) throw(IoError) override;

  USE_RESULT std::pair<std::unique_ptr<Mmap>, IoError> mmap(
      nt_string_view path) override;

  Stat stat(nt_string_view path) override;

  Stat lstat(nt_string_view path) override;

  USE_RESULT IoError mkdir(nt_string_view path) override;

  USE_RESULT IoError rmdir(nt_string_view path) override;

  USE_RESULT IoError unlink(nt_string_view path) override;

  USE_RESULT IoError symlink(
      nt_string_view target,
      nt_string_view source) override;

  USE_RESULT IoError rename(
      nt_string_view old_path,
      nt_string_view new_path) override;

  USE_RESULT IoError truncate(nt_string_view path, size_t size) override;

  USE_RESULT std::pair<std::vector<DirEntry>, IoError> readDir(
      nt_string_view path) override;

  USE_RESULT std::pair<std::string, IoError> readSymlink(
      nt_string_view path) override;

  USE_RESULT std::pair<std::string, IoError> readFile(
      nt_string_view path) override;

  USE_RESULT std::pair<Hash, IoError> hashFile(
      nt_string_view path) override;

  USE_RESULT std::pair<std::string, IoError> mkstemp(
      std::string &&filename_template) override;

 private:
  FileSystem &_inner;
  int _removed_count = 0;
};

}  // namespace shk
