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

#include "labelcreator.h"

#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "postdownloader.h"
#include "programoptions.h"

using json = nlohmann::json;

LabelCreator::LabelCreator(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions, std::string_view repoID,
                           std::unordered_map<std::string, std::vector<std::vector<int>::size_type>> &labelsToCreate,
                           std::string &error)
    : m_ioc(ioc)
    , m_ctx(ctx)
    , m_programOptions(programOptions)
    , m_labelsToCreate(labelsToCreate)
    , m_repoID(repoID)
    , m_error(error)
{
    m_error.clear();
}

void LabelCreator::run()
{
    // Sample QraphQL string for the mutation with one alias named 'label0'
    // "{\"query\": \"mutation CreateLabel { label0: createLabel(input: {color:\\\"FF0000\\\", name:\\\"NAME\\\", repositoryId:\\\"REPO-ID\\\"}) { label { id } } }\"}"
    const std::string start = "{\"query\": \"mutation CreateLabel { ";
    const std::string end = "}\"}";

    int counter = 0;
    std::ostringstream buffer;
    buffer << start;
    for (const auto &pair : m_labelsToCreate) {
        const std::string &label = pair.first;
        std::string alias;
        buffer << makeLabelAlias(counter, label, alias);
        m_labelAliases[alias] = label;
        ++counter;
    }
    buffer << end;

    // Launch the asynchronous operation
    std::make_shared<PostDownloader>(m_ioc, m_ctx, m_programOptions, buffer.str(),
                                     beast::bind_front_handler(&LabelCreator::onFinishedPage, this)
                                     )->run();

    // Run the I/O service. The call will return when
    // the get operation is complete.
    m_ioc.run();
}

void LabelCreator::onFinishedPage(std::shared_ptr<PostDownloader> downloader)
{
    if (!downloader->error().empty()) {
        m_error = downloader->error();
        return;
    }

    if (downloader->response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(downloader->response().base().result_int());
        return;
    }

    gatherLabelIDs(downloader->response().body());

    // `downloader` will go out-of-scope and the pointer object will be deleted (should be the last shared_ptr here)
}

void LabelCreator::gatherLabelIDs(std::string_view response)
{
    try {
        const json data = json::parse(response);
        if (data.contains("errors")) {
            m_error = "The last API call returned an error:\n" + data.dump();
            return;
        }

        const json aliases = data["data"];
        // structured bindings (C++17)
        for (const auto& [alias, labelJson] : aliases.items()) {
            const std::string id = labelJson["label"]["id"].get<std::string>();
            const auto &labelName = m_labelAliases[alias];

            // Change the KEY of m_labelsToCreate from label-name to label-id
            auto node = m_labelsToCreate.extract(labelName);
            node.key() = id;
            m_labelsToCreate.insert(std::move(node));
        }
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

std::string LabelCreator::makeLabelAlias(const int counter, const std::string &name, std::string &alias)
{
    const std::string part1 = ": createLabel(input: {color:\\\"FF0000\\\", name:\\\"";
    const std::string part2 = "\\\", repositoryId:\\\"";
    const std::string part3 = "\\\"}) { label { id } } ";

    std::ostringstream buffer;
    buffer << "label" << counter;
    alias = buffer.str();
    buffer << part1 << name << part2 << m_repoID << part3;

    return buffer.str();
}
