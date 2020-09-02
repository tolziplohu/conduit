#pragma once

#include <stddef.h>

#include "../proc/backend/MockBackEnd.hpp"

namespace uit {

/**
 * TODO
 *
 * @tparam ImplSpec class with static and typedef members specifying
 * implementation details for the conduit framework.
 */
template<typename ImplSpec>
class NopDuct {

  using T = typename ImplSpec::T;

public:

  using InletImpl = NopDuct<ImplSpec>;
  using OutletImpl = NopDuct<ImplSpec>;
  using BackEndImpl = uit::MockBackEnd<ImplSpec>;

  T val{};

  /**
   * TODO.
   */
  template <typename... Args>
  NopDuct(Args&&... args) { ; }

  /**
   * TODO.
   *
   * @param val TODO.
   */
  bool TryPut(const T&) { return false; }

  /**
   * TODO.
   *
   * @return TODO.
   */
  const T& Get() const { return val; }

  /**
   * TODO.
   *
   * @return TODO.
   */
  T& Get() { return val; }

  /**
   * TODO.
   *
   * @return TODO.
   */
  size_t TryConsumeGets(size_t) { return 0; }

  /**
   * TODO.
   */
  static std::string GetName() { return "NopDuct"; }

  /**
   * TODO.
   */
  std::string ToString() const { return std::string{}; }

};

} // namespace uit
