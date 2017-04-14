#pragma once

#include <unordered_map>
#include <vector>

#include "fs/file_id.h"
#include "fs/fingerprint.h"
#include "fs/path.h"
#include "hash.h"

namespace shk {

/**
 * An Invocations object contains information about what Shuriken has done in
 * previous builds. It is used to be able to know what build steps of the build
 * that don't need to be done, but also what build steps that have been done
 * before that might have to be cleaned up.
 *
 * Invocations is a passive dumb data object.
 */
struct Invocations {
  /**
   * List of Path + Fingerprint pairs. This is just for deduplication in storage
   * and to be in a format that is easily processable later. It only has meaning
   * when used together with entries, which contains indices into this array.
   */
  std::vector<std::pair<Path, Fingerprint>> fingerprints;

  /**
   * Contains indices into the fingerprints vector.
   */
  struct Entry {
    std::vector<size_t> output_files;
    std::vector<size_t> input_files;
  };

  /**
   * Contains information about build steps that have been performed. Used to
   * check if the corresponding build step is dirty and has to be re-invoked,
   * but also to be able to clean up outputs when necessary.
   *
   * The key in this map is a hash of the BuildStep that was the basis of the
   * invocation.
   */
  std::unordered_map<Hash, Entry> entries;

  /**
   * The directories that Shuriken has created to make room for outputs of build
   * steps. They are kept track of to be able to remove then when cleaning up.
   *
   * In addition to directories that are created by Shuriken explicitly to make
   * place for build targets, this also contains directories that have been
   * created by build steps. This might seem surprising at first. The rationale
   * is this:
   *
   * Shuriken treats directories similarly to how git does it: Shuriken is all
   * about files. Directories are just there to contain the files, and are not
   * part of the build product. They exist or don't exist rather arbitrarily,
   * but if they have files, they must exist. If a build depends on a directory
   * existing, a workaround is to create a dummy empty file in it.
   *
   * The reason for this design is that unlike files, which can be cleaned up
   * without deleting other build outputs, directories can't just be removed
   * without potentially removing other things as well. This assymetry makes it
   * pretty hard to allow directories to be treated as build step outputs.
   *
   * The key is a FileId, which is used for efficient lookup when cleaning. The
   * value is a Path, useful to know the actual path of the directory.
   *
   * The fact that the key is a FileId means that the directory must actually
   * exist to be able to be here. This is okay because if the directory has been
   * removed since it was last created by the build, it is ok (and actually
   * desired) for Shuriken to not track it anymore.
   */
  std::unordered_map<FileId, std::string> created_directories;
};

inline bool operator==(const Invocations::Entry &a, const Invocations::Entry &b) {
  return
      a.output_files == b.output_files &&
      a.input_files == b.input_files;
}

inline bool operator==(const Invocations &a, const Invocations &b) {
  return
      a.entries == b.entries &&
      a.created_directories == b.created_directories;
}

}  // namespace shk
