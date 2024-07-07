from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext as _build_ext
import os
import sys
import platform
import tempfile
import subprocess

# Read the README file
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

# Define directories
include_dirs = [
    "src/include",
    "src/core",
    "src/algorithms",
    "src/search",
    "src/utils",
    "src/python",
]

# Common sources
common_sources = [
    "src/python/pyragfile.c",
    "src/python/pyragfileheader.c",
    "src/python/similarity.c",
    "src/python/utility.c",
    "src/core/ragfile.c",
    "src/utils/strdup.c",
    "src/algorithms/minhash.c",
    "src/algorithms/quantize.c",
    "src/algorithms/hamming.c",
    "src/algorithms/jaccard.c",
    "src/algorithms/cosine.c",
    "src/search/heap.c",
    "src/search/scan.c",
    "src/utils/file_io.c",
]

# Specific sources for the ragfile module
ragfile_module_sources = common_sources + [
    "src/python/ragfilemodule.c",
]

# Specific sources for the io module
ragfile_io_sources = common_sources + [
    "src/python/io.c",
]

# Specific sources for the minhash module
minhash_sources = [
    "src/algorithms/minhash.c",
    "src/python/pyminhash.c",
]

# Custom build_ext command to check for F16C and NEON support
class build_ext(_build_ext):
    def build_extensions(self):
        # Determine the system architecture
        architecture = platform.machine().lower()

        # Set default compile args
        default_args = ["-std=c11"] if sys.platform != "win32" else []

        if architecture == "x86_64":
            if self.check_flag("-mf16c"):
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args + ["-mf16c", "-DUSE_F16C"])
                print("Using F16C support")
            else:
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args)
                print("F16C not supported. Falling back to software implementation.")
        elif architecture.startswith("arm"):
            if self.check_flag("-mfpu=neon"):
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args + ["-mfpu=neon", "-DUSE_NEON"])
                print("Using NEON support")
            else:
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args)
                print("NEON not supported. Falling back to software implementation.")
        else:
            for ext in self.extensions:
                ext.extra_compile_args.extend(default_args)
                print("Architecture not recognized. Falling back to software implementation.")

        _build_ext.build_extensions(self)

    def check_flag(self, flag):
        """ Check if the given flag is supported by the compiler. """
        with tempfile.NamedTemporaryFile('w', delete=False) as f:
            f.write('int main() { return 0; }\n')
        try:
            subprocess.check_output(['gcc', flag, f.name], stderr=subprocess.STDOUT)
            return True
        except subprocess.CalledProcessError:
            return False
        finally:
            os.remove(f.name)


# Define extensions
extensions = [
    Extension(
        "ragfile.ragfile",
        sources=ragfile_module_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"]  # Add define for using float16
    ),
    Extension(
        "ragfile.io",
        sources=ragfile_io_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"]  # Add define for using float16
    ),
    Extension(
        "ragfile.minhash",
        sources=minhash_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"]  # Add define for using float16
    ),
]

setup(
    name="ragfile",
    version="0.1.0",
    author="D. Lee Miller",
    author_email="dleemiller@gmail.com",
    description="A Python module for handling RAG files with native C extensions",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/dleemiller/RagFile",
    packages=find_packages(),
    ext_modules=extensions,
    cmdclass={"build_ext": build_ext},
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    python_requires=">=3.6",
    install_requires=[],
    extras_require={
        "dev": [
            "check-manifest",
            "pytest",
            "pytest-cov",
            "wheel",
        ],
        "transformers": ["transformers>=4.0.0"],
    },
    package_data={
        "ragfile": ["*"],
    },
    data_files=[
        ("", ["LICENSE", "README.md"]),
        ("include", ["src/include/float16.h"]),  # Include float16.h
    ],
    entry_points={
        "console_scripts": [],
    },
)

