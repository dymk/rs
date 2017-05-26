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

#include <type_traits>

#include <grpc++/grpc++.h>
#include <rxcpp/rx.hpp>

#include "grpc_error.h"
#include "rx_grpc_identity_transform.h"
#include "rx_grpc_tag.h"
#include "stream_traits.h"

namespace shk {
namespace detail {

template <
    typename Transform,
    typename ResponseType>
void handleUnaryResponse(
    bool success,
    const grpc::Status &status,
    ResponseType &&response,
    rxcpp::subscriber<typename decltype(Transform::wrap(
        std::declval<ResponseType>()))::first_type> *subscriber) {
  if (!success) {
    subscriber->on_error(std::make_exception_ptr(GrpcError(grpc::Status(
        grpc::UNKNOWN, "The request was interrupted"))));
  } else if (status.ok()) {
    auto wrapped = Transform::wrap(std::move(response));
    if (wrapped.second.ok()) {
      subscriber->on_next(std::move(wrapped.first));
      subscriber->on_completed();
    } else {
      subscriber->on_error(
          std::make_exception_ptr(GrpcError(wrapped.second)));
    }
  } else {
    subscriber->on_error(std::make_exception_ptr(GrpcError(status)));
  }
}

template <
    typename Reader,
    typename ResponseType,
    typename TransformedRequestType,
    typename Transform>
class RxGrpcClientInvocation;

/**
 * Unary client RPC.
 */
template <
    typename ResponseType,
    typename TransformedRequestType,
    typename Transform>
class RxGrpcClientInvocation<
    grpc::ClientAsyncResponseReader<ResponseType>,
    ResponseType,
    TransformedRequestType,
    Transform> : public RxGrpcTag {
 public:
  using TransformedResponseType = typename decltype(
      Transform::wrap(std::declval<ResponseType>()))::first_type;

  RxGrpcClientInvocation(
      const TransformedRequestType &request,
      rxcpp::subscriber<TransformedResponseType> &&subscriber)
      : _request(request),
        _subscriber(std::move(subscriber)) {}

  void operator()(bool success) override {
    handleUnaryResponse<Transform>(
        success, _status, std::move(_response), &_subscriber);
    delete this;
  }

  template <typename Stub, typename RequestType>
  void invoke(
      std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          const RequestType &request,
          grpc::CompletionQueue *cq),
      Stub *stub,
      grpc::CompletionQueue *cq) {
    auto stream = (stub->*invoke)(&_context, Transform::unwrap(_request), cq);
    stream->Finish(&_response, &_status, this);
  }

 private:
  static_assert(
      !std::is_reference<TransformedRequestType>::value,
      "Request type must be held by value");
  TransformedRequestType _request;
  grpc::ClientContext _context;
  ResponseType _response;
  rxcpp::subscriber<TransformedResponseType> _subscriber;
  grpc::Status _status;
};

/**
 * Server streaming.
 */
template <
    typename ResponseType,
    typename TransformedRequestType,
    typename Transform>
class RxGrpcClientInvocation<
    grpc::ClientAsyncReader<ResponseType>,
    ResponseType,
    TransformedRequestType,
    Transform> : public RxGrpcTag {
 public:
  using TransformedResponseType = typename decltype(
      Transform::wrap(std::declval<ResponseType>()))::first_type;

  RxGrpcClientInvocation(
      const TransformedRequestType &request,
      rxcpp::subscriber<TransformedResponseType> &&subscriber)
      : _request(request),
        _subscriber(std::move(subscriber)) {}

  void operator()(bool success) override {
    switch (_state) {
      case State::INIT: {
        _state = State::READING_RESPONSE;
        _stream->Read(&_response, this);
        break;
      }
      case State::READING_RESPONSE: {
        if (!success) {
          // We have reached the end of the stream.
          _state = State::FINISHING;
          _stream->Finish(&_status, this);
        } else {
          auto wrapped = Transform::wrap(std::move(_response));
          if (wrapped.second.ok()) {
            _subscriber.on_next(std::move(wrapped.first));
            _stream->Read(&_response, this);
          } else {
            _subscriber.on_error(
                std::make_exception_ptr(GrpcError(wrapped.second)));
            _state = State::READ_FAILURE;
            _context.TryCancel();
            _stream->Finish(&_status, this);
          }
        }
        break;
      }
      case State::FINISHING: {
        if (_status.ok()) {
          _subscriber.on_completed();
        } else {
          _subscriber.on_error(std::make_exception_ptr(GrpcError(_status)));
        }
        delete this;
        break;
      }
      case State::READ_FAILURE: {
        delete this;
        break;
      }
    }
  }

  template <typename Stub, typename RequestType>
  void invoke(
      std::unique_ptr<grpc::ClientAsyncReader<ResponseType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          const RequestType &request,
          grpc::CompletionQueue *cq,
          void *tag),
      Stub *stub,
      grpc::CompletionQueue *cq) {
    _stream = (stub->*invoke)(&_context, Transform::unwrap(_request), cq, this);
  }

 private:
  enum class State {
    INIT,
    READING_RESPONSE,
    FINISHING,
    READ_FAILURE
  };

  static_assert(
      !std::is_reference<TransformedRequestType>::value,
      "Request type must be held by value");
  TransformedRequestType _request;
  grpc::ClientContext _context;

  State _state = State::INIT;
  ResponseType _response;
  rxcpp::subscriber<TransformedResponseType> _subscriber;
  grpc::Status _status;
  std::unique_ptr<grpc::ClientAsyncReader<ResponseType>> _stream;
};

/**
 * Client streaming.
 *
 * gRPC supports the use case that the client streams messages, and that the
 * server responds in the middle of the message stream. This implementation only
 * supports reading the response after the client message stream is closed.
 */
template <
    typename RequestType,
    typename ResponseType,
    typename TransformedRequestType,
    typename Transform>
class RxGrpcClientInvocation<
    grpc::ClientAsyncWriter<RequestType>,
    ResponseType,
    TransformedRequestType,
    Transform> : public RxGrpcTag {
 public:
  using TransformedResponseType = typename decltype(
      Transform::wrap(std::declval<ResponseType>()))::first_type;

 public:
  template <typename Observable>
  RxGrpcClientInvocation(
      const Observable &requests,
      rxcpp::subscriber<TransformedResponseType> &&subscriber)
      : _requests(requests.as_dynamic()),
        _subscriber(std::move(subscriber)) {
    static_assert(
        rxcpp::is_observable<Observable>::value,
        "First parameter must be an observable");
  }

  void operator()(bool success) override {
    if (_sent_final_request) {
      if (_request_stream_error) {
        _subscriber.on_error(_request_stream_error);
      } else {
        handleUnaryResponse<Transform>(
            success, _status, std::move(_response), &_subscriber);
      }
      delete this;
    } else {
      if (success) {
        _operation_in_progress = false;
        runEnqueuedOperation();
      } else {
        // This happens when the runloop is shutting down.
        handleUnaryResponse<Transform>(
            success, _status, std::move(_response), &_subscriber);
        delete this;
      }
    }
  }

  template <typename Stub>
  void invoke(
      std::unique_ptr<grpc::ClientAsyncWriter<RequestType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          ResponseType *response,
          grpc::CompletionQueue *cq,
          void *tag),
      Stub *stub,
      grpc::CompletionQueue *cq) {
    _stream = (stub->*invoke)(&_context, &_response, cq, this);
    _operation_in_progress = true;

    _requests.subscribe(
        [this](TransformedRequestType request) {
          _enqueued_requests.emplace_back(std::move(request));
          runEnqueuedOperation();
        },
        [this](const std::exception_ptr &error) {
          _request_stream_error = error;
          _context.TryCancel();
          _stream->Finish(&_status, this);
          _sent_final_request = true;
        },
        [this]() {
          _enqueued_writes_done = true;
          runEnqueuedOperation();
        });
  }

 private:
  void runEnqueuedOperation() {
    if (_operation_in_progress) {
      return;
    }
    if (!_enqueued_requests.empty()) {
      _operation_in_progress = true;
      _stream->Write(
          Transform::unwrap(std::move(_enqueued_requests.front())), this);
      _enqueued_requests.pop_front();
    } else if (_enqueued_writes_done) {
      _enqueued_writes_done = false;
      _enqueued_finish = true;
      _operation_in_progress = true;
      _stream->WritesDone(this);
    } else if (_enqueued_finish) {
      _enqueued_finish = false;
      _operation_in_progress = true;

      // Must be done before the call to Finish because it's not safe to do
      // anything after that call; gRPC could invoke the callback immediately
      // on another thread, which could delete this.
      _sent_final_request = true;

      _stream->Finish(&_status, this);
    }
  }

  rxcpp::observable<TransformedRequestType> _requests;
  ResponseType _response;
  std::unique_ptr<grpc::ClientAsyncWriter<RequestType>> _stream;
  grpc::ClientContext _context;
  rxcpp::subscriber<TransformedResponseType> _subscriber;

  std::exception_ptr _request_stream_error;
  bool _sent_final_request = false;
  bool _operation_in_progress = false;

  // Because we don't have backpressure we need an unbounded buffer here :-(
  std::deque<TransformedRequestType> _enqueued_requests;
  bool _enqueued_writes_done = false;
  bool _enqueued_finish = false;
  grpc::Status _status;
};

/**
 * For server requests with a non-streaming response.
 */
template <
    typename Service,
    typename RequestType,
    // grpc::ServerAsyncResponseWriter<ResponseType> or
    // grpc::ServerAsyncWriter<ResponseType>
    typename Stream>
using RequestMethod = void (Service::*)(
    grpc::ServerContext *context,
    RequestType *request,
    Stream *stream,
    grpc::CompletionQueue *new_call_cq,
    grpc::ServerCompletionQueue *notification_cq,
    void *tag);

/**
 * For server requests with a streaming response.
 */
template <
    typename Service,
    // grpc::ServerAsyncReader<ResponseType, RequestType> or
    // grpc::ServerAsyncReaderWriter<ResponseType, RequestType>
    typename Stream>
using StreamingRequestMethod = void (Service::*)(
    grpc::ServerContext *context,
    Stream *stream,
    grpc::CompletionQueue *new_call_cq,
    grpc::ServerCompletionQueue *notification_cq,
    void *tag);

/**
 * Group of typedefs related to a server-side invocation, to avoid having to
 * pass around tons and tons of template parameters everywhere, and to have
 * something to do partial template specialization on to handle non-streaming
 * vs uni-streaming vs bidi streaming stuff.
 */
template <
    // grpc::ServerAsyncResponseWriter<ResponseType> (non-streaming) or
    // grpc::ServerAsyncWriter<ResponseType> (streaming response) or
    // grpc::ServerAsyncReader<ResponseType, RequestType> (streaming request) or
    // grpc::ServerAsyncReaderWriter<ResponseType, RequestType> (bidi streaming)
    typename StreamType,
    // Generated service class
    typename ServiceType,
    typename ResponseType,
    typename RequestType,
    typename TransformType,
    typename Callback>
class ServerCallTraits {
 public:
  using Stream = StreamType;
  using Service = ServiceType;
  using Response = ResponseType;
  using Request = RequestType;
  using Transform = TransformType;


  using TransformedRequest =
      typename decltype(
          Transform::wrap(std::declval<RequestType>()))::first_type;

 private:
  /**
   * The type of the parameter that the request handler callback takes. If it is
   * a streaming request, it's an observable, otherwise it's an object directly.
   */
  using CallbackParamType = typename std::conditional<
      StreamTraits<Stream>::kRequestStreaming,
      rxcpp::observable<TransformedRequest>,
      TransformedRequest>::type;

  using ResponseObservable =
      decltype(std::declval<Callback>()(std::declval<CallbackParamType>()));

 public:
  using TransformedResponse = typename ResponseObservable::value_type;

  using Method = typename std::conditional<
      StreamTraits<Stream>::kRequestStreaming,
      StreamingRequestMethod<Service, Stream>,
      RequestMethod<Service, RequestType, Stream>>::type;
};


template <typename Stream, typename ServerCallTraits, typename Callback>
class RxGrpcServerInvocation;

/**
 * Unary server RPC.
 */
template <typename ResponseType, typename ServerCallTraits, typename Callback>
class RxGrpcServerInvocation<
    grpc::ServerAsyncResponseWriter<ResponseType>,
    ServerCallTraits,
    Callback> : public RxGrpcTag {
  using Stream = grpc::ServerAsyncResponseWriter<ResponseType>;
  using Service = typename ServerCallTraits::Service;
  using Transform = typename ServerCallTraits::Transform;
  using TransformedRequest = typename ServerCallTraits::TransformedRequest;
  using TransformedResponse = typename ServerCallTraits::TransformedResponse;
  using Method = typename ServerCallTraits::Method;

 public:
  static void request(
      GrpcErrorHandler error_handler,
      Method method,
      Callback &&callback,
      Service *service,
      grpc::ServerCompletionQueue *cq) {
    auto invocation = new RxGrpcServerInvocation(
        error_handler, method, std::move(callback), service, cq);

    (service->*method)(
        &invocation->_context,
        &invocation->_request,
        &invocation->_stream,
        cq,
        cq,
        invocation);
  }

  void operator()(bool success) {
    if (!success) {
      // This happens when the server is shutting down.
      delete this;
      return;
    }

    if (_awaiting_request) {
      // The server has just received a request. Handle it.

      auto wrapped_request = ServerCallTraits::Transform::wrap(
          std::move(_request));
      auto values = wrapped_request.second.ok() ?
          _callback(std::move(wrapped_request.first)).as_dynamic() :
          rxcpp::observable<>::error<TransformedResponse>(
              GrpcError(wrapped_request.second)).as_dynamic();

      // Request the a new request, so that the server is always waiting for
      // one. This is done after the callback (because this steals it) but
      // before the subscribe call because that could tell gRPC to respond,
      // after which it's not safe to do anything with `this` anymore.
      issueNewServerRequest(std::move(_callback));

      _awaiting_request = false;

      values.subscribe(
          [this](TransformedResponse response) {
            _response = std::move(response);
          },
          [this](const std::exception_ptr &error) {
            _stream.FinishWithError(exceptionToStatus(error), this);
          },
          [this]() {
            _stream.Finish(Transform::unwrap(_response), grpc::Status::OK, this);
          });
    } else {
      // The server has now successfully sent a response. Clean up.
      delete this;
    }
  }

 private:
  RxGrpcServerInvocation(
      GrpcErrorHandler error_handler,
      Method method,
      Callback &&callback,
      Service *service,
      grpc::ServerCompletionQueue *cq)
      : _error_handler(error_handler),
        _method(method),
        _callback(std::move(callback)),
        _service(*service),
        _cq(*cq),
        _stream(&_context) {}

  void issueNewServerRequest(Callback &&callback) {
    // Take callback as an rvalue parameter to make it obvious that we steal it.
    request(
        _error_handler,
        _method,
        std::move(callback),  // Reuse the callback functor, don't copy
        &_service,
        &_cq);
  }

  bool _awaiting_request = true;
  GrpcErrorHandler _error_handler;
  Method _method;
  Callback _callback;
  Service &_service;
  grpc::ServerCompletionQueue &_cq;
  grpc::ServerContext _context;
  typename ServerCallTraits::Request _request;
  Stream _stream;
  TransformedResponse _response;
};

/**
 * Server streaming.
 */
template <typename ResponseType, typename ServerCallTraits, typename Callback>
class RxGrpcServerInvocation<
    grpc::ServerAsyncWriter<ResponseType>,
    ServerCallTraits,
    Callback> : public RxGrpcTag {
  using Stream = grpc::ServerAsyncWriter<ResponseType>;
  using Service = typename ServerCallTraits::Service;
  using Transform = typename ServerCallTraits::Transform;
  using TransformedRequest = typename ServerCallTraits::TransformedRequest;
  using TransformedResponse = typename ServerCallTraits::TransformedResponse;
  using Method = typename ServerCallTraits::Method;

 public:
  static void request(
      GrpcErrorHandler error_handler,
      Method method,
      Callback &&callback,
      Service *service,
      grpc::ServerCompletionQueue *cq) {
    auto invocation = new RxGrpcServerInvocation(
        error_handler, method, std::move(callback), service, cq);

    (service->*method)(
        &invocation->_context,
        &invocation->_request,
        &invocation->_stream,
        cq,
        cq,
        invocation);
  }

  void operator()(bool success) {
    if (!success) {
      // This happens when the server is shutting down.
      delete this;
      return;
    }

    switch (_state) {
      case State::AWAITING_REQUEST: {
        // The server has just received a request. Handle it.
        _state = State::AWAITING_RESPONSE;

        auto wrapped_request = ServerCallTraits::Transform::wrap(
            std::move(_request));
        auto values = wrapped_request.second.ok() ?
            _callback(std::move(wrapped_request.first)).as_dynamic() :
            rxcpp::observable<>::error<TransformedResponse>(
                GrpcError(wrapped_request.second)).as_dynamic();

        // Request the a new request, so that the server is always waiting for
        // one. This is done after the callback (because this steals it) but
        // before the subscribe call because that could tell gRPC to respond,
        // after which it's not safe to do anything with `this` anymore.
        issueNewServerRequest(std::move(_callback));

        values.subscribe(
            [this](TransformedResponse response) {
              _enqueued_responses.emplace_back(std::move(response));
              runEnqueuedOperation();
            },
            [this](const std::exception_ptr &error) {
              _enqueued_finish_status = exceptionToStatus(error);
              _enqueued_finish = true;
              runEnqueuedOperation();
            },
            [this]() {
              _enqueued_finish_status = grpc::Status::OK;
              _enqueued_finish = true;
              runEnqueuedOperation();
            });

        break;
      }
      case State::AWAITING_RESPONSE:
      case State::SENDING_RESPONSE: {
        _state = State::AWAITING_RESPONSE;
        runEnqueuedOperation();
        break;
      }
      case State::SENT_FINAL_RESPONSE: {
        delete this;
        break;
      }
    }
  }

 private:
  enum class State {
    AWAITING_REQUEST,
    AWAITING_RESPONSE,
    SENDING_RESPONSE,
    SENT_FINAL_RESPONSE
  };

  RxGrpcServerInvocation(
      GrpcErrorHandler error_handler,
      Method method,
      Callback &&callback,
      Service *service,
      grpc::ServerCompletionQueue *cq)
      : _error_handler(error_handler),
        _method(method),
        _callback(std::move(callback)),
        _service(*service),
        _cq(*cq),
        _stream(&_context) {}

  void issueNewServerRequest(Callback &&callback) {
    // Take callback as an rvalue parameter to make it obvious that we steal it.
    request(
        _error_handler,
        _method,
        std::move(callback),  // Reuse the callback functor, don't copy
        &_service,
        &_cq);
  }

  void runEnqueuedOperation() {
    if (_state != State::AWAITING_RESPONSE) {
      return;
    }
    if (!_enqueued_responses.empty()) {
      _state = State::SENDING_RESPONSE;
      _stream.Write(
          Transform::unwrap(std::move(_enqueued_responses.front())), this);
      _enqueued_responses.pop_front();
    } else if (_enqueued_finish) {
      _enqueued_finish = false;
      _state = State::SENT_FINAL_RESPONSE;
      _stream.Finish(_enqueued_finish_status, this);
    }
  }

  State _state = State::AWAITING_REQUEST;
  bool _enqueued_finish = false;
  grpc::Status _enqueued_finish_status;
  std::deque<TransformedResponse> _enqueued_responses;

  GrpcErrorHandler _error_handler;
  Method _method;
  Callback _callback;
  Service &_service;
  grpc::ServerCompletionQueue &_cq;
  grpc::ServerContext _context;
  typename ServerCallTraits::Request _request;
  Stream _stream;
};

/**
 * Client streaming.
 */
template <
    typename ResponseType,
    typename RequestType,
    typename ServerCallTraits,
    typename Callback>
class RxGrpcServerInvocation<
    grpc::ServerAsyncReader<ResponseType, RequestType>,
    ServerCallTraits,
    Callback> : public RxGrpcTag {
  using Stream = grpc::ServerAsyncReader<ResponseType, RequestType>;
  using Service = typename ServerCallTraits::Service;
  using Transform = typename ServerCallTraits::Transform;
  using TransformedRequest = typename ServerCallTraits::TransformedRequest;
  using TransformedResponse = typename ServerCallTraits::TransformedResponse;
  using Method = typename ServerCallTraits::Method;

 public:
  static void request(
      GrpcErrorHandler error_handler,
      Method method,
      Callback &&callback,
      Service *service,
      grpc::ServerCompletionQueue *cq) {
    auto invocation = new RxGrpcServerInvocation(
        error_handler, method, service, cq);

    auto response = callback(rxcpp::observable<>::create<TransformedRequest>([
        invocation, error_handler, method, service, cq,
        callback=std::move(callback)](
            rxcpp::subscriber<TransformedRequest> subscriber) {

      if (invocation->_subscriber) {
        throw std::logic_error(
            "Can't subscribe to this observable more than once");
      }
      invocation->_subscriber.reset(
          new rxcpp::subscriber<TransformedRequest>(std::move(subscriber)));
      invocation->_callback.reset(
          new Callback(std::move(callback)));


      (service->*method)(
          &invocation->_context,
          &invocation->_reader,
          cq,
          cq,
          invocation);
    }));

    static_assert(
        rxcpp::is_observable<decltype(response)>::value,
        "Callback return type must be observable");
    response.subscribe(
        [invocation](TransformedResponse response) {
          invocation->_response = std::move(response);
        },
        [invocation](const std::exception_ptr &error) {
          invocation->_response_error = error;
          invocation->_has_response = true;
          invocation->trySendResponse();
        },
        [invocation]() {
          invocation->_has_response = true;
          invocation->trySendResponse();
        });
  }

  void operator()(bool success) {
    switch (_state) {
      case State::INIT: {
        if (!success) {
          delete this;
        } else {
          _state = State::REQUESTED_DATA;
          _reader.Read(&_request, this);
        }
        break;
      }
      case State::REQUESTED_DATA: {
        if (_callback) {
          // Request the a new request, so that the server is always waiting for
          // one.
          issueNewServerRequest(std::move(_callback));
        }

        if (success) {
          auto wrapped = Transform::wrap(std::move(_request));
          if (wrapped.second.ok()) {
            _subscriber->on_next(std::move(wrapped.first));
            _reader.Read(&_request, this);
          } else {
            _subscriber->on_error(
                std::make_exception_ptr(GrpcError(wrapped.second)));

            _state = State::SENT_RESPONSE;
            _reader.FinishWithError(wrapped.second, this);
          }
        } else {
          // The client has stopped sending requests.
          _subscriber->on_completed();
          _state = State::STREAM_ENDED;
          trySendResponse();
        }
        break;
      }
      case State::STREAM_ENDED: {
        abort();  // Should not get here
        break;
      }
      case State::SENT_RESPONSE: {
        // success == false implies that the server is shutting down. It doesn't
        // change what needs to be done here.
        delete this;
        break;
      }
    }
  }

 private:
  enum class State {
    INIT,
    REQUESTED_DATA,
    STREAM_ENDED,
    SENT_RESPONSE
  };

  RxGrpcServerInvocation(
      GrpcErrorHandler error_handler,
      Method method,
      Service *service,
      grpc::ServerCompletionQueue *cq)
      : _error_handler(error_handler),
        _method(method),
        _service(*service),
        _cq(*cq),
        _reader(&_context) {}

  void issueNewServerRequest(std::unique_ptr<Callback> &&callback) {
    // Take callback as an rvalue parameter to make it obvious that we steal it.
    request(
        _error_handler,
        _method,
        std::move(*callback),  // Reuse the callback functor, don't copy
        &_service,
        &_cq);
  }

  void trySendResponse() {
    if (_has_response && _state == State::STREAM_ENDED) {
      _state = State::SENT_RESPONSE;
      _reader.Finish(
          Transform::unwrap(_response),
          exceptionToStatus(_response_error),
          this);
    }
  }

  std::unique_ptr<rxcpp::subscriber<TransformedRequest>> _subscriber;
  State _state = State::INIT;
  GrpcErrorHandler _error_handler;
  Method _method;
  std::unique_ptr<Callback> _callback;
  Service &_service;
  grpc::ServerCompletionQueue &_cq;
  grpc::ServerContext _context;
  typename ServerCallTraits::Request _request;
  grpc::ServerAsyncReader<ResponseType, RequestType> _reader;

  TransformedResponse _response;
  std::exception_ptr _response_error;
  bool _has_response;
};

class InvocationRequester {
 public:
  virtual ~InvocationRequester() = default;

  virtual void requestInvocation(
      GrpcErrorHandler error_handler,
      grpc::ServerCompletionQueue *cq) = 0;
};

template <
    // RequestMethod<Service, RequestType, Stream> or
    // StreamingRequestMethod<Service, Stream>
    typename Method,
    typename ServerCallTraits,
    typename Callback>
class RxGrpcServerInvocationRequester : public InvocationRequester {
  using Service = typename ServerCallTraits::Service;

 public:
  RxGrpcServerInvocationRequester(
      Method method, Callback &&callback, Service *service)
      : _method(method), _callback(std::move(callback)), _service(*service) {}

  void requestInvocation(
      GrpcErrorHandler error_handler,
      grpc::ServerCompletionQueue *cq) override {
    using ServerInvocation = RxGrpcServerInvocation<
        typename ServerCallTraits::Stream,
        ServerCallTraits,
        Callback>;
    ServerInvocation::request(
        error_handler, _method, Callback(_callback), &_service, cq);
  }

 private:
  Method _method;
  Callback _callback;
  Service &_service;
};

}  // namespace detail

template <typename Stub, typename Transform>
class RxGrpcServiceClient {
 public:
  RxGrpcServiceClient(std::unique_ptr<Stub> &&stub, grpc::CompletionQueue *cq)
      : _stub(std::move(stub)), _cq(*cq) {}

  /**
   * Non-stream response.
   */
  template <typename ResponseType, typename TransformedRequestType>
  rxcpp::observable<
      typename detail::RxGrpcClientInvocation<
          grpc::ClientAsyncResponseReader<ResponseType>,
          ResponseType,
          TransformedRequestType,
          Transform>::TransformedResponseType>
  invoke(
      std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          const decltype(Transform::unwrap(
              std::declval<TransformedRequestType>())) &request,
          grpc::CompletionQueue *cq),
      const TransformedRequestType &request,
      grpc::ClientContext &&context = grpc::ClientContext()) {
    return invokeImpl<
        grpc::ClientAsyncResponseReader<ResponseType>,
        ResponseType,
        TransformedRequestType>(
            invoke,
            request,
            std::move(context));
  }

  /**
   * Stream response.
   */
  template <typename ResponseType, typename TransformedRequestType>
  rxcpp::observable<
      typename detail::RxGrpcClientInvocation<
          grpc::ClientAsyncReader<ResponseType>,
          ResponseType,
          TransformedRequestType,
          Transform>::TransformedResponseType>
  invoke(
      std::unique_ptr<grpc::ClientAsyncReader<ResponseType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          const decltype(Transform::unwrap(
              std::declval<TransformedRequestType>())) &request,
          grpc::CompletionQueue *cq,
          void *tag),
      const TransformedRequestType &request,
      grpc::ClientContext &&context = grpc::ClientContext()) {
    return invokeImpl<
        grpc::ClientAsyncReader<ResponseType>,
        ResponseType,
        TransformedRequestType>(
            invoke,
            request,
            std::move(context));
  }

  /**
   * Stream request.
   */
  template <
      typename RequestType,
      typename ResponseType,
      typename TransformedRequestType,
      typename SourceOperator>
  rxcpp::observable<
      typename detail::RxGrpcClientInvocation<
          grpc::ClientAsyncWriter<RequestType>,
          ResponseType,
          TransformedRequestType,
          Transform>::TransformedResponseType>
  invoke(
      std::unique_ptr<grpc::ClientAsyncWriter<RequestType>>
      (Stub::*invoke)(
          grpc::ClientContext *context,
          ResponseType *response,
          grpc::CompletionQueue *cq,
          void *tag),
      const rxcpp::observable<TransformedRequestType, SourceOperator> &requests,
      grpc::ClientContext &&context = grpc::ClientContext()) {
    return invokeImpl<
        grpc::ClientAsyncWriter<RequestType>,
        ResponseType,
        TransformedRequestType>(
            invoke,
            requests,
            std::move(context));
  }

 private:
  template <
      typename Reader,
      typename ResponseType,
      typename TransformedRequestType,
      typename RequestOrObservable,
      typename Invoke>
  rxcpp::observable<
      typename detail::RxGrpcClientInvocation<
          Reader,
          ResponseType,
          TransformedRequestType,
          Transform>::TransformedResponseType>
  invokeImpl(
      Invoke invoke,
      const RequestOrObservable &request_or_observable,
      grpc::ClientContext &&context = grpc::ClientContext()) {

    using ClientInvocation =
        detail::RxGrpcClientInvocation<
            Reader, ResponseType, TransformedRequestType, Transform>;
    using TransformedResponseType =
        typename ClientInvocation::TransformedResponseType;

    return rxcpp::observable<>::create<TransformedResponseType>([
        this, request_or_observable, invoke](
            rxcpp::subscriber<TransformedResponseType> subscriber) {

      auto call = new ClientInvocation(
          request_or_observable, std::move(subscriber));
      call->invoke(invoke, _stub.get(), &_cq);
    });
  }

  std::unique_ptr<Stub> _stub;
  grpc::CompletionQueue &_cq;
};

class RxGrpcServer {
  using ServiceRef = std::unique_ptr<void, std::function<void (void *)>>;
  using Services = std::vector<ServiceRef>;
 public:
  RxGrpcServer(
      Services &&services,
      std::unique_ptr<grpc::ServerCompletionQueue> &&cq,
      std::unique_ptr<grpc::Server> &&server)
      : _services(std::move(services)),
        _cq(std::move(cq)),
        _server(std::move(server)) {}

  RxGrpcServer(RxGrpcServer &&) = default;
  RxGrpcServer &operator=(RxGrpcServer &&) = default;

  ~RxGrpcServer() {
    shutdown();
  }

  class Builder {
   public:
    template <typename Service>
    class ServiceBuilder {
     public:
      /**
       * The pointers passed to the constructor are not Transformed by this
       * class; they need to stay alive for as long as this object exists.
       */
      ServiceBuilder(
          Service *service,
          std::vector<std::unique_ptr<detail::InvocationRequester>> *requesters)
          : _service(*service),
            _invocation_requesters(*requesters) {}

      // Non-streaming request, non-streaming response
      template <
          typename Transform = detail::RxGrpcIdentityTransform,
          typename InnerService,
          typename ResponseType,
          typename RequestType,
          typename Callback>
      ServiceBuilder &registerMethod(
          detail::RequestMethod<
              InnerService,
              RequestType,
              grpc::ServerAsyncResponseWriter<ResponseType>> method,
          Callback &&callback) {
        registerMethodImpl<
            detail::ServerCallTraits<
                grpc::ServerAsyncResponseWriter<ResponseType>,
                Service,
                ResponseType,
                RequestType,
                Transform,
                Callback>>(
                    method, std::forward<Callback>(callback));

        return *this;
      }

      // Non-streaming request streaming response
      template <
          typename Transform = detail::RxGrpcIdentityTransform,
          typename InnerService,
          typename ResponseType,
          typename RequestType,
          typename Callback>
      ServiceBuilder &registerMethod(
          detail::RequestMethod<
              InnerService,
              RequestType,
              grpc::ServerAsyncWriter<ResponseType>> method,
          Callback &&callback) {
        registerMethodImpl<
            detail::ServerCallTraits<
                grpc::ServerAsyncWriter<ResponseType>,
                Service,
                ResponseType,
                RequestType,
                Transform,
                Callback>>(
                    method, std::forward<Callback>(callback));

        return *this;
      }

      // Streaming request, non-streaming response
      template <
          typename Transform = detail::RxGrpcIdentityTransform,
          typename InnerService,
          typename ResponseType,
          typename RequestType,
          typename Callback>
      ServiceBuilder &registerMethod(
          detail::StreamingRequestMethod<
              InnerService,
              grpc::ServerAsyncReader<ResponseType, RequestType>> method,
          Callback &&callback) {
        registerMethodImpl<
            detail::ServerCallTraits<
                grpc::ServerAsyncReader<ResponseType, RequestType>,
                Service,
                ResponseType,
                RequestType,
                Transform,
                Callback>>(
                    method, std::forward<Callback>(callback));

        return *this;
      }

     private:
      template <
          typename ServerCallTraits,
          typename Method,
          typename Callback>
      void registerMethodImpl(
          Method method,
          Callback &&callback) {
        using ServerInvocationRequester =
            detail::RxGrpcServerInvocationRequester<
                Method,
                ServerCallTraits,
                Callback>;

        _invocation_requesters.emplace_back(
            new ServerInvocationRequester(
                method, std::move(callback), &_service));
      }

      Service &_service;
      std::vector<std::unique_ptr<detail::InvocationRequester>> &
          _invocation_requesters;
    };

    template <typename Service>
    ServiceBuilder<Service> registerService() {
      auto service = new Service();
      _services.emplace_back(service, [](void *service) {
        delete reinterpret_cast<Service *>(service);
      });
      _builder.RegisterService(service);
      return ServiceBuilder<Service>(service, &_invocation_requesters);
    }

    grpc::ServerBuilder &grpcServerBuilder() {
      return _builder;
    }

    /**
     * Build and start the gRPC server. After calling this method this object is
     * dead and the only valid operation on it is to destroy it.
     */
    RxGrpcServer buildAndStart() {
      RxGrpcServer server(
          std::move(_services),
          _builder.AddCompletionQueue(),
          _builder.BuildAndStart());

      for (const auto &requester: _invocation_requesters) {
        requester->requestInvocation(_error_handler, server._cq.get());
      }

      return server;
    }

   private:
    GrpcErrorHandler _error_handler = [](std::exception_ptr error) {
      std::rethrow_exception(error);
    };
    Services _services;
    std::vector<std::unique_ptr<detail::InvocationRequester>>
        _invocation_requesters;
    grpc::ServerBuilder _builder;
  };

  template <
      typename Transform = detail::RxGrpcIdentityTransform,
      typename Stub>
  RxGrpcServiceClient<Stub, Transform> makeClient(
      std::unique_ptr<Stub> &&stub) {
    return RxGrpcServiceClient<Stub, Transform>(std::move(stub), _cq.get());
  }

  /**
   * Block and process asynchronous events until the server is shut down.
   */
  void run() {
    return detail::RxGrpcTag::processAllEvents(_cq.get());
  }

  /**
   * Block and process one asynchronous event.
   *
   * Returns false if the event queue is shutting down.
   */
  bool next() {
    return detail::RxGrpcTag::processOneEvent(_cq.get());
  }

  void shutdown() {
    // _server and _cq might be nullptr if this object has been moved out from.
    if (_server) {
      _server->Shutdown();
    }
    if (_cq) {
      _cq->Shutdown();
    }
  }

 private:
  // This object doesn't really do anything with the services other than owning
  // them, so that they are valid while the server is servicing requests and
  // that they can be destroyed at the right time.
  Services _services;
  std::unique_ptr<grpc::ServerCompletionQueue> _cq;
  std::unique_ptr<grpc::Server> _server;
};

class RxGrpcClient {
 public:
  ~RxGrpcClient() {
    shutdown();
  }

  template <
      typename Transform = detail::RxGrpcIdentityTransform,
      typename Stub>
  RxGrpcServiceClient<Stub, Transform> makeClient(
      std::unique_ptr<Stub> &&stub) {
    return RxGrpcServiceClient<Stub, Transform>(std::move(stub), &_cq);
  }

  /**
   * Block and process asynchronous events until the server is shut down.
   */
  void run() {
    return detail::RxGrpcTag::processAllEvents(&_cq);
  }

  /**
   * Block and process one asynchronous event.
   *
   * Returns false if the event queue is shutting down.
   */
  bool next() {
    return detail::RxGrpcTag::processOneEvent(&_cq);
  }

  void shutdown() {
    _cq.Shutdown();
  }

 private:
  grpc::CompletionQueue _cq;
};

}  // namespace shk
