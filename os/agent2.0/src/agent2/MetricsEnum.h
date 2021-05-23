#ifndef METRICS_ENUM
#define METRICS_ENUM

/* Enum for the invade operations to be selected. */
enum class OPTIONS
{
  NEW = 0x01,
  DELETE = 0x02,
  INVADE = 0x04,
  RETREAT = 0x08
};

/* Enum for the metric id. */
enum class METRICS
{
  NEW = 20,
  DELETE,
  INVADE,
  RETREAT
};

#endif