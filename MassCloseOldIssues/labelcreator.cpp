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

LabelCreator::LabelCreator(const ProgramOptions &programOptions, PostDownloader &downloader, std::string_view repoID,
                           std::string &error)
    : m_programOptions(programOptions)
    , m_downloader(downloader)
    , m_repoID(repoID)
    , m_error(error)
{
    m_error.clear();
}

void LabelCreator::run()
{
    // Sample QraphQL string for the mutation with one alias named 'label0'
    // "mutation CreateLabel { label0: createLabel(input: {color:\"FF0000\", name:\"NAME\", repositoryId:\"REPO-ID\"}) { label { id } } }"
    const std::string start = "mutation CreateLabel { ";
    const std::string end = "}";
    const std::string body = start + makeLabelAlias(0, m_programOptions.applyLabel) + end;

    m_downloader.setFinishedHandler(beast::bind_front_handler(&LabelCreator::onFinishedPage, this));

    json req;
    req["query"] = body;
    m_downloader.setRequestBody(req.dump());

    m_downloader.run();
    m_downloader.setFinishedHandler(FinishedHandler{});
}

void LabelCreator::onFinishedPage()
{
    if (!m_downloader.error().empty()) {
        m_error = m_downloader.error();
        return;
    }

    if (m_downloader.response().base().result() != http::status::ok) {
        m_error = "The API HTTP response has status code: " + std::to_string(m_downloader.response().base().result_int());
        return;
    }

    gatherLabelID(m_downloader.response().body());
}

void LabelCreator::gatherLabelID(std::string_view response)
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
            m_labelID = labelJson["label"]["id"].get<std::string>();
            return; // there should be only one result
        }
    }
    catch (const std::exception &e) {
        m_error += "Exception: ";
        m_error += e.what();
    }
}

std::string LabelCreator::makeLabelAlias(const int counter, const std::string &name)
{
    const std::string part1 = ": createLabel(input: {color:\"FF0000\", name:\"";
    const std::string part2 = "\", repositoryId:\"";
    const std::string part3 = "\"}) { label { id } } ";

    std::ostringstream buffer;
    buffer << "label" << counter;
    buffer << part1 << name << part2 << m_repoID << part3;

    return buffer.str();
}

std::string LabelCreator::labelId() const
{
    return m_labelID;
}
