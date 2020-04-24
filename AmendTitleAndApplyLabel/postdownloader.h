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
#include <string_view>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

struct ProgramOptions;
class PostDownloader;

using FinishedHandler = std::function<void(std::shared_ptr<PostDownloader>)>;

// Performs an HTTP POST and stores the response
class PostDownloader : public std::enable_shared_from_this<PostDownloader>
{
public:
    explicit PostDownloader(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions, std::string_view body, FinishedHandler hanlder);

    // Start the asynchronous operation
    void run();

    std::string_view error() const;
    const http::response<http::string_body>& response() const;
    bool isKeptAlive() const;
    void sendAnotherRequest(std::string_view body);
    void closeConnection();

private:
    // Completion handlers
    void onResolve(beast::error_code ec, tcp::resolver::results_type results);
    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void onHandshake(beast::error_code ec);
    void onWrite(beast::error_code ec, std::size_t);
    void onRead(beast::error_code ec, std::size_t);
    void onShutdown(beast::error_code ec);

    // Private members
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_request;
    http::response<http::string_body> m_response;
    tcp::resolver m_resolver;
    beast::ssl_stream<beast::tcp_stream> m_stream;

    std::string m_error;
    std::string m_body;
    const ProgramOptions &m_programOptions;
    FinishedHandler m_finishedHanlder;
};
