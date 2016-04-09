import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.download          = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/Chaco-2.2-p2.tar.gz']
    self.functions         = ['interface']
    self.includes          = [] #Chaco does not have an include file
    self.needsMath         = 1
    self.liblist           = [['libchaco.a']]
    self.license           = 'http://www.cs.sandia.gov/web1400/1400_download.html'
    self.downloadonWindows = 1
    self.requires32bitint  = 1;  # 1 means that the package will not work with 64 bit integers
    self.hastests          = 1
    return

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    return

  def Install(self):
    import os
    self.log.write('chacoDir = '+self.packageDir+' installDir '+self.installDir+'\n')

    mkfile = 'make.inc'
    g = open(os.path.join(self.packageDir, mkfile), 'w')
    self.setCompilers.pushLanguage('C')
    g.write('CC = '+self.setCompilers.getCompiler()+'\n')
    g.write('CFLAGS = '+self.setCompilers.getCompilerFlags()+'\n')
    g.write('OFLAGS = '+self.setCompilers.getCompilerFlags()+'\n')
    self.setCompilers.popLanguage()
    g.close()

    if self.installNeeded(mkfile):
      try:
        self.logPrintBox('Compiling and installing chaco; this may take several minutes')
        self.installDirProvider.printSudoPasswordMessage()
        output,err,ret  = config.package.Package.executeShellCommand('cd '+self.packageDir+' && cd code && make clean && make && '+self.installSudo+'mkdir -p '+os.path.join(self.installDir,self.libdir)+' && cd '+self.installDir+' && '+self.installSudo+self.setCompilers.AR+' '+self.setCompilers.AR_FLAGS+' '+self.libdir+'/libchaco.'+self.setCompilers.AR_LIB_SUFFIX+' `find '+self.packageDir+'/code -name "*.o"` && cd '+self.libdir+' && '+self.installSudo+self.setCompilers.AR+' d libchaco.'+self.setCompilers.AR_LIB_SUFFIX+' main.o && '+self.installSudo+self.setCompilers.RANLIB+' libchaco.'+self.setCompilers.AR_LIB_SUFFIX, timeout=2500, log = self.log)
      except RuntimeError, e:
        raise RuntimeError('Error running make on CHACO: '+str(e))
      self.postInstall(output+err, mkfile)
    return self.installDir

  def configureLibrary(self):
    config.package.Package.configureLibrary(self)
    if not self.libraries.check(self.lib, 'ddot_chaco',otherLibs=self.libraries.math):
      raise RuntimeError('You cannot use Chaco package from Sandia as it contains an incorrect ddot() routine that conflicts with BLAS\nUse --download-chaco')

