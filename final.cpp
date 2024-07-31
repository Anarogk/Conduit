#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

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

std::atomic<int> success_count{0};
std::atomic<int> failure_count{0};

std::mutex mtx; // Mutex for thread-safe access to metrics

struct ClientStats {
  long long total_time = 0;
  int num_requests = 0;
};

void simulate_client(asio::io_context& io_context, const std::string& host, int port, ClientStats& stats) {
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(host), port);
  asio::ip::tcp::socket socket(io_context);

  auto start = std::chrono::steady_clock::now();

  socket.async_connect(endpoint,
                      [&](const asio::error_code& ec) {
                        if (!ec) {
                          socket.async_write_some(asio::buffer(http_request.data(), http_request.size()),
                                                 [&](const asio::error_code& ec, std::size_t bytes_transferred) {
                                                   if (!ec) {
                                                     // Read response (omitted for simplicity)
                                                     auto end = std::chrono::steady_clock::now();
                                                     auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                                                     mtx.lock();
                                                     stats.total_time += elapsed.count();
                                                     stats.num_requests++;
                                                     mtx.unlock();

                                                     success_count++;
                                                   } else {
                                                     failure_count++;
                                                   }
                                                 });
                        } else {
                          failure_count++;
                        }
                      });
}

int main() {
  const int num_clients = 100; // Adjust number of clients
  const std::string host = "xx.xx.xx.yy";
  const int port = 80;
  const int num_threads = std::thread::hardware_concurrency(); // Adjust number of threads

  std::vector<ClientStats> thread_stats(num_threads);
  std::vector<std::thread> threads;

  // Create io_contexts and launch worker threads
  std::vector<asio::io_context> io_contexts(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&io_contexts, i]() { io_contexts[i].run(); });
  }

  // Simulate clients from each thread
  for (int i = 0; i < num_clients; ++i) {
    int thread_id = i % num_threads; // Distribute clients across threads
    simulate_client(io_contexts[thread_id], host, port, thread_stats[thread_id]);
  }

  // Wait for all threads to finish
  for (auto& thread : threads) {
    thread.join();
  }

  // Calculate and print overall statistics
  long long total_time = 0;
  int total_requests = 0;
  for (const auto& stats : thread_stats) {
    total_time += stats.total_time;
    total_requests += stats.num_requests;
  }

  double avg_response_time = static_cast<double>(total_time) / total_requests;

  std::cout << "Successes: " << success_count << std::endl;
  std::cout << "Failures: " << failure_count << std::endl;
  std::cout << "Average response time: " << avg_response_time << " ms" << std::endl;

  return 0;
}
