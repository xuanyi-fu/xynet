#include "common/server.h"
#include "xynet/buffer.h"
#include <span>
#include <numeric>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t CHARGEN_PORT = 2019; 
inline constexpr static const size_t   NULL_BUFFER_SIZE = 1024;
inline constexpr static const size_t   CHARGEN_BUFFER_SIZE = 127 - 33 + 1;

struct chargen_buffer_t
{
  array<char, CHARGEN_BUFFER_SIZE> m_buffer = array<char, CHARGEN_BUFFER_SIZE>{};
  span<const byte, CHARGEN_BUFFER_SIZE> m_span = as_bytes(span{m_buffer});
  chargen_buffer_t()
  {
    iota(m_buffer.begin(), m_buffer.end(), 33);
    m_buffer.back() = '\n';
  }
};

inline static const chargen_buffer_t chargen_buffer = chargen_buffer_t{};


auto get_null_buffer_span()
{
  static array<byte, NULL_BUFFER_SIZE> null_buffer{};
  return as_writable_bytes(span{null_buffer});
}

auto discard(socket_t& peer_socket) -> task<>
{
  try
  {
    for(;;)
    {
      [[maybe_unused]]
      auto read_bytes = 
      co_await peer_socket.recv_some(get_null_buffer_span());
    }
  }
  catch(...){}
}

auto writer(socket_t& peer_socket) -> task<>
{
  try
  {
    for(;;)
    {
      [[maybe_unused]]
      auto sent_bytes = co_await peer_socket.send(chargen_buffer.m_span);
    }
  }catch(...){}
}

auto chargen(socket_t peer_socket) -> task<>
{
  co_await when_all(discard(peer_socket), writer(peer_socket));

  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}
}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(chargen, service, CHARGEN_PORT));
}