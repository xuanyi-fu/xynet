////
//// Created by xuanyi on 4/22/20.
////

#include <type_traits>
#include <string>
#include <iostream>
#include <ranges>


#include "xynet/file/file.h"
#include "xynet/socket/socket.h"
#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/async_scope.h"

#include "xynet/http/http_parser.h"

using namespace xynet;

auto test(io_service& service) -> task<>
{
  auto f = file_t{};
  auto listen_socket = socket_t{};
  auto s = socket_t{};
  try
  {
    listen_socket.init();
    listen_socket.bind(socket_address{25565});
    listen_socket.reuse_address();
    listen_socket.listen();
    co_await listen_socket.accept(s);

    co_await f.openat("/home/xuanyi/1.txt", O_RDONLY, 0u);
    co_await f.update_statx();

    // co_await s.splice(f, 0, f.file_size());

  }catch(const std::exception& ex)
  {
    std::puts(ex.what());
  }

  service.request_stop();
}

auto run_service(io_service& service) -> task<>
{
  service.run();
  co_return;
}

int main()
{
  auto service = io_service{};
  auto t = when_all(test(service), run_service(service));
  sync_wait(t);
}
