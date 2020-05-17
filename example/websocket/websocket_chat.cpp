#include "common/server.h"
#include "common/websocket.h"
#include "xynet/coroutine/single_consumer_async_auto_reset_event.h"
#include "xynet/stream_buffer.h"
#include <deque>
#include <stop_token>
#include <unordered_set>
#include <memory>
#include <string_view>
#include <optional>

using namespace std;
using namespace xynet;

inline constexpr static std::size_t MAX_MESSAGE_LEN = 1000;

class chat_room;
class chat_session_interface;

using message_t   = vector<byte>;
using message_ptr = shared_ptr<message_t>;
using chat_session_ptr = chat_session_interface*;

struct chat_session_interface
{
  virtual void deliver(message_ptr message) = 0;
  virtual const socket_address& get_address() = 0;
  virtual ~chat_session_interface() = default;
};

class chat_room
{
public:
  void join(chat_session_ptr participant)
  {
    m_participants.insert(participant);
    welcome(participant);
    deliver_recent_messages(participant);
  }

  void deliver_recent_messages(chat_session_ptr participant)
  {
    for(const auto& message : m_recent_messages)
    {
      participant->deliver(message);
    }
  }

  void welcome(chat_session_ptr participant)
  {
    auto welcome_str = "|        SERVER       |:"s 
      + participant->get_address().to_str() + " has joined the chat\n"s;
    
    auto welcome_message_ptr = make_shared<vector<byte>>(welcome_str.size());
    copy(
      reinterpret_cast<byte*>(&*welcome_str.begin()), 
      reinterpret_cast<byte*>(&*(welcome_str.begin() + welcome_str.size())), 
      welcome_message_ptr->begin());

    for(auto participant : m_participants)
    {
      participant->deliver(welcome_message_ptr);
    }
  }

  void farewell(chat_session_ptr participant)
  {
    auto farewell_str = "|        SERVER       |:"s 
      + participant->get_address().to_str() + " has left the chat\n"s;
    
    auto farewell_message_ptr = make_shared<vector<byte>>(farewell_str.size());
    copy(
      reinterpret_cast<byte*>(&*farewell_str.begin()), 
      reinterpret_cast<byte*>(&*(farewell_str.begin() + farewell_str.size())), 
      farewell_message_ptr->begin());

    for(auto participant : m_participants)
    {
      participant->deliver(farewell_message_ptr);
    }
  }

  void leave(chat_session_ptr participant)
  {
    m_participants.erase(participant);
    farewell(participant);
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
  {
    make_message_head();
  }

  void deliver(message_ptr message) override
  {
    m_write_msgs.push_back(message);
    m_event.set();
  }

  const socket_address& get_address() override
  {
    return m_socket.get_peer_address();
  }

  decltype(auto) start()
  {
    m_room.join(this);
    return when_all(reader(m_stop_source.get_token()), 
      writer(m_stop_source.get_token()));
  }

  virtual ~chat_session() = default;

private:

  auto make_message_head() -> void
  {
    auto ip_port = m_socket.get_peer_address().to_str();
    auto blank = std::size_t{21 - ip_port.size()};
    m_message_head = "|" + ip_port;
    m_message_head.append(blank, ' ');
    m_message_head += "|:";
  }

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
          const auto& message = *m_write_msgs.front();
          auto header = websocket_frame_header
          {
            websocket_flags::WS_FINAL_FRAME 
          | websocket_flags::WS_OP_TEXT,
            message.size()};
          auto sent_bytes = co_await m_socket.send(header.span(), message);
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
    array<byte, MAX_WEBSOCKET_FRAME_SIZE> buf{};
    try
    {
      for(;!token.stop_requested();)
      {
        auto data_span = co_await websocket_recv_data(m_socket, buf);
        auto message = make_shared<vector<byte>>();
        message->reserve(data_span.size() + m_message_head.size());

        copy(
        reinterpret_cast<const byte*>(&*m_message_head.begin()), 
        reinterpret_cast<const byte*>(&*(m_message_head.begin() + m_message_head.size())),
        back_inserter(*message));

        copy(
        reinterpret_cast<const byte*>(&*data_span.data()), 
        reinterpret_cast<const byte*>(&*(data_span.data() + data_span.size())),
        back_inserter(*message));
        

        m_room.deliver(message);
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
  string m_message_head;
  deque<message_ptr> m_write_msgs;
  single_consumer_async_auto_reset_event m_event;
  stop_source m_stop_source;
};

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    puts("usage: websocket_chat [port]");
  }

  auto port = uint16_t{};

  try
  {
    port = stoi(string{argv[1]});
  }catch(...)
  {
    return 0;
  }
  
  auto service = io_service{};
  auto room = chat_room{};
  sync_wait(start_server([&room](socket_t peer_socket) -> task<>
  {
    try
    {
      co_await websocket_handshake(peer_socket);
      auto session = chat_session{std::move(peer_socket), room};
      co_await session.start();
    }catch(...){}

    try
    {
      peer_socket.shutdown();
      co_await peer_socket.close();
    }catch(...){}
  }, service, port));
}


