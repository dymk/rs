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

#include "test_util.h"

namespace shk {

Flatbuffer<TestRequest> MakeTestRequest(int data) {
  flatbuffers::grpc::MessageBuilder fbb;
  auto test_request = CreateTestRequest(fbb, data);
  fbb.Finish(test_request);
  return fbb.GetMessage<TestRequest>();
}

Flatbuffer<TestResponse> MakeTestResponse(int data) {
  flatbuffers::grpc::MessageBuilder fbb;
  auto test_response = CreateTestResponse(fbb, data);
  fbb.Finish(test_response);
  return fbb.GetMessage<TestResponse>();
}

void ShutdownAllowOutstandingCall(RsGrpcServer *server) {
  auto deadline = std::chrono::system_clock::now();
  server->Shutdown(deadline);
}

}  // namespace shk
