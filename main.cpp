#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <cmath>
#include <mutex>
#include <atomic>

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

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

RemotePlotting *RemotePlotting::getInstance() {
  static RemotePlotting instance;
  return &instance;
}

bool RemotePlotting::start() {

  if (thread.joinable()) {
    return false;
  }

  running = true;
  thread = std::thread(&RemotePlotting::threadFunction, this);

  return true;
}

bool RemotePlotting::stop() {

  if (!thread.joinable()) {
    return false;
  }

  running = false;
  thread.join();

  return true;
}

bool RemotePlotting::transmit() {

  if (!running.load()) {
    return false;
  }

  transmitting = true;

  return true;
}

bool RemotePlotting::setPlotValue(sizeType plotIndex, floatType x, floatType y) {
  mutex.lock();
  if (plotIndex >= numberOfPlots) {
    return false;
  }
  data[plotIndex * 2] = x;
  data[plotIndex * 2 + 1] = y;
  mutex.unlock();
  return true;
}

void RemotePlotting::setNumberOfPlots(sizeType n) {
  mutex.lock();
  numberOfPlots = n;
  data.resize(numberOfPlots * 2);
  mutex.unlock();
}

RemotePlotting::~RemotePlotting() {
  stop();
}

void RemotePlotting::threadFunction(RemotePlotting *s) {

  while (s->running.load()) {

    if (s->socketAndConnectionReady.load()) {

      if (s->transmitting.load()) {

        s->mutex.lock();
        auto thisData = s->data;
        auto thisDataSize = s->data.size();
        s->mutex.unlock();

        auto returnValue = send(s->connectionHandle, thisData.data(), thisDataSize * sizeof(floatType), MSG_NOSIGNAL);

        if (-1 == returnValue) {
          shutdown(s->socketHandle, SHUT_RDWR);
          close(s->socketHandle);
          s->socketAndConnectionReady = false;
        }

        s->transmitting = false;
      }

    } else {

      s->socketHandle = socket(AF_INET, SOCK_STREAM, 0);
      if (-1 == s->socketHandle) {
        continue;
      }

      int yes = 1;
      setsockopt(s->socketHandle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int));

      memset(&s->address, 0, sizeof(address));
      s->address.sin_family = AF_INET;
      s->address.sin_addr.s_addr = inet_addr(ip);
      s->address.sin_port = htons(port);
      inet_aton(ip, &s->address.sin_addr);

      if (-1 == bind(s->socketHandle, (struct sockaddr*) &s->address, sizeof(address))) {
        shutdown(s->socketHandle, SHUT_RDWR);
        close(s->socketHandle);
        continue;
      }

      if (-1 == listen(s->socketHandle, 1)) {
        shutdown(s->socketHandle, SHUT_RDWR);
        close(s->socketHandle);
        continue;
      }

      s->connectionHandle = accept(s->socketHandle, nullptr, nullptr);
      if (-1 == s->connectionHandle) {
        shutdown(s->socketHandle, SHUT_RDWR);
        close(s->socketHandle);
        continue;
      }

      s->mutex.lock();
      auto thisNPlots = s->numberOfPlots;
      s->mutex.unlock();

      auto returnValue = send(s->connectionHandle, &thisNPlots, sizeof(sizeType), MSG_NOSIGNAL);

      if (-1 == returnValue) {
        shutdown(s->socketHandle, SHUT_RDWR);
        close(s->socketHandle);
        continue;
      }

      s->socketAndConnectionReady = true;
    }
  }

  if (s->socketAndConnectionReady) {
    shutdown(s->socketHandle, SHUT_RDWR);
    close(s->socketHandle);
    s->socketAndConnectionReady = false;
  }
}

int main() {

  auto plotting = RemotePlotting::getInstance();
  plotting->setNumberOfPlots(3);

  float time = 0;
  float timeStep = 0.001;

  while (true) {

    plotting->start();

    while (true) {

      plotting->setPlotValue(0, time, std::sin(M_PI * 2.0 * time * 2.0));
      plotting->setPlotValue(1, time, std::sin(M_PI * 2.0 * time * 0.5));
      plotting->setPlotValue(2, time, std::sin(M_PI * 2.0 * time * 4.0));
      time += timeStep;

      plotting->transmit();

      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(timeStep * 1000)));
    }
  }

  return 0;
}