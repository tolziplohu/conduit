#include <memory>

#define CATCH_CONFIG_DEFAULT_REPORTER "multiprocess"
#define CATCH_CONFIG_MAIN
#include "Catch/single_include/catch2/catch.hpp"

#include "uit/setup/ImplSpec.hpp"
#include "uit/setup/InterProcAddress.hpp"
#include "uit/ducts/proc/inlet/put=growing+type=trivial/t::PutDuct.hpp"
#include "uitsl/mpi/MpiGuard.hpp"
#include "uitsl/debug/MultiprocessReporter.hpp"

const uitsl::MpiGuard guard;

TEST_CASE("Test t::PutDuct") {

  using ImplSpec = uit::ImplSpec<char>;
  using BackEnd = uit::t::PutDuct<ImplSpec>::BackEndImpl;

  // TODO flesh out stub test
  uit::InterProcAddress address;
  std::shared_ptr<BackEnd> backing{ std::make_shared<BackEnd>() };
  uit::t::PutDuct<ImplSpec>{ address, backing };

}
