#pragma once

#include <string>
#include <vector>

#include "hash.h"
#include "optional.h"
#include "manifest/manifest_generated.h"
#include "manifest/raw_step.h"
#include "manifest/wrapper_view.h"

namespace shk {

/**
 * Manifest objects contain an std::vector<Step>. A StepIndex is an index into
 * that vector, or into a vector of the same length that refers to the same Step
 * objects.
 */
using StepIndex = int;

namespace detail {

inline nt_string_view fbStringToView(const flatbuffers::String *string) {
  return nt_string_view(string->c_str(), string->size());
}

inline StepIndex indexToView(const StepIndex &index) {
  return index;
}

template <typename T>
using FbIterator = flatbuffers::VectorIterator<
    flatbuffers::Offset<T>,
    const T *>;

template <typename View>
inline View toFlatbufferView(
    const flatbuffers::Vector<flatbuffers::Offset<
        typename std::remove_const<typename std::remove_pointer<
            typename View::inner_iterator::value_type>::type>::type>> *vec) {
  return vec ?
      View(vec->begin(), vec->end()) :
      View(
          typename View::inner_iterator(nullptr, 0),
          typename View::inner_iterator(nullptr, 0));
}

}  // namespace detail

using StepIndicesView = WrapperView<
    const StepIndex *,
    StepIndex,
    &detail::indexToView>;

using StringsView = WrapperView<
    detail::FbIterator<flatbuffers::String>,
    nt_string_view,
    &detail::fbStringToView>;

namespace detail {

inline const StepIndicesView toStepIndicesView(
    const flatbuffers::Vector<int32_t> *ints) {
  return ints ?
      StepIndicesView(ints->data(), ints->data() + ints->size()) :
      StepIndicesView();
}

inline nt_string_view toStringView(const flatbuffers::String *str) {
  return str ?
      nt_string_view(str->data(), str->size()) :
      nt_string_view();
}

}  // namespace detail

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
  Step(const ShkManifest::Step &step) : _step(&step) {}

  /**
   * A hash of this build step. The hash is used when comparing against old
   * build steps that have been run to see if the build step is clean.
   */
  const Hash &hash() const {
    return *reinterpret_cast<const Hash *>(_step->hash());
  }

  /**
   * A list of indices for steps that must be done and clean before this step
   * can be run. These correspond to "order only", "implicit inputs" and
   * "inputs" in a build.ninja manifest.
   */
  const StepIndicesView dependencies() const {
    return detail::toStepIndicesView(_step->dependencies());
  }

  /**
   * A list of directories that Shuriken should ensure are there prior to
   * invoking the command.
   */
  StringsView outputDirs() const {
    return detail::toFlatbufferView<StringsView>(_step->output_dirs());
  }

  nt_string_view poolName() const {
    return detail::toStringView(_step->pool_name());
  }

  /**
   * Command that should be invoked in order to perform this build step.
   *
   * The command string is empty for phony rules.
   */
  nt_string_view command() const {
    return detail::toStringView(_step->command());
  }

  /**
   * A short description of the command. Used for prettifying output while
   * running builds.
   */
  nt_string_view description() const {
    return detail::toStringView(_step->description());
  }

  bool phony() const {
    return command().empty();
  }

  /**
   * For compatibility reasons with Ninja, Shuriken keeps track of the path to
   * a potential depfile generated by the build steps. Shuriken does not use
   * this file, it just removes it immediately after the build step has
   * completed.
   */
  nt_string_view depfile() const {
    return detail::toStringView(_step->depfile());
  }

  /**
   * If rspfile is not empty, Shuriken will write rspfile_content to the path
   * specified by rspfile before running the build step and then remove the file
   * after the build step has finished running. Useful on Windows, where
   * commands have a rather short maximum length.
   */
  nt_string_view rspfile() const {
    return detail::toStringView(_step->rspfile());
  }

  nt_string_view rspfileContent() const {
    return detail::toStringView(_step->rspfile_content());
  }

  /**
   * It set to true, Shuriken will treat this build step as one that rewrites
   * manifest files. They are treated specially in the following ways:
   *
   * * They are not rebuilt if the command line changes.
   * * Instead of tracing commands as usual, Shuriken uses inputs and outputs
   *   declared in the manifest.
   * * Files are checked for dirtiness via mtime checks rather than file hashes.
   * * They are not cleaned.
   */
  bool generator() const {
    return _step->generator();
  }

  /**
   * A list of paths to files that this step has as inputs. These are taken from
   * the inputs, the implicit inputs and the order-only dependencies in the
   * manifest.
   *
   * Empty if generator() == false.
   *
   * The use case for this is to emulate Ninja's mtime based cleanliness check
   * for generator rules. Shuriken's normal hash-based traced dependencies don't
   * work well with generator rules, because they force Shuriken to re-run the
   * generator step first thing after the build.ninja file has been generated
   * and Shuriken is invoked for the first time.
   */
  StringsView generatorInputs() const {
    return detail::toFlatbufferView<StringsView>(_step->generator_inputs());
  }

  /**
   * A list of paths to files that this step has as inputs. These are taken from
   * the outputs declared in the manifest.
   *
   * Empty if generator() == false.
   *
   * Also see generatorInputs.
   */
  StringsView generatorOutputs() const {
    return detail::toFlatbufferView<StringsView>(_step->generator_outputs());
  }

 private:
  const ShkManifest::Step *_step;
};

inline bool isConsolePool(nt_string_view pool_name) {
  return pool_name == "console";
}

}  // namespace shk
