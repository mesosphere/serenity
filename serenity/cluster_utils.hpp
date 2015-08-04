#ifndef SERENITY_CLUSTER_UTILS_HPP
#define SERENITY_CLUSTER_UTILS_HPP

#include <string>

#include "curl_easy.h"  // NOLINT [build/include]

#include "glog/logging.h"

#include "rapidjson/document.h"

#include "stout/error.hpp"
#include "stout/result.hpp"

/**
 * Returns IP of host registered in Mesos  DNS hosts/
 */
inline static Result<std::string> GetIpFromMesosDns(
    std::string serviceName,
    std::string mesosDnsAddress,
    std::string mesosDnsPort) {
  std::ostringstream responseStream;
  curl::curl_writer writer(responseStream);
  curl::curl_easy easy(writer);

  std::string url = mesosDnsAddress + ":" + mesosDnsPort
                    + "/v1/hosts/" + serviceName;

  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, url));
  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_FOLLOWLOCATION, 1L));
  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_HTTPGET, 1L));
  try {
    easy.perform();
  }
  catch (curl_easy_exception error) {
    LOG(ERROR) << "Error while executing GET on " << url << "\n"
    << error.what();
    return Error("Error while executing GET on " + url + "\n"
                 + error.what());
  }

  rapidjson::Document doc;
  doc.Parse(responseStream.str().c_str());

  if (doc.IsArray() && doc.Size() > 0) {
    std::string result =(doc[0])["ip"].GetString();
    if (result.empty()) {
      return None();
    } else {
      return result;
    }
  } else {
    return None();
  }
}

#endif  // SERENITY_CLUSTER_UTILS_HPP
