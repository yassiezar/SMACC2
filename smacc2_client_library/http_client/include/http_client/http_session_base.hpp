#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <string>

namespace cl_http {
class http_session_base {
 public:
  virtual ~http_session_base() {}

  using TResponse =
      boost::beast::http::response<boost::beast::http::string_body>;

  // Start the asynchronous operation
  virtual void run(const std::string &host, const std::string &target,
                   const boost::beast::http::verb http_method,
                   const int &version) = 0;

  virtual std::string getPort() = 0;

 protected:
  virtual void on_resolve(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type results) = 0;
  virtual void fail(boost::beast::error_code ec, const char *what) = 0;
  virtual void on_connect(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type) = 0;
  virtual void on_write(boost::beast::error_code ec,
                        std::size_t bytes_transferred) = 0;
  virtual void on_read(boost::beast::error_code ec,
                       std::size_t bytes_transferred) = 0;

  // Optional, needed for SSL connections
  virtual void on_handshake(boost::beast::error_code ec) {}
  virtual void on_shutdown(boost::beast::error_code ec) {}
};
}  // namespace cl_http
