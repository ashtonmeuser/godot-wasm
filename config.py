def can_build(env, platform):
    return platform in ["linux", "linuxbsd", "x11," "windows", "osx", "macos"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "Wasm",
        "StreamPeerWasm",
    ]


def get_doc_path():
    return "doc_classes"
