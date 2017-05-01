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

#include "fs/cleaning_file_system.h"

#include <errno.h>

namespace shk {

CleaningFileSystem::CleaningFileSystem(FileSystem &inner_file_system)
    : _inner(inner_file_system) {}

int CleaningFileSystem::getRemovedCount() const {
  return _removed_count;
}

std::unique_ptr<FileSystem::Stream> CleaningFileSystem::open(
    nt_string_view path, const char *mode) throw(IoError) {
  return _inner.open(path, mode);
}

std::unique_ptr<FileSystem::Mmap> CleaningFileSystem::mmap(
    nt_string_view path) throw(IoError) {
  return _inner.mmap(path);
}

Stat CleaningFileSystem::stat(nt_string_view path) {
  Stat stat;
  stat.result = ENOENT;
  return stat;
}

Stat CleaningFileSystem::lstat(nt_string_view path) {
  Stat stat;
  stat.result = ENOENT;
  return stat;
}

void CleaningFileSystem::mkdir(nt_string_view path)
    throw(IoError) {
  // Don't make directories; the build process creates directories
  // for things that are about to be built.
}

void CleaningFileSystem::rmdir(nt_string_view path)
    throw(IoError) {
  _inner.rmdir(path);
  _removed_count++;
}

void CleaningFileSystem::unlink(nt_string_view path)
    throw(IoError) {
  _inner.unlink(path);
  _removed_count++;
}

bool CleaningFileSystem::symlink(
      nt_string_view target, nt_string_view source, std::string *err) {
  return _inner.symlink(target, source, err);
}

bool CleaningFileSystem::rename(
      nt_string_view old_path,
      nt_string_view new_path,
      std::string *err) {
  return _inner.rename(old_path, new_path, err);
}

USE_RESULT IoError CleaningFileSystem::truncate(
    nt_string_view path, size_t size) {
  return _inner.truncate(path, size);
}

std::pair<std::vector<DirEntry>, bool> CleaningFileSystem::readDir(
      nt_string_view path, std::string *err) {
  return _inner.readDir(path, err);
}

std::pair<std::string, bool> CleaningFileSystem::readSymlink(
      nt_string_view path, std::string *err) {
  return _inner.readSymlink(path, err);
}

std::pair<std::string, bool> CleaningFileSystem::readFile(
      nt_string_view path, std::string *err) {
  return _inner.readFile(path, err);
}

std::pair<Hash, bool> CleaningFileSystem::hashFile(
      nt_string_view path, std::string *err) {
  return _inner.hashFile(path, err);
}

std::pair<std::string, bool> CleaningFileSystem::mkstemp(
      std::string &&filename_template, std::string *err) {
  return _inner.mkstemp(std::move(filename_template), err);
}

}  // namespace shk
