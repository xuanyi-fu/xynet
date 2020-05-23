//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SHUTDOWN_H
#define XYNET_SHUTDOWN_H

#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template <typename F>
struct operation_shutdown
{
  auto shutdown(int flags, std::error_code& error) -> void
  {
    detail::sync_operation
    (
      [fd = static_cast<F*>(this)->get(), flags]()
      {
        return ::shutdown(fd, flags);
      },
      []([[maybe_unused]]int){},
      error
    );
  }
  
  auto shutdown(std::error_code& error) -> void
  {
    shutdown(SHUT_WR);
  }

  auto shutdown(int flags = SHUT_WR) -> void
  {
    auto error = std::error_code{};
    shutdown(flags, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }
};

}

#endif //XYNET_SHUTDOWN_H
