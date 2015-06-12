#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

//#include <mesos/module/resource_estimator.hpp>

#include <mesos/slave/resource_estimator.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/subprocess.hpp>

#include <stout/try.hpp>
#include <stout/stringify.hpp>
#include <stout/hashmap.hpp>
#include <stout/option.hpp>

using namespace mesos;

using mesos::slave::ResourceEstimator;

class SerenityEstimator : public ResourceEstimator
{
public:
  static Try<ResourceEstimator*> create(const Parameters& parameters){
    return new SerenityEstimator();
  }

  virtual Try<Nothing> initialize() {
    return Nothing();
  }

  virtual process::Future<Resources> oversubscribable(){
    return mesos::Resources();
  }


};


// skonefal TODO: waint until the Resource Estimator module lands on mesos master

//static ResourceEstimator* createSerenityEstimator(const Parameters& parameters)
//{
//  LOG(INFO) << "Loading Serenity Estimator module";
//  Try<ResourceEstimator*> result = SerenityEstimator::create(parameters);
//  if (result.isError()) {
//    return NULL;
//  }
//  return result.get();
//}
//
//
//mesos::modules::Module<ResourceEstimator> com_mesosphere_mesos_SerenityEstimator(
//    MESOS_MODULE_API_VERSION,
//    MESOS_VERSION,
//    "Mesosphere",
//    "support@mesosphere.com",
//    "Serenity Estimator",
//    NULL,
//    createSerenityEstimator);
