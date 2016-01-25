#include "build.h"

#include "fingerprint.h"

namespace shk {

using Clock = std::function<time_t ()>;

using StepIndex = size_t;

/**
 * Map of path => index of the step that has this file as an output.
 *
 * This is useful for traversing the build graph in the direction of a build
 * step to a build step that it depends on.
 */
using OutputFileMap = std::unordered_map<Path, StepIndex>;

/**
 * "Map" of StepIndex => Hash of that step. The hash includes everything about
 * that step but not information about its dependencies.
 */
using StepHashes = std::vector<Hash>;

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
   * interrupted is set to true when the user interrupts the build. When this
   * has happened, no more build commands should be invoked.
   */
  bool interrupted = false;

  /**
   * The number of commands that are allowed to fail before the build stops. A
   * value of 0 means that too many commands have failed and the build should
   * stop.
   */
  int remaining_failures = 0;

  void markStepNodeAsDone(StepIndex step_idx) {
    const auto &dependents = step_nodes[step_idx].dependents;
    for (const auto dependent_idx : dependents) {
      auto &dependent = step_nodes[dependent_idx];
      assert(dependent.dependencies);
      dependent.dependencies--;
      if (dependent.dependencies == 0) {
        ready_steps.push_back(dependent_idx);
      }
    }
  }
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
      const Manifest &manifest,
      const StepHashes &step_hashes,
      Build &build)
      : clock(clock),
        file_system(file_system),
        command_runner(command_runner),
        build_status(build_status),
        invocation_log(invocation_log),
        manifest(manifest),
        step_hashes(step_hashes),
        build(build) {}

  const Clock &clock;
  FileSystem &file_system;
  CommandRunner &command_runner;
  BuildStatus &build_status;
  InvocationLog &invocation_log;
  const Manifest &manifest;
  const StepHashes &step_hashes;
  Build &build;
};

/**
 * Compute the "root steps," that is the steps that don't have an output that
 * is an input to some other step. This is the set of steps that are built if
 * there are no default statements in the manifest and no steps where
 * specifically requested to be built.
 */
std::vector<StepIndex> rootSteps(
    const std::vector<Step> &steps,
    const OutputFileMap &output_file_map) {
  std::vector<StepIndex> result;
  // Assume that all steps are roots until we find some step that has an input
  // that is in a given step's list of outputs. Such steps are not roots.
  std::vector<bool> roots(steps.size(), true);

  const auto process_inputs = [&](const std::vector<Path> &inputs) {
    for (const auto &input : inputs) {
      const auto it = output_file_map.find(input);
      if (it != output_file_map.end()) {
        roots[it->second] = false;
      }
    }
  };

  for (size_t i = 0; i < steps.size(); i++) {
    const auto &step = steps[i];
    process_input(step.inputs);
    process_input(step.implicit_inputs);
    process_input(step.dependencies);
  }

  for (size_t i = 0; i < steps.size(); i++) {
    if (roots[i]) {
      result.push_back(i);
    }
  }
  return result;
}

/**
 * Find the steps that should be built if no steps are specifically requested.
 *
 * Uses defaults or the root steps.
 */
std::vector<StepIndex> computeStepsToBuild(
    const Manifest &manifest,
    const OutputFileMap &output_file_map) throw(BuildError) {
  if (manifest.defaults.empty()) {
    return rootSteps(manifest.steps);
  } else {
    std::vector<StepIndex> result;
    for (const auto &default_path : manifest.defaults) {
      const auto it = output_file_map->find(default_path);
      if (it == output_file_map.end()) {
        throw BuildError(
            "default target does not exist: " + default_path.original());
      }
      result.push_back(it->second);
    }
    return result;
  }
}

/**
 * Throws BuildError if there exists an output file that more than one step
 * generates.
 */
OutputFileMap computeOutputFileMap(
    const std::vector<Step> &steps) throw(BuildError) {
  OutputFileMap result;

  for (size_t i = 0; i < steps.size(); i++) {
    const auto &step = steps[i];
    for (const auto &output : step.outputs) {
      const auto ins = result.emplace(output, i);
      if (!ins.second) {
        throw BuildError("Multiple rules generate " + output.original());
      }
    }
  }

  return result;
}

/**
 * Helper for computeBuild.
 *
 * Takes a list of ready-computed StepNodes and finds the inital list of steps
 * that can be built.
 */
std::vector<StepIndex> computeReadySteps(
    const std::vector<StepNode> &step_nodes) {
  std::vector<StepIndex> result;
  for (size_t i = 0; i < manifest.steps.size(); i++) {
    const auto &step_node = build.step_nodes[i];
    if (step_node.should_build && step_node.dependencies == 0) {
      result.push_back(i);
    }
  }
  return result;
}

std::string cycleErrorMessage(const std::vector<Path> &cycle) {
  std::string error = "dependency cycle: ";
  for (const auto &path : cycle) {
    error += path.original() + " -> ";
  }
  error += cycle.first.original();
  return error;
}

/**
 * In the process of calculating a build graph out of the build steps that are
 * declared in the manifest (the computeBuild function does this), Shuriken
 * traverses the build steps via its dependencies. This function helps this
 * process by taking a step and (via callback invocations) providing the files
 * that the given step depends on.
 *
 * This function operates differently on the initial build compared to
 * subsequent builds, and this difference is rather central to the whole design
 * of Shuriken and how Shuriken is different compared to Ninja. During the first
 * build, Shuriken does not care about the difference between inputs, implicit
 * dependencies and order-only dependencies; they are all dependencies and are
 * treated equally.
 *
 * On subsequent builds, Ninja treats order-only dependencies differently from
 * other dependencies, and also brings depfile dependencies into the mix by
 * counting them as part of the implicit dependencies.
 *
 * Shuriken does not do this. It doesn't have to, because it has accurate
 * dependency information from when the build step was last invoked. When there
 * is an up-to-date invocation log entry for the given step, Shuriken completely
 * ignores the dependencies declared in the manifest and uses only the
 * calculated dependencies. This simplifies the logic a bit and unties manifest
 * specified dependencies from dependencies retrieved from running the command.
 */
template<typename Callback>
void visitStepInputs(
    const StepHashes &step_hashes,
    const Invocations &invocations,
    const Manifest &manifest,
    StepIndex idx,
    Callback &&callback) {
  const auto invocation_it = invocations.entries.find(step_hashes[idx]);
  if (invocation_it == Invocations.entries.end()) {
    // There is an entry for this step in the invocation log. Use the real
    // inputs from the last invocation rather than the ones specified in the
    // manifest.
    const auto &output_files = invocation_it->second.output_files;
    for (const auto &output_file : output_files) {
      callback(output_file.first);
    }
  } else {
    // There is no entry for this step in the invocation log.
    const auto process_inputs = [&](const std::vector<Path> &inputs) {
      for (const auto &input : inputs) {
        callback(input);
      }
    };
    const auto &step = manifest.steps[idx];
    process_inputs(step.inputs);
    process_inputs(step.implicit_inputs);
    process_inputs(step.dependencies);
  }
}

/**
 * Recursive helper for computeBuild. Implements the DFS traversal.
 */
void visitStep(
    const Manifest &manifest,
    const StepHashes &step_hashes,
    const Invocations &invocations,
    const OutputFileMap &output_file_map,
    Build &build,
    std::vector<Path> &cycle,
    StepIndex idx) throw(BuildError) {
  auto &step_node = build.step_nodes[idx];
  if (step_node.currently_visited) {
    throw BuildError(cycleErrorMessage(cycle));
  }

  if (step_node.should_build) {
    // The step has already been processed.
    return;
  }
  step_node.should_build = true;

  step_node.currently_visited = true;
  visitStepInputs(
      manifest,
      step_hashes,
      invocations,
      manifest,
      idx,
      [&](const std::vector<Path> &inputs) {
        const auto it = output_file_map.find(input);
        if (it == output_file_map.end()) {
          // This input is not an output of some other build step.
          continue;
        }

        const auto dependency_idx = it->second;
        auto &dependency_node = build.step_nodes[dependency_idx];
        dependency_node.dependents.push_back(step_idx);
        step_node.dependencies++;

        cycle.push_back(input);
        visitStep(
            manifest,
            step_hashes,
            invocations,
            output_file_map,
            build,
            cycle,
            dependency_idx);
        cycle.pop_back();
      });
  step_node.currently_visited = false;
}

/**
 * Create a Build object suitable for use as a starting point for the build.
 */
Build computeBuild(
    const StepHashes &step_hashes,
    const Invocations &invocations,
    const OutputFileMap &output_file_map,
    const Manifest &manifest,
    size_t allowed_failures,
    std::vector<StepIndex> &&steps_to_build) {
  Build build;
  build.step_nodes.resize(manifest.steps.size());

  std::vector<Path> cycle;
  cycle.reserve(32);  // Guess at largest typical build dependency depth
  for (const auto step_idx : steps_to_build) {
    visitStep(
        manifest,
        step_hashes,
        invocations,
        output_file_map,
        build,
        cycle,
        step_idx);
  }

  build.ready_steps = computeReadySteps(build.step_nodes);
  build.allowed_failures = allowed_failures;
  return build;
}

/**
 * The fingerprinting system sometimes asks for a fingerprint of a clean target
 * to be recomputed (this usually happens when the entry is "racily clean" which
 * makes it necessary to hash the file contents to detect if the file is dirty
 * or not). This function takes an Invocations::Entry, recomputes the
 * fingerprints and creates a new Invocations::Entry with fresh fingerprints.
 */
InvocationLog::Entry recomputeInvocationEntry(
    const Clock &clock,
    FileSystem &file_system,
    const Invocations::Entry &entry) throw(IoError) {
  // TODO(peck): Implement me

  return entry;
}

InvocationLog::Entry computeInvocationEntry(
    const Clock &clock,
    FileSystem &file_system,
    const CommandRunner::Result &result) throw(IoError) {
  InvocationLog::Entry entry;

  const auto add = [&](const std::vector<std::string> &paths) {
    std::vector<std::pair<std::string, Fingerprint>> result;
    result.reserve(paths.size());
    for (const auto &path : paths) {}
      result.emplace_back(path, takeFingerprint(file_system, clock(), path));
    }
    return result;
  };

  entry.output_files = add(result.output_files);
  entry.input_files = add(result.input_files);

  return entry;
}

/**
 * Checks if a build step has already been performed and does not need to be
 * run again. This is not purely a read-only action: It uses fingerprints, and
 * if the fingerprint logic wants a fresher fingerprint in the invocation log
 * for the future, isClean provides that.
 */
bool isClean(
    FileSystem &file_system,
    InvocationLog &invocation_log,
    const Invocations &invocations,
    const Hash &step_hash) throw(IoError) {
  const auto it = invocations.find(step_hash);
  if (it == invocations.end()) {
    return false;
  }

  bool should_update = false;
  bool clean = true;
  const auto process_files = [&](
      const std::vector<std::pair<Path, Fingerprint>> &files) {
    for (const auto &file : files) {
      const auto match = fingerprintMatches(
          file_system,
          file.first.original(),
          file.second);
      if (!match.clean) {
        clean = false;
      }
      if (match.should_update) {
        should_update = true;
      }
    }
  };
  const auto &entry = it->second;
  process_files(entry.output_files);
  process_files(entry.input_files);

  if (clean && should_update) {
    // There is no need to update the invocation log when dirty; it will be
    // updated anyway as part of the build.
    invocation_log.ranCommand(
        step_hash, recomputeInvocationEntry(file_system, entry));
  }

  return clean;
}

StepHashes computeStepHashes(const std::vector<Step> &steps) {
  StepHashes hashes;
  hashes.reserve(steps.size());

  for (const auto &step : steps) {
    hashes.push_back(step.hash());
  }

  return hashes;
}

/**
 * Before the actual build is performed, this function goes through the build
 * graph and removes steps that don't need to be built because they are already
 * built.
 */
void discardCleanSteps(
    FileSystem &file_system,
    InvocationLog &invocation_log,
    const Invocations &invocations,
    const Manifest &manifest,
    const StepHashes &step_hashes,
    Build &build) throw(IoError) {
  // This function goes through and consumes build.ready_steps. While doing that
  // it adds an element to new_ready_steps for each dirty step that it
  // encounters. When this function's search is over, it replaces
  // build.ready_steps with this list.
  std::vector<StepIndex> new_ready_steps;

  // Memo map of step index => visited. This is to make sure that each step
  // is processed at most once.
  std::vector<bool> visited(manifest.steps.size(), false);

  // This is a BFS search loop. build.ready_steps is the work stack.
  while (!build.ready_steps.empty()) {
    const auto step_idx = build.ready_steps.back();
    ready_steps.pop_back();
    const auto &step = manifest.steps[step_idx];

    if (visited[step_idx]) {
      continue;
    }
    visited[step_idx] = true;

    const auto &step_hash = step_hashes[step_idx];
    if (isClean(file_system, invocation_log, invocations, step_hash)) {
      build.markStepNodeAsDone(step_idx);
    } else {
      new_ready_steps.push_back(step_idx);
    }
  }

  build.ready_steps.swap(new_ready_steps);
}

void deleteBuildProduct(
    FileSystem &file_system,
    Path path) throw(IoError) {
  // TODO(peck): Implement me
}

void mkdirsForPath(
    FileSystem &file_system,
    InvocationLog &invocation_log,
    Path path) {
  // TODO(peck): Implement me
}

void commandDone(
    BuildCommandParameters &params,
    StepIndex step_idx,
    CommandRunner::Result &&result) throw(IoError) {
  const auto &step = params.manifest.steps[step_idx];

  // TODO(peck): Validate that the command did not read a file that is an output
  //   of a target that it does not depend on directly or indirectly.

  deleteBuildProduct(params.file_system, step.depfile);
  deleteBuildProduct(params.file_system, step.rspfile);

  switch (result.exit_status) {
  case ExitStatus::SUCCESS:
    // TODO(peck): Do something about result.linting_errors

    params.invocation_log.ranCommand(
        params.step_hashes[step_idx],
        computeInvocationEntry(params.clock, params.file_system, result));

    if (false /*TODO(peck):NYI*/ && step.restat && outputsDidNotChange()) {
      // TODO(peck): Mark this step as clean
    } else {
      build.markStepNodeAsDone(step_idx);
    }
    break;

  case ExitStatus::FAILURE:
    assert(params.build.remaining_failures);
    params.build.remaining_failures--;
    break;

  case ExitStatus::INTERRUPTED:
    params.build.interrupted = true;
    break;
  }

  // Feed the command runner with more commands now that this one is finished.
  enqueueBuildCommands(params);
}

void deleteOldOutputs(
    FileSystem &file_system,
    const Invocations &invocations,
    const Hash &step_hash) throw(IoError) {
  const auto it = invocations.entries.find(step_hash);
  if (it == invocations.entries.end()) {
    return;
  }

  const auto &entry = it->second;
  for (const auto &output : entry.output_files) {
    deleteBuildProduct(output.first);
  }
}

bool enqueueBuildCommand(BuildCommandParameters &params) throw(IoError) {
  if (params.build.ready_steps.empty() ||
      params.build.interrupted ||
      !params.command_runner.canRunMore() ||
      params.build.remaining_failures == 0) {
    return false;
  }

  const auto step_idx = params.build.ready_steps.back();
  const auto &step = params.manifest.steps[step_idx];
  params.build.ready_steps.pop_back();

  const auto &step_hash = params.step_hashes[step_idx];
  deleteOldOutputs(params.file_system, params.invocations, step_hash);

  mkdirsForPath(params.file_system, params.invocation_log, step.rspfile);
  params.file_system.writeFile(step.rspfile, step.rspfile_content);

  for (const auto &output : step.outputs) {
    mkdirsForPath(params.file_system, params.invocation_log, output);
  }

  // TODO(peck): What about pools?

  params.command_runner.invoke(
      step.command,
      step.pool_name == "console" ? UseConsole::YES : UseConsole::NO,
      [&params, step_idx](CommandRunner::Result &&result) {
        commandDone(params, step_idx, std::move(result));
      });

  return true;
}


bool enqueueBuildCommands(BuildCommandParameters &params) throw(IoError) {
  while (enqueueBuildCommand(params)) {}
}

void deleteStaleOutputs(
    FileSystem &file_system,
    InvocationLog &invocation_log,
    const StepHashes &step_hashes,
    const Invocations &invocations) throw(IoError) {
  std::unordered_set<Hash> step_hashes_set;
  std::copy(
      step_hashes.begin(),
      step_hashes.end(),
      std::inserter(step_hashes_set, step_hashes_set.begin()));

  for (const auto &entry : invocations.entries) {
    if (step_hashes_set.count(entry.first) == 0) {
      for (const auto &output_file : entry.second.output_files) {
        deleteBuildProduct(file_system, output_file.first);
      }
      invocation_log.cleanedCommand(entry.first);
    }
  }
}

void build(
    const Clock &clock,
    FileSystem &file_system,
    CommandRunner &command_runner,
    BuildStatus &build_status,
    InvocationLog &invocation_log,
    size_t allowed_failures,
    const Manifest &manifest,
    const Invocations &invocations) throw(IoError, BuildError) {
  // TODO(peck): Use build_status

  const auto step_hashes = computeStepHashes(manifest.steps);

  deleteStaleOutputs(file_system, invocation_log, step_hashes, invocations);

  const auto output_file_map = computeOutputFileMap(manifest.steps);

  auto steps_to_build = computeStepsToBuild(manifest, output_file_map);

  auto build = computeBuild(
      step_hashes,
      invocations,
      output_file_map,
      manifest,
      allowed_failures,
      std::move(steps_to_build));

  discardCleanSteps(
      file_system,
      invocation_log,
      invocations,
      manifest,
      step_hashes,
      build);

  BuildCommandParameters params(
      clock,
      file_system,
      command_runner,
      build_status,
      invocation_log,
      manfiest,
      step_hashes,
      build);
  enqueueBuildCommands(params);

  while (!command_runner.empty()) {
    if (command_runner.runCommands()) {
      build.interrupted = true;
    }
  }

  // TODO(peck): Report result
}

}
