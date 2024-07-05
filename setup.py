from setuptools import setup, Extension, find_packages
import os
import sys

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

# Extension for ragfile module
ragfile_module = Extension(
    "ragfile.ragfile",
    sources=ragfile_module_sources,
    include_dirs=include_dirs,
    extra_compile_args=["-std=c11"] if sys.platform != "win32" else [],
)

# Extension for io module
ragfile_io = Extension(
    "ragfile.io",
    sources=ragfile_io_sources,
    include_dirs=include_dirs,
    extra_compile_args=["-std=c11"] if sys.platform != "win32" else [],
)

# Extension for minhash module
minhash_module = Extension(
    "ragfile.minhash",
    sources=minhash_sources,
    include_dirs=include_dirs,
    extra_compile_args=["-std=c11"] if sys.platform != "win32" else [],
)

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
    ext_modules=[ragfile_module, ragfile_io, minhash_module],
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
    ],
    entry_points={
        "console_scripts": [],
    },
)

