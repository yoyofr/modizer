# How to package gbsplay

Dear package maintainer,

thank you for packaging gbsplay!

This document should help you in your packaging work by describing
some internals of our build system.

## Version numbers

Our `configure` script tries to automatically guess the current
version number for any build.  We do it this way, so we don't have to
manually increment a version number on every commit.

The `configure` script contains the default version number which can
be queried like this:

```sh
grep ^VERSION= configure
```

A value of `unknown` will trigger a heuristic to guess the current
version when `./configure` is run.

The actual version number used for the build after running
`./configure` can be queried like this:

```sh
grep ^VERSION config.mk
```

After the gbsplay binary is built, its version can be queried like
this:

```sh
gbsplay -V
```

### Version number from release tarball

If you create a package based on our official tarball, the default
version number will be set correctly and no version detection will
happen.  This will automatically create a correct version number in
the build.

E.g. when doing Debian-style packaging, you would do something like
this:

```sh
mv gbsplay-0.0.99.tar.gz gbsplay_0.0.99.orig.tar.gz
```

The release tarball is the _.tar.gz asset containing the version
number in the filename_ on the GitHub release page.

PLEASE NOTE: The GitHub release page also automatically includes
unversioned `.zip` and `.tar.gz` assets named _Source Code_, but these
files neither include a default version number nor the git metadata to
determine a version number, so you will run into the case _Version
number fallback_ described below. Your version number will look
something like `0.0.99ish` and that's not good.

#### Creating a non–release tarball for an intermediate commit

If you need to include a bugfix that has not yet been released as a
proper gbsplay version, you can just create a release tarball by
yourself:

```sh
git clone https://github.com/mmitch/gbsplay.git
( cd gbsplay; make dist )
ls -l gbsplay-*.tar.gz
```

Use `git checkout <commit>` if you want to package something other
than the current HEAD.

The created tarball (e.g. `gbsplay-0.0.99-1-ge306a0d.tar.gz`) will
have a proper version number default and can be used as your (fake)
upstream package.  We're okay with that if the commit used is from our
official git repository.

### Version number from git (release or intermediate commit)

When a build is run from a full git checkout (including metadata like
git tags; the `.git` subdirectory is present), the `configure` script
will set the version number based on the git metadata:

- If you are on a tagged commit (e.g. `0.0.99`), the version number
  will be set to that commit (e.g. `0.0.99`).

- If you are on a non-tagged commit (e.g. commit `207fb62f`), the
  version number will include both the latest previous tag plus the current
  commit on top of that (e.g. `0.0.99-3-g207fb62`, see _git-describe(1)_).

- If the git metadata is incomplete, several fallbacks are applied.  A
  possible outcome might look something like `0.0.99ish-git-207fb62f`.

While this is mainly provided for users that regularly compile gbsplay
from the latest sources, this can also be used for package builds if
you clone the gbsplay git repository and do a build inside the cloned repo.

Please note that if you add additional tags to the git repo to tag
your package versions, they will be picked up by the configure script
as well and might lead to a version number like
`0.0.99-mydistro-2025.05-1`.  
We are currently undecided whether that's a bug or a feature :)  
It should be fine as long as it's clear which source code commit
relates to the build (e.g. when including the output of `gbsplay -V`
for a bug report).

### Version number fallback

When neither a default version number is set nor a git commit or git
tag can be found, the `configure` script will read the `HISTORY` file,
look for the latest release and add `ish` to it to convey the fact
that the build was done with a source commit of the latest release _or
any later commit_.

This will create a version number like `0.0.99ish` and should be
prevented because it is unclear which commit was actually used for the
build.

## Installation target

The directories that `make install` will write to can be previewed via
`make show-install-dirs`.  The defaults are like this:

```text
$ make show-install-dirs
+ DESTDIR      = 
* bindir       = /usr/local/bin
  includedir   = /usr/local/include/libgbs
* libdir       = /usr/local/lib
* mandir       = /usr/local/man
  man1dir      = /usr/local/man/man1
  man3dir      = /usr/local/man/man3
  man5dir      = /usr/local/man/man5
* appdir       = /usr/local/share/applications
* docdir       = /usr/local/share/doc/gbsplay
  contribdir   = /usr/local/share/doc/gbsplay/contrib
  exampledir   = /usr/local/share/doc/gbsplay/examples
  localedir    = /usr/local/share/locale
                 /usr/local/share/locale/de
                 /usr/local/share/locale/en
  mimedir      = /usr/local/share/mime
                 /usr/local/share/mime/packages
  pkgconfigdir = /usr/local/share/pkgconfig
```

Note: Not all paths are used in all cases.  E.g. if you build without
`--enable-sharedlibgbs` (which is the default) then `includedir`,
`libdir` and `pkgconfigdir` will not be used.

The target directories are affected by the `--prefix` and `--eprefix`
parameters of the `configure` script.

You can override the variables marked `*` by passing parameters like
`--mandir` to `./configure`.  See `./configure --help` for details.

If you need the installation to target a temporary directory during
packaging, you can set the `DESTDIR` parameter when running `make
install`.  See this example:

```text
$ make DESTDIR=/tmp/gbsplay-build-1234 show-install-dirs
+ DESTDIR      = /tmp/gbsplay-build-1234
* bindir       = /tmp/gbsplay-build-1234/usr/local/bin
  includedir   = /tmp/gbsplay-build-1234/usr/local/include/libgbs
* libdir       = /usr/local/lib
* mandir       = /tmp/gbsplay-build-1234/usr/local/man
  man1dir      = /tmp/gbsplay-build-1234/usr/local/man/man1
  man3dir      = /tmp/gbsplay-build-1234/usr/local/man/man3
  man5dir      = /tmp/gbsplay-build-1234/usr/local/man/man5
* appdir       = /tmp/gbsplay-build-1234/usr/local/share/applications
* docdir       = /tmp/gbsplay-build-1234/usr/local/share/doc/gbsplay
  contribdir   = /tmp/gbsplay-build-1234/usr/local/share/doc/gbsplay/contrib
  exampledir   = /tmp/gbsplay-build-1234/usr/local/share/doc/gbsplay/examples
  localedir    = /tmp/gbsplay-build-1234/usr/local/share/locale
                 /tmp/gbsplay-build-1234/usr/local/share/locale/de
                 /tmp/gbsplay-build-1234/usr/local/share/locale/en
  mimedir      = /tmp/gbsplay-build-1234/usr/local/share/mime
                 /tmp/gbsplay-build-1234/usr/local/share/mime/packages
  pkgconfigdir = /tmp/gbsplay-build-1234/usr/local/share/pkgconfig
```

The `DESTDIR` is only taken into account when copying the files during
installation.  Generated files like `libgbs.pc` (for pkgconfig) will still
reference any paths _without_ `DESTDIR`.
