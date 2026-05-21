#!/usr/bin/env python3
"""
Memcached stress tester — GET over UDP, SET over TCP.
No external dependencies except numpy.

Memcached UDP frame header (8 bytes):
  [0:2]  request ID
  [2:4]  sequence number (0)
  [4:6]  total datagrams (1)
  [6:8]  reserved (0)



python3 /opt/memcached_stress.py \
  --num-keys 50000 \
  --num-ops  500000 \
  --threads  8 \
  --alpha    1.2 \
  --get-ratio 0.8 \
  --value-size 256 \
  --warm-up         # client_1 only, to pre-populate before client_2 starts
"""

import os, time, random, argparse, threading, socket, struct
import numpy as np
from dataclasses import dataclass, field

# ── Protocol ──────────────────────────────────────────────────────────────────

def mc_set_tcp(sock, key, value):
    """SET via TCP ASCII protocol."""
    payload = value.encode()
    sock.sendall(f"set {key} 0 300 {len(payload)}\r\n".encode() + payload + b"\r\n")
    sock.recv(64)  # "STORED\r\n"

def mc_get_udp(sock, key, req_id):
    """GET via UDP ASCII protocol. Returns True on hit."""
    body   = f"get {key}\r\n".encode()
    header = struct.pack(">HHHH", req_id & 0xFFFF, 0, 1, 0)
    sock.send(header + body)
    resp = sock.recv(4096)
    # Strip the 8-byte UDP header from the response
    return not resp[8:].startswith(b"END")  # True = hit

# ── Zipf ──────────────────────────────────────────────────────────────────────

def zipf_indices(n, alpha, size, seed):
    rng     = np.random.default_rng(seed)
    weights = 1.0 / np.power(np.arange(1, n + 1, dtype=np.float64), alpha)
    weights /= weights.sum()
    return rng.choice(n, size=size, p=weights)

# ── Stats ─────────────────────────────────────────────────────────────────────

@dataclass
class Stats:
    gets: int = 0; sets: int = 0; hits: int = 0; misses: int = 0; errors: int = 0
    latencies: list = field(default_factory=list)
    lock: threading.Lock = field(default_factory=threading.Lock)

    def record(self, op, hit, lat):
        with self.lock:
            self.latencies.append(lat)
            if op == "get":
                self.gets += 1; self.hits += hit; self.misses += not hit
            else:
                self.sets += 1

    def report(self, elapsed, client_id):
        total = self.gets + self.sets
        lats  = sorted(self.latencies)
        p     = lambda pct: lats[int(len(lats) * pct)] * 1000 if lats else 0
        print(f"\n{'='*50}\n  CLIENT {client_id} — RESULTS\n{'='*50}")
        print(f"  Duration    : {elapsed:.2f}s")
        print(f"  Throughput  : {total/elapsed:,.0f} ops/sec")
        print(f"  GETs (UDP)  : {self.gets:,}  (hits {self.hits:,} / misses {self.misses:,})")
        print(f"  SETs (TCP)  : {self.sets:,}")
        print(f"  Hit rate    : {self.hits/self.gets*100:.1f}%" if self.gets else "  Hit rate    : N/A")
        print(f"  Errors      : {self.errors}")
        print(f"  Latency avg : {sum(lats)/len(lats)*1000:.2f} ms" if lats else "")
        print(f"  Latency p50 : {p(0.50):.2f} ms")
        print(f"  Latency p95 : {p(0.95):.2f} ms")
        print(f"  Latency p99 : {p(0.99):.2f} ms")
        print(f"{'='*50}\n")

# ── Worker ────────────────────────────────────────────────────────────────────

def worker(host, port, indices, get_ratio, value, stats, seed):
    rng = random.Random(seed)

    # One persistent TCP socket for SETs
    tcp = socket.create_connection((host, port), timeout=2)

    # One UDP socket for GETs
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.connect((host, port))
    udp.settimeout(2)

    req_id = seed & 0xFFFF  # starting request ID for this thread

    for idx in indices:
        key = f"k:{idx}"
        op  = "get" if rng.random() < get_ratio else "set"
        try:
            t0 = time.perf_counter()
            if op == "get":
                hit = mc_get_udp(udp, key, req_id)
                req_id = (req_id + 1) & 0xFFFF
                stats.record("get", hit, time.perf_counter() - t0)
            else:
                mc_set_tcp(tcp, key, value)
                stats.record("set", True, time.perf_counter() - t0)
        except Exception:
            stats.errors += 1
            # Reconnect broken sockets
            try: tcp = socket.create_connection((host, port), timeout=2)
            except: pass

    tcp.close()
    udp.close()

# ── Warm-up (TCP only) ────────────────────────────────────────────────────────

def warm_up(host, port, num_keys, value):
    print("Warming up...")
    tcp = socket.create_connection((host, port))
    for i in range(num_keys):
        mc_set_tcp(tcp, f"k:{i}", value)
    tcp.close()
    print(f"  {num_keys:,} keys written.\n")

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    host = os.environ.get("SERVER_HOST", "10.0.1.2")
    port = int(os.environ.get("SERVER_PORT", "9010"))
    cid  = os.environ.get("CLIENT_ID", "0")

    ap = argparse.ArgumentParser()
    ap.add_argument("--host",       default=host)
    ap.add_argument("--port",       type=int, default=port)
    ap.add_argument("--num-keys",   type=int, default=10_000)
    ap.add_argument("--num-ops",    type=int, default=100_000)
    ap.add_argument("--threads",    type=int, default=4)
    ap.add_argument("--alpha",      type=float, default=1.0)
    ap.add_argument("--get-ratio",  type=float, default=0.8)
    ap.add_argument("--value-size", type=int, default=128)
    ap.add_argument("--warm-up",    action="store_true")
    args = ap.parse_args()

    seed  = int(cid) * 1000 if cid.isdigit() else 0
    value = "x" * args.value_size

    print(f"[client {cid}] → {args.host}:{args.port} | "
          f"{args.num_ops:,} ops | alpha={args.alpha} | GET={args.get_ratio:.0%} | "
          f"GET=UDP  SET=TCP")

    if args.warm_up:
        warm_up(args.host, args.port, args.num_keys, value)

    indices = zipf_indices(args.num_keys, args.alpha, args.num_ops, seed)
    chunks  = np.array_split(indices, args.threads)
    stats   = Stats()
    threads = [threading.Thread(target=worker,
                   args=(args.host, args.port, c, args.get_ratio, value, stats, seed + i))
               for i, c in enumerate(chunks)]

    t0 = time.time()
    for t in threads: t.start()
    for t in threads: t.join()
    stats.report(time.time() - t0, cid)

if __name__ == "__main__":
    main()