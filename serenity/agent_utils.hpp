#ifndef SERENITY_AGENT_UTILS_HPP
#define SERENITY_AGENT_UTILS_HPP

#include <mutex>  // NOLINT [build/c++11]
#include <string>

#include "curl_easy.h"  // NOLINT [build/include]

#include "glog/logging.h"

#include "rapidjson/document.h"

#include "stout/error.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

class AgentInfo {
 public:
  static Try<std::string> GetHostName() {
    std::lock_guard<std::mutex> lock(connectionMutex);
    if (hostname.isNone()) {
      Try<Nothing> result = FillAgentInfo();
      if (result.isError()) {
        return result.error();
      }
    }
    return hostname.get();
  }


  static Try<std::string> GetAgentId() {
    std::lock_guard<std::mutex> lock(connectionMutex);
    if (agentId.isNone()) {
      Try<Nothing> result = FillAgentInfo();
      if (result.isError()) {
        return result.error();
      }
    }
    return agentId.get();
  }

 protected:
  static Try<Nothing> FillAgentInfo() {
    Try<std::string> result = GetStateFromAgent();
    if (result.isError()) {
      return Error(result.error());
    } else {
      rapidjson::Document doc;
      doc.Parse(result.get().c_str());

      hostname = (doc["hostname"]).GetString();
      agentId = (doc["id"]).GetString();

      return Nothing();
    }
  }


  static Try<std::string> GetStateFromAgent(
      std::string agentUrl = "http://localhost:5051/state.json") {
    std::ostringstream responseStream;
    curl::curl_writer writer(responseStream);
    curl::curl_easy easy(writer);

    easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, agentUrl));
    easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_FOLLOWLOCATION, 1L));
    easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_HTTPGET, 1L));
    try {
      easy.perform();
    }
    catch (curl_easy_exception error) {
      LOG(ERROR) << "Error while executing GET on " << agentUrl << "\n"
                 << error.what();
      return Error("Error while executing GET on " + agentUrl + "\n"
                   + error.what());
    }

    return responseStream.str();
  }

  static std::mutex connectionMutex;

  static Option<std::string> hostname;
  static Option<std::string> agentId;
};


}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_AGENT_UTILS_HPP
