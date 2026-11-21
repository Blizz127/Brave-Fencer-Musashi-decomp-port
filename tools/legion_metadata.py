"""Stage the Ubuntu Linux player's dependencies and describe their actual inputs."""

import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path


SONAMES = ("libSDL2-2.0.so.0", "libopenal.so.1", "libcrypto.so.3", "libz.so.1")
COMMON_LICENSES = ("Apache-2.0", "Artistic", "GPL-1", "GPL-2", "GPL-3", "LGPL-2", "LGPL-2.1")


def output(*args):
    return subprocess.check_output(args, text=True).strip()


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_identity(root, player, shim, config, receipt):
    if receipt is not None:
        # Accept only these fields from the playtest record. It may contain local paths.
        if not isinstance(receipt, dict):
            raise ValueError("candidate receipt must be a JSON object")
        commit, branch = receipt["source_commit"], receipt["source_branch"]
        if not re.fullmatch(r"[0-9a-f]{40}", commit):
            raise ValueError("candidate receipt requires a full Git source commit")
        if branch is not None:
            output("git", "check-ref-format", "refs/heads/" + branch)
        if not config:
            raise ValueError("cannot verify candidate build type without its CMake configuration")
        if receipt["build_type"] != config:
            raise ValueError("candidate build type differs from CMake configuration")
        if sha256(player) != receipt["player_sha256"] or sha256(shim) != receipt["helper_sha256"]:
            raise ValueError("staged player or shim differs from candidate receipt")
        output("git", "-C", root, "cat-file", "-e", commit + "^{commit}")
        return {"source_commit": commit, "source_branch": branch, "source_identity_basis": "tested candidate receipt"}
    # A checkout can advance after linking. Its HEAD is a label for development
    # archives, never proof of the supplied binary's source revision.
    commit = output("git", "-C", root, "rev-parse", "--verify", "HEAD")
    return {"source_commit": None, "source_branch": None,
            "source_identity_basis": "no tested candidate receipt", "packaging_checkout_commit": commit}


def build_config(binary):
    cache = binary.parent / "CMakeCache.txt"
    single_directory = cache.is_file()
    if not single_directory:
        cache = binary.parent.parent / "CMakeCache.txt"  # Multi-config generator.
        if not cache.is_file():
            return None, None
    values = dict(re.findall(r"^(\w+):\w+=(.*)$", cache.read_text(), re.MULTILINE))
    configs = values.get("CMAKE_CONFIGURATION_TYPES", "").split(";")
    if binary.parent.name in configs:
        build_type = binary.parent.name
    elif single_directory:
        build_type = values.get("CMAKE_BUILD_TYPE") or None
    else:
        return None, None  # The ancestor cache belongs to another build.
    if build_type is not None and not re.fullmatch(r"[\w.+-]+", build_type):
        raise ValueError("invalid CMake build configuration")
    lane = values.get("MUSASHI_NATIVE_LANE")
    if lane is not None:
        lane = lane.upper() in {"ON", "TRUE", "YES", "Y", "1"}
    return build_type, lane


def owner(path):
    # Legacy distro packages may register either /lib or /usr/lib.
    if shutil.which("dpkg-query"):
        candidates = [path, path.parent.resolve() / path.name]
        if str(path).startswith("/usr/lib/"):
            older = Path("/") / path.relative_to("/usr")
            if older.is_file() and older.samefile(path):
                candidates.append(older)
        for candidate in dict.fromkeys(candidates):
            result = subprocess.run(["dpkg-query", "-S", candidate], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.split(": ", 1)[0]
    return None


def dependencies(binary, env=None):
    report = subprocess.check_output(["ldd", binary], text=True, env=env)
    if "=> not found" in report:
        raise ValueError(f"unresolved runtime dependency for {binary.name}")
    return dict(re.findall(r"^\s*(\S+) => (.+?) \(0x[0-9a-fA-F]+\)$", report, re.MULTILINE))


def host_requirements(destination, members):
    """Read the direct host SONAMEs and imported symbol versions of each staged ELF."""
    direct = {}
    maxima = {}
    provided = {Path(member).name for member in members}
    env = dict(os.environ, LC_ALL="C")
    for member in members:
        elf = destination / member
        dynamic = subprocess.check_output(["readelf", "-W", "-d", elf], env=env, text=True)
        needed = set(re.findall(r"\(NEEDED\).*?\[([^]]+)\]", dynamic))
        direct[member] = sorted(needed - provided)
        versions = subprocess.check_output(["readelf", "-W", "--version-info", elf], env=env, text=True)
        # Definitions name versions exported by this ELF. Only the needs table
        # describes required versions supplied by another shared library.
        imports = versions.partition("Version needs section")[2]
        for family, version in re.findall(r"Name: (GLIBCXX|GLIBC|CXXABI|GCC)_([0-9.]+)", imports):
            numbers = tuple(map(int, version.split(".")))
            if numbers > maxima.get(family, ()):
                maxima[family] = numbers
    return {"direct_host_sonames": direct,
            "symbol_versions": {k: ".".join(map(str, v)) for k, v in sorted(maxima.items())}}


def stage(root, binary, destination, receipt):
    build_type, native_lane = build_config(binary)
    adjacent = binary.with_name(binary.name + ".receipt.json")
    supplied = Path(receipt) if receipt else adjacent
    receipt = json.loads(supplied.read_text()) if receipt or supplied.is_file() else None
    player = destination / "musashi_native_boot"
    shim_name = "libmusashi_x11_nograb.so"
    shim = destination / "lib" / shim_name
    shutil.copyfile(binary, player)
    player.chmod(0o755)
    shutil.copyfile(binary.parent / shim_name, shim)
    identity = source_identity(root, player, shim, build_type, receipt)

    revision_for_notices = identity["source_commit"] or identity["packaging_checkout_commit"]

    def public_file(path):
        return subprocess.check_output(["git", "-C", root, "show", f"{revision_for_notices}:{path}"])

    notices = destination / "licenses"
    for filename in ("LICENSE", "NOTICE.md", "CREDITS.md", "LICENSE-NOTES.md"):
        (notices / filename).write_bytes(public_file(filename))
    # A user may stage copied build artifacts without the dependency checkout.
    shutil.copyfile(root / "tools/licenses/PsyCross-LICENSE", notices / "PsyCross-LICENSE")
    fetch = public_file("tools/fetch_toolchains.sh").decode()
    pin = re.search(r'^PSYCROSS_COMMIT="([0-9a-f]{40})"$', fetch, re.MULTILINE)
    if pin is None:
        raise ValueError("missing pinned PsyCross revision")
    revision = pin.group(1)
    patch_path = "tools/patches/psycross-local.patch"
    patch_sha = hashlib.sha256(public_file(patch_path)).hexdigest()
    verified_psycross = bool(receipt and "psycross_revision" in receipt)
    if verified_psycross and (receipt["psycross_revision"] != revision
                              or receipt.get("psycross_patch_sha256") != patch_sha):
        raise ValueError("candidate PsyCross revision or public patch differs from the configured input")
    missing_notices = []
    for name in COMMON_LICENSES:
        text = Path("/usr/share/common-licenses") / name
        if text.is_file():
            shutil.copyfile(text, notices / (name + ".txt"))
        else:
            missing_notices.append(name + ".txt")

    linked = dependencies(binary)
    libs = {}
    for soname in SONAMES:
        link = Path(linked[soname])
        actual = link.resolve(strict=True)
        filename = "lib/" + soname
        shutil.copyfile(actual, destination / filename)
        lib = {"path": filename, "real_file": actual.name, "binary_package": None,
               "source_package": None, "version": None, "source_version": None,
               "arch": None, "license_notice": None}
        libs[soname] = lib
        registered = owner(actual)
        if not registered:
            missing_notices.append(soname)
            continue
        pkg, src_pkg, version, src_version, arch = output(
            "dpkg-query", "-W", "-f=${Package}\t${source:Package}\t${Version}\t${source:Version}\t${Architecture}",
            registered).split("\t")
        lib.update(binary_package=pkg, source_package=src_pkg, version=version,
                   source_version=src_version, arch=arch)
        for provider in {pkg, (owner(link) or registered).split(":")[0]}:
            notice = Path("/usr/share/doc") / provider / "copyright"
            if notice.is_file():
                shutil.copyfile(notice, notices / (provider + "-copyright"))
                if provider == pkg:
                    lib["license_notice"] = "licenses/" + pkg + "-copyright"
            else:
                missing_notices.append(provider + "-copyright")
    if missing_notices and receipt is not None:
        raise ValueError("missing registered runtime providers or notices: " + ", ".join(missing_notices))

    env = dict(os.environ, LD_LIBRARY_PATH=str(destination / "lib"), LC_ALL="C")
    staged = dependencies(player, env)
    dependencies(shim, env)
    for soname in (*SONAMES, shim_name):
        if Path(staged[soname]).resolve() != (destination / "lib" / soname).resolve():
            raise ValueError(f"staged player does not resolve bundled {soname}")

    requirements = {"direct_host_sonames": {}, "symbol_versions": {}}
    if shutil.which("readelf") or receipt is not None:
        members = [player.name, "lib/" + shim_name, *(entry["path"] for entry in libs.values())]
        requirements = host_requirements(destination, members)
    glibc = requirements["symbol_versions"].get("GLIBC")
    sdl, al, ssl, zlib = [libs[name] for name in SONAMES]
    os_release = platform.freedesktop_os_release()
    distro = " ".join(v for v in (os_release.get("NAME"), os_release.get("VERSION_ID"),
                                  al["arch"] or platform.machine()) if v)
    if verified_psycross:
        component = f"""The Linux player statically links the MIT-licensed PsyCross reimplementation
at {revision}, with the project's public
patches. Its license is in PsyCross-LICENSE. The project license, notice
and acknowledgments are included here."""
        accompany = "accompany"
    else:
        component = (f"The project pins the MIT-licensed PsyCross reimplementation at {revision}. "
                     "See PsyCross-LICENSE.")
        accompany = "must accompany"
    if missing_notices:
        text = (component + "\n\nLocal development stage. Runtime origins or license texts are unverified for "
                + ", ".join(sorted(missing_notices)) + ".\n")
    else:
        al_version = al["source_version"].split(":")[-1]
        al_source = f"{al['source_package']}_{al_version}"
        text = f"""{component}

Bundled shared libraries from {distro}
SDL2 classic {sdl['version']}, zlib and component notices ({sdl['binary_package']}-copyright)
OpenAL Soft {al['version']}, LGPL-2.0-or-later ({al['binary_package']}-copyright)
OpenSSL libcrypto {ssl['version']}, Apache-2.0 ({ssl['binary_package']}-copyright)
zlib {zlib['version']}, zlib ({zlib['binary_package']}-copyright)

You may replace the dynamically loaded lib/libopenal.so.1 with a compatible
modified version. The three {al_source.rsplit('-', 1)[0]} source files and their SHA256
checksums {accompany} the Linux download. Extract the matching source with
dpkg-source -x {al_source}.dsc.
"""
    (notices / "THIRD-PARTY.txt").write_text(text)
    return {**identity, "glibc_minimum": glibc, "host_requirements": requirements, "native_lane": native_lane,
            "unverified_runtime_notices": sorted(missing_notices),
            "build_type": build_type, "runtime": "hybrid", "cpu_fallback": "MIPS interpreter",
            "runtime_distribution": distro, "runtime_libraries": list(libs.values()),
            "psycross": {"pinned_revision": revision, "public_patch": patch_path,
                         "public_patch_sha256": patch_sha, "tested_inputs_recorded": verified_psycross,
                         "license_notice": "licenses/PsyCross-LICENSE"}}


if __name__ == "__main__":
    try:
        print(json.dumps(stage(*(Path(arg).resolve() for arg in sys.argv[1:4]), sys.argv[4])))
    except (KeyError, ValueError, OSError, subprocess.CalledProcessError) as error:
        sys.exit(f"package_legion: {error}")
