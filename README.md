Xania MUD source ![C/C++ CI](https://github.com/mattgodbolt/xania/workflows/C/C++%20CI/badge.svg)
----------------

Found on @mattgodbolt's hard disk, and resurrected. Shows the code behind a
UK-based Multi-User Dungeon that my university housemates and I ran, built and
administrated in late 1990s.

## Discord

We chat about the mud [here](https://discord.gg/Xsstufyt8t). Feel free to drop in.

## Building and running

If you want to build and run Xania locally, you'll need a modern Linux (Ubuntu 18.04 and 20.04 have been tested). You'll
need the following packages to build:

```bash
$ sudo apt install git make cmake gcc g++ curl
```

It's pretty likely you have all these already. The hope is that almost all the upstream dependencies of the code are
downloaded and managed by the master `Makefile`.

To build and run, type `make`:

```bash
$ make start
... lots of configuration nonsense here, downloads, cmake, and then compilation ...
All being well, telnet localhost 9000 to log in
$
```


The MUD is now running in the background, on port 9000. `telnet localhost 9000` should get you the logon prompt. Create
a character, and log in...and have fun!

If you want to build but not start it up, simply run `make`.

By default a debuggable unoptimized build is run. Use `make BUILD_TYPE=release` to build the optimized version.

### Stopping and Restart the MUD

To stop Xania, run `make stop`. To restart,  `make restart`.

### Creating an immortal account

To administrate Xania from inside, you'll need a level 100 character. Create a character as above, then `quit`. Edit
the `player/Yourplayername` file and edit the line with `Levl 1` to be `Levl 100`. Then log back in and voila! You are
an immortal!

## Developing

The top-level `Makefile` is more of a convenience layer to set up a consistent environment. It delegates the actual
building to a `cmake`-built system in one of the `cmake-build-{type}` directories (e.g. `cmake-build-debug`). A tiny
amount of back-and-forth between the two systems ensures the `cmake` build system uses the various tools installed
locally in `.tools/` to do work (e.g. `conan` for C++ packages, `clang-format` for code formatting, etc).  The top-level
`Makefile` will configure `cmake` to use `ninja` if you have that installed, else it'll use `make`.

For day-to-day developing you can use your favourite IDE or editor (CLion, vs.code, vi, emacs). If your IDE supports
CMake projects, then it might Just Work out of the box. CLion, for example, will open up and run the code from the IDE.

The mud happily runs without being installed, however it does rely on a few environment variables for configuration 
(see below).

There are two components, `doorman` and `xania`. The former is the TCP side of things, and unless you're changing how
the MUD and the connection process communicate you can probably build and run `doorman` and then leave it running.
`xania` is the MUD part, and so if you're debugging and building, that's probably the target to run.

We use `clang-format` to format all the code consistenly, and a `.clang-format` file is in the root. Again, most IDEs
will pick up on this and just use it. Command-line users can run `make reformat-code` to reformat everything, or
`make check-format` to just see if everything's formatted as it should be. At some point we'll make this part of the CI
process.

### Environment Variables

The mud processes use some environment variables for configuration. Out of the box, `make start` will set reasonable defaults
for these. The default settings are stored in `mud-settings-dev.sh`, and `make start` will read this file.

If you are doing development and launching the processes from an IDE then you prefer to configure these settings separately.
The project includes an example VS Code `launch.json` that sets these.

- `MUD_AREA_DIR`:  Static game database files.
- `MUD_DATA_DIR`:  The base directory of all runtime data. The mud uses these subdirectories:
   - player/ (player character files)
   - gods/ (configuration for deities)
   - system/ (other player generated data e.g. ban lists and bug lists)
   - log/ (the mud processes are unaware of this as log output is redirected from to stdout & stderr)
- `MUD_HTML_DIR`:  Static and dynamically generated HTML.
- `MUD_PORT`:  The TCP port doorman listens on for telnet connections. Default: `9000`.

If you are running either process _directly_ from a shell rather than using `make start` or a launch target in your IDE,
there is also a helper script `mud-settings-dev.sh`.  Source this file from one of your shells then run the process e.g.
after running the build:
```
$ . mud-settings-dev.sh
$ ./install/bin/doorman
```

### Tests

There are some very early tests. To run the tests, use `make test`, or run them from your IDE.

### Debugging Xania in GDB

There's more than one way to do this, and one way is:  after building Xania you can run 
it like so (use of `gdb` is optional):

```bash
$ gdb install/bin/xania
(gdb) r   # to run
```

You can also attach to the running process using gdb -p.

If you're relying on the default `make` targets to build & start things, the `reboot` command from within the MUD will
not restart the processes.

A solution is to not run `make start` at all, but instead, execute `./install/bin/doorman` and
the xania executable directly in another terminal as described above.

### Going "live"

When the MUD is actually deployed, we `make install` to get a built setup with `area`s and binaries etc. That's tar'd
up and copied to a host to run. `player` files and whatnot are managed separately. If you're curious how we're sketching
out ideas for the deployment, check out the `infra/` directory. It's a bit wild in there right now though.

# License

See the [LICENSE](LICENSE) file for more details, but this code is built on and
extends code from  [Rom2.4](http://web.archive.org/web/20000818050433/http://www.hypercube.org/tess/rom/) code.

Xania's log on banner contains a lot of the required info. As you may not wish
to log in, it is:

```
.   .... NO! ...                  ... MNO! ...
.... MNO!! ...................... MNNOO! ..    /    \    /    \
.... MMNO! ......................... MNNOO!!   \     \  /     /
.. MNOONNOO!   MMMMMMMMMMPPPOII!   MNNO!!!!     \     \/     /
.. !O! NNO! MMMMMMMMMMMMMPPPOOOII!! NO! ...      \          /  A N I A
  ...... ! MMMMMMMMMMMMMPPPPOOOOIII! ! ...       /    /\    \
 ........ MMMMMMMMMMMMPPPPPOOOOOOII!! .....     /    /  \    \
 ........ MMMMMOOOOOOPPPPPPPPOOOOMII! ...      /    /    \    \
  ....... MMMMM..    OPPMMP    .,OMI! ....     \   /      \   /
   ...... MMMM::   o.,OPMP,.o   ::I!! ... 
       .... NNM:::.,,OOPM!P,.::::!! ....    ...where it is recognised that 
       .. MMNNNNNOOOOPMO!!IIPPO!!O! .....   shaking hands with Death brings
       ... MMMMMNNNNOO:!!:!!IPPPPOO! ....  longevity and considerable kudos.
         .. MMMMMNNOOMMNNIIIPPPOO!! ......
        ...... MMMONNMMNNNIIIOO!..........
     ....... MN MOMMMNNNIIIIIO! OO .......... DikuMUD by Hans Staerfeldt
  ......... MNO! IiiiiiiiiiiiI OOOO ......... Katja Nyboe, Tom Madsen, Michael
 ..... NNN.MNO! . O!!!!!!!!!O . OONO NO! .... Seifert, and Sebastian Hammer.
 .... MNNNNNO! ...OOOOOOOOOOO .  MMNNON!..... Based on MERC 2.2 code
 ...... MNNNNO! .. PPPPPPPPP .. MMNON!....... by Hatchet, Furey, and Kahn,
    ...... OO! ................. ON! .......  ROM 2.4 (C) 1993-96 Russ Taylor
       ................................       MUDdled by the Xanian Immortals
```

I (@mattgodbolt) believe the code is free to open source, but of course if you
have any issues don't hesitate to contact me.

## Thanks to

The Xania implementors were avid players of [MadROM](http://madrom.net/), and were inspired to get the ROM code and
start hacking on their own MUD. In particular thanks to Etaine, Ozymandius, Neuromancer, Amiga and Crash. Sorry if we
forgot someone!

In the MUD itself, the thanks read:

```
Xania was created by The Moog, Death, Faramir, Rohan  and Wandera,
five bold Gods who saw fit to create a new world where
peace and happiness would reign. Sadly, (due to  sheer incompetence)
they made a nasty mistake somewhere along the line, and an infestation
of evil spread chaotically throughout  the realm. Thus, this place is
no longer safe and it is up to you, bold adventurer, to face up to the
prospect of meeting an untimely demise!

A special thank you to the original immortals and heroes who contributed
to Xania's success, you can see their names using the 'wizlist' command.

And it goes without saying, Xania would have been nothing without the base
code and areas that shipped with Rivers of Mud (ROM), its sister mud, MadROM,
and before that, DikuMUD and MERC. We owe a debt of gratitude to the original
developers and zonesmiths.
```