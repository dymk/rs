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

#include <string>
#include <unordered_set>
#include <vector>

#include <util/hash.h>

#include "fs/fingerprint.h"
#include "io_error.h"

namespace shk {

/**
 * InvocationLog is a class that is used during a build to manipulate the
 * on-disk storage of the invocation log. It does not offer means to read
 * Invocations from the invocation log; that is done in a separate build step
 * so it is done separately.
 */
class InvocationLog {
 public:
  virtual ~InvocationLog() = default;

  /**
   * Writes an entry in the invocation log that Shuriken has created a
   * directory. This will cause Shuriken to delete the directory in subsequent
   * invocations if it cleans up the last file of that directory.
   *
   * It is recommended to only provide normalized paths to this method. For
   * an explanation why, see removedDirectory.
   */
  virtual void createdDirectory(nt_string_view path) throw(IoError) = 0;

  /**
   * Writes an entry in the invocation log stating that Shuriken no longer is
   * responsible for the given directory. This should not be called unless the
   * given folder has been deleted in a cleanup process (or if it's gone).
   *
   * This method does not have any intelligence when it comes to paths; the
   * provided path must be byte equal to the path that was previously provided
   * to createdDirectory. For this reason it is recommended to only give
   * normalized paths to this method and createdDirectory.
   */
  virtual void removedDirectory(nt_string_view path) throw(IoError) = 0;

  /**
   * Take a fingerprint of the provided path. Implementations of this method
   * will probably use takeFingerprint and retakeFingerprint. The reason this
   * method is offered by the InvocationLog interface is that this object has
   * the information required to use retakeFingerprint, which can be
   * significantly more efficient than always using takeFingerprint.
   */
  virtual std::pair<Fingerprint, FileId> fingerprint(const std::string &path) = 0;

  /**
   * Writes an entry in the invocation log that says that the build step with
   * the given hash has been successfully run with information about outputs and
   * dependencies.
   *
   * The InvocationLog will fingerprint the provided input paths, reusing
   * existing fingerprints if possible.
   *
   * Because Reasons(tm) (the main use case of this function needs to have the
   * fingerprint of the outputs), the InvocationLog requires the caller to
   * fingerprint the output paths. It is recommended to use
   * InvocationLog::fingerprint for that, in order to re-use existing
   * fingerprints and avoid re-hashing of file contents whenever possible.
   *
   * Output files that are directories are treated the same as calling
   * createdDirectory. For more info, see Invocations::created_directories.
   */
  virtual void ranCommand(
      const Hash &build_step_hash,
      std::vector<std::string> &&output_files,
      std::vector<Fingerprint> &&output_fingerprints,
      std::vector<std::string> &&input_files,
      std::vector<Fingerprint> &&input_fingerprints,
      std::vector<uint32_t> &&ignored_dependencies,
      std::vector<Hash> &&additional_dependencies)
          throw(IoError) = 0;

  /**
   * Writes an entry in the invocation log that says that the build step with
   * the given hash has been cleaned and can be treated as if it was never run.
   *
   * It is the responsibility of the caller to ensure that all output files are
   * actually cleaned before calling this method.
   */
  virtual void cleanedCommand(
      const Hash &build_step_hash) throw(IoError) = 0;

  /**
   * Leak memory resources associated with this object. After calling this
   * method, the only legal thing to do with this object is to destroy it.
   *
   * This method is useful at the end of a build to save time. Deallocating then
   * is not necessary and it can take a significant amount of work.
   */
  virtual void leakMemory();

  /**
   * Helper function that calls the fingerprint method for each of the provided
   * paths and returns the results in a vector.
   */
  std::vector<Fingerprint> fingerprintFiles(
      const std::vector<std::string> &files);
};

}  // namespace shk
