//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_FILE_H
#define XYNET_FILE_H

#include "xynet/socket/impl/policy.h"
#include "xynet/file_descriptor.h"
#include "xynet/file/impl/openat.h"
#include "xynet/file/impl/statx.h"
#include "xynet/file/impl/preadv.h"

namespace xynet
{

using file_t = file_descriptor<
  detail::map
    <
    detail::module_list
      <
        file_statx,
        operation_openat,
        operation_read
      >
    , detail::with<exception_policy>::template map_front
    >::type
>;

}

#endif //XYNET_SOCKET_H
