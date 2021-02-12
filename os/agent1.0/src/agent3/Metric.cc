#include "os/agent3/Metric.h"
#include "lib/debug.h"

int os::agent::Metric::getPackageSize()
{
  return packageSize;
}

int os::agent::Metric::packageHelper(char *targetBuffer, int metricId, int timestamp, int payloadSize)
{
  //DBG(SUB_AGENT_TELEMETRY, "'packageHelper' now executing...\n");

  /* Flatten for network transfer */
  flatten(metricId, targetBuffer, 0);
  flatten(timestamp, targetBuffer, sizeof(int));
  flatten(payloadSize, targetBuffer, sizeof(int) * 2);

  return sizeof(int) * 3; // Bytes used for the generic package fields
}

/* TODO for Data Flattening
    Data Flattening needs to be checked concerning...
    - Big vs Small Endian
    - Integer Sizes dependend on Architecture (approach: use inttypes.h)
*/
// void os::agent::Metric::flatten(int data, char *target, int targetOffset)
// {
//     int dataSize = sizeof(data);
//     int unitSize = sizeof(char);

//     /* Example Sequence - Mapping an Int32 to series of Chars
//      * Maybe it would be also reasonable to use memcpy()
//      * instead of this 'manual' flattening.
//      *
//      * target[0] = (data) & 0xff;
//      * target[1] = (data >> 8) & 0xff;
//      * target[2] = (data >> 16) & 0xff;
//      * target[3] = (data >> 24) & 0xff;
//      *
//     */
//     for (int i = 0; i < dataSize / unitSize; i++)
//     {
//         target[i + targetOffset] = (data >> i * 8) & 0xff;
//     }
// }

uint8_t os::agent::Metric::flatten(int data, char *target, int targetOffset)
{
  if (data == 0)
  {
    target[targetOffset] = '0';
    target[targetOffset + 1] = ',';
    return 1;
  }
  int temp = data;
  int power = 1;

  while (power < temp)
    power *= 10;

  power /= 10;

  uint8_t digit = 0;
  uint8_t i = 0;

  while (power)
  {
    digit = data / power;
    target[i + targetOffset] = digit + '0';
    data %= power;
    power /= 10;
    i++;
  }

  target[targetOffset + i] = ',';

  return i;
}
