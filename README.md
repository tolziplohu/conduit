![conduit logo](docs/assets/logo.png)

[![version](https://img.shields.io/endpoint?url=https%3A%2F%2Fmmore500.com%2Fconduit%2Fversion-badge.json)](https://github.com/mmore500/conduit/releases)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/eeb84ac3b8a3419f9714819f9191d7bf)](https://app.codacy.com/manual/mmore500/conduit?utm_source=github.com&utm_medium=referral&utm_content=mmore500/conduit&utm_campaign=Badge_Grade_Dashboard)
[![build status](https://travis-ci.com/mmore500/conduit.svg)](https://travis-ci.com/mmore500/conduit)
[![documentation status](https://readthedocs.org/projects/uit/badge/?version=latest)](https://uit.readthedocs.io/en/latest/?badge=latest)
[![documentation coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fmmore500.com%2Fconduit%2Fdocumentation-coverage-badge.json)](https://uit.readthedocs.io/en/latest/)
[![code coverage status](https://codecov.io/gh/mmore500/conduit/branch/master/graph/badge.svg)](https://codecov.io/gh/mmore500/conduit)
[![DockerHub status](https://img.shields.io/docker/build/mmore500/conduit.svg)](https://hub.docker.com/r/mmore500/conduit)
[![Lines of Code](https://tokei.rs/b1/github/mmore500/conduit?category=code)](https://github.com/XAMPPRocky/tokei)
[![Comments](https://tokei.rs/b1/github/mmore500/conduit?category=comments)](https://github.com/XAMPPRocky/tokei)
[![dotos](https://img.shields.io/endpoint?url=https%3A%2F%2Fmmore500.com%2Fconduit%2Fdoto-badge.json)](https://github.com/mmore500/conduit/search?q=todo+OR+fixme&type=)

* Free software: MIT license
* Documentation: [https://conduit.fyi](https://conduit.fyi)
* header-only, namespace-encapsulated software

C++ library that wraps intra-thread, inter-thread, and inter-process communication in a uniform, modular, object-oriented interface, with a focus on asynchronous high-performance computing applications.

## Design

![Inlet and Outlet holding a shared Duct with intra-thread implementation active](docs/assets/default.png)

The conduit model consists of:
* `Inlet`'s, which accepts inputs `T` through a non-blocking call,
* `Duct`'s, which handle transmission logistics,
* `Outlet`'s, which provides the latest `T` or next `T` through a non-blocking call.

`Inlet` and `Outlet` objects both hold a [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr) to a `Duct` object.
The `Duct` object is implemented as a [`std::variant`](https://en.cppreference.com/w/cpp/utility/variant) of three implementation types:
* `IntraDuct` type for intra-thread communication (default),
* `ThreadDuct` type one for inter-thread communication, and
* `ProcDuct` type for inter-process communication.

The `Duct`'s active implementation can be switched at run-time by calling `EmplaceDuct<Type>` from either the `Inlet` or the `Outlet`.
All calls to a `Duct` at run-time are forwarded to its active implementation.
For example, emplacing a `ThreadDuct` might yield the following.

![Inlet and Outlet holding a shared Duct with inter-thread implementation active](docs/assets/emplace.png)

Calling `SplitDuct<Type>` from either the `Inlet` or the `Outlet` will drop the callee's `std::shared_ptr` to the existing `Duct` in favor of a `std::shared_ptr` to a newly-constructed `Duct` with the specified implementation type active.
(This operation is useful for inter-process communication, where coupled `Inlet` and `Outlet`'s do not reside in a common memory space).
For example, calling `SplitDuct<ProcDuct>` on an `Inlet` might yield the following.

![Inlet and Outlet holding separate Ducts](docs/assets/split.png)

`Inlet` and `Outlet` are entirely interchangeable no matter the current `Duct` implementation is active.
Once a `Duct`'s are configured, `Inlet` and `Outlet` objects can be without any concern for underlying implementation.
This abstraction ensures a uniform API whether underlying communication is intra-thread, inter-thread, or inter-process.
Furthermore, a `Duct` implementation can be re-configured or even re-directed at run time without any interaction with an `Inlet` or `Outlet` its tied to.

## Low-Level Interface

Conduit provides three helper construction interfaces:
* `Conduit`, which constructs an `Inlet` and `Outlet` with a shared `Duct`,
* `Sink`, which constructs an `Inlet` with sole holdership of a `Duct`,
* `Source`, which constructs an `Outlet` with sole holdership of a `Duct`.

![comparison of Conduit, Sink, and Source](docs/assets/conduit-sink-source.png)

After constructing a `Conduit`, `Sink`, or `Source`, users can use [structured binding](https://en.cppreference.com/w/cpp/language/structured_binding) or an accessor method to retrieve `Inlet`'s or `Outlet`'s.

Here's an example of how this works in code.

`conduit/low.cpp`:
```cpp
#include <iostream>
#include <ratio>
#include <utility>

#include "conduit/Conduit.hpp"
#include "conduit/ImplSpec.hpp"

// use int as message type
using Spec = uit::ImplSpec<int>;

int main() {

  // construct conduit with thread-safe implementation active
  uit::Conduit<Spec> conduit{
    std::in_place,
    Spec::ThreadDuct
  };

  auto& [inlet, outlet] = conduit;


  uit::ThreadTeam team;

  // start a producer thread
  team.Add(
    [&inlet](){
      for (int i = 0; i < std::mega{}.num; ++i) inlet.MaybePut(i);
    })
  );
  // start a consumer thread
  team.Add(
    [&outlet](){
      int prev{ outlet.GetCurrent() };
      size_t update_counter{};
      for (size_t i = 0; i < std::mega{}.num; ++i) {
        update_counter += std::exchange(prev, outlet.GetCurrent()) == prev);
      }
      std::cout << update_counter << " updates detected" << std::endl
    })
  );

  // wait for threads to complete
  team.Join();

  return 0;
}
```

Navigate to the `conduit` directory.
Then, to compile and run,
```sh
mpic++ --std=c++17 -O3 -DNDEBUG -Iinclude/ low.cpp -lpthread
./a.out
```

:bangbang:
You'll need an MPI compiler and runtime library for the code examples here.
If you don't have those on hand, grab a copy of our pre-built Docker container and hop inside there.
```sh
sudo docker run -it mmore500/conduit:latest
```

If you're on a cluster without root access, you can try using Singularity.
```sh
singularity shell docker://mmore500/conduit
```

## High-Level Interface

The conduit library provides a `Mesh` interface to streamline construction of complex, potentially irregular, conduit networks.
These networks are conceived as a directed graph, with edges representing conduits and nodes representing an actor that holds a set of `Inlet`'s and/or `Outlet`'s.

`Mesh`es are constructed through two independently-specified components,
1. topology: how should nodes be connected?
2. delegation: how should nodes be assigned to threads and processes?

Here's an example topology, with each node connected to a successor in a one-dimensional ring.

![unidirectional ring graph](docs/assets/ring-topology.png)

We might choose to delegate contiguous subsets of nodes to threads and processes.
For example, to distribute 24 nodes over four double-threaded processes, we might perform the following assignment:
* node 0 :arrow_right: thread 0, process 0;
* node 1 :arrow_right: thread 0, process 0;
* node 2 :arrow_right: thread 0, process 0;
* node 3 :arrow_right: thread 1, process 0;
* node 4 :arrow_right: thread 1, process 0;
* node 5 :arrow_right: thread 1, process 0;
* node 6 :arrow_right: thread 0, process 1;
* node 7 :arrow_right: thread 0, process 1;
* node 8 :arrow_right: thread 0, process 1;
* node 9 :arrow_right: thread 1, process 1;
* etc.

![graph nodes assigned to threads and processes](docs/assets/ring-mesh.png)

Arbitrary topologies can be specified, with pre-built factories available to construct the most common configurations.
For example, a two-dimensional lattice grid,

![grid lattice graph](docs/assets/lattice-topology.png)

We can use a specialized delegation function to distribute nodes.

![graph nodes assigned to threads and processes](docs/assets/lattice-mesh.png)

When a `Mesh` is constructed from a topology and a delegation function, edges between nodes are instantiated in terms of `Inlet`'s and `Outlet`'s.
During `Mesh` construction, thread-safe `Duct` implementations are emplaced on conduits that span between nodes assigned to different threads and inter-process `Duct` implementations are emplaced on conduits that span between nodes assigned to different proceses.

Once the `Mesh` is constructed, `GetSubmesh()` returns the network components that are assigned to a particular thread or process.

![nodes within a particular thread on a particular process](docs/assets/submesh.png)

The `GetSubmesh()` call returns an `emp::vector` of `MeshNode`'s.
Each `MeshNode` consists of an "input" vector of `Outlet`'s and an "output" vector of `Inlet`'s.

![mesh node with constituent input outlets and output inlets](docs/assets/mesh-node.png)

Here's what the entire process looks like in code.

`conduit/high.cpp`:
```cpp
#include <iostream>
#include <tuple>
#include <sstream>

#include "conduit/ImplSpec.hpp"
#include "distributed/MPIGuard.hpp"
#include "mesh/Mesh.hpp"
#include "parallel/ThreadTeam.hpp"
#include "topology/RingTopologyFactory.hpp"

const uit::MPIGuard guard; // MPI initialization & finalization boilerplate

struct Message {

  size_t node_id;
  uit::thread_id_t thread_id;
  uit::proc_id_t proc_id;

  std::string ToString() const {

    std::stringstream ss;
    ss << "p" << uit::get_proc_id();
    ss << " / ";
    ss << "t" << thread_id;
    ss << " / ";
    ss << "n" << node_id;

    return ss.str();

  }

};

// transmit Message through conduits with default implementations
using Spec = uit::ImplSpec<Message>;

const size_t num_nodes = 5; // five nodes in our topology
const size_t num_procs = 2; // four MPI processes
const size_t num_threads = 2; // two threads per process

void thread_job(const uit::thread_id_t thread_id, uit::Mesh<Spec>& mesh) {

  auto my_nodes = mesh.GetSubmesh(thread_id);

  for (size_t node_id = 0; node_id < my_nodes.size(); ++node_id) {

    const Message my_message{ node_id, thread_id, uit::get_proc_id() };

    auto& node = my_nodes[node_id];

    for (auto& output : node.GetOutputs()) output.Put( my_message );

    for (auto& input : node.GetInputs()) {
      std::cout
        << input.GetNext().ToString()
        << "  =>  "
        << my_message.ToString()
        << std::endl;
    }

  }

}

int main() {

  uit::Mesh<Spec> mesh{
    uit::RingTopologyFactory{}(num_nodes),
    uit::AssignRoundRobin<uit::thread_id_t>{num_threads},
    uit::AssignContiguously<uit::proc_id_t>{num_procs, num_nodes}
  };

  uit::ThreadTeam team;
  for (uit::thread_id_t tid = 0; tid < num_threads; ++tid) {
    team.Add([tid, &mesh](){ thread_job(tid, mesh); });
  }

  team.Join();

  return 0;

}
```

Now compile and run.

```sh
mpic++ --std=c++17 -O3 -DNDEBUG -Iinclude/ high.cpp -lpthread
mpiexec -n 2 ./a.out
```

## Extensibility

* Create your own topologies by writing a topology-generating method or loading a topology from file, perhaps generated via another tool like [NetworkX](https://networkx.github.io/documentation/stable/reference/readwrite/adjlist.html).
* Define your own delegation functor to control how nodes are distributed between threads and processes.
* Seamlessly write and build with your own intra-thread, inter-thread, or intra-thread duct implementations.

Implementations of inter-process communication currently use the [Messgage Passing Interface (MPI)](https://www.mpi-forum.org/docs/) standard.

## Benchmarks

Benchmarks are performed during Travis builds and also, occasionally, on the iCER HPCC cluster.
Benchmark results are available at [https://osf.io/7jkgp/](https://osf.io/7jkgp/).

## Acknowledgement

This project is built using the [Empirical C++ library](https://github.com/devosoft/Empirical/).

This project is developed in support of the [DISHTINY](https://mmore500.com/dishtiny) digital multicellularity evolution project.

This package was created with [Cookiecutter](https://github.com/audreyr/cookiecutter) and the [devosoft/cookiecutter-empirical-project](https://github.com/devosoft/cookiecutter-empirical-project) project template.

This research was supported in part by NSF grants DEB-1655715 and DBI-0939454.
This material is based upon work supported by the National Science Foundation Graduate Research Fellowship under Grant No. DGE-1424871.
Any opinions, findings, and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the National Science Foundation.

Michigan State University provided computational resources used in this work through the Institute for Cyber-Enabled Research.

This research is conducted in affiliation with the [Digital Evolution Laboratory](https://devolab.org/) at Michigan State University, the [BEACON Center for the Study of Evolution in Action](https://www3.beacon-center.org/), the [Ecology, Evolutionary Biology, and Behavior](https://eebb.natsci.msu.edu/) Program at MSU, and the [Department of Computer Science and Engineering](http://cse.msu.edu/) at MSU.
