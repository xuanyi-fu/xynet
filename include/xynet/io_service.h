//
// Created by xuanyi on 3/26/20.
//

#ifndef XYNET_IO_SERVICE_H
#define XYNET_IO_SERVICE_H

#include <list>
#include <mutex>
#include <liburing.h>
#include <chrono>
#include <atomic>

#include "xynet/async_operation_base.h"
#include "xynet/detail/timeout_storage.h"

namespace xynet
{
class operation_base;
class io_service
{
public:
  using operation_base_ptr = async_operation_base*;
  using operation_base_list = std::list<operation_base_ptr>;
  io_service();
  ~io_service();

  void request_stop();

  io_service(io_service&&) = delete;
  io_service(const io_service&) = delete;
  io_service& operator=(const io_service&) = delete;
  io_service& operator=(io_service&&) = delete;

  void run() noexcept;

  class run_after_operation : public async_operation_base
  {
  public:
    template<typename Duration>
    run_after_operation(io_service* service, Duration&& duration)
    :async_operation_base{service}
    ,m_timeout{std::forward<Duration>(duration)}
    {}

    bool await_ready() noexcept
    {
      return m_timeout.is_zero_timeout();
    }

    void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
    {
      set_awaiting_coroutine(awaiting_coroutine);
      get_service()->try_submit_io([this](::io_uring_sqe* sqe)
      {
        ::io_uring_prep_timeout(sqe, m_timeout.get_timespec_ptr(), 0, 0);
        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      });
    }

    void await_resume() noexcept{return;}

  private:
    detail::timeout_storage<true> m_timeout;
  };

//  [[nodiscard]]
//  io_service_schedule_operation schedule() noexcept
//  {
//    return io_service_schedule_operation{*this};
//  }
//
  
  template<typename Duration>
  [[nodiscard]]
  decltype(auto) run_after(Duration&& duration) noexcept
  {
    return run_after_operation{this, std::forward<Duration>(duration)};
  }

  void schedule_impl(operation_base_ptr) noexcept;
  void schedule_local(operation_base_list&) noexcept;
  void schedule_local(operation_base_ptr) noexcept;
  void schedule_remote(operation_base_ptr) noexcept;

  void execute_pending_local() noexcept;
  void get_completion_queue_operation_bases() noexcept;

  template<typename F>
  bool try_submit_io(F func) noexcept
  {
    //TODO: overflow
    io_uring_sqe* sqe = ::io_uring_get_sqe(&m_ring);
    func(sqe);
    return true;
  }

  template<typename F>
  bool try_submit_io(F func, ::__kernel_timespec* ts) noexcept
  {
    //TODO: overflow

    io_uring_sqe* io_sqe = ::io_uring_get_sqe(&m_ring);
    func(io_sqe);
    io_sqe->flags |= IOSQE_IO_LINK;

    io_uring_sqe* timeout_sqe = ::io_uring_get_sqe(&m_ring);
    io_uring_prep_link_timeout(timeout_sqe, ts, 0);
    timeout_sqe->user_data = 0;

    return true;
  }

  [[nodiscard]]
  bool is_same_io_service_thread() const noexcept;
  static io_service* get_thread_io_service() noexcept;

private:
  ::io_uring m_ring;
  operation_base_list m_local_queue;

  /* stop */

  std::atomic_bool ma_is_stop_requested;

  /* remote queue */
  int m_remote_queue_eventfd;
  bool m_remote_queue_eventfd_poll_sqe_submitted;
  void get_remote_queue_operation_bases() noexcept;
  bool submit_eventfd_poll_sqe() noexcept;
  uint64_t remote_queue_eventfd_user_data() noexcept;
  void enable_remote_queue_eventfd() noexcept;
  void on_remote_queue_eventfd_poll_complete(::io_uring_cqe*) noexcept;
  void disable_remote_queue_eventfd() noexcept;

  /* data members that will be accessed by other threads */
  std::mutex m_remote_queue_mutex;
  operation_base_list m_remote_queue;
};
}

#endif // XYNET_IO_SERVICE_H
