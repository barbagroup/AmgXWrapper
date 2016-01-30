#!/usr/bin/env python

configure_options = [
  '--with-debugging=0',
  '--download-fblaslapack=1',
  '--download-hypre=1',
  '--download-metis=1',
  '--download-parmetis=1',
  '--download-superlu_dist=1',
  '--download-trilinos=1',
  '--download-mpich=1',
  '--with-clanguage=C++',
  '--with-cxx-dialect=C++11',
  '--download-sowing=1',
  '--download-boost=1',
  '--download-cmake=1'
  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
