#!/bin/bash
#
##################################################################
#
# File         : reqmulti.sh (you need MULTI exec. as well)
# Descriptipon : Multi Server export grabber for LinBPQ
#		 This rips the messages exported by fwding and runs the multi server executable
#
# Author       : G7TAJ@GB7BEX.#38.GBR.EU (Steve)
#
# Install in a directory off the BASE_DIR (e.g. /home/pi/linbpq/scripts/)
# Change variables to match your system
#
# You need an export FWD in BPQMail to export P-type msgs to the below directory
#
# add in CRONTAB before you call
# e.g.
# #Check for MULTI msgs (add all multi addresses in TO field)
# 0 1 * * * /home/<usr>/linbpq/scrips/reqmulti7p.sh > /dev/null 2>&1
#
# Replace /dev/null if you want to log the output (e.g. /tmp/reqmulti.log)
#
#
##################################################################


BBS=GB7BEX
HR=.#38.GBR.EU
BBSCALL=$BBS$HR

BASE_DIR=/home/taj/linbpq
CMD_BASE_DIR=$BASE_DIR/scripts/multi
LOG=multi.log
EXPORT_DIR=$BASE_DIR/Mail/Export/multi

cd $CMD_BASE_DIR

IN_MSG=0
IN_HEADER=0
FOUND_WP=0

#if no log - exit
if [ ! -f "$EXPORT_DIR/$LOG" ]; then
#  echo "no file..."
 exit 1;
fi

total=""
firstline=1

while read -r line; do
  first2=${line:0:2}
  first3=${line:0:3}

  if [ IN_MSG ]; then
    total="${total}\n${line}"
  fi

 if [[ $filenext -eq 1 ]]; then
        filenext=0
        parts=($(echo $line | tr " " "\n"))

	FILE=${parts[0]}
	FROMBBS="@${parts[2]}"
	FROMBBS="${FROMBBS/$'\r'/}"  # strip CR
  fi



   if [ "$first2" == "R:" ] && [ $IN_MSG -eq 1 ]; then
	IN_HEADER=1
	LAST_R=$line
   fi


   if [ "$first2" != "R:" ] && [ $IN_HEADER -eq 1 ]; then  # we're out of the R lines so process the last R:
        IN_HEADER=0
	FOUND_WP=1
   fi


   if [ "$first3" == "/EX" ] && [ $IN_MSG -eq 1 ]; then  # END of message so process
	IN_MSG=0
	echo -e "${total}" > out.tmp

        RUN_CMD="$CMD_BASE_DIR/multi out.tmp"
	echo $RUN_CMD

	$RUN_CMD

	FROM_CALL=""
	FROMBBS=""
	FOUND_WP=0
	total=""
	rm out.tmp
   fi


   if [ "$first3" == "SP " ] && [ $IN_MSG -eq 0 ]; then

        total=${line}

        parts=($(echo $line | tr " " "\n"))
        IFS=". " read -r -a bbsarr <<< ${parts[3]}

        echo -n "Is this to our BBS? -> "

        if [ "${bbsarr[0]}" == $BBS ] || [ "${parts[2]}" == "<" ]; then
                FROM_CALL=${parts[5]}	   # if line = SP AUTO7P @ BBSCALL < CALLSIGN
		if [ "$FROM_CALL" == "" ]; then
	            FROM_CALL=${parts[3]}  # if line = SP AUTO7P < CALLSIGN
		fi
                echo "YES"

		#strip any @addr that might be there
		FROM_CALL=($(echo $FROM_CALL | tr "@" "\n"))

                echo "From - $FROM_CALL"
                filenext=1
                IN_MSG=1
                FOUND_WP=0
		FILE=""
        else
                echo "NO - skipping message."
        fi
   fi



done < "$EXPORT_DIR/$LOG"

rm "$EXPORT_DIR/$LOG"
