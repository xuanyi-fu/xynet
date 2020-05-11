////
//// Created by xuanyi on 4/22/20.
////

#include <type_traits>
#include <string>
#include <iostream>
#include <ranges>
#include <openssl/sha.h>

#include "xynet/file/file.h"
#include "xynet/socket/socket.h"
#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/async_scope.h"

#include "xynet/http/http_parser.h"

// using namespace xynet;

// auto test(io_service& service) -> task<>
// {
//   auto f = file_t{};
//   auto listen_socket = socket_t{};
//   auto s = socket_t{};
//   try
//   {
//     listen_socket.init();
//     listen_socket.bind(socket_address{25565});
//     listen_socket.reuse_address();
//     listen_socket.listen();
//     co_await listen_socket.accept(s);

//     co_await f.openat("/home/xuanyi/1.txt", O_RDONLY, 0u);
//     co_await f.update_statx();

//     // co_await s.splice(f, 0, f.file_size());

//   }catch(const std::exception& ex)
//   {
//     std::puts(ex.what());
//   }

//   service.request_stop();
// }

// auto run_service(io_service& service) -> task<>
// {
//   service.run();
//   co_return;
// }

// int main()
// {
//   auto service = io_service{};
//   auto t = when_all(test(service), run_service(service));
//   sync_wait(t);
// }
using namespace std;
using namespace xynet;
using namespace std::literals;

namespace detail
{

}
inline constexpr static char 
websocket_handshake_response_model[] = 
{
'H','T','T','P','/','1','.','1',' ','1','0','1',' ','S','w','i','t','c','h','i','n','g',' ','P','r','o','t','o','c','o','l','s', '\r','\n',
'U','p','g','r','a','d','e',':',' ','w','e','b','s','o','c','k','e','t', '\r','\n',
'C','o','n','n','e','c','t','i','o','n',':',' ','U','p','g','r','a','d','e', '\r','\n',
'S','e','c','-','W','e','b','S','o','c','k','e','t','-','A','c','c','e','p','t',':',' ','+','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X', '\r','\n'
};

inline constexpr static char 
websocket_handshake_accept_concat[] = "XXXXXXXXXXXXXXXXXXXXXX==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

consteval decltype(auto) websocket_handshake_accept_concat_array()
{
  return std::to_array(websocket_handshake_accept_concat);
}

consteval decltype(auto) websocket_handshake_accept_concat_len()
{
  return std::to_array(websocket_handshake_accept_concat).size();
}

consteval decltype(auto) websocket_handshake_response_model_len()
{
  return std::to_array(websocket_handshake_response_model).size();
}

consteval decltype(auto) websocket_handshake_response_model_array()
{
  return std::to_array(websocket_handshake_response_model);
}

consteval decltype(auto) websocket_handshake_response_model_accept_key_pos()
{
  return std::string_view{websocket_handshake_response_model, 
    sizeof(websocket_handshake_response_model)}.find('+');
}

class websocket_request_handler
{
public:

  template<typename... Args>
  decltype(auto) parse(Args&&... args)
  {
    auto ret = m_parser.parse(args...);
    switch(ret)
    {
      case -1: m_state = state::bad_request;
      break;
      case -2: m_state = state::incomplete;
      break;
      default: m_state = state::parsed;
      break;
    }
    return ret;
  }

  void generate_response() 
  {
    if(m_state != state::parsed || !check_all())
    {
      m_state = state::bad_request;
      generate_bad_request();
    }

    // compute the SHA-1 digest from the accept_concat array
    std::array<char, 20> sha1_digest{};
    SHA1(reinterpret_cast<unsigned char*>(m_websocket_handshake_accept_concat.data()), 
    m_websocket_handshake_accept_concat.size() - 1, 
    reinterpret_cast<unsigned char*>(sha1_digest.data()));

    // compute the base64 outof the accept_concat array
    // and fill it into the response
    base64_encode(
      m_websocket_handshake_response.data() 
    + websocket_handshake_response_model_accept_key_pos(),
      sha1_digest.data(), 
      sha1_digest.size());
  }

  const auto& response() const
  {
    if(m_state == state::parsed)
    {
      return m_websocket_handshake_response;
    }

    return m_websocket_handshake_response;
  }

  void generate_bad_request()
  {

  }

  bool check_all() const 
  {
    if(!check_method() || !check_version())
    {
      return false;
    }

    return check_headers();
  }

  bool check_method() const
  {
    return m_parser.method() == "GET"sv;
  }

  decltype(auto) get_path() const
  {
    return m_parser.path();
  }

  bool check_version() const
  {
    return m_parser.version() == 1;
  }

  bool check_Connection(std::string_view header) const
  {
    return header == "Upgrade"sv;
  }

  bool check_Upgrade(std::string_view header) const
  {
    return header == "websocket"sv;
  }

  bool check_Sec_WebSocket_Version(std::string_view header) const
  {
    return header == "13"sv;
  }

  bool check_Sec_WebSocket_Key(std::string_view header) const
  {
    if(header.size() == 24)
    {
      //copy the key from the client to accept_concat array
      std::copy(header.begin(), header.begin() + 24, 
        m_websocket_handshake_accept_concat.begin());
      return true;
    }

    return false;
  }

  bool check_headers() const 
  {
    for(const auto& header : m_parser.headers())
    {
      if(auto it = m_header_checker.find(header.first);
        it != m_header_checker.cend() 
        && (this->*((*it).second))(header.second) == false)
      {
        return false;
      }
    }

    return true;
  }

public:
  inline static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  static int base64_encode(char* encoded, const char* string, int len)
  {
      int i;
      char *p;

      p = encoded;
      for (i = 0; i < len - 2; i += 3) {
      *p++ = basis_64[(string[i] >> 2) & 0x3F];
      *p++ = basis_64[((string[i] & 0x3) << 4) |
                      ((int) (string[i + 1] & 0xF0) >> 4)];
      *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                      ((int) (string[i + 2] & 0xC0) >> 6)];
      *p++ = basis_64[string[i + 2] & 0x3F];
      }
      if (i < len) {
      *p++ = basis_64[(string[i] >> 2) & 0x3F];
      if (i == (len - 1)) {
          *p++ = basis_64[((string[i] & 0x3) << 4)];
          *p++ = '=';
      }
      else {
          *p++ = basis_64[((string[i] & 0x3) << 4) |
                          ((int) (string[i + 1] & 0xF0) >> 4)];
          *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
      }
      *p++ = '=';
      }

      //*p++ = '\0';
      return p++ - encoded;
  }

  enum class state : int 
  {
    empty = 1,
    incomplete = 2,
    parsed = 3,
    bad_request =4
  };

  using header_checker_fucntion_t = bool(websocket_request_handler::*)(std::string_view) const;
  array<char, websocket_handshake_response_model_len()> m_websocket_handshake_response
   = websocket_handshake_response_model_array();

  mutable array<char, websocket_handshake_accept_concat_len()> m_websocket_handshake_accept_concat
  = websocket_handshake_accept_concat_array();

  http_parser m_parser = http_parser{};

  state m_state = state::empty;

  inline static const std::unordered_map<std::string_view, header_checker_fucntion_t> 
  m_header_checker
  {
    {"Connection"sv, &websocket_request_handler::check_Connection},
    {"Upgrade"sv, &websocket_request_handler::check_Upgrade},
    {"Sec-WebSocket-Version"sv, &websocket_request_handler::check_Sec_WebSocket_Version},
    {"Sec-WebSocket-Key"sv, &websocket_request_handler::check_Sec_WebSocket_Key}
  };
};

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int Base64encode(char *encoded, const char *string, int len)
{
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    *p++ = basis_64[((string[i] & 0x3) << 4) |
                    ((int) (string[i + 1] & 0xF0) >> 4)];
    *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                    ((int) (string[i + 2] & 0xC0) >> 6)];
    *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    if (i == (len - 1)) {
        *p++ = basis_64[((string[i] & 0x3) << 4)];
        *p++ = '=';
    }
    else {
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}

int main()
{
  auto request = 
  "GET / HTTP/1.1\r\n"
  "Host: xyfu.me\r\n"
  "Upgrade: websocket\r\n"
  "Connection: Upgrade\r\n"
  "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
  "Sec-WebSocket-Version: 13\r\n\r\n";
  auto p = websocket_request_handler{};
  auto r1 = std::string_view{request, request + 10};
  auto r2 = std::string_view{request};
  p.parse(r1);
  p.parse(r2);

  p.generate_response();

  for(auto c : p.response())
  {
    std::cout<<c;
  }

  // std::array<char, 20> sha1_digest = {};
  // std::array<char, 100> base64_out = {};
  // char key[] = "dGhlIHNhbXBsZSBub25jZQ==";
  // char concat_magic[] = "XXXXXXXXXXXXXXXXXXXXXX==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  // char* concat_magic_ = concat_magic;
  // copy(key, key+24, concat_magic);
  // SHA1(reinterpret_cast<unsigned char*>(concat_magic_), 
  //   sizeof(concat_magic) - 1, 
  //   reinterpret_cast<unsigned char*>(sha1_digest.data()));
  // std::cout<<sha1_digest.data()<<"\n";
  // websocket_request_handler::base64_encode(base64_out.data(), 
  //   sha1_digest.data(), sha1_digest.size());
  // std::cout<<base64_out.data();


}