import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.download          = ['git://https://bitbucket.org/jedbrown/tchem.git']
    self.gitcommit         = 'e1d102530f3d1165483dc1c7f9706d3802b688cd'
    self.functions         = ['TC_getSrc']
    self.includes          = ['TC_interface.h']
    self.liblist           = [['libtchem.a']]
    self.needsMath         = 1
    return

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    self.compilerFlags = framework.require('config.compilerFlags', self)
    return

  def Install(self):
    import os

    libDir         = os.path.join(self.installDir, 'lib')
    includeDir     = os.path.join(self.installDir, 'include')

    args = []
    self.framework.pushLanguage('C')
    args.append('CC="'+self.framework.getCompiler()+'"')
    args.append('CFLAGS="'+self.framework.getCompilerFlags()+'"')
    self.framework.popLanguage()
    if hasattr(self.compilers, 'CXX'):
      self.framework.pushLanguage('Cxx')
      args.append('CXX="'+self.framework.getCompiler()+'"')
      args.append('CXXFLAGS="'+self.framework.getCompilerFlags()+'"')
      self.framework.popLanguage()
    args = '\n'.join(args)

    conffile = os.path.join(self.packageDir, self.package)
    fd = open(conffile, 'w')
    fd.write(args)
    fd.close()

    if self.installNeeded(conffile):
      try:
        self.logPrintBox('Configuring TChem')
        output1,err1,ret1  = config.package.Package.executeShellCommand('cd '+self.packageDir+' && ./configure '+args, timeout=300, log = self.log)
      except RuntimeError, e:
        raise RuntimeError('Error running configure on TChem: '+str(e))
      try:
        self.logPrintBox('Compiling TChem; this may take several minutes')
        output2,err2,ret2  = config.package.Package.executeShellCommand('cd '+self.packageDir+' && make && cp include/TC_*.h %(includeDir)s && cp lib/libtchem* %(libDir)s' % dict(includeDir=includeDir,libDir=libDir), timeout=500, log = self.log)
      except RuntimeError, e:
        raise RuntimeError('Error running make on TChem: '+str(e))
      self.postInstall(output1+err1+output2+err2,'tchem')
    return self.installDir
