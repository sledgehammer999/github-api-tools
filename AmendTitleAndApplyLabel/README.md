AmendTitleAndApplyLabel
-----------------------

This tool fetches issues whose titles match the supplied regexes, removes the matched regex from the title and applies a label to the issue.
Each regex has an associated label. If the label doesn't exist it will be created and will have a red color set.

Help output:
```
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

Dependencies
------------
**No Qt or qmake is needed. Read the `Compilation` section.**
* C++17
* Boost.Beast
* Boost.Program Options
* Boost.String Algo (only for boost::iequals())
* [nlohmann/json](https://github.com/nlohmann/json) (included in repo)
* OpenSSL (indirectly by Boost.Beast and Boost.Asio)

Compilation
-----------
The `AmendTitleAndApplyLabel.pro` file is just there for convenience since I am very familiar with QtCreator.  
Qt or qmake isn't needed for compilation. Read the `Dependencies` section.
You need to define `BOOST_BEAST_USE_STD_STRING_VIEW` via the compiler.

License
--------

MIT. See `LICENSE` file.
