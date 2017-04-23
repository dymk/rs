#include "log/invocations.h"

#include <atomic>
#include <numeric>
#include <thread>

#include "util.h"

namespace shk {

int Invocations::countUsedFingerprints() const {
  std::vector<const Entry *> entry_vec;
  for (const auto &entry : entries) {
    entry_vec.push_back(&entry.second);
  }

  const int num_threads = guessParallelism();
  // used_fingerprints has one entry per worker thread. Each of those is a "map"
  // from fingerprint index (in the fingerprints vector) to a boolean indicating
  // whether it is used by an actual entry.
  std::vector<std::vector<bool>> used_fingerprints(num_threads);
  std::atomic<int> next_entry(0);
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([this, i, &entry_vec, &next_entry, &used_fingerprints] {
      used_fingerprints[i].resize(fingerprints.size());
      auto &fps = used_fingerprints[i];

      for (;;) {
        int entry_idx = next_entry++;
        if (entry_idx >= entry_vec.size()) {
          break;
        }
        const auto *entry = entry_vec[entry_idx];
        for (const auto idx : entry->output_files) {
          fps[idx] = true;
        }
        for (const auto idx : entry->input_files) {
          fps[idx] = true;
        }
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }

  int count = 0;
  for (int i = 0; i < fingerprints.size(); i++) {
    for (int j = 0; j < used_fingerprints.size(); j++) {
      if (used_fingerprints[j][i]) {
        count++;
        break;
      }
    }
  }
  return count;
}

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
        const std::vector<uint32_t> &a_files,
        const std::vector<uint32_t> &b_files) {
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
