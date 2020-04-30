//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_ERROR_CODE_H
#define XYNET_ERROR_CODE_H

#include <system_error>

enum class xynet_error
{
  eof = 1,
  no_service,
};

class xynet_error_category_impl : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "xynet";
  }

  std::string message(int condition) const override
  {
    switch (static_cast<xynet_error>(condition))
    {
      case xynet_error::eof:
        return "connection read eof.";
      case xynet_error::no_service:
        return "no io_service on current thread.";
      default:
        return "unknown error";
    }
  }
};

namespace std
{
template<> struct is_error_code_enum<xynet_error> : public std::true_type{};
}

class xynet_error_instance
{
public:
  inline const static auto instance = xynet_error_category_impl{};

  static const std::error_category& xynet_category()
  {
    return xynet_error_instance::instance;
  }

  static std::error_code make_error_code(xynet_error error)
  {
    return std::error_code(static_cast<int>(error), xynet_category());
  }

  static std::error_condition make_error_condition(xynet_error error)
  {
    return std::error_condition(static_cast<int>(error), xynet_category());
  }
};

#endif //XYNET_ERROR_CODE_H
