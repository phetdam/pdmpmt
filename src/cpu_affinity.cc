/**
 * @file cpu_affinity.cc
 * @author Derek Huang
 * @brief C++ program to display + set CPU affinity
 * @copyright MIT License
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for CPU_SET(), etc.
#endif  // _GNU_SOURCE
#include <sched.h>
#include <sys/sysinfo.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <system_error>
#include <type_traits>
#include <utility>

namespace {

/**
 * CPU set representing a thread's CPU affinity mask.
 *
 * This uses the dynamically-allocated `cpu_set_t` on Linux to suport more than
 * 64 processors. On Windows threads can only set affinity within their process
 * group and thus 64 logical processors are the maximum.
 */
class cpu_set {
private:
#ifdef _WIN32
  /**
   * Processor group limit on Windows (fixed at 64).
   */
  static constexpr std::size_t max_cpus = 64u;
#endif  // _WIN32
  /**
   * CPU set entry.
   *
   * This is a proxy type used to abstract testing and setting of the CPU set
   * mask at a given CPU index less than `size()`.
   */
  template <typename S>
  class entry {
  private:
    /**
     * Helper traits to ensure template type dependence for `operator=`.
     *
     * We need a template parameter dependence to delay instantiation.
     *
     * @tparam B type
     */
    template <typename B, typename = void>
    struct enable_assign {};

    /**
     * True specialization enabling `operator=`.
     *
     * @tparam B type
     */
    template <typename B>
    struct enable_assign<
      B,
      std::enable_if_t<
        std::is_same_v<B, bool> &&               // only allow bool
        !std::is_same_v<S, std::add_const_t<S>>  // disable for non-const
      > > {
      using type = int;
    };

  public:
    using set_type = S;

    /**
     * Ctor.
     *
     * @param set CPU set
     * @param cpu CPU index
     */
    entry(S& set, std::size_t cpu) noexcept : set_{&set}, cpu_{cpu} {}

    /**
     * Indicate if the given CPU is a member of the CPU set.
     */
    operator bool() const noexcept
    {
#if defined(_WIN32)
      // on Windows need to guard against shifting too much
      return (cpu_ < max_cpus) && !!(set_->set_ & (1u << cpu_));
#else
      return !!CPU_ISSET_S(cpu_, set_->alloc_size_, set_->set_);
#endif  // !defined(_WIN32)
    }

    /**
     * Add or remove the given CPU from the set.
     *
     * @note This is conditionally available when `S` is const-qualified.
     *
     * @param add `true` to add, `false` to remove
     */
    template <typename B, typename enable_assign<B>::type = 0>
    auto& operator=(B add) noexcept
    {
#if defined(_WIN32)
      // on Windows need to guard against shifting too much
      if (cpu_ < max_cpus) {
        if (add)
          set_->set_ |= (1u << cpu_);
        else
          set_->set_ &= ~(1u << cpu_);
      }
#else
      if (add)
        CPU_SET_S(cpu_, set_->alloc_size_, set_->set_);
      else
        CPU_CLR_S(cpu_, set_->alloc_size_, set_->set_);
#endif  // !defined(_WIN32)
      return *this;
    }

  private:
    S* set_;           // CPU set
    std::size_t cpu_;  // CPU index
  };

public:
  /**
   * Textual stream format with user-selected characters.
   *
   * Two characters can be provided to indicate CPU availability. The default
   * "off" character is `'-'` and the default "on" character is `'*'`.
   */
  class text_format {
  public:
    /**
     * Ctor.
     *
     * Uses the default on and off characters of `'*'` and `'-'`.
     */
    constexpr text_format() noexcept : text_format{'-', '*'} {}

    /**
     * Ctor.
     *
     * @param off Character to represent offline CPU
     * @param on Character to represent online CPU
     */
    constexpr text_format(char off, char on) noexcept : chars_{off, on} {}

    /**
     * Ctor.
     *
     * The character representing an offline CPU is `'-'`.
     *
     * @param on Character to represent online CPU
     */
    constexpr explicit text_format(char on) noexcept : text_format{'-', on} {}

    /**
     * Ctor.
     *
     * Enables construction from a null-terminated string literal. If only one
     * character is provided (length 2), then it is used for the "on" symbol.
     *
     * @param chars Formatting characters for off and on indication
     */
    template <std::size_t N, std::enable_if_t<(N == 2 || N == 3), int> = 0>
    constexpr explicit text_format(const char (&chars)[N]) noexcept
      : text_format{
          [&chars]
          {
            if constexpr (N == 2u)  // first character as "on" character
              return '-';
            else
              return chars[0];
          }(),
          [&chars]
          {
            if constexpr (N == 2u)  // first character as "on" character
              return chars[0];
            else
              return chars[1];
          }()
        }
    {}

    /**
     * Return the off and on formatting characters.
     */
    constexpr auto chars() const noexcept
    {
      return chars_;
    }

    /**
     * Return the "off" charactter.
     */
    constexpr auto off() const noexcept
    {
      return chars_[0];
    }

    /**
     * Return the "on" character.
     */
    constexpr auto on() const noexcept
    {
      return chars_[1];
    }

  private:
    char chars_[2];  // formatting characters
  };

  /**
   * Textual stream formatter using a specified text format.
   */
  class text_formatter {
  public:
    /**
     * Ctor.
     *
     * @param out Output stream
     * @param fmt Text format specifier
     */
    text_formatter(std::ostream& out, text_format fmt) noexcept
      : out_{&out}, fmt_{std::move(fmt)}
    {}

    /**
     * Return the output stream reference.
     */
    auto& out() const noexcept { return *out_; }

    /**
     * Return the text format specifier.
     */
    auto& fmt() const noexcept { return fmt_; }

  private:
    std::ostream* out_;  // output stream
    text_format fmt_;    // text format
  };

  /**
   * Return a text format given the input arguments.
   *
   * @param on Character to represent online CPU
   */
  static constexpr auto fmt(char on) noexcept
  {
    return text_format{on};
  }

  /**
   * Return a text format given off and on characters.
   *
   * @param off Character to represent offline CPU
   * @param on Character to represent online CPU
   */
  static constexpr auto fmt(char off, char on) noexcept
  {
    return text_format{off, on};
  }

  /**
   * Return a text format given a string literal.
   *
   * @tparam N String literal length (2 or 3)
   *
   * @param chars Formatting characters for off and on indication
   */
  template <std::size_t N, std::enable_if_t<(N == 2 || N == 3), int> = 0>
  static constexpr auto fmt(const char (&chars)[N]) noexcept
  {
    return text_format{chars};
  }

  /**
   * Construct the CPU set with the current thread's CPU affinity mask.
   *
   * This represents the CPU for the `get_nprocs_conf()` CPUs available.
   */
  static auto current()
  {
    cpu_set cpus;
    if (sched_getaffinity(0, cpus.alloc_size_, cpus) < 0)
      throw std::system_error{
        {errno, std::system_category()},
        "call to sched_getaffinity() for current thread"
      };
    return cpus;
  }

  /**
   * Default ctor.
   *
   * Constructs a zeroed CPU set representing `get_nprocs_conf()` CPUs.
   */
  cpu_set()
  {
    from(get_nprocs_conf());
  }

  /**
   * Ctor.
   *
   * Constructs a zeroed CPU set representing the given number of CPUs.
   *
   * @param cpus Number of CPUs
   */
  explicit cpu_set(std::size_t cpus)
  {
    from(cpus);
  }

  /**
   * Copy ctor.
   */
  cpu_set(const cpu_set& other)
  {
    from(other);
  }

  /**
   * Move ctor.
   */
  cpu_set(cpu_set&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Dtor.
   */
  ~cpu_set()
  {
    destroy();
  }

  /**
   * Copy assignment operator.
   */
  auto& operator=(const cpu_set& other)
  {
    destroy();
    from(other);
    return *this;
  }

  /**
   * Move assignment operator.
   */
  auto& operator=(cpu_set&& other) noexcept
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Return the CPU set pointer.
   *
   * This is mostly for C function interop and should be treated opaquely.
   */
  auto set() const noexcept { return set_; }

  /**
   * Return the allocated CPU set size in bytes.
   */
  auto alloc_size() const noexcept { return alloc_size_; }

  /**
   * Return the number of CPUs that can be represented by the CPU set.
   */
  auto size() const noexcept { return size_; }

  /**
   * Return the number of available CPUs in the CPU set.
   */
  auto count() const noexcept
  {
    return static_cast<std::size_t>(CPU_COUNT_S(alloc_size_, set_));
  }

  /**
   * Return an entry corresponding to the given CPU index.
   *
   * @param cpu CPU index
   */
  auto operator[](std::size_t cpu) noexcept
  {
    return entry{*this, cpu};
  }

  /**
   * Return a const entry corresponding to the given CPU index.
   *
   * @param cpu CPU index
   */
  auto operator[](std::size_t cpu) const noexcept
  {
    return entry{*this, cpu};
  }

  /**
   * Implicitly convert into `cpu_set_t*` for C function interop.
   *
   * This can also be used to determine if the `cpu_set` was moved from.
   */
  operator cpu_set_t*() const noexcept
  {
    return set_;
  }

private:
  cpu_set_t* set_;          // CPU set
  std::size_t alloc_size_;  // CPU set allocation size
  std::size_t size_;        // number of CPUs representable by the set

  /**
   * Tag type to select overloads that do not zero the CPU set.
   */
  struct no_zero_tag {};

  /**
   * Global used to select overloads that do not zero the CPU set.
   */
  static constexpr no_zero_tag no_zero{};

  /**
   * Initialize using a given maximum CPU count without zeroing.
   */
  void from(std::size_t cpus, no_zero_tag)
  {
    // allocate CPU set
    set_ = CPU_ALLOC(cpus);
    if (!set_)
      throw std::system_error{{errno, std::system_category()}, "CPU_ALLOC()"};
    // record allocation size + max CPU count
    alloc_size_ = CPU_ALLOC_SIZE(cpus);
    size_ = cpus;
  }

  /**
   * Initialize using a given maximum CPU count.
   */
  void from(std::size_t cpus)
  {
    from(cpus, no_zero);
    CPU_ZERO_S(alloc_size_, set_);  // zero bits
  }

  /**
   * Initialize from another CPU set object by copy.
   */
  void from(const cpu_set& other)
  {
    from(other.size(), no_zero);
    std::memcpy(set_, other.set_, alloc_size_);
  }

  /**
   * Initialize from another CPU set object by move.
   *
   * After move, the moved-from object has `nullptr` data and zero sizes.
   */
  void from(cpu_set&& other) noexcept
  {
    set_ = other.set_;
    alloc_size_ = other.alloc_size_;
    size_ = other.size_;
    other.set_ = nullptr;
    other.alloc_size_ = 0u;
    other.size_ = 0u;
  }

  /**
   * Deallocate the CPU set if not moved from.
   */
  void destroy() noexcept
  {
    if (set_)
      CPU_FREE(set_);
  }
};

/**
 * Stream the `cpu_set` CPU mask as a string of 1s and 0s.
 *
 * @param out Output stream
 * @param cpus CPU set
 */
auto& operator<<(std::ostream& out, const cpu_set& cpus)
{
  // TODO: no range-for support yet
  for (auto i = 0u; i < cpus.size(); i++)
    out << static_cast<int>(cpus[i]);
  return out;
}

/**
 * Return a `text_formatter` to change the way `cpu_set` is formatted.
 *
 * @note All arguments are by reference since MSVC cannot guarantee
 *  left-to-right evaluation otherwise (C4866).
 *
 * @param out Output stream
 * @param fmt Textual format specifier
 */
auto operator<<(std::ostream& out, const cpu_set::text_format& fmt)
{
  return cpu_set::text_formatter{out, fmt};
}

/**
 * Stream the `cpu_set` CPU using the specified textual format.
 *
 * @note All arguments are by reference since MSVC cannot guarantee
 *  left-to-right evaluation otherwise (C4866).
 *
 * @param out Text formatter
 * @param cpus CPU set
 */
auto& operator<<(const cpu_set::text_formatter& out, const cpu_set& cpus)
{
  // TODO: no range-for support yet
  out.out() << "[";
  for (auto i = 0u; i < cpus.size(); i++)
    out.out() << out.fmt().chars()[cpus[i]];
  return out.out() << "]";
}

}  // namespace

int main()
{
  // helper to print CPU set + current CPU number
  auto print_info = [](const cpu_set& cpus)
  {
    std::cout << cpus << " " << cpu_set::fmt("*") << cpus << " current: " <<
      sched_getcpu() << std::endl;
  };
  // get CPU set with current thread's process affinity + print
  auto cpus = cpu_set::current();
  print_info(cpus);
  // modify + set thread affinity
  for (auto i = 0u; i < cpus.size(); i += 2u)
    cpus[i] = false;
  if (sched_setaffinity(0, cpus.alloc_size(), cpus) < 0)
    throw std::system_error{
      {errno, std::system_category()},
      "call to sched_setaffinity() for current thread failed"
    };
  // get thread affinity again + print
  cpus = cpu_set::current();
  print_info(cpus);
  // clear and repeat for each available CPU number
  for (auto i = 0u; i < cpus.size(); i++) {
    cpus = {};
    cpus[i] = true;
    // if EINVAL, CPU is not online/insufficient perms, so don't error
    if (sched_setaffinity(0, cpus.alloc_size(), cpus) < 0) {
      switch (errno) {
      case EINVAL:
        break;
      default:
        throw std::system_error{
          {errno, std::system_category()},
          "call to sched_setaffinity() for current thread failed"
        };
      }
    }
    // if current CPU is not i affinity switch failed
    std::cout << cpus << " " << cpu_set::fmt("*") << cpus << " current: ";
    auto cur_cpu = sched_getcpu();
    // note: cast to appease compiler warning about signed/unsigned mismatch
    if (i == static_cast<decltype(i)>(cur_cpu))
      std::cout << cur_cpu << std::endl;
    else
      std::cout << "EINVAL (no migration)" << std::endl;
  }
  // set the thread's process affinity
  return EXIT_SUCCESS;
}
