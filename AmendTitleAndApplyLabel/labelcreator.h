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

#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;

class ProgramOptions;
class PostDownloader;

class LabelCreator
{
public:
    // The passed arguments must outlive the class instance
    explicit LabelCreator(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions, std::string_view repoID,
                          std::unordered_map<std::string, std::vector<std::vector<int>::size_type>> &labelsToCreate,
                          std::string &error);

    void run();

private:
    void onFinishedPage(std::shared_ptr<PostDownloader> downloader);

    void gatherLabelIDs(std::string_view response);
    std::string makeLabelAlias(const int counter, const std::string &name, std::string &alias);

    net::io_context &m_ioc;
    ssl::context &m_ctx;
    const ProgramOptions &m_programOptions;
    std::unordered_map<std::string, std::vector<std::vector<int>::size_type>> &m_labelsToCreate;
    const std::string m_repoID;
    std::string &m_error;

    // Alias, label-name(= as KEY to m_labelsToCreate)
    std::unordered_map<std::string, std::string> m_labelAliases;
};
