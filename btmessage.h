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

/* Message codes for BT peer-to-peer messages */
enum {
  BT_MSG_CHOKE,
  BT_MSG_UNCHOKE,
  BT_MSG_INTERESTED,
  BT_MSG_NOTINTERESTED,
  BT_MSG_HAVE,
  BT_MSG_BITFIELD,
  BT_MSG_REQUEST,
  BT_MSG_PIECE,
  BT_MSG_CANCEL
};
