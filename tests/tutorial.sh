#!/bin/sh

function pause() {
	echo "Press a key$1";
	read;
}


function cmd() {
	echo "$ $*"
	$*
}

function hr() {
	echo
	echo -e "---\t---\t---"
	echo
}



echo "First, compile script compiler..."
cmd tinyaml -c script_typeless.melang -s script.compiler

hr

echo "Testing RTC : rtc.asm"
pause "to view rtc.asm"; read; less rtc.asm
cmd tinyaml_dbg -c ../extension/RTC/RTC.lib -f -c rtc.asm -f

hr

