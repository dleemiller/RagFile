from setuptools import setup, Extension, find_packages
import os

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

module = Extension(
    "ragfile.ragfilemodule",  # Name of the module
    sources=[
        "ragfile/ragfilemodule.c",  # Python binding source file
        "src/core/ragfile.c",
        "src/core/minhash.c",
        "src/algorithms/jaccard.c",
        "src/algorithms/cosine.c",
        "src/utils/file_io.c",
        "src/search/heap.c",
        "src/search/scan.c",
    ],
    include_dirs=[
        "src/include",
        "src/core",
        "src/algorithms",
        "src/search",
        "src/utils"
    ],
    extra_compile_args=['-std=c11'],  # Ensuring C11 standard is used
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
    ext_modules=[module],
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
    install_requires=[
    ],
    extras_require={
        "dev": [
            "check-manifest",
            "pytest",
            "pytest-cov",
            "wheel",
        ],
    },
    package_data={
        "ragfile": ["*"],
    },
    data_files=[
        ("", ["LICENSE", "README.md"]),
    ],
    entry_points={
        "console_scripts": [
        ],
    },
)


