#include "common/server.h"
#include "xynet/coroutine/single_consumer_async_auto_reset_event.h"
#include <deque>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t ECHO_PORT = 2007;
inline constexpr static const uint16_t ECHO_BUFFER_SIZE = 8196;

class echo_client
{
public:
  echo_client(socket_t peer_socket)
  :m_socket(std::move(peer_socket))
  {}

  auto writer() -> task<>
  {
    try
    {
      while(m_socket.valid())
      {
        if(m_write_msgs.empty())
        {
          co_await m_event;
        }
        else
        {
          [[maybe_unused]]
          auto sent_bytes = co_await m_socket.send(m_write_msgs.front());
          m_write_msgs.pop_front();
        }
      }
    }catch(...)
    {
      co_await stop();
    }
  }

  auto reader() -> task<>
  {
    try
    {
      for(;;)
      {
        auto buf = vector<byte>(ECHO_BUFFER_SIZE);
        auto read_bytes = co_await m_socket.recv_some(buf);
        buf.resize(read_bytes);
        m_write_msgs.emplace_back(std::move(buf));
        m_event.set();
      }
    }catch(...)
    {
      co_await stop();
    }
  }

  decltype(auto) start()
  {
    return when_all(reader(), writer());
  }

private:

  auto stop() -> task<>
  {
    try
    {
      m_socket.shutdown();
      co_await m_socket.close();
    }
    catch(...){}
  }

  socket_t m_socket;
  list<vector<byte>> m_write_msgs;
  single_consumer_async_auto_reset_event m_event;
};

auto echo_revised(socket_t peer_socket) -> task<>
{
  auto client = echo_client{std::move(peer_socket)};
  co_await client.start();
  co_return;
}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(echo_revised, service, ECHO_PORT));
}


