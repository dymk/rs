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
   * Input files, as specified in the manifest. These, together with the
   * implicit dependencies are files that the build step is expected to read
   * from directly.
   */
  std::vector<Path> inputs;

  /**
   * Input files, as specified in the manifest. Like inputs, but the implicit
   * dependencies are not part of the $in and $in_newline variables. Once the
   * manifest has been parsed into Steps objects like these, there isn't really
   * much of a difference between inputs and implicit dependencies.
   */
  std::vector<Path> implicit_inputs;

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

  std::string pool_name;

  /**
   * Command that should be invoked in order to perform this build step.
   *
   * The command string is empty for phony rules.
   */
  std::string command;

  /**
   * A short description of the command. Used for prettifying output while
   * running builds.
   */
  std::string description;

  bool phony() const {
    return command.empty();
  }

  /**
   * If set to true, Shuriken will check if the output files of the build step
   * have been changed after each invocation. If the output files are identical,
   * subsequent build steps that depend only on this step are not run.
   */
  bool restat = false;

  /**
   * It set to true, Shuriken will treat this build step as one that rewrites
   * manifest files. They are treated specially in the following ways:
   *
   * * They are not rebuilt if the command line changes
   * * Files are checked for dirtiness via mtime checks rather than file hashes
   * * They are not cleaned
   */
  bool generator = false;

  /**
   * For compatibility reasons with Ninja, Shuriken keeps track of the path to
   * a potential depfile generated by the build steps. Shuriken does not use
   * this file, it just removes it immediately after the build step has
   * completed.
   */
  Path depfile;

  /**
   * If rspfile is not empty, Shuriken will write rspfile_content to the path
   * specified by rspfile before running the build step and then remove the file
   * after the build step has finished running. Useful on Windows, where
   * commands have a rather short maximum length.
   */
  Path rspfile;
  std::string rspfile_content;
};

}  // namespace shk
