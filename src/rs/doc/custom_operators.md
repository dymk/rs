# Writing custom `rs` operators

The *rs* library is designed with a strong focus on making it easy to manipulate and combine streams in various ways. It does so by offering a broad palette of operators on them, such as `Map`, `ConcatMap`, `Filter`, `Merge` and so on. Using these operators is rather straight forward.

When using this library you might run into situations where the built-in operators are – or at least seem – insufficient. In such cases you might have to write your own custom operator. Doing so can unfortunately be quite tricky. This document attempts to explain the basics of writing custom operators as well as some gotchas that are important to know about.


## First, try to combine existing operators

Do you *really* need a custom operator? In many cases it's possible to create an operator simply by combining operators that already exist. Doing so is usually far simpler than writing a custom operator from scratch. Before writing a custom operator, have a look at [the reference documentation](reference.md) and see if you can think of ways to combine operators.

In fact, a lot of the operators that are provided by rs library are actually simply combinations of other more primitive operators:

* [`Sum`](../include/rs/sum.h) and [`IsEmpty`](../include/rs/is_empty.h) are simple applications of `Reduce`.
* [`StartWith` and `StartWithGet`](../include/rs/start_with.h) use `Concat` and either `Just` (`StartWith`) or `Start` (`StartWithGet`).
* [`Some`](../include/rs/some.h) combines `Filter`, `Take` and `Reduce`.
* [`Skip`](../include/rs/skip.h) and [`SkipWhile`](../include/rs/skip_while.h) use `Filter` with a stateful predicate.
* [`Scan`](../include/rs/scan.h) uses `Map` with a stateful mapper function.
* [`Range`](../include/rs/range.h) uses `From` with a custom STL-lookalike container type.
* [`Min`](../include/rs/min.h), [`Max`](../include/rs/max.h) and [`Last`](../include/rs/last.h) are thin layers on top of `ReduceWithoutInitial`.
* [`IfEmpty`](../include/rs/if_empty.h) uses `Map`, `Concat`, `Empty` and a shared `bool` state variable.
* The [`First`](../include/rs/first.h) with no parameters uses `Take`, `IfEmpty` and `Throw`. The `First` with a predicate parameter uses `First()` and `Filter`.
* [`ElementAt`](../include/rs/element_at.h) uses `Take` and `First` with a stateful predicate.
* [`DefaultIfEmpty`](../include/rs/default_if_empty.h) uses `IfEmpty` and `Just`.
* [`Contains`](../include/rs/contains.h) is built with `Some`.
* [`Concat`](../include/rs/concat.h) uses `ConcatMap` and `Just`.
* [`Average`](../include/rs/average.h) uses `Reduce` and `Map`.
* [`All`](../include/rs/all.h) uses `Filter`, `Take` and `Reduce`.

As you can see, a lot of things can be accomplished simply by combining other operators. Feel free to browse the code base and see how the built-in operators are written.

If you need to combine more than one operator to achieve what you need to do, `BuildPipe` can be quite useful; [`Some`](../include/rs/some.h) uses it, for example.

Another useful technique which can avoid the need for a fully custom operator is to to use stateful mapper and predicate functions. [`Skip`](../include/rs/skip.h), [`SkipWhile`](../include/rs/skip_while.h) and [`Scan`](../include/rs/scan.h) are examples of that.

If you find that it is impossible to do what you want with existing operators, or that combining existing operators does not perform well enough, it makes sense to start looking at writing a custom operator.


## Custom rs operators

**TL;DR:** When writing a custom rs operator, just make sure that the Publisher, Subscriber and Subscription objects that you create conform to [the specifiction](specification.md). However, it is not necessarily easy to understand what to do in practice from just reading the spec. For more detail, keep reading.

An "operator" in rs is a quite loosely defined concept. The only thing all operators have in common is that they create streams (objects that conform to the [Publisher concept](specification.md#1-publisher-code)).

A Publisher in rs is similar to a future or a promise: It represents a possibly asynchronous computation. In most future libraries, a future either finishes successfully with a value, or it fails. An rs Publisher emits zero or more values, and after that it may complete or fail. For more detail, see the [specification](specification.md#1-publisher-code).

In order to get a feeling for what Publishers can and must do, let's go through some progressively more complex examples.

* A trivial stream / Publisher
* An empty stream
* A failing stream
* Writing an operator
* A synchronous stream with a single element
* A map operator
* A sum operator
* Continuing from here


### A trivial stream / Publisher

The simplest possible stream in rs is a stream that never emits any elements and that never finishes. If you want a stream like this, you should use the built-in [`Never`](../include/rs/never.h) operator, but here we are interested in how to make our own.

One way to do it is like this:

```cpp
class DummySubscription : public Subscription {
 public:
  void Request(ElementCount count) {}
  void Cancel() {}
};

class NeverStream : public Publisher {
 public:
  template <typename Subscriber>
  auto Subscribe(Subscriber &&subscriber) const {
    static_assert(
        IsRvalue<SubscriberType>,
        "NeverStream was invoked with a non-rvalue parameter");
    static_assert(
        IsSubscriber<SubscriberType>,
        "NeverStream was invoked with a non-subscriber parameter");

    return DummySubscription();
  }
};

NeverStream never_stream;
```

Breaking it down:

* `never_stream` in the code snippet above is an object of type `NeverStream`, which is a type that conforms to the Publisher concept. This means that it is  a stream.
* `NeverStream` inherits `Publisher`. `Publisher` is an empty non-instantiable type: The only thing it does is that it states the intent of being a Publisher. This helps catching type errors earlier, simplifying compile time error messages.
* `NeverStream` has a `Subscribe` method that will be invoked each time someone starts to listen to the stream.
* The `Subscribe` method must be a template because it must accept any type that conforms to the Subscriber concept, and there is no one type that can be used for that.
* `NeverStream::Subscribe` uses `static_assert` statements to ensure that it is always invoked with an rvalue Subscriber. These are not strictly necessary but help provide easier to understand error messages if the method is used incorrectly.
* The `Subscribe` method returns a Subscription object that allows the entity that wants the data from the stream to request elements and to cancel it.

The example above can be written more concisely:

```cpp
auto never_stream = MakePublisher([](auto &&subscriber) {
  return MakeSubscription();
});
```

`MakeSubscription()` creates a no-op Subscription just like `DummySubscription` and `MakePublisher` avoids the rest of the boilerplate. All rs built-in operators are written using this shorter form; this inherits `Publisher`, does the proper `static_assert`s and also makes it easier to capture state in the publisher with the lambda expression.


### An empty stream

Now, let's look at something slightly more useful: how to create an empty stream (if you want a stream like this in your code, you should use the built-in [`Empty`](../include/rs/empty.h) operator):

```cpp
auto empty_stream = MakePublisher([](auto &&subscriber) {
  subscriber.OnComplete();
  return MakeSubscription();
});
```

* This is similar to the `never_stream` example above.
* When the stream is subscribed to, `empty_stream` immediately reports to the `subscriber` that the stream is complete.


### A failing stream

The code for a stream that fails is a similar to a stream that succeeds, the difference is that `OnError` is called on the Subscriber instead of `OnComplete` (if you want a stream like this in your code, you should use the built-in [`Throw`](../include/rs/throw.h) operator):

```cpp
auto failing_stream = MakePublisher([](auto &&subscriber) {
  subscriber.OnError(std::make_exception_ptr(std::runtime_error("fail")));
  return MakeSubscription();
});
```


### Writing an operator

An rs operator is simply a function that returns a Publisher. For example, here is the `Empty` operator:

```cpp
auto Empty() {
  return MakePublisher([](auto &&subscriber) {
    subscriber.OnComplete();
    return MakeSubscription();
  });
}
```

Being a function, an operator can take parameters:

```cpp
auto Throw(const std::exception_ptr &error) {
  return MakePublisher([error](auto &&subscriber) {
    subscriber.OnError(std::exception_ptr(error));
    return MakeSubscription();
  });
}
```

As you can see, there is nothing special with these functions: What makes them operators is that they return Publishers.


### A synchronous stream with a single element

TODO(peck): Write this section


### A map operator

TODO(peck): Write this section


### A sum operator

TODO(peck): Write this section


### Continuing from here

TODO(peck): Write this section



## Things to think about when writing custom operators

This section contains various tips and tricks and other things that are good to think about when writing custom operators.


### The absence of a `Processor` concept

The Java version of the Reactive Streams specification has a `Processor` interface. A `Processor` is simply a `Subscription` and a `Subscriber` in one object. `Processor` is a useful interface because lots of operators can be implemented as a class that conforms to the `Processor` interface.

rs does not have anything like that. This is not an accident. `Processor`s in Java Reactive Streams very often cause reference cycles: A `Processor` usually needs a reference to the `Subscription` that it is processing, which in turn usually has a `Subscriber` reference back to the `Processor` object. Java's garbage collector has no issues with reference cycles, but in C++ this is tricky to get to work right.

Early versions of the rs library had operator classes that were both a Subscriber and a Subscription at the same time and managed those objects with `shared_ptr` and it resulted in lots of memory leaks. Because of this, and for compile and runtime performance, the current rs library neither has a Processor concept nor does it use `shared_ptr` almost at all.

It is good to know why rs lacks a Processor concept because when writing custom rs operators you need to think *very* carefully about object ownership and lifetime. C++ is much less forgiving than Java in this regard, and it really shows here.


### Unit tests are your friend

rs operators tend to be quite low level constructs. If they have bugs, even bugs that only occur in rare corner cases, you will suffer. In order for rs to be any fun to use, the operators have to Just Work, and this applies to your own custom rs operators too.

One of the very best ways to make sure that an rs operator actually works is to write unit tests. It is worth writing *lots* of unit tests. I recommend aiming for *at least* 100% branch coverage, as well as making sure that the unit tests fail if any single line of code is commented out.

It is painful to have such thorough coverage, but there are so many subtle ways an rs operator can fail that I think the effort is worth it.


### Address Sanitizer is your friend

Even more so than in most C++ code, it is easy to write code that causes memory corruption. In particular, it is easy to write bugs that happen because an object is mutably borrowed out multiple times. The Rust programming language automatically detects and rejects such programs but C++ does not, and these bugs are often quite difficult to spot.

Address Sanitizer, in particular in combination with a good unit test suite, is quite good at detecting these issues. So if you do write custom rs operators, use it! It will save you lots of time.


### The dangers of borrowing

This section is about a particularly painful type of bug that can happen in C++ programs that is extra easy to run into while writing rs operators. Consider the following (buggy) code snippet:

```cpp
class Test {
 public:
  explicit Test(const std::string &str)
      : str_(new std::string(str)) {}

  void foo() {
    bar(*str_);
  }

 private:
  void bar(const std::string &s) {
    baz();
    printf("The string is: %s\n", s.c_str());
  }

  void baz() {
    str_.reset();
  }

  std::unique_ptr<std::string> str_;
};

int main() {
  Test t;
  t.foo();  // This accesses freed memory

  return 0;
}
```

The code above accesses freed memory and is likely to crash at the line that calls `printf`: When that code line is executed, the `baz()` call has freed the string that `s` refers to.

The tricky thing with this example is that looking at each individual part of this code example, it's not clear that anything is wrong. Only when looking at the code as a whole is the bug obvious. This is especially devious when this type of bug involves multiple classes.

The memory safe programming language Rust has an interesting stance on this issue: In Rust, the `foo()` method would not compile, because the call to `bar(*str_);` borrows out `*str_` twice: both the implicit mutable reference to `this`, which contains `*str_`, and a direct reference to `*str_`. It is difficult to prove that such simultaneous borrowing is safe, so Rust always disallows it.

When writing rs operators it is common to end up with multiple layers of Subscription objects that own a Subscriber that owns a Subscription and so on. If a Subscriber's `OnNext` method fails and subsequently chooses to cancel or overwrite an ancestor Subscription, it is easy to end up in a situation where an object that is `this` in a current stack frame gets destroyed, which is quite dangerous.

Unfortunately, I don't have very good advice for how to avoid this type of issue, except:

* If possible, avoid situations where a method has access to a mutable reference to an object that can be in the current call stack. If not, be very careful.
* Address Sanitizer is good at catching this type of bug early. It can also provide very good information about where the object was freed. Without this information, this type of bug can be extremely hard to understand.
* If Address Sanitizer complains about an access to freed memory, where the freed memory in question is the `this` pointer in a method, this might be what's going on.


### Different ways of having a C++ object

When writing rs operators, one has to pay extra attention to how different objects own and refer to each other. Unlike Java, which has one type of reference that is used for almost everything, C++ has multiple ways of having an object, each with different abilities and drawbacks.

The table below might help when selecting how to have an object in an rs operator.

* *Heap allocation*: Heap allocated objects are less cache friendly and less efficient than objects that are held directly.
* *Can be empty*: Sometimes, it is necessary to be able to have no object.
* *Supports polymorphism*: *Yes* if the holder of the object is capable of knowing only some superclass of the object and not the concrete type.
* *Multiple owners*: *Yes* if ownership can be shared.
* *Can have weak pointers*: *Yes* if it's possible to refer to the object without owning it / keeping it alive.

| Type                                    | Heap allocation | Can be empty | Supports polymorphism | Multiple owners | Can have weak pointers |
| --------------------------------------- | --------------- | ------------ | --------------------- | --------------- | ---------------------- |
| `T`                                     | No              | No           | No                    | No              | No                     |
| `std::optional<T>`                      | No              | Yes          | No                    | No              | No                     |
| `std::unique_ptr<T>`                    | Yes             | Yes          | Yes                   | No              | No                     |
| `std::shared_ptr<T>`/`std::weak_ptr<T>` | Yes             | Yes          | Yes                   | Yes             | Yes                    |
| `WeakReferee<T>`/`WeakReference<T>`     | No              | No           | `WeakReference` does  | No              | Up to one              |

These ownership types can be combined. For example, it is possible to have more than one `WeakReference` to an object by wrapping `WeakReferee`s inside each other. The number has to be decided compile time though.
