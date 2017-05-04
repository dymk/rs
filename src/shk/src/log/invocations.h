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

#include <unordered_map>
#include <vector>

#include "fs/file_id.h"
#include "fs/fingerprint.h"
#include "hash.h"
#include "manifest/wrapper_view.h"

namespace shk {

using IndicesView = WrapperView<const uint32_t *, const uint32_t &>;
using HashesView = WrapperView<const Hash *, const Hash &>;

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
   * List of path + Fingerprint pairs. This is just for deduplication in storage
   * and to be in a format that is easily processable later. It only has meaning
   * when used together with entries, which contains indices into this array.
   *
   * Note that there may be (possibly many) entries in this vector that have no
   * corresponding uses in entries. Because of this it is usually not a good
   * idea to go though and process all the entries in this vector.
   */
  std::vector<std::pair<nt_string_view, const Fingerprint &>> fingerprints;

  /**
   * An Invocations::Entry has information about one build step that has been
   * successfully run at some point. It may or may not still be clean.
   */
  struct Entry {
    /**
     * Contains indices into the fingerprints vector.
     */
    IndicesView output_files;
    /**
     * Contains indices into the fingerprints vector.
     */
    IndicesView input_files;

    /**
     * Sorted list of step indices that are in the Step's dependencies list that
     * were not actually used during the step invocation.
     *
     * Must not contain duplicates.
     *
     * Warning: The invocation log parsing does not validate that the indices
     * in this list are all smaller than the number of dependencies for a given
     * step. Code that uses this must tolerate out of bounds indices.
     */
    IndicesView ignored_dependencies;

    /**
     * List of Step hashes for steps that are not directly in the step's
     * dependencies list but that were used anyway when the step was invoked.
     * This list should only include references to Step hashes that this build
     * step indirectly depends on in the Manifest dependency graph.
     *
     * This list allows Shuriken to add items to the ignored_dependencies list
     * that would not otherwise be possible to add there. For example, a build
     * step for a .cpp file could have an order-only dependency to a whole
     * static library target, but the only thing that is used is a generated
     * header that is a transitive dependency of that static library target.
     *
     * Without additional_dependencies, Shuriken would not be able to ignore the
     * static library target, because it does indirectly depend on it. With
     * additional_dependencies, Shuriken can mark only the build step that
     * generates the header as an additional dependency and then ignore the big
     * static library step.
     *
     * Besides, it is not super easy to compute ignored_dependencies if it can't
     * include depenencies that are indirectly used.
     *
     * Must be sorted and must not contain duplicates.
     */
    HashesView additional_dependencies;
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
   * The Invocation::fingerprints vector can contain entries that are not
   * actually referred to by any entry in Invocations::entries. This method
   * counts how many of the fingerprints are actually used.
   */
  int countUsedFingerprints() const;

  /**
   * Given a list of entries, find the indices of all the fingerprints that are
   * referred to by them.
   */
  std::vector<uint32_t> fingerprintsFor(
      const std::vector<const Entry *> &entries) const;

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
  std::unordered_map<FileId, nt_string_view> created_directories;

  /**
   * Opaque owning pointer that points to a buffer that contains the data that
   * the string_views etc in this object point to.
   */
  std::shared_ptr<void> buffer;
};

bool operator==(const Invocations::Entry &a, const Invocations::Entry &b);
bool operator!=(const Invocations::Entry &a, const Invocations::Entry &b);
bool operator==(const Invocations &a, const Invocations &b);
bool operator!=(const Invocations &a, const Invocations &b);

}  // namespace shk
