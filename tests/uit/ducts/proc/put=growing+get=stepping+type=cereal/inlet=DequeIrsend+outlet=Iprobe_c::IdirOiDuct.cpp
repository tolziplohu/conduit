#include "uit/ducts/mock/ThrowDuct.hpp"
#include "uit/ducts/proc/put=growing+get=stepping+type=cereal/inlet=DequeIrsend+outlet=Iprobe_c::IdirOiDuct.hpp"
#include "uit/setup/ImplSpec.hpp"

using ImplSel = uit::ImplSelect<
  uit::a::SerialPendingDuct,
  uit::ThrowDuct,
  uit::c::IdirOiDuct
>;

#define IMPL_NAME "inlet=DequeIrsend+outlet=Iprobe_c::IdirOiDuct"

#include "../ProcDuct.hpp"
#include "../SteppingProcDuct.hpp"
