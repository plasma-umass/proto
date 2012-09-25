# Proto
Probabilistic Race Tolerance

## Building
Proto currently only supports 32-bit x86 on Linux.  To build the 32-bit library and tests, invoke `make CPU=i386` at the project root.

## Testing
Running tests will first build Proto.  To run tests, type `make test CPU=i386` at the root of the project.

To add new tests, duplicate the Makefile and source structure from tests/sample.  Add the new test directory name to the `DIRS` variable in tests/Makefile.
