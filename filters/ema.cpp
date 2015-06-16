#include "filters/ema.hpp"

namespace mesos {
namespace serenity {

IpcEMAFilter::~IpcEMAFilter() {}


Try<Nothing> IpcEMAFilter::consume(ResourceUsage_Executor in)
{
  if (!in.statistics().has_perf()){
    return Error("Perf statistics are necessary for IPC EMA Filter.");
  }

  double cycles = in.statistics().perf().cycles();
  double instructions = in.statistics().perf().instructions();
  double current_ipc = instructions / cycles;

  produce(calculateEMA(current_ipc, in.statistics().perf().timestamp()));

  return Nothing();
}

} // namespace serenity
} // namespace mesos
