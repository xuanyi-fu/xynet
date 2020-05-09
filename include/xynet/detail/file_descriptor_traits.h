//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_FILE_DESCRIPTOR_TRAITS_H
#define XYNET_FILE_DESCRIPTOR_TRAITS_H

#include <experimental/type_traits>
#include <tuple>

namespace xynet::detail
{

template <typename  T>
concept FileDescriptorPolicy = true;

template <typename T>
using FileDescriptorPolicyUseErrorCodeDetector = typename T::policy_use_error_code;

template <typename T>
concept FileDescriptorPolicyUseErrorCode = std::experimental::is_detected_v<FileDescriptorPolicyUseErrorCodeDetector, T>;

template<FileDescriptorPolicy P, typename... Rs>
struct file_descriptor_operation_return_type;

template <FileDescriptorPolicy P, typename... Rs>
requires (sizeof...(Rs) > 1)
struct file_descriptor_operation_return_type<P, Rs...>
{
  using type = typename std::conditional_t
    <
      FileDescriptorPolicyUseErrorCode<P>,
  std::tuple<std::error_code, Rs...>,
  std::tuple<Rs...>
  >;
};

template <FileDescriptorPolicy P, typename... Rs>
requires (sizeof...(Rs) == 1)
struct file_descriptor_operation_return_type<P, Rs...>
{
  using type = typename std::conditional_t
    <
      FileDescriptorPolicyUseErrorCode<P>,
  std::tuple<std::error_code, Rs...>,
  std::tuple_element_t<0, std::tuple<Rs...>>
  >;
};

template <FileDescriptorPolicy P, typename... Rs>
requires (sizeof...(Rs) == 0)
struct file_descriptor_operation_return_type<P, Rs...>
{
  using type = typename std::conditional_t
    <
      FileDescriptorPolicyUseErrorCode<P>,
  std::error_code,
  void
  >;
};

template <FileDescriptorPolicy P, typename... Rs>
using file_descriptor_operation_return_type_t = 
typename file_descriptor_operation_return_type<P, Rs...>::type;

}

#endif //XYNET_FILE_DESCRIPTOR_TRAITS_H
