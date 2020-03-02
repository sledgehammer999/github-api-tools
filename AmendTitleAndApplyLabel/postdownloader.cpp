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

#include "postdownloader.h"

//#include <iostream>

#include "programoptions.h"

namespace {
    using namespace std::literals;
    const std::string_view HOST = "api.github.com"sv;
    const std::string_view TARGET = "/graphql"sv;
    const std::string_view PORT = "443"sv;
}

PostDownloader::PostDownloader(net::io_context &ioc, ssl::context &ctx, const ProgramOptions &programOptions, std::string_view body, FinishedHandler hanlder)
    : m_resolver(ioc)
    , m_stream(ioc, ctx)
    , m_body(body)
    , m_programOptions(programOptions)
    , m_finishedHanlder(hanlder)
{
}

void PostDownloader::run()
{
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(m_stream.native_handle(), TARGET.data())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        m_error = "Failed SNI: " + ec.message();
        return;
    }

    const std::string GITHUB_TOKEN = "token " + m_programOptions.authToken;
    // Set up an HTTP POST request message
    m_request.method(http::verb::post);
    m_request.target(TARGET);
    m_request.set(http::field::host, HOST);
    m_request.set(http::field::user_agent, m_programOptions.userAgent);
    m_request.set(http::field::authorization, GITHUB_TOKEN);
    m_request.set(http::field::accept, "application/vnd.github.bane-preview+json"); // Allows to use the `createLabel` mutation, because it is in "preview" API
    m_request.body() = m_body;
    m_request.prepare_payload();

    /*std::cout << "REQUEST:" << std::endl;
    std::cout << m_request.base() << std::endl;
    std::cout << m_request.body() << std::endl;*/

    // Look up the domain name
    m_resolver.async_resolve(HOST, PORT,
                             beast::bind_front_handler(
                                 &PostDownloader::onResolve,
                                 shared_from_this()));
}

void PostDownloader::onResolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if(ec) {
        m_error = "Failed resolve: " + ec.message();
        return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_stream).async_connect(
                results,
                beast::bind_front_handler(
                    &PostDownloader::onConnect,
                    shared_from_this()));
}

void PostDownloader::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if(ec) {
        m_error = "Failed connect: " + ec.message();
        return;
    }

    // Perform the SSL handshake
    m_stream.async_handshake(
                ssl::stream_base::client,
                beast::bind_front_handler(
                    &PostDownloader::onHandshake,
                    shared_from_this()));
}

void PostDownloader::onHandshake(beast::error_code ec)
{
    if(ec) {
        m_error = "Failed handshake: " + ec.message();
        return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(m_stream, m_request,
                      beast::bind_front_handler(
                          &PostDownloader::onWrite,
                          shared_from_this()));
}

void PostDownloader::onWrite(beast::error_code ec, std::size_t)
{
    if(ec) {
        m_error = "Failed write: " + ec.message();
        return;
    }

    // Receive the HTTP response
    http::async_read(m_stream, m_buffer, m_response,
                     beast::bind_front_handler(
                         &PostDownloader::onRead,
                         shared_from_this()));
}

void PostDownloader::onRead(beast::error_code ec, std::size_t)
{
    if(ec) {
        m_error = "Failed read: " + ec.message();
        return;
    }

    // Write the message to standard out
    /*std::cout << "RESPONSE:" << std::endl;
    std::cout << m_response.base() << std::endl;
    std::cout << m_response.body() << std::endl;*/

    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    m_stream.async_shutdown(
                beast::bind_front_handler(
                    &PostDownloader::onShutdown,
                    shared_from_this()));
}

void PostDownloader::onShutdown(beast::error_code ec)
{
    if(ec == net::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }

    if(ec) {
        m_error = "Failed shutdown: " + ec.message();
        return;
    }

    // If we get here then the connection is closed gracefully
    // Inform the caller that we finished;
    m_finishedHanlder(shared_from_this());
}

std::string_view PostDownloader::error()
{
    return m_error;
}

const http::response<http::string_body>& PostDownloader::response()
{
    return m_response;
}
