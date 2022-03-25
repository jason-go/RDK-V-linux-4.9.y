/**
 Initialize the PRNG
 * \return 0 on success, nonzero otherwise
 */
#include "prng.h"
#include "rand.h"
#include "random.h"

int prng_init(void)
{
	unsigned int seed;
	
	*(volatile unsigned int*)0x82804004 = 1; // check len
	seed = rand();
	srandom(seed);
	return 0;
}
/**
 Get random bytes from the PRNG. Puts len random bytes into buff
 * \param buff the buffer to put the random bytes
 * \param len the number of random bytes to get
 * \return 0 on success, nonzero otherwise
 */
int prng_get_random_bytes(unsigned char * buff, unsigned int len)
{
	unsigned int i;
	unsigned int rand_data;
    
	*(volatile unsigned int*)0x82804008 = len; // check len
	for(i=0;i<len;i++)
	{
		rand_data = rand();
		*(buff+i) = (unsigned char)rand_data;
		//buff[i] = (unsigned char)rand_data;
		*(volatile unsigned int*)0x8280400c = (unsigned char)rand_data; // check random data
	}

 	return 0;
	
}
