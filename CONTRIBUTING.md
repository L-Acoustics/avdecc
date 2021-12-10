# Contributing code

## Coding style
Read and follow the coding style and guidelines [from this file](CODING_STYLE_GUIDELINES.md).

## Submit code
- Create a new branch starting from '*dev*' branch
  - Name it '*issue/#*' if the code is linked to a github issue (replace '*#*' with the github ticket number)
  - Name it '*task/taskName*' if the code is not a github issue (replace '*taskName*' with a clear lowerCamelCase name for the code)
- Add the code in that branch
- Don't forget to update any impacted CHANGELOG file(s)
- Run the *scripts/bashUtils/fix_files.sh* script
- Start a github pull request
