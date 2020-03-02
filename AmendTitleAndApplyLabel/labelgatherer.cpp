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

#include "labelgatherer.h"

#include <iostream>

#include <nlohmann/json.hpp>

#include "postdownloader.h"
#include "programoptions.h"

using json = nlohmann::json;

LabelGatherer::LabelGatherer(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions,
                           std::unordered_map<std::string, std::string> &labels,
                           std::string &error)
    : m_ioc(ioc)
    , m_ctx(ctx)
    , m_programOptions(programOptions)
    , m_labels(labels)
    , m_error(error)
    , m_body1part(generateBody1Part())
    , m_body2part(") { nodes { id name } pageInfo { endCursor hasNextPage } } } }\" }")
{
    m_error.clear();
}

void LabelGatherer::run()
{
    // Launch the asynchronous operation
    std::make_shared<PostDownloader>(m_ioc, m_ctx, m_programOptions, m_body1part + m_body2part,
                                     beast::bind_front_handler(&LabelGatherer::onFinishedPage, this)
                                     )->run();

    // Run the I/O service. The call will return when
    // the get operation is complete.
    m_ioc.run();
}

void LabelGatherer::onFinishedPage(std::shared_ptr<PostDownloader> downloader)
{
    if (!downloader->error().empty()) {
        m_error = downloader->error();
        return;
    }

    if (downloader->response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(downloader->response().base().result_int());
        return;
    }

    gatherLabels(downloader->response().body());

    if (m_error.empty() && m_hasNext) {
        const std::string body = m_body1part + ", after:\\\"" + m_cursor + "\\\"" + m_body2part;
        std::make_shared<PostDownloader>(m_ioc, m_ctx, m_programOptions, body,
                                         beast::bind_front_handler(&LabelGatherer::onFinishedPage, this)
                                         )->run();

        std::cout << "Downloading next Labels cursor: " << m_cursor << std::endl;
    }

    // `downloader` will go out-of-scope and the pointer object will be deleted (should be the last shared_ptr here)
}

void LabelGatherer::gatherLabels(std::string_view response)
{
    try {
        const json data = json::parse(response);
        if (data.contains("errors")) {
            m_error = "The last API call returned an error:\n" + data.dump();
            return;
        }

        if (m_repoId.empty()) {
            const json repo = data["data"]["repository"];
            m_repoId = repo["id"].get<std::string>();
        }

        const json nodes = data["data"]["repository"]["labels"]["nodes"];
        for (const auto &node : nodes) {
            const std::string id = node["id"].get<std::string>();
            const std::string name = node["name"].get<std::string>();
            m_labels[name] = id;
        }

        const json pageinfo = data["data"]["repository"]["labels"]["pageInfo"];
        m_hasNext = pageinfo["hasNextPage"].get<bool>();
        m_cursor = pageinfo["endCursor"].get<std::string>();
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

std::string LabelGatherer::generateBody1Part()
{
    return "{\"query\": \"query { repository(owner:\\\"" +
            m_programOptions.repoOwner +
            "\\\", name:\\\"" +
            m_programOptions.repoName +
            "\\\") { id labels(first:100";
}

std::string LabelGatherer::repoId()
{
    return m_repoId;
}
