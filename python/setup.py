import os
import sys
import shutil
import sysconfig

from setuptools import find_packages
from setuptools.dist import Distribution

if "--inplace" in sys.argv:
    from distutils.core import setup
    from distutils.extension import Extension
else:
    from setuptools import setup
    from setuptools.extension import Extension

CURRENT_DIR = os.path.dirname(__file__)
FFI_MODE = os.environ.get("CVM_FFI", "auto")
CONDA_BUILD = os.getenv("CONDA_BUILD") is not None


def get_lib_path():
    """Get library path, name and version"""
    libinfo_py = os.path.join(CURRENT_DIR, "./cvm/_ffi/libinfo.py")
    libinfo = {"__file__": libinfo_py}
    exec(compile(open(libinfo_py, "rb").read(), libinfo_py, "exec"), libinfo, libinfo)
    version = libinfo["__version__"]
    if not CONDA_BUILD:
        lib_path = libinfo["find_lib_path"]()
        libs = [lib_path[0]]
        if libs[0].find("runtime") == -1:
            for name in lib_path[1:]:
                if name.find("runtime") != -1:
                    libs.append(name)
    else:
        libs = None
    return libs, version


def get_describe_version(original_version):
    return original_version


LIB_LIST, __version__ = get_lib_path()
__version__ = get_describe_version(__version__)


def config_cython():
    """Try to configure _cython and return _cython configuration"""
    if FFI_MODE not in ("_cython"):
        if os.name == "nt" and not CONDA_BUILD:
            print("WARNING: Cython is not supported on Windows, will compile without _cython module")
            return []
        sys_cflags = sysconfig.get_config_var("CFLAGS")
        if sys_cflags and "i386" in sys_cflags and "x86_64" in sys_cflags:
            print("WARNING: Cython library may not be compiled correctly with both i386 and x64")
            return []
    try:
        from Cython.Build import cythonize

        if sys.version_info >= (3, 0):
            subdir = "_cy3"
        else:
            subdir = "_cy2"
        ret = []
        path = "cvm/_ffi/_cython"
        extra_compile_args = ["-std=c++14"]
        if os.name == "nt":
            library_dirs = ["cvm", "../build/Release", "../build"]
            libraries = ["cvm"]
            extra_compile_args = None
            # library is available via conda env
            if CONDA_BUILD:
                library_dirs = [os.environ["LIBRARY_LIB"]]
        else:
            library_dirs = None
            libraries = None

        for fn in os.listdir(path):
            if not fn.endswith(".pyx"):
                continue
            ret.append(
                Extension(
                    "cvm._ffi.%s.%s" % (subdir, fn[:-4]),
                    ["cvm/_ffi/_cython/%s" % fn],
                    include_dirs=["../include",
                                  "../3rdparty/dlpack/include"],
                    extra_compile_args=extra_compile_args,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    language="c++"
                )
            )
        return cythonize(ret, compiler_directives={"language_level": 3})
    except ImportError as error:
        return []


class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True

    def is_pure(self):
        return False


include_libs = False
wheel_include_libs = False
if not CONDA_BUILD:
    if "bdist_wheel" in sys.argv:
        wheel_include_libs = True
    else:
        include_libs = True

setup_kwargs = {}

# For bdist_wheel only
if wheel_include_libs:
    with open("MANIFEST.in", "w") as fo:
        for path in LIB_LIST:
            shutil.copy(path, os.path.join(CURRENT_DIR, "cvm"))
            _, libname = os.path.split(path)
            fo.write("include cvm/%s\n" % libname)
    setup_kwargs = {"include_package_data": True}

if include_libs:
    curr_path = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
    for i, path in enumerate(LIB_LIST):
        LIB_LIST[i] = os.path.relpath(path, curr_path)
    setup_kwargs = {"include_package_data": True, "data_files": [("cvm", LIB_LIST)]}


def get_package_data_files():
    # Relay standard libraries
    return ["relay/std/test.rly", ]


# Temporarily add this directory to the path so we can import the requirements generator
# tool.
sys.path.insert(0, os.path.dirname(__file__))
import gen_requirements

sys.path.pop(0)

requirements = gen_requirements.join_requirements()
extras_require = {
    piece: deps for piece, (_, deps) in requirements.items() if piece not in ("all", "core")
}


setup(
    name="cvm",
    version=__version__,
    description="CVM: An study project of cpp virtual machine",
    zip_safe=False,
    entry_points={"console_scripts": ["cvmc = cvm.driver.cvmc.main:main"]},
    install_requires=requirements["core"][1],
    extras_require=extras_require,
    packages=find_packages(),
    package_dir={"cvm": "cvm"},
    package_data={"cvm": get_package_data_files()},
    distclass=BinaryDistribution,
    url="https://github.com/robot-nano/CVM",
    ext_modules=config_cython(),
    **setup_kwargs,
)
