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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ata.h>
#include <sys/ioctl.h>

#include "ataidle.h"
#include "../mi/atagen.h"
#include "../mi/atadefs.h"
#include "../mi/util.h"

/* send a command to the drive */
int ata_cmd(ATA *ata, int atacmd, int drivercmd)
{
	int rc = 0;

	if (atacmd > 0)
		ata->atacmd.ata_cmd.u.ata.command = atacmd;


	if (drivercmd == IOCATAGMAXCHANNEL) {
		int maxchan = 0;
		rc = ioctl( ata->fd, drivercmd, &maxchan );
		ata->atacmd.ata_cmd.data = malloc(sizeof(int));
		*ata->atacmd.ata_cmd.data = maxchan;
	} else {
		rc = ioctl( ata->fd, IOCATAREQUEST, &(ata->atacmd.ata_cmd) );
	}

	return rc;
}

/* initialize the ata_cmd structure with supplied values */
int ata_setataparams( ATA *ata, int seccount, int count)
{
	/* clear the structure to remove any random values */
	memset(& ata->atacmd, 0, sizeof(struct ata_cmd));
	ata->atacmd.ata_cmd.u.ata.command = (uint8_t) IOCATAREQUEST;
	ata->atacmd.ata_cmd.flags = ATA_CMD_CONTROL;
	ata->atacmd.ata_cmd.timeout = ATA_CMD_TIMEOUT;
	ata->atacmd.ata_cmd.count = count;
	ata->atacmd.ata_cmd.u.ata.count = seccount;

	return 0;
}

void ata_setfeature_param( ATA *ata, int feature_val)
{
	ata->atacmd.ata_cmd.u.ata.feature = feature_val;
}

void ata_setdataout_params( ATA *ata, char ** databuf, int nbytes)
{
	*databuf = malloc(nbytes);

	if(*databuf == 0)
	{
		/* malloc failed, there's no way to recover here */
		fprintf(stderr, "malloc failed. aborting\n");
		exit(EXIT_FAILURE);
	}

	memset(*databuf, 0, nbytes);
	ata->atacmd.ata_cmd.data = *databuf;
	ata->atacmd.ata_cmd.count = nbytes;
	ata->atacmd.ata_cmd.flags = ATA_CMD_READ;
}

