import os
import sys
import shutil
import test
from test import util


class phoenix(test.TarballTest):
  def __init__(self, arches, verbose):
    self.tar_url = 'http://mapreduce.stanford.edu/2.0.0/phoenix-2.0.0.tar.gz'
    self.local_tar = 'phoenix-2.0.0.tar.gz'
    self.tar_md5 = 'aa2e96fb3b242302762f9328fdf3272c'
    self.make_dir = 'phoenix-2.0.0'
    self.make_args = []
    super(phoenix, self).__init__(arches, verbose)

  def patch(self, arch, opt, debug, proto):
    bits = util.bits_for_arch(arch)
    diff = """--- phoenix_x86_opt0_ref/phoenix-2.0.0/Defines.mk 2009-05-27 23:00:58.000000000 -0400
              +++ phoenix_x86_opt0_proto/phoenix-2.0.0/Defines.mk 2012-10-30 10:26:13.083875191 -0400
              @@ -38,8 +38,8 @@
               OS = -D_LINUX_
               CC = gcc
               #DEBUG = -g
              -CFLAGS = -Wall $(OS) $(DEBUG) -O3
              -LIBS = -lpthread
              +CFLAGS = -Wall $(OS) $(DEBUG) -O%d%s -m%d
              +LIBS = %s -m%d
               endif
                
               ifeq ($(OSTYPE),SunOS)""" % (
           opt,
           ' -g' if debug else '',
           bits,
           '-rdynamic ${PROTO_LIB_DIR}/libproto.so -dl' if proto else '-lpthread',
           bits)
    return diff.encode('ascii')

  def build(self, force_build=True):
    pass

  def name(self):
    return 'phoenix'

  def run(self, control=False):
    pass
