// ////
// //// Created by xuanyi on 4/22/20.
// ////
#include "xynet/file_descriptor.h"

#include "xynet/socket/impl/address.h"

#include "xynet/socket/impl/bind.h"
#include "xynet/socket/impl/listen.h"
#include "xynet/socket/impl/shutdown.h"
#include "xynet/socket/impl/socket_init.h"
#include "xynet/socket/impl/setsockopt.h"

#include "xynet/socket/impl/accept.h"
#include "xynet/socket/impl/connect.h"
#include "xynet/socket/impl/recv_all.h"
#include "xynet/socket/impl/send_all.h"
#include "xynet/socket/impl/close.h"

#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/async_scope.h"

#include <stop_token>
#include <random>
#include <iostream>

using namespace xynet;
using namespace std;

using socket_t = file_descriptor
<
  detail::module_list
  <
    address,
    socket_init,
    operation_shutdown,
    operation_bind,
    operation_listen,
    operation_set_options,
    operation_accept,
    operation_connect,
    operation_close,
    operation_recv,
    operation_send
  >
>;

using socket_sync_t = file_descriptor
<
  detail::module_list
  <
    address,
    socket_init,
    operation_shutdown,
    operation_bind,
    operation_listen,
    operation_set_options,
    operation_accept
  >
>;

auto acceptor(auto client,
              xynet::io_service& service,
              uint16_t port) 
-> xynet::task<>
{
  auto scope = xynet::async_scope{};
  auto listen_socket = socket_t{};
  auto ex = std::exception_ptr{};

  try
  {
    listen_socket.init();
    listen_socket.reuse_address();
    listen_socket.bind(xynet::socket_address{port});
    listen_socket.listen();

    for(;;)
    {
      auto peer_socket = socket_t{};
      co_await listen_socket.accept(peer_socket);
      scope.spawn(client(std::move(peer_socket)));
    }
  }
  catch(...)
  {
    ex = std::current_exception();
  }

  co_await scope.join();
  service.request_stop();

  if(ex)
  {
    std::rethrow_exception(ex);
  }

  co_return;
}


int main()
{
  auto service = io_service{};
  auto discard_exception = [&](socket_t peer_socket) -> task<>
  {
    array<byte, 3> buffer{};
    try
    {
      for(;;)
      {
        auto recv_bytes = co_await peer_socket.recv(buffer);
        [[maybe_unused]]
        auto send_bytes = co_await peer_socket.send(std::span{buffer.data(), recv_bytes});
      }
    }catch(...){}
  };

  auto service_spin = [&service](stop_token token)->task<>
  {
    service.run(token);
    co_return;
  };

  auto source = stop_source{};

  sync_wait(when_all(
    acceptor(discard_exception, service, uint16_t{25565}), service_spin(source.get_token())
  ));

}

// using namespace xynet;


// // auto test(io_service& service) -> task<>
// // {
// //   auto f = file_t{};
// //   auto listen_socket = socket_t{};
// //   auto s = socket_t{};
// //   try
// //   {
// //     listen_socket.init();
// //     listen_socket.bind(socket_address{25565});
// //     listen_socket.reuse_address();
// //     listen_socket.listen();
// //     co_await listen_socket.accept(s);

// //     co_await f.openat("/home/xuanyi/1.txt", O_RDONLY, 0u);
// //     co_await f.update_statx();

// //     // co_await s.splice(f, 0, f.file_size());

// //   }catch(const std::exception& ex)
// //   {
// //     std::puts(ex.what());
// //   }

// //   service.request_stop();
// // }

// // auto run_service(io_service& service) -> task<>
// // {
// //   service.run();
// //   co_return;
// // }

// // int main()
// // {
// //   auto service = io_service{};
// //   auto t = when_all(test(service), run_service(service));
// //   sync_wait(t);
// // }

// using namespace std;
// int main()
// {
//   uint32_t mask = 0xFFFFFFFFu;
//   auto data1 = vector<char>{'g','g','g'};
//   auto data2 = array<char, 3>{'g', 'g', '2'};
//   auto span1 = std::span{data1.begin(), data1.size()};
//   auto span2 = std::span{data2.begin(), data2.size()};

//   auto join_data = std::array<std::span<char, std::dynamic_extent>, 2>{span1, span2};
//   auto i = websocket_mask(join_data | std::views::join, mask, 0);
//   for(auto c : data1)
//   {
//     std::cout<<c;
//   }
//   i = websocket_mask(join_data |std::views::join, mask, 0);
//     for(auto c : data1)
//   {
//     std::cout<<c;
//   }
// }
