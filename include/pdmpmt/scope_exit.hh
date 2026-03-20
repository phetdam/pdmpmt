/**
 * @file scope_exit.hh
 * @author Derek Huang
 * @brief C++ header for a scope guard object
 * @copyright MIT License
 */

#ifndef PDMPMT_SCOPE_EXIT_HH_
#define PDMPMT_SCOPE_EXIT_HH_

#include <functional>
#include <utility>

namespace pdmpmt {

/**
 * RAII guard ensuring an operation is invoked on scope exit.
 *
 * This is similar to `std::experimental::scope_exit` in nature.
 */
class scope_exit {
public:
  using callable = std::function<void()>;

  /**
   * Ctor.
   *
   * The `scope_exit` is considered active after construction.
   *
   * @param f Nullary callable to invoke on scope exit
   */
  scope_exit(callable f) noexcept : f_{std::move(f)} {}

  /**
   * Deleted copy ctor.
   */
  scope_exit(const scope_exit&) = delete;

  /**
   * Move ctor.
   *
   * The moved-from `scope_exit` is inactive after the move.
   */
  scope_exit(scope_exit&& other) noexcept
  {
    f_ = std::move(other.f_);
    other.f_ = nullptr;
  }

  /**
   * Dtor.
   *
   * Invokes the stored callable and swallows all exceptions if active.
   */
  ~scope_exit()
  {
    try { if (f_) f_(); }
    catch (...) {}
  }

  /**
   * Indicate if the `scope_exit` is active.
   *
   * If the `scope_exit` is active its callable is invoked on destruction.
   */
  explicit operator bool() const noexcept
  {
    return !!f_;
  }

  /**
   * Make the `scope_exit` inactive by move-assigning its callable.
   */
  void release() noexcept
  {
    f_ = nullptr;
  }

private:
  callable f_;
};

}  // namespace pdmpmt

#endif  // PDMPMT_SCOPE_EXIT_HH_
