"""Compile the status fixture, then verify that it detects individual regressions."""

import os
from pathlib import Path
import shlex
import subprocess

import pytest


TESTS = Path(__file__).resolve().parent
PORT = TESTS.parent / "pc_port"
STATUS = PORT / "cpu_status.c"
PASS_LINE = "CPU_STATUS_PASS synthetic_contexts=1\n"


def run_probe(status_source, tmp_path):
    executable = tmp_path / "cpu_status_probe"
    subprocess.run(
        [
            *shlex.split(os.environ.get("CC", "cc")),
            "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
            "-DMUSASHI_CPU_STATUS_TESTING", "-I", PORT / "include",
            status_source, TESTS / "cpu_status_probe.c", "-o", executable,
        ],
        check=True,
    )
    return subprocess.run([executable], capture_output=True, text=True, timeout=15)


def test_cpu_status(tmp_path):
    probe = run_probe(STATUS, tmp_path)
    assert probe.returncode == 0, probe.stderr
    assert probe.stdout == PASS_LINE


# Exercise each side of the kernel-entry gate separately so an early return
# cannot mask the fixture's check for an early SYS application.
@pytest.mark.parametrize(
    ("original", "replacement", "failed_check"),
    [
        pytest.param(
            "f->sr << 2", "f->sr << 1", "state.sr == entered",
            id="exception-entry-mode-stack",
        ),
        pytest.param(
            "f->saved_sr >> 2", "f->saved_sr >> 1", "state.sr == after",
            id="return-mode-stack",
        ),
        pytest.param(
            "token->identity == f->identity", "token->identity != NULL",
            "!musashi_cpu_status_inspect",
            id="token-identity",
        ),
        pytest.param(
            "!f->kernel_entered || f->sys_applied", "f->sys_applied",
            "!musashi_cpu_status_apply_sys",
            id="sys-before-kernel-entry",
        ),
        pytest.param(
            "!f->kernel_entered ||\n        (f->kind", "(f->kind",
            "!musashi_cpu_status_finish",
            id="return-before-kernel-entry",
        ),
        pytest.param(
            "sizeof(f->gpr)", "sizeof(f->gpr) - sizeof(f->gpr[0])",
            "memcmp(owned_copy, state.gpr",
            id="incomplete-register-snapshot",
        ),
        pytest.param(
            "c->delay_slot ? c->branch_pc : c->pc", "c->pc", "state.epc ==",
            id="delay-slot-epc",
        ),
    ],
)
def test_probe_rejects_regression(tmp_path, original, replacement, failed_check):
    source = STATUS.read_text(encoding="utf-8")
    assert source.count(original) == 1, f"Expected a single mutation site: {original}"
    mutant = tmp_path / "cpu_status.c"
    mutant.write_text(source.replace(original, replacement), encoding="utf-8")

    probe = run_probe(mutant, tmp_path)
    assert probe.returncode == 1, (probe.stdout, probe.stderr)
    assert failed_check in probe.stderr, probe.stderr
    assert PASS_LINE not in probe.stdout
