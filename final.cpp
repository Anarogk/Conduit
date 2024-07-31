#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <system_error>

const std::size_t buffer_size = 1024;
const std::string http_request = "GET /index.html HTTP/1.1\r\n"
                                  "Host: example.com\r\n"
                                  "Connection: close\r\n\r\n";

void GrabData(asio::ip::tcp::socket& socket, std::string& response) {
  std::vector<char> buffer(buffer_size);

  asio::async_read_until(socket,
                          asio::buffer(buffer),
                          "\r\n\r\n",
                          [&](const asio::error_code& ec, std::size_t bytes_transferred) {
                            if (!ec) {
                              response.append(buffer.begin(), buffer.begin() + bytes_transferred);
                              std::cout << "Received response: " << response << std::endl;
                            } else {
                              std::cerr << "Error reading response: " << ec.message() << std::endl;
                            }
                          });
}

int main() {
  asio::io_context context;
  asio::io_context::work work(context);
  std::thread thread([&context]() { context.run(); });

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("xx.xx.xx.yy"), 80);
  asio::ip::tcp::socket socket(context);

  try {
    socket.connect(endpoint);
    std::cout << "Connected!" << std::endl;

    asio::error_code ec;
    socket.write_some(asio::buffer(http_request.data(), http_request.size()), ec);
    if (ec) {
      std::cerr << "Error writing request: " << ec.message() << std::endl;
      return 1;
    }

    std::string response;
    GrabData(socket, response);

    socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    socket.close();
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  thread.join();
  return 0;
}
