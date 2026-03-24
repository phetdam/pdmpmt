/**
 * @file thread_pool.hh
 * @author Derek Huang
 * @brief C++ header for a thread pool for FIFO task execution
 * @copyright MIT License
 */

#ifndef PDMPMT_THREAD_POOL_HH_
#define PDMPMT_THREAD_POOL_HH_

#include <condition_variable>
#include <future>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace pdmpmt {

/**
 * Thread pool implementation that consumes tasks in FIFO order.
 *
 * This class encapsulates the logic for a specified number of worker threads
 * and provides functionality to start and stop the threads, query the number
 * of threads and pending tasks in the queue, and to post tasks.
 *
 * @note The implementation here is relatively simple and is best suited for
 *  CPU-bound tasks. If the tasks are mostly I/O-bound then the worker threads
 *  will be blocked waiting for I/O instead of being useful.
 *
 * @par
 *
 * @note It would be more efficient if we didn't have a `wait()` function to
 *  allow a calling thread to block until all pending tasks were consumed, as
 *  then we could notify one thread instead of all threads blocked on the
 *  condition variable. However, `wait()` is very helpful to have, as the
 *  alternative would be to `while (pool.pending())`.
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
   * This task will be posted at the rear of the queue for execution. Once the
   * task is posted, all the threads blocking on the condition variable will be
   * notified so that they may wake up and begin executing.
   *
   * @note This function is thread-safe.
   *
   * @par
   *
   * @note Although notifying one thread may seem more efficient compared to
   *  notifying all threads, given that threads blocking on `wait()` would need
   *  to be notified, we are forced to notify all threads.
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
    cv_.notify_all();
    return *this;
  }

  // TODO: add tag overload of post() for std::future interop

  /**
   * Request that the worker threads stop executing.
   *
   * If the threads are not running, this is a no-op, but if they are, this
   * marks the pool as not running, wakes up any sleeping worker threads so
   * they can end their task loops, and joins the threads.
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
    cv_.notify_all();
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
   * The thread will block on the condition variable. During the course of task
   * execution it is possible that the thread may be woken up relatively
   * frequently, but will continue to release the lock and sleep until the pool
   * is stopped, e.g. `running()` is `false`, or `pending()` returns zero.
   *
   * @todo Revise semantics. We could `wait()` until the pending task queue is
   *  empty and all threads are *not* running a task (require a boolean or char
   *  vector for bookkeeping this state), or `wait()` until the pending task
   *  queue is empty and at least *one* thread is not running.
   */
  void wait() const
  {
    std::unique_lock lock{mut_};
    cv_.wait(lock, [this] { return !running_ || tasks_.empty(); });
  }

private:
  mutable std::mutex mut_;              // mutex for synchronization
  mutable std::condition_variable cv_;  // condition variable for notification
  bool running_{};                      // indicate if pool is running
  std::vector<std::thread> threads_;    // worker threads
  std::deque<task_type> tasks_;         // task queue

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
   * it will consume the next available task, release the lock, notify all idle
   * worker or outside threads that an event has occurred, and execute.
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
          // if no work, block on condition variable. if spuriously awoken but
          // we still are running and there is still no work, back to sleep
          if (tasks_.empty())
            cv_.wait(lock, [this] { return !running_ || !tasks_.empty(); });
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
        // notify all threads blocking on the condition variable that a task
        // has been consumed. if there are pending tasks, any waiting worker
        // threads will get to wake up and compete for work. any external
        // threads blocking on wait() will also be appropriately woken up
        cv_.notify_all();
        // run task outside of lock
        task();
      }
    };
  }
};

}  // namespace pdmpmt

#endif  // PDMPMT_THREAD_POOL_HH_
