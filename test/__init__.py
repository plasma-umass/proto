import os
import shutil
import sys
import test.util as util


# CLEAN       is the equivalent of running `make clean`.
# RESET       establishes a new source tree, repatching and reconfiguring.
# CACHE_CLEAN removes source tarballs.
# DIST_CLEAN  removes all source trees and downloaded data

CLEAN, RESET, CACHE_CLEAN, DIST_CLEAN = range(4)
OPT_0, OPT_1, OPT_2, OPT_3            = range(4)

default_opts = [(OPT_0, True), (OPT_2, False)]

class IntegrationTest(object):
  def __init__(self, arches, verbose, opts=default_opts):
    self.verbose = verbose
    self.arches = arches
    self.opts = opts
    self.local_tar = self.cwd(self.local_tar)

  def clean(self, level):
    pass

  def cwd(self, *args):
    return os.path.join(os.path.abspath(os.curdir),
                        'test',
                        'integration',
                        self.name(),
                        *args)
  
  def ref_dir(self, arch, opt, *args):
    return self.cwd('%s_%s_opt%d_ref' % (self.name(), arch, opt), *args)

  def proto_dir(self, arch, opt, *args):
    return self.cwd('%s_%s_opt%d_proto' % (self.name(), arch, opt), *args)

  def config(self):
    util.mkdir_p(self.cwd())

  def fetch(self):
    pass


class TarballTest(IntegrationTest):
  def __init__(self, arches, verbose):
    super(TarballTest,self).__init__(arches, verbose)

  def clean(self, level):
    if level == CACHE_CLEAN or level == DIST_CLEAN:
      if os.path.exists(self.local_tar):
        os.remove(self.local_tar)
      if level == CACHE_CLEAN:
        return
    for arch in self.arches:
      for opt,debug in self.opts:
        if level == CLEAN:
          if os.path.exists(self.ref_dir(arch, opt, self.make_dir)):
            util.make(self.ref_dir(arch, opt, make_dir), 'clean')
          if os.path.exists(self.proto_dir(arch, opt, self.make_dir)):
            util.make(self.proto_dir(arch, opt, make_dir), 'clean')
        else:
          if os.path.exists(self.ref_dir(arch, opt)):
            shutil.rmtree(self.ref_dir(arch, opt))
          if os.path.exists(self.proto_dir(arch, opt)):
            shutil.rmtree(self.proto_dir(arch, opt))
 
  def config(self):
    super(TarballTest, self).config()
    util.download_to(self.tar_url, self.local_tar, self.tar_md5, verbose=self.verbose)
    for arch in self.arches:
      for opt,debug in self.opts:
        if not os.path.exists(self.ref_dir(arch, opt)):
          util.untar_to(self.local_tar, self.ref_dir(arch, opt), verbose=self.verbose)
          util.patch(self.patch(arch, opt, debug, False), self.ref_dir(arch, opt), 1, verbose=self.verbose)
        if not os.path.exists(self.proto_dir(arch, opt)):
          util.untar_to(self.local_tar, self.proto_dir(arch, opt), verbose=self.verbose)
          util.patch(self.patch(arch, opt, debug, True), self.proto_dir(arch, opt), 1, verbose=self.verbose)

  def build_dir(self, build_dir, force_build=False):
    bin_file = os.path.join(build_dir, self.bin_name)
    if force_build or not os.path.exists(bin_file):
      util.make(build_dir, *self.make_args)
  
  def build(self, force_build=False):
    for arch in self.arches:
      for opt, debug in self.opts:
        self.build_dir(self.ref_dir(arch, opt, self.make_dir), force_build)
        assert not util.bin_links_to(self.ref_dir(arch, opt, self.make_dir, self.bin_name),
                                     os.path.join(os.environ['PROTO_LIB_DIR'],
                                                  'libproto.so'))
        self.build_dir(self.proto_dir(arch, opt, self.make_dir), force_build)
        assert util.bin_links_to(self.proto_dir(arch, opt, self.make_dir, self.bin_name),
                                 os.path.join(os.environ['PROTO_LIB_DIR'],
                                              'libproto.so'))

