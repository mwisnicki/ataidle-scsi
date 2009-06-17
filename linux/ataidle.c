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

/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/hdreg.h>

/* application-specific includes */
#include "ataidle.h"
#include "../mi/atagen.h"
#include "../mi/atadefs.h"
#include "../mi/util.h"	

/* send a command to the drive */
int ata_cmd(ATA *ata, int cmd, int drivercmd)
{
	int rc = 0;
	
	ata->atacmd.cmd = cmd;
	rc = ioctl( ata->fd, HDIO_DRIVE_CMD, &ata->atacmd );
	
	return rc;
}

/* initialize the ata_cmd structure with supplied values */
int ata_setataparams(ATA *ata, int seccount, int count)
{
	/* clear the structure to remove any random values */
	memset(&ata->atacmd, 0, sizeof(struct ata_cmd));
	
	if(seccount != 0)
		ata->atacmd.sector_number = seccount;
	else
		ata->atacmd.sector_count = 1;
	
	return 0;
}

void ata_setdataout_params(ATA *ata, char ** databuf, int nbytes)
{
	*databuf = (char*) ata->atacmd.buf;
}


void ata_setfeature_param(ATA *ata, int feature)
{
	ata->atacmd.feature = feature;
}
