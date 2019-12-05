//////////////////////////////////////////////////////
// ----------- crc16.h -----------------
// Calculate CRC16 functions header
//
// By Papa Karlo Software, 2019
//
// ---------------------------------------------

#include <stdint.h>


//Returns CRC16-CCITT calculated for a block of 'length' bytes starting at
//'source_adr'
uint16_t crc16(void *source_adr, uint32_t length);

//Performs partial CRC16 calculation on a part of a (long) block. The part
//starts at 'source_adr'; 'length' is the length of the part in bytes. For
//the first part 'crc' must be the initial CRC Register value (0xFFFF if standard
//CRC16-CCITT is used); for any succeeding part 'crc' must be the value returned
//by call to this function that handled the previous part. Value returned by
//calculation on the last part is the CRC of the whole block. It's OK to make
//partial CRC calculations on different blocks alternately or call another
//CRC functions between any partial CRC calculations
uint16_t crc16_part(uint16_t crc, void *source_adr, uint32_t length);

//Calculates CRC16-CCITT for a block of 'length' bytes starting at
//'source_adr' and appends the result to the block (first high byte,
//then low byte).
//Returns the calculated value
uint16_t crc16_add(void *source_adr, uint32_t length);

//Calculates CRC16-CCITT for a block of 'length' bytes starting at
//'source_adr'.
//If the result is 0, returns 1, otherwise returns 0
uint8_t	crc16_OK(void *source_adr, uint32_t length);


