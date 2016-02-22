#pragma once

#include <cstdio>
#include <stdexcept>
#include <string>

#include <util/raii_helper.h>

#include "io_error.h"

namespace shk {

class FileLock {
 public:
  FileLock(const std::string &path) throw(IoError);
  ~FileLock();

 private:
  const std::string _path;
  using FileHandle = util::RAIIHelper<FILE, int, fclose>;
  const FileHandle _f;
};

}  // namespace shk
