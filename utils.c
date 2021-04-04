/*
  utils.c
  Copyright (c) 2000 by Stewart Wilkinson G0LGS
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"

#define _read read
#define _write write
#define O_BINARY 0
#define __a2__ __attribute__ ((packed, aligned(2)))

/*
 *      Convert a string to upper case
 */
char *strupr(char *s)
{
        char *p;

        if (s == NULL)
                return NULL;

        for (p = s; *p != '\0'; p++)
                *p = toupper(*p);

        return s;
}

/*
 *      Convert a string to lower case
 */
char *strlwr(char *s)
{
        char *p;

        if (s == NULL)
                return NULL;

        for (p = s; *p != '\0'; p++)
                *p = tolower(*p);

        return s;
}


char *strdt (long temps)
{
	struct tm *sdate;
	static char cdate[80];

	sdate = localtime (&temps);

	sprintf (cdate, "%02d%02d%02d/%02d%02d",
			 sdate->tm_year % 100,
			 sdate->tm_mon + 1,
			 sdate->tm_mday,
			 sdate->tm_hour,
			 sdate->tm_min);
	return (cdate);
}


#define BUFFER 16384

int copy_ (char *old, char *new)
{
	int retour = 1;
	int fd_orig;
	int fd_dest;
	int nb_lus;
	int ret;
	int dest_access;
	char *buffer;

	buffer = malloc (BUFFER);
	if (buffer == NULL)
		return (0);

	if ((fd_orig = open (old, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == EOF)
	{
		fprintf (stderr, "Cannot find %s\n", old);
		free (buffer);
		return (0);
	}

	dest_access = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY;
	if ((fd_dest = open (new, dest_access, S_IREAD | S_IWRITE)) == EOF)
	{
		close (fd_orig);
		fprintf (stderr, "Cannot create %s\n", new);
		free (buffer);
		return (0);
	}

	for (;;)
	{

		nb_lus = _read (fd_orig, buffer, BUFFER);

		if (nb_lus == -1)
		{
			retour = 0;
			break;
		}

		if (nb_lus == 0)
		{
			retour = 1;
			break;
		}

		ret = _write (fd_dest, buffer, nb_lus);

		if (ret != nb_lus)
		{
			retour = 0;
			break;
		}

	}

	close (fd_orig);
	close (fd_dest);

	free (buffer);

	return (retour);
}

int Check_Call(char *s)
{
	int len = 0;
	int nums = 0;
	char *t = s;

	if( t == (NULL) )
		return -1;

	while (*t) {
		*t = toupper(*t);

		if (!isalnum(*t))
			return -1;

		if (isdigit(*t))
			nums++;

		len++;
		t++;
	}

	if (len < 3 || len > 6 || !nums || nums > 2 )
		return -1;

	return 0;
}

int Check_Home(char *s)
{
	int  dots = 0;
	char allow[39] = "01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ#.";
	char bbs_call[64];
	char *p;

	if (s == (NULL) )
		return -1;

	/* Check Callsign part (up to first dot) */
	strcpy(bbs_call, s);

	if( (p = strchr(bbs_call,'.') ) != NULL ){
		p[0] = '\0';

		if(Check_Call(bbs_call) == -1)
			return -1;
	}

	/* Must not have 2 dots together */
	if( strstr(s,"..") != NULL )
		return -1;

	while (*s) {

		if(!strchr(allow, *s))
			return -1;

		if(strchr(".", *s))
			dots++;

		s++;
	}

	/* Last char must not be a dot */
	s--;
	if( strcmp(s,".") == 0 )
		return -1;

	/* Must have at least 2 dots */
	if(dots < 2)
		return -1;

	return 0;
}

