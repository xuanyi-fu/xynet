#include "common/server.h"
#include <chrono>
#include <stop_token>
#include <numeric>
#include <algorithm>

using namespace std;
using namespace xynet;

struct pingpong_config
{
  size_t client_num;
  size_t message_len;
  chrono::milliseconds timeout;
};

class pingpong_client
{
public:
  pingpong_client(io_service& service, 
                  const socket_address& address, 
                  const pingpong_config& config)
  :m_service{service}
  ,m_address{address}
  ,m_config{config}                  
  {}

  auto session(stop_token token) -> task<double>
  {
    auto s = socket_t{};
    std::puts("Connected.");
    auto buffer = vector<byte>(m_config.message_len);
    auto total_bytes_read = double{};
    try
    {
      s.init();
      co_await s.connect(m_address);
      

      while(!token.stop_requested())
      {
        auto send_bytes = co_await s.send(buffer);
        auto read_bytes = co_await s.recv(buffer);
        total_bytes_read += read_bytes;
      }
    }catch(...){}

    try
    {
      s.shutdown();
      co_await s.close();
    }catch(...){}

    co_return total_bytes_read;
  }

  

  auto run() -> task<>
  {
    auto source = stop_source{};
    auto scope = async_scope{};
    // prepare all the session
    auto sessions = vector<task<double>>();
    sessions.reserve(m_config.client_num);
    for(int i = 0; i < m_config.client_num; ++i)
    {
      sessions.emplace_back(session(source.get_token()));
    }
    

    auto start_time = chrono::system_clock::now();
    scope.spawn(timer(source));
    auto res = co_await when_all(std::move(sessions));
    auto stop_time = chrono::system_clock::now();
    auto total_bytes_read = accumulate(res.begin(), res.end(), double{});
    printf("Total bytes read: %.3f bytes\n", total_bytes_read);
    auto throughput = total_bytes_read / 1024. / 1024. 
      / chrono::duration_cast<chrono::milliseconds>(stop_time - start_time).count() * 1000.;

    printf("Throguhtput: %.3f MiB/s\n", throughput);
    co_await scope.join();
    m_service.request_stop();
  }

  auto timer(const stop_source& source) -> task<>
  {
    co_await m_service.run_after(m_config.timeout);
    source.request_stop();
  }
private:
  io_service&            m_service;
  const socket_address&  m_address;
  const pingpong_config& m_config;
};

int main()
{
  auto config 
    = pingpong_config{.client_num = 1000, .message_len = 65535, .timeout = chrono::seconds{5}};
  auto service = io_service{};
  auto address = socket_address{"127.0.0.1", 2007};
  auto client = pingpong_client(service, address, config);

  sync_wait(when_all
  (
    client.run(),
    [&service]() -> task<>
    {
      service.run();
      co_return;
    }()
  ));
}