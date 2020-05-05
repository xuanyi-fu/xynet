#ifndef XYNET_EXAMPLE_TTCP_MESSAGE_H
#define XYNET_EXAMPLE_TTCP_MESSAGE_H

struct [[gnu::packed]] ttcp_message
{
  int32_t number;
  int32_t length;
};

struct ttcp_payload_t
{
  int32_t length;
  char    data[0];
};

#endif