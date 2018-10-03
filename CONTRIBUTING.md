# Contributing code

## Coding style
- Use the provided clang-format file to correctly format the code
  - A patched version of clang-format should be used (until [this patch has been integrated into clang-format](https://reviews.llvm.org/D44609))
  - clang-format version 7.0.0 (with the patch applied: [clang-7.0.0-BraceWrappingBeforeLambdaBody.patch](clang-7.0.0-BraceWrappingBeforeLambdaBody.patch))
  - Precompiled clang-format can be provided for windows and macOS, on demand
- Always declare function parameters as const (so they cannot be changed inside the function). Except for references, movable objects and pointers that are mutable
- Always put const to the right of the type

## Submit code
- Create a new branch starting from '*dev*' branch
  - Name it '*issue/#*' if the code is linked to a github issue (replace '*#*' with the github ticket number)
  - Name it '*task/taskName*' if the code is not a github issue (replace '*taskName*' with a clear lowerCamelCase name for the code)
- Add the code in that branch
- Don't forget to update any impacted CHANGELOG file(s)
- Run the *fix_files.sh* script
- Start a github pull request
