/*-
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

/* Generic definitions (applicable to all supported operating systems) */

#ifndef ATAGEN_H
#define ATAGEN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __FreeBSD__
	#include <osreldate.h>
	#include <sys/ata.h>
	#include "../freebsd/ataidle.h"
#else
	#include "../linux/ataidle.h"
#endif

struct ata_ident
{
	uint16_t	config;
	uint16_t	word1;
	uint16_t	config_specific;
	uint16_t	word3;
	uint16_t	word4[2];
	uint16_t	word6;
	uint16_t	word7[2];
	uint16_t	word9;
	uint8_t		serial[20];
	uint16_t	word20[2];
	uint16_t	word22;
	uint8_t		firmware[8];
	uint8_t		model[40];
	uint16_t	max_sect_per_trans;
	uint16_t	word48;
	uint16_t	caps1;
	uint16_t	caps2;
	uint16_t	word51[2];
	uint16_t	words_valid;
	uint16_t	word54[5];
	uint16_t	sectors_per_interrupt;
	uint16_t	nsect[2];
	uint16_t	word62;
	uint16_t	mdma_supp;
	uint16_t	pio_supp;
	uint16_t	mdma_trans_time;
	uint16_t	mdma_trans_time_vendor;
	uint16_t	pio_trans_time_noflow;
	uint16_t	pio_trans_time_flow;
	uint16_t	word69[2];
	uint16_t	word71[4];
	uint16_t	queue_depth;
	uint16_t	word76[4];
	uint16_t	version_major;
	uint16_t	version_minor;
	uint16_t	cmd_supp1;
	uint16_t	cmd_supp2;
	uint16_t	cmd_supp_ext;
	uint16_t	cmd_enabled1;
	uint16_t	cmd_enabled2;
	uint16_t	cmd_ext_default;
	uint16_t	udma_modes;
	uint16_t	erase_time_security;
	uint16_t	erate_time_security_enhance;
	uint16_t	apm_value;
	uint16_t	master_passwd_rev_code;
	uint16_t	hardware_reset_result;
	uint16_t	aam_value;
	uint16_t	word95[5];
	uint16_t	max_lba48_address[4];
	uint16_t	word104[23];
	uint16_t	removable_media_features_supp;
	uint16_t	security_status;
	uint16_t	vendor_specific[31];
	uint16_t	cfa_powermode_1;
	uint16_t	word161[15];
	uint16_t	media_serial_no[30];
	uint16_t	word206[49];
	uint16_t	integrity;
};

#define ATA_PM_SUPPORTED	0x0008

#define ATA_APM_SUPPORTED	0x0008
#define ATA_APM_ENABLED		0x0008

#define ATA_AAM_SUPPORTED	0x0200
#define ATA_AAM_ENABLED		0x0200

#define ATA_SMART_SUPPORTED	0x0001
#define ATA_SMART_ENABLED	0x0001

typedef struct 
{
	int fd;
	uint32_t chan;
	uint32_t dev;
	uint32_t cmd;
	struct ata_cmd atacmd;
} ATA;

int	ata_open( ATA *ata, const char *device );
void	ata_close( ATA *ata );
int	ata_setidletimer( ATA *ata, uint32_t idle_mins );
int	ata_sleep( ATA *ata );
int	ata_setstandbytimer( ATA *ata, uint32_t standby_mins );
int	ata_setacoustic( ATA *ata, uint32_t acoustic_val);
int	ata_setapm( ATA *ata, uint32_t apm_val);
void	ata_listdevices( ATA *ata );
int	ata_getmaxchan( ATA *ata, uint32_t *maxchan );
int	ata_cmd( ATA *ata, int atacmd, int drivercmd );
bool	ata_devpresent( ATA *ata );
int	ata_ident( ATA *ata, struct ata_ident * identity);
void	ata_showdeviceinfo( ATA *ata );
void	ata_setfeature_param( ATA *ata, int feature_val);
int	ata_setataparams( ATA *ata, int seccount, int count);
void	ata_setdataout_params( ATA *ata, char ** databuf, int nbytes);

#endif /* ATAIDLE_H */
