#pragma once

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <tuple>
#include <stddef.h>

#include <mpi.h>

#include "../../../../../../third-party/Empirical/source/base/assert.h"
#include "../../../../../../third-party/Empirical/source/base/vector.h"
#include "../../../../../../third-party/Empirical/source/tools/string_utils.h"

#include "../../../../distributed/mpi_utils.hpp"
#include "../../../../distributed/RdmaWindowManager.hpp"
#include "../../../../distributed/RdmaPacket.hpp"
#include "../../../../distributed/Request.hpp"
#include "../../../../utility/CircularIndex.hpp"
#include "../../../../utility/identity.hpp"
#include "../../../../utility/print_utils.hpp"
#include "../../../../utility/WarnOnce.hpp"

#include "../../../InterProcAddress.hpp"

#include "../../backend/RdmaBackEnd.hpp"

namespace uit {

/**
 * TODO
 *
 * @tparam ImplSpec class with static and typedef members specifying
 * implementation details for the conduit framework.
 */
template<typename ImplSpec>
class DequeRputDuct {

public:

  using BackEndImpl = uit::RdmaBackEnd<ImplSpec>;

private:

  using T = typename ImplSpec::T;
  constexpr inline static size_t N{ImplSpec::N};
  using packet_t = uit::RdmaPacket<T>;

  using buffer_t = std::deque<std::tuple<packet_t, uit::Request>>;
  buffer_t buffer;

  size_t epoch{};

  const uit::InterProcAddress address;

  std::shared_ptr<BackEndImpl> back_end;

  uit::Request target_offset_request;
  int target_offset;

  void PostPut() {

    // make sure that target offset has been received
    emp_assert( uit::test_completion(target_offset_request) );

    emp_assert( uit::test_null(
      std::get<uit::Request>(buffer.back())
    ) );

    // TODO FIXME what kind of lock is needed here?
    back_end->GetWindowManager().LockShared( address.GetOutletProc() );

    back_end->GetWindowManager().Rput(
      address.GetOutletProc(),
      reinterpret_cast<const std::byte*>( &std::get<packet_t>(buffer.back()) ),
      sizeof(packet_t),
      target_offset,
      &std::get<uit::Request>(buffer.back())
    );

    back_end->GetWindowManager().Unlock( address.GetOutletProc() );

    emp_assert( !uit::test_null(
      std::get<uit::Request>(buffer.back())
    ) );

  }

  bool TryFinalizePut() {
    emp_assert( !uit::test_null( std::get<uit::Request>(buffer.front()) ) );

    if (uit::test_completion( std::get<uit::Request>(buffer.front()) )) {
      emp_assert( uit::test_null( std::get<uit::Request>(buffer.front()) ) );
      buffer.pop_front();
      return true;
    } else return false;

  }

  void CancelPendingPut() {
    emp_assert( !uit::test_null( std::get<uit::Request>(buffer.front()) ) );

    UIT_Cancel( &std::get<uit::Request>(buffer.front()) );
    UIT_Request_free( &std::get<uit::Request>(buffer.front()) );

    emp_assert( uit::test_null( std::get<uit::Request>(buffer.front()) ) );

    buffer.pop_front();
  }

  void FlushFinalizedPuts() { while (buffer.size() && TryFinalizePut()); }

public:

  DequeRputDuct(
    const uit::InterProcAddress& address_,
    std::shared_ptr<BackEndImpl> back_end_
  ) : address(address_)
  , back_end(back_end_)
  {
    if (uit::get_rank(address.GetComm()) == address.GetInletProc()) {
      // make spoof call to ensure reciporical activation
      back_end->GetWindowManager().Acquire(
        address.GetOutletProc(),
        emp::vector<std::byte>{}
      );

      // we'll emp_assert later to make sure it actually completed
      UIT_Irecv(
        &target_offset, // void *buf
        1, // int count
        MPI_INT, // MPI_Datatype datatype
        address.GetOutletProc(), // int source
        address.GetTag(), // int tag
        address.GetComm(), // MPI_Comm comm
        &target_offset_request // MPI_Request *request
      );
    }

  }

  ~DequeRputDuct() {
    FlushFinalizedPuts();
    while (buffer.size()) CancelPendingPut();
  }

  /**
   * TODO.
   *
   * @param val TODO.
   */
  bool TryPut(const T& val) {
    buffer.emplace_back(
      packet_t(val, ++epoch),
      uit::Request{}
    );
    emp_assert( uit::test_null( std::get<uit::Request>(buffer.back()) ) );
    PostPut();
    return true;
  }

  [[noreturn]] size_t TryConsumeGets(size_t) const {
    throw "ConsumeGets called on DequeRputDuct";
  }

  [[noreturn]] const T& Get() const { throw "Get called on DequeRputDuct"; }

  [[noreturn]] T& Get() { throw "Get called on DequeRputDuct"; }

  static std::string GetType() { return "DequeRputDuct"; }

  std::string ToString() const {
    std::stringstream ss;
    ss << GetType() << std::endl;
    ss << format_member("this", static_cast<const void *>(this)) << std::endl;
    ss << format_member("InterProcAddress address", address) << std::endl;
    return ss.str();
  }

};

} // namespace uit
