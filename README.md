# mini-lspci

mini-lspci is a simple and small replacement for lspci from pciutils.

## Usage

This program currently only supports getting devices from sysfs or some other sysfs-esque directory tree, which is supported on Linux, Managarm, and possibly other systems.

By default, it looks for the `pci.ids` file in `/usr/share/hwdata/pci.ids` (the same location as pciutils' lspci).
If no such file is found or it's missing entries, devices may not have any names associated with them and instead only have their ids displayed.

Usage:
```
        -v               be verbose (show subsystem information)
        -n               show only numeric values
        -nn              show numeric values and names
        -p <path>        set pci.ids path (default: /usr/share/hwdata/pci.ids)
        -V               show version
        -h               show help
```

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

## License

This project is licensed under the Zlib license.
