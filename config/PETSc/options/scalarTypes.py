#!/usr/bin/env python
from __future__ import generators
import user
import config.base

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def __str1__(self):
    desc = []
    if hasattr(self, 'scalartype'):
      desc.append('  Scalar type: ' + self.scalartype)
    if hasattr(self, 'precision'):
      desc.append('  Precision: ' + self.precision)
    return '\n'.join(desc)+'\n'

  def setupHelp(self, help):
    import nargs
    help.addArgument('PETSc', '-with-precision=<single,double,__float128>', nargs.Arg(None, 'double', 'Specify numerical precision'))
    help.addArgument('PETSc', '-with-scalar-type=<real or complex>', nargs.Arg(None, 'real', 'Specify real or complex numbers'))
    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.types     = framework.require('config.types', self)
    self.languages = framework.require('PETSc.options.languages', self)
    self.compilers = framework.require('config.compilers', self)
    self.libraries = framework.require('config.libraries',self)
    self.mpi       = framework.require('config.packages.MPI',self)
    return


  def configureScalarType(self):
    '''Choose between real and complex numbers'''
    self.scalartype = self.framework.argDB['with-scalar-type'].lower()
    if self.scalartype == 'complex':
      self.addDefine('USE_COMPLEX', '1')
      if self.languages.clanguage == 'C' and not self.types.c99_complex:
        raise RuntimeError('C Compiler provided doest not support C99 complex')
      if self.languages.clanguage == 'Cxx' and not self.types.cxx_complex:
        raise RuntimeError('Cxx compiler provided does not support std::complex')
    elif not self.scalartype == 'real':
      raise RuntimeError('--with-scalar-type must be real or complex')
    self.addDefine('USE_SCALAR_'+self.scalartype.upper(), '1')
    self.logPrint('Scalar type is '+str(self.scalartype))
    # On apple isinf() and isnan() do not work when <complex> is included
    self.pushLanguage(self.languages.clanguage)
    if self.scalartype == 'complex' and self.languages.clanguage == 'Cxx':
      if self.checkLink('#include <math.h>\n#include <complex>\n','double b = 2.0;int a = isnormal(b);\n'):
        self.addDefine('HAVE_ISNORMAL',1)
      if self.checkLink('#include <math.h>\n#include <complex>\n','double b = 2.0;int a = isnan(b);\n'):
        self.addDefine('HAVE_ISNAN',1)
      if self.checkLink('#include <math.h>\n#include <complex>\n','double b = 2.0;int a = isinf(b);\n'):
        self.addDefine('HAVE_ISINF',1)
      if self.checkLink('#include <float.h>\n#include <complex>\n','double b = 2.0;int a = _isnan(b);\n'):
        self.addDefine('HAVE__ISNAN',1)
      if self.checkLink('#include <float.h>\n#include <complex>\n','double b = 2.0;int a = _finite(b);\n'):
        self.addDefine('HAVE__FINITE',1)
    else:
      if self.checkLink('#include <math.h>\n','double b = 2.0; int a = isnormal(b);\n'):
        self.addDefine('HAVE_ISNORMAL',1)
      if self.checkLink('#include <math.h>\n','double b = 2.0; int a = isnan(b);\n'):
        self.addDefine('HAVE_ISNAN',1)
      if self.checkLink('#include <math.h>\n','double b = 2.0; int a = isinf(b);\n'):
        self.addDefine('HAVE_ISINF',1)
      if self.checkLink('#include <float.h>\n','double b = 2.0;int a = _isnan(b);\n'):
        self.addDefine('HAVE__ISNAN',1)
      if self.checkLink('#include <float.h>\n','double b = 2.0;int a = _finite(b);\n'):
        self.addDefine('HAVE__FINITE',1)
    self.popLanguage()
    return

  def configurePrecision(self):
    '''Set the default real number precision for PETSc objects'''
    self.precision = self.framework.argDB['with-precision'].lower()
    if self.precision == 'single':
      self.addDefine('USE_REAL_SINGLE', '1')
      self.addMakeMacro('PETSC_SCALAR_SIZE', '32')
    elif self.precision == 'double':
      self.addDefine('USE_REAL_DOUBLE', '1')
      self.addMakeMacro('PETSC_SCALAR_SIZE', '64')
    elif self.precision == '__float128':  # supported by gcc 4.6
      if self.libraries.add('quadmath','logq',prototype='#include <quadmath.h>',call='__float128 f; logq(f);'):
        self.addDefine('USE_REAL___FLOAT128', '1')
        self.addMakeMacro('PETSC_SCALAR_SIZE', '128')
      else:
        raise RuntimeError('quadmath support not found. --with-precision=__float128 works with gcc-4.6 and newer compilers.')
    else:
      raise RuntimeError('--with-precision must be single, double,__float128')
    self.logPrint('Precision is '+str(self.precision))
    return

  def configure(self):
    self.executeTest(self.configureScalarType)
    self.executeTest(self.configurePrecision)
    return
