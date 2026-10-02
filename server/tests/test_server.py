#!/usr/bin/env python3
"""Integration test: starts minimax-server and talks MMProto over TCP."""
import os, re, signal, socket, struct, subprocess, sys, tempfile, time

MAGIC, VERSION = 0x4D4D, 1
HDR = struct.Struct(">HBBHII")  # magic, ver, flags, type, req_id, length


def frame(mtype, req, payload=b""):
    return HDR.pack(MAGIC, VERSION, 0, mtype, req, len(payload)) + payload


def tlv(tag, val):
    return struct.pack(">HI", tag, len(val)) + val


def read_frame(sock):
    def recvn(n):
        b = b""
        while len(b) < n:
            c = sock.recv(n - len(b))
            if not c:
                raise EOFError
            b += c
        return b
    magic, ver, flags, mtype, req, ln = HDR.unpack(recvn(HDR.size))
    assert magic == MAGIC and ver == VERSION
    return mtype, req, recvn(ln)


def parse_tlv(p):
    out, i = {}, 0
    while i < len(p):
        tag, ln = struct.unpack_from(">HI", p, i)
        out[tag] = p[i + 6:i + 6 + ln]
        i += 6 + ln
    return out


def main():
    server_bin, base_cfg = sys.argv[1], sys.argv[2]
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        port = s.getsockname()[1]
    text = open(base_cfg).read()
    text = re.sub(r"^port\s*=\s*\d+", f"port = {port}", text, count=1, flags=re.M)
    with tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False) as f:
        f.write(text)
        cfg = f.name
    proc = subprocess.Popen([server_bin, "-c", cfg], stderr=subprocess.DEVNULL)
    try:
        conn = None
        for _ in range(50):
            try:
                conn = socket.create_connection(("127.0.0.1", port), timeout=3)
                break
            except OSError:
                time.sleep(0.1)
        assert conn, "server did not start"

        conn.sendall(frame(3, 1, tlv(1, b"pytest")))               # HELLO
        t, req, p = read_frame(conn)
        assert (t, req) == (3, 1), (t, req)
        tags = parse_tlv(p)
        assert tags[2] == b"MINI max" and 3 in tags and 4 in tags

        conn.sendall(frame(1, 7, b"abc") + frame(1, 8, b"xyz"))     # two pipelined PINGs
        assert read_frame(conn) == (2, 7, b"abc")
        assert read_frame(conn) == (2, 8, b"xyz")

        conn.sendall(frame(999, 9))                                  # unknown -> ERROR
        t, req, p = read_frame(conn)
        assert (t, req) == (4, 9) and struct.unpack(">I", parse_tlv(p)[5])[0] == 1

        conn.sendall(b"\x00" * 14)                                   # garbage -> closed
        conn.settimeout(3)
        try:
            assert conn.recv(1) == b""
        except ConnectionResetError:   # Windows reports a closed peer as a reset
            pass
        print("server integration: OK")
    finally:
        proc.send_signal(signal.SIGTERM)  # terminate() on Windows
        proc.wait(timeout=5)
        os.unlink(cfg)


if __name__ == "__main__":
    main()
