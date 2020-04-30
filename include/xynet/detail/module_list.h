//
// Created by Xuanyi Fu on 4/18/20.
//

#ifndef XYNET_MODULE_LIST_H
#define XYNET_MODULE_LIST_H
namespace xynet::detail
{

/*template-list metafunctions*/

template <template <typename...> typename ... ops>
struct module_list
{};

template <typename list>
struct is_empty
{
  static constexpr bool value = false;
};

template<>
struct is_empty<module_list<>>
{
  static constexpr bool value = true;
};

/* front */

template <typename list>
struct front;

template<
  template <typename... > typename head,
  template <typename... > typename ... tail
>
struct front<module_list<head, tail...>>
{
  template<typename... Ts>
  using type  = head<Ts...>;
};

template<typename list, typename... Ts>
using front_type = typename front<list>:: template type<Ts...>;

/* push_front */

template <
  typename list,
  template <typename...> typename new_element
>
struct push_front;

template
  <
    template <typename...> typename new_element,
    template <typename...> typename ... elements
  >
struct push_front<module_list<elements...>, new_element>
{
  using type = module_list<new_element, elements...>;
};

/* pop_front */

template<typename List>
struct pop_front;

template
  <
    template <typename...> typename front,
    template <typename...> typename ...tail
  >
struct pop_front<module_list<front, tail...>>
{
  using type = module_list<tail...>;
};

template
  <
    typename list,
    template<template <typename ...> typename > typename MetaFunc,
    bool empty = is_empty<list>::value
  >
struct map;

template<
  typename list,
  template<template <typename ...> typename > typename MetaFunc
>
struct map<list, MetaFunc, false> : public push_front
  <
    typename map<typename pop_front<list>::type, MetaFunc>::type,
    MetaFunc<front<list>:: template type>::template type
  >
{};

template
  <
    typename list,
    template<template <typename ...> typename > typename MetaFunc
  >
struct map<list, MetaFunc, true>
{
  using type = list;
};

/* ------------------------------------------------------------------------------ */

/* typelist metafunctions */

/* instantiated module list */

template <typename ... Modules>
struct inst_module_list{};

/* get the front of a typelist */

template <typename List>
struct typelist_front{};

template <template <typename ...> typename List, typename Head, typename... Tails>
struct typelist_front<List<Head, Tails...>>
{
  using type = Head;
};

/* pop the front of a typelist */

template <typename List>
struct typelist_pop_front{};

template <template <typename ...> typename List, typename Head, typename... Tails>
struct typelist_pop_front<List<Head, Tails...>>
{
  using type = List<Tails...>;
};

/* find a type from a typelist */

template <typename List, typename Target>
struct typelist_find;

template<typename Target, template <typename ...> typename List, typename Head, typename... Tails>
struct typelist_find<List<Head, Tails...>, Target>
{
  using type = std::conditional_t
    <
      std::is_same_v<Target, Head>,
      Head,
      typename typelist_find<List<Tails...>, Target>::type
    >;
};

template <typename Target, template <typename ...> typename List>
struct typelist_find<List<>, Target>
{
  using type = void;
};

/* Policy Map MetaFunction Generator  */

template<typename T>
struct with
{
  template <template <typename ...> typename module>
  struct map_front
  {
    template<typename... Ts>
    using type = module<T, Ts ...>;
  };
};

}


#endif //XYNET_MODULE_LIST_H
