/* MIT License

Copyright (c) 2020 sledgehammer999 <hammered999@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "programoptions.h"

#include <sstream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

ProgramOptions ProgramOptions::parseCmdLine(int &argc, char *argv[], std::string &error)
{
    ProgramOptions opt;
    po::options_description desc("This program applies the provided regex on each issue title. "
                                 "If there is a regex match the issue title is renamed without "
                                 "the matched part and the provided label is applied to it too.\n"
                                 "Options");
    desc.add_options()
            ("help", "Show this help message")
    ;

    po::options_description required("Required");
    required.add_options()
            ("repo-owner", po::value<std::string>(&opt.repoOwner)->required(), "Set the repo owner (github repos are in the format owner/name)")
            ("repo-name", po::value<std::string>(&opt.repoName)->required(), "Set the repo name (github repos are in the format owner/name)")
            ("auth-token", po::value<std::string>(&opt.authToken)->required(), "Set your Personal Access Token (OAuth token might work too)")
            ("user-agent", po::value<std::string>(&opt.userAgent)->required(), "Set the user-agent. Ideally set an email so GitHub can contact you if something is wrong.")
            ("regex", po::value<std::vector<std::string>>()->required(), "Set the regex to apply on the issue title. You can pass this argument multiple times. It uses the ECMAScript grammar and it is case insensitive. The number of regexes and the number of labels provided must be equal.")
            ("label", po::value<std::vector<std::string>>(&opt.labelList)->required(), "Set the label to apply on the regex matched issue. You can pass this argument multiple times. The number of regexes and the number of labels provided must be equal.")
    ;

    po::options_description optional("Optional");
    optional.add_options()
            ("dry-run", po::bool_switch(&opt.dryRun), "Don't perform any changes/mutations on the given repo. Perform only the queries and print relevant information.")
    ;

    desc.add(required);
    desc.add(optional);

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const std::exception& e) {
        error = e.what();
    }

    if (error.empty()) {
        std::vector<std::string> list = vm["regex"].as<std::vector<std::string>>();
        opt.regexList.reserve(list.size());

        for (const auto &str: list) {
            try {
                opt.regexList.emplace_back(str, std::regex::icase|std::regex::optimize);
            }
            catch (const std::exception& e) {
                error = "Regex \'" + str + "\' isn't valid. Error: " + e.what();
                break;
            }
        }
    }

    if (error.empty() && (opt.regexList.size() != opt.labelList.size()))
        error = "The number of the provided regexes and the number of the provided labels are different";

    // Always print the help message if the switch is present regardless of other errors
    if (vm.count("help")) {
        std::ostringstream stream;
        stream << desc;
        error = stream.str();
    }

    return opt;
}
