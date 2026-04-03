/**
 * @file thread_pool.hh
 * @author Derek Huang
 * @brief C++ header for a thread pool for FIFO task execution
 * @copyright MIT License
 */

#ifndef PDMPMT_THREAD_POOL_HH_
#define PDMPMT_THREAD_POOL_HH_

#include <condition_variable>
#include <exception>
#include <future>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace pdmpmt {

namespace detail {

/**
 * Tag type indicating an overload with `std::future` integration.
 */
struct use_future_type {};

}  // namespace detail

/**
 * Tag global indicating an overload with `std::future` integration.
 */
inline constexpr detail::use_future_type use_future;

/**
 * Thread pool implementation that consumes tasks in FIFO order.
 *
 * This class encapsulates the logic for a specified number of worker threads
 * and provides functionality to start and stop the threads, query the number
 * of threads and pending tasks in the queue, and to post tasks. Condition
 * variables are used for efficient notification of both worker threads and
 * external threads that are waiting on the pending task queue to be empty.
 *
 * @note The implementation here is relatively simple and is best suited for
 *  CPU-bound tasks. If the tasks are mostly I/O-bound then the worker threads
 *  will be blocked waiting for I/O instead of being useful.
 */
class thread_pool {
public:
  using task_type = std::function<void()>;

  /**
   * Ctor.
   *
   * Constructs and starts a thread pool with the given number of threads.
   *
   * @param size Number of threads in pool
   */
  thread_pool(unsigned size) : thread_pool{std::launch::async, size} {}

  /**
   * Ctor.
   *
   * Constructs a thread pool with the given number of threads and based on the
   * value of the launch policy may or may not actual start the threads. If the
   * policy contains `std::launch::async`, the threads are started immediately
   * during construction. If the policy contains `std::launch::deferred`, then
   * the threads will *not* be started, unless `std::launch::async` is also
   * specified. Any implementation-defined flags are ignored and are treated as
   * if `std::launch::async` was specified in the policy.
   *
   * @param policy Thread launch policy
   * @param size Number of threads in pool
   */
  thread_pool(std::launch policy, unsigned size) : threads_(size)
  {
    // async launch
    if ((policy & std::launch::async) == std::launch::async)
      start();
    // deferred launch
    if ((policy & std::launch::deferred) == std::launch::deferred)
      return;
    // otherwise, treat as async launch anyways
    start();
  }

  /**
   * Dtor.
   *
   * If the worker threads in the pool are still running they will be stopped.
   */
  ~thread_pool()
  {
    stop();
  }

  /**
   * Start the worker threads and begin processing events.
   *
   * If the threads are already running, this is a no-op, but if they are not
   * running, this starts all the threads' task loops.
   *
   * @note This function is thread-safe.
   */
  void start()
  {
    // always acquire mutex for correct ordering of events
    std::lock_guard _{mut_};
    // no-op
    if (running_)
      return;
    // ok, not running, so mark as running
    running_ = true;
    // start threads
    // note: all threads will be briefly blocked at the beginning of their
    // iterations, but this is fine, as we want to have all the threads started
    // before we release the lock and allow them to work
    for (auto& thread : threads_)
      thread = std::thread{task_loop()};
  }

  /**
   * Post a task for execution.
   *
   * The task will be posted at the rear of the queue for execution. Once the
   * task is posted, one worker thread blocking on a condition variable will be
   * notified so that it may wake up and begin executing the task.
   *
   * @note This function is thread-safe.
   *
   * @param task Task to execute
   */
  auto& post(task_type task)
  {
    // lock to push new task
    {
      std::lock_guard _{mut_};
      tasks_.push_back(std::move(task));
    }
    // note: notify outside of lock since we want threads to be awake anyways
    // note: only need to notify one thread at a time per task
    cv_push_.notify_one();
    return *this;
  }

  /**
   * Post a task and arguments for execution and obtain a future to the result.
   *
   * This `post()` overload provides `std::future` integration, enabling the
   * caller to block for the task's result. Do *not* discard the future before
   * the shared state has been updated as otherwise behavior is undefined.
   *
   * @note This function is thread-safe.
   *
   * @tparam F *Callable* to copy or move
   * @tparam Ts *Callable* argument types
   *
   * @param f Callable to invoke
   * @param args Arguments to forward and invoke with
   * @returns `std::future` to obtain the result from
   */
  template <typename F, typename... Ts>
  auto post(detail::use_future_type, F&& f, Ts&&... args)
  {
    // deduced return type
    using R = std::invoke_result_t<F, Ts...>;
    // create promise and obtain future
    // note: using shared_ptr to allow closure to be copy-constructible
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();
    // form appropriate task with the moved promise, callable, and arguments
    auto task = [
      f = std::forward<F>(f),
      // see https://stackoverflow.com/a/49902823/14227825. in C++20 we can
      // directly initialize with the std::forward<Ts>(args)... expansion
      args = std::tuple{std::forward<Ts>(args)...},
      prom = std::move(promise)
    ]() noexcept
    {
      try {
        // if no value returned set_value() is called separately
        if constexpr (std::is_same_v<R, void>) {
          std::apply(f, args);
          prom->set_value();
        }
        // otherwise directly set value
        else
          prom->set_value(std::apply(f, args));
      }
      catch (...) {
        prom->set_exception(std::current_exception());
      }
    };
    // lock to push new task and return future
    post(std::move(task));
    return future;
  }

  /**
   * Request that the worker threads stop executing.
   *
   * If the threads are not running, this is a no-op, but if they are, this
   * marks the pool as not running, wakes up any sleeping worker threads so
   * they can end their task loops, and joins the threads. Any threads external
   * to the thread pool blocking on `wait()` are also woken up.
   *
   * @note This function is thread-safe.
   *
   * @par
   *
   * @note This function will block until the last running task is done.
   */
  void stop()
  {
    {
      // always acquire mutex for correct ordering of events
      std::lock_guard _{mut_};
      // no-op
      if (!running_)
        return;
      // ok, we are running, so mark as stopped
      running_ = false;
    }
    // notify all threads outside of lock (spurious wakeup is fine now)
    cv_push_.notify_all();
    cv_pop_.notify_all();
    // join all threads to stop work
    for (auto& thread : threads_)
      thread.join();
  }

  /**
   * Return the number of threads in the pool.
   *
   * @note This function is thread-safe even without acquiring the internal
   *  mutex because the number of threads doesn't change after construction.
   */
  auto size() const noexcept
  {
    return threads_.size();
  }

  /**
   * Indicate if the pool is currently running or not.
   *
   * @note This function is thread-safe.
   */
  bool running() const
  {
    std::lock_guard _{mut_};
    return running_;
  }

  /**
   * Return the number of pending tasks in the queue.
   *
   * @note This function is thread-safe.
   *
   * @par
   *
   * @note After calling `stop()` any pending tasks will still be in the queue,
   *  so if `start()` is then called, execution will resume.
   */
  auto pending() const
  {
    std::lock_guard _{mut_};
    return tasks_.size();
  }

  /**
   * Wait until the pool stops running or until pending task queue is empty.
   *
   * The thread will block on a condition variable until a task is consumed, in
   * which case it will be notified to wake. Until the pool is stopped, i.e.
   * `running()` is `false`, or `pending()` returns zero, the thread blocks.
   *
   * @todo Revise semantics. We could `wait()` until the pending task queue is
   *  empty and all threads are *not* running a task (require a boolean or char
   *  vector for bookkeeping this state), or `wait()` until the pending task
   *  queue is empty and at least *one* thread is not running.
   */
  void wait() const
  {
    std::unique_lock lock{mut_};
    cv_pop_.wait(lock, [this] { return !running_ || tasks_.empty(); });
  }

private:
  mutable std::mutex mut_;                   // mutex for synchronization
  // two condition variables: one for indicating that work has been pushed onto
  // the task queue, one that indicates work has been popped from the queue
  mutable std::condition_variable cv_push_;
  mutable std::condition_variable cv_pop_;
  bool running_{};                           // indicate if pool is running
  std::vector<std::thread> threads_;         // worker threads
  std::deque<task_type> tasks_;              // task queue

  /**
   * Return the task loop for a worker thread.
   *
   * This is the main loop run by each worker thread as they wait for tasks.
   * Until the pool is stopped, on each iteration, the thread will check the
   * queue for work, and if there is work, get the task, release the lock, and
   * then run the task. If there is no work, however, the thread blocks on the
   * condition variable to avoid wasting CPU time.
   *
   * When a sleeping thread is successfully awoken and has reclaimed the lock,
   * it will consume the next available task, release the lock, and execute.
   * Condition variables are notified appropriately.
   */
  task_type task_loop()
  {
    return [this]
    {
      // task to run
      task_type task;
      // until done
      while (true) {
        {
          // note: need to acquire mut_ before even checking running_
          std::unique_lock lock{mut_};
          // if stopped, done
          if (!running_)
            return;
          // if no work, block until notified of work. if spuriously awoken but
          // we still are running and there is still no work, back to sleep
          if (tasks_.empty())
            cv_push_.wait(lock, [this] { return !running_ || !tasks_.empty(); });
          // if properly woken up we either need to stop running, there is work
          // to be done, or both are true. if not running, return
          if (!running_)
            return;
          // otherwise, there *must* be work in the queue (as otherwise wait()
          // would not successfully return and unlock the thread). therefore,
          // consume a task from the queue to execute
          task = std::move(tasks_.front());
          tasks_.pop_front();
        }
        // notify all external threads blocking on the condition variable that
        // a task has been consumed. any external threads blocking that are
        // waiting for pending tasks to be cleared will wake and check
        cv_pop_.notify_all();
        // run task outside of lock
        // TODO: may want to try-catch to avoid crashing on exception
        task();
      }
    };
  }
};

}  // namespace pdmpmt

#endif  // PDMPMT_THREAD_POOL_HH_
