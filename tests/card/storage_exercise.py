"""Linux-only real-filesystem tests for the independently authored bridge."""
from contextlib import ExitStack
from pathlib import Path
import os
import resource
import select
import signal
import subprocess
import sys
import tempfile

SIZE = 131072
EXE = Path(sys.argv[1]).resolve()
checks = 0


def pattern(tag):
    return bytes((i * 23 + i // 113 + tag) % 256 for i in range(SIZE))


def expect(actual, wanted, label):
    global checks
    if actual != wanted:
        raise AssertionError(f"{label}: result differs")
    checks += 1


def limit_file_size():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (4096, 4096))


class Peer:
    def __init__(self, directory, limited=False):
        self.process = subprocess.Popen(
            [str(EXE), str(directory)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            text=True, bufsize=1, preexec_fn=limit_file_size if limited else None,
        )

    def run(self, command, expected):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()
        got = self.process.stdout.readline().strip()
        expect(got, expected, f"{command} returned {got!r}, expected {expected!r}")

    def stop(self, abandon=False):
        if self.process.poll() is None:
            self.process.stdin.write("abandon\n" if abandon else "end\n")
            self.process.stdin.flush()
            expect(self.process.wait(timeout=15), 0, "peer exit")
        self.process.stdin.close()
        self.process.stdout.close()


def scenarios(directory):
    slot0 = directory / "bfm_card0.mcd"
    slot1 = directory / "bfm_card1.mcd"
    with ExitStack() as cleanup:
        def new(limited=False, path=directory):
            peer = Peer(path, limited)
            cleanup.callback(peer.stop)
            return peer

        a = new()
        b = new()
        a.run("snap 0 0 0", "1 1")
        a.run("snap 0 2 0", "3 1")
        a.run("put 0 0 -1 10", "0")
        expect(slot0.exists(), False, "detached write")
        a.run("attach 0 0", "0")
        a.run("attach 0 0", "0")
        a.run("attach 1 0", "-6")
        a.run("detach 1 0", "ok")
        b.run("attach 0 0", "-6")
        a.run("snap 0 0 0", "2 1")
        a.run("put 0 0 20 10", "0")
        a.run("put 0 0 -1 10", "1")
        expect(slot0.read_bytes(), pattern(10), "created image")
        a.run("put 0 0 -1 11", "0")
        a.run("put 0 0 22 11", "0")
        a.run("snap 0 0 10", "0 1")
        a.run("put 0 0 10 11", "1")
        expect(slot0.read_bytes(), pattern(11), "CAS replacement")

        b.run("attach 0 1", "0")
        b.run("put 0 1 -1 44", "1")
        expect(slot1.read_bytes(), pattern(44), "independent second slot")
        a.run("attach 1 1", "-6")
        a.run("drop 0 0", "ok")
        b.run("attach 1 0", "0")
        b.run("snap 1 0 11", "0 1")
        a.run("snap 0 0 11", "1 1")
        a.run("put 0 0 11 33", "0")
        b.run("put 1 0 11 12", "1")
        b.stop(abandon=True)
        a.run("attach 0 0", "0")
        a.run("attach 0 1", "0")
        a.run("snap 0 0 12", "0 1")
        a.run("snap 0 1 44", "0 1")

        # A peer outside the bridge changed only the last byte of the image.
        changed = bytearray(pattern(12))
        changed[-1] ^= 1
        slot0.write_bytes(changed)
        a.run("snap 0 0 12", "0 0")
        a.run("put 0 0 12 98", "0")
        expect(slot0.read_bytes(), changed, "last-byte CAS mismatch")

        for bad in [b"", pattern(12)[:-1], pattern(12) + b"x"]:
            slot0.write_bytes(bad)
            a.run("snap 0 0 12", "3 1")
            a.run("put 0 0 -1 98", "0")
            a.run("put 0 0 12 98", "0")
            expect(slot0.read_bytes(), bad, "malformed file preservation")

        slot0.write_bytes(pattern(12))
        if os.geteuid() != 0:
            slot0.chmod(0)
            try:
                a.run("snap 0 0 12", "3 1")
                a.run("put 0 0 -1 88", "0")
            finally:
                slot0.chmod(0o600)
            expect(slot0.read_bytes(), pattern(12), "unreadable file preservation")
        a.stop()
        # Force a short filesystem write while an actual committed prior image exists.
        limited = new(limited=True)
        limited.run("attach 0 0", "0")
        limited.run("snap 0 0 12", "0 1")
        limited.run("put 0 0 12 77", "0")
        expect(slot0.read_bytes(), pattern(12), "short host write cannot replace prior image")
        limited.stop()
        virgin = directory / 'unused-session'
        virgin.mkdir()
        first_write = new(limited=True, path=virgin)
        first_write.run("attach 0 0", "0")
        first_write.run("snap 0 0 0", "2 1")
        first_write.run("put 0 0 -1 55", "0")
        expect((virgin / "bfm_card0.mcd").exists(), False, "failed initial save stays missing")
        first_write.stop()
        cold = new()
        cold.run("attach 0 0", "0")
        cold.run("snap 0 0 12", "0 1")
        cold.run("put 0 0 12 13", "1")
        cold.run("attach 0 1", "0")
        cold.run("snap 0 1 44", "0 1")
        expect(slot0.read_bytes(), pattern(13), "post-failure durable retry")
        cold.stop()


with tempfile.TemporaryDirectory(prefix='synthetic-storage-', dir='.') as tmp:
    scenarios(Path(tmp).resolve())
print(f"{checks} filesystem/process checks passed")

if len(sys.argv) > 2:
    fork_exe = Path(sys.argv[2]).resolve()
    fork_checks = 0
    for mode in ("inspect-before-exit", "first-acquire-after-exit"):
        with tempfile.TemporaryDirectory(prefix='synthetic-fork-', dir='.') as tmp:
            with subprocess.Popen(
                [str(fork_exe), str(Path(tmp).resolve()), mode],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            ) as process:
                if process.wait(timeout=15) != 0:
                    raise AssertionError("original owner failed")
                process.stdin.write(b"G")
                process.stdin.flush()
                # Parent has exited. stdout stays open until its child exits.
                parent_report = process.stdout.readline().split()
                if not select.select([process.stdout], [], [], 15)[0]:
                    raise AssertionError("surviving child timed out")
                child_report = process.stdout.readline().split()
                if parent_report[:1] != [b"parent"] or child_report[:1] != [b"child"]:
                    raise AssertionError(f"fork reports: {parent_report!r}, {child_report!r}")
                fork_checks += int(parent_report[1]) + int(child_report[1])
                expect((Path(tmp) / "bfm_card0.mcd").read_bytes(), bytes(SIZE),
                       "surviving child saved exactly one full image")
    print(f"2 fork/abandon scenarios passed, {fork_checks} parent/child assertions")
