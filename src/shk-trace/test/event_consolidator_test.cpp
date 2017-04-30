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

#include <catch.hpp>

#include "event_consolidator.h"

namespace shk {
namespace {

struct ParsedTrace {
  struct Input {
    Input() = default;
    Input(std::string &&path, bool directory_listing)
        : path(path), directory_listing(directory_listing) {}

    std::string path;
    bool directory_listing = false;
  };

  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<std::string> errors;
};

ParsedTrace generateTrace(const EventConsolidator &consolidator) {
  flatbuffers::FlatBufferBuilder builder(1024);
  builder.Finish(consolidator.generateTrace(builder));

  auto trace = GetTrace(builder.GetBufferPointer());

  ParsedTrace parsed_trace;

  for (int i = 0; i < trace->inputs()->size(); i++) {
    const auto *input = trace->inputs()->Get(i);
    parsed_trace.inputs.push_back(input->c_str());
  }

  for (int i = 0; i < trace->outputs()->size(); i++) {
    const auto *output = trace->outputs()->Get(i);
    parsed_trace.outputs.push_back(output->c_str());
  }

  for (int i = 0; i < trace->errors()->size(); i++) {
    const auto *error = trace->errors()->Get(i);
    parsed_trace.errors.push_back(error->c_str());
  }

  return parsed_trace;
}

bool hasError(const EventConsolidator &consolidator) {
  return !generateTrace(consolidator).errors.empty();
}

}  // anonymous namespace

TEST_CASE("EventConsolidator") {
  using ET = EventType;

  EventConsolidator ec;

  SECTION("Copyable") {
    ec.event(ET::FATAL_ERROR, "");
    auto ec2 = ec;

    CHECK(hasError(ec));
    CHECK(hasError(ec2));
  }

  SECTION("Assignable") {
    EventConsolidator ec2;
    ec2.event(ET::FATAL_ERROR, "");
    ec = ec2;

    CHECK(hasError(ec));
    CHECK(hasError(ec2));
  }

  SECTION("SingleEvents") {
    SECTION("Read") {
      ec.event(ET::READ, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadDirectory") {
      ec.event(ET::READ_DIRECTORY, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("Write") {
      ec.event(ET::WRITE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      REQUIRE(trace.errors.empty());
    }

    SECTION("Create") {
      ec.event(ET::CREATE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("Delete") {
      ec.event(ET::DELETE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      CHECK(trace.outputs.empty());

      REQUIRE(trace.errors.size() == 1);
      CHECK(trace.errors[0] ==
          "Process deleted file it did not create: hello");
    }

    SECTION("FatalError") {
      ec.event(ET::FATAL_ERROR, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      CHECK(trace.outputs.empty());

      REQUIRE(trace.errors.size() == 1);
      CHECK(trace.errors[0] == "hello");
    }
  }

  SECTION("TwoEvents") {
    SECTION("ReadTwice") {
      ec.event(ET::READ, "hello");
      ec.event(ET::READ, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadDifferentFiles") {
      ec.event(ET::READ, "hello1");
      ec.event(ET::READ, "hello2");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 2);
      if (trace.inputs[0] == "hello2") {
        std::swap(trace.inputs[0], trace.inputs[1]);
      }
      CHECK(trace.inputs[0] == "hello1");
      CHECK(trace.inputs[1] == "hello2");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadThenReadDirectory") {
      ec.event(ET::READ, "hello");
      ec.event(ET::READ_DIRECTORY, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadDirectoryThenRead") {
      ec.event(ET::READ_DIRECTORY, "hello");
      ec.event(ET::READ, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadDirectoryTwice") {
      ec.event(ET::READ_DIRECTORY, "hello");
      ec.event(ET::READ_DIRECTORY, "hello");
      auto trace = generateTrace(ec);

      REQUIRE(trace.inputs.size() == 1);
      CHECK(trace.inputs[0] == "hello");

      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("CreateThenRead") {
      ec.event(ET::CREATE, "hello");
      ec.event(ET::READ, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("CreateThenReadDirectory") {
      ec.event(ET::CREATE, "hello");
      ec.event(ET::READ, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("CreateThenWrite") {
      ec.event(ET::CREATE, "hello");
      ec.event(ET::WRITE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("CreateThenDelete") {
      ec.event(ET::CREATE, "hello");
      ec.event(ET::DELETE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());
      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }

    SECTION("ReadThenWrite") {
      ec.event(ET::READ, "hello");
      ec.event(ET::WRITE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      REQUIRE(trace.errors.empty());
    }

    SECTION("ReadDirectoryThenWrite") {
      ec.event(ET::READ_DIRECTORY, "hello");
      ec.event(ET::WRITE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      REQUIRE(trace.errors.empty());
    }

    SECTION("ReadThenCreate") {
      ec.event(ET::READ, "hello");
      ec.event(ET::CREATE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("ReadDirectoryThenCreate") {
      ec.event(ET::READ_DIRECTORY, "hello");
      ec.event(ET::CREATE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("DeleteThenCreate") {
      ec.event(ET::DELETE, "hello");
      ec.event(ET::CREATE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }
  }

  SECTION("ThreeEvents") {
    SECTION("CreateThenDeleteThenCreate") {
      ec.event(ET::CREATE, "hello");
      ec.event(ET::DELETE, "hello");
      ec.event(ET::CREATE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());

      REQUIRE(trace.outputs.size() == 1);
      CHECK(trace.outputs[0] == "hello");

      CHECK(trace.errors.empty());
    }

    SECTION("DeleteThenCreateThenDelete") {
      ec.event(ET::DELETE, "hello");
      ec.event(ET::CREATE, "hello");
      ec.event(ET::DELETE, "hello");
      auto trace = generateTrace(ec);

      CHECK(trace.inputs.empty());
      CHECK(trace.outputs.empty());
      CHECK(trace.errors.empty());
    }
  }
}

}  // namespace shk
