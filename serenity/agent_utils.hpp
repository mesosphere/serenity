#ifndef SERENITY_AGENT_UTILS_HPP
#define SERENITY_AGENT_UTILS_HPP

#include <stdio.h>
#include <mutex>  // NOLINT [build/c++11]
#include <regex>  // NOLINT [build/c++11]
#include <string>

#include "curl_easy.h"  // NOLINT [build/include]

#include "glog/logging.h"

#include "rapidjson/document.h"

#include "serenity/os_utils.hpp"

#include "stout/error.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/result.hpp"
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


  static Try<std::string> GetStateFromAgent() {
    Result<std::string> agentUrl = GetLocalMesosAgentAddress();
    if (agentUrl.isNone()) {
      return Error("Could not find local mesos agent URL");
    } else if (agentUrl.isError()) {
      return agentUrl.error();
    }

    std::ostringstream responseStream;
    curl::curl_writer writer(responseStream);
    curl::curl_easy easy(writer);

    easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, agentUrl.get()));
    easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_FOLLOWLOCATION, 1L));
    easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_HTTPGET, 1L));
    try {
      easy.perform();
    }
    catch (curl_easy_exception error) {
      LOG(ERROR) << "Error while executing GET on " << agentUrl.get() << "\n"
                 << error.what();
      return Error("Error while executing GET on " + agentUrl.get() + "\n"
                   + error.what());
    }

    return responseStream.str();
  }


  /**
  * TODO(skonefal): change default process name when it changes in Mesos 0.24
  */
  static Result<std::string> GetLocalMesosAgentAddress(
      std::string agentProcessName = "mesos-slave") {
    const std::regex regex(
        "^.*?(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:\\d{2,6}).*?\\/"
        + agentProcessName, std::regex_constants::ECMAScript
                            | std::regex_constants::icase);

    constexpr uint32_t BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    FILE *fd = popen("netstat -ntldp", "r");
    if (fd == nullptr) {
      return Error("Cannot popen netstat -ntldp to get local mesos address");
    }

    while (fgets(buf, BUF_SIZE, fd) != NULL) {
      std::cmatch cmatch;
      if (std::regex_search(buf, cmatch, regex)) {
        pclose(fd);
        return cmatch.str(1);
      }
    }
    pclose(fd);
    return None();
  }

  static std::mutex connectionMutex;

  static Option<std::string> hostname;
  static Option<std::string> agentId;
};


}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_AGENT_UTILS_HPP
