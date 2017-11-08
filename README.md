# Icecream Setup for Mac OS X (Mozilla Toronto Office)

Want to build a clobber of Firefox in 5 minutes on your Mac laptop in the
Toronto office? Here are the instructions for you!

## Instructions

Install the prerequisites (I recommend using homebrew)

```bash
$ xcode-select --install
$ brew install lzo
```

Clone this repository, this will take a while because it contains a binary copy
of a clang toolchain (which is a gross thing to keep in git, I know)

```bash
$ git clone https://github.com/mstange/icecc-osx-moztor ~/icecream
```

Install the daemon. This will create a launchd plist which will be run on startup.
The argument passed to the `install.sh` script is the scheduler to connect to.

```bash
$ sudo ~/icecream/install.sh 10.242.24.68
```

Update your mozconfig to point cc and c++ to the compiler wrappers

```bash
CC="$HOME/icecream/cc"
CXX="$HOME/icecream/c++"
```

Build firefox with sufficient jobs to saturate your network! (NOTE: If you get
failures due to posix_spawn failing, use a lower job count, as your computer is
failing to create more processes).

```bash
$ ./mach build -j100
```

## Important Notes

Use a _wired_ connection when building with icecream. Performance over WiFi is
not nearly as good as performance over a wired LAN connection. On the same note,
I also discourage you from building over the VPN due to latency issues.

If your builds seem to be hanging without building, you can try restarting your
`iceccd`. The easiest way to do that is to find it in Activity Monitor and kill 
it, the `launchd` should restart it automatically.

## Using with non-firefox builds

Setting the `CC` and `CXX` environment variables like in the mozconfig will
probably do the right thing for just about any build system. Just make sure to
set the job count high enough.

## Work in Progress

Currently the version of `iceccd` running locally doesn't dispatch objective-c
files to the network, instead building them locally. This greatly slows down the
build, at least partially because many of the jobs are taken up with waiting
objective-c builds. To fix this, the icecream daemons on every computer in the
office will need to be patched with Benoit's patch, but this hasn't been done
yet.

## How do I watch it work?

Either ask someone on a linux computer to run `icemon` to get pretty graphics,
or `telnet` into the server,

```bash
$ telnet 10.242.24.68 8766
```

## Updating the clang bundle

The clang bundle in this repo was built from llvm svn revision 317612.

Here are the steps you need to follow if you want to update the bundle to a
different revision:

You need both a Linux machine and a macOS machine in order to prepare a clang bundle.
That's because you want to have one build of clang that can run both on your own
machine (which is macOS) and one which can be distributed to the icecc worker machines
(which run Linux). These need to produce the same code, so they should be built
from the same llvm revision.

First, prepare the Linux compiler bundle:

1. [Clone the llvm repo](http://clang.llvm.org/get_started.html) on Linux using `svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm`,
   or `svn up` your existing clone.
2. You also need clone or update the clang subrepo at `llvm/tools/clang`:

   ```
   cd llvm/tools
   svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
   cd ../..
   ```

3. Build clang. This should be done in a `build` directory next to the `llvm`
   directory, using CMake.
   Here are the commands I used to do this. I'm specifying `~/code/clang_darwin_on_linux/ `
   as the install path. I also have icecream compiler wrapper scripts in `~/.bin/`,
   so I'm using an existing icecream setup on the Linux machine to compile clang.

    ```
    mkdir build
    cd build
    CC=~/.bin/gcc CXX=~/.bin/g++ cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-apple-darwin16.0.0 -DCMAKE_INSTALL_PREFIX:PATH=~/code/clang_darwin_on_linux/ ../llvm
    ninja -j100 && ninja install
    ```

4. Create the `clang_darwin_on_linux` package using the `icecc-create-env` tool from the [mozilla-osx branch of Benoit's icecream repo](https://github.com/bgirard/icecream/):

   1. Prepare the `icecc-create-env` tool (you only need to do this the first time you follow these steps):

      ```
      git clone -b mozilla-osx https://github.com/bgirard/icecream/
      cd icecream/
      ./autogen.sh
      CC=~/.bin/gcc CXX=~/.bin/g++ ./configure
      make -j100
      chmod +x client/icecc-create-env
      ```

   2. Create the package:

      ```
      ./client/icecc-create-env --clang ~/code/clang_darwin_on_linux/bin/clang $PWD/compilerwrapper/compilerwrapper
      ```
      This will create a file named `somelonghash.tar.gz` in the current directory.

5. Transfer the `somelonghash.tar.gz ` file to your Mac and save it in the root directory of this repository, right next to this readme file. Keep the filename that `icecc-create-env` chose, **do not rename the file to the same name as the old, existing package**. (The filename is used as a disambiguation key by icecream. If there exist different toolchains with the same name, then they will collide on the builders.)
6. Adjust the `cc` and `c++` wrapper scripts in the same directory to point to the new file.
7. Run `svn info` in your llvm clone on Linux and note the revision number.

Now it's time to compile clang for macOS.

1. Clone llvm on macOS and `svn up -r ...` to the same revision as on Linux:

   ```
   svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
   cd llvm/
   svn up -r therevision
   cd ..
   ```

2. You also need the subrepos `llvm/tools/clang` and `llvm/projects/libcxx`. (The latter was not needed on Linux.)

   ```
   cd llvm/tools/
   svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
   cd clang
   svn up -r therevision
   cd ../../projects/
   svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
   cd libcxx
   svn up -r therevision
   cd ../../..
   ```

3. Build clang on macOS. I used these commands (which will produce a clang installation at `~/code/clang_darwin_on_darwin/`):

   ```
   mkdir build
   cd build
   cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-apple-darwin16.0.0 -DDEFAULT_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/ -DCMAKE_INSTALL_PREFIX:PATH=~/code/clang_darwin_on_darwin/ ../llvm
   ninja -j8 && ninja install
   ```

4. Replace the `clang_darwin_on_darwin` directory in this directory with the one that the previous build command produced in `~/code/`.

This concludes the update. You can now adjust the llvm revision mentioned in this readme (at the start of this section), and commit and push your changes.
