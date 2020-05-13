#ifndef XYNET_HTTP_HTTP_PARSER
#define XYNET_HTTP_HTTP_PARSER

#include "xynet/http/picohttpparser.h"
#include <string_view>
#include <ranges>

namespace xynet
{

class http_parser
{
public:

  int parse(std::string_view str)
  {
    return parse(str.data(), str.size());
  }

  template<typename T, size_t Extent>
  int parse(std::span<T, Extent> span)
  {
    auto byte_span = std::as_bytes(span);
    return parse(
      reinterpret_cast<const char*>(byte_span.data()),
      byte_span.size());
  }

  [[nodiscard]]
  std::ranges::view auto headers() const
  {
    return m_headers
    | std::views::take_while([](const auto& header)
    {
      return header.name != nullptr;
    })
    | std::views::transform([](const auto& header)
    {
      return std::make_pair
      (
        std::string_view{header.name, header.name_len},
        std::string_view{header.value, header.value_len}
      );
    });
  }

  auto path() const 
  {
    return std::string_view{mp_path, m_path_len};
  }

  auto method() const
  {
    return std::string_view{mp_method, m_method_len};
  }

  auto version() const
  {
    return m_version;
  }

private:
  int parse(const char* buf, size_t len)
  {
    int ret = phr_parse_request(buf, len,
      &mp_method, &m_method_len,
      &mp_path, &m_path_len,
      &m_version,
      m_headers.data(), &m_headers_num,
      last_len);
    last_len = len;
    return ret;
  }

  const char* mp_method = nullptr;
  size_t m_method_len = 0;

  const char* mp_path = nullptr;
  size_t m_path_len = 0;

  int m_version = 0;

  std::array<::phr_header, 100> m_headers = {};
  size_t m_headers_num = 100;

  size_t last_len = 0;

};

}

#endif //XYNET_HTTP_HTTP_PARSER