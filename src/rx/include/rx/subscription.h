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

#include <limits>
#include <memory>
#include <type_traits>

namespace shk {

/**
 * Classes that conform to the Subscription concept should inherit from this
 * class to signify that they are a Subscription.
 *
 * Subscription types must have the following method:
 *
 * * void Request(size_t count);
 *
 * Destroying a Subscription object implicitly cancels the subscription.
 */
class SubscriptionBase {
 protected:
  ~SubscriptionBase() = default;
};

namespace detail {

class EmptySubscription : public SubscriptionBase {
 public:
  void Request(size_t count);
};

}  // namespace detail

template <typename T>
constexpr bool IsSubscription = std::is_base_of<SubscriptionBase, T>::value;

/**
 * Type erasure wrapper for Subscription objects.
 */
class Subscription : public SubscriptionBase {
 public:
  static constexpr size_t kAll = std::numeric_limits<size_t>::max();

  /**
   * S should implement the Subscription concept.
   */
  template <typename S>
  explicit Subscription(
      typename std::enable_if<IsSubscription<S>, S>::type &&s)
      : eraser_(std::make_unique<SubscriptionEraser<S>>(std::forward<S>(s))) {}

  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  void Request(size_t count);

 private:
  class Eraser {
   public:
    virtual ~Eraser();
    virtual void Request(size_t count) = 0;
  };

  template <typename S>
  class SubscriptionEraser : public Eraser {
   public:
    SubscriptionEraser(S &&subscription)
        : subscription_(std::move(subscription)) {}

    void Request(size_t count) override {
      subscription_.Request(count);
    }

   private:
    S subscription_;
  };

  std::unique_ptr<Eraser> eraser_;
};

detail::EmptySubscription MakeSubscription();

template <typename RequestCb>
auto MakeSubscription(RequestCb &&request) {
  class RequestSubscription : public SubscriptionBase {
   public:
    RequestSubscription(RequestCb &&request)
        : request_(std::forward<RequestCb>(request)) {}

    void Request(size_t count) {
      request_(count);
    }

   private:
    RequestCb request_;
  };

  return RequestSubscription(std::forward<RequestCb>(request));
}

}  // namespace shk
