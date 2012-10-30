#!/usr/bin/env python3

import sys
from optparse import OptionParser
import os
import subprocess
from traceback import format_exc
import test
from test import util

default_tests = [
    'test.integration.phoenix.phoenix',
    'test.integration.pfscan.pfscan',
    'test.integration.pbzip2.pbzip2',
]

clean_pyc_dirs = [
    'test',
    'test/integration',
]

def handle_failure(test, exception=None):
  if not exception:
    sys.stderr.write("\nTest %s failed.\n" % test.name())
  else:
    if isinstance(exception, util.TestFailure):
      exc_str = str(exception)
    else:
      exc_str = format_exc(exception)
    sys.stderr.write("\nTest %s failed: %s\n" % (test.name(), exc_str))

def test_generator(tests, arches, verbose):
  for test_path in tests:
    last_sep = test_path.rfind('.')
    if last_sep != -1:
      test_mod_name = test_path[:last_sep]
      test_class_name = test_path[last_sep+1:]
      __import__(test_mod_name)
      test_mod = sys.modules[test_mod_name]
      test_class = getattr(test_mod, test_class_name)
    else:
      test_class = globals()[test_path]
    yield test_class(arches, verbose)

def run_tests(tests,
              verbose,
              continue_failure,
              control):
  for test in test_generator(tests, util.get_arches(), verbose):
    try:
      test.config()
      test.build(force_build=False)
      test.run(control=control)
    except Exception as e:
      handle_failure(test, e)
      if not continue_failure:
        break

def clean(tests,
          continue_failure,
          verbose,
          level):
  for test_suite in test_generator(tests, util.get_arches(), verbose):
    try:
      test_suite.clean(level)
    except Exception as e:
      handle_failure(test_suite, e)
      if not continue_failure:
        break
  if level != test.CACHE_CLEAN:
# clean up pyc files, if they are there and the .py file is also there.
    for dir in clean_pyc_dirs:
      for file in dir:
        abs_file = os.path.join(os.path.abspath(os.curdir), dir, file)
        if os.path.isfile(abs_file):
          (root, ext) = os.path.splitext(abs_file)
          if ext == 'pyc':
            py_file = root + '.py'
            if os.path.exists(py_file) and os.path.isfile(py_file):
              os.remove(abs_file)
  # finally make clean in the sub directory.
  util.make(os.path.join(os.path.abspath(os.curdir), 'test', 'unit'), 'clean')

def handle_run(args):
  parser = OptionParser(usage='%prog run [OPTIONS]')
  parser.add_option('-f',
                    '--continue-on-failure',
                    dest='fail',
                    action='store_true',
                    default=False,
                    help='Continue to next test on failure.')
  parser.add_option('-n',
                    '--control',
                    dest="control",
                    action="store_true",
                    default=False,
                    help='Test programs against themselves '
                         + '(to check for testing validitiy)')
  parser.add_option('-v',
                    '--verbose',
                    dest='verbose',
                    action='store_true',
                    default=False)
  parser.add_option('-t',
                    '--tests',
                    dest='tests',
                    action='store',
                    default='',
                    help='Run only specified tests, separated by comma.'
                         + 'If an empty string is specified, all tests'
                         + 'are run.')
  (options, args) = parser.parse_args(args)
  run_tests(options.tests.split(',') if options.tests else default_tests,
            options.verbose,
            options.fail,
            options.control)

def handle_clean(args):
  parser = OptionParser(usage='%prog clean [OPTIONS]')
  parser.add_option('-f',
                    '--continue-on-failure',
                    dest='fail',
                    action='store_true',
                    default=False,
                    help='Continue to next test on failure.')
  parser.add_option('-t',
                    '--tests',
                    dest='tests',
                    action='store',
                    default='',
                    help='Run only specified tests, separated by comma.'
                         + 'If an empty string is specified, all tests'
                         + 'are run.')
  parser.add_option('-l',
                    '--level',
                    dest='level',
                    action='store',
                    default='clean',
                    help='Level of clean desired:'\
 ' \'clean\': the equivalent of running `make clean` for a certain test.'\
 ' \'reset\': deletes the source trees, leading to a fresh configuration.'\
 ' \'cache-clean\': removes all downloaded tarballs and data, but not source '\
 ' trees. \'dist-clean\': does all of the above.')
  parser.add_option('-v',
                    '--verbose',
                    dest='verbose',
                    action='store_true',
                    default=False)
  (options, args) = parser.parse_args(args)
  level_trans = {
      'clean'       : test.CLEAN,
      'reset'       : test.RESET,
      'cache-clean' : test.CACHE_CLEAN,
      'cach_clean'  : test.CACHE_CLEAN,
      'dist-clean'  : test.DIST_CLEAN,
      'dist_clean'  : test.DIST_CLEAN,
  }
  clean(options.tests.split(',') if options.tests else default_tests,
        options.fail,
        options.verbose,
        level_trans[options.level.lower()])
  
def usage():
  sys.stdout.write('USAGE: %s <cmd> [options]\n' % sys.argv[0])
  cmd_len = 0
  tab_width = 4
  for (key, val) in cmds.items():
    (fnc, help) = val
    cmd_len = max(cmd_len, len(key))
  for (key, val) in cmds.items():
    (fnc, help) = val
    sys.stdout.write(' ' * tab_width)
    sys.stdout.write(key)
    sys.stdout.write(' ' * (tab_width + cmd_len - len(key)))
    sys.stdout.write(help)
    sys.stdout.write('\n')
  sys.stdout.write('\n')

def handle_unit(args):
  cwd = os.path.join(os.path.abspath(os.curdir),
                     'test',
                     'unit')
  file_name = os.path.join(cwd, 'gtest-1.6.0.zip')
  util.download_to('http://googletest.googlecode.com/files/gtest-1.6.0.zip',
                   file_name,
                   '4577b49f2973c90bf9ba69aa8166b786',
                   verbose=True)
  if not os.path.exists(os.path.join(cwd, 'gtest-1.6.0')):
    subprocess.check_call(['unzip', file_name, '-d', cwd])

  util.make(cwd)
  bin = os.path.join(cwd, 'gtest_proto')

  subprocess.check_call([bin] + args,
                        stdin = sys.stdin,
                        stdout = sys.stdout,
                        stderr = sys.stderr)

cmds = {
    'clean' : (handle_clean, 'clean testing code'),
    'itest' : (handle_run,   'run integration testing code'),
    'test'  : (handle_unit,  'run unit tests via google test'),  
    'help'  : (usage,        'this usage text'),
}

def main():
  if len(sys.argv) < 2:
    usage()
    sys.exit(0)
  cmd = sys.argv[1]
  if cmd not in cmds:
    sys.stderr.write('%s: \'%s\' is not a valid command. See %s help for usage'\
                     ' information.' % (sys.argv[0], cmd, sys.argv[0]))
    sys.exit(1)
  (fnc, help) = cmds[cmd]
  fnc(sys.argv[2:])
