/**
 * \file Monitor_Json.c
 */
#include "Monitor_Json.h"
#include <stdio.h>

uint32 MonitorJson_build(const MonitorData *data, char *buf, uint32 bufLen)
{
    int    printed;
    uint32 len = 0u;

    /* 1. Open object, timestamp in seconds with us resolution */
    printed = snprintf(buf, bufLen, "{\"t\":%.6f", data->t);
    if ((printed < 0) || ((uint32)printed >= bufLen))
    {
        return 0u;
    }
    len += (uint32)printed;

    /* 2. Append all active signals */
    for (uint32 i = 0u; (i < data->numSignals) && (i < MONITOR_MAX_SIGNALS); i++)
    {
        printed = snprintf(buf + len, bufLen - len,
                           ",\"%s\":%.3f",
                           data->signals[i].name,
                           (float64)data->signals[i].value);
        if ((printed < 0) || ((uint32)printed >= (bufLen - len)))
        {
            return 0u;   /* overflow guard */
        }
        len += (uint32)printed;
    }

    /* 3. Close object */
    printed = snprintf(buf + len, bufLen - len, "}");
    if ((printed < 0) || ((uint32)printed >= (bufLen - len)))
    {
        return 0u;
    }
    len += (uint32)printed;

    return len;
}
