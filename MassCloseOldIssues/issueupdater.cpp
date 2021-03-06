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

namespace
{
    constexpr int BATCH_SIZE = 10;
}

IssueUpdater::IssueUpdater(const ProgramOptions &programOptions, PostDownloader &downloader,
                           const std::vector<std::string> &issues,
                           std::string_view labelID, std::string &error)
    : m_programOptions(programOptions)
    , m_downloader(downloader)
    , m_issues(issues)
    , m_labelID(labelID)
    , m_error(error)
    , m_hasNextBatch(m_issues.size() > 0)
{
    m_error.clear();
}

void IssueUpdater::run()
{
    if (!hasNextBatch())
        return;

    m_downloader.setFinishedHandler(beast::bind_front_handler(&IssueUpdater::onFinishedPage, this));

    json req;
    req["query"] = nextBatch();
    m_downloader.setRequestBody(req.dump());

    m_downloader.run();
    m_downloader.setFinishedHandler(FinishedHandler{});
}

std::string IssueUpdater::nextBatch()
{
    if (!hasNextBatch())
        return {};

    // Sample QraphQL string for the mutation with one alias named 'issue0'
    // "mutation UpdateIssue { comment0: : addComment(input: {subjectId:\"ID\", body:\"COMMENT\"}) { clientMutationId }
    //                         label0: addLabelsToLabelable(input: {labelableId:\"ID\", labelIds:[\"ID\"]}) { clientMutationId }
    //                         close0: closeIssue(input: {issueId:\"ID\"\"}) { clientMutationId }
    //                         lock0: lockLockable(input: {lockableId:\"ID\"}) { clientMutationId } }"

    const std::string start = "mutation UpdateIssue { ";
    const std::string end = "}";

    int counter = 0;
    std::ostringstream buffer;
    buffer << start;
    for (; ((m_issuePos < m_issues.size()) && (counter < BATCH_SIZE)); ++m_issuePos) {
        const auto &issueID = m_issues[m_issuePos];

        if (!m_programOptions.comment.empty())
            buffer << makeCommentAlias(counter, issueID);

        if (!m_labelID.empty())
            buffer << makeLabelAlias(counter, issueID);

        buffer << makeCloseAlias(counter, issueID);

        if (m_programOptions.lock)
            buffer << makeLockAlias(counter, issueID);

        ++counter;
    }

    if (m_issuePos == m_issues.size())
        m_hasNextBatch = false;

    buffer << end;
    return buffer.str();
}

bool IssueUpdater::hasNextBatch()
{
    return m_hasNextBatch;
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

    if (!m_error.empty() || !hasNextBatch())
        return;

    json req;
    req["query"] = nextBatch();
    m_downloader.setRequestBody(req.dump());
    m_downloader.sendRequest();
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
    const std::string part1 = ": addComment(input: {subjectId:\"";
    const std::string part2 = "\", body:\"";
    const std::string part3 = "\"}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "comment" << counter << part1 << issueID << part2 << m_programOptions.comment << part3;

    return buffer.str();
}

std::string IssueUpdater::makeLabelAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": addLabelsToLabelable(input: {labelableId:\"";
    const std::string part2 = "\", labelIds:[";
    const std::string part3 = "]}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "label" << counter << part1 << issueID << part2 << "\"" << m_labelID << "\"" << part3;

    return buffer.str();
}

std::string IssueUpdater::makeCloseAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": closeIssue(input: {issueId:\"";
    const std::string part2 = "\"}) { clientMutationId } ";

    std::ostringstream buffer;
    buffer << "close" << counter << part1 << issueID << part2;

    return buffer.str();
}

std::string IssueUpdater::makeLockAlias(const int counter, const std::string &issueID) const
{
    const std::string part1 = ": lockLockable(input: {lockableId:\"";
    const std::string part2 = "\"}) { clientMutationId } ";

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
