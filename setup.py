from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext as _build_ext
import os
import sys
import platform
import tempfile

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


# Custom build_ext command to check for float16 support and other architecture-specific features
class build_ext(_build_ext):
    def build_extensions(self):
        # Determine the system architecture
        architecture = platform.machine().lower()
        print(f"Detected architecture: {architecture}")

        # Set default compile args
        default_args = ["-std=c11"] if sys.platform != "win32" else []

        # Define a function to check if a flag is supported
        def check_flag(flag):
            with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
                f.write("int main() { return 0; }")
                fname = f.name
            try:
                self.compiler.compile([fname], extra_preargs=[flag])
                return True
            except:
                return False
            finally:
                os.remove(fname)

        # Define a function to check if AVX-512 intrinsic compilation is supported
        def check_avx512_support():
            with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
                f.write(
                    """
                #include <immintrin.h>
                int main() {
                    __m128 iv = _mm_set_ss(1.0f);
                    __m128i resv = _mm_cvtps_ph(iv, _MM_FROUND_TO_NEAREST_INT);
                    return 0;
                }"""
                )
                fname = f.name
            try:
                self.compiler.compile(
                    [fname], extra_preargs=["-mavx512f", "-mavx512bw", "-mavx512vl"]
                )
                return True
            except:
                return False
            finally:
                os.remove(fname)

        # Add architecture-specific flags if supported
        avx512_supported = False
        neon_supported = False
        if architecture in ["x86_64", "amd64", "i386", "i686"]:
            if check_avx512_support():
                avx512_supported = True
                for ext in self.extensions:
                    ext.extra_compile_args.extend(
                        default_args
                        + ["-mavx512f", "-mavx512bw", "-mavx512vl", "-DUSE_AVX512F"]
                    )
                print("Using AVX512F support")
            else:
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args)
        elif architecture.startswith("arm"):
            if check_flag("-mfpu=neon"):
                neon_supported = True
                for ext in self.extensions:
                    ext.extra_compile_args.extend(
                        default_args + ["-mfpu=neon", "-DUSE_NEON"]
                    )
                print("Using NEON support")
            else:
                for ext in self.extensions:
                    ext.extra_compile_args.extend(default_args)
        else:
            for ext in self.extensions:
                ext.extra_compile_args.extend(default_args)

        # Define a macro to indicate if SIMD support is available
        for ext in self.extensions:
            if avx512_supported or neon_supported:
                ext.define_macros.append(("USE_SIMD", "1"))
            else:
                ext.define_macros.append(("USE_SIMD", "0"))

        _build_ext.build_extensions(self)


# Define extensions
extensions = [
    Extension(
        "ragfile.ragfile",
        sources=ragfile_module_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"],  # Add define for using float16
    ),
    Extension(
        "ragfile.io",
        sources=ragfile_io_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"],  # Add define for using float16
    ),
    Extension(
        "ragfile.minhash",
        sources=minhash_sources,
        include_dirs=include_dirs,
        extra_compile_args=["-DUSE_FLOAT16"],  # Add define for using float16
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
