/**
 * \file Monitor_Json.h
 * \brief Flexible JSON serializer for AURIX TC4x (no malloc).
 *        Output: {"t":<sec>,"name":value,...}
 */
#ifndef MONITOR_JSON_H_
#define MONITOR_JSON_H_

#include "Ifx_Types.h"

#define MONITOR_MAX_SIGNALS     (32u)

typedef struct
{
    const char *name;                /* JSON key, e.g. "ia" */
    float32     value;
} JsonSignal;

typedef struct
{
    float64    t;                    /* timestamp [s] (PlotJuggler) */
    uint32     numSignals;
    JsonSignal signals[MONITOR_MAX_SIGNALS];
} MonitorData;

/** Reset the signal list. Call before adding signals each message. */
static inline void MonitorJson_reset(MonitorData *d, float64 t_seconds)
{
    d->t = t_seconds;
    d->numSignals = 0u;
}

/** Append one named signal. Ignores overflow beyond MONITOR_MAX_SIGNALS. */
static inline void MonitorJson_add(MonitorData *d, const char *name, float32 v)
{
    if (d->numSignals < MONITOR_MAX_SIGNALS)
    {
        d->signals[d->numSignals].name  = name;
        d->signals[d->numSignals].value = v;
        d->numSignals++;
    }
}

/**
 * \brief  Build the JSON string.
 * \return Bytes written, or 0 on error/overflow.
 */
uint32 MonitorJson_build(const MonitorData *data, char *buf, uint32 bufLen);

#endif /* MONITOR_JSON_H_ */
