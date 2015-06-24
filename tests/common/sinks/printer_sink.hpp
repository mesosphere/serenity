#ifndef SERENITY_PRINTER_SINK_HPP
#define SERENITY_PRINTER_SINK_HPP

#include <stout/json.hpp>
#include <stout/protobuf.hpp>
#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template<typename T>
class PrinterSink : public Consumer<T>
{
public:

  PrinterSink<T>() : numberOfMessagesConsumed(0) {};

  Try<Nothing> consume(T& in)
  {
    JSON::Protobuf buffer(in);
    std::cout << "Msg #" << this->numberOfMessagesConsumed << std::endl
      << buffer << std::endl;

    this->numberOfMessagesConsumed++;
    return Nothing();
  }
  uint32_t numberOfMessagesConsumed;
};

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

#endif //SERENITY_PRINTER_SINK_HPP
