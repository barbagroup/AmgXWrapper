import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.gitcommit = '17bb8834f8a2e2a19ac76c3aa2b117f40cf16991'
    self.download  = ['git://https://bitbucket.org/caidao22/pkg-revolve.git']
    self.functions = ['revolve_create_offline']
    self.includes  = ['revolve_c.h']
    self.liblist   = [['librevolve.a']]
    self.cxx       = 1
    self.hastests  = 1
    # revolve include files are in the lib directory
    return

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    self.petscdir = framework.require('PETSc.options.petscdir',self.setCompilers)
    return

  def Install(self):
    import os

    self.framework.pushLanguage('Cxx')
    g = open(os.path.join(self.packageDir,'make.inc'),'w')
    g.write('CP               = '+self.programs.cp+'\n')
    g.write('RM               = '+self.programs.RM+'\n')
    g.write('MKDIR            = '+self.programs.mkdir+'\n')

    g.write('AR               = '+self.setCompilers.AR+'\n')
    g.write('ARFLAGS          = '+self.setCompilers.AR_FLAGS+'\n')
    g.write('AR_LIB_SUFFIX    = '+self.setCompilers.AR_LIB_SUFFIX+'\n')
    g.write('RANLIB           = '+self.setCompilers.RANLIB+'\n')

    g.write('PREFIX           = '+self.installDir+'\n')

    g.write('CXX              = '+self.setCompilers.getCompiler()+'\n')
    g.write('CXXFLAGS         = '+self.setCompilers.getCompilerFlags()+'\n')
    g.close()

    self.framework.popLanguage()

    if self.installNeeded('make.inc'):
      self.logPrintBox('Configuring, compiling and installing revolve; this may take several seconds')
      self.installDirProvider.printSudoPasswordMessage()
      output1,err1,ret1  = config.package.Package.executeShellCommand('cd '+self.packageDir+' && make clean && make lib',timeout=500, log = self.log)
      output2,err2,ret2  = config.package.Package.executeShellCommand('cd '+self.packageDir+' && '+self.installSudo+' make install ',timeout=250, log = self.log)
      self.postInstall(output1+err1+output2+err2,'make.inc')
    return self.installDir
