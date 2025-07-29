// Small subset of chrono

%{
#include <chrono>
#include <ctime>
%}

namespace std {
  // ratio class
  template <intmax_t _Nx, intmax_t _Dx = 1>
  struct ratio { // holds the ratio of _Nx to _Dx
      static constexpr intmax_t num =
          _Sign_of<_Nx>::value * _Sign_of<_Dx>::value * _Abs<_Nx>::value / _Gcd<_Nx, _Dx>::value;

      static constexpr intmax_t den = _Abs<_Dx>::value / _Gcd<_Nx, _Dx>::value;

      using type = ratio<num, den>;
  };
  using atto  = ratio<1, 1000000000000000000LL>;
  using femto = ratio<1, 1000000000000000LL>;
  using pico  = ratio<1, 1000000000000LL>;
  using nano  = ratio<1, 1000000000>;
  using micro = ratio<1, 1000000>;
  using milli = ratio<1, 1000>;
  using centi = ratio<1, 100>;
  using deci  = ratio<1, 10>;
  using deca  = ratio<10, 1>;
  using hecto = ratio<100, 1>;
  using kilo  = ratio<1000, 1>;
  using mega  = ratio<1000000, 1>;
  using giga  = ratio<1000000000, 1>;
  using tera  = ratio<1000000000000LL, 1>;
  using peta  = ratio<1000000000000000LL, 1>;
  using exa   = ratio<1000000000000000000LL, 1>;
  using time_t = uint64_t;
} // namespace std

namespace std::chrono {
  // duration class
  template <class _Rep, class _Period = ratio<1>>
  class duration {
    public:
      using rep    = _Rep;
      using period = typename _Period::type;
      constexpr duration() = default;
      constexpr explicit duration(long long _Val) noexcept
          : _MyRep(static_cast<_Rep>(_Val)) {}
      constexpr _Rep count() const noexcept {
          return _MyRep;
      }
    private:
        _Rep _MyRep; // the stored rep
  };
  using nanoseconds  = duration<long long, nano>;
  using microseconds = duration<long long, micro>;
  using milliseconds = duration<long long, milli>;
  using seconds      = duration<long long>;

  // time_point class
  template <class _Clock, class _Duration = typename _Clock::duration>
  class time_point {
    static_assert(__is_duration<_Duration>::value,
                  "Second template parameter of time_point must be a std::chrono::duration");

  public:
    typedef _Clock clock;
    typedef _Duration duration;
    typedef typename duration::rep rep;
    typedef typename duration::period period;

  private:
    duration __d_;

  public:
     constexpr time_point() : __d_(duration::zero()) {}
     constexpr explicit time_point(const duration& __d) : __d_(__d) {}
    template <class _Duration2, __enable_if_t<is_convertible<_Duration2, duration>::value, int> = 0>
     constexpr time_point(const time_point<clock, _Duration2>& __t)
        : __d_(__t.time_since_epoch()) {}
     constexpr duration time_since_epoch() const { return __d_; }
  };

  class system_clock {
  public:
    typedef microseconds duration; // WARNING: This is implementation defined
    typedef duration::rep rep;
    typedef duration::period period;
    typedef chrono::time_point<system_clock> time_point;
    static constexpr const bool is_steady = false;

    static time_point now() noexcept;
    static time_t to_time_t(const time_point& __t) noexcept;
    static time_point from_time_t(time_t __t) noexcept;
  };
} // namespace std::chrono

%nspace std::chrono::duration<long long, std::nano>;
%nspace std::chrono::duration<long long, std::micro>;
%nspace std::chrono::duration<long long, std::milli>;
%nspace std::chrono::duration<long long>;
%nspace std::chrono::time_point<std::chrono::system_clock>;

%template(nanoseconds) std::chrono::duration<long long, std::nano>;
%template(microseconds) std::chrono::duration<long long, std::micro>;
%template(milliseconds) std::chrono::duration<long long, std::milli>;
%template(seconds) std::chrono::duration<long long>;
%template(system_clock_time_point) std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long long, std::micro>>;
