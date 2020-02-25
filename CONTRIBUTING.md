# Contributing code

## Coding style
Read and follow the coding style and guidelines [from this file](CODING_STYLE_GUIDELINES.md).

## Submit code
- Create a new branch starting from '*dev*' branch
  - Name it '*issue/#*' if the code is linked to a github issue (replace '*#*' with the github ticket number)
  - Name it '*task/taskName*' if the code is not a github issue (replace '*taskName*' with a clear lowerCamelCase name for the code)
- Add the code in that branch
- Don't forget to update any impacted CHANGELOG file(s)
- Run the *fix_files.sh* script
- Start a github pull request

## To run `fix_files.sh` on MAC

* Update `bash` from 3.x to latest (5.x)

```bash
brew install bash
echo "/usr/local/bin/bash" | sudo tee -a /etc/shells
chsh -s /usr/local/bin/bash
```

* Install latest `clang-format`

```bash
brew install romansavrulin/clang-format/clang-format-lambda --HEAD
```