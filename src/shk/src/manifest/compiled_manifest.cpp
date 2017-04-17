#include "manifest/compiled_manifest.h"

namespace shk {
namespace detail {

PathToStepMap computeOutputPathMap(
    const std::vector<RawStep> &steps) throw(BuildError) {
  PathToStepMap result;

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

std::vector<StepIndex> rootSteps(
    const std::vector<Step> &steps) throw(BuildError) {
  std::vector<StepIndex> result;
  // Assume that all steps are roots until we find some step that has an input
  // that is in a given step's list of outputs. Such steps are not roots.
  std::vector<bool> roots(steps.size(), true);

  for (size_t i = 0; i < steps.size(); i++) {
    for (const auto dependency_idx : steps[i].dependencies()) {
      roots[dependency_idx] = false;
    }
  }

  for (size_t i = 0; i < steps.size(); i++) {
    if (roots[i]) {
      result.push_back(i);
    }
  }

  return result;
}

std::string cycleErrorMessage(const std::vector<Path> &cycle) {
  if (cycle.empty()) {
    // There can't be a cycle without any nodes. Then it's not a cycle...
    return "[internal error]";
  }

  std::string error;
  for (const auto &path : cycle) {
    error += path.original() + " -> ";
  }
  error += cycle.front().original();
  return error;
}

}  // namespace detail

namespace {

detail::PathToStepMap computeInputPathMap(
    const std::vector<RawStep> &steps) throw(BuildError) {
  detail::PathToStepMap result;

  auto process = [&](int idx, const std::vector<Path> &paths) {
    for (const auto path : paths) {
      result.emplace(path, idx);
    }
  };

  for (size_t i = 0; i < steps.size(); i++) {
    const auto &step = steps[i];

    process(i, step.inputs);
    process(i, step.implicit_inputs);
    process(i, step.dependencies);
  }

  return result;
}

PathToStepList computePathList(
    const detail::PathToStepMap &path_map) {
  PathToStepList result;

  for (const auto &path_pair : path_map) {
    auto path = path_pair.first.original();
    try {
      canonicalizePath(&path);
    } catch (const PathError &) {
      continue;
    }
    result.emplace_back(std::move(path), path_pair.second);
  }

  std::sort(result.begin(), result.end());

  return result;
}

Step convertRawStep(
    const detail::PathToStepMap &output_path_map,
    std::vector<std::unique_ptr<flatbuffers::FlatBufferBuilder>> &step_buffers,
    RawStep &&raw) {
  step_buffers.emplace_back(new flatbuffers::FlatBufferBuilder(1024));
  auto &builder = *step_buffers.back();

  std::vector<StepIndex> dependencies;
  const auto process_inputs = [&](
      const std::vector<Path> &paths) {
    for (const auto &path : paths) {
      const auto it = output_path_map.find(path);
      if (it != output_path_map.end()) {
        dependencies.push_back(it->second);
      }
    }
  };
  process_inputs(raw.inputs);
  process_inputs(raw.implicit_inputs);
  process_inputs(raw.dependencies);

  auto deps_vector = builder.CreateVector(
      dependencies.data(), dependencies.size());

  std::unordered_set<std::string> output_dirs_set;
  for (const auto &output : raw.outputs) {
    output_dirs_set.insert(dirname(output.original()));
  }

  std::vector<flatbuffers::Offset<flatbuffers::String>> output_dirs;
  output_dirs.reserve(output_dirs_set.size());
  for (auto &output : output_dirs_set) {
    if (output == ".") {
      continue;
    }
    output_dirs.push_back(builder.CreateString(output));
  }
  auto output_dirs_vector = builder.CreateVector(
      output_dirs.data(), output_dirs.size());

  auto pool_name_string = builder.CreateString(raw.pool_name);
  auto command_string = builder.CreateString(raw.command);
  auto description_string = builder.CreateString(raw.description);
  auto depfile_string = builder.CreateString(raw.depfile);
  auto rspfile_string = builder.CreateString(raw.rspfile);
  auto rspfile_content_string = builder.CreateString(raw.rspfile_content);

  ShkManifest::StepBuilder step_builder(builder);
  step_builder.add_hash(
      reinterpret_cast<const ShkManifest::Hash *>(raw.hash().data.data()));
  step_builder.add_dependencies(deps_vector);
  step_builder.add_output_dirs(output_dirs_vector);
  step_builder.add_pool_name(pool_name_string);
  step_builder.add_command(command_string);
  step_builder.add_description(description_string);
  step_builder.add_generator(raw.generator);
  step_builder.add_depfile(depfile_string);
  step_builder.add_rspfile(rspfile_string);
  step_builder.add_rspfile_content(rspfile_content_string);
  builder.Finish(step_builder.Finish());

  Step step(*flatbuffers::GetRoot<ShkManifest::Step>(
      builder.GetBufferPointer()));

  return step;
}

std::vector<Step> convertStepVector(
    const detail::PathToStepMap &output_path_map,
    std::vector<std::unique_ptr<flatbuffers::FlatBufferBuilder>> &step_buffers,
    std::vector<RawStep> &&steps) {
  std::vector<Step> ans;
  ans.reserve(steps.size());

  for (auto &step : steps) {
    ans.push_back(convertRawStep(
        output_path_map, step_buffers, std::move(step)));
  }

  return ans;
}

std::vector<StepIndex> computeStepsToBuildFromPaths(
    const std::vector<Path> &paths,
    const detail::PathToStepMap &output_path_map) throw(BuildError) {
  std::vector<StepIndex> result;
  for (const auto &default_path : paths) {
    const auto it = output_path_map.find(default_path);
    if (it == output_path_map.end()) {
      throw BuildError(
          "Specified target does not exist: " + default_path.original());
    }
    // This may result in duplicate values in result, which is ok
    result.push_back(it->second);
  }
  return result;
}

bool hasDependencyCycle(
    const CompiledManifest &manifest,
    const detail::PathToStepMap &output_path_map,
    const std::vector<RawStep> &raw_steps,
    std::vector<bool> &currently_visited,
    std::vector<bool> &already_visited,
    std::vector<Path> &cycle_paths,
    StepIndex idx,
    std::string *cycle) {
  if (currently_visited[idx]) {
    *cycle = detail::cycleErrorMessage(cycle_paths);
    return true;
  }

  if (already_visited[idx]) {
    // The step has already been processed.
    return false;
  }
  already_visited[idx] = true;

  bool found_cycle = false;
  const auto process_inputs = [&](const std::vector<Path> &inputs) {
    if (found_cycle) {
      return;
    }

    for (const auto &input : inputs) {
      const auto it = output_path_map.find(input);
      if (it == output_path_map.end()) {
        // This input is not an output of some other build step.
        continue;
      }

      const auto dependency_idx = it->second;

      cycle_paths.push_back(input);
      if (hasDependencyCycle(
              manifest,
              output_path_map,
              raw_steps,
              currently_visited,
              already_visited,
              cycle_paths,
              dependency_idx,
              cycle)) {
        found_cycle = true;
        return;
      }
      cycle_paths.pop_back();
    }
  };

  currently_visited[idx] = true;
  process_inputs(raw_steps[idx].inputs);
  process_inputs(raw_steps[idx].implicit_inputs);
  process_inputs(raw_steps[idx].dependencies);
  currently_visited[idx] = false;

  return found_cycle;
}

std::string getDependencyCycle(
    const CompiledManifest &compiled_manifest,
    const detail::PathToStepMap &output_path_map,
    const std::vector<RawStep> &raw_steps) {
  std::vector<bool> currently_visited(compiled_manifest.steps().size());
  std::vector<bool> already_visited(compiled_manifest.steps().size());
  std::vector<Path> cycle_paths;
  cycle_paths.reserve(32);  // Guess at largest typical build dependency depth

  std::string cycle;
  for (StepIndex idx = 0; idx < compiled_manifest.steps().size(); idx++) {
    if (hasDependencyCycle(
            compiled_manifest,
            output_path_map,
            raw_steps,
            currently_visited,
            already_visited,
            cycle_paths,
            idx,
            &cycle)) {
      break;
    }
  }

  return cycle;
}

StepIndex getManifestStep(
    const detail::PathToStepMap &output_path_map,
    Path manifest_path) {
  auto step_it = output_path_map.find(manifest_path);
  if (step_it == output_path_map.end()) {
    return -1;
  } else {
    return step_it->second;
  }
}

}  // anonymous namespace

CompiledManifest::CompiledManifest(
    Path manifest_path,
    RawManifest &&manifest)
    : CompiledManifest(
          detail::computeOutputPathMap(manifest.steps),
          manifest_path,
          std::move(manifest)) {}

CompiledManifest::CompiledManifest(
    const detail::PathToStepMap &output_path_map,
    Path manifest_path,
    RawManifest &&manifest)
    : _builder(std::make_shared<flatbuffers::FlatBufferBuilder>(1024)),
      _outputs(computePathList(output_path_map)),
      _inputs(computePathList(computeInputPathMap(manifest.steps))),
      _steps(convertStepVector(
          output_path_map, _step_buffers, std::move(manifest.steps))),
      _defaults(computeStepsToBuildFromPaths(
          manifest.defaults, output_path_map)),
      _roots(detail::rootSteps(_steps)),
      _pools(std::move(manifest.pools)) {

  std::vector<flatbuffers::Offset<ShkManifest::StepPathReference>> outputs;
  auto outputs_vector = _builder->CreateVector(
      outputs.data(), outputs.size());

  std::vector<flatbuffers::Offset<ShkManifest::StepPathReference>> inputs;
  auto inputs_vector = _builder->CreateVector(
      inputs.data(), inputs.size());

  std::vector<flatbuffers::Offset<ShkManifest::Step>> steps;
  auto steps_vector = _builder->CreateVector(
      steps.data(), steps.size());

  std::vector<StepIndex> defaults;
  auto defaults_vector = _builder->CreateVector(
      defaults.data(), defaults.size());

  std::vector<StepIndex> roots;
  auto roots_vector = _builder->CreateVector(
      roots.data(), roots.size());

  std::vector<flatbuffers::Offset<ShkManifest::Pool>> pools;
  auto pools_vector = _builder->CreateVector(
      pools.data(), pools.size());

  auto build_dir_string = _builder->CreateString(manifest.build_dir);

  auto dependency_cycle_string = _builder->CreateString(
      getDependencyCycle(
          *this,
          output_path_map,
          manifest.steps));

  ShkManifest::ManifestBuilder manifest_builder(*_builder);
  manifest_builder.add_outputs(outputs_vector);
  manifest_builder.add_inputs(inputs_vector);
  manifest_builder.add_steps(steps_vector);
  manifest_builder.add_defaults(defaults_vector);
  manifest_builder.add_roots(roots_vector);
  manifest_builder.add_pools(pools_vector);
  manifest_builder.add_build_dir(build_dir_string);
  manifest_builder.add_manifest_step(getManifestStep(output_path_map, manifest_path));
  manifest_builder.add_dependency_cycle(dependency_cycle_string);
  _builder->Finish(manifest_builder.Finish());

  _manifest = ShkManifest::GetManifest(_builder->GetBufferPointer());
}

}  // namespace shk
