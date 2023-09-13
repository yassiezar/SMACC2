#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <http_client/http_session_base.hpp>
#include <iostream>
#include <string>

namespace cl_http {
class http_session : public std::enable_shared_from_this<http_session>,
                     public http_session_base {
 public:
  // Objects are constructed with a strand to
  // ensure that handlers do not execute concurrently.
  http_session(boost::asio::any_io_executor ioc,
               const std::function<void(const TResponse &)> response);

  virtual ~http_session() {}

  // Start the asynchronous operation
  void run(const std::string &host, const std::string &target,
           const boost::beast::http::verb http_method,
           const int &version) override;

  std::string getPort() override { return kPort; }

 private:
  const std::string kPort = "80";

  void on_resolve(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type results) override;
  void fail(boost::beast::error_code ec, const char *what) override;
  void on_connect(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type) override;
  void on_write(boost::beast::error_code ec,
                std::size_t bytes_transferred) override;
  void on_read(boost::beast::error_code ec,
               std::size_t bytes_transferred) override;

  std::function<void(const TResponse &)> onResponse;

  boost::asio::ip::tcp::resolver resolver_;
  boost::beast::tcp_stream stream_;
  boost::beast::flat_buffer buffer_;  // (Must persist between reads)
  boost::beast::http::request<boost::beast::http::empty_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
};
}  // namespace cl_http
