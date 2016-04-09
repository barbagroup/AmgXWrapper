import config.package
import os

class Configure(config.package.CMakePackage):
  def __init__(self, framework):
    config.package.CMakePackage.__init__(self, framework)
    self.download         = ['http://software.lanl.gov/ascem/tpls/mstk-2.23.tgz']
    self.downloadfilename = 'mstk'
    self.includes         = ['MSTK.h']
    self.liblist          = [['libmstk.a']]
    self.functions        = []
    self.cxx              = 1
    self.requirescxx11    = 0
    self.downloadonWindows= 0
    self.hastests         = 1
    return

  def setupDependencies(self, framework):
    config.package.CMakePackage.setupDependencies(self, framework)
    self.compilerFlags   = framework.require('config.compilerFlags', self)
    self.mpi             = framework.require('config.packages.MPI',self)
    self.metis           = framework.require('config.packages.metis',self)
    self.zoltan          = framework.require('config.packages.Zoltan',self)
    self.deps            = [self.mpi]
    return

  def formCMakeConfigureArgs(self):
    args = config.package.CMakePackage.formCMakeConfigureArgs(self)
    args.append('-DUSE_XSDK_DEFAULTS=YES')
    if self.compilerFlags.debugging:
      args.append('-DCMAKE_BUILD_TYPE=DEBUG')
      args.append('-DXSDK_ENABLE_DEBUG=YES')
    else:
      args.append('-DCMAKE_BUILD_TYPE=RELEASE')
      args.append('-DXSDK_ENABLE_DEBUG=NO')

    args.append('-DENABLE_PARALLEL=yes')
    if not self.metis.found and not self.zoltan.found:
      raise  RuntimeError('MSTK requires either Metis (--download-metis) or Zoltan (--download-zoltan --download-parmetis --download-metis)!')
    if self.metis.found:
      args.append('-DENABLE_METIS:BOOL=ON')
      args.append('-DMETIS_DIR:FILEPATH='+self.metis.directory)
    if self.zoltan.found:
      args.append('-DENABLE_ZOLTAN:BOOL=ON')
      args.append('-DZOLTAN_DIR:FILEPATH='+self.zoltan.directory)

    #  Need to pass -DMETIS_5 to C and C++ compiler flags otherwise assumes older Metis
    args = self.rmArgsStartsWith(args,['-DCMAKE_CXX_FLAGS:STRING','-DCMAKE_C_FLAGS:STRING'])
    args.append('-DCMAKE_C_FLAGS:STRING="'+self.removeWarningFlags(self.setCompilers.getCompilerFlags())+' -DMETIS_5"')
    if hasattr(self.compilers, 'CXX'):
      self.framework.pushLanguage('Cxx')
      args.append('-DCMAKE_CXX_FLAGS:STRING="'+self.removeWarningFlags(self.framework.getCompilerFlags())+' -DMETIS_5"')
    self.framework.popLanguage()

    # mstk does not use the standard -DCMAKE_INSTALL_PREFIX
    args.append('-DINSTALL_DIR='+self.installDir)
    return args

