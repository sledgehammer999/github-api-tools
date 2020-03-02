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

#include "issueupdater.h"

#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "issueatrributes.h"
#include "postdownloader.h"
#include "programoptions.h"

using json = nlohmann::json;

IssueUpdater::IssueUpdater(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions,
                           const std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &issues,
                           std::string &error)
    : m_ioc(ioc)
    , m_ctx(ctx)
    , m_programOptions(programOptions)
    , m_issues(issues)
    , m_error(error)
{
    m_error.clear();
}

void IssueUpdater::run()
{
    // Sample QraphQL string for the mutation with one alias named 'issue0'
    // "{\"query\": \"mutation UpdateIssue { issue0: updateIssue(input: {id:\\\"ISSUE-ID\\\", title:\\\"TITLE\\\", labelIds:[\\\"ID0\\\", \\\"ID1\\\"]}) { } }\"}"
    const std::string start = "{\"query\": \"mutation UpdateIssue { ";
    const std::string end = "}\"}";

    int counter = 0;
    std::ostringstream buffer;
    buffer << start;
    for (const auto &pair : m_issues) {
        const auto &subIssues = pair.second;
        for (const auto &attr : subIssues) {
            buffer << makeIssueAlias(counter, attr);
            ++counter;
        }
    }
    buffer << end;

    // Launch the asynchronous operation
    std::make_shared<PostDownloader>(m_ioc, m_ctx, m_programOptions, buffer.str(),
                                     beast::bind_front_handler(&IssueUpdater::onFinishedPage, this)
                                     )->run();

    // Run the I/O service. The call will return when
    // the get operation is complete.
    m_ioc.run();
}

void IssueUpdater::onFinishedPage(std::shared_ptr<PostDownloader> downloader)
{
    if (!downloader->error().empty()) {
        m_error = downloader->error();
        return;
    }

    if (downloader->response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(downloader->response().base().result_int());
        return;
    }

    gatherIssues(downloader->response().body());

    // `downloader` will go out-of-scope and the pointer object will be deleted (should be the last shared_ptr here)
}

void IssueUpdater::gatherIssues(std::string_view response)
{
    try {
        const json data = json::parse(response);
        if (data.contains("errors")) {
            m_error = "The last API call returned an error:\n" + data.dump();
            return;
        }

        const json issues = data["data"];
        std::cout << "Updated " << issues.size() << " issues" << std::endl;
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

std::string IssueUpdater::makeIssueAlias(const int counter, const IssueAttributes &attr)
{
    const std::string part1 = ": updateIssue(input: {id:\\\"";
    const std::string part2 = "\\\", title:\\\"";
    const std::string part3 = "\\\", labelIds:[";
    const std::string part4 = "]}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "issue" << counter << part1 << attr.ID << part2 << attr.title << part3 << makeLabelArray(attr.labelIDs) << part4;

    return buffer.str();
}

std::string IssueUpdater::makeLabelArray(const std::vector<std::string> &labelIDs)
{
    if (labelIDs.empty())
        return {};

    std::ostringstream buffer;
    for (auto i = labelIDs.cbegin(); i != --labelIDs.cend(); ++i)
        buffer << "\\\"" << *i << "\\\"" << ", ";
    buffer << "\\\"" << labelIDs.back() << "\\\"";

    return buffer.str();
}
