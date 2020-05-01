////
//// Created by xuanyi on 4/25/20.
////
//
#ifndef XYNET_BUFFER_H
#define XYNET_BUFFER_H

#include <span>
#include <vector>
#include <climits>
#include <array>
#include <ranges>

#include <sys/uio.h>

namespace xynet
{

namespace
{

template<std::size_t Extent>
struct buffer_sequence_storage
{
  using iterator = typename std::array<::iovec, Extent>::iterator;
  using size_t   = typename std::array<::iovec, Extent>::size_type;
  template<typename... Spans>
  constexpr explicit buffer_sequence_storage(Spans&&... spans)
    :m_val{::iovec{
    .iov_base = const_cast<void *>(static_cast<const void *>(spans.data())),
    .iov_len = static_cast<decltype(std::declval<::iovec>().iov_len)>(spans.size_bytes())
  }...}
  {}

  std::array<::iovec, Extent> m_val;
};

template<std::size_t... sizes>
buffer_sequence_storage(std::span<std::byte, sizes>... spans)
-> buffer_sequence_storage<sizeof...(sizes)>;

template<>
struct buffer_sequence_storage<std::dynamic_extent>
{
  using iterator = typename std::vector<::iovec>::iterator;
  using size_t   = typename std::vector<::iovec>::size_type;
  template<typename BufferRangeView>
  requires std::ranges::view<BufferRangeView>
  explicit buffer_sequence_storage(BufferRangeView view)
    :m_val{std::ranges::begin(view), std::ranges::end(view)}
  {}

  std::vector<::iovec> m_val;
};

template <std::size_t Extent>
struct buffer_sequence_base
{
public:

  using iov_len_t = decltype(std::declval<::iovec>().iov_len);

  template<typename... Args>
  buffer_sequence_base(Args&&... args)
    :m_sequence{std::forward<Args>(args)...}
    ,m_iterator{std::ranges::begin(m_sequence.m_val)}
  {}

  auto get_iov_span() -> std::pair<::iovec*, iov_len_t> const
  {
    return m_iterator == std::ranges::end(m_sequence.m_val)
           ? std::make_pair(nullptr, 0)
           : std::make_pair(static_cast<::iovec*>(std::addressof(*m_iterator)),
                            static_cast<iov_len_t>(
                              std::distance
                                (
                                  m_iterator,
                                  std::ranges::end(m_sequence.m_val)
                                )));
  };

  auto commit(iov_len_t len) -> void
  {
    while(len > 0 && m_iterator != std::ranges::end(m_sequence.m_val))
    {
      if(len >= m_iterator->iov_len)
      {
        len -= m_iterator->iov_len;
        ++m_iterator;
      }
      else
      {
        m_iterator->iov_base = static_cast<char *>(m_iterator->iov_base) + len;
        m_iterator->iov_len  -= len;
        len = 0;
      }
    }
  }

private:
  buffer_sequence_storage<Extent> m_sequence;
  typename buffer_sequence_storage<Extent>::iterator m_iterator;
};

}

// dummy template parameter for deduction guide to work with default template parameter
template<typename, std::size_t Extent = std::dynamic_extent>
struct buffer_sequence final : public buffer_sequence_base<Extent>
{
public:
  template<typename... Containers>
  buffer_sequence(Containers&&... containers)
  :buffer_sequence_base<Extent>{std::as_writable_bytes(std::span{std::forward<Containers>(containers)})...}
  {}

  template<typename... Ts, std::size_t... sizes>
  buffer_sequence(std::span<Ts, sizes>... spans)
  :buffer_sequence_base<Extent>{std::as_writable_bytes(spans)...}
  {}

  template<std::size_t... sizes>
  buffer_sequence(std::span<std::byte, sizes>... spans)
  :buffer_sequence_base<Extent>{std::move(spans)...}
  {}

  template<typename BufferRange>
  requires std::ranges::viewable_range<BufferRange&>
           && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
  buffer_sequence(BufferRange& buffer_range)
  :buffer_sequence_base<Extent>{
    buffer_range
  | std::ranges::views::transform(
  [](auto& buffer)
  {
    return ::iovec{
      .iov_base = static_cast<void *>(std::ranges::data(buffer)),
      .iov_len  = static_cast<size_t>(std::ranges::size(buffer) *
                                      sizeof(std::ranges::range_value_t<decltype(buffer)>))};
  })}
  {}
};

template<typename... T>
buffer_sequence(T&&...) -> buffer_sequence<void, sizeof...(T)>;

template<typename... Ts, std::size_t... sizes>
buffer_sequence(std::span<Ts, sizes>... spans) -> buffer_sequence<void, sizeof...(Ts)>;

template<std::size_t... sizes>
buffer_sequence(std::span<std::byte, sizes>... spans) -> buffer_sequence<void, sizeof...(sizes)>;

template<typename BufferRange>
requires std::ranges::viewable_range<BufferRange&>
         && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
buffer_sequence(BufferRange&) -> buffer_sequence<void, std::dynamic_extent>;


/* const buffer sequence */

template<typename, std::size_t Extent = std::dynamic_extent>
struct const_buffer_sequence final : public buffer_sequence_base<Extent>
{
public:
  template<typename... Containers>
  const_buffer_sequence(Containers&&... containers)
    :buffer_sequence_base<Extent>{std::as_bytes(std::span{std::forward<Containers>(containers)})...}
  {}

  template<typename... Ts, std::size_t... sizes>
  const_buffer_sequence(std::span<Ts, sizes>... spans)
    :buffer_sequence_base<Extent>{std::as_bytes(spans)...}
  {}

  template<std::size_t... sizes>
  const_buffer_sequence(std::span<const std::byte, sizes>... spans)
    :buffer_sequence_base<Extent>{std::move(spans)...}
  {}

  template<typename BufferRange>
  requires std::ranges::viewable_range<BufferRange&>
           && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
  const_buffer_sequence(BufferRange& buffer_range)
    :buffer_sequence_base<Extent>{
    buffer_range
    | std::ranges::views::transform(
      [](auto& buffer)
      {
        return ::iovec{
          .iov_base = const_cast<void*>(static_cast<const void *>(std::ranges::data(buffer))),
          .iov_len  = static_cast<size_t>(std::ranges::size(buffer) *
                                          sizeof(std::ranges::range_value_t<decltype(buffer)>))};
      })}
  {}
};

template<typename... T>
const_buffer_sequence(T&&...) -> const_buffer_sequence<void, sizeof...(T)>;

template<typename... Ts, std::size_t... sizes>
const_buffer_sequence(std::span<Ts, sizes>... spans) -> const_buffer_sequence<void, sizeof...(Ts)>;

template<std::size_t... sizes>
const_buffer_sequence(std::span<const std::byte, sizes>... spans) -> const_buffer_sequence<void, sizeof...(sizes)>;

template<typename BufferRange>
requires std::ranges::viewable_range<BufferRange&>
         && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
const_buffer_sequence(BufferRange&) -> const_buffer_sequence<void, std::dynamic_extent>;

}

#endif //XYNET_BUFFER_H