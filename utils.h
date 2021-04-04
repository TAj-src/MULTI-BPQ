/*
  utils.h
  Copyright (c) 2000 by Stewart Wilkinson G0LGS
*/

#ifndef _UTILS_H
#define _UTILS_H

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

extern char *strupr(char *);
extern char *strlwr(char *);
extern char *strdt(long temps);
extern int copy_ (char *old, char *new );
extern int Check_Call(char *s);
extern int Check_Home(char *s);

#endif
