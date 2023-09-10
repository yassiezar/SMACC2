// Copyright 2023 RobosoftAI Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*****************************************************************************************************************
 *
 * 	 Authors: Jaycee Lock
 *
 ******************************************************************************************************************/

#include <http_client/http_client.hpp>

namespace cl_http {

///////////////////////////
// ClHttp implementation //
///////////////////////////

ClHttp::ClHttp(const std::string &server_name, const int &timeout)
    : initialized_{false},
      timeout_{timeout},
      server_{server_name},
      worker_guard_{boost::asio::make_work_guard(io_context_.get_executor())},
      ssl_context_{boost::asio::ssl::context::tlsv12_client} {
  load_root_certificates(ssl_context_);
  ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
}

ClHttp::~ClHttp() {
  worker_guard_.reset();
  tcp_connection_runner_.join();
}

void ClHttp::onInitialize() {
  if (!initialized_) {
    tcp_connection_runner_ = std::thread{[&]() { io_context_.run(); }};
    this->initialized_ = true;
  }
}

void ClHttp::makeRequest(const kHttpRequestMethod http_method,
                         const std::string &path) {
  auto path_used = path;
  if (path[0] != '/') {
    std::reverse(path_used.begin(), path_used.end());
    path_used += '/';
    std::reverse(path_used.begin(), path_used.end());
  }

  RCLCPP_INFO(this->getLogger(), "SSL? %d", server_.isSSL());
  RCLCPP_INFO(this->getLogger(), "Server %s", server_.getServerName().c_str());
  RCLCPP_INFO(this->getLogger(), "Path %s", path_used.c_str());
  RCLCPP_INFO(this->getLogger(), "Port %s", server_.getPort().c_str());

  std::make_shared<http_session>(boost::asio::make_strand(io_context_),
                                 ssl_context_, callbackHandler)
      ->run(server_.getServerName(), path_used, server_.getPort(),
            static_cast<boost::beast::http::verb>(http_method), HTTP_VERSION);
}

/////////////////////////////////
// http_session implementation //
/////////////////////////////////

ClHttp::http_session::http_session(
    boost::asio::any_io_executor ioc, boost::asio::ssl::context &ssl_context,
    const std::function<void(const TResponse &)> response)
    : onResponse{response}, resolver_{ioc}, stream_{ioc, ssl_context} {}
// stream_{boost::asio::make_strand(ioc)} {}

void ClHttp::http_session::run(const std::string &host,
                               const std::string &target,
                               const std::string &port,
                               const boost::beast::http::verb http_method,
                               const int &version) {
  // Set up an HTTP request
  req_.version(version);
  req_.method(http_method);
  req_.target(target);
  req_.set(boost::beast::http::field::host, host);
  req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // Look up the domain name
  resolver_.async_resolve(host.c_str(), port.c_str(),
                          boost::beast::bind_front_handler(
                              &http_session::on_resolve, shared_from_this()));
}

void ClHttp::http_session::on_resolve(
    boost::beast::error_code ec,
    boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) return fail(ec, "resolve");

  // Set a timeout on the operation
  boost::beast::get_lowest_layer(stream_).expires_after(
      std::chrono::seconds(1));

  // Make the connection on the IP address we get from a lookup
  boost::beast::get_lowest_layer(stream_).async_connect(
      results, boost::beast::bind_front_handler(&http_session::on_connect,
                                                shared_from_this()));
}

void ClHttp::http_session::fail(boost::beast::error_code ec, char const *what) {
  std::cout << "Failure!..." << std::endl;
  std::cerr << what << ": " << ec.message() << "\n";
  res_.result(boost::beast::http::status::bad_request);
  res_.reason() = ec.message();
  onResponse(res_);
}

void ClHttp::http_session::on_connect(
    boost::beast::error_code ec,
    boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
  if (ec) return fail(ec, "connect");

  // Set a timeout on the operation
  boost::beast::get_lowest_layer(stream_).expires_after(
      std::chrono::seconds(1));

  // Send the HTTP request to the remote host
  stream_.async_handshake(boost::asio::ssl::stream_base::client,
                          boost::beast::bind_front_handler(
                              &http_session::on_handshake, shared_from_this()));
}

void ClHttp::http_session::on_handshake(boost::beast::error_code ec) {
  if (ec) return fail(ec, "handshake");

  // Set a timeout on the operation
  boost::beast::get_lowest_layer(stream_).expires_after(
      std::chrono::seconds(1));

  // Send the HTTP request to the remote host
  boost::beast::http::async_write(
      stream_, req_,
      boost::beast::bind_front_handler(&http_session::on_write,
                                       shared_from_this()));
}

void ClHttp::http_session::on_write(boost::beast::error_code ec,
                                    std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) return fail(ec, "write");

  // Receive the HTTP response
  boost::beast::http::async_read(
      stream_, buffer_, res_,
      boost::beast::bind_front_handler(&http_session::on_read,
                                       shared_from_this()));
}

void ClHttp::http_session::on_read(boost::beast::error_code ec,
                                   std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) return fail(ec, "read");

  boost::beast::get_lowest_layer(stream_).expires_after(
      std::chrono::seconds(1));

  // Gracefully close the socket
  // stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  stream_.async_shutdown(boost::beast::bind_front_handler(
      &http_session::on_shutdown, shared_from_this()));
}

void ClHttp::http_session::on_shutdown(boost::beast::error_code ec) {
  // not_connected happens sometimes so don't bother reporting it.
  if (ec == boost::asio::error::eof) {
    // Rationale:
    // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    ec = {};
  }
  // if (ec) return fail(ec, "shutdown");

  // If we get here then the connection is closed gracefully
  onResponse(res_);
}
}  // namespace cl_http
