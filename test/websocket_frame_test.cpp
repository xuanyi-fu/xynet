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

  auto header = websocket_frame_header{flags, length};
  auto parser = websocket_frame_header_parser{};
  auto ret = parser.parse(header.span());
  CHECK_EQ(ret, header.span().size());
  CHECK_EQ(flags, parser.flags());
  CHECK_EQ(length, parser.length());
}