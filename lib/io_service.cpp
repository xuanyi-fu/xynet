//
// Created by xuanyi on 3/26/20.
//

#include "xynet/io_service.h"
#include "xynet/detail/scope_guard.h"
#include <utility>
#include <sys/eventfd.h>
#include <unistd.h>
#include <g3log/g3log.hpp>

namespace xynet
{

thread_local io_service* thread_io_service = nullptr;

io_service::io_service()
:m_ring{}
,m_local_queue{}
,ma_is_stop_requested{false}
,m_remote_queue_eventfd{-1}
,m_remote_queue_eventfd_poll_sqe_submitted{false}
,m_remote_queue_mutex{}
,m_remote_queue{}
{
  if(thread_io_service == nullptr)
  {
    thread_io_service = this;
  }
  else
  {
    // LOG(FATAL) << "io_service::io_service(), try to initialize another io_service on the same thread. "
    //              "the original io_service is: " << thread_io_service;
  }

  // initialize the io_uring

  if(int ret = ::io_uring_queue_init(32 * 1024, &m_ring, 0);
  ret < 0)
  {
    // LOG(FATAL) << "io_service::io_service(), io_uring_queue_init() failed, return: " << ret;
  }

  // initialize the eventfd

  if(int ret = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  ret < 0)[[unlikely]]
  {
    // LOG(FATAL) << "io_service::io_service(), ::eventfd() failed, return: " << ret;
  }
  else [[likely]]
  {
    m_remote_queue_eventfd = ret;
  }
}

io_service::~io_service()
{
  thread_io_service = nullptr;
  ::close(m_remote_queue_eventfd);
  ::io_uring_queue_exit(&m_ring);
}

void io_service::request_stop()
{
  bool expected = false;
  if(ma_is_stop_requested.compare_exchange_weak(expected, true))
  {
    enable_remote_queue_eventfd();
  }
}

void io_service::get_remote_queue_operation_bases() noexcept
{
  auto remote_operation_bases = operation_base_list{};
  {
    auto guard = std::lock_guard<std::mutex>{m_remote_queue_mutex};
    remote_operation_bases.swap(m_remote_queue);
  }

  schedule_local(remote_operation_bases);
}

bool io_service::submit_eventfd_poll_sqe() noexcept
{
  auto eventfd_poll_sqe = [this](::io_uring_sqe* sqe) noexcept
  {
    ::io_uring_prep_poll_add(sqe, m_remote_queue_eventfd, POLL_IN);
    sqe->user_data = remote_queue_eventfd_user_data();
  };

  return try_submit_io(eventfd_poll_sqe);
}

void io_service::on_remote_queue_eventfd_poll_complete(::io_uring_cqe* cqe) noexcept
{
  if(cqe->res < 0)
  {
    // LOG(FATAL) << "io_service::on_remote_queue_eventfd_poll_complete(), cqe res: "<<cqe->res;
  }
  disable_remote_queue_eventfd();
  m_remote_queue_eventfd_poll_sqe_submitted = false;
}

uint64_t io_service::remote_queue_eventfd_user_data() noexcept
{
  return reinterpret_cast<uint64_t>(&m_remote_queue_eventfd);
}

void io_service::enable_remote_queue_eventfd() noexcept
{
  uint64_t tmp{1};
  if(auto wroteBytes = ::write(m_remote_queue_eventfd, &tmp, sizeof(tmp));
  wroteBytes < 0)
  {
    int saved_errno = errno;
    // LOG(FATAL) << "io_service::enable_remote_queue_eventfd(), wroteBytes: "<<wroteBytes<<" ERRNO: "<<saved_errno;
  }
  else if(wroteBytes != sizeof(tmp))
  {
    // LOG(FATAL) << "io_service::enable_remote_queue_eventfd(), wroteBytes: "<<wroteBytes;
  }

}
void io_service::disable_remote_queue_eventfd() noexcept
{
  uint64_t tmp{1};
  if(auto readBytes = ::read(m_remote_queue_eventfd, &tmp, sizeof(tmp));
  readBytes < 0)
  {
    int saved_errno = errno;
    // LOG(FATAL) << "io_service::disable_remote_queue_eventfd(), readBytes: "<<readBytes<<" ERRNO: "<<saved_errno;
  }
  else if(readBytes != sizeof(tmp))
  {
    // LOG(FATAL) << "io_service::disable_remote_queue_eventfd(), readBytes: "<<readBytes;
  }
}

bool io_service::is_same_io_service_thread() const noexcept
{
  return this == thread_io_service;
}

io_service* io_service::get_thread_io_service() noexcept
{
  return thread_io_service;
}

void io_service::schedule_impl(async_operation_base* op) noexcept
{
  if(op == nullptr)[[unlikely]]
  {
    // LOG(WARNING) << "io_service::schedule_impl(), op == nullptr";
    return;
  }
  else if(is_same_io_service_thread())[[likely]]
  {
    schedule_local(op);
  }
  else
  {
    schedule_remote(op);
  }
}

void io_service::schedule_local(operation_base_ptr ops) noexcept
{
  m_local_queue.push_back(ops);
}
void io_service::schedule_local(operation_base_list& ops) noexcept
{
  m_local_queue.splice(m_local_queue.end(), ops);
}

void io_service::schedule_remote(operation_base_ptr op) noexcept
{
  bool need_enable_eventfd{false};

  {
    auto guard = std::lock_guard<std::mutex>(m_remote_queue_mutex);
    if(m_remote_queue.empty())
    {
      need_enable_eventfd = true;
    }
    m_remote_queue.push_back(op);
  }

  if(need_enable_eventfd)
  {
    enable_remote_queue_eventfd();
  }

}

void io_service::get_completion_queue_operation_bases() noexcept
{
  auto cq_operation_bases = operation_base_list{};
  ::io_uring_cqe* cqe;
  unsigned head;
  int cqe_count = 0;
  io_uring_for_each_cqe(&m_ring, head, cqe)
  {
    ++cqe_count;

    // cqe is the completion of eventfd poll
    if(cqe->user_data == remote_queue_eventfd_user_data())
    {
      on_remote_queue_eventfd_poll_complete(cqe);
      m_remote_queue_eventfd_poll_sqe_submitted = false;
      continue;
    }
    //TODO: use some address other than nullptr
    else if(cqe->user_data == 0)
    {
      // This indicates that this entry is a link timeout, just ignore it.
      continue;
    }

    // cqe is the completion of socket_init I/O
    auto& cqe_io_state = *reinterpret_cast<operation_base_ptr>(
        static_cast<uintptr_t>(cqe->user_data)
        );
    cqe_io_state.set_value(cqe->res, cqe->flags);
    cq_operation_bases.push_back(&cqe_io_state);
  }
  // LOG(INFO) << "io_service::get_completion_queue_operation_bases() has processed " << cqe_count << "cqes.";
  ::io_uring_cq_advance(&m_ring, cqe_count);
  schedule_local(cq_operation_bases);
}

void io_service::execute_pending_local() noexcept
{
  if(m_local_queue.empty())
  {
    return;
  }

  auto pending_num = m_local_queue.size();
  auto pendingList = operation_base_list{};
  pendingList.swap(m_local_queue);

  for(auto* state : pendingList)
  {
    state->execute(state);
  }

  // LOG(INFO) << "io_service::execute_pending_local() has executed " << pending_num << "operations.";
}

void io_service::run() noexcept
{
  auto* old_io_service = std::exchange(thread_io_service, this);
  scope_guard _{[old_io_service]{std::exchange(thread_io_service, old_io_service);}};

  while(true)
  {
    execute_pending_local();

    if(ma_is_stop_requested.load())
    {
      break;
    }

    if(!m_remote_queue_eventfd_poll_sqe_submitted)
    {
      m_remote_queue_eventfd_poll_sqe_submitted =
          submit_eventfd_poll_sqe();
      // // LOG(INFO) << "io_service::run_() has submitted the eventfd poll sqe";
    }

    ::io_uring_submit_and_wait(&m_ring, 1);

    get_completion_queue_operation_bases();
    get_remote_queue_operation_bases();
  }
}


}