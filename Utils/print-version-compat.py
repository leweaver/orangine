import platform
(bits,linkage)=platform.architecture()
(major,minor,patch)=platform.python_version_tuple()
print('Python',major,bits,'OK' if int(major)==3 and int(minor)>=7 else 'TOO_OLD')