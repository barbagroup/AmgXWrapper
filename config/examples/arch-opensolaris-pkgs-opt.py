#!/usr/bin/env python

configure_options = [
  #'--with-debugger=/bin/true',
  '--with-debugging=0',
  'FFLAGS=-ftrap=%none',

  '--download-mpich=1',
  '--with-c2html=0',

  #'-download-fblaslapack=1',
  '--download-cmake=1',
  '--download-metis=1',
  '--download-parmetis=1',
  '--download-triangle=1',
  '--download-superlu=1',
  '--download-superlu_dist=1',
  '--download-fblaslapack=1', # -lsunperf is insufficient for scalapack
  '--download-scalapack=1',
  '--download-mumps=1',
  '--download-parms=1',
  #'--download-elemental=1', breaks with solaris compilers
  #'--download-hdf5',
  '--download-sundials=1',
  '--download-hypre=1',
  '--download-suitesparse=1',
  '--download-chaco=1',
  '--download-spai=1',
  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
