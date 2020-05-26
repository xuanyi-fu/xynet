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

- `buffer_sequence` and `const_buffer_sequence`

  `buffer_sequence` takes a sequences of Containers which satisfy the concept [`std::ranges::contiguous_range`](https://en.cppreference.com/w/cpp/ranges/contiguous_range) then transform them into [iovec](http://man7.org/linux/man-pages/man2/readv.2.html).

  `buffer_sequence` could be constructed by:

  ```cpp
  template<typename... Containers>
  buffer_sequence(Containers&&... containers)
  ```

  or 

  ```cpp
  template<typename BufferRange>
  requires std::ranges::viewable_range<BufferRange&>
           && std::ranges::contiguous_range<std::ranges::range_value_t<BufferRange>>
  buffer_sequence(BufferRange& buffer_range)
  ```