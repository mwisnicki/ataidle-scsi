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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sysexits.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <camlib.h>
#include <cam/scsi/scsi_message.h>

#include <sys/types.h>
#include <sys/ata.h>
#include <sys/ioctl.h>

#include "ataidle.h"
#include "../mi/atagen.h"
#include "../mi/atadefs.h"
#include "../mi/util.h"

static const char * const scsi_prefix_da = "/dev/da";

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* open ata device */
int ata_open(ATA **ataptr, const char *device) {
	int rc;
	assert(ataptr != NULL);
	
	*ataptr = malloc(sizeof(ATA));
	{
		ATA *ata = *ataptr;
		if (ata == NULL)
			err(EX_SOFTWARE, NULL);
		ata->devhandle.fd = -1;

		/* TODO better detection of SCSI/SAT */
		ata->access_mode = ACCESS_MODE_ATA;
		if (strncmp(device, scsi_prefix_da, strlen(scsi_prefix_da)) == 0)
			ata->access_mode = ACCESS_MODE_SAT;

		switch (ata->access_mode) {
		case ACCESS_MODE_ATA:
			rc = open( device, O_RDONLY );
			if (rc > 0)
				ata->devhandle.fd = rc;
			break;
		case ACCESS_MODE_SAT:
			rc = cam_get_device( device, ata->devhandle.camdevname,
					     DEV_IDLEN,
					     &(ata->devhandle.camunit) );
			if (rc != 0)
				goto fail;
			ata->devhandle.camdev = cam_open_spec_device(
				ata->devhandle.camdevname,
				ata->devhandle.camunit,	O_RDONLY, NULL );
			if (ata->devhandle.camdev == NULL) {
				rc = -1;
				warnx("%s", cam_errbuf);
				goto fail;
			}
			
			break;
		}
	}
	return rc;
fail:
	free(*ataptr);
	return rc;
}

/* close ata device and free memory, set pointer to NULL */
void ata_close(ATA **ataptr) {
	if (ataptr != NULL) {
		ATA *ata = *ataptr;
		if (ata != NULL) {
			switch (ata->access_mode) {
			case ACCESS_MODE_ATA:
				if (ata->devhandle.fd > 0)
					close(ata->devhandle.fd);
				ata->devhandle.fd = -1;
				break;
			case ACCESS_MODE_SAT:
				if (ata->devhandle.camdev != NULL)
					cam_close_device(ata->devhandle.camdev);
				ata->devhandle.camdev = NULL;
				break;
			}
			free(ata);
		}
		*ataptr = NULL;
	}
}

/* check if ata points to opened device */
int ata_is_opened(ATA *ata)
{
	if (ata == NULL)
		return FALSE;
	switch (ata->access_mode) {
	case ACCESS_MODE_ATA:
		return ata->devhandle.fd > 0;
	case ACCESS_MODE_SAT:
		return ata->devhandle.camdev != NULL;
	default:
		err(EX_SOFTWARE, "unknown access mode %d", ata->access_mode);
		return FALSE; /* UNREACHABLE */
	};
}

/* TODO move generic ATA <-> SAT CDB conversion code to mi */

void sat_cdb_set_command(union sat_cdb* cdb, enum ata_command atacmd)
{
	switch (cdb->sc_h.opcode) {
	case SAT_ATA_PASSTHROUGH_12:
		cdb->scshort.command = atacmd;
		break;
	case SAT_ATA_PASSTHROUGH_16:
		cdb->sclong.command = atacmd;
		break;
	}
}

static
int translate_ata_to_csio(struct ccb_scsiio *csio, ATA *ata, enum ata_command atacmd, int drivercmd)
{
	int rc = 0;
	int cmdlen = 16;
	union sat_cdb* cdb = (union sat_cdb*) csio->cdb_io.cdb_bytes;

	bzero(&(&csio->ccb_h)[1], sizeof(struct ccb_scsiio) - sizeof(struct ccb_hdr));

	/* cam_fill_csio() sucks */

	csio->ccb_h.func_code = XPT_SCSI_IO;
	csio->ccb_h.flags = 0;
	csio->ccb_h.retry_count = 1;
	csio->ccb_h.cbfcnp = NULL;
	csio->ccb_h.timeout = ata->atacmd.ata_cmd.timeout * 1000; /* ATA_CMD_TIMEOUT * 1000; */
	csio->data_ptr = NULL;
	csio->dxfer_len = 0;
	csio->sense_len = 32;
	csio->cdb_len = cmdlen;
	csio->tag_action = MSG_SIMPLE_Q_TAG;

	cdb->sc_h.opcode = SAT_ATA_PASSTHROUGH_16;

	switch (ata->atacmd.ata_cmd.flags) {
	case ATA_CMD_READ:
		csio->ccb_h.flags = CAM_DIR_IN;
		csio->data_ptr = (u_int8_t*) ata->atacmd.ata_cmd.data;
		csio->dxfer_len = ata->atacmd.ata_cmd.count;
		cdb->sc_h.protocol = ATA_PROT_PIO_DATA_IN;
		cdb->sc_h.extend = 1;
		break;
#if 0
	case ATA_CMD_CONTROL:
		csio->ccb_h.flags = CAM_DIR_NONE;
		break;
#endif
	default:
		err(EX_SOFTWARE, "unknown ata command flag %d", ata->atacmd.ata_cmd.flags);
		return -1; /* UNREACHABLE */
	}

	sat_cdb_set_command(cdb, atacmd);

	switch (atacmd) {
	case ATA__IDENTIFY:
		break;
	case ATA__ATAPI_IDENTIFY:
	case ATA__SETFEATURES:
	case ATA_IDLE:
	case ATA_STANDBY:
		err(EX_SOFTWARE, "unknown ata command %x", atacmd);
		return -1; /* UNREACHABLE */
	}

	return rc;
}

/* send a command to the drive */
int ata_cmd(ATA *ata, enum ata_command atacmd, int drivercmd)
{
	int rc = 0;

	if (atacmd > 0)
		ata->atacmd.ata_cmd.u.ata.command = atacmd;

	switch (ata->access_mode) {
	case ACCESS_MODE_ATA:
		if (drivercmd == IOCATAGMAXCHANNEL) {
			int maxchan = 0;
			rc = ioctl( ata->devhandle.fd, drivercmd, &maxchan );
			ata->atacmd.ata_cmd.data = malloc(sizeof(int));
			*ata->atacmd.ata_cmd.data = maxchan;
		} else {
			rc = ioctl( ata->devhandle.fd, IOCATAREQUEST, &(ata->atacmd.ata_cmd) );
		}
		break;
	case ACCESS_MODE_SAT:
		if (drivercmd == IOCATAGMAXCHANNEL) {
			/* XXX hardcoded to 1 channel */
			rc = 0;
			ata->atacmd.ata_cmd.data = malloc(sizeof(int)); /* memory leak (above too) */
			*ata->atacmd.ata_cmd.data = 1;
		} else {
			union ccb * ccb;
			struct ccb_scsiio * csio;

			ccb = cam_getccb(ata->devhandle.camdev);
			if (!ccb)
				return -1;

			csio = &ccb->csio;

			rc = translate_ata_to_csio(csio, ata, atacmd, drivercmd);
			if (rc) return rc;

			rc = cam_send_ccb(ata->devhandle.camdev, ccb);
			cam_freeccb(ccb);
			if (rc) return rc;
		}
		break;
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

void ata_setfeature_param( ATA *ata, enum ata_feature feature_val)
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

