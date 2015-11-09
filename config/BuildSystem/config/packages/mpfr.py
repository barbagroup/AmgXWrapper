import config.package

class Configure(config.package.GNUPackage):
  def __init__(self, framework):
    config.package.GNUPackage.__init__(self, framework)
    self.download  = ['http://www.mpfr.org/mpfr-current/mpfr-3.1.3.tar.gz']
    self.functions = ['mpfr_get_version']
    self.includes  = ['mpfr.h']
    self.liblist   = [['libmpfr.a']]
    self.pkgname   = 'mpfr-3.1.3'
    return

  def setupDependencies(self, framework):
    config.package.GNUPackage.setupDependencies(self, framework)
    self.gmp  = framework.require('config.packages.gmp',self)
    self.deps = [self.gmp]

  def formGNUConfigureArgs(self):
    args = config.package.GNUPackage.formGNUConfigureArgs(self)
    args.append('--with-gmp='+self.gmp.getInstallDir())
    return args
