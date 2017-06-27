# `rs` reference documentation

This document has API documentation for the rs Reactive Streams library.

See also
Table of contents


## `All(Predicate)`

**Defined in:** [`rs/all.h`](../include/rs/all.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[bool])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#every), [ReactiveX](http://reactivex.io/documentation/operators/all.html)

**Description:** Operator that returns a stream that emits exactly one value: `true` if all the input elements match the given predicate, `false` otherwise. As soon as an element is encountered that matches the predicate, the boolean is emitted and the input stream is cancelled.

**Example usage:**

```cpp
auto input = Just(1, 2, 5, 1, -3, 1);
auto all_positive = Pipe(
    input,
    All([](int value) { return value > 0; }));
```

**See also:** `Some`


## `Average()`

**Defined in:** [`rs/average.h`](../include/rs/average.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[ResultType])`, where `ResultType` is `double` by default and configurable by a template parameter to `Average()`.

**External documentation:** [RxMarbles](http://rxmarbles.com/#average), [ReactiveX](http://reactivex.io/documentation/operators/average.html)

**Description:** Operator that returns a stream that emits exactly one value: the (numeric) average of all of the input elements.

**Example usage:**

```cpp
auto average = Pipe(
    Just(1, 2, 3, 4, 5),
    Average());
```

## `BuildPipe(Operator...)`

**Defined in:** [`rs/pipe.h`](../include/rs/pipe.h)

**Kind:** [Operator Builder Builder](#kind_operator_builder_builder)

**[Type](#types):** `(Publisher[a] -> Publisher[b]), (Publisher[b] -> Publisher[c]), ... -> (Publisher[a] -> Publisher[c])`

**Description:** Helper function that can be used to combine a series of operators into a composite operator.

* `BuildPipe()` returns a unary identity function.
* `BuildPipe(x)` is basically equivalent to `x` if `x` is a unary functor.
* `BuildPipe(x, y)` is basically equivalent to `[](auto &&a) { return y(x(std::forward<decltype(a)>(a))); }`

**Example usage:**

```cpp
// root_mean_square is an operator that takes a Publisher and returns a
// Publisher that emits the root mean square of all the elements in the input
// publisher.
auto root_mean_square = BuildPipe(
    Map([](auto x) { return x * x; }),
    Average(),
    Map([](auto x) { return sqrt(x); }));

// Returns a Publisher that emits sqrt((1*1 + 2*2 + 3*3) / 3)
root_mean_square(Just(1, 2, 3))
```


## `Catch(Publisher)`

**Defined in:** [`rs/catch.h`](../include/rs/catch.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(std::exception_ptr&& -> Publisher[b]) -> (Publisher[a] -> Publisher[a, b])`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/catch.html)

**Description:** `Catch` is rs' asynchronous version of the `try`/`catch` statement in C++. It creates and operator that takes a Publisher and returns a Publisher that if successful behaves the same, but it if fails invokes the provided callback with the `std::exception_ptr` error and concatenates the callback's return value to the input stream.

**Example usage:**

```cpp
// one_to_ten is a Publisher that enumerates integers from 1 to 10
auto one_to_ten = Pipe(
    Range(1, 10),
    Catch([](std::exception_ptr &&error) {
      printf("This is not called because the stream does not fail\n");
      return Empty();
    }));
```

```cpp
auto video_id = Pipe(
    video_content.Upload("filename.mp4"),
    Catch([](std::exception_ptr &&error) {
      // If the Upload operation fails, this is called.

      // Log the error and rethrow
      printf("Video upload failed\n");
      return Throw(error);
    }));
```

```cpp
auto video_id = Pipe(
    video_content.Upload("filename.mp4"),
    Catch([](std::exception_ptr &&error) {
      // If the Upload operation fails, this is called.

      // Handle the error
      return Just(video_content.MakeEmptyId());
    }));
```


## `Concat(Publisher...)`

**Defined in:** [`rs/concat.h`](../include/rs/concat.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `Publisher[a], Publisher[b], ... -> Publisher[a, b, ...]`

**External documentation:** [RxMarbles](http://rxmarbles.com/#concat), [ReactiveX](http://reactivex.io/documentation/operators/concat.html)

**Description:** Concatenate zero or more Publishers. The values of the first stream will be emitted first. Then, the values of the second stream are emitted and so on.

**Example usage:**

```cpp
auto count_to_100_twice = Concat(
    Range(1, 100),
    Range(1, 100));
```

```cpp
// Concat with only one stream creates a Publisher that behaves just like the
// inner publisher.
auto one_two_three_stream = Concat(Just(1, 2, 3));
```

```cpp
// Concat() is equivalent to Empty()
auto empty = Concat();
```

**See also:** `Empty`


## `Count(Publisher...)`

**Defined in:** [`rs/count.h`](../include/rs/count.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[CountType])`, where `CountType` is `int` by default and configurable by a template parameter to `Count()`.

**External documentation:** [RxMarbles](http://rxmarbles.com/#count), [ReactiveX](http://reactivex.io/documentation/operators/count.html)

**Description:** Count the number of elements in a Publisher and emit only one value: The number of elements in the input stream.

**Example usage:**

```cpp
// count is a Publisher that emits just the int 3
auto count = Count(Just(1, 2, 3));
```

```cpp
// By default, Count counts using an int, but that is configurable
auto count = Count<double>(Just(1));
```


## `DefaultIfEmpty(Value...)`

**Defined in:** [`rs/default_if_empty.h`](../include/rs/default_if_empty.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `a, b, ... -> (Publisher[c] -> Publisher[a, b, c, ...])`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/defaultifempty.html)

**Description:** `DefaultIfEmpty` takes a Publisher and returns one that behaves just like it, except that if it finishes without emitting any elements, it emits given values.

**Example usage:**

```cpp
// Because the input stream emits values, DefaultIfEmpty does nothing in this
// case.
auto one_two_three = Pipe(
    Just(1, 2, 3),
    DefaultIfEmpty(42));
```

```cpp
// The input stream is empty, so DefaultIfEmpty emits 42
auto fourty_two = Pipe(
    Empty(),
    DefaultIfEmpty(42));
```

```cpp
// DefaultIfEmpty accepts any number of parameters
auto one_two_three = Pipe(
    Empty(),
    DefaultIfEmpty(1, 2, 3));

auto empty = Pipe(
    Empty(),
    DefaultIfEmpty());  // DefaultIfEmpty with no parameters is a no-op.
```

**See also:** `IfEmpty`


## `ElementAt(size_t)`

**Defined in:** [`rs/element_at.h`](../include/rs/element_at.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `size_t -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#elementAt), [ReactiveX](http://reactivex.io/documentation/operators/elementat.html)

**Description:** Takes a stream and returns a stream that emits only one value: The `nth` element of the stream (zero indexed). If the number of elements in the stream is less than or equal to `nth`, the returned stream fails with an `std::out_of_range` exception.

**Example usage:**

```cpp
auto fifty = Pipe(
    Range(0, 100),
    ElementAt(50));
```

```cpp
// Fails with an std::out_of_range exception.
auto fail = Pipe(
    Just(1),
    ElementAt(1));
```

**See also:** `First`


## `ElementCount`

**Defined in:** [`rs/element_count.h`](../include/rs/element_count.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** `ElementCount` is the type that is given to the `Request` method on the Subscription concept. It is possible to `Request` zero or more values, and it's also possible to `Request` all the values of a stream by calling `Request(ElementCount::Unbounded())`.

In [the Java version of Reactive Streams](https://github.com/reactive-streams/reactive-streams-jvm#3.17), the `Subscription.request` method takes a `long` and uses `java.lang.Long.MAX_VALUE` to signify an unbounded number of elements. rs could have simply used C++'s corresponding `long long` type, but I found that doing so is very prone to integer overflow bugs.

`ElementCount` is similar to a `long long` but it handles integer overflow automatically.

**Example usage:**

```cpp
// Construct and assign ElementCount values
auto a = ElementCount(1);
auto unbounded = ElementCount::Unbounded();
a = ElementCount(2);
a = 3;

// Inspect the value
bool is_unbounded = unbounded.IsUnbounded();
long long value = a.Get();

// ElementCount overloads the basic addition and subtraction operators
auto b = a + a;
auto c = b - a;
auto d = a + 1;
auto e = a - 1;
b++;
++b;
b--;
--b;
b += a;
b -= a;
b += 1;
b -= 1;

// ElementCount can be compared with other values
bool f = a < b;
bool g = a < 1;
bool h = a <= b;
bool i = a <= 1;
bool j = a > b;
bool k = a > 1;
bool l = a >= b;
bool m = a >= 1;
bool n = a == b;
bool o = a != b;
bool p = a == 1;
bool q = 1 == a;

// Overflow handling:
unbounded == unbounded;
unbounded + 1 == unbounded;
unbounded - 1 == unbounded;
unbounded + unbounded == unbounded;
unbounded - unbounded == unbounded;
```


## `Empty()`

**Defined in:** [`rs/empty.h`](../include/rs/empty.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `void -> Publisher[]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/empty-never-throw.html)

**Description:** Creates a stream that emits no values.

**Example usage:**

```cpp
// empty is a Publisher that emits no values.
auto empty = Empty();
```

**See also:** `Just`


## `Filter(Predicate)`

**Defined in:** [`rs/filter.h`](../include/rs/filter.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#filter), [ReactiveX](http://reactivex.io/documentation/operators/filter.html)

**Description:** Takes a Publisher and returns one that behaves like it, except that it only emits the values that match a given predicate. It is similar to the `filter` function in functional programming (there are variations of it in [Javascript](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/filter), [Java 8 Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html#filter-java.util.function.Predicate-) and [Boost Ranges](http://www.boost.org/doc/libs/1_64_0/libs/iterator/doc/filter_iterator.html), for example).

**Example usage:**

```cpp
auto divisible_by_three = Pipe(
    Range(1, 100000),
    Filter([](int x) { return (x % 3) == 0; }));
```


## `First()`

**Defined in:** [`rs/first.h`](../include/rs/first.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#first), [ReactiveX](http://reactivex.io/documentation/operators/first.html)

**Description:** Takes a stream and returns a stream that emits only the first value in the input stream. If the input stream is empty, the returned stream fails with an `std::out_of_range` exception.

As soon as one element has been emitted from the input stream, the returned stream completes and the input stream is cancelled.

**Example usage:**

```cpp
auto five = Pipe(
    Just(5, 10, 15),
    First());
```

```cpp
// Fails with an std::out_of_range exception.
auto fail = Pipe(
    Empty(),
    First());
```

**See also:** `Take`, `ElementAt`, `Last`


## `First(Predicate)`

**Defined in:** [`rs/first.h`](../include/rs/first.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#first), [ReactiveX](http://reactivex.io/documentation/operators/first.html)

**Description:** Takes a stream and returns a stream that emits only the first value in the input stream that matches the given predicate. If the input stream is empty or if no elements matched the predicate, the returned stream fails with an `std::out_of_range` exception.

As soon as one element that matches the predicate has been emitted from the input stream, the returned stream completes and the input stream is cancelled.

**Example usage:**

```cpp
auto ten = Pipe(
    Just(5, 10, 15),
    First([](int x) { return x > 7; }));
```

```cpp
// Fails with an std::out_of_range exception.
auto fail = Pipe(
    Just(1, 2, 3, 4),
    First([](int x) { return x > 7; }));
```

**See also:** `Take`, `ElementAt`


## `FlatMap(Mapper)`

**Defined in:** [`rs/flat_map.h`](../include/rs/flat_map.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> Publisher[b]) -> (Publisher[a] -> Publisher[b])`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/flatmap.html)

**Description:** Similar to `Map`, but `FlatMap` takes a mapper function that returns a Publisher rather than a value. This makes it possible for the mapper function to perform asynchronous operations or to return zero or more than one value. This is similar to the [`flatMap` method in Java 8 Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html#flatMap-java.util.function.Function-).

**Example usage:**

```cpp
auto user_ids = std::vector<std::string>{ "1", "2", "3" };

// FlatMap can be used when the mapper function needs to perform asynchronous
// operations
auto users = Pipe(
    From(user_ids),
    FlatMap([user_database](const std::string &user_id) {
      return user_database->Lookup(user_id);
    }));
```

```cpp
// FlatMap can be used to implement the Filter operator
auto only_even = Pipe(
    Just(1, 2, 3, 4, 5),
    FlatMap([](int x) {
      if ((x % 2) == 0) {
        return Publisher<int>(Just(x));
      } else {
        return Publisher<int>(Empty());
      }
    }));
```

```cpp
// zero zeroes, one one, two twos, three threes etc. This stream emits:
// 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5
auto numbers = Pipe(
    Range(0, 5),
    FlatMap([](int x) {
      return Repeat(x, x);
    }));
```

**See also:** `Map`, `Concat`


## `From(Container)`

**Defined in:** [`rs/from.h`](../include/rs/from.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `Container[a] -> Publisher[a]`, where `Container[a]` is an STL-style container, for example `std::vector<a>`.

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/from.html)

**Description:** Create a Publisher from an STL-style container.

**Example usage:**

```cpp
auto numbers = From(std::vector<int>{ 1, 2, 3, 4, 5 });
```

**See also:** `Just`, `Empty`


## `IfEmpty(Publisher)`

**Defined in:** [`rs/if_empty.h`](../include/rs/if_empty.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `Publisher[a] -> (Publisher[b] -> Publisher[a, b])`

**Description:** `IfEmpty` takes a Publisher and returns one that behaves just like it, except that if it finishes without emitting any elements, it emits the values of the provided default stream. Similar to, but more generic, than `DefaultIfEmpty`.

**Example usage:**

```cpp
// Because the input stream emits values, IfEmpty does nothing in this case.
auto one_two_three = Pipe(
    Just(1, 2, 3),
    IfEmpty(Just(42)));
```

```cpp
// Here, IfEmpty is used to require that the input stream emits at least one
// element.
auto fail = Pipe(
    Empty(),
    IfEmpty(Throw(std::out_of_range("Input must not be empty"))));
```

**See also:** `DefaultIfEmpty`


## `IsPublisher`

**Defined in:** [`rs/publisher.h`](../include/rs/publisher.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** `IsPublisher` is a type predicate that checks if a given type claims to conform to the Publisher concept. (It checks if the given type publically inherits `PublisherBase`.)

**Example usage:**

```cpp
template <typename PublisherT>
void TakePublisher(const PublisherT &publisher) {
  static_assert(
      IsPublisher<PublisherT>,
      "TakePublisher must be called with a Publisher");

  // ...
}
```

**See also:** `IsSubscriber`, `IsSubscription`


## `IsSubscriber`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** `IsSubscriber` is a type predicate that checks if a given type claims to conform to the Subscriber concept. (It checks if the given type publically inherits `SubscriberBase`.)

**Example usage:**

```cpp
template <typename SubscriberT>
void TakeSubscriber(const SubscriberT &subscriber) {
  static_assert(
      IsSubscriber<SubscriberT>,
      "TakeSubscriber must be called with a Subscriber");

  // ...
}
```

**See also:** `IsPublisher`, `IsSubscription`


## `IsSubscription`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** `IsSubscription` is a type predicate that checks if a given type claims to conform to the Subscription concept. (It checks if the given type publically inherits `SubscriptionBase`.)

**Example usage:**

```cpp
template <typename SubscriptionT>
void TakeSubscription(const SubscriptionT &subscription) {
  static_assert(
      IsSubscription<SubscriptionT>,
      "TakeSubscription must be called with a Subscription");

  // ...
}
```

**See also:** `IsPublisher`, `IsSubscription`


## `Just(Value...)`

**Defined in:** [`rs/just.h`](../include/rs/just.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `a, b, ... -> Publisher[a, b, ...]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/just.html)

**Description:** Constructs a Publisher that emits the given value or values.

The provided values must be copyable. If they are not, `Start` can be used instead.

**Example usage:**

```cpp
auto one = Just(1);

// It's possible to create a stream with more than one value
auto one_and_two = Just(1, 2);
// It's possible to create a stream with more zero values; this is like Empty
auto empty = Just();

// The types of the values to emit may be different
auto different_types = one_hi = Just(1, "hi");
```

**See also:** `Empty`, `From`, `Start`


## `Last()`

**Defined in:** [`rs/last.h`](../include/rs/last.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#last), [ReactiveX](http://reactivex.io/documentation/operators/last.html)

**Description:** Takes a stream and returns a stream that emits only the last value in the input stream. If the input stream is empty, the returned stream fails with an `std::out_of_range` exception.

**Example usage:**

```cpp
auto fifteen = Pipe(
    Just(5, 10, 15),
    Last());
```

```cpp
// Fails with an std::out_of_range exception.
auto fail = Pipe(
    Empty(),
    Last());
```

**See also:** `First`, `ElementAt`


## `MakePublisher(Callback)`

**Defined in:** [`rs/publisher.h`](../include/rs/publisher.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `(Subscriber[a]&& -> Subscription[]) -> Publisher[a]`

**Description:** Helper function that can be used to create custom Publishers. Please note that creating a custom Publisher is considered an advanced use of the rs library. When doing so, you must ensure that your Publisher object conforms to the rules in the [rs specification](specification.md).

`MakePublisher` takes a functor, for example a lambda, that takes a Subscriber and returns a Subscription and uses that to create a Publisher. It is also possible, but sometimes more verbose, to create a custom Publisher type directly by defining a class that inherits `PublisherBase` and that has a `Subscribe` method.

**Example usage:**

```cpp
// An implementation of the Empty operator:
auto Empty() {
  return MakePublisher([](auto &&subscriber) {
    subscriber.OnComplete();
    return MakeSubscription();
  });
}
```


## `MakeSubscriber()`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `void -> Subscriber[a]`

**Description:** Creates a Subscriber that doesn't do anything. This is useful mostly in test code.

**Example usage:**

```cpp
auto my_dummy_subscriber = MakeSubscriber();
my_dummy_subscriber.OnNext(1);  // Does nothing
my_dummy_subscriber.OnNext("test");  // Does nothing
my_dummy_subscriber.OnComplete();  // Does nothing
```


## `MakeSubscriber(OnNext, OnError, OnComplete)`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `(a -> void), (std::exception_ptr&& -> void), (void -> void) -> Subscriber[a]`

**Description:** Creates a Subscriber from a set of callback functors. It is always possible to create your own Subscriber class by inheriting `SubscriberBase` and defining the `OnNext`, `OnComplete` and `OnError` methods, but this helper function is often more convenient.

Most application code requires few or no custom Subscribers. The most common cases where it is necessary to create your own is to terminate a Publisher to get its final result or to create a custom operator that cannot be built by combining operators that already exist.

Regardless of whether this helper is used or if you create your own Subscriber class, you must follow the [rules for Subscribers as defined in the rs specification](specification.md#2-subscriber-code).

**Example usage:**

```cpp
auto fib = Just(1, 1, 2, 3, 5);
// Subscribe to the values of fib
auto subscription = fib.Subscribe(MakeSubscriber(
    [](int value) {
      printf("Got value: %d\n", value);
    },
    [](std::exception_ptr &&error) {
      printf("Something went wrong\n");
    },
    [] {
      printf("The stream completed successfully\n");
    }));
subscription.Request(ElementCount::Unbounded());
```


## `MakeSubscriber(const std::shared_ptr<SubscriberType> &)`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `std::shared_ptr<Subscriber[a]> -> Subscriber[a]`

**Description:** When writing custom operators it is sometimes helpful to have one object that is both a Subscription and a Subscriber at the same time. Doing that directly is sometimes not possible because both the Subscriber and the Subscription has to be given/`std::move`d away, and you can't do that twice.

To work around that limitation, it's possible to create a proxy Subscriber that has a `std::shared_ptr` to the actual Subscriber object. This allows giving away an owning reference to both the Subscriber and the Subscription.

When using this, be careful with reference cycles that would cause memory leaks. There is also a `std::weak_ptr` variant of this class that can be used to avoid this issue.

This is an API for advanced use of the rs library, primarily for making it easier to write custom operators. Most applications will have no use for this function.


## `MakeSubscriber(const std::weak_ptr<SubscriberType> &)`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `std::weak_ptr<Subscriber[a]> -> Subscriber[a]`

**Description:** This is just like the `std::shared_ptr` version of the `MakeSubscriber` function, except that the returned Subscriber holds the inner Subscriber by `std::weak_ptr` rather than an owning `std::shared_ptr`. This can be useful to avoid reference cycles that cause memory leaks.


## `MakeSubscription()`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `void -> Subscription[]`

**Description:** Creates a Subscription that doesn't do anything. This is useful mostly in test code.

**Example usage:**

```cpp
auto my_dummy_subscription = MakeSubscription();
my_dummy_subscription.Request(ElementCount(1));  // Does nothing
my_dummy_subscription.Cancel();  // Does nothing
```


## `MakeSubscription(RequestCb, CancelCb)`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `(ElementCount -> void), (void -> void) -> Subscription[]`

**Description:** Creates a Subscription from a set of callback functors. It is always possible to create your own Subscription class by inheriting `SubscriptionBase` and defining the `Request` and `Cancel` method, but this helper function is often more convenient.

Creating custom Subscription objects is usually needed only for making custom operators that cannot be built by combining operators that already exist.

Regardless of whether this helper is used or if you create your own Subscription class, you must follow the [rules for Subscriptions as defined in the rs specification](specification.md#3-subscription-code).

**Example usage:**

```cpp
auto sub = MakeSubscription(
    [](ElementCount count) {
      printf("%lld values requested\n", count.Get());
    },
    []() {
      printf("The Subscription was cancelled\n");
    });
```


## `MakeSubscription(const std::shared_ptr<SubscriptionType> &)`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `std::shared_ptr<Subscription[]> -> Subscription[]`

**Description:** When writing custom operators it is sometimes helpful to have one object that is both a Subscription and a Subscriber at the same time. Doing that directly is sometimes not possible because both the Subscriber and the Subscription has to be given/`std::move`d away, and you can't do that twice.

To work around that limitation, it's possible to create a proxy Subscription that has a `std::shared_ptr` to the actual Subscription object. This allows giving away an owning reference to both the Subscriber and the Subscription.

When using this, be careful with reference cycles that would cause memory leaks.

This is an API for advanced use of the rs library, primarily for making it easier to write custom operators. Most applications will have no use for this function.


## `Map(Mapper)`

**Defined in:** [`rs/map.h`](../include/rs/map.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> b) -> (Publisher[a] -> Publisher[b])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#map), [ReactiveX](http://reactivex.io/documentation/operators/map.html)

**Decription:** Takes a Publisher and returns one that emits the same number of values, but each value is transformed by the provided mapper function. It is similar to the `map` function in functional programming (there are variations of it in [Javascript](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/map), [Java 8 Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html#map-java.util.function.Function-) and [Boost Ranges](http://www.boost.org/doc/libs/1_64_0/libs/iterator/doc/transform_iterator.html), for example).

**Example usage:**

```cpp
auto one_to_hundred_strings = Pipe(
    Range(1, 100),
    Map([](int num) {
      return std::to_string(num);
    }));
```


## `Max()`

**Defined in:** [`rs/max.h`](../include/rs/max.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#max), [ReactiveX](http://reactivex.io/documentation/operators/max.html)

**Description:** Takes a stream and returns a stream that emits only the biggest value in the input stream. `operator<` is used for comparison.

**Example usage:**

```cpp
auto biggest = Pipe(
    Just(1, 5, 2, 6, 4, 2, 6, 2, 5, 8, 2, 1),
    Max());
```

**See also:** `Min`, `Reduce`, `ReduceWithoutInitial`


## `Merge(Publisher...)`

**Defined in:** [`rs/merge.h`](../include/rs/merge.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `Publisher[a], Publisher[b], ... -> Publisher[a, b, ...]`

**External documentation:** [RxMarbles](http://rxmarbles.com/#merge), [ReactiveX](http://reactivex.io/documentation/operators/merge.html)

**Description:** Takes a number of streams and returns a stream that emits all of the values that the input streams emit, with no ordering between the streams (unlike `Concat`, which emits values from the streams one by one).

**Example usage:**

```cpp
auto six = Pipe(
    Merge(Just(2), Just(2), Just(2)),
    Sum());
```

**See also:** `Concat`, `Zip`


## `Min()`

**Defined in:** [`rs/min.h`](../include/rs/min.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#min), [ReactiveX](http://reactivex.io/documentation/operators/min.html)

**Description:** Takes a stream and returns a stream that emits only the smallest value in the input stream. `operator<` is used for comparison.

**Example usage:**

```cpp
auto smallest = Pipe(
    Just(1, 5, 2, 6, 4, 0, 6, 2, 5, 8, 2, 1),
    Min());
```


## `Never()`

**Defined in:** [`rs/never.h`](../include/rs/never.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `void -> Publisher[]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/empty-never-throw.html)

**Description:** Creates a Publisher that never emits any value and never finishes. Useful mostly for testing purposes.

**Example usage:**

```cpp
auto never = Never();
```

**See also:** `Empty`, `Throw`, `Just`


## `Pipe(Publisher, Operator...)`

**Defined in:** [`rs/pipe.h`](../include/rs/pipe.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `Publisher[a], (Publisher[a] -> Publisher[b]), (Publisher[b] -> Publisher[c]), ... -> Publisher[c]`

**Description:** When using rs, it is very common to combine multiple streams and operators, so it's important that doing so is concise and that the code is easy to read. `Pipe` is a helper function for that: `Pipe(a, b, c)` is basically the same as writing `c(b(a))` and `Pipe(a)` is basically the same as `a`

**Example usage:**

Without the `Pipe` operator, it is difficult to avoid writing tricky to read code such as:

```cpp
auto even = Filter([](int x) { return (x % 2) == 0; });
auto square = Map([](int x) { return x * x; });
auto sum = Sum();
auto sum_of_even_squares = sum(square(even(Range(1, 100))));
```

Many ReactiveX libraries such as RxCpp and RxJava make this convenient by making all operators methods on their `Publisher` type, to let the user write code along the lines of

```cpp
// This example does not actually work
auto sum_of_even_squares = Range(1, 100)
    .Filter([](int x) { return (x % 2) == 0; })
    .Map([](int x) { return x * x; })
    .Sum();
```

This code looks nice, but this design has the drawback of making it impossible for application code to add custom operators that are as convenient to use. rs avoids this by offering the `Pipe` helper function instead of a Publisher base class with all the built-in operators as methods:

```cpp
auto sum_of_even_squares = Pipe(
    Range(1, 100),
    Filter([](int x) { return (x % 2) == 0; }),
    Map([](int x) { return x * x; }),
    Sum());
```

This allows third-party code to add custom operators on Publishers that can be used exactly like the built-in operators. The rs built-ins receive no special treatment.

**See also:** `BuildPipe`


## `Publisher`

**Defined in:** [`rs/publisher.h`](../include/rs/publisher.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** As [described in the README](../README.md#the-absence-of-a-publisher-type), an rs Publisher is a C++ *concept* – like iterators in C++ – rather than a single concrete class. This permits aggressive compiler optimizations and some interesting API convenience features. However, it is sometimes useful to be able to give a name to any Publisher.

`Publisher` is an [*eraser type*](../README.md#eraser-types) that uses virtual method calls to be able to give a name to the Publisher type.

`Publisher` is a variadric template:

* `Publisher<>` can encapsulate any Publisher that emits no values (it can only finish or fail).
* `Publisher<T>` can encapsulate any Publisher that emits values of type `T`.
* `Publisher<T, U>` can encapsulate any Publisher that emits values of type `T` and `U`.
* and so on.

**Example usage:**

In the example below, `Publisher<int>` is used to be able to name the return type of `EvenSquares`, which makes it possible to place its definition in a `.cpp` implementation file, which would otherwise have been tricky:

```cpp
Publisher<int> EvenSquares(int n) {
  return Publisher<int>(Pipe(
      Range(1, n),
      Filter([](int x) { return (x % 2) == 0; }),
      Map([](int x) { return x * x; }),
      Sum()));
}
```

In the example below, `Publisher<int>` is used to get one type for two different Publisher types. Without `Publisher<int>` it would have been tricky to write the lambda in the example since it has to have only one return type:

```cpp
auto only_even = Pipe(
    Just(1, 2, 3, 4, 5),
    FlatMap([](int x) {
      if ((x % 2) == 0) {
        return Publisher<int>(Just(x));
      } else {
        return Publisher<int>(Empty());
      }
    }));
```

**See also:** `Subscriber`, `Subscription`


## `PublisherBase`

**Defined in:** [`rs/publisher.h`](../include/rs/publisher.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** One of the [requirements for Publisher types](specification.md#1-publisher-code) is that they must inherit `PublisherBase` to signify the intent of being a Publisher.

`PublisherBase` itself is a class that does not do anything at all; its sole purpose is to make it so that Publisher types have to declare that they want to be Publishers.

`PublisherBase` has a `protected` constructor. This makes it impossible to construct `PublisherBase` objects directly; only subclasses of it can be instantiated.

**See also:** `SubscriberBase`, `SubscriptionBase`


## `Range(Value, size_t)`

**Defined in:** [`rs/range.h`](../include/rs/range.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `a, size_t -> Publisher[Numberic]`, where `a` is any numeric type.

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/range.html)

**Description:** Constructs a Publisher that emits a range of values. The first parameter is the first value of the range, the second parameter is the number of values to emit.

**Example usage:**

```cpp
auto ten_to_twenty = Range(10, 10);
```

**See also:** `From`, `Just`, `Repeat`


## `Reduce(Accumulator, Reducer)`

**Defined in:** [`rs/reduce.h`](../include/rs/reduce.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `a, (a, b -> a) -> (Publisher[b] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#reduce), [ReactiveX](http://reactivex.io/documentation/operators/reduce.html)

**Description:** Reduces (this is called left fold in some languages and libraries) a stream of values into one single value. This is similar to `std::accumulate` and [the `reduce` method in Java 8 Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html#reduce-T-java.util.function.BinaryOperator-).

`Reduce` takes an initial accumulator value and a function that takes the accumulator and a value from the stream and combines them into one value. It works with empty streams.

`Reduce` works only with accumulator types that are copyable. For accumulator types that are noncopyable, use `ReduceGet`.

**Example usage:**

```cpp
// An implementation of the Sum operator that works with ints
auto Sum() {
  return Reduce(0, [](int accum, int value) {
    return accum + value;
  });
}
```

**See also:** `ReduceGet`, `ReduceWithoutInitial`


## `ReduceGet(MakeInitial, Reducer)`

**Defined in:** [`rs/reduce.h`](../include/rs/reduce.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(void -> a), (a, b -> a) -> (Publisher[b] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#reduce), [ReactiveX](http://reactivex.io/documentation/operators/reduce.html)

**Description:** `ReduceGet` is like `Reduce`, but it takes a function that creates the initial value rather than the initial value directly. This can be used when the initial value is expensive to create or when it is not copyable.

`ReduceGet` works with empty streams.

**Example usage:**

```cpp
// An implementation of the Sum operator that works with ints
auto Sum() {
  return ReduceGet([] { return 0; }, [](int accum, int value) {
    return accum + value;
  });
}
```

**See also:** `Reduce`, `ReduceWithoutInitial`


## `ReduceWithoutInitial<Accumulator>(Reducer)`

**Defined in:** [`rs/reduce.h`](../include/rs/reduce.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a, a -> a) -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#reduce), [ReactiveX](http://reactivex.io/documentation/operators/reduce.html)

**Description:** `ReduceWithoutInitial` is like `Reduce` except that instead of taking an initial accumulator value up-front it uses first value of the input stream.

`ReduceWithoutInitial` does not accept empty input streams; if the input stream finishes without emitting a value, the output stream fails with an `std::out_of_range` exception.

`Reduce` and `ReduceGet` allow the accumulator type to be different from the types of the input values, but `ReduceWithoutInitial` requires the accumulator to be of the same type as the input values.

**Example usage:**

```cpp
// An implementation of the Max operator that works with ints
auto Max() {
  return ReduceWithoutInitial<int>([](int accum, int value) {
    return std::max(accum, value);
  });
}
```

**See also:** `Reduce`, `ReduceGet`


## `Repeat(Value, size_t)`

**Defined in:** [`rs/repeat.h`](../include/rs/repeat.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `a, size_t -> Publisher[a]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/repeat.html)

**Description:** Construct a Publisher that emits a specific value a given number of times.

**Example usage:**

```cpp
auto hello_a_hundred_times = Repeat("hello", 100);
```


## `RequireRvalue`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Background:** This is a short introduction to universal references in C++. If you already know this, you can skip this background part.

When defining C++ function or method templates where a parameter type is inferred, the function will accept a value, an rvalue reference and an lvalue reference:

```cpp
template <typename T>
void UseValue(T &&t) {
  // ...
}

void UseString(std::string &&s) {
  // ...
}
```

In the example above, `T &&` is a [universal reference](https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers). This means that `UseValue` can be invoked with not just rvalue references but also lvalue references:

```cpp
UseString(std::string("hello"));  // Valid
UseValue(std::string("hello"));  // Valid

std::string a("a");
UseString(std::move(a));  // Valid
UseValue(std::move(a));  // Valid

std::string b("b");
UseString(b);  // NOT valid
UseValue(b);  // Also valid
```

The fact that `UseValue(b)` is valid while `UseString(b)` is not is a bit counterintuitive, but it is also quite useful because it allows [perfect forwarding](http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/).

**Description:** There are cases where it is useful to have functions or methods that take an rvalue reference to a deduced type, but because C++ uses the syntax for a deduced rvalue parameter type for universal references (that also accepts an lvalue reference), it's not straight forward to do so.

`RequireRvalue` is an `std::enable_if`-style template that can be used to require that a parameter that would otherwise be a universal reference is a non-const rvalue reference or a value (which behaves the same).

`RequireRvalue` is an API for advanced use of the rs library. It is mostly useful when creating custom templated operators.

In rs, `RequireRvalue` is used in the built-in implementations of the Subscriber concept, for the `OnNext` method. [`OnNext`'s parameter must be an rvalue reference](specification.md#2-subscriber-code), and by always enforcing that in `OnNext` certain type errors are caught earlier than if `RequireRvalue` would not have been used.

**Example usage:**

```cpp
template <typename T, class = RequireRvalue<T>>
void UseRvalue(T &&t) {
  // ...
}
```

```cpp
UseRvalue(std::string("hello"));  // Valid

std::string a("a");
UseRvalue(std::move(a));  // Valid

std::string b("b");
UseRvalue(b);  // NOT valid
```

**See also:** `SubscriberBase`


## `Scan(Accumulator, Mapper)`

**Defined in:** [`rs/scan.h`](../include/rs/scan.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `a, (a, b -> a) -> (Publisher[b] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#scan), [ReactiveX](http://reactivex.io/documentation/operators/scan.html)

**Description:** `Scan` is like `Map`, but the mapper function is given the previously emitted value as well. In order to have something when processing the first value, `Scan` takes the first "previously emitted value" as a parameter.

**Example usage:**

```cpp
// An operator that takes a Publisher of ints and returns a Publisher of ints
// where each output value is the sum of all previously seen values.
auto RunningSum() {
  return Scan(0, [](int accum, int val) { return accum + val; });
}
```

```cpp
// sums is a stream that emits 3, 5, 6  (it is 3, then 3+2, then 3+2+1)
auto sums = Pipe(
    Just(3, 2, 1),
    RunningSum());
```

**See also:** `Map`


## `Skip(size_t)`

**Defined in:** [`rs/skip.h`](../include/rs/skip.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `size_t -> (Publisher[a] -> Publisher[a])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#skip), [ReactiveX](http://reactivex.io/documentation/operators/skip.html)

**Description:** Takes a Publisher and returns one that behaves like it, except that the first `n` elements are dropped. If the stream ends before that many elements are emitted, the stream finishes without emitting any element (it is not an error).

**Example usage:**

```cpp
// two_three_four is a stream that emits 2, 3, 4
auto two_three_four = Pipe(
    Just(0, 1, 2, 3, 4),
    Skip(2));
```

```cpp
// empty is a stream of ints that emits no values
auto empty = Pipe(
    Just(0, 1, 2),
    Skip(10));
```

**See also:** `Filter`, `SkipWhile`, `Take`, `TakeWhile`


## `SkipWhile(Predicate)`

**Defined in:** [`rs/skip_while.h`](../include/rs/skip_while.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[a])`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/skipwhile.html)

**Description:** Takes a stream of values and returns a stream that has the same values in it except for the values that come before the first value for which the the predicate returns true; they are dropped.

**Example usage:**

```cpp
// first_positive_and_onward is a stream that emits 1, 0, -2, 1
auto first_positive_and_onward = Pipe(
    Just(-3, -2, -5, 1, 0, -2, 1),
    SkipWhile([](int x) { return x > 0; }));
```

**See also:** `Filter`, `Skip`, `Take`, `TakeWhile`


## `Some(Predicate)`

**Defined in:** [`rs/some.h`](../include/rs/some.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[bool])`

**External documentation:** [RxMarbles](http://rxmarbles.com/#some)

**Description:** Operator that returns a stream that emits exactly one value: `true` if any of the input elements match the given predicate, `false` otherwise. As soon as an element is encountered that matches the predicate, the boolean is emitted and the input stream is cancelled.

**Example usage:**

```cpp
auto input = Just(1, 2, 5, 1, -3, 1);
auto has_negative = Pipe(
    input,
    Some([](int value) { return value < 0; }));
```

**See also:** `All`


## `Splat(Functor)`

**Defined in:** [`rs/splat.h`](../include/rs/splat.h)

**Kind:** [Core Library API](#kind_core_library_api)

**[Type](#types):** `(a, b, ... -> c) -> (std::tuple<a, b, ...>&& -> c)`

**Description:** `Splat` is a helper function that can make it easier to access the individual elements of a tuple or a pair. What it does is similar to `std::tie`, but it is meant to be used in a different context. In cases where you would write:

```cpp
[](std::tuple<int, std::string> t) {
  auto &num = std::get<0>(t);
  auto &str = std::get<1>(t);
  ...
}
```

You could use `Splat` and instead write:

```cpp
Splat([](int num, std::string str) {
  ...
})
```

`Splat` works with all types for which `std::tuple_size` and `std::get` are defined: In addition to tuples it also works with `pair<>`s and `array<>`s.

**Example usage:**

`Splat` is particularly useful when dealing with streams that have tuples, for example because of `Zip`. Instead of writing

```cpp
Pipe(
    Zip(Just(1, 2), Just("a", "b"))
    Map([](std::tuple<int, std::string> &&tuple) {
      return std::get<1>(tuple) + " " + std::to_string(std::get<0<>(tuple)));
    }));
```

which does not read particularly nicely, `Splat` allows you to write

```cpp
Pipe(
    Zip(Just(1, 2), Just("a", "b"))
    Map(Splat([](int num, std::string str) {
      return str + " " + std::to_string(num);
    })));
```

**See also:** `Zip`


## `Start(CreateValue...)`

**Defined in:** [`rs/start.h`](../include/rs/start.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `(void -> a), (void -> b), ... -> Publisher[a, b, ...]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/start.html)

**Description:** Constructs a Publisher that emits one value for each of the provided functors. The provided functors are invoked and their return values are emitted to the output stream.

`Start` is particularly useful if the emitted values are not copyable, for example if they are `std::unique_ptr`s. If they are copyable, consider using `Just` instead; it has a simpler interface.

**Example usage:**

```cpp
auto one = Start([] { return 1; });

// It's possible to create a stream with more than one value
auto one_and_two = Start([] { return 1; }, [] { return 2; });
// It's possible to create a stream with more zero values; this is like Empty
auto empty = Start();

// The types of the values to emit may be different
auto different_types = one_hi = Start(
    [] { return 1; }, [] { return "hi"; });
```

**See also:** `Empty`, `From`, `Just`


## `StartWith(Value...)`

**Defined in:** [`rs/start_with.h`](../include/rs/start_with.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `a, b, ... -> (Publisher[c] -> Publisher[a, b, ..., c]`

**External documentation:** [RxMarbles](http://rxmarbles.com/#startWith), [ReactiveX](http://reactivex.io/documentation/operators/startwith.html)

**Description:** Prepends the provided values to a stream.

The parameters of `StartWith` must be copyable. If they are not, consider using `StartWithGet`.

**Example usage:**

```cpp
// stream is a stream that emits "hello", "world", 1, 2, 3
auto stream = Pipe(
    Just(1, 2, 3),
    StartWith("hello", "world"));
```

```cpp
// It is possible to call StartWith with no parameters. Then it is a no-op
// operator.
//
// stream is a stream that emits 1, 2, 3
auto stream = Pipe(
    Just(1, 2, 3),
    StartWith());
```

**See also:** `Concat`, `StartWithGet`


## `StartWithGet(MakeValue...)`

**Defined in:** [`rs/start_with.h`](../include/rs/start_with.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(void -> a), (void -> b), ... -> (Publisher[c] -> Publisher[a, b, ..., c]`

**External documentation:** [RxMarbles](http://rxmarbles.com/#startWith), [ReactiveX](http://reactivex.io/documentation/operators/startwith.html)

**Description:** `StartWithGet` is like `StartWith`, but it takes functions that create the values to be emitted rather than the values directly. This can be used when the values are expensive to craete or when they are noncopyable.

**Example usage:***

```cpp
// stream is a stream that emits "hello", "world", 1, 2, 3
auto stream = Pipe(
    Just(1, 2, 3),
    StartWithGet([] { return "hello"; }, [] { return "world"; }));
```

**See also:** `Concat`, `StartWith`


## `Subscriber`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** As [described in the README](../README.md#the-absence-of-a-publisher-type), an rs Subscriber is a C++ *concept* – like iterators in C++ – rather than a single concrete class. This permits aggressive compiler optimizations and some interesting API convenience features. However, it is sometimes useful to be able to give a name to any Subscriber.

`Subscriber` is an [*eraser type*](../README.md#eraser-types) that uses virtual method calls to be able to give a name to the Subscriber type.

`Subscriber` is a variadric template:

* `Subscriber<>` can encapsulate any Subscriber that can't receive any values (it can only finish or fail).
* `Subscriber<T>` can encapsulate any Subscriber that can receive values of type `T`.
* `Subscriber<T, U>` can encapsulate any Subscriber that can receive values of type `T` and `U`.
* and so on.

The `Subscriber` eraser type is intended for advanced use of the rs library. Storing and passing around `Subscriber` objects is not done much except in custom operators, and most custom operators do not type erase Subscribers.

The `Subscriber` eraser type is used internally in the `Publisher` eraser type implementation.

**Example usage:**

```cpp
Subscriber<int> subscriber = Subscriber<int>(MakeSubscriber(
    [](int value) {
      printf("Got value: %d\n", value);
    },
    [](std::exception_ptr &&error) {
      printf("Something went wrong\n");
    },
    [] {
      printf("The stream completed successfully\n");
    }));

subscriber.OnNext(42);
subscriber.OnComplete();
```

**See also:** `Publisher`, `Subscription`


## `SubscriberBase`

**Defined in:** [`rs/subscriber.h`](../include/rs/subscriber.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** One of the [requirements for Subscriber types](specification.md#2-subscriber-code) is that they must inherit `SubscriberBase` to signify the intent of being a Subscriber.

`SubscriberBase` itself is a class that does not do anything at all; its sole purpose is to make it so that Subscriber types have to declare that they want to be Subscribers.

`SubscriberBase` has a `protected` constructor. This makes it impossible to construct `SubscriberBase` objects directly; only subclasses of it can be instantiated.

**See also:** `PublisherBase`, `SubscriptionBase`


## `Subscription`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** As [described in the README](../README.md#the-absence-of-a-publisher-type), an rs Subscription is a C++ *concept* – like iterators in C++ – rather than a single concrete class. This permits aggressive compiler optimizations and some interesting API convenience features. However, it is sometimes useful to be able to give a name to any Subscription.

`Subscription` is an [*eraser type*](../README.md#eraser-types) that uses virtual method calls to be able to have one concrete type that can wrap any Subscription.

The `Subscription` eraser type is intended for advanced use of the rs library. It can be useful for classes that need to store Subscription objects as member variables: Type erasing the Subscription allows the header file with the class declaration to not leak information about exactly what is subscribed to.

**Example usage:**

```cpp
Subscription sub = Subscription(MakeSubscription(
    [](ElementCount count) {
      printf("%lld values requested\n", count.Get());
    },
    []() {
      printf("The Subscription was cancelled\n");
    }));
```

**See also:** `Publisher`, `Subscriber`


## `SubscriptionBase`

**Defined in:** [`rs/subscription.h`](../include/rs/subscription.h)

**Kind:** [Core Library API](#kind_core_library_api)

**Description:** One of the [requirements for Subscription types](specification.md#3-subscription-code) is that they must inherit `SubscriptionBase` to signify the intent of being a Subscription.

`SubscriptionBase` itself is a class that does not do anything at all; its sole purpose is to make it so that Subscription types have to declare that they want to be Subscriptions.

`SubscriptionBase` has a `protected` constructor. This makes it impossible to construct `SubscriptionBase` objects directly; only subclasses of it can be instantiated.

**See also:** `PublisherBase`, `SubscriberBase`


## `Sum()`

**Defined in:** [`rs/sum.h`](../include/rs/sum.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `void -> (Publisher[a] -> Publisher[SumType])`, where `SumType` is `int` by default and configurable by a template parameter to `Sum()`.

**External documentation:** [RxMarbles](http://rxmarbles.com/#sum), [ReactiveX](http://reactivex.io/documentation/operators/sum.html)

**Description:** Takes a stream and returns a stream that emits exactly one value: The sum of all values in the stream. `operator+` is used for adding values. By default the type of the emitted value is `int` but that can be customized.

**Example usage:**

```cpp
// sum is a Publisher that emits the sum of all numbers between 1 and 100
auto sum = Pipe(
    Range(1, 100),
    Sum());
```

```cpp
// It's possible to specify the type of the output value
auto sum = Pipe(
    Range(1, 100),
    Sum<double>());
```

**See also:** `Reduce`


## `Take(Count)`

**Defined in:** [`rs/take.h`](../include/rs/take.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `CountType -> (Publisher[a] -> Publisher[a])`, where `CountType` is a numeric type, for example `int`, `size_t` or `ElementCount`.

**External documentation:** [RxMarbles](http://rxmarbles.com/#take), [ReactiveX](http://reactivex.io/documentation/operators/take.html)

**Description:** Takes up to the given number of elements from the input stream. If the input stream completes earlier than that, the output stream also completes. When the target number of elements have been emitted, the input stream is cancelled.

**Example usage:**

```cpp
// sum is a Publisher that emits the sum of all numbers between 1 and 10
auto sum = Pipe(
    Range(1, 100),
    Take(10),
    Sum());
```

```cpp
// sum is a Publisher that emits 1
auto sum = Pipe(
    Just(1),
    Take(10),  // It is not an error if the input stream has too few elements.
    Sum());
```

**See also:** `TakeWhile`


## `TakeWhile(Predicate)`

**Defined in:** [`rs/take_while.h`](../include/rs/take_while.h)

**Kind:** [Operator Builder](#kind_operator_builder)

**[Type](#types):** `(a -> bool) -> (Publisher[a] -> Publisher[a])`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/takewhile.html)

**Description:** Takes elements from the input stream until the predicate returns `false` for an element: Then the output strem completes and the input stream is cancelled. If the input stream completes before the predicate returns `false`, the output stream also completes successfully.

**Example usage:**

```cpp
// positive is a Publisher that emits 3, 1, 5
auto positive = Pipe(
    Just(3, 1, 5, -1, 2, 4, 5),
    TakeWhile([](int x) { return x > 0; }));
```

```cpp
// sum is a Publisher that emits 3, 1, 5
auto sum = Pipe(
    Just(3, 1, 5),
    // It is not an error if the input stream completes before the predicate
    // returns false.
    TakeWhile([](int x) { return x > 0; }));
    Sum());
```

**See also:** `Take`


## `Throw(Exception)`

**Defined in:** [`rs/throw.h`](../include/rs/throw.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `ExceptionType -> Publisher[]`, where `ExceptionType` is anything that can be thrown as an exception in C++ except an `std::exception_ptr`.

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/empty-never-throw.html)

**Description:** Creates a Publisher that immediately fails with the given error. This variant of `Throw` wraps the given error by calling `std::make_exception_ptr` on its parameter.

**Example usage:**

```cpp
auto fail = Throw(std::runtime_error("fail"));
```

```cpp
// An implementation of the First operator
auto First() {
  // Use IfEmpty to ensure that there is at least one value.
  return BuildPipe(
      Take(1),
      IfEmpty(Throw(std::out_of_range("fail"))));
}
```

**See also:** `Empty`, `Just`, `Never`


## `Throw(const std::exception_ptr &)`

**Defined in:** [`rs/throw.h`](../include/rs/throw.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `std::exception_ptr -> Publisher[]`

**External documentation:** [ReactiveX](http://reactivex.io/documentation/operators/empty-never-throw.html)

**Description:** Creates a Publisher that immediately fails with the given error.

**Example usage:**

```cpp
auto fail = Throw(std::make_exception_ptr(std::runtime_error("fail")));
```

The documentation for the `Throw` operator overload that takes an exception object rather than an `std::exception_ptr` has more examples.

**See also:** `Empty`, `Just`, `Never`


## `Zip(Publisher...)`

**Defined in:** [`rs/zip.h`](../include/rs/zip.h)

**Kind:** [Operator](#kind_operator)

**[Type](#types):** `Publisher[a], Publisher[b], ... -> Publisher[std::tuple<a, b, ...>]`

**External documentation:** [RxMarbles](http://rxmarbles.com/#zip), [ReactiveX](http://reactivex.io/documentation/operators/zip.html)

**Description:** Takes a number of input Publishers and returns a Publisher that emits tuples of values from all the input streams. For example, zipping `[1, 2, 3]` with `[a, b, c]` emits `[(1, a), (2, b), (3, c)]`.

In code that performs server/client style RPC/REST communication, `Zip` is very handy when making two or more requests in parallel and then continue when all of the requests have finished.

Many functional programming languages and libraries has a version of this function that operates on lists that is also called `zip`, for example [Haskell](http://hackage.haskell.org/package/base-4.9.1.0/docs/Prelude.html#v:zip).

**Example usage:**

```cpp
// zipped is a Publisher that emits "a 1", "b 2"
auto zipped = Pipe(
    Zip(Just(1, 2), Just("a", "b"))
    Map([](std::tuple<int, std::string> &&tuple) {
      return std::get<1>(tuple) + " " + std::to_string(std::get<0<>(tuple)));
    }));
```

The example above can be written in a nicer way using `Splat`:

```cpp
// zipped is a Publisher that emits "a 1", "b 2"
auto zipped = Pipe(
    Zip(Just(1, 2), Just("a", "b"))
    Map(Splat([](int num, std::string str) {
      return str + " " + std::to_string(num);
    })));
```

A more real world-like use case:

```cpp
std::string RenderShoppingCart(
    const UserInfo &user_info,
    const std::vector<ShoppingCartItem> &cart_items) {
  return "...";
}

Publisher<std::string> RequestAndRenderShoppingCart(
    const Backend &backend,
    const std::string &user_id) {
  Publisher<UserInfo> user_info_request = backend.GetUserInfo(user_id);
  Publisher<std::vector<ShoppingCartItem>> cart_items_request =
      backend.GetShoppingCartItems(user_id);

  return Publisher<std::string>(Pipe(
      Zip(user_info_request, cart_items_request),
      Map(Splat(&RenderShoppingCart)));
}
```

**See also:** `Merge`, `Splat`


## Legend

### Kinds

* <em><a name="kind_operator">Operator</a></em>: Function that returns a Publisher.
* <em><a name="kind_operator_builder">Operator Builder</a></em>: Function that returns an Operator.
* <em><a name="kind_operator_builder_builder">Operator Builder Builder</a></em>: Function that returns an Operator Builder.
* <em><a name="kind_core_library_api">Core Library API</a></em>: A function or type that is part of the core of rs, not an operator.


### Types

Each operator described here has a section that describes its type. Because C++ does not have a syntax for concepts, these descrpitions have a made-up syntax:

* `Publisher[x]` is a C++ type that fulfills the requirements of the Publisher concept that publishes elements of type `x`.
* `Publisher[x, y]` is a Publisher that can emit elements of type `x` and `y`.
* `Subscriber[x]` is a C++ type that fulfills the requirements of the Subscriber concept that publishes elements of type `x`.
* `Subscription[]` is a C++ type that fulfills the requirements of the Subscription concept.
* `x -> y` is a function that takes `x` as a parameter and returns `y`.
* `x, y -> z` is a function with two parameters of type `x` and `y` and returns `z`.
* `bool`, `size_t` and `std::exception_ptr&&` refer to the C++ types with the same name.
* `void` is the C++ `void` type. It is also used here to denote functions that take no parameters: for example, `void -> bool` a function that takes no parameters and returns a `bool`.
* Letters such as `a`, `b` and `c` can represent any type. If a letter occurs more than once in a type declaration, all occurences refer to the same type.
