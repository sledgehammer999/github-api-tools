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

#include "HowardHinnant/date.h"

namespace po = boost::program_options;

ProgramOptions ProgramOptions::parseCmdLine(int &argc, char *argv[], std::string &error)
{
    ProgramOptions opt;
    po::options_description desc("This program closes issues that haven't been updated until the set time point.\n"
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
            ("cutoff-timepoint", po::value<std::string>()->required(), "Issues that haven't been updated until the timepoint are closed. The timepoint must be a UTC extended format ISO-8601 string.")
    ;

    po::options_description optional("Optional");
    optional.add_options()
            ("apply-label", po::value<std::string>(&opt.applyLabel), "Label to apply to issues that are going to be closed. Label will be created if needed.")
            ("comment", po::value<std::string>(&opt.comment), "Leave a comment in the issues that are going to be closed.")
            ("skip-label", po::value<std::vector<std::string>>(&opt.labelList), "Issues with this label are excluded from being closed. You can pass this argument multiple times.")
            ("lock", po::bool_switch(&opt.lock), "Lock the issues in addition to closing them.")
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
        std::string datetime = vm["cutoff-timepoint"].as<std::string>();
        std::istringstream in{datetime};
        in >> date::parse("%FT%TZ", opt.cutoffTimePoint);
        if (in.fail())
            error = "Failed to parsed the value of the cutoff-timepoint parameter";
    }

    // Always print the help message if the switch is present regardless of other errors
    if (vm.count("help")) {
        std::ostringstream stream;
        stream << desc;
        error = stream.str();
    }

    return opt;
}
