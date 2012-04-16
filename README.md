ETrun
=====

ETrun is a Wolfenstein: Enemy Territory game modification based on ET-GPL.
The objective of this mod is to bring timeruns support to it.
ETrun is currently under development.

**Visit www.timeruns.net for more information.**

Features
--------

* Instant players spawn
* No warmup, no debriefing, no timelimit
* Unlimited stamina
* Many bugfixes and exploit fixes
* Introduced ETrun cvars (see https://github.com/boutetnico/ETrun/wiki/ETrun-cvars)
* Ghost players (no collision between players)

Bootstrapping
-------------

On Linux, make sure you have the following packages installed:

* gcc
* autoconf
* automake
* libtool
* gcc-mingw32 (needed if you want to compile for Windows)

Then, run the bootstrap script:

<pre>
$ ./bootstrap
</pre>

Compiling for Windows
---------------------

On Windows: use MSVC compiler.

On Linux, run:

<pre>
$ ./windows_compile.sh
</pre>

Compiling for Linux
-------------------

On Windows: not supported.

On Linux, run:

<pre>
$ ./linux_compile.sh
</pre>

Compiling for OSX
-----------------

On OSX, run:

<pre>
$ ./osx_compile.sh
</pre>

You can also use Xcode projects located in src directory.