from setuptools import setup, Extension

ragfile_module = Extension('ragfile',
                           sources=['ragfilemodule.c',
                                    '../src/core/ragfile.c',
                                    '../src/core/minhash.c',
                                    '../src/algorithms/jaccard.c',
                                    '../src/algorithms/cosine.c',
                                    '../src/utils/file_io.c'],
                           include_dirs=['../include', '../src/core', '../src/algorithms', '../src/utils'])

setup(name='ragfile',
      version='1.0',
      description='Python bindings for RagFile library',
      ext_modules=[ragfile_module])
