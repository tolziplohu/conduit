#pragma once
#ifndef UIT_DUCTS_DUCT_HPP_INCLUDE
#define UIT_DUCTS_DUCT_HPP_INCLUDE

#include <stddef.h>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "../../../third-party/Empirical/include/emp/base/assert.hpp"
#include "../../../third-party/Empirical/include/emp/base/optional.hpp"
#include "../../../third-party/Empirical/include/emp/meta/TypePack.hpp"
#include "../../../third-party/Empirical/include/emp/tools/string_utils.hpp"

#include "../../uitsl/containers/safe/unordered_map.hpp"
#include "../../uitsl/math/math_utils.hpp"
#include "../../uitsl/meta/HasMemberFunction.hpp"
#include "../../uitsl/mpi/mpi_init_utils.hpp"
#include "../../uitsl/parallel/thread_utils.hpp"
#include "../../uitsl/utility/print_utils.hpp"

namespace uit {
namespace internal {

UITSL_GENERATE_HAS_MEMBER_FUNCTION( CanStep );

/**
 * Performs data transmission between an `Inlet` and an `Outlet.
 *
 * Under the hood, a `Duct` actually wraps a `std::variant` of three
 * implementation types:
 *
 *  1. `IntraDuct`: implementation type that performs transmission between an
 *     `Inlet` and `Outlet` that reside on the same thread in the same process.
 *  2. `ThreadDuct`: implementation type that performs transmission betweeen an
 *     `Inlet` and `Outlet` that reside on different threads, but are within the
 *     same process.
 *  3. `ProcDuct`: implementation type that performs transmission between an
 *     `Inlet` and `Outlet` that reside on different processes. (See note.)
 *
 * The `std::variant` template represents a type-safe union. A `std::variant`
 * instance holds an instance of just one of its alternative types. However,
 * the type which a `std::variant` instance holds may be freely determined and
 * arbitrarily switched during the lifetime of the instance.
 *
 * When a `Duct` is constructed, the `IntraDuct` implementation is constructed
 * and held by the `Duct`'s `std::variant`. An alternate implementation (i.e.,
 * a thread-safe or process-safe implementaiton) may be emplaced within the
 * `Duct`'s `std::variant` by calling `EmplaceDuct`. This design enables the
 * `Duct`'s active implementation to be switched at run-time, even if after
 * the `Duct` has been already transmitted through. However, when the `Duct`'s
 * implementation is switched, no attempt is made to transfer state (i.e.,
 * pending data) between the destructed implementation and its replacement. All
 * `Duct` operations are forwarded to the active implementation type within a
 * `Duct`'s `std::variant`.
 *
 * The actual identity of the `IntraDuct`, `ThreadDuct`, and `ProcDuct`
 * implementations are themselves modular and may be specified via the
 * `ImplSpec` template paramater. You can find available intra-thread,
 * inter-thread, and inter-process implementations within the
 * `include/ducts/intra`, `include/ducts/thread`, and `include/ducts/proc`
 * directories, respectively. You may also supply your own implementations so
 * long as they satisfy the method signatures within this class that are
 * forwarded to the `std::variant`'s active implementation.
 *
 * @tparam ImplSpec class with static and typedef members specifying
 *   implementation details for the conduit framework. See
 *  `include/uit/setup/ImplSpec.hpp`.
 *
 * @note Because processes do not share memory space and inter-process
 *   communication code is typically asymmetric between the sender and
 *   receiver, the `ProcDuct` implementation actually corresponds to two
 *   distinct implementations: a `ProcInletDuct` implementaiton and a
 *   `ProcOutletDuct`implementation. The `ProcInletDuct` performs the
 *   inter-process sending role and the `ProcOutletDuct` performs the
 *   inter-process receiving role. So, in reality a `Duct` wraps a
 *   `std::variant` of four implementation types.
 * @note End users should probably never have to directly instantiate this
 *   class. The `Conduit`, `Sink`, and `Source` classes take care of creating a
 *   `Duct` and tying it to an `Inlet` and/or `Outlet`. Better yet, the
 *   `MeshTopology` interface allows end users to construct a conduit network
 *    in terms of a connection topology and a mapping to assign nodes to
 *    threads and processes without having to manually construct `Conduits` and
 *    emplace necessary thread-safe and/or process-safe `Duct` implementations.
 */
template<typename ImplSpec>
class Duct {

  using ducts_t = typename emp::TypePack<
    typename ImplSpec::IntraDuct,
    typename ImplSpec::ThreadDuct,
    typename ImplSpec::ProcInletDuct,
    typename ImplSpec::ProcOutletDuct
  >::make_unique;

  typename ducts_t::template apply<std::variant> impl;

  using T = typename ImplSpec::T;

  bool MaybeHoldsIntraImpl() const {
    return std::holds_alternative<typename ImplSpec::IntraDuct>( impl );
  }

  bool MaybeHoldsThreadImpl() const {
    return std::holds_alternative<typename ImplSpec::ThreadDuct>( impl );
  }

  bool MaybeHoldsProcImpl() const {
    return (
      std::holds_alternative<typename ImplSpec::ProcInletDuct>( impl )
      || std::holds_alternative<typename ImplSpec::ProcOutletDuct>( impl )
    );
  }

  bool HoldsAmbiguousImpl() const {
    return uitsl::sum(
      MaybeHoldsIntraImpl(),
      MaybeHoldsThreadImpl(),
      MaybeHoldsProcImpl()
    ) > 1;
  }

  using uid_t_ = std::uintptr_t;

  using t_registry_t = uitsl::safe::unordered_map<uid_t_, uitsl::thread_id_t>;
  inline static t_registry_t inlet_thread_registry;
  inline static t_registry_t outlet_thread_registry;

  using p_registry_t = uitsl::safe::unordered_map<uid_t_, uitsl::proc_id_t>;
  inline static p_registry_t inlet_proc_registry;
  inline static p_registry_t outlet_proc_registry;

  using edge_id_registry_t = uitsl::safe::unordered_map<uid_t_, size_t>;
  inline static edge_id_registry_t edge_id_registry;

  using node_id_registry_t = uitsl::safe::unordered_map<uid_t_, size_t>;
  inline static node_id_registry_t inlet_node_id_registry;
  inline static node_id_registry_t outlet_node_id_registry;

  using mesh_id_registry_t = uitsl::safe::unordered_map<uid_t_, size_t>;
  inline static mesh_id_registry_t mesh_id_registry;

public:

  /// TODO.
  using uid_t = uid_t_;

  /**
   * Copy constructor.
   */
  Duct(Duct& other) = delete;

  /**
   * Copy constructor.
   */
  Duct(const Duct& other) = delete;

  /**
   * Move constructor.
   */
  Duct(Duct&& other) = delete;

  /**
   * Forwarding constructor.
   *
   * Use `std::in_place_t<ImplType>` followed by constructor arguments to
   * initialize the `Duct` with `ImplType` active.
   */
  template <typename... Args>
  Duct(Args&&... args)
  : impl(std::forward<Args>(args)...)
  { ; }

  /**
   * TODO.
   *
   * @tparam WhichDuct TODO
   * @tparam Args TODO
   * @param args TODO
   */
  template <typename WhichDuct, typename... Args>
  void EmplaceImpl(Args&&... args) {
    impl.template emplace<WhichDuct>(std::forward<Args>(args)...);
  }

  /**
   * TODO.
   *
   * @param val TODO.
   * @return TODO.
   */
  bool TryPut(const T& val) {
    return std::visit(
      [&val](auto& arg) -> bool { return arg.TryPut(val); },
      impl
    );
  }

  /**
   * TODO.
   *
   * @param val TODO.
   * @return TODO.
   */
  template<typename P>
  bool TryPut(P&& val) {
    return std::visit(
      [&val](auto& arg) -> bool { return arg.TryPut(std::forward<P>(val)); },
      impl
    );
  }

  /**
   * TODO.
   *
   */
  bool TryFlush() {
    return std::visit(
      [](auto& arg) -> bool { return arg.TryFlush(); },
      impl
    );
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  const T& Get() const {
    return std::visit(
      [](auto& arg) -> const T& { return arg.Get(); },
      impl
    );
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  T& Get() {
    return std::visit(
      [](auto& arg) -> T& { return arg.Get(); },
      impl
    );
  }

  /**
   * TODO.
   *
   * @param count maximum number of gets to consume.
   * @return number of gets actually consumed.
   */
  size_t TryConsumeGets(const size_t requested) {
    return std::visit(
      [requested](auto& arg) -> size_t {
        return arg.TryConsumeGets(requested);
      },
      impl
    );
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  std::string WhichImplIsActive() const {
    return std::visit(
      [](auto& arg) -> std::string { return arg.GetName(); },
      impl
    );
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  emp::optional<bool> HoldsIntraImpl() const {
    if ( MaybeHoldsIntraImpl() ) {
      return HoldsAmbiguousImpl() ? std::nullopt : emp::optional<bool>{ true };
    } else return false;
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  emp::optional<bool> HoldsThreadImpl() const {
    if ( MaybeHoldsThreadImpl() ) {
      return HoldsAmbiguousImpl() ? std::nullopt : emp::optional<bool>{ true };
    } else return false;
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  emp::optional<bool> HoldsProcImpl() const {
    if ( MaybeHoldsProcImpl() ) {
      return HoldsAmbiguousImpl() ? std::nullopt : emp::optional<bool>{ true };
    } else return false;
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  std::string WhichImplHeld() const {
    if ( HoldsIntraImpl().value_or(false) ) return "intra";
    else if ( HoldsThreadImpl().value_or(false) ) return "thread";
    else if ( HoldsProcImpl().value_or(false) ) return "proc";
    else return "ambiguous";
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  uid_t GetUID() const { return reinterpret_cast<uid_t>(this); }

  bool CanStep() const {
    return std::visit(
      [](const auto& arg) -> bool {
        using impl_t = typename std::decay<decltype(arg)>::type;
        if constexpr ( HasMemberFunction_CanStep<impl_t, bool()>::value ) {
          return impl_t::CanStep();
        } /* else */ // removed to silence no return from non-void warning
        return false;
      },
      impl
    );
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterInletProc(const uitsl::proc_id_t proc) const {
    inlet_proc_registry[GetUID()] = proc;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterOutletProc(const uitsl::proc_id_t proc) const {
    outlet_proc_registry[GetUID()] = proc;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterInletThread(const uitsl::thread_id_t thread) const {
    inlet_thread_registry[GetUID()] = thread;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterOutletThread(const uitsl::thread_id_t thread) const {
    outlet_thread_registry[GetUID()] = thread;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterEdgeID(const size_t edge_id) const {
    edge_id_registry[GetUID()] = edge_id;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterInletNodeID(const size_t node_id) const {
    inlet_node_id_registry[GetUID()] = node_id;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterOutletNodeID(const size_t node_id) const {
    outlet_node_id_registry[GetUID()] = node_id;
  }

  /// Optional, for instrumentaiton purposes.
  void RegisterMeshID(const size_t mesh_id) const {
    mesh_id_registry[GetUID()] = mesh_id;
  }

  emp::optional<uitsl::proc_id_t> LookupInletProc() const {
    return inlet_proc_registry.contains( GetUID() )
      ? emp::optional<uitsl::proc_id_t>{ inlet_proc_registry.at( GetUID() ) }
      : std::nullopt
    ;
  }

  emp::optional<uitsl::proc_id_t> LookupOutletProc() const {
    return outlet_proc_registry.contains( GetUID() )
      ? emp::optional<uitsl::proc_id_t>{ outlet_proc_registry.at( GetUID() ) }
      : std::nullopt
    ;
  }

  emp::optional<uitsl::thread_id_t> LookupInletThread() const {
    return inlet_thread_registry.contains( GetUID() )
      ? emp::optional<uitsl::thread_id_t>{inlet_thread_registry.at( GetUID() )}
      : std::nullopt
    ;
  }

  emp::optional<uitsl::thread_id_t> LookupOutletThread() const {
    return outlet_thread_registry.contains( GetUID() )
      ? emp::optional<uitsl::thread_id_t>{outlet_thread_registry.at( GetUID() )}
      : std::nullopt
    ;
  }

  emp::optional<size_t> LookupEdgeID() const {
    return edge_id_registry.contains( GetUID() )
      ? emp::optional<size_t>{edge_id_registry.at( GetUID() )}
      : std::nullopt
    ;
  }

  emp::optional<size_t> LookupInletNodeID() const {
    return inlet_node_id_registry.contains( GetUID() )
      ? emp::optional<size_t>{inlet_node_id_registry.at(GetUID())}
      : std::nullopt
    ;
  }

  emp::optional<size_t> LookupOutletNodeID() const {
    return outlet_node_id_registry.contains( GetUID() )
      ? emp::optional<size_t>{outlet_node_id_registry.at(GetUID())}
      : std::nullopt
    ;
  }

  emp::optional<size_t> LookupMeshID() const {
    return mesh_id_registry.contains( GetUID() )
      ? emp::optional<size_t>{mesh_id_registry.at(GetUID())}
      : std::nullopt
    ;
  }

  /**
   * TODO.
   *
   * @return TODO.
   */
  std::string ToString() const {
    std::stringstream ss;
    ss << uitsl::format_member(
      "uitsl::get_proc_id()",
      uitsl::get_proc_id()
    ) << '\n';
    ss << uitsl::format_member(
      "GetUID()",
      GetUID()
    ) << '\n';
    ss << uitsl::format_member(
      "std::variant impl",
      std::visit(
        [](auto& arg) -> std::string { return arg.ToString(); },
        impl
      )
    );
    return ss.str();
  }

};

} // namespace internal
} // namespace uit

#endif // #ifndef UIT_DUCTS_DUCT_HPP_INCLUDE
