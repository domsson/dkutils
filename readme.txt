Dungeon Keeper Small Utilities
------------------------------

Note: Many files in are compressed. Use dernc.exe to decompress them.
Usage: dernc <filename> [filename...]

Further note: To run these programs, go32.exe must be in the current
directory, or put it somewhere in your path.

xsfx: Sound extractor
---------------------

This extracts sounds to a series of wav files.
Usage: xsfx <filename>

The filename is either speech.dat or sound.dat. Note, *lots* of
files are created... make sure you have plenty of room on your hard
disk.

script: Level script (.txt file) editor
---------------------------------------

Slightly unfinished, this is used in conjunction with Adikted to
create levels. This part is to specify which monsters, spells, doors
etc are available. All that's missing at the moment is the gold
level. More on this when it's finished.

dktext: Text modifier
---------------------

Usage: dktext <filename>            - List all messages
       dktext <filename> v <number> - Display specified message
       dktext <filename> e <number> - Replace message
       dktext <filename> a          - Add message to end

The filename will usually be data\text.dat.

xgfx: Graphics extractor
------------------------

Run from the data directory (containing main.pal, etc). You need to
specify a data/tab file and a pal file, eg

xgfx editicn1 main

A load of .bmp files will be created in the current directory.

exanim: Animation extractor
---------------------------

Again, run from the data directory. No arguments are needed. A load
of .bmps are created along with anims.txt which shows which frames
go together.

terrain: Block extractor
------------------------

Run from the data directory, no arguments needed. 4 bmps are created.

rsfx: Sound reverser
--------------------

Usage: rsfx <filename>

This just reverses sound effects within a file (sound.dat,
speech.dat). Just run it... it will create speech.new or sound.new
depending on what you specfied. Just copy this over the original
.dat file to use it. 

Warning: You need lots of free disk space to run this program, as it
creates another file of the same size as the original.

cfg: Creature.txt editor
------------------------

Simply run cfg.exe from the command prompt, in the directory
containing creature.txt (usually the data directory).

Within menus, pressing up/down/left/right/page up/page down moves
the cursor. Return edits a value. Escape goes back to the previous
menu. 'S' saves the file (as creature.new), 'Q' quits.

To build this from source, you'll need the S-Lang libraries from
http://space.mit.edu/~davis/slang.html

Known bug: Dark Mistress has 2 secondary jobs in the original
creature.txt. My editor can't deal with this, and assumes her
secondary job is just torture. Edit the file by hand if you want to
restore the jobs.

dklevel: Level distribution formatter
-------------------------------------

If and when the business of distributing home-made levels ever takes
off, this may be useful - it is used to create a level archive from
a set of files, or extract the files back from the archive. For the
moment, you're safe to ignore this :)

--------------------------------------------------------------------

Any problems, just mail me at skeet@pobox.com.

Cheers,
Jon Skeet
