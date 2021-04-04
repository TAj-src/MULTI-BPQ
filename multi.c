/*
	MUTLI Server modified by Steve - G7TAJ , for LibBPQ

originally:
 	MULTI Server for XFBB by Stewart Wilkinson G0LGS

	TODO:
		Per User Greeting in Outgoing messages.
		Duplicate checking / Loop detection.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "utils.h"

FILE *init_ptr, *msg_ptr, *dis_ptr, *out_ptr, *hlp_ptr, *cfg_ptr, *new_ptr;
char lname[10];
char owner[10];
char descr[80];
char lhelp[128];
char lhead[128];
char lfoot[128];
int  ackon = 0;
int  showdist = 1;

char buffer[255];

char datafile[10];
char sent_to[128];
char new_to[128];
char from_call[10];
char from_bbs[80];

char subject[80];
char mail_in[128];
char sys_name[20];
char sys_call[10];
char bbs_call[80];

char tocall[10];
char tobbs[80];

char dcall[10];
char dbbs[80];

char conf_path[128];
char config[128];
char text[80];

int  np1, np2;
int  fnd = 0;
int  sender_ok = 0;
int  status = 0;

time_t time_now;
struct tm *now;

#define STATUS		1
#define OWNER		2
#define DESCR		3
#define ACKON		4
#define SHOWDIST	5

#define MODE_OPEN	0
#define MODE_CLOSED	1
#define MODE_OWNER	2
#define VERS_STR	"G7TAJ MULTI SERVER V" Version " (Orig:(c) by G0LGS)\n"

#define TAIL "\n# [MULTI V" Version " " Vdate " for LinBPQ by G7TAJ (Orig:(c) by G0LGS)]\n"

#define INIT "multi_bpq.cfg"

int ReadInit()
{

	strcpy(conf_path, ".");

        #define BBS_CALL 1
        #define SYSOP_NAME 2
        #define SYSOP_CALL 3
        #define MAIL_IN 4

        int end, index;

        /* Open config file */
        init_ptr = fopen( INIT, "rt");
        if (init_ptr == NULL) {
                fprintf( stderr, "Unable to open '%s'\n", INIT );
                return 1;
        }

        /* Read the lines we need . */
        index=end=0;
        while (! end && fgets(buffer, sizeof(buffer), init_ptr) && ! feof(init_ptr) ) {

                /* Comments ignored */
                if (*buffer == '#')
                        continue;

                switch (++index) {

                        case BBS_CALL :
                                sscanf( buffer, "%s", bbs_call);
                                /* We only want up to the first '.' */
                                *strstr(bbs_call,".") = '\0';
                                break;

                        case SYSOP_NAME :
                                sscanf( buffer, "%s", sys_name);
                                break;

                        case SYSOP_CALL :
                                sscanf( buffer, "%s", sys_call);
                                break;

                        case MAIL_IN :
                                sscanf( buffer, "%s", mail_in);
                                end = 1;
                                break;
                }
        }

        fclose(init_ptr);


        // printf("cfg=\nBBS=\t%s\nSYSOP Name=\t%s\nSYSOP Call=\t%s\nMail IN=\t%s\n", bbs_call, sys_name,sys_call,mail_in);

        return 0;
}

int main(int argc, char **argv)
{
	int i;
	int end = 0;
	int msg_pos = 0;
	int dis_pos = 0;
	int index = 0;
	int Date, Time;

	bzero ( datafile, 10 );

	/* Check the Arguments */
	if (argc != 2){
		fprintf( stderr, "%s", TAIL );
		exit(1);
	}

	if( ReadInit() != 0 ){
		fprintf( stderr, "Error Processing Config file\n" );
		exit(1);
        }

	/* Open the Message */
	msg_ptr = fopen(argv[1], "r");
	if (msg_ptr == NULL){
		fprintf( stderr, "Cannot Open '%s'\n", argv[1] );
		exit(1);
	}

	/* Read the From line */

	/* BPQ line -> can be....

            SP TEST @ GB7BEX.#38.GBR.EU < G7TAJ $19075_GB7BEX
            SP TEST < G7TAJ $19078_GB7BEX

        */

	fgets(buffer, sizeof(buffer), msg_ptr);

        if ( strstr( buffer, "@" ) ) {
          //printf("Contains @\n");
          sscanf(buffer, "%*s %s %*[@] %*s %*s %s", sent_to, from_call );
        } else {
          sscanf(buffer, "%*s %s %*s %s", sent_to, from_call );
        }

	/* Read the Subject */
	fgets(buffer, sizeof(buffer), msg_ptr);
	sscanf(buffer, "%[^\n]\n", subject);

	msg_pos = ftell( msg_ptr );

	/* Process any R: line to get originating BBS & Skip T: lines */
	end=0;
	while( ! end && fgets( buffer, sizeof(buffer), msg_ptr ) && ! feof(msg_ptr) ){

		/* R: lines */
		if( sscanf( buffer,"R:%d/%d%*[^@]@%s", &Date, &Time, from_bbs ) >= 1 ) {
			msg_pos = ftell( msg_ptr );
		        if ( from_bbs[0] == ':' ) {
			         memcpy( from_bbs, from_bbs+1 , strlen(from_bbs)-1 );
			         from_bbs[ strlen(from_bbs) -1 ] = '\0';
		        }

			continue;

		/* T: lines */
		}else if( sscanf( buffer,"T:%[^\n]\n", text ) != 0 ) {
			msg_pos = ftell( msg_ptr );
			continue;

		/* Terminate on when no R: or T: line */
		} else {
			end=1;
		}

	}


	/* Close the message file  */
	fclose(msg_ptr);

   	/* We need the 'to' in lower case  */
	strcpy( datafile, sent_to );
	strlwr( datafile );

	/* Build listname from 'to' address  */
	strcpy( lname, sent_to );

	sprintf( sent_to, "%s/%s.dat", conf_path, datafile );
	sprintf( new_to, "%s/%s.new", conf_path, datafile );
	sprintf( config, "%s/%s.cfg", conf_path, datafile );
	sprintf( lhelp, "%s/%s.hlp", conf_path, datafile );
	sprintf( lhead, "%s/%s.head", conf_path, datafile );
	sprintf( lfoot, "%s/%s.foot", conf_path, datafile );

	/* Check we can open distribution file */
	dis_ptr = fopen( sent_to, "r" );
        if( dis_ptr == NULL ) {
		fprintf( stderr, "Cannot Open '%s'\n", sent_to );
		exit(1);
	}
	fclose( dis_ptr );

	/* Open config file */
	cfg_ptr = fopen( config, "r" );
        if( cfg_ptr == NULL ) {
		fprintf( stderr, "Cannot Open '%s'\n", config );
		exit(1);
	}

	/* Read config file */
	index=end=0;
	while( !end && fgets(buffer, sizeof(buffer), cfg_ptr) && ! feof(cfg_ptr) ){

		/* Comments ignored */
		if (*buffer == '#')
			continue;

		/* Blank lines ignored */
		if (*buffer == '\n')
			continue;

		switch (++index) {

			case STATUS :

				if ( (strncasecmp(buffer, "Closed", 6) == 0) ) {
					status = MODE_CLOSED;

				}else if ( (strncasecmp(buffer, "Owner", 5) == 0) ) {
					status = MODE_OWNER;

				}

				break;

			case OWNER :
				sscanf( buffer, "%s", owner);
				break;

			case DESCR :
				sscanf( buffer, "%[^\n]\n", descr);
				break;

			case ACKON :
				sscanf( buffer, "%[^\n]\n", text );

				if( (strncasecmp( text, "NOACK", 5 ) == 0) ) {
					ackon = 0;
				}else if ( (strncasecmp( text, "ACK", 3 ) == 0) ) {
					ackon = 1;
				}

			case SHOWDIST :
				sscanf( buffer, "%[^\n]\n", text );

				if( (strncasecmp( text, "NODIST", 6 ) == 0) ) {
					showdist = 0;
				}else if ( (strncasecmp( text, "DIST", 4 ) == 0) ) {
					showdist = 1;
				}

				end=1;
				break;
		}
	}

	fclose(cfg_ptr);

	/* Check the number of parameters read from the config file */
	if( index < 3 ) {
		fprintf( stderr, "Incomplete Config file\n" );
		exit(1);
	}


	if ( (strncasecmp( subject, "/VER", 4 ) == 0) ) {

		if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
			fprintf( stderr, "Unable to append to '%s'\n", mail_in );
			exit(1);
		}

		if( strlen(from_bbs) >= 6 ) {
		   	fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
		}else{
		   	fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
		}

		fprintf( out_ptr, "Re: %s\n", subject );

   		fprintf( out_ptr, VERS_STR  );

		fprintf( out_ptr, "%s", TAIL );
		fprintf( out_ptr, "/EX\n" );
		fclose( out_ptr );
		exit(0);
	}

	/* Check sender is permitted */

	if ( status == MODE_CLOSED ) {

		/* The owner is always permitted (even if they are not it the list iteslf) */
		if (strcasecmp( owner, from_call ) == 0)
			sender_ok++;

		/* The sysop is always permitted (even if they are not it the list iteslf) */
		if (strcasecmp( sys_call, from_call ) == 0)
			sender_ok++;


		/* Open the Distribution List */
		if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ){
			fprintf( stderr, "Unable to open '%s'\n", sent_to );
			exit(1);
		}

		while ( fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ){

			/* Comments ignored */
			if (*buffer == '#') {
				continue;

			/* Blank lines ignored */
			}else if (*buffer == '\n') {
				continue;

			/* Originator in the list ? */
			} else if ( sscanf( buffer, "%[^@ \t] @ %s", tocall, tobbs ) >= 1) {
				if ( strcasecmp( tocall, from_call ) == 0 )
					sender_ok++;
			}
		}

		fclose( dis_ptr );

		/* Is the Sender Permitted */
		if( ! sender_ok ) {

			if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
				fprintf( stderr, "Unable to append to '%s'\n", mail_in );
				exit(1);
			}

			if( strlen(from_bbs) >= 6 ) {
			   	fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
			}else{
		   		fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
			}
			fprintf( out_ptr, "Access to %s is denied\n", lname );

			fprintf( out_ptr, "Sorry %s, you are not permitted access to the %s list.\n", from_call, lname );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );
			fclose( out_ptr );
			exit(0);
		}

	}else if ( status == MODE_OWNER ) {

		/* The owner is always permitted (even if they are not it the list iteslf) */
		if (strcasecmp( owner, from_call ) == 0)
			sender_ok++;

		/* The sysop is always permitted (even if they are not it the list iteslf) */
		if (strcasecmp( sys_call, from_call ) == 0)
			sender_ok++;


		/* Is the Sender Permitted */
		if( ! sender_ok ) {

			if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
				fprintf( stderr, "Unable to append to '%s'\n", mail_in );
				exit(1);
			}

			if( strlen(from_bbs) >= 6 ) {
			   	fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
			}else{
		   		fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
			}
			fprintf( out_ptr, "Access to %s is denied\n", lname );

			fprintf( out_ptr, "Sorry %s, you are not permitted access to the %s list.\n", from_call, lname );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );

			fclose( out_ptr );
			exit(0);
		}

	}

	/* Lets handle the /commands */

	/* Deal with /LIST */
   	if ( (strncasecmp(subject, "/LIS", 4 ) == 0) ){

		if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ){
			fprintf( stderr, "Unable to open '%s'\n", sent_to );
			exit(1);
		}

		if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
			fprintf( stderr, "Unable to append to '%s'\n", mail_in );
			exit(1);
		}

		if( strlen(from_bbs) >= 6 ) {
			fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
		}else{
			fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
		}

		fprintf( out_ptr, "Re: %s\n", subject );

		if( *descr != '\0' ){
			fprintf( out_ptr, "\nThese Users are on the '%s' (%s) mailing list:\n\n", descr, lname );
		}else{
			fprintf( out_ptr, "\nThese Users are on the %s mailing list:\n\n", lname );
		}

		while ( fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ) {

			/* Comments ignored */
			if (*buffer == '#') {
				continue;

			/* Blank Lines ignored */
			} else if (*buffer == '\n') {
				continue;

			}else if( sscanf(buffer, "%[^@ \t] @ %s", tocall, tobbs ) == 2 ) {
				fprintf( out_ptr, "%12s @ %s\n", tocall, tobbs );

			}else if( sscanf(buffer, "%s", tocall ) == 1 ){
				fprintf( out_ptr, "%12s\n", tocall );

			}
		}

		fclose( dis_ptr );

		fprintf( out_ptr, "%s", TAIL );
		fprintf( out_ptr, "/EX\n" );

		fclose( out_ptr );
		exit(0);
	}


	/* Deal with /HELP */
	else if ( (strncasecmp( subject, "/HEL", 4 ) == 0) ) {

		hlp_ptr = fopen( "multi.help", "r" );
	        if( hlp_ptr == NULL ) {
			fprintf( stderr, "Cannot Open 'multi.help'\n");
			exit(1);
		}

		if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
			fprintf( stderr, "Unable to append to '%s'\n", mail_in );
			exit(1);
		}

		if( strlen(from_bbs) >= 6 ) {
			fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
		}else{
			fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
		}

		fprintf( out_ptr, "Re: %s\n", subject );

		fprintf( out_ptr, "HELP for Multi V" Version "\n" );

		while (fgets(buffer, sizeof(buffer), hlp_ptr) && ! feof(hlp_ptr) ){

			/* Comments ignored */
			if (*buffer == '#')
				continue;

			fprintf( out_ptr, "%s", buffer );
		}

		fclose( hlp_ptr );


		/* Optional per list HELP file */
		hlp_ptr = fopen( lhelp, "r" );
	        if( hlp_ptr != NULL ) {

			while (fgets(buffer, sizeof(buffer), hlp_ptr) && ! feof(hlp_ptr) ) {

				/* Comments ignored */
				if (*buffer == '#')
					continue;

				fprintf( out_ptr, "%s", buffer );
			}

			fclose( hlp_ptr );
		}

		fprintf( out_ptr, "%s", TAIL );
		fprintf( out_ptr, "/EX\n" );

		fclose( out_ptr );
		exit(0);
	}
 

	/* Deal with /ADD */
	else if ( (strncasecmp(subject, "/ADD", 4) == 0) ) {

		/* Open list */
		if ( status == MODE_OPEN ) {

			if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
				fprintf( stderr, "Unable to append to '%s'\n", mail_in );
				exit(1);
			}

			if( strlen(from_bbs) >= 6 ) {
				fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
			}else{
				fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
			}

			fprintf( out_ptr, "Re: %s\n", subject );

			/* Deal with optional CALLSIGN and BBS (part of subject line) for new list members */

			np1 = sscanf(subject, "%*s %[^@ \t] @ %s", tocall, tobbs );

			if( (dis_ptr = fopen( sent_to, "a" ) ) == NULL ){
				fprintf( stderr, "Unable to open '%s'\n", sent_to );
				exit(1);
			}

			switch( np1 ) {

				case 1:
					fprintf( dis_ptr, "%s\n", strupr(tocall) );
					fprintf( out_ptr, "\n%s has been added to the list\n", strupr(tocall) );
					break;

				case 2:
					fprintf( dis_ptr, "%s @ %s\n", strupr(tocall), strupr(tobbs) );
					fprintf( out_ptr, "\n%s @ %s has been added to the list\n", strupr(tocall), strupr(tobbs) );
					break;

				default:
					fprintf( dis_ptr, "%s\n", strupr(from_call) );
					fprintf( out_ptr, "\nYou have been added to the list\n" );
					break;
			}

			fclose( dis_ptr );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );

			fclose( out_ptr );
			exit(0);

		}else{ 

		/* Closed / Owner Only list */

			/* Allow list owner & Sysop to add to list */
			if ( (strcasecmp( owner, from_call ) == 0) || (strcasecmp( sys_call, from_call ) ==0) ) {

				if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
					fprintf( stderr, "Unable to append to '%s'\n", mail_in );
					exit(1);
				}

				if( strlen(from_bbs) >= 6 ) {
					fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
				}else{
					fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
				}

				fprintf( out_ptr, "Re: %s\n", subject );

				/* Deal with optional CALLSIGN and BBS (part of subject line) for new list members */

				np1 = sscanf( subject, "%*s %[^@ \t] @ %s", tocall, tobbs );

				if( (dis_ptr = fopen( sent_to, "a" ) ) == NULL ){
					fprintf( stderr, "Unable to open '%s'\n", sent_to );
					exit(1);
				}

				switch( np1 ) {

					case 1:
						fprintf( dis_ptr, "%s\n", strupr(tocall) );
						fprintf( out_ptr, "\n%s has been added to the list.\n", strupr(tocall) );
						break;

					case 2:
						fprintf( dis_ptr, "%s @ %s\n", tocall, tobbs );
						fprintf( out_ptr, "\n%s @ %s has been added to the list.\n", strupr(tocall), strupr(tobbs) );
						break;

					default:
						fprintf( out_ptr, "\nAdditional Parameters required for /ADD\n" );
						break;

				}

				fclose( dis_ptr );


				fprintf( out_ptr, "%s", TAIL );
				fprintf( out_ptr, "/EX\n" );

				fclose( out_ptr );
				exit(0);

			/* Don't allow none list owners to add to list */
			}else{
				if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
					fprintf( stderr, "Unable to append to '%s'\n", mail_in );
					exit(1);
				}

				if( strlen(from_bbs) >= 6 ) {
					fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
				}else{
					fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
				}

				fprintf( out_ptr, "Re: %s\n", subject );

				if( *descr != '\0' ){
					fprintf( out_ptr, "\nSorry %s, the %s (%s) list is a closed list. Please ask\n", from_call, descr, lname );   
				}else{
					fprintf( out_ptr, "\nSorry %s, the %s list is a closed list. Please ask\n", from_call, lname );
				}

 				fprintf( out_ptr, "%s (the List Owner) or %s (the Sysop of %s) for access to this list.\n", owner, sys_call, bbs_call );

				fprintf( out_ptr, "%s", TAIL );
				fprintf( out_ptr, "/EX\n" );

				fclose( out_ptr );
				exit(0);
			}
		}
	}


	/* /REMOVE option */

	else if ( ( strncasecmp(subject, "/REM", 4 ) == 0) ) {

		if( (out_ptr = fopen( mail_in, "a" )) == NULL ) {
			fprintf( stderr, "Unable to append to '%s'\n", mail_in );
			exit(1);
		}

		if( strlen(from_bbs) >= 6 ) {
			fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
		}else{
			fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
		}

		fprintf( out_ptr, "Re: %s\n", subject );

		np1 = sscanf( subject, "%*s %[^@ \t] @ %s", tocall, tobbs );

		/* Allow from the user */
		if( (np1 <=0 ) | ( (np1 >= 1) && ( (strcasecmp( tocall, from_call ) == 0) ) ) ) {

			if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ) {
				fprintf( stderr, "Unable to open '%s'\n", sent_to );
				exit(1);
			}

			if( (new_ptr = fopen( new_to, "w" ) ) == NULL ) {
				fprintf( stderr, "Unable to open '%s' for writing\n", sent_to );
				exit(1);
			}

			while (fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ) {

				/* Comments */
				if (*buffer == '#') {
					fprintf( new_ptr, "%s", buffer );

				/* Blank lines */
				}else if (*buffer == '\n') {
					fprintf( new_ptr, "%s", buffer );

				} else {

					np2 = sscanf( buffer, "%[^@ \t] @ %s", dcall, dbbs );

					if( (np1 == np1) && (np2 == 1) ){

						if( (strcasecmp(tocall, dcall) == 0) ) {
							fprintf( out_ptr, "Removing %s\n", dcall );
							fnd++;
							continue;
						}

					}else if( (np1 == np2) && (np2 == 2) ){
						if( (strcasecmp(tocall, dcall) == 0) && (strcasecmp(tobbs, dbbs) == 0) ) {
							fprintf( out_ptr, "Removing %s @ %s\n", dcall, dbbs );
							fnd++;
							continue;
						}
					}

					fprintf( new_ptr, "%s", buffer );
				}
			}

			if( fnd ){
				fprintf ( out_ptr, "\n%d Entry(s) removed\n", fnd );
			}else{
				switch( np1 ){
					case 1:
						fprintf( out_ptr, "\nSorry No match for '%s'\n", tocall );
						break;
					case 2:
						fprintf( out_ptr, "\nSorry No match for '%s @ %s'\n", tocall, tobbs );
						break;
				}
			}

			fclose( dis_ptr );
			unlink( sent_to );

			fclose( new_ptr );
			rename( new_to, sent_to );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );

			fclose( out_ptr );
			exit(0);


		/* Allow from List owner */
		} else if ( (strcasecmp( owner, from_call ) == 0)

		/* or Sysop */
		|| (strcasecmp( sys_call, from_call ) == 0) ) {

			if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ) {
				fprintf( stderr, "Unable to open '%s'\n", sent_to );
				exit(1);
			}

			if( (new_ptr = fopen( new_to, "w" ) ) == NULL ) {
				fprintf( stderr, "Unable to open '%s' for writing\n", sent_to );
				exit(1);
			}

			while (fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ) {

				/* Comments */
				if (*buffer == '#') {
					fprintf( new_ptr, "%s", buffer );

				/* Blank lines */
				}else if (*buffer == '\n') {
					fprintf( new_ptr, "%s", buffer );

				} else {

					np2 = sscanf( buffer, "%[^@ \t] @ %s", dcall, dbbs );

					if( (np1 == np1) && (np2 == 1) ){

						if( (strcasecmp(tocall, dcall) == 0) ) {
							fprintf( out_ptr, "Removing %s\n", dcall );
							fnd++;
							continue;
						}

					}else if( (np1 == np2) && (np2 == 2) ){
						if( (strcasecmp(tocall, dcall) == 0) && (strcasecmp(tobbs, dbbs) == 0) ) {
							fprintf( out_ptr, "Removing %s @ %s\n", dcall, dbbs );
							fnd++;
							continue;
						}
					}

					fprintf( new_ptr, "%s", buffer );

				}
			}

			if( fnd ){
				fprintf ( out_ptr, "\n%d Entry(s) removed\n", fnd );
			}else{
				switch( np1 ){
					case 1:
						fprintf( out_ptr, "\nNo match for '%s'\n", tocall );
						break;
					case 2:
						fprintf( out_ptr, "\nNo match for '%s @ %s'\n", tocall, tobbs );
						break;
				}
			}

			fclose( dis_ptr );
			unlink( sent_to );

			fclose( new_ptr );
			rename( new_to, sent_to );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );

			fclose( out_ptr );
			exit(0);

		} else {

	   		fprintf( out_ptr, "/REMOVE command not permitted for %s\n", lname );

 			fprintf( out_ptr, "Please get %s (list owner) or %s (the Sysop of %s) to remove members from list.\n", owner, sys_call, bbs_call );

			fprintf( out_ptr, "%s", TAIL );
			fprintf( out_ptr, "/EX\n" );

			fclose( out_ptr );
			exit(0);
		}

	/* un-recognised commands */
	}else if ( (strncasecmp( subject, "/", 1 ) == 0) ) {

		if( (out_ptr = fopen(mail_in, "a")) == NULL ) {
			fprintf( stderr, "Error appending to %s\n", mail_in );
			exit(1);
	        }

		if( strlen(from_bbs) >= 6 ) {
			fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
		}else{
			fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
		}

		fprintf( out_ptr, "Re: %s\n", subject );

		fprintf( out_ptr, "\nSorry %s,\n %s is not a recognised command", from_call, subject );

		fprintf( out_ptr, "\nTry /HELP" );

		fprintf( out_ptr, "\n%s", TAIL );
		fprintf( out_ptr, "/EX\n" );

		fclose( out_ptr );
		exit(0);
	}

	/* No Commands found in subject so deal with the message itself */
	/* ie: Copy the message to the listed users */


	/* Get Current Time Information */
	time_now = time(NULL);
	now = localtime( &time_now );

	/* Open the Distribution List */
	if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ){
		fprintf( stderr, "Unable to open '%s'\n", sent_to );
		exit(1);
	}

	/* and mail.in files */
	if( (out_ptr = fopen(mail_in, "a")) == NULL ) {
		fprintf( stderr, "Error appending to %s\n", mail_in );
		exit(1);
        }

	while ( fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ){

		/* Comments ignored */
		if (*buffer == '#')
			continue;

		/* Blank lines ignored */
		if (*buffer == '\n')
			continue;

		/* Originator is ignored */
		if (strncasecmp(buffer, from_call, strlen(from_call) ) == 0)
			continue;

		if( sscanf(buffer, "%[^@ \t] @ %s", tocall, tobbs ) == 2) {

			/* Send command line to mail.in */
			fprintf( out_ptr, "SP %s @ %s < %s\n", tocall, tobbs, from_call );

		}else if( sscanf(buffer, "%s", tocall ) == 1 ){

			/* Send command line to mail.in */
			fprintf( out_ptr, "SP %s < %s\n", tocall, from_call );

		}

		/* Send subject line to mail.in */
		fprintf( out_ptr, "%s\n", subject );


		/* Insert FAKE R: line (if necessary) - so WP system doesn't get confused */
		if( strlen (from_bbs) >=6 ){
			fprintf( out_ptr, "R:%02d%02d%02d/%02d%02d @:%s\n\n", now->tm_year % 100, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, from_bbs );
		}


		/* Add MULTI Header */
		fprintf( out_ptr, TAIL );
		fprintf( out_ptr, "# From: %s @ %s\n", lname, bbs_call );

		if( strlen (from_bbs) >=6 ) {
			fprintf( out_ptr, "# Originally From: %s @ %s\n", from_call, from_bbs );
		}else{
			fprintf( out_ptr, "# Originally From: %s\n", from_call );
		}

		fprintf( out_ptr, "# ========================\n" );

		if( *descr != '\0' ){
			fprintf( out_ptr, "# To the Members of '%s' :\n", descr );
		}else{
			fprintf( out_ptr, "# To:\n" );
		}

		/* Add list of members */
		if( showdist ) {

			/* Remember where we got too */
			dis_pos = ftell( dis_ptr );

			fseek( dis_ptr, 0, SEEK_SET);

			fprintf( out_ptr, "#" );

			i=0;
			while ( fgets(buffer, sizeof(buffer), dis_ptr) && ! feof( dis_ptr ) ){

				/* Comments ignored */
				if (*buffer == '#')
					continue;

				/* Blank Lines ignored */
				if (*buffer == '\n')
					continue;

				/* Originator is ignored */
				if ( strncasecmp( buffer, from_call, strlen(from_call) ) == 0 )
					continue;

				if( ( sscanf(buffer, "%[^@ \t] @ %s", tocall, tobbs ) == 2) ) {
					fprintf( out_ptr, "%12s", tocall );

				}else if( sscanf(buffer, "%s", tocall ) == 1 ){
					fprintf( out_ptr, "%12s", tocall);

				}

				i++;
				if( i >= 6 ) {
					i=0;
					fprintf( out_ptr, "\n#" );
				}
			}

			fprintf( out_ptr, "\n# ========================\n" );

			fseek( dis_ptr, dis_pos, SEEK_SET);

		}

		/* Optional list header */
		if( (msg_ptr = fopen( lhead, "r" )) != NULL ){
			while( fgets( buffer, sizeof(buffer), msg_ptr ) && ! feof( msg_ptr ) ){
				fprintf( out_ptr, "%s", buffer );
			}
			fclose(msg_ptr);
		}

		/* Append the Rest of Original Mesage */
		msg_ptr = fopen(argv[1], "r");		/* Open the received message */
		if (msg_ptr != NULL){
			fseek( msg_ptr, msg_pos, SEEK_SET );

			while( fgets( buffer, sizeof(buffer), msg_ptr ) && ! feof( msg_ptr ) ){

				/* Ignore FBB From: line */
				if( strncmp( buffer, "From: ", 6 ) == 0 ) {
					continue;

				/* Ignore FBB To  : line */
				}else if( strncmp( buffer, "To  : ", 6 ) == 0 ) {
					continue;

				/* Handle MULTI HEADER / FOOTER */
				}else if( strncmp( buffer, "# [MULTI", 8 ) == 0 ) {

					end = 0;
					while( ! end && fgets( buffer, sizeof(buffer), msg_ptr ) && ! feof(msg_ptr) ) {

						/* Comments ignored */
						if (*buffer == '#') {
							continue;
						}else{
							end=1;
						}
					}

				/* Ignore /ack ? */
				}else if( ! ackon && (strncasecmp(buffer, "/ack", 4 ) == 0 ) ) {
					continue;

				/* Ignore /ex */
				}else if( strncasecmp(buffer, "/ex", 3 ) == 0 ) {
					continue;
				}

				fprintf( out_ptr, "%s", buffer );

			}

			fclose(msg_ptr);

		}else{
			fprintf( out_ptr, "%s\n", "ERROR: MULTI was unable to copy message" );
		}

		/* Optional list footer */
		if( (msg_ptr = fopen( lfoot, "r" )) != NULL ){
			while( fgets( buffer, sizeof(buffer), msg_ptr ) && ! feof( msg_ptr ) ){
				fprintf( out_ptr, "%s", buffer );
			}
			fclose(msg_ptr);
		}

		fprintf( out_ptr, "%s", TAIL );
		fprintf( out_ptr, "/EX\n" );

	}

	fclose( dis_ptr );

	/* Send confirmation message to the Originator */

	if( strlen(from_bbs) >= 6 ) {
		fprintf( out_ptr, "\nSP %s @ %s < %s\n", from_call, from_bbs, bbs_call );
	}else{
		fprintf( out_ptr, "\nSP %s < %s\n", from_call, bbs_call );
	}

	fprintf( out_ptr, "Re: %s\n", subject );

	fprintf( out_ptr, "Message confirmation\n" );

	fprintf( out_ptr, "Your message \'%s\' has been distributed to:\n\n", subject );

	if( (dis_ptr = fopen( sent_to, "r" ) ) == NULL ){
		fprintf( stderr, "Unable to open '%s'\n", sent_to );
		exit(1);
	}

	while ( fgets(buffer, sizeof(buffer), dis_ptr) && ! feof(dis_ptr) ){

		/* Comments ignored */
		if (*buffer == '#')
			continue;

		/* Blank lines ignored */
		if (*buffer == '\n')
			continue;

		/* Originator is ignored */
		if ( strncasecmp( buffer, from_call, strlen(from_call) ) == 0 )
			continue;

		if( sscanf(buffer, "%[^@ \t] @ %s", tocall, tobbs ) == 2) {
			fprintf( out_ptr, "%12s @ %s\n", tocall, tobbs );

		}else if( sscanf(buffer, "%s", tocall ) == 1 ){
			fprintf( out_ptr, "%12s\n", tocall );

		}
	}

	fclose( dis_ptr );

	fprintf( out_ptr, "\n%s", TAIL );
	fprintf( out_ptr, "/EX\n" );

	fclose( out_ptr );

	unlink(argv[1]);
	exit(0);
}

