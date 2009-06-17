#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define ATAIDLE_VERSION "2.4"

void 	usage(void);
int	ata_strtolong( const char *src, long * dest );
int	ata_getidleval( uint32_t idle_mins, uint16_t *timer_val );
char*	ata_getversionstring(uint16_t ata_version);
void	byteswap(char * buf, int from, int to);
void	strpack(char * buf, int from, int to);
bool	checkargs( int argc, char ** argv, const char *optstr, bool * needchandev );
void	byteswap_ata_data( int16_t * buf );
void	hexdump( const char *data, int count );
void	mem_swap(int16_t * val);

#endif /* UTIL_H */
