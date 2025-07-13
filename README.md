**Tantrum Zentrum VJ**

*vibe coded with Claude AI*

I just wanted a very simple way to queue up video clips using MIDI messages sent from the Akai MPC Live II.

**Installation**

1. Download or clone the repository
2. `cd /path/to/folder/build`
3. `rm -rf *`
4. `cmake ..`
5. `make`
6. `cd ../`

**How to use**

1. put your MP4s in the `videos` folder.
2. list the clips and their stop/start note numbers in `data/clips.csv`
3. connect your sequencer/keyboard to the laptop via a MIDI interface
4. read the help with `/path/to/folder/build/vj-app --help`
5. run the program with `/path/to/folder/build/vj-app`
6. start playing your MIDI notes and the videos will play on the laptop
