//
// Created by xuanyi on 3/27/20.
//

#ifndef XYNET_SCOPE_GUARD_H
#define XYNET_SCOPE_GUARD_H

#include <type_traits>

template <typename T, typename... Args>
concept SCOPE_GUARD_FUNC = true;

namespace xynet
{
template <SCOPE_GUARD_FUNC F>
struct scope_guard
{
  [[no_unique_address]] F m_func;

  explicit scope_guard(F&& func) noexcept
  :m_func(static_cast<F&&>(func)){}

  ~scope_guard()
  {
    static_cast<F&&>(m_func)();
  }
};

template <SCOPE_GUARD_FUNC F>
scope_guard(F&& f)->scope_guard<F>;

}


#endif // XYNET_SCOPE_GUARD_H
