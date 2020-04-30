//
// Created by xuanyi on 4/22/20.
//

#ifndef XYNET_UNWRAP_REFERENCE_H
#define XYNET_UNWRAP_REFERENCE_H


///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_UNWRAP_REFERENCE_HPP_INCLUDED
#define CPPCORO_DETAIL_UNWRAP_REFERENCE_HPP_INCLUDED

#include <functional>

namespace xynet
{
namespace detail
{
template<typename T>
struct unwrap_reference
{
  using type = T;
};

template<typename T>
struct unwrap_reference<std::reference_wrapper<T>>
{
  using type = T;
};

template<typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;
}
}

#endif

#endif //XYNET_UNWRAP_REFERENCE_H
