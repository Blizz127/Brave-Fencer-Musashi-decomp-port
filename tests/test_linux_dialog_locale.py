"""Exercise the Linux error-dialog process with the same environment SDL reads."""
import os
from pathlib import Path
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(sys.platform != "linux", reason="Linux platform adapter")
def test_utf8_dialog_preserves_game_locale_and_path(tmp_path):
    probe = tmp_path / "dialog.c"
    probe.write_text(r'''#include "platform.h"
#include <langinfo.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t musashi_native_lane_current(void) { return 0; }

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--show-error-dialog-utf8") == 0) {
        if (!musashi_platform_set_dialog_locale() || !setlocale(LC_ALL, "")) return 1;
        printf("dialog %s %s\n", nl_langinfo(CODESET), argv[2]);
        return 0;
    }
    if (argc != 2) return 2;
    int displayed = musashi_platform_show_utf8_dialog(argv[1]);
    const char *all = getenv("LC_ALL");
    printf("game %d %s\n", displayed, all ? all : "(unset)");
    return 0;
}
''')
    binary = tmp_path / "dialog"
    subprocess.run(["cc", "-std=c99", "-I", ROOT / "pc_port", "-I", ROOT / "pc_port/include",
                    probe, ROOT / "pc_port/platform/linux.c", "-ldl", "-o", binary], check=True)
    path = "Étude/房間/bfm-port/last-run.log"
    clean_env = {k: v for k, v in os.environ.items() if not k.startswith("LC_")}
    for env, message, expected in [
        ({"LANG": "C"}, path, [f"dialog UTF-8 {path}", "game 1 (unset)"]),
        ({"LANG": "C.UTF-8", "LC_ALL": "C"}, path, [f"dialog UTF-8 {path}", "game 1 C"]),
        ({"LANG": "C", "LC_CTYPE": "C.UTF-8"}, path, ["game 0 (unset)"]),
        ({"LANG": "C"}, "regular/bfm-port/last-run.log", ["game 0 (unset)"]),
    ]:
        output = subprocess.check_output([binary, message], env={**clean_env, **env}, encoding="utf-8")
        assert output.splitlines() == expected
