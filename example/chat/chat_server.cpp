#include "common/server.h"
#include "xynet/coroutine/single_consumer_async_auto_reset_event.h"
#include <deque>
#include <stop_token>
#include <unordered_set>
#include <memory>

using namespace std;
using namespace xynet;

class chat_room;
class chat_session_interface;

using message_t = vector<byte>;
using message_ptr = shared_ptr<message_t>;
using chat_session_ptr = chat_session_interface*;

struct chat_session_interface
{
  virtual void deliver(message_ptr message) = 0;
  virtual ~chat_session_interface() = default;
};

class chat_room
{
public:
  void join(chat_session_ptr participant)
  {
    m_participants.insert(participant);
    for(const auto& message : m_recent_messages)
    {
      participant->deliver(message);
    }
  }

  void leave(chat_session_ptr participant)
  {
    m_participants.erase(participant);
  }

  void deliver(message_ptr message)
  {
    m_recent_messages.push_back(message);
    while(m_recent_messages.size() > 10)
    {
      m_recent_messages.pop_front();
    }

    for(auto participant : m_participants)
    {
      participant->deliver(message);
    }
  }
private:
  unordered_set<chat_session_ptr> m_participants;
  deque<message_ptr> m_recent_messages;
};

class chat_session : public chat_session_interface
{
public:
  chat_session(socket_t peer_socket, chat_room& room)
  :m_socket{std::move(peer_socket)}
  ,m_room{room}
  {}

  void deliver(message_ptr message) override
  {
    m_write_msgs.push_back(message);
    m_event.set();
  }

  decltype(auto) start()
  {
    m_room.join(this);
    return when_all(reader(m_stop_source.get_token()), writer(m_stop_source.get_token()));
  }

  virtual ~chat_session() = default;

private:

  auto writer(stop_token token) -> task<>
  {
    try
    {
      while(!token.stop_requested())
      {
        if(m_write_msgs.empty())
        {
          co_await m_event;
        }
        else
        {
          [[maybe_unused]]
          auto sent_bytes = co_await m_socket.send(*m_write_msgs.front());
          m_write_msgs.pop_front();
        }
      }
    }catch(...)
    {
      co_await stop();
    }
  }

  auto reader(stop_token token) -> task<>
  {
    try
    {
      for(;!token.stop_requested();)
      {
        auto buf_ptr = make_shared<message_t>(256);
        auto read_bytes = co_await m_socket.recv_some(*buf_ptr);
        buf_ptr->resize(read_bytes);
        m_room.deliver(buf_ptr);
      }
    }catch(...)
    {
      co_await stop();
    }
  }

  auto stop() -> task<>
  {
    if(m_stop_source.request_stop())
    {
      m_room.leave(this);
      try
      {
        m_socket.shutdown();
        co_await m_socket.close();
      }
      catch(...){}
    }
  }

  socket_t m_socket;
  chat_room& m_room;
  deque<message_ptr> m_write_msgs;
  single_consumer_async_auto_reset_event m_event;
  stop_source m_stop_source;
};

int main()
{
  auto service = io_service{};
  auto room = chat_room{};
  sync_wait(start_server([&room](socket_t peer_socket) -> task<>
  {
    auto session = chat_session{std::move(peer_socket), room};
    co_await session.start();
    co_return;
  }, service, 25565));
}


