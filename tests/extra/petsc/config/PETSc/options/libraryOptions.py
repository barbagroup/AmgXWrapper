#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def __str__(self):
    return ''

  def setupHelp(self, help):
    import nargs
    help.addArgument('PETSc', '-with-log=<bool>',              nargs.ArgBool(None, 1, 'Activate logging code in PETSc'))
    help.addArgument('PETSc', '-with-threadsafety=<bool>',     nargs.ArgBool(None, 0, 'Allow individual threads in PETSc to call PETSc routines'))
    help.addArgument('PETSc', '-with-info=<bool>',             nargs.ArgBool(None, 1, 'Activate PetscInfo() (i.e. -info)  code in PETSc'))
    help.addArgument('PETSc', '-with-ctable=<bool>',           nargs.ArgBool(None, 1, 'Activate CTABLE hashing for certain search functions - to conserve memory'))
    help.addArgument('PETSc', '-with-fortran-kernels=<bool>',  nargs.ArgBool(None, 0, 'Use Fortran for linear algebra kernels'))
    help.addArgument('PETSc', '-with-is-color-value-type=<char,short>',nargs.ArgString(None, 'short', 'char, short can store 256, 65536 colors'))
    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.debugging   = framework.require('PETSc.options.debugging', self)
    self.compilers   = framework.require('config.compilers', self)
    self.libraries   = framework.require('config.libraries', self)
    self.types       = framework.require('config.types', self)
    return



  def configureLibraryOptions(self):
    '''Sets PETSC_USE_DEBUG, PETSC_USE_INFO, PETSC_USE_LOG, PETSC_USE_CTABLE and PETSC_USE_FORTRAN_KERNELS'''
    '''Also sets PETSC_AssertAlignx() in Fortran and PETSC_Alignx() in C for IBM BG/P compiler '''
    if self.framework.argDB['with-threadsafety']:
      self.addDefine('HAVE_THREADSAFETY',1)
      self.useThreadSafety = 1
    else:
      self.useThreadSafety = 0

    if self.useThreadSafety and self.framework.argDB['with-log']:
      raise RuntimeError('Must use --with-log=0 with --with-threadsafety')

    self.useLog   = self.framework.argDB['with-log']
    self.addDefine('USE_LOG',   self.useLog)

    if self.debugging.debugging:
      self.addDefine('USE_DEBUG',1)
    elif not config.setCompilers.Configure.isIBM(self.framework.getCompiler(), self.log):
      # IBM XLC version 12.1 (BG/Q and POWER) miscompiles PetscMalloc3()
      # by reordering "*(void**)&ptr = x" as though ptr was not modified
      # by this statement.
      self.addDefine('USE_MALLOC_COALESCED',1)

    self.useInfo   = self.framework.argDB['with-info']
    self.addDefine('USE_INFO',   self.useInfo)

    self.useCtable = self.framework.argDB['with-ctable']
    if self.useCtable:
      self.addDefine('USE_CTABLE', '1')

    # used in src/mat/impls/sbaij/seq/relax.h
    self.libraries.saveLog()
    if not self.libraries.isBGL():
      self.addDefine('USE_BACKWARD_LOOP','1')
    self.logWrite(self.libraries.restoreLog())

    self.useFortranKernels = self.framework.argDB['with-fortran-kernels']
    if not hasattr(self.compilers, 'FC') and self.useFortranKernels:
      raise RuntimeError('Cannot use fortran kernels without a Fortran compiler')
    if self.useFortranKernels:
      self.addDefine('USE_FORTRAN_KERNELS', 1)
      if self.libraries.isBGL():
        self.addDefine('AssertAlignx(a,b)','call ALIGNX(a,b)')
      else:
        self.addDefine('AssertAlignx(a,b)','  ')

    if self.libraries.isBGL():
      self.addDefine('Alignx(a,b)','__alignx(a,b)')
    else:
      self.addDefine('Alignx(a,b)','  ')
    return

  def configureISColorValueType(self):
    '''Sets PETSC_IS_COLOR_VALUE_TYPE, MPIU_COLORING_VALUE, IS_COLORING_MAX required by ISColor'''
    self.isColorValueType  = self.framework.argDB['with-is-color-value-type']
    if self.isColorValueType != 'char' and self.isColorValueType != 'short':
      raise RuntimeError('Incorrect --with-is-color-value-type value specified. Can be either char or short. Specified value is :'+self.isColorValueType)
    if self.isColorValueType == 'char':
      max = pow(2,self.types.sizes['known-sizeof-char']*self.types.bits_per_byte)-1
      mpi_type = 'MPI_UNSIGNED_CHAR'
      sz = 'PETSC_SIZEOF_CHAR'
    else:
      max = pow(2,self.types.sizes['known-sizeof-short']*self.types.bits_per_byte)-1
      mpi_type = 'MPI_UNSIGNED_SHORT'
      sz  = 'PETSC_SIZEOF_SHORT'

    self.framework.addDefine('MPIU_COLORING_VALUE',mpi_type)
    self.framework.addDefine('IS_COLORING_MAX',max)
    self.addDefine('IS_COLOR_VALUE_TYPE', self.isColorValueType)
    self.addDefine('IS_COLOR_VALUE_TYPE_SIZE', sz)
    return

  def configure(self):
    self.executeTest(self.configureLibraryOptions)
    self.executeTest(self.configureISColorValueType)
    return
