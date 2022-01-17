#!/usr/bin/env python

from distutils.core import setup

setup(name='oe',
      version='1.0',
      description='Python Distribution Utilities',
      author='Greg Ward',
      author_email='gward@python.net',
      url='https://www.python.org/sigs/distutils-sig/',
      packages=['oe_app', 'oe_scripting'],
      package_dir={
            'oe_app': "OeApp/scripts",
            'oe_scripting': "OeScripting/scripts",
      }
      )