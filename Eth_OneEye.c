/**
 * \file Eth_OneEye.c
 * \brief OneEye BB protocol over UDP/Ethernet - Implementation
 *
 * This file implements the Ifx_Oe_StdIf_DPipe interface using ring buffers,
 * allowing the Infineon SyncProtocol (BB) to communicate over UDP instead of UART.
 *
 * KEY CONCEPT:
 *   UART DPipe uses ASCLIN hardware FIFOs + ISRs for TX/RX.
 *   UDP DPipe uses software ring buffers + UDP send/receive for TX/RX.
 *   The SyncProtocol doesn't care which one — it just calls read()/write().
 *
 * TX flow: SyncProtocol write() → TX ring → OneEye_service() drains → Eth_Core2_sendUdp()
 * RX flow: UDP packet received → OneEye_rxHandler() → RX ring → SyncProtocol read()
 */

#include "Eth_OneEye.h"
#include "Eth_Core.h"
#include "ifx_oe_syncprotocol.h"
#include "ifx_oe_stdif_dpipe.h"
#include <string.h>

/*********************************************************************************************************************/
/*-------------------------------------------------Ring Buffer-------------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * Lock-free SPSC (Single-Producer Single-Consumer) ring buffer.
 *
 * TX ring: SyncProtocol is producer (write), OneEye_service is consumer (drain to UDP)
 * RX ring: OneEye_rxHandler is producer (fill from UDP), SyncProtocol is consumer (read)
 */
typedef struct
{
    volatile uint32 head;                       /**< Write position (modified only by producer) */
    volatile uint32 tail;                       /**< Read position (modified only by consumer)  */
    uint8           buf[ONEEYE_RING_SIZE];      /**< Circular data buffer                      */
} OneEye_Ring;

static uint32 ring_count(const OneEye_Ring* r)
{
    return (r->head - r->tail) & ONEEYE_RING_MASK;
}

static uint32 ring_free(const OneEye_Ring* r)
{
    return (ONEEYE_RING_SIZE - 1u) - ring_count(r);
}

static uint32 ring_write(OneEye_Ring* r, const uint8* data, uint32 len)
{
    uint32 avail = ring_free(r);
    if (len > avail) len = avail;
    if (len == 0u)   return 0u;

    uint32 pos   = r->head & ONEEYE_RING_MASK;
    uint32 toEnd = ONEEYE_RING_SIZE - pos;

    if (len <= toEnd)
    {
        memcpy(&r->buf[pos], data, len);
    }
    else
    {
        memcpy(&r->buf[pos], data, toEnd);
        memcpy(&r->buf[0], &data[toEnd], len - toEnd);
    }

    /* Ensure data is written before head is updated (memory barrier) */
    /* On TriCore, stores are strongly ordered, but __dsync() is safest for multi-core */
    __dsync();
    r->head += len;

    return len;
}

static uint32 ring_read(OneEye_Ring* r, uint8* data, uint32 len)
{
    uint32 avail = ring_count(r);
    if (len > avail) len = avail;
    if (len == 0u)   return 0u;

    uint32 pos   = r->tail & ONEEYE_RING_MASK;
    uint32 toEnd = ONEEYE_RING_SIZE - pos;

    if (len <= toEnd)
    {
        memcpy(data, &r->buf[pos], len);
    }
    else
    {
        memcpy(data, &r->buf[pos], toEnd);
        memcpy(&data[toEnd], &r->buf[0], len - toEnd);
    }

    __dsync();
    r->tail += len;

    return len;
}

static void ring_clear(OneEye_Ring* r)
{
    r->tail = r->head;
}

/*********************************************************************************************************************/
/*-------------------------------------------------Global Variables--------------------------------------------------*/
/*********************************************************************************************************************/

static OneEye_Ring  g_txRing;                              /**< TX: SyncProtocol writes here                */
static OneEye_Ring  g_rxRing;                              /**< RX: UDP received data goes here             */
static volatile boolean g_rxEvent = FALSE;                 /**< Read event flag                             */
static volatile boolean g_txEvent = FALSE;                 /**< Write event flag                            */
static uint32       g_sendCount   = 0;                     /**< Total bytes sent counter                    */

/* Diagnostic counters */
static uint32       g_diagSendOk    = 0;   /**< setSendMessageBuffer returned non-NULL   */
static uint32       g_diagSendFail  = 0;   /**< setSendMessageBuffer returned NULL        */
static boolean      g_addClientOk   = FALSE; /**< addClient result                        */

static Ifx_Oe_StdIf_DPipe          g_udpDPipe;            /**< DPipe interface for SyncProtocol            */
static Ifx_Oe_SyncProtocol         g_syncProtocol;        /**< BB SyncProtocol instance                    */
static Ifx_Oe_SyncProtocol_Client  g_motorClient;         /**< Client for motor data                       */

/*********************************************************************************************************************/
/*------------------------------------------DPipe Function Implementations-------------------------------------------*/
/*********************************************************************************************************************/
/*
 * These implement the same interface that IfxAsclin_Asc_write/read/etc. provide for UART.
 * SyncProtocol calls these through function pointers — it doesn't know or care
 * whether the underlying transport is UART, UDP, SPI, or carrier pigeon.
 */

/**
 * Write data into TX ring buffer.
 * SyncProtocol calls this to send frame headers, payloads, and ACKs.
 * The data stays in the ring until OneEye_service() drains it to UDP.
 */
static boolean udpDPipe_write(void* driver, void* data, Ifx_Oe_SizeT* count, Ifx_Oe_TickTime timeout)
{
    (void)driver; (void)timeout;

    if (*count == 0u) return TRUE;

    uint32 written = ring_write(&g_txRing, (const uint8*)data, (uint32)*count);
    g_sendCount += written;
    *count = (Ifx_Oe_SizeT)written;

    return (written > 0u);
}

/**
 * Read data from RX ring buffer.
 * SyncProtocol calls this to receive frame headers, payloads, and ACKs from OneEye.
 * Data was pushed here by OneEye_rxHandler() when a UDP packet arrived.
 */
static boolean udpDPipe_read(void* driver, void* data, Ifx_Oe_SizeT* count, Ifx_Oe_TickTime timeout)
{
    (void)driver; (void)timeout;

    uint32 got = ring_read(&g_rxRing, (uint8*)data, (uint32)*count);
    *count = (Ifx_Oe_SizeT)got;

    return (got > 0u);
}

/** How many bytes are available to read from the RX ring */
static sint32 udpDPipe_getReadCount(void* driver)
{
    (void)driver;
    return (sint32)ring_count(&g_rxRing);
}

/** How many bytes of free space in the TX ring */
static sint32 udpDPipe_getWriteCount(void* driver)
{
    (void)driver;
    return (sint32)ring_free(&g_txRing);
}

static Ifx_Oe_StdIf_DPipe_ReadEvent udpDPipe_getReadEvent(void* driver)
{
    (void)driver;
    return &g_rxEvent;
}

static Ifx_Oe_StdIf_DPipe_WriteEvent udpDPipe_getWriteEvent(void* driver)
{
    (void)driver;
    return &g_txEvent;
}

/** Check if at least 'count' bytes are available to read */
static boolean udpDPipe_canReadCount(void* driver, Ifx_Oe_SizeT count, Ifx_Oe_TickTime timeout)
{
    (void)driver; (void)timeout;
    return (ring_count(&g_rxRing) >= (uint32)count);
}

/** Check if at least 'count' bytes of free space in TX */
static boolean udpDPipe_canWriteCount(void* driver, Ifx_Oe_SizeT count, Ifx_Oe_TickTime timeout)
{
    (void)driver; (void)timeout;
    return (ring_free(&g_txRing) >= (uint32)count);
}

/** Wait until TX ring is empty (all data sent) */
static boolean udpDPipe_flushTx(void* driver, Ifx_Oe_TickTime timeout)
{
    (void)driver; (void)timeout;
    return (ring_count(&g_txRing) == 0u);
}

static void udpDPipe_clearTx(void* driver)
{
    (void)driver;
    ring_clear(&g_txRing);
}

static void udpDPipe_clearRx(void* driver)
{
    (void)driver;
    ring_clear(&g_rxRing);
}

static void udpDPipe_onReceive(void* driver)   { (void)driver; }
static void udpDPipe_onTransmit(void* driver)  { (void)driver; }
static void udpDPipe_onError(void* driver)     { (void)driver; }

static uint32 udpDPipe_getSendCount(void* driver)
{
    (void)driver;
    return g_sendCount;
}

static Ifx_Oe_TickTime udpDPipe_getTxTimeStamp(void* driver)
{
    (void)driver;
    return Ifx_Oe_Time_now();
}

static void udpDPipe_resetSendCount(void* driver)
{
    (void)driver;
    g_sendCount = 0;
}

/*********************************************************************************************************************/
/*----------------------------------------------DPipe Init-----------------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * Initialize the DPipe with all function pointers.
 * This is the equivalent of Ifx_Oe_UartDPipe_initDPipe() from the UART example.
 */
static void udpDPipe_initPipe(void)
{
    Ifx_Oe_StdIf_DPipe_initStdIf(&g_udpDPipe);

    g_udpDPipe.driver         = NULL;  /* No hardware driver object needed */
    g_udpDPipe.write          = (Ifx_Oe_StdIf_DPipe_Write)          udpDPipe_write;
    g_udpDPipe.read           = (Ifx_Oe_StdIf_DPipe_Read)           udpDPipe_read;
    g_udpDPipe.getReadCount   = (Ifx_Oe_StdIf_DPipe_GetReadCount)   udpDPipe_getReadCount;
    g_udpDPipe.getReadEvent   = (Ifx_Oe_StdIf_DPipe_GetReadEvent)   udpDPipe_getReadEvent;
    g_udpDPipe.getWriteCount  = (Ifx_Oe_StdIf_DPipe_GetWriteCount)  udpDPipe_getWriteCount;
    g_udpDPipe.getWriteEvent  = (Ifx_Oe_StdIf_DPipe_GetWriteEvent)  udpDPipe_getWriteEvent;
    g_udpDPipe.canReadCount   = (Ifx_Oe_StdIf_DPipe_CanReadCount)   udpDPipe_canReadCount;
    g_udpDPipe.canWriteCount  = (Ifx_Oe_StdIf_DPipe_CanWriteCount)  udpDPipe_canWriteCount;
    g_udpDPipe.flushTx        = (Ifx_Oe_StdIf_DPipe_FlushTx)        udpDPipe_flushTx;
    g_udpDPipe.clearTx        = (Ifx_Oe_StdIf_DPipe_ClearTx)        udpDPipe_clearTx;
    g_udpDPipe.clearRx        = (Ifx_Oe_StdIf_DPipe_ClearRx)        udpDPipe_clearRx;
    g_udpDPipe.onReceive      = (Ifx_Oe_StdIf_DPipe_OnReceive)      udpDPipe_onReceive;
    g_udpDPipe.onTransmit     = (Ifx_Oe_StdIf_DPipe_OnTransmit)     udpDPipe_onTransmit;
    g_udpDPipe.onError        = (Ifx_Oe_StdIf_DPipe_OnError)        udpDPipe_onError;
    g_udpDPipe.getSendCount   = (Ifx_Oe_StdIf_DPipe_GetSendCount)   udpDPipe_getSendCount;
    g_udpDPipe.getTxTimeStamp = (Ifx_Oe_StdIf_DPipe_GetTxTimeStamp) udpDPipe_getTxTimeStamp;
    g_udpDPipe.resetSendCount = (Ifx_Oe_StdIf_DPipe_ResetSendCount) udpDPipe_resetSendCount;
    g_udpDPipe.txDisabled     = FALSE;
}

/*********************************************************************************************************************/
/*-----------------------------------------------Public API----------------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * Internal UDP RX callback — called by Eth_Core2_processRxFrames()
 * for every received UDP packet. We filter by port and push matching
 * packets into our RX ring buffer.
 */
static void OneEye_udpRxCb(const uint8* payload, uint16 length,
                            uint16 srcPort, uint16 dstPort)
{
    /* Only accept packets destined for our OneEye port */
    if (dstPort == ONEEYE_UDP_PORT)
    {
        ring_write(&g_rxRing, payload, (uint32)length);
        g_rxEvent = TRUE;
    }
}

void OneEye_init(void)
{
    /* 1. Clear ring buffers */
    memset(&g_txRing, 0, sizeof(g_txRing));
    memset(&g_rxRing, 0, sizeof(g_rxRing));

    /* 2. Initialize DPipe (all function pointers) */
    udpDPipe_initPipe();

    /* 3. Register our UDP receive callback with the Ethernet driver.
     *    Eth_Core2_service1ms() will parse incoming frames and call us
     *    for each UDP packet received.
     */
    Eth_Core2_registerUdpRxCallback(OneEye_udpRxCb);

    /* 4. Initialize BB SyncProtocol with our UDP DPipe
     *    This is the SAME call as the UART example, just with a different pipe:
     *    Was:  Ifx_Oe_SyncProtocol_init(&g_syncProtocol, 1000, &g_OneEyeUart_uart.pipe);
     *    Now:  Ifx_Oe_SyncProtocol_init(&g_syncProtocol, 1000, &g_udpDPipe);
     */
    Ifx_Oe_SyncProtocol_init(&g_syncProtocol, 1000, &g_udpDPipe);

    /* 5. Add clients (channels) for each data type you want to send/receive */
    g_addClientOk = Ifx_Oe_SyncProtocol_addClient(&g_syncProtocol,
                                   &g_motorClient,
                                   ONEEYE_LOCAL_PORT,      /* localPort  */
                                   ONEEYE_REMOTE_PORT,     /* remotePort */
                                   sizeof(OneEye_MotorData), /* receiveMessagePayloadSize */
                                   4u);                     /* transmitBufferLength       */
}


void OneEye_service(void)
{
    /* 1. Run the BB protocol state machine.
     *    This reads from RX ring (incoming ACKs/data from OneEye)
     *    and writes to TX ring (outgoing frames to OneEye).
     */
    Ifx_Oe_SyncProtocol_execute(&g_syncProtocol);

    /* 2. Drain TX ring → send via UDP.
     *    This batches all data written by SyncProtocol_execute() into one UDP packet.
     *    Much more efficient than sending each write() as a separate packet.
     */
    uint32 txCount = ring_count(&g_txRing);

    if (txCount > 0u)
    {
        uint8 txBuf[1024];

        if (txCount > sizeof(txBuf))
        {
            txCount = sizeof(txBuf);
        }

        uint32 read = ring_read(&g_txRing, txBuf, txCount);

        if (read > 0u)
        {
            /* Send with OneEye-specific ports (not the default config ports) */
            Eth_Core2_sendUdpTo(txBuf, (uint16)read,
                                ONEEYE_UDP_PORT,   /* src port */
                                ONEEYE_UDP_PORT);  /* dst port */
        }
    }
}


void OneEye_rxHandler(const uint8* data, uint16 length)
{
    /* Push received UDP payload into the RX ring buffer.
     * SyncProtocol will read it on the next OneEye_service() → execute() call.
     *
     * This function should be called from your UDP receive callback
     * when a packet arrives on ONEEYE_UDP_PORT.
     */
    ring_write(&g_rxRing, data, (uint32)length);
    g_rxEvent = TRUE;
}


void OneEye_sendMotorData(const OneEye_MotorData* pData)
{
    Ifx_Oe_SyncProtocol_Message* pMsg =
        Ifx_Oe_SyncProtocol_setSendMessageBuffer(&g_motorClient,
                                                  ONEEYE_MSG_ID_MOTOR_XY,
                                                  sizeof(OneEye_MotorData));
    if (pMsg != NULL)
    {
        memcpy(pMsg->messagePayload, pData, sizeof(OneEye_MotorData));
        Ifx_Oe_SyncProtocol_sendMessage(pMsg);
        g_diagSendOk++;
    }
    else
    {
        g_diagSendFail++;
    }
}


void OneEye_sendPhaseData(const OneEye_PhaseData* pData)
{
    /* If you added a second client for phase data, use that client here.
     * For now, using the same client with a different message ID.
     */
    Ifx_Oe_SyncProtocol_Message* pMsg =
        Ifx_Oe_SyncProtocol_setSendMessageBuffer(&g_motorClient,
                                                  ONEEYE_MSG_ID_PHASE_CURR,
                                                  sizeof(OneEye_PhaseData));
    if (pMsg != NULL)
    {
        memcpy(pMsg->messagePayload, pData, sizeof(OneEye_PhaseData));
        Ifx_Oe_SyncProtocol_sendMessage(pMsg);
    }
}


uint32 OneEye_getTxPending(void)
{
    return ring_count(&g_txRing);
}


uint32 OneEye_getDiagWriteCount(void)
{
    return g_sendCount;
}

/** Returns: 1 = addClient succeeded, 0 = failed */
uint32 OneEye_getDiagAddClientOk(void)
{
    return (uint32)g_addClientOk;
}

/** Returns number of times setSendMessageBuffer returned non-NULL */
uint32 OneEye_getDiagSendOk(void)
{
    return g_diagSendOk;
}

/** Returns number of times setSendMessageBuffer returned NULL */
uint32 OneEye_getDiagSendFail(void)
{
    return g_diagSendFail;
}
