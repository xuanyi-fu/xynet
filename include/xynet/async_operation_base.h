//
// Created by Xuanyi Fu on 4/16/20.
//

#ifndef XYNET_ASYNC_OPERATION_BASE_H
#define XYNET_ASYNC_OPERATION_BASE_H

#include <coroutine>
#include "xynet/detail/error_code.h"

namespace xynet
{

class io_service;

class async_operation_base
{
public:
  using callback_t = void(async_operation_base*);

  async_operation_base() = default;

  explicit async_operation_base(io_service* service) noexcept
  :mp_service{service}
  {}

  async_operation_base(io_service* service, callback_t callback) noexcept
  :mp_service{service}
  ,m_callback{callback}
  {}

  void set_value(int res, int flags) noexcept
  {
    m_res = res;
    m_flags = flags;
  }

  void execute(async_operation_base* op) noexcept
  {
    m_callback(op);
  }

  auto get_service() const noexcept
  {
    return mp_service;
  }

  auto get_res() const noexcept
  {
    return m_res;
  }
  auto get_flags() const noexcept
  {
    return m_flags;
  }

  static void on_operation_completed(async_operation_base *base) noexcept
  {
    base->m_awaiting_coroutine.resume();
  }

protected:

  void set_callback(callback_t callback) noexcept
  {
    m_callback = callback;
  }

  auto get_awaiting_coroutine() noexcept
  {
    return m_awaiting_coroutine;
  }

  void set_res(int res) noexcept
  {
    m_res = res;
  }

  void set_awaiting_coroutine(std::coroutine_handle<> awaiting_coroutine) noexcept
  {
    m_awaiting_coroutine = awaiting_coroutine;
  }

  auto no_error_in_result() const noexcept
  {
    return get_service() != nullptr && get_res() >= 0;
  }

  auto get_error_code() const noexcept
  {
    if(mp_service == nullptr)
    {
      return xynet_error_instance::make_error_code(xynet_error::no_service);
    }
    else if (m_res < 0)
    {
      return std::error_code{-m_res, std::system_category()};
    }
    else
    {
      return std::error_code{};
    }
  }

private:
  int m_res = 0;
  int m_flags = 0;
  io_service* mp_service = nullptr;
  callback_t* m_callback = &async_operation_base::on_operation_completed;
  std::coroutine_handle<> m_awaiting_coroutine = nullptr;
};

}
#endif //XYNET_ASYNC_OPERATION_BASE_H
