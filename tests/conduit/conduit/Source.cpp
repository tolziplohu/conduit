#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_DEFAULT_REPORTER "multiprocess"
#include "Catch/single_include/catch2/catch.hpp"

#include "uit/conduit/Source.hpp"
#include "uit/conduit/ImplSpec.hpp"
#include "uit/debug/MultiprocessReporter.hpp"
#include "uit/mpi/MpiGuard.hpp"

const uit::MpiGuard guard;

TEST_CASE("Test Source") {

  // TODO flesh out stub test
  uit::Source<uit::ImplSpec<char>> source;
  source.get<0>();

  [[maybe_unused]] auto& [outlet] = source;

}
