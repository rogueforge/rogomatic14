/*
 * setup.c: Rog-O-Matic XIV (CMU) Wed Jan 30 17:38:07 1985 - mlm
 * Copyright (C) 1985 by A. Appel, G. Jacobson, L. Hamey, and M. Mauldin
 *
 * This is the program which forks and execs the Rogue & the Player
 */

# include <stdio.h>
# include <signal.h>
# include <string.h>
# include <pty.h>
# include <termios.h>
# include "install.h"

# define READ    0
# define WRITE   1

/* Define the Rog-O-Matic pseudo-terminal (Concept Based) */

# define ROGUETERM "rg|rterm:am:bs:ce=^[^S:cl=^L:cm=^[a%+ %+ :co#80:li#24:so=^[D:se=^[d:pt:ta=^I:up=^[;:db:xn:"

int   frogue, trogue;
extern char *getname();

main (argc, argv)
int   argc;
char *argv[];

{ int   ptm, pts;			/* Master and slave of Rogue's terminal */
  struct termios tio;
  struct winsize ws;
  int   child, score = 0, oldgame = 0;
  int   cheat = 0, noterm = 1, echo = 0, nohalf = 0, replay = 0;
  int   emacs = 0, rf = 0, terse = 0, user = 0, quitat = 2147483647;
  char  *rfile = "", *rfilearg = "", options[32];
  char  ropts[256], roguename[128];

  while (--argc > 0 && (*++argv)[0] == '-')
  { while (*++(*argv))
    { switch (**argv)
      { case 'c': cheat++;        break; /* Will use trap arrows! */
        case 'e': echo++;         break; /* Echo file to roguelog */
        case 'f': rf++;           break; /* Next arg is the rogue file */
        case 'h': nohalf++;       break; /* No halftime show */
        case 'n': quitat = -1;    break; /* Do not try to beat score */
        case 'p': replay++;       break; /* Play back roguelog */
        case 'r': oldgame++;      break; /* Use saved game */
        case 's': score++;        break; /* Give scores only */
        case 't': terse++;        break; /* Give status lines only */
        case 'u': user++;         break; /* Start up in user mode */
        case 'w': noterm = 0;     break; /* Watched mode */
        case 'E': emacs++;        break; /* Emacs mode */
        default:  printf 
                  ("Usage: rogomatic [-cefhprstuwE] or rogomatic [file]\n");
                  exit (1);
      }
    }

    if (rf) 
    { if (--argc) rfilearg = *++argv;
      rf = 0;
    }
  }

  if (argc > 1)
  { printf ("Usage: rogomatic [-cefhprstuwE] or rogomatic <file>\n");
    exit (1);
  }

  /* Find which rogue to use */
  if (*rfilearg)
  { if (access (rfilearg, 1) == 0)	rfile = rfilearg;
    else				{ perror (rfilearg); exit (1); }
  }
  else if (access ("rogue", 1) == 0)	rfile = "rogue";
# ifdef NEWROGUE
  else if (access (NEWROGUE, 1) == 0)	rfile = NEWROGUE;
# endif
# ifdef ROGUE
  else if (access (ROGUE, 1) == 0)	rfile = ROGUE;
# endif
  else
  { perror ("rogue");
    exit (1);
  }

  if (!replay && !score && quitat > 0)
    quitat = findscore (rfile, "Rog-O-Matic");

  sprintf (options, "%d,%d,%d,%d,%d,%d,%d,%d",
           cheat, noterm, echo, nohalf, emacs, terse, user,quitat);
  sprintf (roguename, "Rog-O-Matic %s for %s", RGMVER, getname ());
  sprintf (ropts, "name=%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
           roguename, "fruit=apricot", "terse", "noflush", "noask",
           "jump", "step", "nopassgo", "inven=slow", "seefloor");

  if (score)  { dumpscore (argc==1 ? argv[0] : DEFVER); exit (0); }
  if (replay) { replaylog (argc==1 ? argv[0] : ROGUELOG, options); exit (0); }

  /*
   * Rogue insists on playing from a terminal, so give it a pseudo
   * terminal rather than a pair of pipes.  The terminal is raw, so
   * that the characters pass through unchanged in both directions,
   * and is not made Rogue's controlling terminal, so that no control
   * character we send can turn into a signal.
   */

  memset (&tio, 0, sizeof (tio));
  cfmakeraw (&tio);
  tio.c_cflag |= CREAD | CS8;
  tio.c_cc[VMIN] = 1; tio.c_cc[VTIME] = 0;
  cfsetispeed (&tio, B38400); cfsetospeed (&tio, B38400);
  memset (&ws, 0, sizeof (ws));
  ws.ws_row = 24; ws.ws_col = 80;

  if (openpty (&ptm, &pts, NULL, &tio, &ws) < 0)
  { fprintf (stderr, "Cannot get a pseudo terminal!\n");
    exit (1);
  }

  frogue = ptm;
  trogue = dup (ptm);

  if ((child = fork ()) == 0)
  { close (0);
    dup (pts);
    close (1);
    dup (pts);
    close (pts);
    close (ptm);	/* Close parent's (player's) end of the terminal */
    close (trogue);

    putenv ("TERMCAP", ROGUETERM);
    putenv ("TERM", "rg");
    putenv ("ROGUEOPTS", ropts);
    putenv ("LINES", "24");
    putenv ("COLUMNS", "80");
    if (oldgame)  execl (rfile, rfile, "-r", NULL);
    if (argc)     execl (rfile, rfile, argv[0], NULL);
    execl (rfile, rfile, NULL);
    _exit (1);
  }

  else
  { /* Encode the open files into a two character string */

    char ft[3] = "aa", rp[32]; ft[0] += frogue; ft[1] += trogue;

    close (pts);	/* Close child's (rogue's) end of the terminal */

    /* Pass the process ID of the Rogue process as an ASCII string */
    sprintf (rp, "%d", child);

    if (!author ()) nice (4);

    execl ("player", "player", ft, rp, options, roguename, NULL);
# ifdef PLAYER
    execl (PLAYER, "player", ft, rp, options, roguename, NULL);
# endif
    printf ("Rogomatic not available, 'player' binary missing.\n");
    kill (child, SIGKILL);
  }
}

/* 
 * replaylog: Given a log file name and an options string, exec the player
 * process to replay the game.  No Rogue process is needed (since we are
 * replaying an old game), so the frogue and trogue file descrptiors are
 * given the fake value 'Z'.
 */

replaylog (fname, options)
char *fname, *options;
{ execl ("player", "player", "ZZ", "0", options, fname, NULL);
# ifdef PLAYER
  execl (PLAYER, "player", "ZZ", "0", options, fname, NULL);
# endif
  printf ("Replay not available, 'player' binary missing.\n");
  exit (1);
}

/*
 * author:
 *	See if a user is an author of the program
 */

author()
{
  switch (getuid())
  { case 1337:	/* Fuzzy */
    case 1313:	/* Guy */
    case 1241:	/* Andrew */
    case 345:	/* Leonard */
    case 342:	/* Gordon */
		return 1;
    default:	return 0;
  }
}
