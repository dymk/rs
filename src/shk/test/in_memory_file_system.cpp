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

#include "in_memory_file_system.h"

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include <blake2.h>
#include <util/path_operations.h>

#include "fs/path.h"

namespace shk {

InMemoryFileSystem::InMemoryFileSystem(const std::function<time_t ()> &clock)
    : _clock(clock) {
  _directories.emplace("/", Directory(clock(), _ino++));
}

void InMemoryFileSystem::enqueueMkstempResult(std::string &&path) {
  _mkstemp_paths.push_back(std::move(path));
}

USE_RESULT std::pair<std::unique_ptr<FileSystem::Stream>, IoError>
InMemoryFileSystem::open(
    nt_string_view path,
    const char *mode) {
  return open(/*expect_symlink:*/false, path, mode);
}

USE_RESULT std::pair<std::unique_ptr<FileSystem::Mmap>, IoError>
InMemoryFileSystem::mmap(nt_string_view path) {
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return {
        nullptr,
        IoError("A component of the path prefix is not a directory", ENOTDIR) };
  case EntryType::DIRECTORY:
    return {
        nullptr,
        IoError("The named file is a directory", EISDIR) };
  case EntryType::FILE_DOES_NOT_EXIST:
    return {
        nullptr,
        IoError("No such file or directory", ENOENT) };
  case EntryType::FILE: {
    const auto &file = l.directory->files[l.basename];
    return {
        std::unique_ptr<Mmap>(new InMemoryMmap(file)),
        IoError::success() };
  }
  }
}

Stat InMemoryFileSystem::stat(nt_string_view path) {
  // Symlinks are not fully supported so stat is the same as lstat
  return stat(/*follow_symlink:*/true, path);
}

Stat InMemoryFileSystem::lstat(nt_string_view path) {
  return stat(/*follow_symlink:*/false, path);
}

USE_RESULT IoError InMemoryFileSystem::mkdir(nt_string_view path) {
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return IoError("A component of the path prefix is not a directory", ENOTDIR);
  case EntryType::FILE:
  case EntryType::DIRECTORY:
    return IoError("The named file exists", EEXIST);
  case EntryType::FILE_DOES_NOT_EXIST:
    l.directory->directories.emplace(l.basename);
    _directories.emplace(l.canonicalized, Directory(_clock(), _ino++));
    return IoError::success();
  }
}

USE_RESULT IoError InMemoryFileSystem::rmdir(nt_string_view path) {
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return IoError("A component of the path prefix is not a directory", ENOTDIR);
  case EntryType::FILE_DOES_NOT_EXIST:
    return IoError("The named directory does not exist", ENOENT);
  case EntryType::FILE:
    return IoError("The named directory is a file", EPERM);
  case EntryType::DIRECTORY:
    const auto &dir = _directories.find(l.canonicalized)->second;
    if (!dir.empty()) {
      return IoError(
          "The named directory contains files other than `.' and `..' in it",
          ENOTEMPTY);
    } else {
      l.directory->directories.erase(l.basename);
      l.directory->mtime = _clock();
      _directories.erase(l.canonicalized);
    }
    return IoError::success();
  }
}

USE_RESULT IoError InMemoryFileSystem::unlink(nt_string_view path) {
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return IoError("A component of the path prefix is not a directory", ENOTDIR);
  case EntryType::FILE_DOES_NOT_EXIST:
    return IoError("The named file does not exist", ENOENT);
  case EntryType::DIRECTORY:
    return IoError("The named file is a directory", EPERM);
  case EntryType::FILE:
    l.directory->files.erase(l.basename);
    l.directory->mtime = _clock();
    return IoError::success();
  }
}

USE_RESULT IoError InMemoryFileSystem::symlink(
      nt_string_view target,
      nt_string_view source) {
  std::string err;
  if (auto error = writeFile(source, target)) {
    return error;
  }
  const auto l = lookup(source);
  assert(l.entry_type == EntryType::FILE);

  const auto file = l.directory->files.find(l.basename)->second;
  file->symlink = true;

  return IoError::success();
}

USE_RESULT IoError InMemoryFileSystem::rename(
      nt_string_view old_path,
      nt_string_view new_path) {
  const auto old_l = lookup(old_path);
  const auto new_l = lookup(new_path);

  switch (old_l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return IoError(
        "A component of the path prefix is not a directory", ENOTDIR);

  case EntryType::FILE_DOES_NOT_EXIST:
    return IoError("The named file does not exist", ENOENT);

  case EntryType::DIRECTORY:
    switch (new_l.entry_type) {
    case EntryType::DIRECTORY_DOES_NOT_EXIST:
      return IoError(
          "A component of the path prefix is not a directory", ENOTDIR);
    case EntryType::FILE:
      return IoError("The new file exists but is not a directory", ENOTDIR);
    case EntryType::DIRECTORY:
      if (new_path != old_path) {
        if (auto io_error = rmdir(new_path)) {
          return io_error;
        }
      }
      [[clang::fallthrough]];
    case EntryType::FILE_DOES_NOT_EXIST:
      old_l.directory->directories.erase(old_l.basename);
      new_l.directory->directories.insert(new_l.basename);
      old_l.directory->mtime = _clock();
      new_l.directory->mtime = _clock();

      std::unordered_map<std::string, std::string> dirs_to_rename;
      for (const auto &dir : _directories) {
        const auto &dir_name = dir.first;
        if (dir_name.size() < old_l.canonicalized.size()) {
          continue;
        }
        if (std::equal(
                dir_name.begin(),
                dir_name.end(),
                old_l.canonicalized.begin())) {
          // Need to move entries in _directories around, but that cannot be
          // done while iterating over it.
          dirs_to_rename[dir_name] =
              new_l.canonicalized +
              dir_name.substr(old_l.canonicalized.size());
        }
      }

      for (const auto &dir_to_rename : dirs_to_rename) {
        if (dir_to_rename.first != dir_to_rename.second) {
          auto old_dir = std::move(_directories.find(dir_to_rename.first)->second);
          _directories.erase(dir_to_rename.first);
          _directories[dir_to_rename.second] = std::move(old_dir);
        }
      }
      break;
    }
    break;

  case EntryType::FILE:
    switch (new_l.entry_type) {
    case EntryType::DIRECTORY_DOES_NOT_EXIST:
      return IoError(
          "A component of the path prefix is not a directory", ENOTDIR);
    case EntryType::DIRECTORY:
      return IoError("The new file is a directory", EISDIR);
    case EntryType::FILE:
      if (new_path != old_path) {
        if (auto error = unlink(new_path)) {
          return error;
        }
      }
      [[clang::fallthrough]];
    case EntryType::FILE_DOES_NOT_EXIST:
      std::string contents;
      IoError error;
      std::tie(contents, error) = readFile(old_path);
      if (error) {
        return error;
      }
      if (auto error = unlink(old_path)) {
        return error;
      }
      if (auto error = writeFile(new_path, contents)) {
        return error;
      }
      break;
    }
    break;
  }

  return IoError::success();
}

USE_RESULT IoError InMemoryFileSystem::truncate(
    nt_string_view path, size_t size) {
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return IoError(
        "A component of the path prefix is not a directory", ENOTDIR);
  case EntryType::FILE_DOES_NOT_EXIST:
    return IoError("The named file does not exist", ENOENT);
  case EntryType::DIRECTORY:
    return IoError("The named file is a directory", EPERM);
  case EntryType::FILE:
    const auto file = l.directory->files.find(l.basename)->second;
    file->contents.resize(size);
    file->mtime = _clock();
    return IoError::success();
  }
}

USE_RESULT std::pair<std::vector<DirEntry>, IoError>
InMemoryFileSystem::readDir(nt_string_view path) {
  std::vector<DirEntry> result;
  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return std::make_pair(
        std::vector<DirEntry>(),
        IoError("A component of the path prefix is not a directory", ENOTDIR));
  case EntryType::FILE_DOES_NOT_EXIST:
    return std::make_pair(
        std::vector<DirEntry>(),
        IoError("The named directory does not exist", ENOENT));
  case EntryType::FILE:
    return std::make_pair(
        std::vector<DirEntry>(),
        IoError("The named directory is a file", EPERM));
  case EntryType::DIRECTORY:
    const auto &dir = _directories.find(l.canonicalized)->second;
    for (const auto &dir_path : dir.directories) {
      result.emplace_back(DirEntry::Type::DIR, dir_path);
    }
    for (const auto &file : dir.files) {
      result.emplace_back(DirEntry::Type::REG, file.first);
    }
    return std::make_pair(std::move(result), IoError::success());
  }
}

USE_RESULT std::pair<std::string, IoError> InMemoryFileSystem::readSymlink(
      nt_string_view path) {
  std::string result;

  std::unique_ptr<FileSystem::Stream> stream;
  IoError error;
  std::tie(stream, error) = open(/*expect_symlink:*/true, path, "r");
  if (error) {
    return { "", error };
  }

  uint8_t buf[1024];
  while (!stream->eof()) {
    size_t read_bytes;
    IoError error;
    std::tie(read_bytes, error) = stream->read(buf, 1, sizeof(buf));
    if (error) {
      return { "", error };
    }
    result.append(reinterpret_cast<char *>(buf), read_bytes);
  }
  return std::make_pair(std::move(result), IoError::success());
}

USE_RESULT std::pair<std::string, IoError> InMemoryFileSystem::readFile(
      nt_string_view path) {
  std::string result;

  std::unique_ptr<FileSystem::Stream> stream;
  IoError error;
  std::tie(stream, error) = open(path, "r");
  if (error) {
    return { "", error };
  }

  uint8_t buf[1024];
  while (!stream->eof()) {
    size_t read_bytes;
    IoError error;
    std::tie(read_bytes, error) = stream->read(buf, 1, sizeof(buf));
    if (error) {
      return { "", error };
    }
    result.append(reinterpret_cast<char *>(buf), read_bytes);
  }
  return std::make_pair(std::move(result), IoError::success());
}

USE_RESULT std::pair<Hash, IoError> InMemoryFileSystem::hashFile(
      nt_string_view path) {
  // This is optimized for readability rather than speed
  Hash hash;
  blake2b_state state;
  blake2b_init(&state, hash.data.size());
  std::string file_contents;
  IoError error;
  std::tie(file_contents, error) = readFile(path);
  if (error) {
    return std::make_pair(Hash(), error);
  }
  blake2b_update(
      &state,
      reinterpret_cast<const uint8_t *>(file_contents.data()),
      file_contents.size());
  blake2b_final(&state, hash.data.data(), hash.data.size());
  return std::make_pair(hash, IoError::success());
}

USE_RESULT std::pair<std::string, IoError> InMemoryFileSystem::mkstemp(
      std::string &&filename_template) {
  if (!_mkstemp_paths.empty()) {
    auto result = std::move(_mkstemp_paths.front());
    _mkstemp_paths.pop_front();
    return std::make_pair(std::move(result), IoError::success());
  }

  for (;;) {
    std::string filename = filename_template;
    if (mktemp(&filename[0]) == NULL) {
      return std::make_pair(
          std::string(),
          IoError(
              std::string("Failed to create path for temporary file: ") +
                  strerror(errno),
              0));
    }
    // This is potentially an infinite loop… but since this is for testing I
    // don't care to do anything about that.
    if (stat(filename).result == ENOENT) {
      filename_template = std::move(filename);
      if (auto error = writeFile(filename_template, "")) {
        return std::make_pair(std::string(), error);
      }
      return std::make_pair(std::move(filename_template), IoError::success());
    }
  }
}

bool InMemoryFileSystem::operator==(const InMemoryFileSystem &other) const {
  return _directories == other._directories;
}

bool InMemoryFileSystem::Directory::empty() const {
  return files.empty() && directories.empty();
}

bool InMemoryFileSystem::Directory::operator==(const Directory &other) const {
  return (
      files == other.files &&
      directories == other.directories);
}

InMemoryFileSystem::InMemoryFileStream::InMemoryFileStream(
    const std::function<time_t ()> &clock,
    const std::shared_ptr<File> &file,
    bool read,
    bool write,
    bool append)
    : _clock(clock),
      _read(read),
      _write(write),
      _position(append ? file->contents.size() : 0),
      _file(file) {}

USE_RESULT std::pair<size_t, IoError>
InMemoryFileSystem::InMemoryFileStream::read(
    uint8_t *ptr, size_t size, size_t nitems) {
  if (!_read) {
    return { 0, IoError("Attempted read from a write only stream", 0) };
  }
  if (auto error = checkNotEof()) {
    return { 0, error };
  }

  const auto bytes = size * nitems;
  const auto bytes_remaining = _file->contents.size() - _position;
  if (bytes > bytes_remaining) {
    _eof = true;
  }

  const auto items_to_read = std::min(bytes_remaining, bytes) / size;
  const auto bytes_to_read = items_to_read * size;

  const auto it = _file->contents.begin() + _position;
  std::copy(
      it,
      it + bytes_to_read,
      reinterpret_cast<char *>(ptr));
  _position += bytes_to_read;

  return { items_to_read, IoError::success() };
}

USE_RESULT IoError InMemoryFileSystem::InMemoryFileStream::write(
  const uint8_t *ptr, size_t size, size_t nitems) {
  if (!_write) {
    return IoError("Attempted write to a read only stream", 0);
  }
  if (auto error = checkNotEof()) {
    return error;
  }

  const auto bytes = size * nitems;
  const auto new_size = _position + bytes;
  if (_file->contents.size() < new_size) {
    _file->contents.resize(new_size);
  }
  std::copy(
      reinterpret_cast<const char *>(ptr),
      reinterpret_cast<const char *>(ptr + bytes),
      _file->contents.begin() + _position);
  _position += bytes;

  _file->mtime = _clock();

  return IoError::success();
}

USE_RESULT std::pair<long, IoError>
InMemoryFileSystem::InMemoryFileStream::tell() const {
  return { _position, IoError::success() };
}

bool InMemoryFileSystem::InMemoryFileStream::eof() const {
  return _eof;
}

USE_RESULT IoError InMemoryFileSystem::InMemoryFileStream::checkNotEof() const {
  return _eof ?
      IoError("Attempted to write to file that is past eof", 0) :
      IoError::success();
}

InMemoryFileSystem::InMemoryMmap::InMemoryMmap(const std::shared_ptr<File> &file)
    : _file(file) {}

string_view InMemoryFileSystem::InMemoryMmap::memory() {
  return string_view(_file->contents.c_str(), _file->contents.size());
}

USE_RESULT std::pair<std::unique_ptr<FileSystem::Stream>, IoError>
InMemoryFileSystem::open(
    bool expect_symlink,
    nt_string_view path,
    const char *mode) {
  const auto mode_string = std::string(mode);
  bool read = false;
  bool write = false;
  bool truncate = false;
  bool create = false;
  bool append = false;
  if (mode_string == "r") {
    read = true;
    write = false;
  } else if (mode_string == "r+") {
    read = true;
    write = false;
  } else if (mode_string == "w" || mode_string == "wb") {
    read = false;
    write = true;
    truncate = true;
    create = true;
  } else if (mode_string == "w+") {
    read = true;
    write = true;
    truncate = true;
    create = true;
  } else if (mode_string == "a" || mode_string == "ab") {
    read = false;
    write = true;
    append = true;
    create = true;
  } else {
    return { nullptr, IoError("Unsupported mode " + mode_string, 0) };
  }

  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    return {
        nullptr,
        IoError("A component of the path prefix is not a directory", ENOTDIR) };
  case EntryType::DIRECTORY:
    return {
        nullptr,
        IoError("The named file is a directory", EISDIR) };
  case EntryType::FILE_DOES_NOT_EXIST:
    if (!create) {
      return { nullptr, IoError("No such file or directory", ENOENT) };
    }
    {
      const auto &file = std::make_shared<File>(_ino++);
      file->mtime = _clock();
      l.directory->files[l.basename] = file;
      l.directory->mtime = _clock();
      return {
          std::unique_ptr<Stream>(
              new InMemoryFileStream(_clock, file, read, write, append)),
          IoError::success() };
    }
  case EntryType::FILE:
    {
      const auto &file = l.directory->files[l.basename];
      if (!expect_symlink && file->symlink) {
        return { nullptr, IoError("Can't open symlink file", EINVAL) };
      }
      if (truncate) {
        file->contents.clear();
      }
      return {
          std::unique_ptr<Stream>(
              new InMemoryFileStream(_clock, file, read, write, append)),
          IoError::success() };
    }
  }
}

Stat InMemoryFileSystem::stat(
    bool follow_symlink, nt_string_view path) {
  Stat stat;

  const auto l = lookup(path);
  switch (l.entry_type) {
  case EntryType::DIRECTORY_DOES_NOT_EXIST:
    stat.result = ENOTDIR;
    break;
  case EntryType::FILE_DOES_NOT_EXIST:
    stat.result = ENOENT;
    break;
  case EntryType::FILE:
  case EntryType::DIRECTORY:
    stat.metadata.mode = 0755;  // Pretend this is the umask
    if (l.entry_type == EntryType::FILE) {
      const auto &file = l.directory->files[l.basename];
      if (follow_symlink && file->symlink) {
        throw std::runtime_error("Symlink following is not supported");
      }
      stat.metadata.size = file->contents.size();
      stat.metadata.ino = file->ino;
      stat.metadata.mode |= file->symlink ? S_IFLNK : S_IFREG;
      stat.mtime = file->mtime;
    } else {
      const auto &dir = _directories.find(l.canonicalized)->second;
      stat.metadata.ino = dir.ino;
      stat.metadata.mode |= S_IFDIR;
      stat.mtime = dir.mtime;
    }
    // TODO(peck): Set mtime
    break;
  }

  return stat;
}

InMemoryFileSystem::LookupResult InMemoryFileSystem::lookup(
    nt_string_view path) {
  LookupResult result;
  result.canonicalized = "/" + std::string(path);
  try {
    canonicalizePath(&result.canonicalized);
  } catch (const PathError &path_error) {
    result.entry_type = EntryType::DIRECTORY_DOES_NOT_EXIST;
    return result;
  }

  string_view dirname_piece;
  string_view basename_piece;
  std::tie(dirname_piece, basename_piece) = basenameSplitPiece(
      result.canonicalized);
  const auto dirname = std::string(dirname_piece);
  const auto basename = std::string(basename_piece);

  const auto it = _directories.find(dirname);
  if (it == _directories.end()) {
    result.entry_type = EntryType::DIRECTORY_DOES_NOT_EXIST;
    return result;
  }

  auto &directory = it->second;

  if (basename == "/") {
    result.entry_type = EntryType::DIRECTORY;
  } else if (directory.files.count(basename)) {
    result.entry_type = EntryType::FILE;
  } else if (directory.directories.count(basename)) {
    result.entry_type = EntryType::DIRECTORY;
  } else {
    result.entry_type = EntryType::FILE_DOES_NOT_EXIST;
  }

  result.directory = &directory;
  result.basename = basename;
  return result;
}

}  // namespace shk
