#ifndef ATADEFS_H
#define ATADEFS_H

#include <stdint.h>

static const uint32_t ATA__SETFEATURES		= 0xEF;
static const uint32_t ATA__IDENTIFY		= 0xEC;
static const uint32_t ATA__ATAPI_IDENTIFY	= 0xA1;
static const uint32_t ATA_IDLE			= 0xE3;
static const uint32_t ATA_STANDBY		= 0xE2;

#ifndef ATA_SLEEP
static const uint32_t ATA_SLEEP			= 0xE6;
#endif

static const uint32_t ATA_AUTOACOUSTIC_ENABLE 	= 0x42;
static const uint32_t ATA_AUTOACOUSTIC_DISABLE	= 0xC2;
static const uint32_t ATA_APM_ENABLE		= 0x05;
static const uint32_t ATA_APM_DISABLE		= 0x85;
static const uint32_t ATA_AUTOACOUSTIC_MAXPERF 	= 0xFE;
static const uint32_t ATA_AUTOACOUSTIC_MINPERF 	= 0x80;
static const uint32_t ATA_APM_MINPOWER_NO_STANDBY = 0x80;
static const uint32_t ATA_APM_MINPERF		= 0x01;
static const uint32_t ATA_APM_MAXPERF		= 0xFE;
static const uint32_t ATA_POWERSTATUS_GET	= 0xE5;
static const uint32_t ATA_CMD_TIMEOUT		= 10;
static const uint32_t ATA_IDLEVAL_IMMEDIATE	= 900;

#ifndef __FreeBSD__
static const uint32_t ATA_STANDBY_IMMEDIATE	= 0xE0;
static const uint32_t ATA_IDLE_IMMEDIATE	= 0xE1;
#endif

#endif /* ATADEFS_H */
