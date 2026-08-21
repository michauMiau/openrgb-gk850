#!/usr/bin/env python3
"""GK850W USBPcap (Windows) parser.

tshark mis-parses these USBPcap captures, so we walk the pcapng blocks
ourselves. USBPcap uses EGPB (Enhanced General Packet Block, type 6), not
EPB (type 4).

EGPB layout (little-endian):
  [0:8)    block total length, type=6
  [8:12)   reserved
  [12:20)  timestamp (ns)
  [20:24)  options / reserved
  [24:28)  interface id
  [28:32)  original packet length
  [32:34)  header length
  [34)     flags  (bit0x08 = OUT host->dev, 0x10 = IN dev->host)
  ...      reserved
  [..:)    packet data

We extract HID SET_REPORT (host->device) transactions by finding the
control setup token pattern  21 09 <report_id> 03  and pairing it with the
following data payload.
"""
import struct
import sys


def parse_egpb(data):
    """Return list of dicts: {iface, flags, data} in capture order."""
    off = 0
    reports = []
    n = len(data)
    while off + 32 <= n:
        btype, total_len = struct.unpack('<II', data[off:off+8])
        if total_len < 32 or total_len > 1_000_000 or off + total_len > n:
            break
        if btype == 6:  # EGPB
            iface_id = struct.unpack('<I', data[off+24:off+28])[0]
            pkt_len = struct.unpack('<I', data[off+28:off+32])[0]
            flags = data[off+34] if off+34 < n else 0
            # packet data begins after the EGPB fixed header (>=32 bytes).
            # USBPcap pads to a 36-byte header in practice; find via len.
            start = off + total_len - pkt_len
            if start < off + 32:
                start = off + 32
            pkt_data = data[start:start+pkt_len]
            reports.append({
                'iface': iface_id,
                'flags': flags,
                'data': pkt_data,
                'idx': len(reports),
            })
        off += total_len
    return reports


def find_hid_set_reports(reports):
    """Find HID SET_REPORT transactions.

    A control OUT setup packet has first bytes: 21 09 <report_id> 03
    (bmRequestType=0x21, bRequest=0x09 SET_REPORT, wValue=rid<<8,
     wIndex=0x03). The report payload follows in the same or subsequent
    packet(s) on the same endpoint.
    """
    txns = []
    for i, r in enumerate(reports):
        d = r['data']
        if len(d) < 12:
            continue
        # USBPcap endpoint header is 4 bytes; setup token+data follow.
        for j in range(len(d)-8):
            if (d[j] == 0x21 and d[j+1] == 0x09 and d[j+3] == 0x03):
                report_id = d[j+2]
                w_index = struct.unpack('<H', d[j+4:j+6])[0]
                w_length = struct.unpack('<H', d[j+6:j+8])[0]
                # data payload: after setup (8) + endpoint header (4)
                ds = j + 8 + 4
                if ds + w_length <= len(d):
                    payload = d[ds:ds+w_length]
                    txns.append({
                        'idx': i,
                        'iface': r['iface'],
                        'report_id': report_id,
                        'w_index': w_index,
                        'w_length': w_length,
                        'data': bytes(payload),
                    })
    return txns


def main():
    if len(sys.argv) < 2:
        print("usage: gk850_pcap.py <file.pcapng> [report_id]")
        sys.exit(1)
    path = sys.argv[1]
    rid_filter = int(sys.argv[2]) if len(sys.argv) > 2 else None

    data = open(path, 'rb').read()
    reports = parse_egpb(data)
    print(f"[{path}] {len(reports)} EGPB packets")
    txns = find_hid_set_reports(reports)
    if rid_filter is not None:
        txns = [t for t in txns if t['report_id'] == rid_filter]
    print(f"{len(txns)} SET_REPORT transactions"
          + (f" (rid={rid_filter})" if rid_filter is not None else ""))
    for t in txns:
        d = t['data']
        head = d[:16].hex()
        print(f"  txn@{t['idx']} rid={t['report_id']} idx={t['w_index']}"
              f" len={len(d)} head={head}")


if __name__ == '__main__':
    main()
