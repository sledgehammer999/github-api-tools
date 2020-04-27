MassCloseOldIssues
-----------------------

This tool closes issues that haven't been until the cutoff timepoint. It can additionally apply a label on the matching issues, leave a comment and lock them.</br>
It also can skip issues that have certai labels.

Help output:
```
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
  --label arg             Issues with this label are excluded from being
                          closed. You can pass this argument multiple times.
  --lock                  Lock the issues in addition to closing them.
  --dry-run               Don't perform any changes/mutations on the given
                          repo. Perform only the queries and print relevant
                          information.
```

Dependencies
------------
**No Qt or qmake is needed. Read the `Compilation` section.**
* C++17
* Boost.Beast
* Boost.Program Options
* Boost.String Algo (only for boost::iequals())
* [nlohmann/json](https://github.com/nlohmann/json) (included in repo)
* [ HowardHinnant/date](https://github.com/HowardHinnant/date) (`date.h`, included in repo)
* OpenSSL (indirectly by Boost.Beast and Boost.Asio)

Compilation
-----------
The `MassCloseOldIssues.pro` file is just there for convenience since I am very familiar with QtCreator.</br>
Qt or qmake isn't needed for compilation. Read the `Dependencies` section.</br>
You need to define `BOOST_BEAST_USE_STD_STRING_VIEW` via the compiler. Then just compile all the `.cpp` files.</br>

License
--------

MIT. See `LICENSE` file.
