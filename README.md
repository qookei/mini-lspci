# mini-lspci

mini-lspci is a simple and small replacement for lspci from pciutils. mini-lspci aims to provide partial compatiblity with pciutils' lspci.

## Usage

Currently, mini-lspci supports the following providers:
 - sysfs - fetches device information from sysfs,
 - stdin - fetches vendor:device pairs from standard input.

This program uses the `pci.ids` file for the vendor and device name information. The default path for the file is `/usr/share/hwdata/pci.ids`. This file can be acquired from https://pci-ids.ucw.cz/ (or it's mirror: https://github.com/pciutils/pciids/).

Check `-h` for available command-line options.

## Compilation

You will need:
 - A C++20 compliant compiler
 - Meson and Ninja

To build and install mini-lspci, do the following:
```
$ meson builddir # optionally pass --prefix /.../ to set installation prefix
$ ninja -C builddir
$ ninja -C builddir install # optionally set the DESTDIR environment variable to set the system root
```

You can find the list of Meson's built-in options [here](https://mesonbuild.com/Builtin-options.html).

## License

This project is licensed under the Zlib license.
