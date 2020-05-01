//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_H
#define XYNET_SOCKET_H

#include "xynet/socket/impl/policy.h"

#include "xynet/socket/impl/socket_init.h"
#include "xynet/socket/impl/address.h"

#include "xynet/socket/impl/setsockopt.h"
#include "xynet/socket/impl/bind.h"
#include "xynet/socket/impl/listen.h"
#include "xynet/socket/impl/shutdown.h"

#include "xynet/socket/impl/accept.h"
#include "xynet/socket/impl/send_all.h"
#include "xynet/socket/impl/recv_all.h"
#include "xynet/socket/impl/close.h"

namespace xynet
{

using socket_t = file_descriptor<
  map
    <
    detail::module_list
      <
        socket_init,
        address,
        operation_shutdown,
        operation_set_options,
        operation_bind,
        operation_listen,
        operation_accept,
        operation_send,
        operation_recv,
        operation_close
      >
    , with<exception_policy>::template map_front
    >::type
>;

}

#endif //XYNET_SOCKET_H
