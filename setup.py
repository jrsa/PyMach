#!/usr/bin/env python3
from distutils.core import setup, Extension

mach = Extension('mach', sources = ['mach.c', 'attach.c'])

setup(name = 'mach',
      version = '3.0',
      description = 'Wrap some low-level Mach stuff for Python 3',
      author='Pedro José Pereira Vieito',
      author_email='pvieito@gmail.com',
      ext_modules = [mach]
      )
