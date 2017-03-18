[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/sop/cygscript/blob/master/LICENSE)

# Cygscript
A helper program allowing to register an extension *(eg. `.sh`)* to be
executed in [Cygwin](https://www.cygwin.com/) by double-clicking a file from
Windows explorer or dragging and dropping files to an icon of a registered extension.

## Installing from source
Install Cygwin packages:

    gcc-g++ make automake autoconf

Clone source:

    git clone https://github.com/sop/cygscript.git

Prepare environment:

    aclocal && autoheader && automake --add-missing && autoconf

Compile and install:

    ./configure && make && make install-strip

## Usage
Register `.sh` *(default)* filetype:

    cygscript -r

Default is to register only for the current user. To register for all
users, use `cygscript -ra`. Note that this prompts for an elevated process.

To unregister filetype:

    cygscript -u

To register another filetype, pass the extension as an --ext argument:

    cygscript -r --ext bash

## License
This project is licensed under the [MIT License](https://github.com/sop/cygscript/blob/master/LICENSE).
