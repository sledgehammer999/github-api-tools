# GitHub API Tools

This is a collection of helpful tools that operate using the GitHuB APIs.

Each subfolder contains a separate tool.

## Compilation

The tools are known to build on Linux and on Windows via MinGW.

You will need a recent compiler and standard C++ library, with at least decent C++17 support.
For example, currently, the tools build on Ubuntu 20.04 (which uses GCC 9.x) but not on 18.04 with GCC 8.x.

Similarly, compilation may fail with older MinGW version such as 8.1, which comes bundled with Qt 5.15.
Use a recent version via MSYS2 instead.

Configure and build all tools:

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To build only one or some tools (more than one may be specified, separated by spaces):

```sh
cmake --build build --target <some_tool>
```

### Available configure-time options

* `GH_API_TOOLS_VERBOSE_CONFIGURE` (default: `OFF`): Show information about PACKAGES_FOUND and PACKAGES_NOT_FOUND in the configure output (only useful for debugging the CMake build scripts)

## Tools

### AmendTitleAndApplyLabel

This tool fetches issues whose titles match the supplied regexes, removes the matched regex from the title and applies a label to the issue.
Each regex has an associated label. If the label doesn't exist it will be created and will have a red color set.

Help output:

```txt
This program applies the provided regex on each issue title. If there is a regex match the issue title is renamed without the matched part and the provided label is applied to it too.
Options:
  --help                 Show this help message

Required:
  --repo-owner arg       Set the repo owner (github repos are in the format
                         owner/name)
  --repo-name arg        Set the repo name (github repos are in the format
                         owner/name)
  --auth-token arg       Set your Personal Access Token (OAuth token might work
                         too)
  --user-agent arg       Set the user-agent. Ideally set an email so GitHub can
                         contact you if something is wrong.
  --regex arg            Set the regex to apply on the issue title. You can
                         pass this argument multiple times. It uses the
                         ECMAScript grammar and it is case insensitive. The
                         number of regexes and the number of labels provided
                         must be equal.
  --label arg            Set the label to apply on the regex matched issue. You
                         can pass this argument multiple times. The number of
                         regexes and the number of labels provided must be
                         equal.

Optional:
  --dry-run              Don't perform any changes/mutations on the given repo.
                         Perform only the queries and print relevant
                         information.
```

#### Dependencies

* C++17
* Boost headers:
  * Boost.Beast
  * Boost.String Algo (only for boost::iequals())
* Boost.Program Options
* OpenSSL
* zlib (transitive dependency of OpenSSL)
* [nlohmann/json](https://github.com/nlohmann/json) (included in repo)

### MassCloseOldIssues

This tool closes issues that haven't been until the cutoff timepoint. It can additionally apply a label on the matching issues, leave a comment and lock them.</br>
It can also skip issues that have certain labels.

Help output:

```txt
$ ./mass_close_old_issues.exe --help
This program closes issues that haven't been updated until the set time point.
Options:
  --help                  Show this help message

Required:
  --repo-owner arg        Set the repo owner (github repos are in the format
                          owner/name)
  --repo-name arg         Set the repo name (github repos are in the format
                          owner/name)
  --auth-token arg        Set your Personal Access Token (OAuth token might
                          work too)
  --user-agent arg        Set the user-agent. Ideally set an email so GitHub
                          can contact you if something is wrong.
  --cutoff-timepoint arg  Issues that haven't been updated until the timepoint
                          are closed. The timepoint must be a UTC extended
                          format ISO-8601 string.

Optional:
  --apply-label arg       Label to apply to issues that are going to be closed.
                          Label will be created if needed.
  --comment arg           Leave a comment in the issues that are going to be
                          closed.
  --skip-label arg        Issues with this label are excluded from being
                          closed. You can pass this argument multiple times.
  --lock                  Lock the issues in addition to closing them.
  --dry-run               Don't perform any changes/mutations on the given
                          repo. Perform only the queries and print relevant
                          information.
```

#### Dependencies

* C++17
* Boost headers:
  * Boost.Beast
  * Boost.String Algo (only for boost::iequals())
* Boost.Program Options
* OpenSSL
* zlib (transitive dependency of OpenSSL)
* [nlohmann/json](https://github.com/nlohmann/json) (included in repo)
* [HowardHinnant/date](https://github.com/HowardHinnant/date) (included in repo)

## License

MIT. See `LICENSE` file.
