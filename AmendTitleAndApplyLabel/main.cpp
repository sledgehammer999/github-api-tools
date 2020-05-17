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

#include <boost/algorithm/string/predicate.hpp>

#include "issueattributes.h"
#include "issuegatherer.h"
#include "issueupdater.h"
#include "labelcreator.h"
#include "labelgatherer.h"
#include "postdownloader.h"
#include "programoptions.h"


void updateLabelIDs(std::vector<IssueAttributes> &issues, std::string_view labelID)
{
    for (auto &issue : issues) {
        std::vector<std::string> &IDs = issue.labelIDs;
        auto iter = find(IDs.cbegin(), IDs.cend(), labelID);

        if (iter == IDs.cend())
            IDs.emplace_back(labelID);
    }
}

std::unordered_map<std::string, std::vector<std::vector<int>::size_type>>
updateLabelsPerRegex(std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &issues,
                     std::unordered_map<std::string, std::string> &labels,
                     const ProgramOptions &options)
{
    std::unordered_map<std::string, std::vector<std::vector<int>::size_type>> labelsToCreate;

    for (auto &pair : issues) {
        // The KEY (pair.first) of 'issues' holds the index into the regex/label vectors
        const std::string &label = options.labelList[pair.first];
        const auto pred = [&label](const auto &input)
        {
            return boost::iequals(input.first, label);
        };
        // Find if the label name exists in the repository
        auto iter = std::find_if(labels.cbegin(), labels.cend(), pred);

        // It it exists assign its ID to all affected issues
        // If not put it in a list to create it later
        if (iter != labels.cend()) {
            updateLabelIDs(pair.second, iter->second);
        }
        else {
            // Label doesn't exist.
            // We save the index for the label to refer to it later
            // Same label might be applied to multiple regexes
            auto iter2 = std::find_if(labelsToCreate.begin(), labelsToCreate.end(), pred);

            if (iter2 != labelsToCreate.cend()) {
                auto &indexVec = iter2->second;
                indexVec.emplace_back(pair.first);
            }
            else {
                labelsToCreate[label] = { pair.first };
            }
        }
    }

    return labelsToCreate;
}

int main(int argc, char *argv[])
{
    std::string error;
    const ProgramOptions options = ProgramOptions::parseCmdLine(argc, argv, error);
    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> issues;

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
        std::cout << "No issues matched any of the provided regexes" << std::endl;
        return 0;
    }

    for (const auto &pair : issues)
        std::cout << pair.second.size() << " issues matching regex at " << pair.first << " pos" << std::endl;

    std::unordered_map<std::string, std::string> labels;
    // The arguments must outlive the class instance
    LabelGatherer labelGatherer{options, downloader, labels, error};
    // run() runs the io_context and blocks
    labelGatherer.run();

    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    // Map missing label names to indexes of the `issues` maps
    // Multiple regexes might use the same label name
    std::unordered_map<std::string, std::vector<std::vector<int>::size_type>>
            labelsToCreate = updateLabelsPerRegex(issues, labels, options);

    if (labelsToCreate.size() > 0) {
        std::cout << "Need to create labels: ";
        for (const auto &pair : labelsToCreate)
            std::cout << pair.first << " ";
        std::cout << std::endl;

        if (options.dryRun) {
            std::cout << "This is a dry run, stopping now." << std::endl;
            return 0;
        }

        // The arguments must outlive the class instance
        LabelCreator lblCreator{downloader, labelGatherer.repoId(), labelsToCreate, error};
        // run() runs the io_context and blocks
        lblCreator.run();

        if (!error.empty()) {
            std::cout << error << std::endl;
            return -1;
        }

        // Now `labelsToCreate` holds the label IDs not the label names as KEY
        // Update the relevant issues with the label ID
        for (const auto &pair : labelsToCreate) {
            const auto &indexVec = pair.second;
            for (const auto &index : indexVec) {
                std::vector<IssueAttributes> &subIssues = issues[index];
                updateLabelIDs(subIssues, pair.first);
            }
        }
    }

    if (options.dryRun) {
        std::cout << "This is a dry run, stopping now." << std::endl;
        return 0;
    }

    // The arguments must outlive the class instance
    IssueUpdater issueUpdater{downloader, issues, error};
    // run() runs the io_context and blocks
    issueUpdater.run();

    if (!error.empty()) {
        std::cout << error << std::endl;
        return -1;
    }

    return 0;
}
