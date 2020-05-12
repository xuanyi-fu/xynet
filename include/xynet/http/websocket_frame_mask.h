#ifndef XYNET_HTTP_WEBSOCKET_FRAME_MASK_H
#define XYNET_HTTP_WEBSOCKET_FRAME_MASK_H

#include <ranges>

template<typename R>
requires std::ranges::range<R> 
&& std::disjunction_v
<
std::is_convertible<std::ranges::range_value_t<R>, std::byte>,
std::is_convertible<std::ranges::range_value_t<R>, char>,
std::is_convertible<std::ranges::range_value_t<R>, unsigned char>
>
auto websocket_mask(R&& data, uint32_t mask, size_t i)
{
  std::ranges::transform(data, std::ranges::begin(data),
  [mask_ptr = reinterpret_cast<const std::byte*>(&mask), &i](auto c) mutable 
  {
    //gcc10 seems missing the operator^ between bytes
    return static_cast<decltype(c)>(static_cast<unsigned char>(c) 
      ^ static_cast<unsigned char>(mask_ptr[i++ % 4]));
  });

  return i;
}

#endif //XYNET_HTTP_WEBSOCKET_FRAME_MASK_H