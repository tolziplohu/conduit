#include "mpi.h"

#define CATCH_CONFIG_MAIN

#include "Catch/single_include/catch2/catch.hpp"

#include "distributed/MPIGuard.h"

const uit::MPIGuard guard;

TEST_CASE("Test MPIGuard") {

  // TODO flesh out stub test
  MPI_Barrier(MPI_COMM_WORLD);
  REQUIRE(true);

}
