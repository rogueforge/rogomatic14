/*
 * gene.c: Rog-O-Matic XIV (CMU) Sat Feb 23 20:35:56 1985 - wel
 * Copyright (C) 1985 by A. Appel, G. Jacobson, L. Hamey, and M. Mauldin
 *
 * Initialize and summarize the gene pool
 */

# include <stdio.h>
# include "types.h"
# include "install.h"

extern FILE *openlog ();
int knob[MAXKNOB];
char genelock[100];
char genelog[100];
char genepool[100];

main (argc, argv)
int   argc;
char *argv[];
{ int m=10, init=0, seed=0, version=DEFRV, full=0;

  /* Get the options */
  while (--argc > 0 && (*++argv)[0] == '-')
  { while (*++(*argv))
    { switch (**argv)
      { case 'i':	init++;
	when 'f':	full++;
	when 'm':	m = atoi(*argv+1); SKIPARG;
			printf ("Gene pool size %d.\n", m);
	when 's':	seed = atoi(*argv+1); SKIPARG;
			printf ("Random seed %d.\n", m);
	when 'v':	version = atoi(*argv+1); SKIPARG;
			printf ("Rogue version %d.\n", version);
	otherwise:	quit (1, "Usage: gene [-i] [-msv<value>]\n");
      }
    }
  }

  /* Assign the gene log and pool file names */
  (void) sprintf (genelock, "%s/GeneLock%d", RGMDIR, version);
  (void) sprintf (genelog, "%s/GeneLog%d", RGMDIR, version);
  (void) sprintf (genepool, "%s/GenePool%d", RGMDIR, version);

  critical ();				/* Disable interrupts */
  if (lock_file (genelock, MAXLOCK))
  { if (init) 
    { srand (seed);			/* Set the random number generator */
      (void) openlog (genelog);		/* Open the gene log file */
      initpool (MAXKNOB, m);		/* Random starting point */
      writegenes (genepool);		/* Write out the gene pool */
      closelog ();			/* Close the log file */
    }
    else if (! readgenes (genepool))	/* Read the gene pool */
      quit (1, "Cannot read file '%s'\n", genepool);

   unlock_file (genelock);
  }
  else
    quit (1, "Cannot access file '%s'\n", genepool);

  uncritical ();			/* Re-enable interrupts */
  analyzepool (full);			/* Print a summary */
}
