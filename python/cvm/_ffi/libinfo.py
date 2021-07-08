"""Library information"""
import os
import sys


def split_env_var(env_var, split):
    """Splits environment variable string.

    Args:
        env_var: str
            Name of environment variable.
        split: str
            String to split env_var on.

    Returns:
        splits: list(string)
            If env_var exists, split env_var. Otherwise, empty list.
    """
    if os.environ.get(env_var, None):
        return [p.strip() for p in os.environ[env_var].split(split)]
    return []


def get_dll_directories():
    """Get the possible dll directories"""
    ffi_dir = os.path.dirname(os.path.realpath(os.path.expanduser(__file__)))
    source_dir = os.path.join(ffi_dir, "..", "..", "..")
    install_lib_dir = os.path.join(ffi_dir, "..", "..", "..", "..")

    dll_path = []

    if os.environ.get("CVM_LIBRARY_PATH", None):
        dll_path.append(os.environ["CVM_LIBRARY_PATH"])

    if sys.platform.startswith("linux") or sys.platform.startswith("freebsd"):
        dll_path.extend(split_env_var("LD_LIBRARY_PATH", ":"))
        dll_path.extend(split_env_var("PATH", ":"))
    elif sys.platform.startswith("darwin"):
        dll_path.extend(split_env_var("DYLD_LIBRARY_PATH", ":"))
        dll_path.extend(split_env_var("PATH", ":"))
    elif sys.platform.startswith("win32"):
        dll_path.extend(split_env_var("PATH", ";"))

    # Pip lib directory
    dll_path.append(os.path.join(ffi_dir, ".."))
    # Default cmake build directory
    dll_path.append(os.path.join(source_dir, "build"))
    dll_path.append(os.path.join(source_dir, "build", "Release"))
    # Default make build directory
    dll_path.append(os.path.join(source_dir, "lib"))

    dll_path.append(install_lib_dir)

    if os.path.isdir(source_dir):
        dll_path.append(os.path.join(source_dir, "web", "dist", "wasm"))
        dll_path.append(os.path.join(source_dir, "web", "dist"))

    dll_path = [os.path.realpath(x) for x in dll_path]
    return [x for x in dll_path if os.path.isdir(x)]


def find_lib_path(name=None, search_path=None, optional=False):
    """Find dynamic library files.

    Parameters
    ----------
    name : list of str
        List of names to be found

    Returns
    -------
    lib_path : list(string)
        List of all found path to the libraries
    """
    use_runtime = os.environ.get("CVM_USE_RUNTIME_LIB", False)
    dll_path = get_dll_directories()

    if search_path is not None:
        if isinstance(search_path, list):
            dll_path = dll_path + search_path
        else:
            dll_path.append(search_path)

    if name is not None:
        if isinstance(name, list):
            lib_dll_path = []
            for n in name:
                lib_dll_path += [os.path.join(p, n) for p in dll_path]
        else:
            lib_dll_path = [os.path.join(p, name) for p in dll_path]
        runtime_dll_path = []
    else:
        if sys.platform.startswith("win32"):
            lib_dll_path = [os.path.join(p, "libcvm.dll") for p in dll_path] + [
                os.path.join(p, "cvm.dll") for p in dll_path
            ]
            runtime_dll_path = [os.path.join(p, "libcvm.runtime.dll") for p in dll_path] + [
                os.path.join(p, "cvm_runtime.dll") for p in dll_path
            ]
        elif sys.platform.startswith("darwin"):
            lib_dll_path = [os.path.join(p, "libcvm.dylib") for p in dll_path]
            runtime_dll_path = [os.path.join(p, "libcvm_runtime.dylib") for p in dll_path]
        else:
            lib_dll_path = [os.path.join(p, "libcvm.so") for p in dll_path]
            runtime_dll_path = [os.path.join(p, "libcvm_runtime.so") for p in dll_path]

    if not use_runtime:
        # try to find lib_dll_path
        lib_found = [p for p in lib_dll_path if os.path.exists(p) and os.path.isfile(p)]
        lib_found += [p for p in runtime_dll_path if os.path.exists(p) and os.path.isfile(p)]
    else:
        # try to find runtime_dll_path
        use_runtime = True
        lib_found = [p for p in runtime_dll_path if os.path.exists(p) and os.path.isfile(p)]

    if not lib_found:
        message = (
            "Cannot find the files.\n"
            + "List of candidates:\n"
            + str("\n".join(lib_dll_path + runtime_dll_path))
        )
        if not optional:
            raise RuntimeError(message)
        return None

    if use_runtime:
        sys.stderr.write("Load runtime library %s... exec only\n" % lib_found[0])
        sys.stderr.flush()
    return lib_found


def find_include_path(name=None, search_path=None, optional=False):
    """Find header files for C compilation.

    Parameters
    ----------
    name : list of str
        List of directory names to be searched.

    Returns
    -------
    include_path : list(string)
        List of all found paths to header files.
    """
    ffi_dir = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
    source_dir = os.path.join(ffi_dir, "..", "..", "..")

    third_party_dir = os.path.join(source_dir, "3rdparty")

    header_path = []

    if os.environ.get("CVM_INCLUDE_PATH", None):
        header_path.append(os.environ["CVM_INCLUDE_PATH"])

    header_path.append(source_dir)
    header_path.append(third_party_dir)

    header_path = [os.path.abspath(x) for x in header_path]
    if search_path is not None:
        if isinstance(search_path, list):
            header_path = header_path + search_path
        else:
            header_path.append(search_path)
    if name is not None:
        if isinstance(name, list):
            cvm_include_path = []
            for n in name:
                cvm_include_path += [os.path.join(p, n) for p in header_path]
        else:
            cvm_include_path = [os.path.join(p, name) for p in header_path]
    else:
        cvm_include_path = [os.path.join(p, "include") for p in header_path]

        # try to find include path
        include_found = [p for p in cvm_include_path if os.path.exists(p) and os.path.isdir(p)]

    if not include_found:
        message = (
            "Cannot find the files.\n"
            + "List of candidates:\n"
            + str("\n".join(cvm_include_path))
        )
        if not optional:
            raise RuntimeError(message)
        return None

    return include_found


__version__ = "0.1.dev0"
