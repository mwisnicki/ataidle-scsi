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

#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "atadefs.h"
#include "atagen.h"
#include "util.h"

static int is_big_endian(void);

/* calculate the idle timer value to send to the drive. */
int ata_getidleval(uint32_t idle_mins, uint16_t *timer_val)
{
	int rc = 0;

	/* idle value of 0 disables spindown */
	if( idle_mins == 0 )
		*timer_val = 0;

	/* value is (value*5)s between 1 and 20 minutes */
	if( idle_mins <= 20 )
		*timer_val = (idle_mins*60)/5;

	/* special case for 21 minutes */
	if( idle_mins == 21 )
		*timer_val = 252;

	/* there is no encoding for values between 21 and 30 minutes */
	if( (idle_mins > 21) && (idle_mins < 30) ) {
		printf("cannot set timeout for values 20-30 minutes\n" );
		rc = -2;
	}

	/* after 30 mins, encoding is (idle_mins-29)*30, so you
	 * can only encode multiples of 30 minutes
	 */
	if( idle_mins >= 30 ) {
		/* if it's not a multiple of 30 minutes, or it's greater than 5 hours,
		 * we can't handle it.
		 */
		if( (((idle_mins % 30) != 0) || (idle_mins > 330))
				&& (idle_mins != ATA_IDLEVAL_IMMEDIATE) )
		{
			printf( "idle value must be a multiple of 30 minutes, "
					"up to 5 hours\n" );
			rc = -2;
		}

		/* otherwise, calculate the timer value */
		if(idle_mins == ATA_IDLEVAL_IMMEDIATE)
			*timer_val = ATA_IDLEVAL_IMMEDIATE;
		else
			*timer_val = 241 + (idle_mins/30);
	}

	return rc;
}


char * ata_getversionstring( unsigned short ata_version )
{
	const int ATAVERSION_LEN = 16;
	char * version = malloc(ATAVERSION_LEN);
	int i;

	if(version == 0) { /* malloc failed */
		fprintf(stderr, "malloc failed\n");
		return NULL;
	}

	memset(version, 0, ATAVERSION_LEN);

	for(i = 0; i < 15; i++) {
		if( (ata_version >> i) > 0 ) {
			sprintf(version, "ATA-%d", i);
		}
	}

	return version;
}


void usage(void)
{
	printf( "ataidle version " ATAIDLE_VERSION "\n\n"
			"usage: \n"
			"ataidle [-h] [-i] [-s] [-o] [-I idle] [-S standby] [-A acoustic] [-P apm]\n"
			"\tdevice\n\n"
			"Options:\n");
	printf(
			"-h\t\tdisplay this help and exit\n"
			"-I\t\tset the idle timeout in minutes\n"
			"-i\t\tput the drive into idle mode\n"
			"-S\t\tset the standby timeout in minutes\n"
			"-s\t\tput the drive into standby mode\n"
			"-o\t\tput the drive into sleep mode\n"
			"-A\t\tset the acoustic level, values 1-127\n"
			"-P\t\tset the power management level, values 1-254\n"
			"device\t\tthe device node e.g /dev/ad0\n\n"
			"if no options are specified, information\n"
			"about the device will be displayed\n\n");

	exit(EX_USAGE);
}

void byteswap(char * buf, int from, int to)
{
	int i;
	for(i = from; i <= to; i+=2)
	{
		char b1 = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = b1;
	}
}

void strpack(char * buf, int from, int to)
{
	int i = 0, j= 0, k = 0;
	int numchars = (to-from)+1;
	char *tmpbuf = malloc(numchars*sizeof(char));

	if(tmpbuf == 0) { /* malloc failed, skip the string packing */
		fprintf(stderr, "malloc failed\n");
		return;
	}

	strncpy(tmpbuf, buf+from, numchars);
	memset(buf+from, 0, numchars);

	i = 0;

	while(tmpbuf[i] == ' ')
		i++;

	j = 0;
	k = 0;

	for(j = i; j < numchars; j++)
	{
		buf[from+k] = tmpbuf[j];
		k++;
	}

	free(tmpbuf);
}

void mem_swap(int16_t * val)
{
	int8_t * smallval = (int8_t*) val;
	int8_t tmp = smallval[0];

	smallval[0] = smallval[1];
	smallval[1] = tmp;
}

static int is_big_endian(void) 
{
	int i = 0;
	((char*)(&i))[0] = 1;
	return (i != 1);
}

void ata_showdeviceinfo( ATA *ata )
{
	int rc = 0;
	struct ata_ident ident;
	int16_t *buf;
	unsigned int mbsize;
	char model[41];
	char serial[21];
	char firmware[9];
	char *ata_version = NULL;

	memset(&ident, 0, sizeof(struct ata_ident));

	rc = ata_ident( ata,(struct ata_ident*)  &ident );

	if (rc) {
		printf("could not get device information: is a device attached?\n");
		return;
	}

	buf = (int16_t*) &ident;

	memset(model, 0, 41);
	memset(serial, 0, 21);
	memset(firmware, 0, 9);

	strncpy(model, (char*) ident.model, 40);
	strncpy(serial, (char*) ident.serial, 20);
	strncpy(firmware, (char*) ident.firmware, 8);

	printf("Model:\t\t\t%s\n", model);
	printf("Serial:\t\t\t%s\n", serial);
	printf("Firmware Rev:\t\t%s\n", firmware);
	ata_version = ata_getversionstring(buf[80]);
	printf("ATA revision:\t\t%s\n", (ident.version_major > 1)?
			ata_version : "unknown/pre ATA-2");
	printf("LBA 48:\t\t\t%s\n", (buf[86] & 0x400)? "yes" : "no");
	printf("Geometry:\t\t%d cyls, %d heads, %d spt\n", buf[1], buf[3], buf[6]);

	if (ata_version != NULL)
		free(ata_version);

	mbsize = 0;

	if (buf[86] & 0x400)
	{
		mbsize = (((uint64_t)ident.max_lba48_address[0] +
			((uint64_t)ident.max_lba48_address[1] << 16) +
			((uint64_t)ident.max_lba48_address[2] << 32) +
			((uint64_t)ident.max_lba48_address[3] << 48))*512)/1048576;
	}
	else 
	{
		mbsize = ((ident.nsect[0] + (ident.nsect[1] << 16))*512)/1048576;
	}

	printf("Capacity:\t\t%u%s\n", (mbsize < 1024)?
			mbsize : mbsize/1024, (mbsize < 1024)? "MB" : "GB");
	printf("SMART Supported: \t%s\n", (buf[82] & 1)? "yes" : "no" );
	if(buf[82] & 1)
		printf("SMART Enabled: \t\t%s\n", (buf[85] & 1)? "yes" : "no" );
	printf("APM Supported: \t\t%s\n", (buf[83] & 8)? "yes" : "no" );
	if(buf[83] & 8)
		printf("APM Enabled: \t\t%s\n", (buf[86] & 8)? "yes" : "no" );
	printf("AAM Supported: \t\t%s\n", (buf[83] & 0x200)? "yes" : "no" );
	printf("AAM Enabled: \t\t%s\n", (buf[86] & 0x200)? "yes" : "no");
	if((buf[86] & 0x200)) {
		printf("Current AAM: \t\t%d\n", ((buf[94] & 0x00FF))-127);
		printf("Vendor Recommends AAM: \t%d\n", ((buf[94] & 0xFF00) >> 8)-127);
	}

	if((buf[86] & 8))
		printf("APM Value: \t\t%d\n", buf[91]);
}

void byteswap_ata_data( int16_t * buf )
{
	int i;

	for (i = 0; i < 10; i++)
		mem_swap(buf+i);

	for (i = 20; i < 23; i++)
		mem_swap(buf+i);

	for (i = 47; i < 255; i++)
		mem_swap(buf + i);

}

/*
 * Set the Advanced Power Management mode for the drive.   Modern hard
 * drives can have a number of power management states, ranging from
 * lowest (least performance) to highest power consumption, which results
 * in the highest performance.
 */
int ata_setapm( ATA *ata, uint32_t apm_val)
{
	int rc = 0;

	/* user inputs vale 1-126, 0x01-0xFE */

	if (apm_val > ATA_APM_MAXPERF) {
		printf("invalid APM value: must be %d-%d\n", ATA_APM_MINPERF, ATA_APM_MAXPERF);
		rc = -1;
	}

	/* allocate and initialize the ata_cmd structure */
	ata_setataparams(ata, apm_val, 0);
	ata_setfeature_param(ata, ATA_APM_ENABLE);

	if(apm_val == 0)
		ata_setfeature_param(ata, ATA_APM_DISABLE);

	/* send the APM command to the drive as a FEATURE */
	if(!rc) {
		rc = ata_cmd(ata, ATA__SETFEATURES, 0);

		if(rc) {
			perror("set APM failed");
		} else {
			printf("APM set to %d\n", apm_val);
			if(apm_val == ATA_APM_MAXPERF)
				printf("APM set to highest power consumption\n");
			else if(apm_val == ATA_APM_MINPERF)
				printf("APM set to lowest power consumption\n");
			else if(apm_val == 0)
				printf("APM Disabled\n");
		}
	}
	return rc;
}

/*
 * Sets the acoustic level on modern hard drives.   This is used to run it
 * at a lower speed/performance level, which in turn reduces noise.
 */
int ata_setacoustic(ATA *ata, uint32_t acoustic_val)
{
	int rc = 0;
	static const int aam_user_offset = 127;

	acoustic_val += aam_user_offset; /* scale it 0x80-0xFE, 128-254 */
	/* range check our acoustic level parameter */
	if (acoustic_val > ATA_AUTOACOUSTIC_MAXPERF) {
		printf("invalid acoustic value: must be %d-%d\n", 1, ATA_AUTOACOUSTIC_MAXPERF - aam_user_offset);
		rc = -1;
	}

	ata_setataparams(ata, acoustic_val, 0);
	ata_setfeature_param(ata, ATA_AUTOACOUSTIC_ENABLE);

	/* disable AAM if user specified 0, which is 127 once offset */
	if(acoustic_val == 127)
		ata_setfeature_param(ata, ATA_AUTOACOUSTIC_DISABLE);

	/* send the drive a SET_FEATURES command
	 * with a FEATURE of ATA_AUTOACOUSTIC_ENABLE
	 */
	if(!rc) {
		rc = ata_cmd( ata, ATA__SETFEATURES, 0 );

		if(rc)
			perror("set AAM failed");
		else
		{
			printf("AAM set to %d\n", acoustic_val-aam_user_offset);
			if(acoustic_val == ATA_AUTOACOUSTIC_MAXPERF)
				printf("AAM set to maximum performance\n");
			else if(acoustic_val == ATA_AUTOACOUSTIC_MINPERF)
				printf("AAM set to quietest setting\n");
			else if(acoustic_val == 127)
				printf("AAM disabled\n");
		}
	}
	return rc;
}

/* command the device to spindown after idle_mins of no disk activity */
int ata_setidletimer(ATA *ata, uint32_t idle_mins)
{
	uint16_t timer_val = 0;
	int rc = 0;

	rc = ata_getidleval( idle_mins, &timer_val );

	if(timer_val == ATA_IDLEVAL_IMMEDIATE)
		ata_setataparams(ata, 0, 0);
	else
		ata_setataparams(ata, timer_val, 0);

	/* send the IDLE command to the drive */
	if(!rc) {
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			rc = ata_cmd(ata, ATA_IDLE_IMMEDIATE, 0);
		else
			rc = ata_cmd(ata, ATA_IDLE, 0);
	}

	if(rc)
	{
		if(rc == -2)
			printf("error setting idle timeout\n");
		else
			perror("error setting idle timeout");
	}
	else
	{
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			printf("drive set to idle immediately\n");
		else if(timer_val == 0)
			printf("turned off idle timer\n");
		else
			printf("idle timer set to %d minutes\n", idle_mins);
	}
	return rc;
}

/* comand the drive to go into sleep mode */
int ata_sleep(ATA *ata)
{
	int rc = 0;

	ata_setataparams(ata, 0, 0);

	/* send the SLEEP command to the drive */
	rc = ata_cmd(ata, ATA_SLEEP, 0);

	if (rc)
	{
		perror("error setting sleep mode");
	}
	else
	{
		printf("drive set to sleep\n");
	}

	return rc;
}

/* command the device to spindown after standby_mins of no disk activity */
int ata_setstandbytimer( ATA *ata, uint32_t standby_mins)
{
	uint16_t timer_val = 0;
	int  rc = 0;

	rc = ata_getidleval( standby_mins, &timer_val );

	if (timer_val == ATA_IDLEVAL_IMMEDIATE)
		ata_setataparams(ata, 0, 0);
	else
		ata_setataparams(ata, timer_val, 0);

	/* send the STANDBY command to the drive */
	if(!rc)
	{
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			rc = ata_cmd(ata, ATA_STANDBY_IMMEDIATE, 0);
		else
			rc = ata_cmd(ata, ATA_STANDBY, 0);
	}
	if(rc)
	{
		perror("error setting idle timeout");
	} else {
		if( timer_val == ATA_IDLEVAL_IMMEDIATE )
			printf("drive set to standby immediately\n");
		else if(timer_val == 0)
			printf("turned off standby timer\n");
		else
			printf("standby timer set to %d minutes\n", standby_mins);
	}
	return rc;
}

/* this function sends an IDENTIFY command to a drive */
int ata_ident(ATA *ata, struct ata_ident * identity)
{
	int rc = 0;
	char * buf  = NULL;

	ata_setataparams(ata, 0, 0);
	ata_setdataout_params(ata, &buf, 512);
	rc = ata_cmd(ata, ATA__IDENTIFY, 0);

	if(rc)
	{
		/* then try an ATAPI IDENTIFY, maybe it didn't like an ATA_IDENTIFY */
		ata_setataparams(ata, 0, 0);
		ata_setdataout_params(ata, &buf, 512);
		rc = ata_cmd(ata, ATA__ATAPI_IDENTIFY, 0);
	}

	if (!rc)
	{
		byteswap( buf, 20, 39 ); /* serial */
		byteswap( buf, 46, 52 ); /* firmware */
		byteswap( buf, 54, 92 ); /* model */
		strpack( buf, 20, 39 );
	}

	if (is_big_endian())
		byteswap_ata_data((int16_t*)buf);

	if(!rc)
		memcpy(identity, buf, sizeof(struct ata_ident));

	return rc;
}

void hexdump(const char *data, int count)
{
	int i;
	for (i = 0; i < count; i++)
	{
		printf( "%c ", data[i]);
	}
}
