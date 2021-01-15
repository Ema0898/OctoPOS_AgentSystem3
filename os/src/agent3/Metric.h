#ifndef METRIC_H
#define METRIC_H

#include <stdint.h>

namespace os
{
  namespace agent
  {

    class Metric
    {
    public:
      /*
         * Pure Virtual Function!
         * Must be implemented by derived classes.
        */
      virtual char *package() = 0;
      //virtual int getMetricId() = 0;
      //virtual int getTimestamp() = 0;

    protected:
      static int getPackageSize();
      static int packageHelper(char *target, int metricId, int timestamp, int payloadSize);
      static uint8_t flatten(int data, char *target, int targetOffset);

    private:
      const static int packageSize = 512;
    };
  } // namespace agent
} // namespace os

#endif
