/* packet-ethercat-frame.c
 * Routines for ethercat packet disassembly
 *
 * $Id: packet-ethercat-frame.c 26554 2008-10-25 20:24:31Z wmeier $
 *
 * Copyright (c) 2007 by Beckhoff Automation GmbH
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

/* Include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/addr_resolv.h>
#include <epan/strutil.h>
#include <epan/etypes.h>

#include "packet-ethercat-frame.h"

/* Define the Ethercat frame proto */
static int proto_ethercat_frame = -1;

static dissector_table_t ethercat_frame_dissector_table;
static dissector_handle_t ethercat_frame_data_handle;

/* Define the tree for the EtherCAT frame */
static int ett_ethercat_frame = -1;
static int hf_ethercat_frame_length = -1;
static int hf_ethercat_frame_reserved = -1;
static int hf_ethercat_frame_type = -1;

static const value_string EthercatFrameTypes[] =
{
   { 1, "EtherCAT command", },
   { 2, "ADS", },
   { 3, "RAW-IO", },
   { 4, "NV", },
   { 0,  NULL }
};

static const value_string ethercat_frame_reserved_vals[] =
{
   { 0, "Valid"},
   { 1, "Invalid (must be zero for conformance with the protocol specification)"},
   { 0, NULL}
};

/* Ethercat Frame */
static void dissect_ethercat_frame(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
   tvbuff_t *next_tvb;
   proto_item *ti;
   proto_tree *ethercat_frame_tree;
   gint offset = 0;
   EtherCATFrameParserHDR hdr;

   if (check_col(pinfo->cinfo, COL_PROTOCOL))
   {
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "ECATF");
   }

   if (check_col(pinfo->cinfo, COL_INFO))
   {
      col_clear(pinfo->cinfo, COL_INFO);
   }

   if (tree)
   {
      ti = proto_tree_add_item(tree, proto_ethercat_frame, tvb, offset, EtherCATFrameParserHDR_Len, TRUE);
      ethercat_frame_tree = proto_item_add_subtree(ti, ett_ethercat_frame);

      proto_tree_add_item(ethercat_frame_tree, hf_ethercat_frame_length, tvb, offset, EtherCATFrameParserHDR_Len, TRUE);
      proto_tree_add_item(ethercat_frame_tree, hf_ethercat_frame_reserved, tvb, offset, EtherCATFrameParserHDR_Len, TRUE);
      proto_tree_add_item(ethercat_frame_tree, hf_ethercat_frame_type, tvb, offset, EtherCATFrameParserHDR_Len, TRUE);
   }
   hdr.hdr = tvb_get_letohs(tvb, offset);
   offset = EtherCATFrameParserHDR_Len;

   /* The EtherCAT frame header has now been processed, allow sub dissectors to
      handle the rest of the PDU. */
   next_tvb = tvb_new_subset (tvb, offset, -1, -1);

   if (!dissector_try_port(ethercat_frame_dissector_table, hdr.v.protocol,
       next_tvb, pinfo, tree))
   {
      if (check_col (pinfo->cinfo, COL_PROTOCOL))
      {
         col_add_fstr (pinfo->cinfo, COL_PROTOCOL, "0x%04x", hdr.v.protocol);
      }
      /* No sub dissector wanted to handle this payload, decode it as general
         data instead. */
      call_dissector (ethercat_frame_data_handle, next_tvb, pinfo, tree);
   }
}

void proto_register_ethercat_frame(void)
{
   static hf_register_info hf[] =
   {
      { &hf_ethercat_frame_length,
      { "Length", "ecatf.length",
      FT_UINT16, BASE_HEX, NULL, 0x07FF,
      "", HFILL }
      },

      { &hf_ethercat_frame_reserved,
      { "Reserved", "ecatf.reserved",
      FT_UINT16, BASE_HEX, VALS(&ethercat_frame_reserved_vals), 0x0800,
      "", HFILL}
      },

      { &hf_ethercat_frame_type,
      { "Type", "ecatf.type",
      FT_UINT16, BASE_HEX, VALS(EthercatFrameTypes), 0xF000,
      "E88A4 Types", HFILL }
      }
   };

   static gint *ett[] =
   {
      &ett_ethercat_frame
   };

   proto_ethercat_frame = proto_register_protocol("EtherCAT frame header",
      "ETHERCAT","ethercat");
   proto_register_field_array(proto_ethercat_frame,hf,array_length(hf));
   proto_register_subtree_array(ett, array_length(ett));

   register_dissector("ecatf", dissect_ethercat_frame, proto_ethercat_frame);

   /* Define a handle (ecatf.type) for sub dissectors that want to dissect
      the Ethercat frame ether type (E88A4) payload. */
   ethercat_frame_dissector_table = register_dissector_table("ecatf.type",
      "EtherCAT frame type", FT_UINT8, BASE_DEC);
}

void proto_reg_handoff_ethercat_frame(void)
{
   dissector_handle_t ethercat_frame_handle;

   ethercat_frame_handle = find_dissector("ecatf");
   dissector_add("ethertype", ETHERTYPE_ECATF, ethercat_frame_handle);
   dissector_add("udp.port", ETHERTYPE_ECATF, ethercat_frame_handle);
   dissector_add("tcp.port", ETHERTYPE_ECATF, ethercat_frame_handle);
   ethercat_frame_data_handle = find_dissector("data");
}
