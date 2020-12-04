#include "Catch/single_include/catch2/catch.hpp"

#include "uitsl/concurrent/ConcurrentTimeoutBarrier.hpp"
#if 0
#endif
#include "uitsl/parallel/ThreadIbarrierFactory.hpp"
#include "uitsl/parallel/ThreadTeam.hpp"


constexpr size_t num_threads{ 2 };

inline void do_work_concurrent() {

  static uitsl::ThreadIbarrierFactory factory{ num_threads };

  const uitsl::ConcurrentTimeoutBarrier barrier{ factory.MakeBarrier() };

}


TEST_CASE("Test ConcurrentTimeoutBarrier") {

  uitsl::ThreadTeam team;

  team.Add(do_work_concurrent);
  team.Add(do_work_concurrent);

  team.Join();

}
