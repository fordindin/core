/* 
   Copyright (C) 2008 - Mark Burgess

   This file is part of Cfengine 3 - written and maintained by Mark Burgess.
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version. 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

/*****************************************************************************/
/*                                                                           */
/* File: selfdiagnostic.c                                                    */
/*                                                                           */
/*****************************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"

/*****************************************************************************/

void AgentDiagnostic()

{
FOUT = stdout;

printf("----------------------------------------------------------\n");
printf("Cfengine 3 - Performing level 2 self-diagnostic (dialogue)\n");
printf("----------------------------------------------------------\n\n");
TestVariableScan();
TestExpandPromise();
TestExpandVariables();
TestSearchFilePromiser();
}

/******************************************************************/

void TestSearchFilePromiser()

{ struct Promise pp,*pcopy;
  struct Body *bp;
  int i;
  char *list_text1 = "a,b,c,d,e,f,g";
  char *list_text2 = "1,2,3,4,5,6,7";
  struct Rlist *rp, *args, *listvars = NULL, *scalarvars = NULL;
  struct Constraint *cp;
  struct FnCall *fp;

/* Still have diagnostic scope */
  
pp.promiser = "/var/.*/[c|l].*";
pp.promisee = "the monitor";
pp.classes = "proletariat";
pp.petype = CF_SCALAR;
pp.lineno = 0;
pp.audit = NULL;
pp.conlist = NULL;

printf("\nTestSearchFilePromiser(%s)\n\n",pp.promiser);
LocateFilePromiserGroup(pp.promiser,&pp,VerifyFilePromise);

pp.promiser = "/var/[^/]*/[c|l].*";
printf("\nTestSearchFilePromiser(%s)\n\n",pp.promiser);
LocateFilePromiserGroup(pp.promiser,&pp,VerifyFilePromise);

pp.promiser = "/var/[c|l][A-Za-z0-9_ ]*";
printf("\nTestSearchFilePromiser(%s)\n\n",pp.promiser);
LocateFilePromiserGroup(pp.promiser,&pp,VerifyFilePromise);

AppendConstraint(&(pp.conlist),"path","literal",CF_SCALAR,NULL);
pp.promiser = "/var/[^/]*/[c|l].*";
printf("\nTestSearchFilePromiser(%s)\n\n",pp.promiser);
LocateFilePromiserGroup(pp.promiser,&pp,VerifyFilePromise);

pp.promiser = "/var/.*/h.*";
printf("\nTestSearchFilePromiser(%s)\n\n",pp.promiser);
LocateFilePromiserGroup(pp.promiser,&pp,VerifyFilePromise);

}
