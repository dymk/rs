#pragma once

#include <string>
#include <vector>

#include "hash.h"
#include "path.h"

namespace shk {

/**
 * A Step is a dumb data object that represents one build statment in the
 * build manifest.
 *
 * Parsing the build manifest and evaluating the rules results in a list of
 * Step objects. When the Steps object have been created, the manifest and the
 * variable environments etc can be discarded. It is not possible to recreate
 * the manifest from the list of steps; Step objects contain already evaluated
 * commands.
 */
struct Step {
  /**
   * Command that should be invoked in order to perform this build step.
   *
   * The command string is empty for phony rules.
   */
  std::string command;

  bool phony() const {
    return command.empty();
  }

  bool restat = false;

  /**
   * Input files, as specified in the manifest. These are files that the build
   * step is going to read from directly. In the Ninja manifest, these are the
   * "explicit" and the "implicit" dependencies.
   *
   * Because the only difference between Ninja "explicit" and "implicit"
   * dependencies is that implicit dependencies don't show up in the $in
   * variable there is no need to distinguish between them in Step objects. The
   * command has already been evaluated so there is no point in differentiating
   * them anymore.
   */
  std::vector<Path> inputs;

  /**
   * Dependencies are paths to targets that generate output files that this
   * target may depend on. These are different from inputs because they
   * themselves are often not read by the build step. A common use case for this
   * is targets that generate headers that other targets may depend on.
   *
   * These correspond to "order only" dependencies in the Ninja manifest.
   *
   * dependencies and inputs are kept separate because persistent caching cares
   * about the difference.
   */
  std::vector<Path> dependencies;

  /**
   * Output files, as specified in the manifest. These are used as names for
   * targets, to deduce the dependencies between different build steps and to
   * make sure that the directory where the outputs should live exists before
   * the command is invoked.
   */
  std::vector<Path> outputs;
};

}  // namespace shk
