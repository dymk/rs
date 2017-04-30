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

#include "cmdline_options.h"

namespace shk {
namespace {

CmdlineOptions parse(const std::vector<std::string> &options) {
  std::vector<char *> opts_cstr{ const_cast<char *>("trace") };
  for (const auto &option : options) {
    opts_cstr.push_back(const_cast<char *>(option.c_str()));
  }
  return CmdlineOptions::parse(opts_cstr.size(), opts_cstr.data());
}

}  // anonymous namespace

TEST_CASE("CmdlineOptions") {
  SECTION("Version") {
    CHECK(parse({ "--version" }).result == CmdlineOptions::Result::VERSION);
  }

  SECTION("Help") {
    CHECK(parse({ "--help" }).result == CmdlineOptions::Result::HELP);
    CHECK(parse({ "-h" }).result == CmdlineOptions::Result::HELP);
  }

  SECTION("Empty") {
    CHECK(parse({}).result == CmdlineOptions::Result::HELP);
  }

  SECTION("Nonflag") {
    CHECK(parse({ "xyz" }).result == CmdlineOptions::Result::HELP);
  }

  SECTION("Trailing") {
    CHECK(parse({ "-f", "file", "-c", "cmd", "xyz" }).result == CmdlineOptions::Result::HELP);
    CHECK(parse({ "-f", "file", "xyz", "-c", "cmd" }).result == CmdlineOptions::Result::HELP);
  }

  SECTION("JustCommand") {
    auto options = parse({ "-c", "abc" });
    CHECK(options.result == CmdlineOptions::Result::SUCCESS);
    CHECK(options.tracefile == "/dev/null");
    CHECK(options.command == "abc");
    CHECK(options.suicide_when_orphaned == false);
    CHECK(options.json == false);
  }

  SECTION("Json") {
    SECTION("Short") {
      auto options = parse({ "-c", "abc", "-j" });
      CHECK(options.result == CmdlineOptions::Result::SUCCESS);
      CHECK(options.tracefile == "/dev/null");
      CHECK(options.command == "abc");
      CHECK(options.suicide_when_orphaned == false);
      CHECK(options.json == true);
    }

    SECTION("Long") {
      auto options = parse({ "-c", "abc", "--json" });
      CHECK(options.result == CmdlineOptions::Result::SUCCESS);
      CHECK(options.tracefile == "/dev/null");
      CHECK(options.command == "abc");
      CHECK(options.suicide_when_orphaned == false);
      CHECK(options.json == true);
    }
  }

  SECTION("SuicideWhenOrphaned") {
    auto options = parse({ "-c", "hej", "--suicide-when-orphaned" });
    CHECK(options.result == CmdlineOptions::Result::HELP);
  }

  SECTION("SuicideWhenOrphanedShort") {
    auto options = parse({ "-c", "hej", "-O" });
    CHECK(options.result == CmdlineOptions::Result::HELP);
  }

  SECTION("JustTracefile") {
    CHECK(parse({ "-f", "xyz" }).result == CmdlineOptions::Result::HELP);
  }

  SECTION("MissingFollowup") {
    CHECK(parse({ "-f" }).result == CmdlineOptions::Result::HELP);
    CHECK(parse({ "-f", "file", "-c" }).result == CmdlineOptions::Result::HELP);
    CHECK(parse({ "-c", "cmd", "-f" }).result == CmdlineOptions::Result::HELP);
  }

  SECTION("TwoTracefiles") {
    CHECK(
        parse({ "-c", "abc", "-f", "xyz", "-f", "123" }).result ==
        CmdlineOptions::Result::HELP);
  }

  SECTION("EmptyTraceFile") {
    CHECK(
        parse({ "-c", "abc", "-f", "" }).result ==
        CmdlineOptions::Result::HELP);
  }

  SECTION("TwoCommands") {
    CHECK(
        parse({ "-c", "abc", "-c", "xyz", "-f", "123" }).result ==
        CmdlineOptions::Result::HELP);
  }

  SECTION("CommandFirst") {
    auto options = parse({ "-c", "abc", "-f", "123" });
    CHECK(options.tracefile == "123");
    CHECK(options.command == "abc");
  }

  SECTION("TracefileFirst") {
    auto options = parse({ "-f", "abc", "-c", "123" });
    CHECK(options.tracefile == "abc");
    CHECK(options.command == "123");
  }

  SECTION("Server") {
    SECTION("Short") {
      auto options = parse({ "-s" });
      CHECK(options.server == true);
    }

    SECTION("Long") {
      auto options = parse({ "--server" });
      CHECK(options.server == true);
    }

    SECTION("TrailingArg") {
      CHECK(parse({ "-s", "xyz" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("Capture") {
      SECTION("Short") {
        auto options = parse({ "-s", "-C", "f" });
        CHECK(options.result == CmdlineOptions::Result::SUCCESS);
        CHECK(options.server);
        CHECK(options.capture == "f");
      }

      SECTION("Long") {
        auto options = parse({ "-s", "--capture", "f" });
        CHECK(options.result == CmdlineOptions::Result::SUCCESS);
        CHECK(options.server);
        CHECK(options.capture == "f");
      }

      SECTION("WithoutFile") {
        CHECK(parse({ "-s", "-c" }).result == CmdlineOptions::Result::HELP);
      }
    }

    SECTION("SuicideWhenOrphaned") {
      SECTION("Short") {
        auto options = parse({ "-s", "-O" });
        CHECK(options.server == true);
        CHECK(options.suicide_when_orphaned == true);
      }

      SECTION("SuicideWhenOrphaned") {
        auto options = parse({ "-s", "--suicide-when-orphaned" });
        CHECK(options.server == true);
        CHECK(options.suicide_when_orphaned == true);
      }
    }

    SECTION("WithTraceFile") {
      CHECK(parse({ "-s", "-f", "f" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("WithReplay") {
      CHECK(parse({ "-s", "-r", "f" }).result == CmdlineOptions::Result::HELP);
      CHECK(
          parse({ "-s", "--replay", "f" }).result ==
          CmdlineOptions::Result::HELP);
    }

    SECTION("WithCommand") {
      CHECK(parse({ "-s", "-c", "c" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("Json") {
      CHECK(parse({ "-s", "-j" }).result == CmdlineOptions::Result::HELP);
      CHECK(parse({ "-s", "--json" }).result == CmdlineOptions::Result::HELP);
    }
  }

  SECTION("Replay") {
    SECTION("Short") {
      auto options = parse({ "-r", "f" });
      CHECK(options.result == CmdlineOptions::Result::SUCCESS);
      CHECK(options.replay == "f");
    }

    SECTION("Long") {
      auto options = parse({ "--replay", "f" });
      CHECK(options.result == CmdlineOptions::Result::SUCCESS);
      CHECK(options.replay == "f");
    }

    SECTION("MissingFile") {
      CHECK(parse({ "-r" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("TrailingArg") {
      CHECK(parse({ "-r", "f", "xyz" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("Capture") {
      CHECK(parse({ "-r", "f", "-C" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("SuicideWhenOrphaned") {
      CHECK(parse({ "-r", "f", "-O" }).result == CmdlineOptions::Result::HELP);
    }

    SECTION("WithTraceFile") {
      CHECK(
          parse({ "-r", "f", "-f", "f" }).result ==
          CmdlineOptions::Result::HELP);
    }

    SECTION("WithCapture") {
      CHECK(
          parse({ "-r", "f", "-C", "f" }).result ==
          CmdlineOptions::Result::HELP);
      CHECK(
          parse({ "-r", "f", "--capture", "f" }).result ==
          CmdlineOptions::Result::HELP);
    }

    SECTION("WithCommand") {
      CHECK(
          parse({ "-r", "f", "-c", "c" }).result ==
          CmdlineOptions::Result::HELP);
    }

    SECTION("Json") {
      CHECK(
          parse({ "-r", "f", "-j" }).result ==
          CmdlineOptions::Result::HELP);
      CHECK(
          parse({ "-r", "f", "--json" }).result ==
          CmdlineOptions::Result::HELP);
    }
  }
}

}  // namespace shk
