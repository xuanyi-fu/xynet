//
// Created by xuanyi on 4/9/20.
//

#ifndef XYNET_COUT_SINK_H
#define XYNET_COUT_SINK_H

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <iostream>
namespace xynet::log_util
{
struct cout_sink {

  void ReceiveLogMessage(g3::LogMessageMover logEntry) {
    if(logEntry.get()._level.value == WARNING.value)
      std::cout << logEntry.get().toString() << std::endl;
  }

};

struct cout_log_initilizer
{
  std::unique_ptr<g3::LogWorker> m_logworker;
  cout_log_initilizer()
  :m_logworker{g3::LogWorker::createLogWorker()}
  {
    m_logworker->addSink(std::make_unique<cout_sink>(), &cout_sink::ReceiveLogMessage);
    initializeLogging(m_logworker.get());
  }
};

}


#endif //XYNET_COUT_SINK_H
