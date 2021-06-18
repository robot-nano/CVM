import os
import sys

from setuptools import find_packages
from setuptools.dist import Distribution

if "--inplace" in sys.argv:
    from distutils.core import setup
    from distutils.extension import Extension
else:
    from setuptools import setup
    from setuptools.extension import Extension


def get_describe_version(original_version):
    return original_version


def config_cython():
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
                    include_dirs=[],
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


setup_kwargs = {}


__version__ = 0.1
__version__ = get_describe_version(__version__)


def get_package_data_files():
    # Relay standard libraries
    return ["relay/std/test.rly",]


setup(
    name="cvm",
    version=__version__,
    description="CVM: An study project of cpp virtual machine",
    zip_safe=False,
    entry_points={"console_scripts": [""]},
    install_requires=[""],
    extras_require={},
    packages=find_packages(),
    package_dir={"cvm": "cvm"},
    package_data={"cvm": get_package_data_files()},
    distclass=BinaryDistribution,
    url="https://github.com/robot-nano/CVM",
    ext_modules=config_cython(),
    **setup_kwargs,
)
