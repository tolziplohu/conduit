#include "Catch/single_include/catch2/catch.hpp"

#include "uit/setup/ImplSpec.hpp"

#include "netuit/arrange/ProConTopologyFactory.hpp"
#include "netuit/arrange/RingTopologyFactory.hpp"
#include "netuit/mesh/Mesh.hpp"

TEST_CASE("Test Mesh", "[nproc:1]") {

  // TODO flesh out stub test
  using Spec = uit::ImplSpec<char>;

  netuit::Mesh<Spec> mesh{netuit::RingTopologyFactory{}(100)};

  REQUIRE( mesh.GetNodeCount() == 100 );
  REQUIRE( mesh.GetEdgeCount() == 100 );
  REQUIRE( mesh.GetSubmesh().size() == 100 );

}

TEST_CASE("Test with ProConTopologyFactory", "[nproc:1]") {

  using Spec = uit::ImplSpec<char>;

  netuit::Mesh<Spec> mesh{netuit::ProConTopologyFactory{}(100)};

  REQUIRE( mesh.GetNodeCount() == 100 );
  REQUIRE( mesh.GetEdgeCount() == 50 );
  REQUIRE( mesh.GetSubmesh().size() == 100 );

}

// TODO add tests with more TopologyFactories
// TODO add tests with no-connection nodes
