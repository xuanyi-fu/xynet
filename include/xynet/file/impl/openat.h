//
// Created by xuanyi on 5/8/20.
//

#ifndef XYNET_FILE_OPENAT_H
#define XYNET_FILE_OPENAT_H

#include "xynet/detail/async_operation.h"
#include "xynet/detail/throw_or_return.h"
#include <fcntl.h>
#include <filesystem>

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_openat
{
  struct async_openat : public async_operation<P, async_openat, false>
  {

    async_openat(F& file, std::filesystem::path path, int flags, mode_t mode) noexcept
    : async_operation<P, async_openat, false>{}
    , m_file{file}
    , m_path{std::move(path)}
    , m_file_flags{flags}
    , m_file_mode{mode}
    {}

    auto initial_check() const noexcept
    {
      return true;
    }

    auto try_start() noexcept
    {
      return [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_openat(sqe,
                               AT_FDCWD,
                               m_path.c_str(),
                               m_file_flags,
                               m_file_mode);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    -> detail::file_descriptor_operation_return_type_t<P>
    {
      if (async_operation_base::no_error_in_result())
      {
        m_file.set(async_operation_base::get_res());
      }
      return detail::async_throw_or_return<P>(async_operation_base::get_error_code());
    }

  private:
    F& m_file;
    std::filesystem::path m_path;
    int m_file_flags;
    mode_t m_file_mode;
  };


  template<typename... Args>
  [[nodiscard]]
  decltype(auto) openat(Args&&... args) noexcept 
  {
    return async_openat{*static_cast<F*>(this), std::forward<Args>(args)...};
  }
};

}

#endif
