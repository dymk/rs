#pragma once

#include <unordered_set>
#include <vector>

#include "build_error.h"
#include "build_status.h"
#include "clock.h"
#include "command_runner.h"
#include "file_system.h"
#include "invocation_log.h"
#include "invocations.h"
#include "manifest.h"
#include "optional.h"
#include "step.h"

namespace shk {

using StepIndex = size_t;

/**
 * Get the Path to a given output file for a step in the manifest. This handles
 * the ^ command line interface syntax.
 */
Path interpretPath(Paths &paths, const Manifest &manifest, std::string &&path)
    throw(BuildError);

/**
 * Takes command line arguments and calls interpretPath on each of them.
 */
std::vector<Path> interpretPaths(
    Paths &paths,
    const Manifest &manifest,
    int argc,
    char *argv[]) throw(BuildError);

/**
 * Like detail::computeStepsToBuild, but with a more convenient interface for
 * use by tools.
 */
std::vector<StepIndex> computeStepsToBuild(
    Paths &paths,
    const Manifest &manifest,
    int argc,
    char *argv[0]) throw(BuildError);

namespace detail {

/**
 * Map of path => index of the step that has this file as an output.
 *
 * This is useful for traversing the build graph in the direction of a build
 * step to a build step that it depends on.
 *
 * This map is configured to treat paths that are the same according to
 * Path::isSame as equal. This is important because otherwise the lookup
 * will miss paths that point to the same thing but with different original
 * path strings.
 */
using OutputFileMap = std::unordered_map<
    Path, StepIndex, Path::IsSameHash, Path::IsSame>;

/**
 * "Map" of StepIndex => Hash of that step. The hash includes everything about
 * that step but not information about its dependencies.
 */
using StepHashes = std::vector<Hash>;

/**
 * "Map" of StepIndex => bool that indicates if the Step has been built before
 * and at the time the build was started, its direct inputs and outputs were
 * unchanged since the last time its command was run.
 *
 * That a step is "clean" in this sense does not imply that the step will not
 * be re-run during the build, because it might depend on a file that will
 * change during the build.
 *
 * This variable is used during the initial discardCleanSteps phase where
 * clean steps are marked as already done, and also by restat steps when their
 * outputs don't change.
 */
using CleanSteps = std::vector<bool>;

/**
 * "Map" of index in Invocations::fingerprints => MatchesResult with information
 * about if each fingerprint matches or not. This information is computed lazily
 * and memoized in this data structure, to avoid having to compute if a
 * fingerprint matches for every build step that has that fingerprint, since on
 * average a fingerprint is typically used by quite a few steps.
 *
 * A reason for why this is constructed lazily rather than precomputed all up
 * front is that it is common that the manifest contains fingerprints that are
 * no longer used. They will be removed eventually in a subsequent invocation
 * log compaction, but until they are, it is problematic to fingerprint those,
 * especially because it is likely that they are "should_update"/racily clean,
 * which requires hashing their file contents to calculate the MatchesResult.
 */
using FingerprintMatchesMemo = std::vector<Optional<MatchesResult>>;

/**
 * During the build, the Build object has one StepNode for each Step in the
 * Manifest. The StepNode contains information about dependencies between
 * steps in a format that is efficient when building.
 */
struct StepNode {
  /**
   * List of steps that depend on this step.
   *
   * When a build step is completed, the builder visits the StepNode for each
   * dependent step and decrements the dependencies counter. If the counter
   * reaches zero, that StepNode is ready to be built and can be added to the
   * Build::ready_steps list.
   */
  std::vector<StepIndex> dependents;

  /**
   * The number of not yet built steps that this step depends on.
   */
  int dependencies = 0;

  /**
   * true if the user has asked to build this step or any step that depends on
   * this step. If false, the step should not be run even if it is dirty.
   *
   * This piece of information is used only when computing the initial list of
   * steps that are ready to be built; after that it is not needed because
   * dependents and dependencies never point to or from a step that should not
   * be built.
   */
  bool should_build = false;

  /**
   * Used when computing the Build graph in order to detect cycles.
   */
  bool currently_visited = false;
};

/**
 * Build is the data structure that keeps track of the build steps that are
 * left to do in the build and helps to efficiently provide information about
 * what to do next when a build step has completed.
 */
struct Build {
  /**
   * step_nodes.size() == manifest.steps.size()
   *
   * step_nodes contains step dependency information in an easily accessible
   * format.
   */
  std::vector<StepNode> step_nodes;

  /**
   * List of steps that are ready to be run.
   */
  std::vector<StepIndex> ready_steps;

  /**
   * The number of commands that are allowed to fail before the build stops. A
   * value of 0 means that too many commands have failed and the build should
   * stop.
   */
  int remaining_failures = 0;
};

/**
 * There are a bunch of functions in this file that take more or less the same
 * parameters, and quite many at that. The point of this struct is to avoid
 * having to pass all of them explicitly, which just gets overly verbose, hard
 * to read and painful to change.
 */
struct BuildCommandParameters {
  BuildCommandParameters(
      const Clock &clock,
      FileSystem &file_system,
      CommandRunner &command_runner,
      BuildStatus &build_status,
      InvocationLog &invocation_log,
      const Invocations &invocations,
      const Manifest &manifest,
      const StepHashes &step_hashes,
      Build &build)
      : clock(clock),
        file_system(file_system),
        command_runner(command_runner),
        build_status(build_status),
        invocations(invocations),
        invocation_log(invocation_log),
        manifest(manifest),
        step_hashes(step_hashes),
        build(build) {}

  const Clock &clock;
  FileSystem &file_system;
  CommandRunner &command_runner;
  BuildStatus &build_status;
  const Invocations &invocations;
  InvocationLog &invocation_log;
  const Manifest &manifest;
  const StepHashes &step_hashes;
  Build &build;
};

/**
 * Throws BuildError if there exists an output file that more than one step
 * generates.
 */
OutputFileMap computeOutputFileMap(
    const std::vector<Step> &steps) throw(BuildError);

/**
 * Compute the "root steps," that is the steps that don't have an output that
 * is an input to some other step. This is the set of steps that are built if
 * there are no default statements in the manifest and no steps where
 * specifically requested to be built.
 */
std::vector<StepIndex> rootSteps(
    const std::vector<Step> &steps,
    const OutputFileMap &output_file_map);

/**
 * Find the steps that should be built. If there are no specified paths, this
 * function will use defaults specified in the manifest, or find the root nodes
 * to build.
 *
 * The returned array may contain duplicate values.
 *
 * Uses defaults or the root steps.
 */
std::vector<StepIndex> computeStepsToBuild(
    const Manifest &manifest,
    const OutputFileMap &output_file_map,
    const std::vector<Path> &specified_outputs) throw(BuildError);

/**
 * Generate a string that describes a cycle, for example "a -> b -> a".
 * cycle must be a non-empty vector.
 */
std::string cycleErrorMessage(const std::vector<Path> &cycle);

StepHashes computeStepHashes(const std::vector<Step> &steps);

/**
 * Create a Build object suitable for use as a starting point for the build.
 */
Build computeBuild(
    const StepHashes &step_hashes,
    const Invocations &invocations,
    const OutputFileMap &output_file_map,
    const Manifest &manifest,
    size_t failures_allowed,
    std::vector<StepIndex> &&steps_to_build) throw(BuildError);

/**
 * Checks if a build step has already been performed and does not need to be
 * run again. This is not purely a read-only action: It uses fingerprints, and
 * if the fingerprint logic wants a fresher fingerprint in the invocation log
 * for the future, isClean provides that.
 */
bool isClean(
    const Clock &clock,
    FileSystem &file_system,
    InvocationLog &invocation_log,
    FingerprintMatchesMemo &fingerprint_matches_memo,
    const Invocations &invocations,
    const Hash &step_hash) throw(IoError);

/**
 * Create a CleanSteps object. For more info, see the documentation for that
 * typedef.
 */
CleanSteps computeCleanSteps(
    const Clock &clock,
    FileSystem &file_system,
    InvocationLog &invocation_log,
    const Invocations &invocations,
    const StepHashes &step_hashes,
    const Build &build) throw(IoError);

/**
 * Before the actual build is performed, this function goes through the build
 * graph and removes steps that don't need to be built because they are already
 * built.
 *
 * Returns the number of discarded steps.
 */
int discardCleanSteps(
    const CleanSteps &clean_steps,
    Build &build);

/**
 * For build steps that have been configured to restat outputs after completion,
 * this is the function that performs the restat check.
 *
 * This function is similar to isClean but it's not quite the same. It does not
 * look at inputs, it only checks output files. Also, it ignores
 * MatchesResult::should_update because it has already been handled by isClean
 * earlier.
 */
bool outputsWereChanged(
    FileSystem &file_system,
    const Invocations &invocations,
    const Hash &step_hash) throw(IoError);

/**
 * Prior to invoking the command for a step, delete the files that it previously
 * created.
 */
void deleteOldOutputs(
    FileSystem &file_system,
    const Invocations &invocations,
    InvocationLog &invocation_log,
    const Hash &step_hash) throw(IoError);

void deleteStaleOutputs(
    FileSystem &file_system,
    InvocationLog &invocation_log,
    const StepHashes &step_hashes,
    const Invocations &invocations) throw(IoError);

int countStepsToBuild(const Build &build);

}  // namespace detail

enum class BuildResult {
  NO_WORK_TO_DO,
  SUCCESS,
  INTERRUPTED,
  FAILURE,
};

using MakeBuildStatus = std::function<
    std::unique_ptr<BuildStatus> (int total_steps)>;

/**
 * Main entry point for performing a build.
 */
BuildResult build(
    const Clock &clock,
    FileSystem &file_system,
    CommandRunner &command_runner,
    const MakeBuildStatus &make_build_status,
    InvocationLog &invocation_log,
    size_t failures_allowed,
    const std::vector<Path> &specified_outputs,
    const Manifest &manifest,
    const Invocations &invocations) throw(IoError, BuildError);

}  // namespace shk
