#!/usr/bin/env python

configure_options = [

  '--useThreads=0', # for some reason cmake hangs when invoked from configure on bsd?
  'LIBS=-lexecinfo', # for some reason gcc46+ requires this library

  '--with-clanguage=cxx',
  '--with-scalar-type=complex',

  #'-download-fblaslapack=1',
  '--download-mpich=1',
  '--download-cmake=1',
  '--download-metis=1',
  '--download-parmetis=1',
  '--download-triangle=1',
  #'--download-superlu=1',
  #'--download-superlu_dist=1',
  '--download-scalapack=1',
  '--download-mumps=1',
  '--download-hdf5',
  # no with-cxx-dialect=C++11 support '--download-elemental=1',
  #'--download-sundials=1',
  #'--download-hypre=1',
  #'--download-suitesparse=1',
  #'--download-chaco=1',
  #'--download-spai=1',

  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
