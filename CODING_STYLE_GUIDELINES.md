# Coding style and guidelines
##### Revision 4
This file describes the coding style and guidelines used by this repository, which must be followed for any new code.

## Coding style and guidelines
### Naming conventions
- Variables, functions, methods, class members, namespaces, files: lowerCamelCase
- Classes, structs, enum classes, enum values, aliases, constexpr, static class variables: UpperCamelCase
- _C-style defines_: ALL_CAPS_WITH_UNDERSCORE_TO_SEPARATE_WORDS
- Class member names should start with an underscore
- Static variables should start with s_
- When enumerating a map/unordered_map using a _for loop_, name the iterator loop variable _somethingKV_ (_key/value_)
- Always use the full namespace for types used to declare observer and delegate methods in public APIs, so the user overriding the methods doesn't have to type all of them manually

### C-style
- Prohibit _C-style cast_, use _static_cast_ instead
- Prohibit _C-style types_ (int, long, char), use _cstdint types_ instead (std::int32_t, std::int8_t)
- Prohibit _typedef_ keyword, use _using_ instead
- Prohibit _C-style enums_, use _enum class_ instead

### Declaration and Initialization
- Always initialize non static data members (NSDM) except when impossible (when the data member is a reference), using brace initialization
- Always initialize structs fields using brace initialization to prevent UB in cases like this:
```c++
// Start with a struct defined like this
struct Foo
{
	int a;
};

// Declare a variable bar of type Foo using aggregate initialization
auto const bar = Foo{5};

// At a later time, a new field is added to Foo so that it's now defined like this
struct Foo
{
	int a;
	int b;
};

// !!! The previously declared bar variable will still compile, as it is using aggregate initialization, but it will now have an unitialized field without knowing it !!!

// No such UB would have happened if Foo was defined like this
struct Foo
{
	int a{0};
	int b{0};
};
```
- Always specify the type to the right of the = sign when declaring a variable (and always use the = sign [for the sake of consistency](https://youtu.be/xnqTKD8uD64?t=2381)). Since c++17 and [dcl.init], there is no downside to this rule.
- [Always use _auto_ for variables declaration](https://youtu.be/xnqTKD8uD64?t=1808) as it's impossible to have an uninitialized variable (possible since c++17 thanks to [[dcl.init] update in P0135R1: Guaranteed copy elision](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0135r1.html))
- Always put _const_ to the right of the type (for consistency and prevent ambiguity when used with auto and pointers)
- Add _virtual_ and _override_ keywords when _overriding_ a _virtual method_ for clarity and consistency
- Always declare function parameters as const (so they cannot be changed inside the function), except for obvious reasons (references, movable objects and pointers that are mutable)
- Always declare variables as const if they never change

### Header inclusion order
- Current library public headers (using "")
- Current library private headers (using "")
- Other non-standard libraries public headers (using <>)
- Standard libraries public headers (using <>)

Separate each category with an empty line feed.

Example:
```c++
#include "la/avdecc/avdecc.hpp"

#include "entity.hpp"

#include <windows.h>
#include <boost/boost.hpp>

#include <vector>
```


### Unsorted
- Don't use function parameter to return a value (except for in/out _objects_, but never for _simple types_), use the return value of the function (use pair/tuple/struct/optional if required). Except if absolutely required.
- [A non-owning pointer passed to a function should be declared as _&_ if required and _*_ if optional](https://youtu.be/xnqTKD8uD64?t=956)
- [Never pass a shared_pointer directly if the function does not participate in ownership (taking a count in the reference)](https://youtu.be/xnqTKD8uD64?t=1034)
- [Never pass to a function a dereferenced non local shared_pointer (sptr.get() or *sptr, where sptr is global or is class member that can be changed by another thread or sub-function). Take a reference count before dereferencing.](https://youtu.be/xnqTKD8uD64?t=1569)
- Always handle all cases in a _switch-case statement_ (make use of the _default keyword_ if required)
- Always scope statement blocs with curling braces when it's optional (_if_, _else_, ...)

## clang-format
Use the provided [clang-format file](.clang-format) to correctly format the code.

- A patched version of the clang-format binary is required ([fixing issues in baseline code](https://reviews.llvm.org/D44609))
- clang-format version 7.0.0 (with the patch applied: [clang-7.0.0-BraceWrappingBeforeLambdaBody.patch](clang-7.0.0-BraceWrappingBeforeLambdaBody.patch))
- Precompiled clang-format with patch applied, for windows and macOS, [can be found here](http://www.kikisoft.com/Hive/clang-format)

## clang-tidy
TBD / WIP
