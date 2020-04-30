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

//  [[nodiscard]]
//  io_service_schedule_operation schedule() noexcept
//  {
//    return io_service_schedule_operation{*this};
//  }
//
//  [[nodiscard]]
//  io_service_run_after_operation run_after(auto duration) noexcept
//  {
//    return io_service_run_after_operation{*this, duration};
//  }

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
