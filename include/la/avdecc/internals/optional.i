// https://github.com/swig/swig/issues/1307

%{
#include <optional>
%}

namespace std {

  template<class T> class optional {
    public:
    typedef T value_type;

    optional();
    optional(T value);
    optional(const optional& other);

    constexpr bool has_value() const noexcept;
    constexpr const T& value() const&;
    //constexpr T& value() &;
    constexpr T&& value() &&;
    constexpr const T&& value() const&&;
  };
} // namespace std

#if defined(SWIGCSHARP)
    %include "csharp_optional.i"
#elif defined(SWIGJAVA)
    %include "java_optional.i"
#elif defined(SWIGPYTHON)
    %include "python_optional.i"
#else
    #error "No OptionalValue<> typemaps for this language."
#endif

// If there is no language-specific DEFINE_OPTIONAL_CLASS() definition, assume
// that we only have "simple" optional values.
#ifndef DEFINE_OPTIONAL_CLASS
%define DEFINE_OPTIONAL_CLASS(scope, classname, name)
DEFINE_OPTIONAL_SIMPLE(name, scope::classname, 0)
%enddef
#endif
