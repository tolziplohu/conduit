#pragma once

#include <type_traits>

#include "../inlet/put=growing+type=trivial/t::SendDuct.hpp"
#include "../outlet/get=stepping+type=trivial/t::RingIrecvDuct.hpp"

namespace uit {
namespace t {

/**
 * TODO
 *
 * @tparam ImplSpec class with static and typedef members specifying
 * implementation details for the conduit framework.
 */
template<typename ImplSpec>
struct IsOriDuct {

  using InletImpl = uit::t::SendDuct<ImplSpec>;
  using OutletImpl = uit::t::RingIrecvDuct<ImplSpec>;

  static_assert(std::is_same<
    typename InletImpl::BackEndImpl,
    typename OutletImpl::BackEndImpl
  >::value);

  using BackEndImpl = typename InletImpl::BackEndImpl;

};

} // namespace t
} // namespace uit
