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

#include "path.h"

namespace shk {

namespace detail {

bool canonicalizePath(
    std::string *path,
    shk::SlashBits *slash_bits,
    std::string *err) {
  size_t len = path->size();
  char* str = 0;
  if (len > 0) {
    str = &(*path)[0];
  }
  if (!canonicalizePath(str, &len, slash_bits, err)) {
    return false;
  }
  path->resize(len);
  return true;
}

#ifdef _WIN32

namespace {

shk::SlashBits shiftOverBit(int offset, shk::SlashBits bits) {
  // e.g. for |offset| == 2:
  // | ... 9 8 7 6 5 4 3 2 1 0 |
  // \_________________/   \_/
  //        above         below
  // So we drop the bit at offset and move above "down" into its place.
  shk::SlashBits above = bits & ~((1 << (offset + 1)) - 1);
  shk::SlashBits below = bits & ((1 << offset) - 1);
  return (above >> 1) | below;
}

}  // anonymous namespace

#endif

bool canonicalizePath(
    char *path,
    size_t *len,
    shk::SlashBits *slash_bits,
    std::string *err) {
  // WARNING: this function is performance-critical; please benchmark
  // any changes you make to it.
  if (*len == 0) {
    *err = "empty path";
    return false;
  }

  const int kMaxPathComponents = sizeof(shk::SlashBits) * 8 - 2;
  char* components[kMaxPathComponents];
  int component_count = 0;

  char* start = path;
  char* dst = start;
  const char* src = start;
  const char* end = start + *len;

#ifdef _WIN32
  shk::SlashBits bits = 0;
  shk::SlashBits bits_mask = 1;
  int bits_offset = 0;
  // Convert \ to /, setting a bit in |bits| for each \ encountered.
  for (char* c = path; c < end; ++c) {
    switch (*c) {
      case '\\':
        bits |= bits_mask;
        *c = '/';
        // Intentional fallthrough.
      case '/':
        bits_mask <<= 1;
        bits_offset++;
    }
  }
  if (bits_offset > kMaxPathComponents) {
    *err = "too many path components";
    return false;
  }
  bits_offset = 0;
#endif

  if (*src == '/') {
#ifdef _WIN32
    bits_offset++;
    // network path starts with //
    if (*len > 1 && *(src + 1) == '/') {
      src += 2;
      dst += 2;
      bits_offset++;
    } else {
      ++src;
      ++dst;
    }
#else
    ++src;
    ++dst;
#endif
  }

  while (src < end) {
    if (*src == '.') {
      if (src + 1 == end || src[1] == '/') {
        // '.' component; eliminate.
        src += 2;
#ifdef _WIN32
        bits = shiftOverBit(bits_offset, bits);
#endif
        continue;
      } else if (src[1] == '.' && (src + 2 == end || src[2] == '/')) {
        // '..' component.  Back up if possible.
        if (component_count > 0) {
          dst = components[component_count - 1];
          src += 3;
          --component_count;
#ifdef _WIN32
          bits = shiftOverBit(bits_offset, bits);
          bits_offset--;
          bits = shiftOverBit(bits_offset, bits);
#endif
        } else {
          *dst++ = *src++;
          *dst++ = *src++;
          *dst++ = *src++;
        }
        continue;
      }
    }

    if (*src == '/') {
      src++;
#ifdef _WIN32
      bits = shiftOverBit(bits_offset, bits);
#endif
      continue;
    }
 
    if (component_count == kMaxPathComponents) {
      throw std::runtime_error(std::string("path has too many components: ") + path);
    }
    components[component_count] = dst;
    ++component_count;

    while (*src != '/' && src != end) {
      *dst++ = *src++;
    }
#ifdef _WIN32
    bits_offset++;
#endif
    *dst++ = *src++;  // Copy '/' or final \0 character as well.
  }

  if (dst == start) {
    *err = "path canonicalizes to the empty path";
    return false;
  }

  *len = dst - start - 1;
#ifdef _WIN32
  *slash_bits = bits;
#else
  *slash_bits = 0;
#endif
  return true;
}

}  // namespace detail

std::string Path::pathDecanonicalized() const {
  auto result = _canonicalized_path->path;
#ifdef _WIN32
  unsigned uint64_t mask = 1;
  for (char *c = &result[0]; (c = strchr(c, '/')) != NULL;) {
    if (_slash_bits & mask)
      *c = '\\';
    c++;
    mask <<= 1;
  }
#endif
  return result;
}

Path Paths::get(const std::string &path) {
  SlashBits slash_bits;
  _canonicalized_paths.emplace(path);
  // TODO(peck): Implement me
  const detail::CanonicalizedPath *canonicalized_path =
      &*_canonicalized_paths.find(detail::CanonicalizedPath(path));
  return Path(canonicalized_path, slash_bits);
}

}  // namespace shk
