#include <optional>

// The macro arguments for all these macros are the name of the exported class
// and the C++ type T of std::optional<T> to generate the typemaps for.

// Common part of the macros below, shouldn't be used directly.
%define DEFINE_OPTIONAL_HELPER(OptT, T)

// The std::optional<> specializations themselves are only going to be used
// inside our own code, the user will deal with either T? or T, depending on
// whether T is a value or a reference type, so make them private to our own
// assembly.
%typemap(csclassmodifiers) std::optional< T > "internal class"

// Do this to use reference typemaps instead of the pointer ones used by
// default for the member variables of this type.
//
// Notice that this must be done before %template below, SWIG must know about
// all features attached to the type before dealing with it.
%naturalvar std::optional< T >;

// Even although we're not going to really use them, we must still name the
// exported template instantiation, otherwise SWIG would give it an
// auto-generated name starting with SWIGTYPE which would be even uglier.
%template(OptT) std::optional< T >;

%enddef

// This macro should be used for simple types that can be represented as
// Nullable<T> in C#.
//
// It exists in 2 versions: normal one for native C# code and special dumbed
// down version used for COM clients such as VBA.
#ifndef USE_OBJECT_FOR_SIMPLE_OPTIONAL_VALUES

%define DEFINE_OPTIONAL_SIMPLE(OptT, T, defValue)

DEFINE_OPTIONAL_HELPER(OptT, T)

// Define the type we want to use in C#.
%typemap(cstype) std::optional< T >, const std::optional< T > & "$typemap(cstype, T)?"

// This typemap is used to convert C# OptT to the handler passed to the
// intermediate native wrapper function. Notice that it assumes that the OptT
// variable is obtained from the C# variable by prefixing it with "opt", this
// is important for the csvarin typemap below which uses "optvalue" because the
// property value is accessible as "value" in C#.
%typemap(csin,
         pre="    OptT opt$csinput = $csinput.HasValue ? new OptT($csinput.Value) : new OptT();"
         ) std::optional< T >, const std::optional< T >& "$csclassname.getCPtr(opt$csinput)"

// This is used for functions returning optional values.
%typemap(csout, excode=SWIGEXCODE) std::optional< T >, const std::optional< T >& {
    OptT optvalue = new OptT($imcall, $owner);$excode
    if (optvalue.has_value())
      return optvalue.value();
    else
      return defValue;
  }

// This code is used for the optional-valued properties in C#.
%typemap(csvarin, excode=SWIGEXCODE2) std::optional< T >, const std::optional< T >& %{
    set {
      OptT optvalue = value.HasValue ? new OptT(value.Value) : new OptT();
      $imcall;$excode
    }%}
%typemap(csvarout, excode=SWIGEXCODE2) std::optional< T >, const std::optional< T >& %{
    get {
      OptT optvalue = new OptT($imcall, $owner);$excode
      if (optvalue.has_value())
          return optvalue.value();
      else
          return defValue;
    }%}

%enddef

#else // COM version of the macro

%define DEFINE_OPTIONAL_SIMPLE(OptT, T, defValue)

DEFINE_OPTIONAL_HELPER(OptT, T)

%typemap(cstype) std::optional< T >, const std::optional< T > & "object"

// Note: the test for the input value being empty (in the sense of VT_EMPTY) is
// done using ToString() because checking "is $typemap(cstype, T)" doesn't work
// for enum types and there doesn't seem to be any other way to generically
// check if the input argument contains a value or not.
%typemap(csin,
         pre="    OptT opt$csinput = $csinput.ToString().Length > 0 ? new OptT(($typemap(cstype, T))$csinput) : new OptT();"
        ) std::optional< T >, const std::optional< T >& "$csclassname.getCPtr(opt$csinput)"

%typemap(csout, excode=SWIGEXCODE) std::optional< T >, const std::optional< T >& {
    OptT optvalue = new OptT($imcall, $owner);$excode
    if (optvalue.has_value())
      return optvalue.value();
    else
      return defValue;
  }

// No need to define csvar{in,out} as we don't use properties for
// optional-valued values anyhow because VBA doesn't handle them neither.

%enddef

#endif // normal/COM versions of DEFINE_OPTIONAL_SIMPLE

%define DEFINE_OPTIONAL_CLASS_HELPER(OptT, T)

DEFINE_OPTIONAL_HELPER(OptT, T)

%typemap(cstype) std::optional< T >, const std::optional< T > & "$typemap(cstype, T)"

%typemap(csin,
         pre="    OptT opt$csinput = $csinput != null ? new OptT($csinput): new OptT();"
         ) std::optional< T >, const std::optional< T >& "$csclassname.getCPtr(opt$csinput)"

%typemap(csout, excode=SWIGEXCODE) std::optional< T >, const std::optional< T >& {
    OptT optvalue = new OptT($imcall, $owner);$excode
    if (optvalue.has_value())
      return optvalue.value();
    else
      return null;
  }

%typemap(csvarin, excode=SWIGEXCODE2) std::optional< T >, const std::optional< T >& %{
    set {
      OptT optvalue = value != null ? new OptT(value) : new OptT();
      $imcall;$excode
    }%}

%typemap(csvarout, excode=SWIGEXCODE2) std::optional< T >, const std::optional< T >& %{
    get {
      OptT optvalue = new OptT($imcall, $owner);$excode
      return optvalue.has_value() ? optvalue.value() : null;
    }%}

%typemap(csdirectorin) std::optional< T >, const std::optional< T >& "$typemap(csdirectorin, T*)"

%enddef

// This macro should be used for optional classes which are represented by
// either a valid object or a default value in C#.
//
// Its arguments are the scope in which the class is defined (either a
// namespace or, possibly, a class name if this is a nested class) and the
// unqualified name of the class, the name of the exported optional type is
// defined by the third argument.
// Default value must be defined as forth argument, null by default
%define DEFINE_OPTIONAL_CLASS(scope, classname, name)

DEFINE_OPTIONAL_CLASS_HELPER(name, scope::classname)

%enddef