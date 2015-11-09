import config.package
import os

class Configure(config.package.GNUPackage):
  def __init__(self, framework):
    config.package.GNUPackage.__init__(self, framework)
    self.download  = ['http://www.open-mpi.org/software/ompi/v1.8/downloads/openmpi-1.8.5.tar.gz',
                      'http://ftp.mcs.anl.gov/pub/petsc/externalpackages/openmpi-1.8.5.tar.gz']
    self.downloadfilename = 'openmpi'
    self.skippackagewithoptions = 1
    self.isMPI = 1
    return

  def formGNUConfigureArgs(self):
    args = config.package.GNUPackage.formGNUConfigureArgs(self)
    args.append('--with-rsh=ssh')
    args.append('MAKE='+self.make.make)
    if not hasattr(self.compilers, 'CXX'):
      raise RuntimeError('Error: OpenMPI requires C++ compiler. None specified')
    if hasattr(self.compilers, 'FC'):
      self.pushLanguage('FC')
      if not self.compilers.fortranIsF90:
        args.append('--disable-mpi-f90')
        args.append('FC=""')
      self.popLanguage()
    else:
      args.append('--disable-mpi-f77')
      args.append('--disable-mpi-f90')
      args.append('F77=""')
      args.append('FC=""')
    if not self.argDB['with-shared-libraries']:
      args.append('--enable-shared=no')
      args.append('--enable-static=yes')
    args.append('--disable-vt')
    # have OpenMPI build its own private copy of hwloc to prevent possible conflict with one used by PETSc
    args.append('--with-hwloc=internal')
    return args

  def Install(self):
    '''After downloading and installing OpenMPI we need to reset the compilers to use those defined by the OpenMPI install'''
    installDir = config.package.GNUPackage.Install(self)
    self.updateCompilers(installDir,'mpicc','mpicxx','mpif77','mpif90')
    return installDir

