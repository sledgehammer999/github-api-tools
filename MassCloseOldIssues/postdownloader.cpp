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

PostDownloader::PostDownloader(const ProgramOptions &programOptions)
    : m_ctx(boost::asio::ssl::context::tlsv12_client)
    , m_resolver(m_ioc)
    , m_stream(m_ioc, m_ctx)
{
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(m_stream.native_handle(), TARGET.data())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        m_error = "Failed SNI: " + ec.message();
        return;
    }

    const std::string GITHUB_TOKEN = "token " + programOptions.authToken;
    // Set up an HTTP POST request message
    m_request.method(http::verb::post);
    m_request.target(TARGET);
    m_request.set(http::field::host, HOST);
    m_request.set(http::field::user_agent, programOptions.userAgent);
    m_request.set(http::field::authorization, GITHUB_TOKEN);
    m_request.set(http::field::accept, "application/vnd.github.bane-preview+json"); // Allows to use the `createLabel` mutation, because it is in "preview" API

    initializeConnection();
}

PostDownloader::~PostDownloader()
{
    if (!(m_isOpenConnection && m_error.empty()))
        return;

    closeConnection();
    m_ioc.run();
}

void PostDownloader::initializeConnection()
{
    m_resolver.async_resolve(HOST, PORT,
                             beast::bind_front_handler(
                                 &PostDownloader::onResolve,
                                 this));

    m_ioc.run();
    m_ioc.restart();
}

void PostDownloader::run()
{
    sendRequest();

    m_ioc.run();
    m_ioc.restart();
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
                    this));
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
                    this));
}

void PostDownloader::onHandshake(beast::error_code ec)
{
    if(ec) {
        m_error = "Failed handshake: " + ec.message();
        return;
    }

    m_isOpenConnection = true;

   // this is the end of the procedure started by initializeConnection()
}

void PostDownloader::onWrite(beast::error_code ec, std::size_t)
{
    if(ec) {
        m_error = "Failed write: " + ec.message();
        return;
    }

    /*std::cout << "REQUEST:" << std::endl;
    std::cout << m_request.base() << std::endl;
    std::cout << m_request.body() << std::endl;*/

    // Receive the HTTP response
    http::async_read(m_stream, m_buffer, m_response,
                     beast::bind_front_handler(
                         &PostDownloader::onRead,
                         this));
}

void PostDownloader::onRead(beast::error_code ec, std::size_t)
{
    if(ec) {
        m_error = "Failed read: " + ec.message();
        return;
    }

    /*// Write the message to standard out
    std::cout << "RESPONSE:" << std::endl;
    std::cout << m_response.base() << std::endl;
    std::cout << m_response.body() << std::endl;*/

    // Inform the caller that we got a response;
    m_finishedHanlder();
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
}

std::string_view PostDownloader::error() const
{
    return m_error;
}

void PostDownloader::setFinishedHandler(FinishedHandler handler)
{
    m_finishedHanlder = handler;
}

void PostDownloader::setRequestBody(std::string_view body)
{
    m_body = body;
}

const http::response<http::string_body>& PostDownloader::response() const
{
    return m_response;
}

bool PostDownloader::isKeptAlive() const
{
    return m_response.keep_alive();
}

void PostDownloader::sendRequest()
{
    m_response = {};
    m_request.body() = m_body;
    m_request.prepare_payload();

    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(m_stream, m_request,
                      beast::bind_front_handler(
                          &PostDownloader::onWrite,
                          this));
}

void PostDownloader::closeConnection()
{
    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    m_stream.async_shutdown(
                beast::bind_front_handler(
                    &PostDownloader::onShutdown,
                    this));

}
