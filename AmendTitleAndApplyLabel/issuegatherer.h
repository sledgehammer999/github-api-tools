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

#pragma once

#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <nlohmann/json.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;

using json = nlohmann::json;

struct IssueAttributes;
class ProgramOptions;
class PostDownloader;

class IssueGatherer
{
public:
    // The passed arguments must outlive the class instance
    explicit IssueGatherer(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions,
                           std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &issues,
                           std::string &error);

    void run();

private:
    void onFinishedPage(std::shared_ptr<PostDownloader> downloader);

    bool matchAndAmendTitle(const std::regex &regex, std::string &title);
    std::vector<std::string> gatherLabels (const json &LabelsNodes);
    void gatherIssues(std::string_view response);
    std::string generateBody1Part();


    net::io_context &m_ioc;
    ssl::context &m_ctx;
    const ProgramOptions &m_programOptions;
    std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &m_issues;
    std::string &m_error;


    const std::string m_body1part;
    const std::string m_body2part;
    std::string m_cursor;
    bool m_hasNext = false;
};
