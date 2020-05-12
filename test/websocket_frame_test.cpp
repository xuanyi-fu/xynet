//
// Created by xuanyi on 5/11/20.
//
#include "doctest/doctest.h"
#include "xynet/buffer.h"
#include "xynet/http/websocket_frame_header.h"

using namespace xynet;

TEST_CASE("websocket frame header test")
{
  auto flags  = websocket_flags::WS_NONE;
  auto length = UINT32_MAX;

  SUBCASE("zero length")
  {
    length = 0;
  }

  SUBCASE("length < 126")
  {
    length = 120;
  }

  SUBCASE("length == 126")
  {
    length = 126;
  }

  SUBCASE("length > 126 && length < 0xffffu")
  {
    length = 0xffffu - 1234u;
  }

  SUBCASE("length > 0xffffu")
  {
    length = 0xffffu + 1;
  }

  SUBCASE("flag FIN")
  {
    flags |= websocket_flags::WS_FIN;
    length = 120;
  }

  SUBCASE("flag MASK")
  {
    flags |= websocket_flags::WS_HAS_MASK;
  }

  SUBCASE("multi-flags, length < 126")
  {
    flags |= websocket_flags::WS_FIN;
    flags |= websocket_flags::WS_HAS_MASK;
    flags |= websocket_flags::WS_OP_PING;
    length = 120;
  }

  auto header = websocket_frame_header{flags, length};
  auto parser = websocket_frame_header_parser{};
  auto ret = parser.parse(header.span());
  CHECK_EQ(ret, header.span().size());
  CHECK_EQ(flags, parser.flags());
  CHECK_EQ(length, parser.length());
}

TEST_CASE("websocket frame header test2")
{
  auto flags  = websocket_flags::WS_NONE;
  flags |= websocket_flags::WS_FIN;
  flags |= websocket_flags::WS_HAS_MASK;
  flags |= websocket_flags::WS_OP_PING;
  auto length = size_t{120};

  auto header = websocket_frame_header{flags, length};
  auto header_length = header.span().size();

  for(auto span1_length = size_t{}; span1_length < header_length; ++span1_length)
  {
    auto span1 = std::span{header.span().data(), span1_length};
    auto span2 = std::span{header.span().data() + span1_length, header_length - span1_length};

    auto parser = websocket_frame_header_parser{};
    auto ret1 = parser.parse(span1);
    CHECK_EQ(ret1, websocket_frame_header_parser::npos);
    auto ret2 = parser.parse(span2);
    CHECK_EQ(flags, parser.flags());
    CHECK_EQ(length, parser.length());
  }  
}