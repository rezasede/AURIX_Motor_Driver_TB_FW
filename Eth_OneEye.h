/**
 * \file Eth_OneEye.h
 * \brief OneEye BB protocol over UDP/Ethernet
 *
 * Uses the Infineon OneEye SyncProtocol (BB protocol) transported over UDP
 * instead of UART. The DPipe interface is implemented using ring buffers
 * for both TX and RX paths.
 *
 * Data flow:
 *   OneEye(PC) <--UDP--> [Core2: RX/TX ring buffers] <--DPipe--> SyncProtocol(BB)
 */
#ifndef ETH_ONEEYE_H
#define ETH_ONEEYE_H

#include "Ifx_Types.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/** UDP port used by OneEye - must match your .oeprj project settings */
#define ONEEYE_UDP_PORT          65000u

/** Ring buffer size (power of 2). Must be >= SyncProtocol FIFO size */
#define ONEEYE_RING_SIZE         2048u
#define ONEEYE_RING_MASK         (ONEEYE_RING_SIZE - 1u)

/** SyncProtocol ports - must match OneEye project configuration */
#define ONEEYE_LOCAL_PORT        2u
#define ONEEYE_REMOTE_PORT       2u

/** Message IDs for each data channel
 *  MUST be > 0x0FFF — IDs 0x0000..0x0FFF are reserved by SyncProtocol core!
 */
#define ONEEYE_MSG_ID_MOTOR_XY   0x1000u
#define ONEEYE_MSG_ID_PHASE_CURR 0x1001u

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/

/** Motor XY + raw Z data sent to OneEye */
typedef struct
{
    uint32 x_raw;
    uint32 y_raw;
    uint32 z_raw;
} OneEye_MotorData;

/** Phase currents data sent to OneEye */
typedef struct
{
    sint16 raw_diff[3];
} OneEye_PhaseData;

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * \brief Initialize OneEye BB protocol over UDP (call once after Eth_Core2_init)
 *
 * Sets up the DPipe with ring buffers and initializes SyncProtocol + clients.
 */
void OneEye_init(void);

/**
 * \brief Service the OneEye protocol (call continuously in Core 2 main loop)
 *
 * This does two things:
 *   1) Calls SyncProtocol_execute() to process BB frames
 *   2) Drains TX ring buffer → sends via UDP
 *
 * Must be called frequently for responsive communication.
 */
void OneEye_service(void);

/**
 * \brief Feed received UDP data into the OneEye RX path
 *
 * Call this when a UDP packet is received on ONEEYE_UDP_PORT.
 * Pushes the raw bytes into the RX ring buffer so SyncProtocol can read them.
 *
 * \param data   Pointer to received UDP payload
 * \param length Number of bytes
 */
void OneEye_rxHandler(const uint8* data, uint16 length);

/**
 * \brief Send motor XY data to OneEye
 * \param pData Pointer to motor data struct
 */
void OneEye_sendMotorData(const OneEye_MotorData* pData);

/**
 * \brief Send phase current data to OneEye
 * \param pData Pointer to phase current data struct
 */
void OneEye_sendPhaseData(const OneEye_PhaseData* pData);

/**
 * \brief Get number of bytes pending in TX ring (for diagnostics)
 * \return Bytes waiting to be sent via UDP
 */
uint32 OneEye_getTxPending(void);

/**
 * \brief Get total bytes written by SyncProtocol to DPipe (for diagnostics)
 * \return Cumulative byte count — if this stays 0, the protocol isn't writing anything
 */
uint32 OneEye_getDiagWriteCount(void);
uint32 OneEye_getDiagAddClientOk(void);
uint32 OneEye_getDiagSendOk(void);
uint32 OneEye_getDiagSendFail(void);

#endif /* ETH_ONEEYE_H */
