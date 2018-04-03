#pragma once

#include <memory>

namespace common
{
  class interface_base
  {
  };

  template<typename T, typename base = interface_base>
  struct interface : virtual public base
  {
    using ptr = std::unique_ptr<T>;
    using ref = std::shared_ptr<T>;
    using weak_ref = std::weak_ptr<T>;

    virtual ~interface() = default;
  };
} // namespace common