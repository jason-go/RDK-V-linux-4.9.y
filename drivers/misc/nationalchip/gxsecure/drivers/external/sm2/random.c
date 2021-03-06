#define RAND_MAX 2147483647
/*
 * This generator is a combination of three linear congruential generators
 * with periods or 2^15-405, 2^15-1041 and 2^15-1111. It has a period that
 * is the product of these three numbers.
 */

#include "random.h"
static long int seed1 = 1;
static long int seed2 = 1;
static long int seed3 = 1;

#define CRANK(a,b,c,m,s) 	\
	q = s/a;		\
	s = b*(s-a*q) - c*q;	\
	if(s<0) s+=m;

long int random(void)
{
	register long int q;

	CRANK(206, 157, 31, 32363, seed1);
	CRANK(217, 146, 45, 31727, seed2);
	CRANK(222, 142, 133, 31657, seed3);

	return seed1 ^ seed2 ^ seed3;
}

void srandom(unsigned int seed)
{
	seed &= RAND_MAX;
	seed1 = seed % 32362 + 1;
	seed2 = seed % 31726 + 1;
	seed3 = seed % 31656 + 1;
}
