#include "uit/conduit/ImplSpec.hpp"
#include "uit/conduit/proc/put=growing+get=stepping/inlet=CerealDequeIrsend+outlet=CerealIprobe_IcdirOciDuct.hpp"

using ImplSel = uit::ImplSelect<
  uit::SerialPendingDuct,
  uit::AtomicPendingDuct,
  uit::IcdirOciDuct
>;

#include "../ProcDuct.hpp"
