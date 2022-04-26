# GeoCoding-Datalogger
Adafruit feather/wing based GPS and weather logger for geocoding photographs

Uploading this in very raw state:

using adafruit gps module, datalogger module, and oled screen

button B speeds up logging interval, B slows it down, A, pondering using to set the timezone

this code is a bit of a mess, but I wanted to check it in to version it, but it works!

There is are things I want to harden:

managing the SD card inserts and removals, I havent made sure the file is closed and reopened (or a new one is made)

making sure I have the right size "ints" because if this runs for a while, it will flood normal ints, I fear.

power mgmt, I keep wondering if I need to sleep this thing and slow the logging way down, but so far I can run it for days on a usb battery pack, so havent tinkered with this, but it would be cool to optimize it

the file output works in some geocoding apps, but now houdahGeo, which is the app I have used for a long time, for some reason the libraty they use understands my output file, but the app doesnt, working on that one

some might be thinking...but you can do this on your phone! well yes, but if you look in the code I have a temp sensor, why I made this is basically to log other variables with the lat/lon/alt. I want to note the Wx as I hike, ski, etc... I plan on adding more sensors as I think about what might be interesting to note.

This is based on the Adafruit code/libs, and all props to them for their amazing work!
