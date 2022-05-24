#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include <netinet/in.h>

typedef float floatType;
typedef uint32_t sizeType;

class RemotePlotting {

  ~RemotePlotting();

  int socketHandle{};
  sockaddr_in address{};
  int connectionHandle{};

  std::thread thread{};
  static void threadFunction(RemotePlotting *self);
  std::mutex mutex{};

  std::atomic<bool> socketAndConnectionReady{};
  std::atomic<bool> running{};
  std::atomic<bool> transmitting{};

  static constexpr uint16_t port = 8080;
  static constexpr auto ip = "127.0.0.1";

  sizeType numberOfPlots{};
  std::vector<floatType> data{};

public:
  static RemotePlotting *getInstance();

  bool start();
  bool stop();

  void setNumberOfPlots(sizeType n);
  bool setPlotValue(sizeType plotIndex, floatType x, floatType y);

  bool transmit();
};
