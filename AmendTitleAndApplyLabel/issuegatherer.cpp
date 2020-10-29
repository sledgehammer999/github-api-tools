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

#include "issuegatherer.h"

#include <iostream>

#include "issueattributes.h"
#include "postdownloader.h"
#include "programoptions.h"

IssueGatherer::IssueGatherer(const ProgramOptions &programOptions, PostDownloader &downloader,
                           std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &issues,
                           std::string &error)
    : m_programOptions(programOptions)
    , m_downloader(downloader)
    , m_issues(issues)
    , m_error(error)
    , m_body1part(generateBody1Part())
    , m_body2part(") { nodes { id title labels(first:100){ nodes { id } } } pageInfo { endCursor hasNextPage } } } }\" }")
{
    m_error.clear();
}

void IssueGatherer::run()
{
    m_downloader.setFinishedHandler(beast::bind_front_handler(&IssueGatherer::onFinishedPage, this));
    m_downloader.setRequestBody(m_body1part + m_body2part);
    m_downloader.run();
    m_downloader.setFinishedHandler(FinishedHandler{});
}

void IssueGatherer::onFinishedPage()
{
    if (!m_downloader.error().empty()) {
        m_error = m_downloader.error();
        return;
    }

    if (m_downloader.response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(m_downloader.response().base().result_int());
        return;
    }

    gatherIssues(m_downloader.response().body());

    if (m_error.empty() && m_hasNext) {
        const std::string body = m_body1part + ", after:\\\"" + m_cursor + "\\\"" + m_body2part;

        m_downloader.setRequestBody(body);
        m_downloader.sendRequest();

        std::cout << "Downloading next Issues cursor: " << m_cursor << std::endl;
    }
}

void IssueGatherer::gatherIssues(std::string_view response)
{
    try {
        const json data = json::parse(response);
        if (data.contains("errors")) {
            m_error = "The last API call returned an error:\n" + data.dump();
            return;
        }

        const json nodes = data["data"]["repository"]["issues"]["nodes"];
        for (const auto &node : nodes) {
            std::string title = node["title"].get<std::string>();

            for (std::vector<std::regex>::size_type i = 0; i < m_programOptions.regexList.size(); ++i) {
                if (matchAndAmendTitle(m_programOptions.regexList[i], title)) {
                    const std::string id = node["id"].get<std::string>();
                    std::vector<IssueAttributes> &subIssues = m_issues[i];

                    subIssues.emplace_back(id, title, gatherLabels(node["labels"]["nodes"]));
                    break;
                }
            }
        }

        const json pageinfo = data["data"]["repository"]["issues"]["pageInfo"];
        m_hasNext = pageinfo["hasNextPage"].get<bool>();
        m_cursor = pageinfo["endCursor"].get<std::string>();
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

bool IssueGatherer::matchAndAmendTitle(const std::regex &regex, std::string &title)
{
    if (!std::regex_search(title, regex))
        return false;

    title = std::regex_replace(title, regex, "");

    // We need to escape special characters(eg double quotes) for GraphQL
    // Strings literals need 2 levels of escape
    // Example title: Test "x"
    // Correctly escaped, it should look like this
    // when the GraphQL query string is printed: Test \\\"x\\\"
    std::string buffer;
    buffer.reserve(title.size() + 2); // reserve 2 extra characters. Usually there shouldn't be more than one pair of doublequotes
    for (auto ch : title) {
        switch (ch) {
        case '\"':
            buffer.append("\\\"");
            break;
        case '\\':
            buffer.append("\\\\");
            break;
        case '/':
            buffer.append("\\/");
            break;
        default:
            buffer.append(1, ch);
            break;
        }
    }

    title.swap(buffer);

    return true;
}

std::vector<std::string> IssueGatherer::gatherLabels (const json &LabelsNodes)
{
    std::vector<std::string> labels;

    for (const auto &node : LabelsNodes)
        labels.emplace_back(node["id"].get<std::string>());

    return labels;
}

std::string IssueGatherer::generateBody1Part()
{
    return "{\"query\": \"query { repository(owner:\\\"" +
            m_programOptions.repoOwner +
            "\\\", name:\\\"" +
            m_programOptions.repoName +
            "\\\") { issues(first:100, states:OPEN, orderBy:{field:CREATED_AT, direction:DESC}";
}
