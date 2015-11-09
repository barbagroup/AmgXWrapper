import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.download     = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/PARTY_1.99.tar.gz']
    self.functions    = ['party_lib']
    self.includes     = ['party_lib.h']
    self.liblist      = [['libparty.a']]
    self.license      = 'http://www2.cs.uni-paderborn.de/cs/robsy/party.html'
    return

  def Install(self):
    import os

    g = open(os.path.join(self.packageDir,'make.inc'),'w')
    self.setCompilers.pushLanguage('C')
    g.write('CC = '+self.setCompilers.getCompiler()+' '+self.setCompilers.getCompilerFlags()+'\n')
    self.setCompilers.popLanguage()
    g.close()

    if self.installNeeded('make.inc'):
      try:
        self.logPrintBox('Compiling party; this may take several minutes')
        self.installDirProvider.printSudoPasswordMessage()
        output,err,ret  = config.package.Package.executeShellCommand('cd '+os.path.join(self.packageDir,'src')+' && make clean && make all && cd .. && '+self.installSudo+'mkdir -p '+os.path.join(self.installDir,self.libdir)+'&& '+self.installSudo+'cp -f *.a '+os.path.join(self.installDir,self.libdir,'')+' && '+self.installSudo+'mkdir -p '+os.path.join(self.installDir,self.includedir)+' && '+self.installSudo+'cp -f party_lib.h '+os.path.join(self.installDir,self.includedir,''), timeout=2500, log = self.log)
      except RuntimeError, e:
        raise RuntimeError('Error running make on PARTY: '+str(e))
      self.postInstall(output+err,'make.inc')
    return self.installDir
