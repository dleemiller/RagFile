from setuptools import setup, Extension, find_packages
import os

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

module = Extension(
    "ragfile.ragfile",
    sources=[
        "ragfile/ragfilemodule.c",
        "src/core/ragfile.c",
        "src/core/minhash.c",
        "src/algorithms/jaccard.c",
        "src/algorithms/cosine.c",
        "src/utils/file_io.c",
    ],
    include_dirs=["src/include"],
)

setup(
    name="ragfile",
    version="0.1.0",
    author="D. Lee Miller",
    author_email="dleemiller@gmail.com",
    description="File format for flat file RAG",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/ragfile",
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
        # Add any additional dependencies here
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
        "ragfile": ["ragfilemodule.c"],
    },
    data_files=[
        ("", ["LICENSE", "README.md"]),
    ],
    entry_points={
        "console_scripts": [
            # Add command line scripts here
        ],
    },
)
