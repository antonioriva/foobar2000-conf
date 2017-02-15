fooCDtect by baralgin, ver. 2.1

fooCDtect is a front-end for aucdtect.exe by Aleksander Djuric (www.true-audio.com). 
It helps to determine the authenticity of musical CDs. FooCDtect uses converter of 
foobar2000 as a decoder and you may check any types of lossless music(and lossy as well :) ).

Switches:
--output <filename> 	temp file name (this option is necessary)
--threads <integer> 	set maximum number active threads (max 8), default [0] - unlimited
--mode <integer> 	detect mode [0..40], default 0,
		0 - slow and most accurate, 40 - fast, but less accurate
--tpath <path> 	temporary folder for wav-files (example: --tpath "d:\temp")
--priority <0|1|2|3>	checking priority (0 - use converter settings, 1 - idle, 2 - below normal, 3 - normal)

--lfor <1|2|3>	log format (1 - simple, 2 - normal, 3 - verbose)
--lenc <1|2|3>	default log encoding (1 - ansi, 2 - utf16, 3 - utf8)
--autodel 		autodelete "saved" tracks

foobar2000, converter settings:

Encoder: fooCDtect2.exe
Extension: aucdtect
Parameters(for example):
	--output %d --threads 2 --mode 0 --tpath "R:\temp" --lfor 3 --lenc 1
or simply: --output %d
Bit Depth Control: Format is lossless(or hybrid)
And don't forget to disable dither.



Change log.
2.1 - 12.09.2010
	- program SubSystem changed from Windows to Console. Now it works with foobar2000 1.1.x.
	- fixed a few interface bugs
	- project switched to VS2008 Express

2.1 beta - 25.09.2008
	- now, foocdtect window is created as a child of foobar2000.
	- switches "--topmost" and "--verlog" were removed.
	- Verbose log doesn't contain temporary file names.
added: 
	- ability to change checking priority and number of maximum threads
	(right click in the left part of the status bar).
	- three types of log format: simple, normal and verbose.
	- ability to choose ANSI, UTF16 or UTF8 encoding for log(UTF16 and UTF8 with BOM).
	- path to temporary folder for big wav-files(switch: --tpath). 
	- simple statistic window(menu File->Stats).


