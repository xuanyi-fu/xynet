//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_POLICY_H
#define XYNET_SOCKET_POLICY_H

namespace xynet
{

struct exception_policy{};

struct error_code_policy
{
  using policy_use_error_code = void;
};

}

#endif //XYNET_POLICY_H
