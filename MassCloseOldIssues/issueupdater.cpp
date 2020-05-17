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

#include "postdownloader.h"
#include "programoptions.h"

using json = nlohmann::json;

IssueUpdater::IssueUpdater(const ProgramOptions &programOptions, PostDownloader &downloader,
                           const std::vector<std::string> &issues,
                           std::string_view labelID, std::string &error)
    : m_programOptions(programOptions)
    , m_downloader(downloader)
    , m_issues(issues)
    , m_labelID(labelID)
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
    for (const auto &issueID : m_issues) {
        if (!m_programOptions.comment.empty())
            buffer << makeCommentAlias(counter, issueID);

        if (!m_labelID.empty())
            buffer << makeLabelAlias(counter, issueID);

        buffer << makeCloseAlias(counter, issueID);

        if (m_programOptions.lock)
            buffer << makeLockAlias(counter, issueID);

        ++counter;
    }
    buffer << end;

    m_downloader.setFinishedHandler(beast::bind_front_handler(&IssueUpdater::onFinishedPage, this));
    m_downloader.setRequestBody(buffer.str());
    m_downloader.run();
    m_downloader.setFinishedHandler(FinishedHandler{});
}

void IssueUpdater::onFinishedPage()
{
    if (!m_downloader.error().empty()) {
        m_error = m_downloader.error();
        return;
    }

    if (m_downloader.response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(m_downloader.response().base().result_int());
        return;
    }

    checkResponse(m_downloader.response().body());
}

void IssueUpdater::checkResponse(std::string_view response)
{
    try {
        const json data = json::parse(response);
        if (data.contains("errors")) {
            m_error = "The last API call returned an error:\n" + data.dump();
            return;
        }

        const json issues = data["data"];
        std::cout << "Updated " << countIssues(issues.size()) << " issues" << std::endl;
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

std::string IssueUpdater::makeCommentAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": addComment(input: {subjectId:\\\"";
    const std::string part2 = "\\\", body:\\\"";
    const std::string part3 = "\\\"}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "comment" << counter << part1 << issueID << part2 << m_programOptions.comment << part3;

    return buffer.str();
}

std::string IssueUpdater::makeLabelAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": addLabelsToLabelable(input: {labelableId:\\\"";
    const std::string part2 = "\\\", labelIds:[";
    const std::string part3 = "]}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "label" << counter << part1 << issueID << part2 << "\\\"" << m_labelID << "\\\"" << part3;

    return buffer.str();
}

std::string IssueUpdater::makeCloseAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": closeIssue(input: {issueId:\\\"";
    const std::string part2 = "\\\"}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "close" << counter << part1 << issueID << part2;

    return buffer.str();
}

std::string IssueUpdater::makeLockAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": lockLockable(input: {lockableId:\\\"";
    const std::string part2 = "\\\"}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "lock" << counter << part1 << issueID << part2;

    return buffer.str();
}

std::size_t IssueUpdater::countIssues(std::size_t responseItems) const
{
    std::size_t size = 1;

    if (!m_programOptions.comment.empty())
        ++size;

    if (!m_labelID.empty())
        ++size;

    if (m_programOptions.lock)
        ++size;

    return responseItems / size;
}
