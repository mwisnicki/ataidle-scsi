/*-
 *
 * Copyright 2004-2008 Bruce Cran <bruce@cran.org.uk>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mi/atadefs.h"
#include "mi/util.h"
#include "mi/atagen.h"

#ifdef __FreeBSD__
	#include <osreldate.h>

	#if __FreeBSD_version < 600034
		#error ATAidle requires FreeBSD 6.1 or newer
	#endif
#endif

int main( int argc, char ** argv )
{
	int rc = 0;
	ATA *ata;
	long opt_val;
	int ch;
	struct ata_ident ident;
	const char * const optstr = "hA:S:sI:iP:o";

	ata = malloc(sizeof(ATA));

	if (ata == NULL)
		err(EX_SOFTWARE, NULL);

	/* need more than just the executable name */
	if( argc == 1 )
		usage();

	ata->fd = -1;

	/* now we've done all the checking of parameters etc.,
	 * let's see what the user wants us to do.
	 */

	/* if we've got a non-option string, 
	 * see if it's a device node */
	if (argv[argc-1][0] != '-') {
		struct stat sb;
		rc = stat( argv[argc-1], &sb );
		if (rc)
			err(EX_OSFILE, "%s", argv[argc-1]);

		if (S_ISBLK(sb.st_mode) || S_ISCHR(sb.st_mode))
			rc = open( argv[argc-1], O_RDONLY );
		else
			errx(EX_OSFILE, "%s isn't a device node", argv[argc-1]);

		if (rc > 0)
			ata->fd = rc;
		else
			err(EX_IOERR, "error opening %s", argv[argc-1]);

		if (argc == 2)
			ata_showdeviceinfo(ata);
	}	

	optind = 1;
	opterr = 1;

	if (ata->fd != -1) {
		rc = ata_ident( ata, &ident );
		if (rc)
			errx(EX_SOFTWARE, "an error occurred identifying the device %s\n", argv[argc-1]);
	}


	while ((ch = getopt(argc, argv, optstr)) != -1)
	{
		switch(ch)
		{
			/* S = Standby */
			case 'S':
				opt_val = strtol( optarg, NULL, 10 );
				if(opt_val == LONG_MIN || opt_val == LONG_MAX)
					warnx("invalid standby value");
				else {
					if (ident.cmd_supp1 & ATA_PM_SUPPORTED)
						rc = ata_setstandbytimer( ata, opt_val );
					else
						warnx("the device does not support power management");
				}
				break;

			case 's':
			      rc = ata_setstandbytimer( ata, ATA_IDLEVAL_IMMEDIATE );
				break;

			/* o = Sleep (off) */
			case 'o':
				if (ident.cmd_supp1 & ATA_PM_SUPPORTED)
					rc = ata_sleep( ata );
				else
					warnx("the device does not support power management");
				break;

			/* I = Idle */
			case 'I':
				opt_val = strtol( optarg, NULL, 10 );

				if(opt_val == LONG_MIN || opt_val == LONG_MAX)
					warnx("invalid idle value");
				else {
					if (ident.cmd_supp1 & ATA_PM_SUPPORTED)
						rc = ata_setidletimer( ata, opt_val );
					else
						warnx("the device does not support power management");
				}
				break;

			case 'i':
				rc = ata_setidletimer(ata, ATA_IDLEVAL_IMMEDIATE);
				break;

			/* A = AutoAcoustic */
			case 'A':
				opt_val = strtol( optarg, NULL, 10 );
				if(opt_val == LONG_MIN || opt_val == LONG_MAX)
					warnx("invalid acoustic value");
				else {
					if (ident.cmd_supp2 & ATA_AAM_SUPPORTED)
						rc = ata_setacoustic( ata, opt_val );
					else
						warnx("the device does not support acoustic management");
				}
				break;

			/* P = Power (APM) */
			case 'P':
				opt_val = strtol( optarg, NULL, 10 );
				if(opt_val == LONG_MIN ||  opt_val == LONG_MAX)
					warnx("invalid apm value");
				else {
					if (ident.cmd_supp2 & ATA_APM_SUPPORTED)
						rc = ata_setapm( ata, opt_val );
					else
						warnx("the device does not support advanced power management");
				}
				break;

			case 'h':
			default:
				usage();
				break;
		}
	}

	if (ata != NULL)
	{
		if (ata->fd > 0)
			close(ata->fd);

		free(ata);
	}
	
	return (rc);
}

