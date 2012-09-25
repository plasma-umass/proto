# Proto
Probabilistic Race Tolerance

## Building
Proto currently only supports 32-bit x86 on Linux.  By default, the makefile will assume CPU=i386.  This can be overridden on the command line by running `make CPU=x86_64`.

## Testing
Running tests will first build Proto.  To run tests, type `make test` at the root of the project.

To add new tests, duplicate the Makefile and source structure from tests/sample.  Add the new test directory name to the `DIRS` variable in tests/Makefile.
