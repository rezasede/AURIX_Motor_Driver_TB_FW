/**
 * \file Monitor_Json.h
 * \brief Flexibler JSON-Serializer für AURIX TC4D (ohne malloc)
 */
#ifndef MONITOR_JSON_H_
#define MONITOR_JSON_H_

#include "Ifx_Types.h"

/* Maximale Anzahl an Float-Signalen, die du gleichzeitig senden willst */
#define MONITOR_MAX_SIGNALS    50

typedef struct
{
    const char *name;  /* Der JSON-Schlüssel, z.B. "x", "y", "v_bus" */
    float value;       /* Der Wert */
} JsonSignal;

typedef struct
{
    uint64 t;                                      /* Zeitstempel (immer dabei) */
    JsonSignal signals[MONITOR_MAX_SIGNALS];       /* Array für deine Variablen */
    uint32 numSignals;                             /* Wie viele Signale sind aktuell aktiv? */
} MonitorData;

/**
 * \brief Erstellt einen JSON-String aus den dynamischen Daten.
 * \return Anzahl der geschriebenen Bytes, oder 0 bei Fehler/Überlauf.
 */
uint32 MonitorJson_build(const MonitorData *data, char *buf, uint32 bufLen);



#endif /* MONITOR_JSON_H_ */
