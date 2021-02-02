# amake
Simple _Makefile_ generator based on single source directory.

## How to build it
Invoke ```make``` or ```make release```

## How to use it
```
amake - Simple Makefile generator
	(C) Emanuele Oriani 2021

Usage: amake [--help] [-src SRCDIR] [-o OUTNAME] [gcc/g++ options] [gcc/g++ libraries]
	--help : display this help
	-src : set the sources and headers directory
	-o : set the name for the compiled file
	gcc/g++ options : set any option you want to pass to the compiler
	gcc/g++ libraries : set any library you want to pass to the compiler
```
