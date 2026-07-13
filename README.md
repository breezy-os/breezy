
# Introduction

Breezy is a project with the end-goal of making self-hosting incredibly simple without sacrificing many of the conveniences that cloud services provide. At this point, it's still in the very early stages of development -- I'm primarily working on writing a custom Wayland compositor that will drive most of the user experience going forward.

The project is not currently open to pull requests, but this will almost definitely change in the future once I get more of the foundation in place. If you run into any issues or questions, feel free to email me at: `breezy@zenittini.dev`

If you'd like to follow along with Breezy's development, check out the website and accompanying video devlogs for updates and some light educational content! :)

https://breezy.zenittini.dev

# First-Time Setup

The project relies on the following libraries, some you may need to install manually through your package manager, but others might be provided in your OS already. When you run the meson compile command (later on), it will tell you which you're missing. Oftentimes, you'll need both the base package *and* the `-dev` / `-devel` package.

* **Graphics:** `libdrm` `gbm` `opengl` `mesa`
* **Input:** `udev` `libinput` `libseat` `libxkbcommon`
* **Other:** `wayland` (possibly with a `-server` and `-client` suffix)

You'll need to install the build tool, Meson (https://mesonbuild.com/). For Void Linux, it's available in the XBPS package manager:

```bash
sudo xbps-install -Su meson
```

The way Meson works is that you create a build directory, then run your compile (and/or test) commands from within that directory. All built artifacts and intermediate files are placed somewhere within that build directory. To create your build directory:

```bash
meson setup ./build
```

# Building / Running

> **NOTE:** I currently have my graphics card device hard-coded. I intend on fixing this prior to Breezy being "useful to others", but if you want to run Breezy before that point, you may need to change the `/dev/dri/card1` specified here: https://github.com/breezy-os/breezy/blob/46a6d9c8cbd96780ed619c89fe3b6c27a93c7c07/compositor/src/bz_graphics.c#L794-L801
> 
> If `/dev/dri/card1` isn't working for you (will probably crash / segfault), then a likely value would be `/dev/dri/card0`.

Once your first-time setup is complete, you can build the compositor and run the tests using the following commands:

```bash
cd ./build
meson compile
meson test
```

Prior to running Breezy, you'll need to enable the proper suppressions and settings for our sanitizers. You do this by setting the following environment variables in whatever environment you're running Breezy from. Breezy needs to be run from a Virtual Terminal (VT) separate from all other Wayland / X11 windowing systems, so that's the environment where you'll need to set these:

```bash
export LSAN_OPTIONS="suppressions=/path/to/lsan.supp"
export ASAN_OPTIONS="symbolize=1:fast_unwind_on_malloc=0"
```

To switch between VTs, on most Linux system you'd press Ctrl+Alt+F1 / F2 / F3, etc. Your current UI session is one of these (oftentimes F7, but not always).

Once you're logged in to a non-graphical VT, navigate to your Breezy git directory, then into your build directory, then run:

```bash
./compositor/breezy > ./stdout 2> ./stderr
```

This will run Breezy, sending all output to the `./build/stdout` and `./build/stderr` files. (Leave off the stdout and stderr redirection if you don't care about logs.)

# Using Breezy

As of the last time I updated this readme (July 7, 2026), Breezy doesn't have too many capabilities... Here's the complete list:

1. To quit out of Breezy, run `<Super> + <Esc>`
2. To launch the test client, run `<Super> + T`. You can launch multiple at once.
3. To quit one of the test clients, run `<Super> + Q`. If you ran multiple clients, they'll be quit in a FIFO order.

# Debugging

(Mostly for personal reference, here are a few Valgrind commands I've used when debugging various issues...)

```bash
# Basics
valgrind --tool=memcheck --leak-check=full --track-origins=yes ./compositor/breezy > ./stdout 2> stderr

# Suppression files
valgrind --tool=memcheck --leak-check=full --track-origins=yes --gen-suppressions=all ./compositor/breezy 2> leaks.txt
valgrind --tool=memcheck --leak-check=full --track-origins=yes --suppressions=../mesa.supp ./compositor/breezy > ./stdout 2> stderr
```

...and a troubleshooting command for tests that have failing assertions and therefore skip their cleanup, resulting in memory leaks in the logs rather than the actual failure:

```bash
# (swap out the test name, ofc)
ASAN_OPTIONS=detect_leaks=0 meson test bz_wl_surface
```