////
//// Created by xuanyi on 4/22/20.
////

#include <type_traits>
#include <string>
#include <iostream>

#include "xynet/socket/socket.h"
#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/async_scope.h"


using namespace xynet;

using socket_exception_t = xynet::socket_t;

auto timeout(io_service& service) -> task<>
{
  co_await service.run_after(std::chrono::seconds{5});
  std::cout<<"wtf\n";
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
  auto t = when_all(timeout(service), run_service(service));
  sync_wait(t);
}