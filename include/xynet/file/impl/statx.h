//
// Created by xuanyi on 5/4/20.
//

#ifndef XYNET_FILE_STATX_H
#define XYNET_FILE_STATX_H

#include "xynet/detail/async_operation.h"
#include "xynet/detail/throw_or_return.h"
#include "sys/stat.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
class file_statx
{
private:
  struct async_statx : public async_operation<P, async_statx, false>
  {
    async_statx(F& file, int flags, unsigned int mask, struct ::statx* statxbuf) noexcept
    : async_operation<P, async_statx, false>{}
    ,m_file{file}
    ,m_statx_flags{flags}
    ,m_statx_mask{mask}
    ,mp_statxbuf{statxbuf}
    {}

    auto initial_check() const noexcept
    {
      return true;
    }

    auto try_start() noexcept
    {
      return [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_statx(sqe,
                              m_file.get(),
                              &m_statx_pathname,
                              m_statx_flags | AT_EMPTY_PATH,
                              m_statx_mask  | STATX_BASIC_STATS,
                              mp_statxbuf);
        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    -> detail::file_descriptor_operation_return_type_t<P>
    {
      return detail::async_throw_or_return<P>(async_operation_base::get_error_code());
    }

  private:
    F& m_file;
    int m_statx_flags;
    unsigned int m_statx_mask;
    struct ::statx* mp_statxbuf;
    char m_statx_pathname = '\0';
};

struct ::statx m_statxbuf = {};

public:

  [[nodiscard]]
  decltype(auto) update_statx() noexcept 
  {
    return async_statx{*static_cast<F*>(this), 0, 0, &m_statxbuf};
  }

  [[nodiscard]]
  auto file_size() noexcept
  {
    return m_statxbuf.stx_size;
  }

  [[nodiscard]]
  auto last_modified() noexcept
  {
    return std::chrono::system_clock::from_time_t(
      std::time_t{m_statxbuf.stx_mtime.tv_sec}
    );
  }

};

}

#endif //XYNET_ACCEPT_H
