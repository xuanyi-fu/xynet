#include "common/server.h"
#include "ttcp_message.h"
#include <cstdio>
#include <chrono>

using namespace std;
using namespace xynet;

auto ttcp_client(io_service& service, 
                const socket_address& address, 
                ttcp_message message)
-> task<>
{
  auto s = socket_t{};
  try
  {
    s.init();
    co_await s.connect(address);

    message.length = ::htonl(message.length);
    message.number = ::htonl(message.number);

    auto sent_bytes = co_await s.send(span{reinterpret_cast<byte*>(&message), sizeof(message)});

    const auto ttcp_payload_bytes = 
    static_cast<size_t>(sizeof(int32_t) + ::ntohl(message.length));

    constexpr auto free_payload = [](ttcp_payload_t* p){::free(p);};

    unique_ptr<ttcp_payload_t, decltype(free_payload)> ttcp_payload
    {static_cast<ttcp_payload_t*>(::malloc(ttcp_payload_bytes))};

    ttcp_payload->length = ::htonl(message.length);
    printf("ttcp_payload->lenght: %d\n", ttcp_payload->length);

    for(int i = 0; i < message.length; ++i)
    {
      ttcp_payload->data[i] = "1234567890"[i % 10];
    }

    auto start_time = chrono::system_clock::now();
    for(int i = 0; i < ::ntohl(message.number); ++i)
    {
      std::printf("sending %d: %lu bytes\n", i, ttcp_payload_bytes);
      sent_bytes = co_await s.send(span{reinterpret_cast<byte*>(ttcp_payload.get()), ttcp_payload_bytes});
      std::printf("sent %lu\n", sent_bytes);
      auto ack = int32_t{};
      auto read_bytes = co_await s.recv(span{reinterpret_cast<byte*>(&ack), sizeof(ack)});
      ack = ::ntohl(ack);
      if(ack != message.length)
      {
        puts("ACK length check failed.");
        co_return;
      }
    }
    auto end_time = chrono::system_clock::now();
    auto time_elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    auto total_len_MiB = 1. * message.length * message.number / 1024. / 1024. ;
    auto throughput = total_len_MiB / time_elapsed.count() * 1000.;
    printf("Throughtput: %.3f MiB/s\n", throughput); 

  }catch(const exception& ex)
  {
    puts(ex.what());
  }

  try
  {
    s.shutdown();
    co_await s.close();
  }catch(...){}

  service.request_stop();
}

int main(int argc, char** argv)
{
  if(argc != 5)
  {
    puts("usage: ttcp_client [destination] [port] [message number] [message length]");
  }

  try
  {
    auto ip = std::string{argv[1]};
    auto port = static_cast<uint16_t>(stoi(std::string{argv[2]}));
    auto message_number = stoi(std::string{argv[3]});
    auto message_length = stoi(std::string{argv[4]});
    auto address = socket_address{ip, port};
    auto message = ttcp_message{.number = message_number, .length = message_length};
    auto service = io_service{};

    sync_wait
    (
      when_all
      (
        ttcp_client(service, address, message), 
        [&service]()->task<>{service.run(); co_return;}()
      )
    );
  }
  catch(const exception& ex)
  {
    puts(ex.what());
  }


}