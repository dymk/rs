// Copyright 2011 Google Inc. All Rights Reserved.
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


#include "fs/persistent_file_system.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/dir.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <blake2.h>

#include <util/assert.h>
#include <util/raii_helper.h>

#include "nullterminated_string.h"

namespace shk {
namespace {

using FileHandle = RAIIHelper<FILE *, int, fclose>;

class PersistentFileSystem : public FileSystem {
  template<typename T>
  static std::pair<T, IoError> checkForMinusOne(T result) {
    if (result == -1) {
      return { T(), IoError(strerror(errno), errno) };
    } else {
      return { result, IoError::success() };
    }
  }

  template<typename T>
  static IoError checkForMinusOneIoError(T result) {
    if (result == -1) {
      return IoError(strerror(errno), errno);
    } else {
      return IoError::success();
    }
  }

  class FileStream : public FileSystem::Stream {
   public:
    FileStream(FileHandle &&handle)
        : _f(std::move(handle)) {}

    static USE_RESULT std::pair<std::unique_ptr<Stream>, IoError>
    open(nt_string_view path, const char *mode) {
      FileHandle handle(fopen(NullterminatedString(path).c_str(), mode));
      if (!handle.get()) {
        return { nullptr, IoError(strerror(errno), errno) };
      }
      fcntl(fileno(handle.get()), F_SETFD, FD_CLOEXEC);
      return {
          std::unique_ptr<Stream>(new FileStream(std::move(handle))),
          IoError::success() };
    }

    USE_RESULT std::pair<size_t, IoError> read(
        uint8_t *ptr, size_t size, size_t nitems) override {
      auto result = fread(ptr, size, nitems, _f.get());
      if (eof()) {
        return std::make_pair(result, IoError::success());
      } else if (ferror(_f.get()) != 0) {
        return std::make_pair(0, IoError("Failed to read from stream", 0));
      } else {
        SHK_ASSERT(result == nitems);
        return std::make_pair(result, IoError::success());
      }
    }

    USE_RESULT IoError write(
        const uint8_t *ptr, size_t size, size_t nitems) override {
      fwrite(ptr, size, nitems, _f.get());
      if (ferror(_f.get()) != 0) {
        return IoError("Failed to write to stream", 0);
      }
      return IoError::success();
    }

    USE_RESULT std::pair<long, IoError> tell() const override {
      return checkForMinusOne(ftell(_f.get()));
    }

    bool eof() const override {
      return feof(_f.get()) != 0;
    }

   private:
    FileHandle _f;
  };

  class FileMmap : public Mmap {
   public:
    FileMmap(size_t size, void *memory, int fd)
        : _size(size),
          _memory(memory),
          _f(fd) {}

    static USE_RESULT std::pair<std::unique_ptr<Mmap>, IoError> open(
        nt_string_view path) {
      struct stat input;
      NullterminatedString nt_path(path);
      auto ret = ::stat(nt_path.c_str(), &input);
      if (ret == -1) {
        return { nullptr, IoError(strerror(errno), errno) };
      }

      if (input.st_size) {
        int fd = ::open(nt_path.c_str(), O_RDONLY);
        if (fd == -1) {
          return { nullptr, IoError(strerror(errno), errno) };
        }

        void *memory =
            ::mmap(nullptr, input.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (memory == MAP_FAILED) {
          return { nullptr, IoError(strerror(errno), errno) };
        }

        return {
            std::unique_ptr<Mmap>(new FileMmap(input.st_size, memory, fd)),
            IoError::success() };
      } else {
        return {
            std::unique_ptr<Mmap>(new FileMmap(0, nullptr, -1)),
            IoError::success() };
      }
    }

    virtual ~FileMmap() {
      if (_memory != MAP_FAILED) {
        munmap(_memory, _size);
      }

      if (_f != -1) {
        close(_f);
      }
    }

    string_view memory() override {
      return string_view(static_cast<const char *>(_memory), _size);
    }

   private:
    size_t _size;
    void *_memory;
    int _f;
  };

 public:
  USE_RESULT std::pair<std::unique_ptr<Stream>, IoError> open(
      nt_string_view path, const char *mode) override {
    return FileStream::open(path, mode);
  }

  USE_RESULT std::pair<std::unique_ptr<Mmap>, IoError> mmap(
      nt_string_view path) override {
    return FileMmap::open(path);
  }

  Stat stat(nt_string_view path) override {
    return genericStat(::stat, path);
  }

  Stat lstat(nt_string_view path) override {
    return genericStat(::lstat, path);
  }

  USE_RESULT IoError mkdir(nt_string_view path) override {
    return checkForMinusOneIoError(
        ::mkdir(NullterminatedString(path).c_str(), 0777));
  }

  USE_RESULT IoError rmdir(nt_string_view path) override {
    return checkForMinusOneIoError(
        ::rmdir(NullterminatedString(path).c_str()));
  }

  USE_RESULT IoError unlink(nt_string_view path) override {
    return checkForMinusOneIoError(
        ::unlink(NullterminatedString(path).c_str()));
  }

  USE_RESULT IoError symlink(
      nt_string_view target,
      nt_string_view source) override {
    return checkForMinusOneIoError(
        ::symlink(
            NullterminatedString(target).c_str(),
            NullterminatedString(source).c_str()));
  }

  USE_RESULT IoError rename(
      nt_string_view old_path,
      nt_string_view new_path) override {
    return checkForMinusOneIoError(::rename(
        NullterminatedString(old_path).c_str(),
        NullterminatedString(new_path).c_str()));
  }

  USE_RESULT IoError truncate(nt_string_view path, size_t size) override {
    return checkForMinusOneIoError(
        ::truncate(NullterminatedString(path).c_str(), size));
  }

  USE_RESULT std::pair<std::vector<DirEntry>, IoError> readDir(
      nt_string_view path) override {
    std::vector<DirEntry> result;

    DIR *dp = opendir(NullterminatedString(path).c_str());
    if (!dp) {
      return std::make_pair(
          std::vector<DirEntry>(),
          IoError(strerror(errno), errno));
    }

    dirent *dptr;
    while (NULL != (dptr = readdir(dp))) {
      result.emplace_back(direntTypeToType(dptr->d_type), dptr->d_name);
    }
    closedir(dp);

    return std::make_pair(std::move(result), IoError::success());
  }

  USE_RESULT std::pair<std::string, IoError> readSymlink(
      nt_string_view path) override {
    std::vector<char> buf;
    int to_reserve = 128;

    int res = 0;
    for (;;) {
      buf.resize(to_reserve);
      res = readlink(
          NullterminatedString(path).c_str(),
          buf.data(),
          buf.capacity());
      if (res == to_reserve) {
        to_reserve *= 2;
      } else {
        break;
      }
    }

    if (res == -1) {
      return std::make_pair("", IoError("Failed to read symlink", 0));
    }

    return std::make_pair(buf.data(), IoError::success());
  }

  USE_RESULT std::pair<std::string, IoError> readFile(
      nt_string_view path) override {
    const auto file_stat = stat(path);
    std::string contents;
    contents.reserve(file_stat.metadata.size);
    auto error = processFile(path, [&contents](const char *buf, size_t len) {
      contents.append(buf, len);
    });
    if (error) {
      return { "", error };
    }
    return std::make_pair(std::move(contents), IoError::success());
  }

  USE_RESULT std::pair<Hash, IoError> hashFile(
      nt_string_view path, string_view extra_data) override {
    Hash hash;
    blake2b_state state;
    blake2b_init(&state, hash.data.size());

    const auto process = [&state](const char *buf, size_t len) {
      blake2b_update(&state, reinterpret_cast<const uint8_t *>(buf), len);
    };

    process(extra_data.data(), extra_data.size());

    auto error = processFile(path, process);
    if (error) {
      return { Hash(), error };
    }
    blake2b_final(&state, hash.data.data(), hash.data.size());
    return std::make_pair(hash, IoError::success());
  }

  USE_RESULT std::pair<std::string, IoError> mkstemp(
      std::string &&filename_template) override {
    const auto fd = ::mkstemp(&filename_template[0]);
    if (fd == -1) {
      return std::make_pair(
          std::string(""),
          IoError(
              std::string("Failed to create path for temporary file: ") +
                  strerror(errno),
              errno));
    }
    close(fd);
    return std::make_pair(filename_template, IoError::success());
  }

 private:
  static DirEntry::Type direntTypeToType(unsigned char type) {
    switch (type) {
    case DT_FIFO:
      return DirEntry::Type::FIFO;
    case DT_CHR:
      return DirEntry::Type::CHR;
    case DT_DIR:
      return DirEntry::Type::DIR;
    case DT_BLK:
      return DirEntry::Type::BLK;
    case DT_REG:
      return DirEntry::Type::REG;
    case DT_LNK:
      return DirEntry::Type::LNK;
    case DT_SOCK:
      return DirEntry::Type::SOCK;
    case DT_WHT:
      return DirEntry::Type::WHT;
    case DT_UNKNOWN:
    default:
      return DirEntry::Type::UNKNOWN;
    }
  }

  template<typename Append>
  IoError processFile(
      nt_string_view path,
      Append &&append) {
#ifdef _WIN32
    // This makes a ninja run on a set of 1500 manifest files about 4% faster
    // than using the generic fopen code below.
    err->clear();
    HANDLE f = ::CreateFile(
        NullterminatedString(path).c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (f == INVALID_HANDLE_VALUE) {
      return IoError(GetLastErrorString(), ENOENT);
    }

    for (;;) {
      DWORD len;
      char buf[64 << 10];
      if (!::ReadFile(f, buf, sizeof(buf), &len, NULL)) {
        return IoError(GetLastErrorString(), 0);
      }
      if (len == 0) {
        break;
      }
      append(buf, len);
    }
    ::CloseHandle(f);
#else
    FileHandle f(fopen(NullterminatedString(path).c_str(), "rb"));
    if (!f.get()) {
      return IoError(strerror(errno), errno);
    }
    fcntl(fileno(f.get()), F_SETFD, FD_CLOEXEC);

    char buf[64 << 10];
    size_t len;
    while ((len = fread(buf, 1, sizeof(buf), f.get())) > 0) {
      append(buf, len);
    }
    if (ferror(f.get())) {
      const auto err = errno;
      return IoError(strerror(err), err);
    }
#endif
    return IoError::success();
  }

  template<typename StatFunction>
  Stat genericStat(StatFunction fn, nt_string_view path) {
    Stat result;
    struct stat input;
    auto ret = fn(NullterminatedString(path).c_str(), &input);
    if (ret == -1) {
      result.result = errno;
    } else {
      result.result = 0;
      result.metadata.ino = input.st_ino;
      result.metadata.dev = input.st_dev;
      result.metadata.mode = input.st_mode;
      result.metadata.size = input.st_size;
      result.mtime = input.st_mtime;
    }
    return result;
  }
};

}  // anonymous namespace

std::unique_ptr<FileSystem> persistentFileSystem() {
  return std::unique_ptr<FileSystem>(new PersistentFileSystem());
}

}  // namespace shk
