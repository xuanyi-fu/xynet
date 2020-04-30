//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ASYNC_OPERATION_H
#define XYNET_SOCKET_ASYNC_OPERATION_H

#include "xynet/async_operation_base.h"
#include "xynet/io_service.h"

namespace xynet
{
template<detail::FileDescriptorPolicy P, typename T>
class async_operation : public async_operation_base
{
public:
  async_operation() noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  {}

  explicit async_operation(async_operation_base::callback_t callback) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  {}

  bool await_ready() noexcept
  {
    return get_service() == nullptr;
  };

  void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
  {
    set_awaiting_coroutine(awaiting_coroutine);
    static_cast<T *>(this)->try_start();
  }

  decltype(auto) await_resume()
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return static_cast<T *>(this)->get_result();
  }
};
}

#endif //XYNET_ASYNC_OPERATION_H
