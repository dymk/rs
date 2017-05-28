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

#include <rx/subscription.h>

namespace shk {

template <typename T>
auto Just(T &&t) {
  return [t = std::forward<T>(t)](auto subscriber) {
    return MakeSubscription(
        [
            t,
            subscriber = std::move(subscriber),
            sent = false](size_t count) mutable {
          if (!sent && count != 0) {
            sent = true;
            subscriber.OnNext(std::move(t));
            subscriber.OnComplete();
          }
        });
  };
}

#if 0
template <typename Source>
auto Count(const Source &source) {
  return [source](auto subscriber) {
    return MakeSubscription(
        [
            source,
            subscriber = std::move(subscriber),
            sent = false](size_t count) mutable {
          if (!sent && count != 0) {
            sent = true;

            auto subscription = source(
                MakeSubscriber(
                    [](auto &&next) { count++; },
                    [](std::exception_ptr &&error) { fail },
                    []() { complete }));
            subscription.Request(Subscription::kAll);
          }
        });
  };
}
#endif

}  // namespace shk
