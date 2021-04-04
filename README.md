# AUTO7P-BPQ

Auto7P server for LinBPQ 

Requires a BPQ export and the reqmulti.sh scraper, which launches the multi.c once compiled.


 File         : reqmulti.sh (you need MULTI exec. as well)
 Descriptipon : Multi Server export grabber for LinBPQ
                This rips the messages exported by fwding and runs the multi server executable

 Author       : G7TAJ@GB7BEX.#38.GBR.EU (Steve)

 Install in a directory off the BASE_DIR (e.g. /home/pi/linbpq/scripts/)
 Change variables to match your system

 You need an export FWD in BPQMail to export P-type msgs to the below directory

 add in CRONTAB before you call
 e.g.
 #Check for MULTI msgs (add all multi addresses in TO field)
 0 1 * * * /home/<usr>/linbpq/scrips/reqmulti.sh > /dev/null 2>&1

 Replace /dev/null if you want to log the output (e.g. /tmp/reqmulti.log)
