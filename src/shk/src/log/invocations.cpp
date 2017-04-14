#include "log/invocations.h"

namespace shk {

bool operator==(
    const Invocations::Entry &a, const Invocations::Entry &b) {
  return
      a.output_files == b.output_files &&
      a.input_files == b.input_files;
}

bool operator!=(
    const Invocations::Entry &a, const Invocations::Entry &b) {
  return !(a == b);
}

bool operator==(const Invocations &a, const Invocations &b) {
  if (a.created_directories != b.created_directories) {
    return false;
  }

  if (a.entries.size() != b.entries.size()) {
    return false;
  }

  for (const auto &a_entry : a.entries) {
    const auto b_it = b.entries.find(a_entry.first);
    if (b_it == b.entries.end()) {
      return false;
    }

    const auto files_are_same = [&](
        const std::vector<std::pair<std::string, Fingerprint>> &a_fps,
        const std::vector<std::pair<std::string, Fingerprint>> &b_fps,
        const std::vector<size_t> &a_files,
        const std::vector<size_t> &b_files) {
      if (a_files.size() != b_files.size()) {
        return false;
      }

      for (int i = 0; i < a_files.size(); i++) {
        if (a_fps[a_files[i]] != b_fps[b_files[i]]) {
          return false;
        }
      }
      return true;
    };

    if (!files_are_same(
            a.fingerprints,
            b.fingerprints,
            a_entry.second.output_files,
            b_it->second.output_files) ||
        !files_are_same(
            a.fingerprints,
            b.fingerprints,
            a_entry.second.input_files,
            b_it->second.input_files)) {
      return false;
    }
  }

  return true;
}

bool operator!=(const Invocations &a, const Invocations &b) {
  return !(a == b);
}

}  // namespace shk
