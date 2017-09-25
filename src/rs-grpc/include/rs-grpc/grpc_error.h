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

#include <stdexcept>

#include <grpc++/grpc++.h>

namespace shk {

using GrpcErrorHandler = std::function<void (std::exception_ptr)>;

class GrpcError : public std::runtime_error {
 public:
  explicit GrpcError(const grpc::Status &status)
      : runtime_error(what(status)),
        status_(status) {}

  const char *what() const throw() override {
    return what(status_);
  }

 private:
  static const char *what(const grpc::Status &status) throw() {
    const auto &message = status.error_message();
    return message.empty() ? "[No error message]" : message.c_str();
  }

  const grpc::Status status_;
};

inline std::string ExceptionMessage(const std::exception_ptr &error) {
  try {
    std::rethrow_exception(error);
  } catch (const std::exception &exception) {
    return exception.what();
  } catch (...) {
    return "Unknown error";
  }
}

inline grpc::Status ExceptionToStatus(const std::exception_ptr &error) {
  if (!error) {
    return grpc::Status::OK;
  } else {
    // TODO(peck): Make it possible to respond with other errors
    // than INTERNAL (by catching GrpcErrors and reporting that)
    const auto what = ExceptionMessage(error);
    return grpc::Status(grpc::INTERNAL, what);
  }
}

}  // namespace shk
