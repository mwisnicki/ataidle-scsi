#ifndef ATADEFS_H
#define ATADEFS_H

#include <stdint.h>

/** Enumerate ATA commands so I can switch() on them */
/* XXX conflicts with definitions, rename all to ATA_CMD_* */
enum ata_command {
#ifndef ATA_SLEEP
    ATA_SLEEP			= 0xE6,
#endif
#ifndef __FreeBSD__
    ATA_STANDBY_IMMEDIATE	= 0xE0,
    ATA_IDLE_IMMEDIATE		= 0xE1,
#endif
    ATA__SETFEATURES		= 0xEF,
    ATA__IDENTIFY		= 0xEC,
    ATA__ATAPI_IDENTIFY		= 0xA1,
    ATA_IDLE			= 0xE3,
    ATA_STANDBY			= 0xE2
};

enum ata_feature {
    ATA_AUTOACOUSTIC_ENABLE 	= 0x42,
    ATA_AUTOACOUSTIC_DISABLE	= 0xC2,
    ATA_APM_ENABLE		= 0x05,
    ATA_APM_DISABLE		= 0x85
};

enum ata_constant {
    ATA_AUTOACOUSTIC_MAXPERF 	= 0xFE,
    ATA_AUTOACOUSTIC_MINPERF 	= 0x80,
    ATA_APM_MINPOWER_NO_STANDBY = 0x80,
    ATA_APM_MINPERF		= 0x01,
    ATA_APM_MAXPERF		= 0xFE,
    ATA_CMD_TIMEOUT		= 10,
    ATA_IDLEVAL_IMMEDIATE	= 900
};

enum ata_protocol {
    ATA_PROT_HARD_RESET		= 0, /* HRST */
    ATA_PROT_SOFT_RESET		= 1, /* SRST */
    ATA_PROT_RESERVED0		= 2,
    ATA_PROT_NON_DATA		= 3,
    ATA_PROT_PIO_DATA_IN	= 4,
    ATA_PROT_PIO_DATA_OUT	= 5,
    ATA_PROT_DMA		= 6,
    ATA_PROT_DMA_QUEUED		= 7,
    ATA_PROT_DEVICE_DIAG	= 8,
    ATA_PROT_DEVICE_RESET	= 9,
    ATA_PROT_UDMA_DATA_IN	= 10,
    ATA_PROT_UDMA_DATA_OUT	= 11,
    ATA_PROT_FPDMA		= 12,
    ATA_PROT_RESERVED1		= 13,
    ATA_PROT_RESERVED2		= 14,
    ATA_PROT_RET_RESP_INFO	= 15
};

#endif /* ATADEFS_H */
