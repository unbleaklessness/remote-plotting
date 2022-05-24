#include "RemotePlotting.h"

#include <cmath>

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
