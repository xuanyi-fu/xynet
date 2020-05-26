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
- like C++17 [`<filesystem>`](https://en.cppreference.com/w/cpp/header/filesystem), support error handling in both `std::error_code` and exception
- Efficient:
  - Few dynamic allocation: most of the allocations are for the coroutine stack itself.
  - Few system calls compared with epoll based libraries: All asynchronous operations are submitted and reaped through io_uring by a single system call.
  
Requirements
-----

- Compiler: GCC10.0.1 (or later)
- Linux Kernel: 5.2 (or later)
- [Liburing](https://www.github.com/axboe/liburing): 0.5(or later)
- OpenSSL: for the SHA-1 in websocket(will be removed later)

Example
-----

- [RFC864 Character Generator Protocol](https://tools.ietf.org/html/rfc864): [chargen.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/simple/chargen.cpp)
- [RFC867 Daytime Protocol](https://tools.ietf.org/html/rfc867): [daytime.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/simple/daytime.cpp)
- [RFC863 Discard Protocol](https://tools.ietf.org/html/rfc863): [discard.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/simple/discard.cpp)
- [RFC862 Echo Protocol](https://tools.ietf.org/html/rfc862): [echo.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/simple/echo.cpp)
- [RFC868 Time Protocol](https://tools.ietf.org/html/rfc862): [time.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/simple/time.cpp)
- boost::asio ping-pong performance test: [pingpong](https://github.com/xuanyi-fu/xynet/tree/master/example/pingpong)
- [ttcp](https://en.wikipedia.org/wiki/Ttcp): [ttcp](https://github.com/xuanyi-fu/xynet/tree/master/example/ttcp)
- boost::asio chatroom: [chat](https://github.com/xuanyi-fu/xynet/tree/master/example/chat)
- websocket discard: [websocket_discard.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/websocket/websocket_discard.cpp)
- websocket echo: [websocket_echo.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/websocket/websocket_echo.cpp)
- [boost::beast websocket chatroom](https://www.boost.org/doc/libs/1_70_0/libs/beast/doc/html/beast/examples.html#beast.examples.chat_server): [websocket_chat.cpp](https://github.com/xuanyi-fu/xynet/blob/master/example/websocket/websocket_chat.cpp)

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
    - default constructor will initialize the underlying file descriptor to be an invalid one.
    - Mixin'ed modules use `set()` and `get()` to work with the file_descriptor. 

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

- Modules 

  All modules support error handling by 
    - `std::error_code`: by passing an lvalue reference of `std::error_code` (like C++17 [`<filesystem>`](https://en.cppreference.com/w/cpp/header/filesystem)), should there be an error, the lvalue reference passed will assign with a new error_code, otherwise, it will be [`clear()`](https://en.cppreference.com/w/cpp/error/error_code/clear).

    - `exception`. An [`std::system_error`](https://en.cppreference.com/w/cpp/header/system_error) constructed by the reason encapsulated in a `std::error_code` will be throwed if there is an error.

  - [`socket_init`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/socket_init.h)

    - `init()`, `init(std::erro_code& error)` create a IPv4/TCP socket and set the `file_descriptor` with the file_descriptor returned form [`::socket()`](http://man7.org/linux/man-pages/man2/socket.2.html).
  
  - [`address`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/address.h)

    the size of `file_descriptor` will increase if this module is used.

    Getters and setters are provided: `set_local_address(const socket_address& address)`, `set_peer_address(const socket_address& address)`, `get_local_address()`, `get_peer_address()`.
    Moudles like `operation_accept`, `operation_connect` , will set the `address` on success. 


  synchronous operation modules

  - [`operation_shutdown`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/shutdown.h)

    `shutdown(int how = SHUTWR)`, `shutdown(int how, std::error_code& error)`, `shutdown(std::error_code& error)`

  - [`operation_bind`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/bind.h)

    bind the socket to a given address. If module [`address`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/address.h) is used, set the local address on success. If `bind` with `socket_address{0}`, then a random port will be assigned and the local address will also be set on success.

    `bind(const socket_address& address)`, `bind(const socket_address& address, std::error_code& error)` 

  - [`operation_set_options`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/setsockopt.h)

    call `::setsockopt`.

    `void setsockopt(int level, int optname, const void* optval, socklen_t optlen, std::error_code& error)`, `void setsockopt(int level, int optname, const void* optval, socklen_t optlen)`,
    `void reuse_address(std::error_code& error)`,
    `void reuse_address()`

  - [`operation_listen`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/listen.h)

    put the socket into listen state
    `listen(int backlog = SOMAXCONN)`, `listen(int backlog, std::error_code& error)`, `listen(std::error_code& error)`.

  asynchronous operation modules

  all asynchronous operations provides an optional argument `duration` to impose a timeout.

  - [`operation_accept`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/accept.h)

  ```cpp
  template<typename F2, typename... Args> [[nodiscard]]
  decltype(auto) accept(F2& peer_socket, Args&&... args) noexcept
  ```

    - `peer_socket` The socket into which the new connection will be accpeted.

      1. Args is void: 
      The awaiter returned by the function will throw exception to report the error.
      A std::system_error constructed with the corresponding std::error_code will be 
      throwed if the accept operation is failed after being co_await'ed.
      2. Args is a Duration, i.e. std::chrono::duration<Rep, Period>. 
      This duration will be treated as the timeout for the operation. If the operation 
      does not finish within the given duration, the operation will be canneled and an error_code
        (std::errc::operation_canceled) will be returned. 
        
      3. Args is an lvalue reference of a std::error_code
      The awaiter returned by the function will use std::error_code to report the error.
      Should there be an error in the operation, the std::error_code passed by lvalue reference will 
      be reset. Otherwise, it will be clear.
    
      4. Args are first a Duration, second an lvalue reference of a std::error_code
      The operation will have the features described in 2 and 3.
  
  - [`operation_connect`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/connect.h)

  ```cpp
  template<typename... Args> [[nodiscard]]
  decltype(auto) connect(const socket_address& address, Args&&... args) noexcept
  ```

  `address`: the address with which the connection will be established.
  `args`: same as `args` desribed in `operation_accept`

  - [`operation_close`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/close.h)

  This operation will read from the socket until it reads a 0(eof). Then it will call close(2) on the socket.

  ```cpp
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) close(Args&&... args) noexcept 
  ```

  `args`: same as `args` desribed in `operation_accept`

  - [`operation_recv`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/recv_all.h)

    `recv([std::error_code& error], [Duration&& duration], Args&&... args)`
    where `args` will be forwarded to the constructor of `buffer_sequence`.

    - The buffers will be filled with the same order as the order they were in the lvalue reference  of a viewable range or the order they were in args. 
    - The operation will finish if there is an error(including the socket reads EOF) or all the buffers are filled. That is, it may use recvmsg(2) more than once.
    - After the operation is co_await'ed and then finishes, it will return the bytes transferred.

    `recv_some([std::error_code& error], [Duration&& duration], Args&&... args)`
    where `args` will be forwarded to the constructor of `buffer_sequence`.

    this awaiter will resume the coroutine after the first recv operation finished, regardless of whether the buffers are filled up or not.

  - [`operation_send`](https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/socket/impl/send_all.h)

    `recv([std::error_code& error], [Duration&& duration], Args&&... args)`
     where `args` will be forwarded to the constructor of `const_buffer_sequence`