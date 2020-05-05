#include "common/server.h"
#include "ttcp_message.h"
#include <cstdio>

using namespace std;
using namespace xynet;

auto ttcp_server(socket_t peer_socket) -> task<>
{
  auto message = ttcp_message{};
  try
  {
    auto read_bytes = co_await peer_socket.recv(span{reinterpret_cast<byte*>(&message), sizeof(message)});
    message.length = ::ntohl(message.length);
    message.number = ::ntohl(message.number);

    if(read_bytes != sizeof(message))
    {
      printf("read ttcp_message failed. expected: %lu, received: %lu\n", sizeof(message), read_bytes);
      co_return;
    }
    else if(!(message.number > 0))
    {
      puts("ttcp_message number must be greater than 0.");
      co_return;
    }
    else if(!(message.length > 0))
    {
      puts("ttcp_message length must be greater than 0.");
      co_return;
    }

    printf("Receive number: %d, Receive length: %d\n", message.number, message.length);

    //prepare the buffer for receiving the ttcp payload.
    auto ttcp_payload_len = int32_t{message.length};
    auto ttcp_payload_buf = vector<byte>(ttcp_payload_len);

    for(int i = 0; i < message.number; ++i)
    {
      read_bytes = co_await peer_socket.recv(span{reinterpret_cast<byte*>(&ttcp_payload_len), 
      sizeof(ttcp_payload_len)});
      // ttcp_payload_len = ::ntohl(ttcp_payload_len);
      
      if((read_bytes != sizeof(ttcp_payload_len)) 
      || ttcp_payload_len != message.length)
      {
        printf("ttcp_payload length check failed. expected: %d, received: %d\n", 
        message.length, ttcp_payload_len);
        co_return;
      }

      read_bytes = co_await peer_socket.recv(ttcp_payload_buf);
      auto ttcp_ack = int32_t{ttcp_payload_len};
      // ttcp_ack = ::htonl(ttcp_ack);

      auto sent_bytes = co_await peer_socket.send(span{reinterpret_cast<byte*>(&ttcp_ack), sizeof(ttcp_ack)});
      if(sent_bytes != sizeof(ttcp_ack))
      {
        puts("ttcp_ack sent failed.");
        co_return;
      }
    }

    puts("all payload received successfully.");
  }catch(const std::exception& ex)
  {
    puts(ex.what());
  }

  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}

  co_return;
}

int main(int argc, char** argv)
{
 if(argc != 2)
 {
   puts("usage: ttcp_server [port]");
   return 0;
 }

 uint16_t port;
 try
 {  
  port = static_cast<uint16_t>(stoi(string{argv[1]}));
 }
 catch(const exception& ex)
 {
   puts(ex.what());
 }
 
 auto service = io_service{};
 sync_wait(start_server(ttcp_server, service, port));
 return 0;
}