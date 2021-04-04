# AUTO7P-BPQ

Auto7P server for LinBPQ 

Requires a BPQ export and the reqmulti.sh scraper, which launches the multi.c once compiled.

 Author       : G7TAJ@GB7BEX.#38.GBR.EU (Steve)

 Install in a directory off the BASE_DIR (e.g. /home/pi/linbpq/scripts/)

Change:
 variables in reqmulti.sh to match your system
       
 multi_bpq.cfg settings
        
 You need an export FWD in BPQMail to export P-type msgs to the below directory

 add in CRONTAB 
 e.g.
 #Check for MULTI msgs (add all multi addresses in TO field)
 0 1 * * * /home/<usr>/linbpq/scrips/reqmulti.sh > /dev/null 2>&1

 Replace /dev/null if you want to log the output (e.g. /tmp/reqmulti.log)


-----------------
To compile

   make

Copy program to the script directory mentioned earlier.


Follow the original README to create / modify some 'group' config files (test.cfg test.dat included here)
