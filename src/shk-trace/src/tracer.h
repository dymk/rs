#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <dispatch/dispatch.h>
#include <libc.h>

#include "dispatch.h"
#include "event.h"
#include "event_info.h"
#include "kdebug.h"
#include "kdebug_controller.h"
#include "syscall_constants.h"
#include "syscall_tables.h"

namespace shk {

/**
 * Tracer uses a low-level KdebugController object and (via the delegate)
 * exposes a higher-level stream of events (such as thread creation/termination
 * and file manipulation events). It does not format its output and it does not
 * follow process children.
 */
class Tracer {
 public:
  class Delegate {
   public:
    enum class Response {
      OK,
      QUIT_TRACING
    };

    enum class SymlinkBehavior {
      FOLLOW,
      NO_FOLLOW
    };

    virtual ~Delegate() = default;

    virtual void newThread(
        pid_t pid,
        uintptr_t parent_thread_id,
        uintptr_t child_thread_id) = 0;

    /**
     * Invoked when a thread has terminated. The return value of this callback
     * gives an opportunity for the Delegate to instruct the tracer to stop.
     */
    virtual Response terminateThread(uintptr_t thread_id) = 0;

    /**
     * A path that is "" means that the path refers to the file or directory ´
     * that at_fd points to.
     */
    virtual void fileEvent(
        uintptr_t thread_id,
        EventType type,
        int at_fd,
        std::string &&path,
        SymlinkBehavior symlink_behavior) = 0;

    /**
     * Invoked whenever a file descriptor to a file or directory has been
     * opened, along with the path of the file (possibly relative), its
     * initial cloexec flag and the file descriptor that the path is relative
     * to (possibly AT_FDCWD which means the working directory).
     *
     * For some operations, Tracer will call both fileEvent and open.
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void open(
        uintptr_t thread_id,
        int fd,
        int at_fd,
        std::string &&path,
        bool cloexec) = 0;

    /**
     * Invoked whenever a file descriptor has been duplicated.
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void dup(
        uintptr_t thread_id,
        int from_fd,
        int to_fd,
        bool cloexec) = 0;

    /**
     * Invoked whenever the cloexec flag has been set on a file descriptor.
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void setCloexec(
        uintptr_t thread_id, int fd, bool cloexec) = 0;

    /**
     * Invoked whenever a file descriptor has been closed. (Except for when they
     * are implicitly closed due to exec.)
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void close(uintptr_t thread_id, int fd) = 0;

    /**
     * Invoked whenever a process's working directory has been changed. The path
     * may be relative. The file descriptor that the path is relative to is also
     * passed, in at_fd (which may be AT_FDCWD which means the current working
     * directory).
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void chdir(
        uintptr_t thread_id, std::string &&path, int at_fd) = 0;

    /**
     * Invoked whenever a thread has changed its thread-local working directory.
     * The path may be relative. If so, it is relative to the file pointed to by
     * the file descriptor at_fd (which may be AT_FDCWD whic means the current
     * working directory).
     */
    virtual void threadChdir(
        uintptr_t thread_id, std::string &&path, int at_fd) = 0;

    /**
     * Invoked whenever a process has successfully invoked an exec family system
     * call.
     *
     * thread_id is the id of the thread that made the system call that caused
     * this to happen.
     */
    virtual void exec(uintptr_t thread_id) = 0;
  };

  Tracer(
      int num_cpus,
      KdebugController &kdebug_ctrl,
      Delegate &delegate);
  Tracer(const Tracer &) = delete;
  Tracer &operator=(const Tracer &) = delete;
  ~Tracer();

  void start(dispatch_queue_t queue);

  /**
   * Block until the tracing server has stopped listening (this happens when the
   * delegate instructs it to quit). This method can be called from any thread.
   *
   * Returns true on success, or false on timeout.
   *
   * This method should be called at most once.
   */
  bool wait(dispatch_time_t timeout);

 private:
  void loop(dispatch_queue_t queue);

  void set_enable(bool enabled);
  void set_remove();
  uint64_t sample_sc(std::vector<kd_buf> &event_buffer);
  void enter_event_now(uintptr_t thread, int type, kd_buf *kd);
  void enter_event(uintptr_t thread, int type, kd_buf *kd);
  void enter_illegal_event(uintptr_t thread, int type);
  void exit_event(
      uintptr_t thread,
      int type,
      uintptr_t arg1,
      uintptr_t arg2,
      uintptr_t arg3,
      uintptr_t arg4,
      int syscall);
  void format_print(
      event_info *ei,
      uintptr_t thread,
      int type,
      uintptr_t arg1,
      uintptr_t arg2,
      uintptr_t arg3,
      uintptr_t arg4,
      int syscall,
      const char *pathname1 /* nullable */,
      const char *pathname2 /* nullable */);

  std::atomic<bool> _shutting_down;
  DispatchSemaphore _shutdown_semaphore;
  std::vector<kd_buf> _event_buffer;

  KdebugController &_kdebug_ctrl;
  Delegate &_delegate;

  std::unordered_map<uint64_t, std::string> _vn_name_map;
  event_info_map _ei_map;

  int _trace_enabled = 0;
};

}
