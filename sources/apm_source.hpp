#ifndef SERENITY_APM_SOURCE_H
#define SERENITY_APM_SOURCE_H

#include <process/future.hpp>
#include <process/http.hpp>
#include <process/process.hpp>

#include <stout/none.hpp>
#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include "messages/serenity.pb.h"
#include "serenity/serenity.hpp"

using namespace process;

using namespace process::http;

using std::string;

namespace mesos {
namespace serenity {

class ApmSource :
    Producer<TaskPerformance>,
    public Process<ApmSource>
{
public:
  template <typename ...Any>
  ApnSource() {}

  ~ApmSource() noexcept;

};

} // namespace serenity
} // namespace mesos

#endif // SERENITY_APM_SOURCE_H
