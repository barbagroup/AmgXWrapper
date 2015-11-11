#!/usr/bin/python
if __name__ == '__main__':
  import sys
  import os
  sys.path.insert(0, os.path.abspath('config'))
  import configure
  configure_options = [
    '--download-hdf5',
    '--download-moab',
    '--download-mpich',
    '--download-netcdf',
    '--download-netcdf-cxx',
    '--with-clanguage=c++',
    'PETSC_ARCH=arch-moab',
  ]
  configure.petsc_configure(configure_options)
