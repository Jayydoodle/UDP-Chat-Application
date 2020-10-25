#include <stdio.h> /* for perror() */
#include <stdlib.h> /* for exit() */

/* ====================== DieWithError =========================
/* Error handler for TCP send and recv methods
/*
/* @param errorMessage - the error message to deliver
/* @returns nothing
*/
void DieWithError(char *errorMessage)
{
	perror(errorMessage);
	exit(1);
}
