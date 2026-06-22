/**
 * \file Monitor_Json.c
 */
#include "Monitor_Json.h"
#include <stdio.h>

uint32 MonitorJson_build(const MonitorData *data, char *buf, uint32 bufLen)
{
    int printed;
    uint32 currentLen = 0;

    /* 1. Start des JSON-Objekts und Zeitstempel schreiben */
    printed = snprintf(buf, bufLen, "{\"t\":%llu", (unsigned long long)data->t);

    if (printed < 0 || (uint32)printed >= bufLen) return 0u;
    currentLen += (uint32)printed;

    /* 2. Alle aktiven Signale aus dem Array hinzufügen */
    for (uint32 i = 0; i < data->numSignals; i++)
    {
        /* Wichtig: Wenn das Limit erreicht ist, stoppen */
        if (i >= MONITOR_MAX_SIGNALS) break;

        /* snprintf startet immer am aktuellen Ende des Puffers (buf + currentLen) */
        printed = snprintf(buf + currentLen, bufLen - currentLen,
                           ",\"%s\":%.3f",
                           data->signals[i].name,
                           (double)data->signals[i].value);

        if (printed < 0 || (uint32)printed >= (bufLen - currentLen))
        {
            return 0u; /* Überlaufschutz */
        }
        currentLen += (uint32)printed;
    }

    /* 3. JSON-Objekt schließen */
    printed = snprintf(buf + currentLen, bufLen - currentLen, "}");
    if (printed < 0 || (uint32)printed >= (bufLen - currentLen)) return 0u;
    currentLen += (uint32)printed;

    return currentLen;
}
