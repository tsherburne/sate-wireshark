/* packet-mac-lte.h
 *
 * Martin Mathieson
 * $Id: packet-mac-lte.h 28342 2009-05-13 09:36:42Z martinm $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* radioType */
#define FDD_RADIO 1
#define TDD_RADIO 2

/* Direction */
#define DIRECTION_UPLINK   0
#define DIRECTION_DOWNLINK 1

/* rntiType */
#define NO_RNTI 0
#define P_RNTI  1
#define RA_RNTI 2
#define C_RNTI  3
#define SI_RNTI 4


/* Context info attached to each LTE MAC frame */
typedef struct mac_lte_info
{
    /* Needed for decode */
    guint8          radioType;
    guint8          direction;
    guint8          rntiType;

    /* Extra info to display */
    guint16         rnti;
    guint16         ueid;
    guint16         subframeNumber;
    guint8          isPredefinedData;
    guint16         length;
    guint8          reTxCount;
    guint8          crcStatusValid;
    guint8          crcStatus;
} mac_lte_info;


typedef struct mac_lte_tap_info {
    /* Info from context */
    guint16  rnti;
    guint8   rntiType;
    guint8   isPredefinedData;
    guint8   reTxCount;
    guint8   crcStatusValid;
    guint8   crcStatus;
    guint8   direction;

    /* Number of bytes (which part is used depends upon context settings) */
    guint32  single_number_of_bytes;
    guint32  bytes_for_lcid[11];
    guint32  sdus_for_lcid[11];
    guint8   number_of_rars;
} mac_lte_tap_info;

