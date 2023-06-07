// Small subset of chrono

%{
#include <chrono>
%}

namespace std {
  // ratio class
  template <intmax_t _Nx, intmax_t _Dx = 1>
  struct ratio { // holds the ratio of _Nx to _Dx
      static_assert(_Dx != 0, "zero denominator");
      static_assert(-INTMAX_MAX <= _Nx, "numerator too negative");
      static_assert(-INTMAX_MAX <= _Dx, "denominator too negative");

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
} // namespace std

namespace std::chrono {
  // Duration class
  template <class _Rep, class _Period = ratio<1>> class duration {
    public:
      using rep    = _Rep;
      using period = typename _Period::type;
      constexpr duration() = default;
      constexpr explicit duration(long long _Val) noexcept(is_arithmetic_v<_Rep>) /* strengthened */
          : _MyRep(static_cast<_Rep>(_Val)) {}
      constexpr _Rep count() const noexcept(is_arithmetic_v<_Rep>) /* strengthened */ {
          return _MyRep;
      }
    private:
        _Rep _MyRep; // the stored rep
  };

  using nanoseconds  = duration<long long, nano>;
  using microseconds = duration<long long, micro>;
  using milliseconds = duration<long long, milli>;
  using seconds      = duration<long long>;
} // namespace std::chrono

%nspace std::chrono::duration<long long, std::nano>;
%nspace std::chrono::duration<long long, std::micro>;
%nspace std::chrono::duration<long long, std::milli>;
%nspace std::chrono::duration<long long>;

%template(nanoseconds) std::chrono::duration<long long, std::nano>;
%template(microseconds) std::chrono::duration<long long, std::micro>;
%template(milliseconds) std::chrono::duration<long long, std::milli>;
%template(seconds) std::chrono::duration<long long>;
