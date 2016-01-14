#include "generators.h"

namespace shk {

void showValue(const Path &path, std::ostream &os) {
  os << "'" << path.canonicalized() << "'";
}

namespace gen {

rc::Gen<std::string> pathComponent() {
  return rc::gen::nonEmpty(
      rc::gen::container<std::string>(rc::gen::inRange<char>('a', 'z')));
}

rc::Gen<std::vector<std::string>> pathComponents() {
  return rc::gen::resize(
      10,
      rc::gen::container<std::vector<std::string>>(pathComponent()));
}

std::string joinPathComponents(
    const std::vector<std::string> &path_components) {
  std::string path;
  for (const auto &path_component : path_components) {
    if (!path.empty()) {
      path += "/";
    }
    path += path_component;
  }
  return path;
}

rc::Gen<std::string> pathString() {
  return rc::gen::exec([] {
    return joinPathComponents(*pathComponents());
  });
}

rc::Gen<shk::Path> path(const std::shared_ptr<Paths> &paths) {
  return rc::gen::exec([paths] {
    return paths->get(*pathString());
  });
}

rc::Gen<shk::Path> pathWithSingleComponent(const std::shared_ptr<Paths> &paths) {
  return rc::gen::exec([paths] {
    return paths->get(*pathComponent());
  });
}

rc::Gen<std::vector<Path>> pathVector(const std::shared_ptr<Paths> &paths) {
  return rc::gen::container<std::vector<Path>>(path(paths));
}

rc::Gen<std::vector<Path>> pathWithSingleComponentVector(
    const std::shared_ptr<Paths> &paths) {
  return rc::gen::container<std::vector<Path>>(pathWithSingleComponent(paths));
}

}  // namespace gen
}  // namespace shk
