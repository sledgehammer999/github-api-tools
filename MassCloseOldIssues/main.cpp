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

#include <iostream>

#include "issuegatherer.h"
#include "issueupdater.h"
#include "labelcreator.h"
#include "labelgatherer.h"
#include "postdownloader.h"
#include "programoptions.h"

int main(int argc, char *argv[])
{
    std::string error;
    const ProgramOptions options = ProgramOptions::parseCmdLine(argc, argv, error);
    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    std::vector<std::string> issues;

    PostDownloader downloader(options);
    if (!downloader.error().empty()) {
        std::cout << downloader.error() << std::endl;
        return -1;
    }

    // The arguments must outlive the class instance
    IssueGatherer issueGatherer{options, downloader, issues, error};
    // run() runs the io_context and blocks
    issueGatherer.run();

    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    if (issues.size() == 0) {
        std::cout << "No issues were found" << std::endl;
        return 0;
    }

    std::cout << issues.size() << " issues were found" << std::endl;

    std::string labelID;
    if (!options.applyLabel.empty()) {
        // The arguments must outlive the class instance
        LabelGatherer labelGatherer{options, downloader, error};
        // run() runs the io_context and blocks
        labelGatherer.run();

        if (!error.empty()) {
            std::cout << error << std::endl;
            return -1;
        }

        if (labelGatherer.labelId().empty()) {
            std::cout << "Need to create label \'" + options.applyLabel + "\'" << std::endl;

            if (options.dryRun) {
                std::cout << "This is a dry run, stopping now." << std::endl;
                return 0;
            }

            // The arguments must outlive the class instance
            LabelCreator lblCreator{options, downloader, labelGatherer.repoId(), error};
            // run() runs the io_context and blocks
            lblCreator.run();

            if (!error.empty()) {
                std::cout << error << std::endl;
                return -1;
            }

            if (lblCreator.labelId().empty()) {
                std::cout << "The label wasn't created" << std::endl;
                return -1;
            }

            labelID = lblCreator.labelId();
        }
        else {
            labelID = labelGatherer.labelId();
        }
    }

    if (options.dryRun) {
        std::cout << "This is a dry run, stopping now." << std::endl;
        return 0;
    }

    // The arguments must outlive the class instance
    IssueUpdater issueUpdater{options, downloader, issues, labelID, error};
    // run() runs the io_context and blocks
    issueUpdater.run();

    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    return 0;
}
