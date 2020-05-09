//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_FILE_PREADV_H
#define XYNET_FILE_PREADV_H

#include <type_traits>
#include "xynet/buffer.h"
#include "xynet/detail/async_operation.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{
template<detail::FileDescriptorPolicy P, typename F>
struct operation_read
{
  template<typename BufferSequence>
  struct async_read : public async_operation<P, async_read<BufferSequence>, false>
  {
    template<typename... Args>
    async_read(F& file, off_t offset, Args&&... args) noexcept
    :async_operation<P, async_read, false>{}
    ,m_file{file}
    ,m_buffers{static_cast<Args&&>(args)...}
    ,m_offset{offset}
    {}

    auto initial_check() const noexcept
    {
      return true;
    }

    [[nodiscard]]
    auto try_start() noexcept 
    {
      return [this](::io_uring_sqe* sqe)
      {
        ::io_uring_prep_readv(sqe,
                              m_file.get(),
                              m_buffers.get_iov_ptr(),
                              m_buffers.get_iov_cnt(),
                              m_offset);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    ->detail::file_descriptor_operation_return_type_t<P, std::size_t>
    {
      return async_throw_or_return<P>(async_operation_base::get_error_code()
      , static_cast<std::size_t>(async_operation_base::get_res()));
    }

  private:
    F& m_file;
    BufferSequence m_buffers;
    off_t m_offset;
  };

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) read_some(Args&&... args)
  noexcept
  {
    return async_read<decltype(buffer_sequence{std::forward<Args>(args)...})>
    {*static_cast<F*>(this), 0, std::forward<Args>(args)...};
  }

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) read_some_offset(off_t offset, Args&&... args)
  noexcept
  {
    return async_read<decltype(buffer_sequence{std::forward<Args>(args)...})>
    {*static_cast<F*>(this), offset, std::forward<Args>(args)...};
  }

};

}

#endif //XYNET_READ_H
