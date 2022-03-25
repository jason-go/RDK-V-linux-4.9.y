/**
@file prng.h
***************************************************************************************
*     _____   ______    _____   _    _   _____    ______            _____    _____ 
*    / ____| |  ____|  / ____| | |  | | |  __ \  |  ____|          |_   _|  / ____|
*   | (___   | |__    | |      | |  | | | |__) | | |__     ______    | |   | |     
*    \___ \  |  __|   | |      | |  | | |  _  /  |  __|   |______|   | |   | |     
*    ____) | | |____  | |____  | |__| | | | \ \  | |____            _| |_  | |____ 
*   |_____/  |______|  \_____|  \____/  |_|  \_\ |______|          |_____|  \_____|
*   
***************************************************************************************
* © Copyright 2017 Secure-IC S.A.S.
* This file is part of SIC-Trusted IP cores family from Secure-IC S.A.S. 
* This file relies on Secure-IC S.A.S. patent portfolio.
* This file cannot be used nor duplicated without prior approval from Secure-IC S.A.S.
***************************************************************************************
* File:        prng.h
* Author(s):   SECURE-IC S.A.S.
* Description: Header file for PRNG wrapper module
***************************************************************************************
* @brief Header file for PRNG wrapper module
* \attention <B>This wrapper contains function prototypes that need to be defined in the client application.</B>
*/ 
 
 
#ifndef PRNG_H
#define PRNG_H

/**
 Initialize the PRNG
 * \return 0 on success, nonzero otherwise
 */
int prng_init(void);

/**
 Get random bytes from the PRNG. Puts len random bytes into buff
 * \param buff the buffer to put the random bytes
 * \param len the number of random bytes to get
 * \return 0 on success, nonzero otherwise
 */
int prng_get_random_bytes(unsigned char * buff, unsigned int len);



#ifdef CPT_RANDOM
void print_cpt_rand_max();

void cpt_decrease(int len);

void cpt_increase(int len, int val);
#endif

#endif // PRNG_H

