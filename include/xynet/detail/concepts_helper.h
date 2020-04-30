//
// Created by Xuanyi Fu on 4/15/20.
//

#ifndef XYNET_CONCEPTS_HELPER_H
#define XYNET_CONCEPTS_HELPER_H

#include <experimental/type_traits>
#include <system_error>

namespace xynet::detail
{


//template< class T, class U >
//concept SameHelper = std::is_same_v<T, U>;
//
//
//template< class T, class U >
//concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

//template <typename T>
//using SocketPolicyUseAddressDetector = typename T::policy_socket_use_address;
//
//template <typename T>
//concept SocketPolicyUseAddress = std::experimental::is_detected_v<SocketPolicyUseAddressDetector, T>;



}

#endif //XYNET_CONCEPTS_HELPER_H
