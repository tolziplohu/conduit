#include <memory>

#define CATCH_CONFIG_DEFAULT_REPORTER "multiprocess"
#define CATCH_CONFIG_MAIN
#include "Catch/single_include/catch2/catch.hpp"

#include "uitsl/debug/MultiprocessReporter.hpp"
#include "uitsl/mpi/MpiGuard.hpp"

#include "uit/ducts/proc/outlet/get=skipping+type=trivial/t::BlockIrecvDuct.hpp"
#include "uit/setup/ImplSpec.hpp"
#include "uit/setup/InterProcAddress.hpp"

const uitsl::MpiGuard guard;

TEST_CASE("Test t::BlockIrecvDuct") {

  using ImplSpec = uit::MockSpec<char>;
  using BackEnd = uit::t::BlockIrecvDuct<ImplSpec>::BackEndImpl;

  // TODO flesh out stub test
  uit::InterProcAddress address;
  std::shared_ptr<BackEnd> backing{ std::make_shared<BackEnd>() };
  uit::t::BlockIrecvDuct<ImplSpec>{ address, backing };

}
