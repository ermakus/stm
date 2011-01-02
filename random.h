/* 
 * Copyright 2010 Anton Ermak, All Right Reserved
 *
 * Based on libbt, Copyright 2003,2004,2005 Kevin Smathers
 *
 * Redistribution of this file in either its original form, or in an
 * updated form may be done under the terms of the GNU GENERAL PUBLIC LICENSE. 
 *
 * If this license is unacceptable to you then you
 * may not redistribute this work.
 * 
 * See the file COPYING for details.
 */

#ifndef __RANDOM__H
#define __RANDOM__H

#ifdef __cplusplus
extern "C" {
#endif

void randomid( char *buf, int bytes);
int rnd( int range);

#ifdef __cplusplus
}
#endif

#endif
