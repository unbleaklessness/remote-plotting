#include "RemotePlotting.h"

#include <cstring>

#include <arpa/inet.h>
#include <unistd.h>

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

      if (s->resettingSocketAndConnection.load()) {

        s->resettingSocketAndConnection = false;
        shutdown(s->socketHandle, SHUT_RDWR);
        close(s->socketHandle);
        s->socketAndConnectionReady = false;

      } else {

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
      }

    } else {

      s->socketHandle = socket(AF_INET, SOCK_STREAM, 0);
      if (-1 == s->socketHandle) {
        continue;
      }

      int yes = 1;
      setsockopt(s->socketHandle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int));

      s->mutex.lock();
      auto thisPort = s->port_;
      auto thisIP = s->ip_;
      s->mutex.unlock();

      memset(&s->address, 0, sizeof(address));
      s->address.sin_family = AF_INET;
      s->address.sin_addr.s_addr = inet_addr(thisIP.c_str());
      s->address.sin_port = htons(thisPort);
      inet_aton(thisIP.c_str(), &s->address.sin_addr);

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

bool RemotePlotting::isStarted() {
  return thread.joinable();
}

void RemotePlotting::setAddress(const std::string &ip, uint16_t port) {
  mutex.lock();
  this->ip_ = ip;
  this->port_ = port;
  mutex.unlock();
  resettingSocketAndConnection = true;
}

