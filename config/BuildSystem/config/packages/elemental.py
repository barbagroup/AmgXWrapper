import config.package

class Configure(config.package.CMakePackage):
  def __init__(self, framework):
    config.package.CMakePackage.__init__(self, framework)
    self.gitcommit        = 'e81005dc3af9b331d43e16e4d221e889378e2c49'
    self.download          = ['git://https://github.com/elemental/Elemental']
    self.liblist          = [['libEl.a','libElSuiteSparse.a','libpmrrr.a']]
    self.includes         = ['El.hpp']
    self.cxx              = 1
    self.requirescxx11    = 1
    self.downloadonWindows= 0
    self.hastests         = 1
    self.downloadfilename = 'Elemental'
    return

  def setupDependencies(self, framework):
    config.package.CMakePackage.setupDependencies(self, framework)
    self.sharedLibraries = framework.require('PETSc.options.sharedLibraries', self)
    self.compilerFlags   = framework.require('config.compilerFlags', self)
    self.blasLapack      = framework.require('config.packages.BlasLapack',self)
    self.mpi             = framework.require('config.packages.MPI',self)
    self.metis           = framework.require('config.packages.metis',self)
    self.parmetis        = framework.require('config.packages.parmetis',self)
    self.deps            = [self.mpi,self.blasLapack,self.metis,self.parmetis]
    #
    # also requires the ./configure option --with-cxx-dialect=C++11
    return

  def formCMakeConfigureArgs(self):
    if not self.cmake.found:
      raise RuntimeError('CMake > 2.8.12 is needed to build Elemental')
    args = config.package.CMakePackage.formCMakeConfigureArgs(self)
    if self.compilerFlags.debugging:
      args.append('-DEL_ZERO_INIT=ON')
      args.append('-DCMAKE_BUILD_TYPE:STRING="Debug"')
    else:
      args.append('-DCMAKE_BUILD_TYPE:STRING="Release"')
    args.append('-DEL_DISABLE_VALGRIND=ON')
    args.append('-DEL_USE_QT5=OFF') # otherwise we would need Qt5 include paths to compile
    args.append('-DEL_FORCE_METIS_BUILD=OFF')
    args.append('-DEL_FORCE_PARMETIS_BUILD=OFF')
    args.append('-DEL_PREVENT_METIS_DOWNLOAD=ON')
    args.append('-DEL_PREVENT_PARMETIS_DOWNLOAD=ON')
    args.append('-DINSTALL_PYTHON_PACKAGE=FALSE')
    args.append('-DEL_DISABLE_SCALAPACK=ON')
    args.append('-DMETIS_INCLUDE_DIR:STRING="'+self.metis.include[0]+'"')
    args.append('-DMETIS_LIBRARY:STRING="'+self.libraries.toString(self.metis.lib)+'"')
    args.append('-DPARMETIS_INCLUDE_DIR:STRING="'+self.parmetis.include[0]+'"')
    args.append('-DPARMETIS_LIBRARY:STRING="'+self.libraries.toString(self.parmetis.lib)+'"')
    args.append('-DMATH_LIBS:STRING="'+self.libraries.toString(self.blasLapack.dlib)+'"')
    if self.setCompilers.isDarwin(self.log):
      # shared library build doesn't work on Apple
      args.append('-DBUILD_SHARED_LIBS=off')
    if not self.sharedLibraries.useShared:
      args.append('-DBUILD_SHARED_LIBS=off')

    self.framework.pushLanguage('C')
    args.append('-DMPI_C_COMPILER="'+self.framework.getCompiler()+'"')
    if self.argDB['with-64-bit-indices']:
      args.append('-DEL_USE_64BIT_INTS=ON')
    self.framework.popLanguage()

    self.framework.pushLanguage('Cxx')
    if config.setCompilers.Configure.isSolaris(self.log):
       raise RuntimeError('Sorry, Elemental does not compile with Oracle/Solaris/Sun compilers')
    args.append('-DMPI_CXX_COMPILER="'+self.framework.getCompiler()+'"')
    self.framework.popLanguage()

    if hasattr(self.compilers, 'FC'):
      self.framework.pushLanguage('FC')
      args.append('-DMPI_Fortran_COMPILER="'+self.framework.getCompiler()+'"')
      self.framework.popLanguage()
    return args
