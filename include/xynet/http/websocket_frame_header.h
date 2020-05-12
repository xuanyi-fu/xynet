// BSD 3-Clause License

// Copyright (c) 2019, PHP ION project
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef XYNET_HTTP_WEBSOCKET_FRAME_HEADER_H
#define XYNET_HTTP_WEBSOCKET_FRAME_HEADER_H
#include <type_traits>
#include <cstdint>
#include <algorithm>
#include <string_view>
#include <span>

namespace xynet
{

enum class websocket_flags : unsigned char
{
// opcodes
  WS_NONE        = 0x0,
  WS_OP_CONTINUE = 0x0,
  WS_OP_TEXT     = 0x1,
  WS_OP_BINARY   = 0x2,
  WS_OP_CLOSE    = 0x8,
  WS_OP_PING     = 0x9,
  WS_OP_PONG     = 0xA,
  WS_OP_MASK     = 0xF,

// marks
  WS_FIN         = 0x10,
  WS_FINAL_FRAME = 0x10,
  WS_HAS_MASK    = 0x20,

};

constexpr bool websocket_flags_not_none(websocket_flags flag)
{
  return static_cast<bool>(flag);
}

constexpr websocket_flags operator&(websocket_flags lhs, websocket_flags rhs)
{
  using T = std::underlying_type_t<decltype(lhs)>;
  return static_cast<websocket_flags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

constexpr websocket_flags operator|(websocket_flags lhs, websocket_flags rhs)
{
  using T = std::underlying_type_t<decltype(lhs)>;
  return static_cast<websocket_flags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

constexpr websocket_flags operator^(websocket_flags lhs, websocket_flags rhs)
{
  using T = std::underlying_type_t<decltype(lhs)>;
  return static_cast<websocket_flags>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
}

constexpr websocket_flags operator~(websocket_flags lhs)
{
  using T = std::underlying_type_t<decltype(lhs)>;
  return static_cast<websocket_flags>(~static_cast<T>(lhs));
}

constexpr websocket_flags& operator&=(websocket_flags& lhs, websocket_flags rhs)
{
  lhs = lhs & rhs;
  return lhs;
}

constexpr websocket_flags& operator|=(websocket_flags& lhs, websocket_flags rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

constexpr websocket_flags& operator^=(websocket_flags& lhs, websocket_flags rhs)
{
  lhs = lhs ^ rhs;
  return lhs;
}



namespace detail
{

constexpr decltype(auto) calc_frame_header_size(websocket_flags flags, std::size_t data_len)
{
  std::size_t size = 2;
  if(data_len >= 126) {
    if(data_len > 0xFFFF) {
      size += 8;
    } else {
      size += 2;
    }
  }
  if(websocket_flags_not_none(
    flags & websocket_flags::WS_HAS_MASK)) {
    size += 4;
  }
  return size;
}

constexpr decltype(auto) calc_frame_size(websocket_flags flags, std::size_t data_len)
{
  return data_len + calc_frame_header_size(flags, data_len);
}

//TODO: constinit
inline constexpr const auto WS_MAX_FRAME_HEADER_SIZE = calc_frame_header_size(websocket_flags::WS_HAS_MASK, UINT32_MAX);

size_t websocket_frame_header_builder(unsigned char *frame, websocket_flags flags, const char *mask, std::size_t data_len)
{
  size_t body_offset = 0;
  frame[0] = 0;
  frame[1] = 0;
  if(websocket_flags_not_none(flags & websocket_flags::WS_FIN)) {
    frame[0] = (char) (1u << 7u);
  }
  frame[0] |= static_cast<unsigned char>(flags & websocket_flags::WS_OP_MASK);
  if(websocket_flags_not_none(flags & websocket_flags::WS_HAS_MASK)) {
    frame[1] = (char) (1u << 7u);
  }
  if(data_len < 126u) {
    frame[1] |= data_len;
    body_offset = 2;
  } else if(data_len <= 0xFFFF) {
    frame[1] |= 126u;
    frame[2] = (char) (data_len >> 8u);
    frame[3] = (char) (data_len & 0xFFu);
    body_offset = 4;
  } else {
    frame[1] |= 127u;
    frame[2] = (char) ((data_len >> 56u) & 0xFFu);
    frame[3] = (char) ((data_len >> 48u) & 0xFFu);
    frame[4] = (char) ((data_len >> 40u) & 0xFFu);
    frame[5] = (char) ((data_len >> 32u) & 0xFFu);
    frame[6] = (char) ((data_len >> 24u) & 0xFFu);
    frame[7] = (char) ((data_len >> 16u) & 0xFFu);
    frame[8] = (char) ((data_len >>  8u) & 0xFFu);
    frame[9] = (char) ((data_len)        & 0xFFu);
    body_offset = 10;
  }
  if(websocket_flags_not_none(flags & websocket_flags::WS_HAS_MASK)) {
    if(mask != nullptr) {
      std::copy(mask, mask + 4, &frame[body_offset]);
    }
    body_offset += 4;
  }
  return body_offset;
}

} // namespace detail

class websocket_frame_header
{
public:

  websocket_frame_header(websocket_flags flags, std::size_t data_len) noexcept
  {
    m_header_length =
      detail::websocket_frame_header_builder(
        reinterpret_cast<unsigned char*>(m_header.data()),
        flags, nullptr, data_len);
  }

  websocket_frame_header(websocket_flags flags, std::span<char, 4> mask, std::size_t data_len) noexcept
  :websocket_frame_header{flags, data_len}
  {
    std::copy(mask.data(), mask.data() + mask.size(), m_mask.data());
  }

  websocket_frame_header(websocket_flags flags, uint32_t mask, std::size_t data_len) noexcept
  :websocket_frame_header{flags, data_len}
  {
    std::copy(reinterpret_cast<char *>(&mask),
      reinterpret_cast<char *>(&mask) + 4, m_mask.data());
  }

  [[nodiscard]]
  std::string_view view() const
  {
    return std::string_view{m_header.data(), m_header_length};
  }

  [[nodiscard]]
  decltype(auto) span() const
  {
    return std::span<const std::byte, std::dynamic_extent>
      {
        reinterpret_cast<const std::byte*>(m_header.data()),
        m_header_length
      };
  }

private:
  std::array<char, detail::WS_MAX_FRAME_HEADER_SIZE> m_header = {};
  std::array<char, 4> m_mask = {};
  std::size_t m_header_length = 0;
}; // class websocket_frame_header

class websocket_frame_header_parser
{
public:

  decltype(auto) parse(std::string_view str)
  {
    return parse(reinterpret_cast<const unsigned char*>(str.data()), str.size());
  }

  template<typename T, std::size_t Extent>
  decltype(auto) parse(std::span<T, Extent> sp)
  {
    if constexpr (std::is_same_v<std::remove_const_t<T>, char>)
    {
      return parse(sp.data(), sp.size());
    }

    auto byte_span = std::as_bytes(sp);
    return parse(reinterpret_cast<const char*>(byte_span.data()), byte_span.size());
  }

  


private:

  enum class parser_state : uint32_t
  {
    s_start,
    s_head,
    s_length,
    s_mask,
    s_finished
  };

  size_t parse(const unsigned char* data, size_t len);

  parser_state m_state = parser_state::s_start;
  websocket_flags m_flags = websocket_flags::WS_NONE;

  std::array<char, 4> m_mask = {};

  size_t   m_length  = 0;
  size_t   m_require = 0;
};

size_t websocket_frame_header_parser::parse(const unsigned char *data, size_t len)
{
  const unsigned char * p;
  const unsigned char * end = data + len;
  size_t frame_offset = 0;

  for(p = data; p != end; p++) {
    switch(m_state) {
      case parser_state::s_start:
        m_length      = 0;
        m_flags       = static_cast<websocket_flags>(*p) & websocket_flags::WS_OP_MASK;
        if((*p) & (1u<<7u)) {
          m_flags = static_cast<websocket_flags>(m_flags | websocket_flags::WS_FIN);
        }
        m_state = parser_state::s_head;

        frame_offset++;
        break;
      case parser_state::s_head:
        m_length  = static_cast<size_t>(*p) & 0x7Fu;
        if((*p) & 0x80u) {
          m_flags |= websocket_flags::WS_HAS_MASK;
        }
        if(m_length >= 126) {
          if(m_length == 127) {
            m_require = 8;
          } else {
            m_require = 2;
          }
          m_length = 0;
          m_state = parser_state::s_length;
        } else if (websocket_flags_not_none
          (m_flags & websocket_flags::WS_HAS_MASK)) {
          m_state = parser_state::s_mask;
          m_require = 4;
        }  else {
          m_state = parser_state::s_finished;
          return p - data + 1;
        }

        frame_offset++;
        break;
      case parser_state::s_length:
        while((p < end) && m_require) {
          m_length <<= 8u;
          m_length |= static_cast<unsigned char>(*p);
          m_require--;
          frame_offset++;
          p++;
        }
        p--;
        if(!m_require) {
          if (websocket_flags_not_none(m_flags & websocket_flags::WS_HAS_MASK)) {
            m_state = parser_state::s_mask;
            m_require = 4;
          } else {
            m_state = parser_state::s_finished;
            return p - data + 1;
          }
        }
        break;
      case parser_state::s_mask:
        while((p < end) && m_require) {
          m_mask[4 - m_require--] = (*p);
          frame_offset++;
          p++;
        }
        p--;
        if(!m_require) {
          m_state = parser_state::s_finished;
          return p - data + 1;
        }
        break;
      default:
        break;
    }
  }

  // incomplete
  return -1;
}// class websocket_frame_header_parser

}// namespace xynet
#endif // XYNET_HTTP_WEBSOCKET_FRAME_HEADER_H