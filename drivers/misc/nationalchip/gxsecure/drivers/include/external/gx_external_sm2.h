/**
\file sm2.h
***************************************************************************************
*     _____   ______    _____   _    _   _____    ______            _____    _____
*    / ____| |  ____|  / ____| | |  | | |  __ \  |  ____|          |_   _|  / ____|
*   | (___   | |__    | |      | |  | | | |__) | | |__     ______    | |   | |
*    \___ \  |  __|   | |      | |  | | |  _  /  |  __|   |______|   | |   | |
*    ____) | | |____  | |____  | |__| | | | \ \  | |____            _| |_  | |____
*   |_____/  |______|  \_____|  \____/  |_|  \_\ |______|          |_____|  \_____|
*
***************************************************************************************
* ? Copyright 2017 Secure-IC S.A.S.
* This file is part of SIC-Trusted IP cores family from Secure-IC S.A.S.
* This file relies on Secure-IC S.A.S. patent portfolio.
* This file cannot be used nor duplicated without prior approval from Secure-IC S.A.S.
***************************************************************************************
* File:        sm2.h
* Author(s):   SECURE-IC S.A.S.
* Description: Header file
***************************************************************************************
*/
#ifndef SM2_H
#define SM2_H




int sm2_sign(const unsigned int * const p,
                         const unsigned int * const a,
                         const unsigned int * const b,
                         unsigned int size_p_in_words,
                         const unsigned int * const n,
                         unsigned int size_n_in_words,
                         unsigned int h,
                         const unsigned int * const x_G,
                         const unsigned int * const y_G,
                         const unsigned int * const d,
                         unsigned int size_d_in_words,
                         const unsigned int * const k,
                         unsigned int size_k_in_words,
                         const unsigned int * const m,
                         unsigned int size_m_in_words,
                         unsigned int * const r,
                         unsigned int * const s);

int sm2_verif(const unsigned int * const p,
                          const unsigned int * const a,
                          const unsigned int * const b,
                          unsigned int size_p_in_words,
                          const unsigned int * const n,
                          unsigned int size_n_in_words,
                          unsigned int h,
                          const unsigned int * const x_G,
                          const unsigned int * const y_G,
                          const unsigned int * const x_P,
                          const unsigned int * const y_P,
                          const unsigned int * const r,
                          const unsigned int * const s,
                          const unsigned int * const m,
                          unsigned int size_m_in_words,
                          unsigned int * p_result);

#endif // SM2_H
