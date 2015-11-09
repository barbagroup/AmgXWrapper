import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.functions     = ['PAPI_library_init']
    self.includes      = ['papi.h']
    self.liblist       = [['libpapi.a','libperfctr.a']]
