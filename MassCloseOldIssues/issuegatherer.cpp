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

#include <boost/algorithm/string/predicate.hpp>

#include "HowardHinnant/date.h"

#include "postdownloader.h"
#include "programoptions.h"

namespace {
    using timePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
    bool parseISOTimePoint(const std::string &timepoint, timePoint &tp)
    {
        std::istringstream in{timepoint};
        in >> date::parse("%FT%TZ", tp);
        return !in.fail();
    }

}

IssueGatherer::IssueGatherer(const ProgramOptions &programOptions, PostDownloader &downloader,
                           std::vector<std::string> &issues,
                           std::string &error)
    : m_programOptions(programOptions)
    , m_downloader(downloader)
    , m_issues(issues)
    , m_error(error)
    , m_body1part(generateBody1Part())
    , m_body2part(") { nodes { id createdAt updatedAt labels(first:100){ nodes { name } } } pageInfo { endCursor hasNextPage } } } }\" }")
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
            const std::string createdAt = node["createdAt"].get<std::string>();
            timePoint createdTimepoint;
            if (!parseISOTimePoint(createdAt, createdTimepoint))
                continue;

            const std::string updatedAt = node["updatedAt"].get<std::string>();
            timePoint updatedTimepoint;
            if (!parseISOTimePoint(updatedAt, updatedTimepoint))
                continue;

            if (createdTimepoint >= m_programOptions.cutoffTimePoint) {
                m_hasNext = false;
                return;
            }

            if (updatedTimepoint >= m_programOptions.cutoffTimePoint)
                continue;

            std::vector<std::string> labels = gatherLabels(node["labels"]["nodes"]);
            // Check if labels match

            bool labelMatch = false;
            for (const auto &label : m_programOptions.labelList) {
                const auto pred = [&label](const std::string &labelTest)
                {
                    return boost::algorithm::iequals(label, labelTest);
                };

                if (std::any_of(labels.cbegin(), labels.cend(), pred)) {
                    labelMatch = true;
                    break;
                }
            }

            if (labelMatch)
                continue;

            m_issues.emplace_back(node["id"].get<std::string>());
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

std::vector<std::string> IssueGatherer::gatherLabels (const json &LabelsNodes)
{
    std::vector<std::string> labels;

    for (const auto &node : LabelsNodes)
        labels.emplace_back(node["name"].get<std::string>());

    return labels;
}

std::string IssueGatherer::generateBody1Part()
{
    return "{\"query\": \"query { repository(owner:\\\"" +
            m_programOptions.repoOwner +
            "\\\", name:\\\"" +
            m_programOptions.repoName +
            "\\\") { issues(first:100, states:OPEN, orderBy:{field:CREATED_AT, direction:ASC}";
}
