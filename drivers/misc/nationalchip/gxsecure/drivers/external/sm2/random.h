
/*
 * This generator is a combination of three linear congruential generators
 * with periods or 2^15-405, 2^15-1041 and 2^15-1111. It has a period that
 * is the product of these three numbers.
 */

long int random(void);

void srandom(unsigned int seed);



