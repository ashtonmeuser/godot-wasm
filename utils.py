import os
import re
import shutil
from urllib import request
import tarfile
from zipfile import ZipFile

WASMER_BASE_URL = "https://github.com/wasmerio/wasmer/releases/download/{0}/wasmer-{1}.tar.gz"
WASMER_VER_DEFAULT = "v6.0.1"
WASMTIME_BASE_URL = "https://github.com/bytecodealliance/wasmtime/releases/download/{0}/wasmtime-{0}-{1}-c-api.{2}"
WASMTIME_VER_DEFAULT = "v36.0.2"


def _validate_version(v):
    """Validate semver string"""
    if not re.fullmatch(r"v\d+\.\d+\.\d+(-.+)?", v):
        raise (ValueError("Invalid runtime version"))


def _strip_tar_members(f, s=""):
    """Optionally strip tarfile top level directory"""
    for member in f.getmembers():
        if re.fullmatch(s, member.path):
            continue  # Top level dir
        elif re.match(s + r"\/", member.path):  # Nested file
            member.path = member.path.split("/", 1)[1]
        yield member


def _download_tarfile(url, dest, rename={}):
    """Download and extract tarfile removing redundant top level dir"""
    strip = r"^{}[\w\-.]*".format(dest)  # Dir of same name as destination
    filename = "tmp.tar.gz"  # Temporary tarfile name
    os.makedirs(dest, exist_ok=True)
    request.urlretrieve(url, filename)
    with tarfile.open(filename) as file:
        file.extractall(dest, members=_strip_tar_members(file, strip))
    for k, v in rename.items():
        os.rename(k, v)
    os.remove(filename)


def _strip_zip_members(f, s=""):
    """Optionally strip zipfile top level directory"""
    for member in f.infolist():
        if member.is_dir():
            continue # Skip directories
        if re.fullmatch(s, member.filename):
            continue  # Top level dir
        elif re.match(s + r"\/", member.filename):  # Nested file
            member.filename = member.filename.split("/", 1)[1]
        yield member


def _download_zipfile(url, dest, rename={}):
    """Download and extract zipfile removing redundant top level dir"""
    strip = r"^{}[\w\-.]*".format(dest)  # Dir of same name as destination
    filename = "tmp.zip"  # Temporary zipfile name
    os.makedirs(dest, exist_ok=True)
    request.urlretrieve(url, filename)
    with ZipFile(filename, "r") as file:
        file.extractall(dest, members=_strip_zip_members(file, strip))
    for k, v in rename.items():
        os.rename(k, v)
    os.remove(filename)


def _patch_dll_import():
    """Patch old Wasm C API header (see https://github.com/WebAssembly/wasm-c-api/pull/183)"""
    for path in ["wasmer/include/wasm.h", "wasmtime/include/wasm.h"]:
        if not os.path.isfile(path):
            continue
        with open(path, "r") as file:
            content = file.read()
        content = content.replace("__declspec(dllimport)", "")
        with open(path, "w") as file:
            file.write(content)


def download_wasmer(env, force=False, version=WASMER_VER_DEFAULT):
    _validate_version(version)
    if not force and os.path.isdir("wasmer") and len(os.listdir("wasmer")):
        return  # Skip download
    print("Downloading Wasmer library {}".format(version))
    shutil.rmtree("wasmer", True)  # Remove old library
    if env["platform"] in ["osx", "macos"]:
        # For macOS, we need to universalize the AMD and ARM libraries
        _download_tarfile(
            WASMER_BASE_URL.format(version, "darwin-amd64"),
            "wasmer",
            {"wasmer/lib/libwasmer.a": "wasmer/lib/libwasmer.amd64.a"},
        )
        _download_tarfile(
            WASMER_BASE_URL.format(version, "darwin-arm64"),
            "wasmer",
            {"wasmer/lib/libwasmer.a": "wasmer/lib/libwasmer.arm64.a"},
        )
        os.system("lipo wasmer/lib/libwasmer.*64.a -output wasmer/lib/libwasmer.a -create")
    elif env["platform"] in ["linux", "linuxbsd", "x11"]:
        _download_tarfile(WASMER_BASE_URL.format(version, "linux-amd64"), "wasmer")
    elif env["platform"] == "windows":
        if env.get("use_mingw"):
            _download_tarfile(WASMER_BASE_URL.format(version, "windows-gnu64"), "wasmer")
        else:
            _download_tarfile(WASMER_BASE_URL.format(version, "windows-amd64"), "wasmer")
        # Temporary workaround for Wasm C API and Wasmer issue
        # See https://github.com/ashtonmeuser/godot-wasm/issues/26
        # See https://github.com/ashtonmeuser/godot-wasm/issues/29
        _patch_dll_import()


def download_wasmtime(env, force=False, version=WASMTIME_VER_DEFAULT):
    _validate_version(version)
    if not force and os.path.isdir("wasmtime") and len(os.listdir("wasmtime")):
        return  # Skip download
    print("Downloading Wasmtime library {}".format(version))
    shutil.rmtree("wasmtime", True)  # Remove old library
    if env["platform"] in ["osx", "macos"]:
        # For macOS, we need to universalize the AMD and ARM libraries
        _download_tarfile(
            WASMTIME_BASE_URL.format(version, "x86_64-macos", "tar.xz"),
            "wasmtime",
            {"wasmtime/lib/libwasmtime.a": "wasmtime/lib/libwasmtime.amd64.a"},
        )
        _download_tarfile(
            WASMTIME_BASE_URL.format(version, "aarch64-macos", "tar.xz"),
            "wasmtime",
            {"wasmtime/lib/libwasmtime.a": "wasmtime/lib/libwasmtime.arm64.a"},
        )
        os.system("lipo wasmtime/lib/libwasmtime.*64.a -output wasmtime/lib/libwasmtime.a -create")
    elif env["platform"] in ["linux", "linuxbsd", "x11"]:
        _download_tarfile(WASMTIME_BASE_URL.format(version, "x86_64-linux", "tar.xz"), "wasmtime")
    elif env["platform"] == "windows":
        _download_zipfile(WASMTIME_BASE_URL.format(version, "x86_64-windows", "zip"), "wasmtime")
