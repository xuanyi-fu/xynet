#ifndef XYNET_DETAIL_SYNC_OPERATION
#define XYNET_DETAIL_SYNC_OPERATION

namespace xynet::detail
{
  template<typename Syscall, typename OnSuccess>
  decltype(auto) sync_operation
  ( 
    Syscall&& syscall, 
    OnSuccess&& on_success, 
    std::error_code& error
  )
  {
    auto ret = syscall();

    if(ret >= 0)
    {
      error.clear();
      return on_success(ret);
    }
    else
    {
      auto saved_errno = errno;
      error.assign(saved_errno, std::system_category());
    }
  }
}

#endif //XYNET_DETAIL_SYNC_OPERATION