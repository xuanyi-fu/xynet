xynet
=====
Experimental C++ network library based on io_uring and coroutine.

Features
-----

- Header only
- Based on io_uring, the new Linux asynchronous I/O API
- Based on C++20 coroutine
- [`file_descriptor`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/file_descriptor.h) is designed in the CRTP Mixin pattern, all operations(recv, send, listen...) are modularized and thus optional.
- [`buffer_sequence`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/buffer.h) is designed to be compatible with C++20 [`std::span`](https://en.cppreference.com/w/cpp/header/span)
- Efficient:
  - Few dynamic allocation: most of the allocations are for the coroutine stack itself.
  - Few system calls compared with epoll based libraries: All asynchronous operations are submitted and reaped through io_uring by a single system call.
  
Requirements
-----

- Compiler: GCC10.0.1 (or later)
- Linux Kernel: 5.2 (or later)
- [Liburing](https://www.github.com/axboe/liburing): 0.5(or later)
- OpenSSL: for the SHA-1 in websocket(will be removed later)

Usage
-----

- `file_descriptor`

  ```cpp
  template <template <typename...> typename ... Modules>
  class file_descriptor<detail::module_list<Modules ...>>
    : public file_descriptor_base
    , public Modules<file_descriptor<detail::module_list<Modules ...>>>...
  {};
  ```

  `file_descriptor` is a handler that has unique ownership of file descriptor, like `std::unique_ptr`.
    - move-only, 
    - RAII. [`::close()`](http://man7.org/linux/man-pages/man2/close.2.html) is called in its destructor if the underlying file descriptor is valid.
    - Modularized: `Modules` are interfaces that are mixin'ed into the base class. `Modules` access `file_descriptor` by static polymorphism, i.e. cast `this` into the base class type.

- `socket_t`

  ```cpp
  using socket_t = file_descriptor
  <
    detail::module_list
    <
      socket_init,
      address,
      operation_shutdown,
      operation_set_options,
      operation_bind,
      operation_listen,
      operation_accept,
      operation_connect,
      operation_send,
      operation_recv,
      operation_close
    >
  >;
  ```

  convenient type alias that has all modules for socket. You can define you own type alias. For example, an acceptor does not need to do send or recv.

  ```cpp
  using acceptor_t = file_descriptor
  <
    detail::module_list
    <
      socket_init,
      operation_set_options,
      operation_bind,
      operation_listen,
      operation_accept
    >
  >;
  ```


- `buffer_sequence` and `const_buffer_sequence`

  `buffer_sequence` takes a sequences of Containers which satisfy the concept [`std::ranges::contiguous_range`](https://en.cppreference.com/w/cpp/ranges/contiguous_range) then transform them into [iovec](http://man7.org/linux/man-pages/man2/readv.2.html).

  `buffer_sequence` could be constructed using:
  
  ```cpp
  template<typename... Containers>
  buffer_sequence(Containers&&... containers)
  ```
  where `Containers` could be 
    - `std::span<std::byte, size or std::dynamic_extent>`
    - `std::span<Ts, size or std::dynamic_extent>`
    - contiguous ranges that are writable

  or 

  ```cpp
  template<typename BufferRange>
  requires std::ranges::viewable_range<BufferRange&>
           && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
  buffer_sequence(BufferRange& buffer_range)
  ```

  where `BufferRange` is a range of contiguous ranges. Notice that, different from the former one, `iovec`s are stored in a `std::vector` where dynamic allocation is inevitable.