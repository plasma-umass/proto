#!/usr/bin/env python3

from importlib import import_module
import platform
import sys

if __name__ == '__main__':
  (major, minor, patch) = platform.python_version_tuple()
  if int(major) < 3:
    sys.stderr.write('This requires python 3, you are running %s.%s.%s.\n' % (
                     major, minor, patch))
    sys.exit(1)
  runner = import_module('test.runner')
  runner.main()
