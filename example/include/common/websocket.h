#include "xynet/io_service.h"
#include "xynet/socket/socket.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/async_scope.h"

#include "xynet/http/websocket_request_handler.h"
#include "xynet/http/websocket_frame_header.h"
#include "xynet/http/websocket_frame_mask.h"

inline constexpr static const uint16_t MAX_HTTP_REQUEST_SIZE = 1024;
inline constexpr static const uint16_t MAX_WEBSOCKET_FRAME_SIZE = 1024;
inline constexpr static const uint16_t MAX_WEBSOCKET_APP_DATA_LEN = 1000;

using namespace xynet;
using namespace std;

struct websocket_close_exception : public std::exception
{
  const char* what() const noexcept override
  {
    return "websocket closed";
  }

  virtual ~websocket_close_exception() = default;
};

struct http_bad_request_exception : public std::exception
{
  const char* what() const noexcept override
  {
    return "HTTP Bad Request";
  }

  virtual ~http_bad_request_exception() = default;
};

auto websocket_handshake(socket_t& peer_socket) -> task<>
{
  array<byte, MAX_HTTP_REQUEST_SIZE> buf{};
  auto parser = websocket_request_handler{};
  auto recv_bytes = std::size_t{};
  auto ret = int{};
  while(recv_bytes < MAX_HTTP_REQUEST_SIZE)
  {
    recv_bytes += co_await peer_socket.recv_some
      (std::span{buf.data() + recv_bytes, buf.size() - recv_bytes});
    auto ret = parser.parse(std::span{buf.data(), recv_bytes});
    if(ret == -1)
    {
      continue;
    }
    else
    {
      auto response = parser.generate_response();
      co_await peer_socket.send(response);
      break;
    }
  }

  if(ret == -2)
  {
    throw http_bad_request_exception{};
  }

}

auto websocket_send_close(socket_t& peer_socket, uint16_t status_code) -> task<>
{
  auto header = websocket_frame_header{websocket_flags::WS_OP_CLOSE 
    | websocket_flags::WS_FINAL_FRAME, 2};
  status_code = ::htons(status_code);
  co_await peer_socket.send(header.span(), 
    std::span{reinterpret_cast<const byte*>(&status_code), 2});
  
  throw websocket_close_exception{};
}

auto websocket_check_parser_result(socket_t& peer_socket, 
const websocket_frame_header_parser& parser)
->task<std::size_t>
{
  auto [flags, mask, length] = parser.result();

  if(websocket_flags_not_none(flags & websocket_flags::WS_OP_CLOSE))
  {
    co_await websocket_send_close(peer_socket, 1000);
  }

  if(!websocket_flags_not_none(flags & websocket_flags::WS_FINAL_FRAME))
  {
    co_await websocket_send_close(peer_socket, 1003);
  }

  if(!websocket_flags_not_none(flags & websocket_flags::WS_HAS_MASK))
  {
    co_await websocket_send_close(peer_socket, 1008);
  }

  if(length > MAX_WEBSOCKET_APP_DATA_LEN)
  {
    co_await websocket_send_close(peer_socket, 1009);
  }

  co_return length;
}

auto websocket_recv_data(socket_t& peer_socket, auto& buf) 
-> task<decltype(std::span{buf.data(), 0u})>
{
  auto recv_bytes = size_t{};
  auto parser = websocket_frame_header_parser{};

  while(recv_bytes < MAX_HTTP_REQUEST_SIZE)
  {
    recv_bytes += co_await peer_socket.recv_some
      (std::span{buf.data() + recv_bytes, buf.size() - recv_bytes});
    
    if(auto ret = parser.parse(std::span{buf.data(), recv_bytes});
        ret == -1)
    {
      continue;
    }
    else
    {
      auto [flags, mask, length] = parser.result();
      co_await websocket_check_parser_result(peer_socket, parser);
      auto data_span = std::span{buf.data() + ret, length};
      websocket_mask(data_span, mask, 0);
      co_return data_span;
    }
  }
}