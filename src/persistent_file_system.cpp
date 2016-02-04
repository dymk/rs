// Copyright 2011 Google Inc. All Rights Reserved.
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


#include "persistent_file_system.h"

#include <assert.h>
#include <errno.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>

#include <blake2.h>

namespace shk {
namespace {

class PersistentFileSystem : public FileSystem {
  template<typename T>
  static T checkForMinusOne(T result) {
    if (result == -1) {
      throw IoError(strerror(errno), errno);
    } else {
      return result;
    }
  }

  class FileStream : public FileSystem::Stream {
   public:
    FileStream(const std::string &path, const char *mode) throw(IoError) {
      _f = fopen(path.c_str(), mode);
      if (!_f) {
        throw IoError(strerror(errno), errno);
      }
    }

    virtual ~FileStream() {
      if (_f) {
        fclose(_f);
      }
    }

    size_t read(uint8_t *ptr, size_t size, size_t nitems) throw(IoError) override {
      auto result = fread(ptr, size, nitems, _f);
      if (eof()) {
        return result;
      } else if (ferror(_f) != 0) {
        throw IoError("Failed to read from stream", 0);
      } else {
        assert(result == nitems);
        return result;
      }
    }

    void write(const uint8_t *ptr, size_t size, size_t nitems) throw(IoError) override {
      fwrite(ptr, size, nitems, _f);
      if (ferror(_f) != 0) {
        throw IoError("Failed to write to stream", 0);
      }
    }

    long tell() const throw(IoError) override {
      return checkForMinusOne(ftell(_f));
    }

    bool eof() const override {
      return feof(_f) != 0;
    }

   private:
    FILE *_f = nullptr;
  };

 public:
  std::unique_ptr<Stream> open(
      const std::string &path, const char *mode) throw(IoError) override {
    return std::unique_ptr<Stream>(new FileStream(path, mode));
  }

  Stat stat(const std::string &path) override {
    return genericStat(::stat, path);
  }

  Stat lstat(const std::string &path) override {
    return genericStat(::lstat, path);
  }

  void mkdir(const std::string &path) throw(IoError) override {
    checkForMinusOne(::mkdir(path.c_str(), 0777));
  }

  void rmdir(const std::string &path) throw(IoError) override {
    checkForMinusOne(::rmdir(path.c_str()));
  }

  void unlink(const std::string &path) throw(IoError) override {
    checkForMinusOne(::unlink(path.c_str()));
  }

  void rename(
      const std::string &old_path,
      const std::string &new_path) throw(IoError) override {
    checkForMinusOne(::rename(old_path.c_str(), new_path.c_str()));
  }

  std::vector<DirEntry> readDir(
      const std::string &path) throw(IoError) override {
    std::vector<DirEntry> result;

    DIR *dp = opendir(path.c_str());
    if (!dp) {
      throw IoError(strerror(errno), errno);
    }

    dirent *dptr;
    while (NULL != (dptr = readdir(dp))) {
      result.emplace_back(direntTypeToType(dptr->d_type), dptr->d_name);
    }
    closedir(dp);

    return result;
  }

  std::string readFile(const std::string &path) throw(IoError) override {
    std::string contents;
    processFile(path, [&contents](const char *buf, size_t len) {
      contents.append(buf, len);
    });
    return contents;
  }

  Hash hashFile(const std::string &path) throw(IoError) override {
    Hash hash;
    blake2b_state state;
    blake2b_init(&state, hash.data.size());
    processFile(path, [&state](const char *buf, size_t len) {
      blake2b_update(&state, reinterpret_cast<const uint8_t *>(buf), len);
    });
    blake2b_final(&state, hash.data.data(), hash.data.size());
    return hash;
  }

  std::string mkstemp(
      std::string &&filename_template) throw(IoError) override {
    if (::mkstemp(&filename_template[0]) == -1) {
      throw IoError(
          std::string("Failed to create path for temporary file: ") +
          strerror(errno),
          errno);
    }
    return filename_template;
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
  void processFile(
      const std::string &path,
      Append &&append) throw(IoError) {
#ifdef _WIN32
    // This makes a ninja run on a set of 1500 manifest files about 4% faster
    // than using the generic fopen code below.
    err->clear();
    HANDLE f = ::CreateFile(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (f == INVALID_HANDLE_VALUE) {
      throw IoError(GetLastErrorString(), ENOENT);
    }

    for (;;) {
      DWORD len;
      char buf[64 << 10];
      if (!::ReadFile(f, buf, sizeof(buf), &len, NULL)) {
        throw IoError(GetLastErrorString(), 0);
      }
      if (len == 0) {
        break;
      }
      append(buf, len);
    }
    ::CloseHandle(f);
#else
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
      throw IoError(strerror(errno), errno);
    }

    char buf[64 << 10];
    size_t len;
    while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
      append(buf, len);
    }
    if (ferror(f)) {
      const auto err = errno;
      fclose(f);
      throw IoError(strerror(err), err);
    }
    fclose(f);
#endif
  }

  template<typename StatFunction>
  Stat genericStat(StatFunction fn, const std::string &path) {
    Stat result;
    struct stat input;
    auto ret = fn(path.c_str(), &input);
    if (ret == -1) {
      result.result = ret;
    } else {
      result.result = 0;
      result.metadata.ino = input.st_ino;
      result.metadata.dev = input.st_dev;
      result.metadata.mode = input.st_mode;
      result.metadata.size = input.st_size;
      result.timestamps.mtime = input.st_mtime;
      result.timestamps.ctime = input.st_ctime;
    }
    return result;
  }
};

}  // anonymous namespace

std::unique_ptr<FileSystem> persistentFileSystem() {
  return std::unique_ptr<FileSystem>(new PersistentFileSystem());
}

}  // namespace shk
