#ifndef XYNET_DETAIL_ENDIAN
#define XYNET_DETAIL_ENDIAN

#include <type_traits>
#include <arpa/inet.h>
#include <string>

namespace xynet::detail
{
template <typename UnsignedInt>
static UnsignedInt host_to_network(UnsignedInt host);

template <typename UnsignedInt>
static UnsignedInt network_to_host(UnsignedInt net);

template <typename T> struct dependentFalse : std::false_type {};

template <typename UnsignedInt>
UnsignedInt host_to_network(UnsignedInt host [[maybe_unused]]) {
  static_assert(dependentFalse<UnsignedInt>::value,
                "SocketAddress::host_to_network instantiation failed: "
                "host must be uint32_t or uint16_t");
}

template <typename UnsignedInt>
UnsignedInt network_to_host(UnsignedInt net [[maybe_unused]]) {
  static_assert(dependentFalse<UnsignedInt>::value,
                "SocketAddress::network_to_host instantiation failed: "
                "host must be uint32_t or uint16_t");
}

template <> uint32_t host_to_network<uint32_t>(uint32_t host) {
  return ::htonl(host);
}

template <> uint16_t host_to_network<uint16_t>(uint16_t host) {
  return ::htons(host);
}

template <> uint16_t network_to_host<uint16_t>(uint16_t net) {
  return ::ntohs(net);
}

inline ::in_addr ip_str_to_in_addr(const std::string& str)
{
  auto ret = ::in_addr{};
  if(::inet_pton(AF_INET, str.c_str(), &ret) == 1)[[likely]]
  {
    return ret;
  }
  else[[unlikely]]
  {
    return {};
  }
}

inline void sockaddr_in_to_c_str(char* buf, size_t size, const ::sockaddr_in& addr)
{
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
  uint16_t port = xynet::detail::network_to_host(addr.sin_port);
  snprintf(buf, size, "%s:%u", host, port);
}

}

#endif // XYNET_DETAIL_ENDIAN