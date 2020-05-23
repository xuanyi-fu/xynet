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
    auto buffer = vector<byte>(m_config.message_len);
    auto total_bytes_read = double{};
    try
    {
      s.init();
      co_await s.connect(m_address);    
      while(!token.stop_requested())
      {
        [[maybe_unused]]
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
    auto total_time = chrono::duration_cast<chrono::milliseconds>(stop_time - start_time);
    auto throughput = total_bytes_read / 1024. / 1024. 
      / total_time.count() * 1000.;

    printf("Time: %ld ms, Throguhtput: %.3f MiB/s\n",total_time.count(), throughput);
    co_await scope.join();
    m_service.request_stop();
  }

  auto timer(const stop_source& source) -> task<>
  {
    co_await m_service.schedule(m_config.timeout);
    source.request_stop();
  }
private:
  io_service&            m_service;
  const socket_address&  m_address;
  const pingpong_config& m_config;
};

int main(int argc, char** argv)
{
  if(argc != 6)
  {
    puts("usage: pingpong_client [destination] [port] [client number] [message length bytes] [timeout seconds]");
    return 0;
  }
  auto dst = string{};
  auto num = size_t{};
  auto len = size_t{};
  auto port = uint16_t{};
  auto timeout = chrono::seconds{};

  try
  {
    dst = string(argv[1]);
    port = static_cast<uint16_t>(stoi(string(argv[2])));
    num = stoi(string(argv[3]));
    len = stoi(string(argv[4]));
    timeout = chrono::seconds{stoi(std::string(argv[5]))};
  }catch(const exception& ex)
  {
    puts(ex.what());
    return 0;
  }

  auto config 
    = pingpong_config{.client_num = num, .message_len = len, .timeout = timeout};
  auto service = io_service{};
  auto address = socket_address{dst, port};
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