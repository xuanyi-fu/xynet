//
// Created by Xuanyi Fu on 4/15/20.
//

#ifndef XYNET_FILE_DESCRIPTOR_H
#define XYNET_FILE_DESCRIPTOR_H

#include <unistd.h>
#include <utility>
#include <xynet/detail/file_descriptor_traits.h>
#include <xynet/detail/module_list.h>

namespace xynet
{

class file_descriptor_base
{
public:

  file_descriptor_base() noexcept
  : m_fd{-1}
  {}

  explicit file_descriptor_base(int fd) noexcept : m_fd{fd} {}
  file_descriptor_base(file_descriptor_base&& other)noexcept :m_fd{std::exchange(other.m_fd, -1)}{}
  ~file_descriptor_base(){ if(valid()){close__();}}
  void set(int fd) noexcept
  {
    m_fd = fd;
  }
  [[nodiscard]] int get() const noexcept {return m_fd;}
  [[nodiscard]] bool valid() const noexcept {return m_fd >= 0;}
  void close__() noexcept {::close(std::exchange(m_fd, -1));}
private:
  int m_fd;
};

template <typename ModuleList>
struct file_descriptor;

template <template <typename...> typename ... Modules>
class file_descriptor<detail::module_list<Modules ...>>
  : public file_descriptor_base
  , public Modules<file_descriptor<detail::module_list<Modules ...>>>...
{};

/* file descriptor traits */


/* get the instantiated module list from the file descriptor */

template <typename F>
struct file_descriptor_inst_module_list;

template<template <typename ...> typename... Modules>
struct file_descriptor_inst_module_list<file_descriptor<detail::module_list<Modules ...>>>
{
  using type = detail::inst_module_list<Modules<void>...>;
};

/* match an inst module with a module template */

template<typename L, template <typename ...> typename R>
struct module_match : public std::false_type {};

template<typename... T, template <typename ...> typename R>
struct module_match<R<T...>, R> : public std::true_type
{};

/* find the instantiated module from an instantiated module list */

template<typename List, template <typename ...> typename Target>
struct inst_module_list_match
{
  using type = void;
};

template<template <typename...> typename Target, typename Head, typename... Tails>
struct inst_module_list_match<detail::inst_module_list<Head, Tails...>, Target>
{
  using type = std::conditional_t
    <
    module_match<Head, Target>::value,
    Head,
    typename inst_module_list_match<detail::inst_module_list<Tails...>, Target>::type
    >;
};

template<template <typename...> typename Target>
struct inst_module_list_match<detail::inst_module_list<>, Target>
{
  using type = void;
};

/* get the policy associated with a module template */

template <typename Module>
struct get_policy_from_module;

template <template <typename...> typename Module, typename T>
struct get_policy_from_module<Module<T, void>>
{
  using type = T;
};

template <>
struct get_policy_from_module<void>
{
  using type = void;
};

/* check if a file descriptor has a module */

template<typename FileDescriptor, template <typename ...> typename Module>
inline constexpr bool file_descriptor_has_module_v =
  not std::is_same_v
    <
    typename inst_module_list_match
      <
        typename file_descriptor_inst_module_list<FileDescriptor>::type,
        Module
      >::type,
    void
    >;
/* get the policy associated with a module template in a file descriptor */

template <typename FileDescriptor, template <typename ...> typename Module>
using policy_from_file_descriptor_t =
typename get_policy_from_module
<
  typename inst_module_list_match
    <
      typename file_descriptor_inst_module_list<FileDescriptor>::type, Module
    >::type
>::type;


}

#endif //XYNET_FILE_DESCRIPTOR_H
