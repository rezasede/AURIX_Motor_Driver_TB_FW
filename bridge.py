#!/usr/bin/env python3
"""MCU binary bulk -> PlotJuggler JSON bridge, with link statistics + optional CSV log.

To add/remove a signal:
  1. Change the MCU MonSample struct, flash.
  2. Edit NAMES below to match (same order!).
  Everything else adapts automatically.
"""
import socket, struct, json, time

# ---------------- config ----------------
RX_PORT   = 9870                 # port the MCU sends to
PJ_ADDR   = ("127.0.0.1", 5000)  # PlotJuggler UDP server (json)
DECIM     = 10                   # forward every Nth sample (20kHz/10 = 2kHz live)
LOG_CSV   = False                # True -> log.csv at full rate
REPORT_S  = 1.0                  # stats interval [s]

# Signal names — MUST mirror the MCU MonSample float members, same order.
NAMES = ["ia","ib","ic","va","vb","vc","vdc","id","iq","theta","speed","AUX1"]
# -----------------------------------------

# --- everything below derives from NAMES; do not hardcode indices ---
NUM_F       = len(NAMES)
SAMPLE_FMT  = "<I" + "f" * NUM_F + "I"        # t_us | floats... | status
SAMPLE_SIZE = struct.calcsize(SAMPLE_FMT)
IDX_VALS    = slice(1, 1 + NUM_F)             # float block inside unpacked tuple
IDX_STATUS  = 1 + NUM_F
HDR_FMT     = "<IIHH"                         # magic, seq, count, sampleSize
HDR_SIZE    = struct.calcsize(HDR_FMT)
MAGIC       = 0x4D4F4E31                      # "MON1"

rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
rx.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
rx.bind(("0.0.0.0", RX_PORT))
tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

log = open("log.csv", "w") if LOG_CSV else None
if log:
    log.write("t," + ",".join(NAMES) + ",status\n")

print(f"listening UDP:{RX_PORT} -> PlotJuggler {PJ_ADDR}  decim={DECIM}  "
      f"logging={'ON' if log else 'off'}")
print(f"expecting {NUM_F} signals, sampleSize={SAMPLE_SIZE} B: {', '.join(NAMES)}")
print(f"{'time':>8} | {'pkts/s':>7} {'smpl/s':>7} {'Mbit/s':>7} | "
      f"{'smpl/pkt':>8} | {'gap avg':>8} {'gap max':>8} | {'MCU t/smpl':>10} | {'lost':>6}")

last_seq   = None
lost_total = 0
t0         = time.time()
mismatch_warned = False

iv_pkts = iv_samples = iv_bytes = 0
iv_gap_sum = iv_gap_max = 0.0
iv_spp_min, iv_spp_max = 1 << 30, 0
iv_t_us_first = iv_t_us_last = None
t_prev_pkt = None
t_report   = time.time()

n = 0
while True:
    data, addr = rx.recvfrom(4096)
    t_now = time.time()
    if len(data) < HDR_SIZE:
        continue
    magic, seq, count, ssize = struct.unpack_from(HDR_FMT, data, 0)
    if magic != MAGIC:
        continue
    if ssize != SAMPLE_SIZE:
        if not mismatch_warned:
            print(f"\n!! sampleSize mismatch: MCU sends {ssize} B/sample, "
                  f"NAMES ({NUM_F} signals) expects {SAMPLE_SIZE} B.")
            print(f"!! MCU has {(ssize - 8) // 4} float signals. "
                  f"Fix NAMES here or the MonSample struct on the MCU. "
                  f"Dropping packets until they match.")
            mismatch_warned = True
        continue
    mismatch_warned = False   # recovered (e.g. after reflash)

    # ---- loss via sequence ----
    if last_seq is not None:
        gap = (seq - last_seq) & 0xFFFFFFFF
        if gap != 1:
            lost_total += gap - 1
    last_seq = seq

    # ---- interval stats ----
    iv_pkts    += 1
    iv_samples += count
    iv_bytes   += len(data) + 42          # + UDP/IP/Eth wire overhead
    iv_spp_min  = min(iv_spp_min, count)
    iv_spp_max  = max(iv_spp_max, count)
    if t_prev_pkt is not None:
        dt = t_now - t_prev_pkt
        iv_gap_sum += dt
        iv_gap_max  = max(iv_gap_max, dt)
    t_prev_pkt = t_now

    # ---- unpack samples ----
    for i in range(count):
        v = struct.unpack_from(SAMPLE_FMT, data, HDR_SIZE + i * SAMPLE_SIZE)
        t_s    = v[0] * 1e-6
        vals   = v[IDX_VALS]
        status = v[IDX_STATUS]

        if iv_t_us_first is None:
            iv_t_us_first = v[0]
        iv_t_us_last = v[0]

        if log:
            log.write(f"{t_s:.6f}," + ",".join(f"{x:.4f}" for x in vals)
                      + f",{status}\n")

        n += 1
        if n % DECIM == 0:
            msg = {"t": t_s, **dict(zip(NAMES, vals)), "status": status}
            tx.sendto(json.dumps(msg).encode(), PJ_ADDR)

    # ---- report ----
    if t_now - t_report >= REPORT_S:
        dt_iv = t_now - t_report
        pps   = iv_pkts    / dt_iv
        sps   = iv_samples / dt_iv
        mbps  = iv_bytes * 8 / dt_iv / 1e6
        gavg  = (iv_gap_sum / max(iv_pkts - 1, 1)) * 1e3
        gmax  = iv_gap_max * 1e3
        if iv_samples > 1 and iv_t_us_last is not None:
            mcu_dt = ((iv_t_us_last - iv_t_us_first) & 0xFFFFFFFF) / (iv_samples - 1)
        else:
            mcu_dt = 0.0
        spp = (f"{iv_spp_min}-{iv_spp_max}" if iv_spp_min != iv_spp_max
               else f"{iv_spp_max}")

        print(f"{t_now - t0:8.1f} | {pps:7.0f} {sps:7.0f} {mbps:7.2f} | "
              f"{spp:>8} | {gavg:7.2f}m {gmax:7.2f}m | {mcu_dt:8.1f}us | {lost_total:6d}")

        iv_pkts = iv_samples = iv_bytes = 0
        iv_gap_sum = iv_gap_max = 0.0
        iv_spp_min, iv_spp_max = 1 << 30, 0
        iv_t_us_first = None
        t_report = t_now