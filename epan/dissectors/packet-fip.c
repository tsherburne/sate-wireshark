/*
 * packet-fip.c
 * Routines for FIP dissection - FCoE Initialization Protocol
 * Copyright (c) 2008 Cisco Systems, Inc. (jeykholt@cisco.com)
 *
 * $Id: packet-fip.c 26647 2008-10-31 15:11:57Z stig $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Based on packet-fcoe.c, Copyright 2006, Nuova Systems, (jre@nuovasystems.com)
 * Based on packet-fcp.c, Copyright 2001, Dinesh G Dutt (ddutt@cisco.com)
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * For FIP protocol details, see http://t11.org.
 * This version uses preliminary details not yet standardized.
 * Based on http://www.t11.org/ftp/t11/pub/fc/bb-5/08-543v1.pdf
 * and      http://www.t11.org/ftp/t11/pub/fc/bb-5/08-545v1.pdf
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/etypes.h>
#include <epan/expert.h>

void proto_reg_handoff_fip(void);

/*
 * FIP protocol information.
 */
#define FIP_HEADER_LEN  10
#define FIP_BPW         4               /* bytes per descriptor length unit */

/*
 * FIP opcodes and subcodes.
 */
enum fip_opcode {
        FIP_OP_DISC =   1,              /* discovery, advertisement, etc. */
        FIP_OP_LS = 2,                  /* Link Service request or reply */
        FIP_OP_CTRL =   3,              /* control */
        FIP_OP_VLAN =   4               /* VLAN request or reply */
};

/*
 * Subcodes for FIP_OP_DISC.
 */
enum fip_disc_subcode {
        FIP_SC_SOL =    1,              /* solicitation */
        FIP_SC_ADV =    2               /* advertisement */
};

/*
 * Subcodes for FIP_OP_LS.
 */
enum fip_ls_subcode {
        FIP_SC_REQ =    1,              /* request */
        FIP_SC_REP =    2               /* reply */
};

enum fip_ctrl_subcode {
        FIP_SC_KA =     1,              /* keep-alive */
        FIP_SC_CVL =    2               /* clear virtual link */
};

enum fip_vlan_subcode {
        FIP_VL_REQ =    1,              /* request */
        FIP_VL_REP =    2               /* reply */
};

static const value_string fip_opcodes[] = {
        { FIP_OP_DISC,      "Discovery" },
        { FIP_OP_LS,        "Link Service" },
        { FIP_OP_CTRL,      "Control" },
        { FIP_OP_VLAN,      "VLAN" },
        { 0,                NULL }
};

static const value_string fip_disc_subcodes[] = {
        { FIP_SC_SOL,       "Solicitation" },
        { FIP_SC_ADV,       "Advertisement" },
        { 0,    NULL }
};

static const value_string fip_ls_subcodes[] = {
        { FIP_SC_REQ,       "ELS Request" },
        { FIP_SC_REP,       "ELS Response" },
        { 0,    NULL }
};

static const value_string fip_ctrl_subcodes[] = {
        { FIP_SC_KA,       "Keep-Alive" },
        { FIP_SC_CVL,      "Clear Virtual Link" },
        { 0,    NULL }
};

static const value_string fip_vlan_subcodes[] = {
        { FIP_VL_REQ,       "VLAN Request" },
        { FIP_VL_REP,       "VLAN Response" },
        { 0,    NULL }
};

/*
 * Descriptor types.
 */
enum fip_desc_type {
        FIP_DT_PRI =    1,              /* priority for forwarder selection */
        FIP_DT_MAC =    2,              /* MAC address */
        FIP_DT_MAP_OUI = 3,             /* FC-MAP OUI */
        FIP_DT_NAME =   4,              /* switch name or node name */
        FIP_DT_FAB =    5,              /* fabric descriptor */
        FIP_DT_FCOE_SIZE = 6,           /* max FCoE frame size */
        FIP_DT_FLOGI =  7,              /* FLOGI request or response */
        FIP_DT_FDISC =  8,              /* FDISC request or response */
        FIP_DT_LOGO =   9,              /* LOGO request or response */
        FIP_DT_ELP =    10,             /* ELP request or response */
        FIP_DT_VN =     11,             /* VN_Port Info */
        FIP_DT_FKA =    12,             /* FIP keep-alive / advert. period */
        FIP_DT_VEND =   13,             /* Vendor-specific TLV */
        FIP_DT_VLAN =   14              /* VLAN number */
};

static const value_string fip_desc_types[] = {
        { FIP_DT_PRI,   "Priority" },
        { FIP_DT_MAC,   "MAC Address" },
        { FIP_DT_MAP_OUI, "FPMA MAP OUI" },
        { FIP_DT_NAME,  "Switch or Node Name" },
        { FIP_DT_FAB,   "Fabric Descriptor" },
        { FIP_DT_FCOE_SIZE, "Max FCoE frame size" },
        { FIP_DT_FLOGI, "FLOGI Encapsulation" },
        { FIP_DT_FDISC, "FDISC Encapsulation" },
        { FIP_DT_LOGO,  "LOGO Encapsulation" },
        { FIP_DT_ELP,   "ELP Encapsulation" },
        { FIP_DT_VN,    "VN_Port Info" },
        { FIP_DT_FKA,   "FKA_ADV_Period" },
        { FIP_DT_VEND,  "Vendor_ID" },
        { FIP_DT_VLAN,  "VLAN" },
        { 0,    NULL }
};

/*
 * flags in header fip_flags.
 */
enum fip_flag {
        FIP_FL_FPMA =   0x8000,         /* supports FPMA fabric-provided MACs */
        FIP_FL_SPMA =   0x4000,         /* supports SPMA server-provided MACs */
        FIP_FL_AVAIL =  0x0004,         /* available for FLOGI */
        FIP_FL_SOL =    0x0002,         /* this is a solicited message */
        FIP_FL_FPORT =  0x0001          /* sent from an F port */
};

static int proto_fip            = -1;
static int hf_fip_ver           = -1;
static int hf_fip_op            = -1;
static int hf_fip_disc_subcode  = -1;
static int hf_fip_ls_subcode    = -1;
static int hf_fip_ctrl_subcode  = -1;
static int hf_fip_vlan_subcode  = -1;
static int hf_fip_hex_subcode   = -1;
static int hf_fip_dlen          = -1;
static int hf_fip_flags         = -1;
static int hf_fip_flag_fpma     = -1;
static int hf_fip_flag_spma     = -1;
static int hf_fip_flag_avail    = -1;
static int hf_fip_flag_sol      = -1;
static int hf_fip_flag_fport    = -1;

static const int *hf_fip_flags_fields[] = {
    &hf_fip_flag_fpma,
    &hf_fip_flag_spma,
    &hf_fip_flag_avail,
    &hf_fip_flag_sol,
    &hf_fip_flag_fport,
    NULL
};

static int hf_fip_desc_type     = -1;
static int hf_fip_desc_len      = -1;
static int hf_fip_desc_pri      = -1;
static int hf_fip_desc_mac      = -1;
static int hf_fip_desc_map      = -1;
static int hf_fip_desc_name     = -1;
static int hf_fip_desc_fab_vfid = -1;
static int hf_fip_desc_fab_map  = -1;
static int hf_fip_desc_fab_name = -1;
static int hf_fip_desc_fcoe_size = -1;
static int hf_fip_desc_vn_mac   = -1;
static int hf_fip_desc_vn_fid   = -1;
static int hf_fip_desc_vn_wwpn  = -1;
static int hf_fip_desc_fka      = -1;
static int hf_fip_desc_vend     = -1;
static int hf_fip_desc_vend_data = -1;
static int hf_fip_desc_vlan     = -1;
static int hf_fip_desc_unk      = -1;

static int ett_fip              = -1;
static int ett_fip_flags        = -1;
static int ett_fip_dt_pri       = -1;
static int ett_fip_dt_mac       = -1;
static int ett_fip_dt_map       = -1;
static int ett_fip_dt_name      = -1;
static int ett_fip_dt_fab       = -1;
static int ett_fip_dt_mdl       = -1;
static int ett_fip_dt_caps      = -1;
static int ett_fip_dt_vn        = -1;
static int ett_fip_dt_fka       = -1;
static int ett_fip_dt_vend      = -1;
static int ett_fip_dt_vlan      = -1;
static int ett_fip_dt_unk       = -1;

static dissector_handle_t fc_handle;

/*
 * Insert common descriptor type and length fields.
 */
static void
fip_desc_type_len(proto_tree *tree, tvbuff_t *tvb)
{
    proto_tree_add_item(tree, hf_fip_desc_type, tvb, 0, 1, FALSE);
    proto_tree_add_item(tree, hf_fip_desc_len, tvb, 1, 1, FALSE);
}

static void
dissect_fip(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint op;
    guint sub;
    guint rlen;
    guint flags;
    proto_item *ti;
    proto_item *item;
    proto_tree *fip_tree;
    proto_tree *subtree;
    guint dtype;
    guint dlen;
    guint desc_offset;
    guint val;
    tvbuff_t *desc_tvb;
    tvbuff_t *ls_tvb = NULL;
    const char *info;
    char *text;

    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "FIP");

    if (!tvb_bytes_exist(tvb, 0, FIP_HEADER_LEN)) {
        if (check_col(pinfo->cinfo, COL_INFO))
            col_set_str(pinfo->cinfo, COL_INFO, "[packet too short]");
        if (tree)
            proto_tree_add_protocol_format(tree, proto_fip, tvb, 0,
                                            -1, "FIP [packet too short]");
        return;
    }

    op = tvb_get_ntohs(tvb, 2);
    sub = tvb_get_guint8(tvb, 5);

    switch (op) {
    case FIP_OP_DISC:
        info = val_to_str(sub, fip_disc_subcodes, "Discovery 0x%x");
        break;
    case FIP_OP_LS:
        info = val_to_str(sub, fip_ls_subcodes, "Link Service 0x%x");
        break;
    case FIP_OP_CTRL:
        info = val_to_str(sub, fip_ctrl_subcodes, "Control 0x%x");
        break;
    case FIP_OP_VLAN:
        info = val_to_str(sub, fip_vlan_subcodes, "VLAN 0x%x");
        break;
    default:
        info = val_to_str(op, fip_opcodes, "Unknown op 0x%x");
        break;
    }

    if (check_col(pinfo->cinfo, COL_INFO))
        col_set_str(pinfo->cinfo, COL_INFO, info);

    rlen = tvb_get_ntohs(tvb, 6);
    flags = tvb_get_ntohs(tvb, 8);

    ti = proto_tree_add_protocol_format(tree, proto_fip, tvb, 0,
                                        FIP_HEADER_LEN + rlen * FIP_BPW,
                                        "FIP %s", info);
    fip_tree = proto_item_add_subtree(ti, ett_fip);
    proto_tree_add_item(fip_tree, hf_fip_ver, tvb, 0, 1, FALSE);
    proto_tree_add_item(fip_tree, hf_fip_op, tvb, 2, 2, FALSE);
    switch (op) {
    case FIP_OP_DISC:
        proto_tree_add_item(fip_tree, hf_fip_disc_subcode, tvb, 5, 1, FALSE);
        break;
    case FIP_OP_LS:
        proto_tree_add_item(fip_tree, hf_fip_ls_subcode, tvb, 5, 1, FALSE);
        break;
    case FIP_OP_CTRL:
        proto_tree_add_item(fip_tree, hf_fip_ctrl_subcode, tvb, 5, 1, FALSE);
        break;
    case FIP_OP_VLAN:
        proto_tree_add_item(fip_tree, hf_fip_vlan_subcode, tvb, 5, 1, FALSE);
        break;
    default:
        proto_tree_add_item(fip_tree, hf_fip_hex_subcode, tvb, 5, 1, FALSE);
        break;
    }
    proto_tree_add_item(fip_tree, hf_fip_dlen, tvb, 6, 2, FALSE);

    proto_tree_add_bitmask(fip_tree, tvb, 8, hf_fip_flags,
            ett_fip_flags, hf_fip_flags_fields, FALSE);

    desc_offset = FIP_HEADER_LEN;
    rlen *= FIP_BPW;
    proto_tree_add_text(fip_tree, tvb, desc_offset, rlen, "Descriptors:");

    while (rlen > 0 && tvb_bytes_exist(tvb, desc_offset, 2)) {
        dlen = tvb_get_guint8(tvb, desc_offset + 1) * FIP_BPW;
        if (!dlen) {
            proto_tree_add_text(fip_tree, tvb, desc_offset, -1,
                    "Descriptor [length error]");
            break;
        }
        if (!tvb_bytes_exist(tvb, desc_offset, dlen) || dlen > rlen) {
            break;
        }
        desc_tvb = tvb_new_subset(tvb, desc_offset, dlen, -1);
        dtype = tvb_get_guint8(desc_tvb, 0);
        desc_offset += dlen;
        rlen -= dlen;

        item = proto_tree_add_text(fip_tree, desc_tvb, 0, -1, "Descriptor: %s ",
          val_to_str(dtype, fip_desc_types, "Unknown 0x%x"));

        switch (dtype) {
        case FIP_DT_PRI:
            subtree = proto_item_add_subtree(item, ett_fip_dt_pri);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_pri, desc_tvb,
                    3, 1, FALSE);
            proto_item_append_text(item, "%u", tvb_get_guint8(desc_tvb, 3));
            break;
        case FIP_DT_MAC:
            subtree = proto_item_add_subtree(item, ett_fip_dt_mac);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_mac, desc_tvb,
                    2, 6, FALSE);
            proto_item_append_text(item, "%s",
                    tvb_bytes_to_str_punct(desc_tvb, 2, 6, ':'));
            break;
        case FIP_DT_MAP_OUI:
            subtree = proto_item_add_subtree(item, ett_fip_dt_map);
            fip_desc_type_len(subtree, desc_tvb);
            text = fc_to_str(tvb_get_ptr(desc_tvb, 5, 3));
            proto_tree_add_string(subtree, hf_fip_desc_map, desc_tvb,
                    5, 3, text);
            proto_item_append_text(item, "%s", text);
            break;
        case FIP_DT_NAME:
            subtree = proto_item_add_subtree(item, ett_fip_dt_name);
            fip_desc_type_len(subtree, desc_tvb);
            text = fcwwn_to_str(tvb_get_ptr(desc_tvb, 4, 8));
            proto_tree_add_string(subtree, hf_fip_desc_name,
                    desc_tvb, 4, 8, text);
            proto_item_append_text(item, "%s", text);
            break;
        case FIP_DT_FAB:
            subtree = proto_item_add_subtree(item, ett_fip_dt_fab);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_fab_vfid, desc_tvb,
                    2, 2, FALSE);
            text = fc_to_str(tvb_get_ptr(desc_tvb, 5, 3));
            proto_tree_add_string(subtree, hf_fip_desc_fab_map, desc_tvb,
                    5, 3, text);
            text = fcwwn_to_str(tvb_get_ptr(desc_tvb, 8, 8));
            proto_tree_add_string(subtree, hf_fip_desc_fab_name,
                    desc_tvb, 8, 8, text);
            proto_item_append_text(item, "%s", text);
            break;
        case FIP_DT_FCOE_SIZE:
            subtree = proto_item_add_subtree(item, ett_fip_dt_mdl);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_fcoe_size, desc_tvb,
                    2, 2, FALSE);
            proto_item_append_text(item, "%u", tvb_get_ntohs(desc_tvb, 2));
            break;
        case FIP_DT_FLOGI:
        case FIP_DT_FDISC:
        case FIP_DT_LOGO:
        case FIP_DT_ELP:
            subtree = proto_item_add_subtree(item, ett_fip_dt_caps);
            fip_desc_type_len(subtree, desc_tvb);
            ls_tvb = tvb_new_subset(desc_tvb, 4, dlen - 4, -1);
            call_dissector(fc_handle, ls_tvb, pinfo, subtree);
            proto_item_append_text(item, "%u bytes", dlen - 4);
            break;
        case FIP_DT_VN:
            subtree = proto_item_add_subtree(item, ett_fip_dt_vn);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_vn_mac, desc_tvb,
                    2, 6, FALSE);
            proto_tree_add_item(subtree, hf_fip_desc_vn_fid, desc_tvb,
                    9, 3, FALSE);
            text = fcwwn_to_str(tvb_get_ptr(desc_tvb, 12, 8));
            proto_tree_add_string(subtree, hf_fip_desc_vn_wwpn,
                    desc_tvb, 12, 8, text);
            proto_item_append_text(item, "MAC %s  FC_ID %6.6x",
                    tvb_bytes_to_str_punct(desc_tvb, 2, 6, ':'),
                    tvb_get_ntoh24(desc_tvb, 9));
            break;
        case FIP_DT_FKA:
            subtree = proto_item_add_subtree(item, ett_fip_dt_fka);
            fip_desc_type_len(subtree, desc_tvb);
            val = tvb_get_ntohl(desc_tvb, 4);
            proto_tree_add_uint_format_value(subtree, hf_fip_desc_fka,
                    desc_tvb, 4, 4, val, "%u ms", val);
            proto_item_append_text(item, "%u ms", val);
            break;
        case FIP_DT_VEND:
            subtree = proto_item_add_subtree(item, ett_fip_dt_vend);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_vend, desc_tvb,
                    4, 8, FALSE);
            if (tvb_bytes_exist(desc_tvb, 9, -1)) {
                proto_tree_add_item(subtree, hf_fip_desc_vend_data,
                     desc_tvb, 9, -1, FALSE);
            }
            break;
        case FIP_DT_VLAN:
            subtree = proto_item_add_subtree(item, ett_fip_dt_vlan);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_vlan, desc_tvb,
                    2, 2, FALSE);
            proto_item_append_text(item, "%u", tvb_get_ntohs(desc_tvb, 2));
            break;
        default:
            subtree = proto_item_add_subtree(item, ett_fip_dt_unk);
            fip_desc_type_len(subtree, desc_tvb);
            proto_tree_add_item(subtree, hf_fip_desc_unk, desc_tvb,
                    2, -1, FALSE);
            break;
        }
    }
}

void
proto_register_fip(void)
{
    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
        /*
         * FIP header fields.
         */
        { &hf_fip_ver,
          {"Version", "fip.ver", FT_UINT8, BASE_DEC,
            NULL, 0xf0, "", HFILL}},
        { &hf_fip_op,
          {"Opcode", "fip.opcode", FT_UINT16, BASE_HEX,
           VALS(&fip_opcodes), 0, "", HFILL}},
        { &hf_fip_disc_subcode,
          {"Discovery Subcode", "fip.disc_subcode", FT_UINT8, BASE_HEX,
           VALS(&fip_disc_subcodes), 0, "", HFILL}},
        { &hf_fip_ls_subcode,
          {"Link Service Subcode", "fip.ls.subcode", FT_UINT8, BASE_HEX,
           VALS(&fip_ls_subcodes), 0, "", HFILL}},
        { &hf_fip_ctrl_subcode,
          {"Control Subcode", "fip.ctrl_subcode", FT_UINT8, BASE_HEX,
           VALS(&fip_ctrl_subcodes), 0, "", HFILL}},
        { &hf_fip_vlan_subcode,
          {"VLAN Subcode", "fip.vlan_subcode", FT_UINT8, BASE_HEX,
           VALS(&fip_vlan_subcodes), 0, "", HFILL}},
        { &hf_fip_hex_subcode,
          {"Unknown Subcode", "fip.subcode", FT_UINT8, BASE_HEX,
           NULL, 0, "", HFILL}},
        { &hf_fip_dlen,
          {"Length of Descriptors (words)", "fip.dl_len", FT_UINT16, BASE_DEC,
            NULL, 0, "", HFILL}},
        { &hf_fip_flags,
          {"Flags", "fip.flags", FT_UINT16, BASE_HEX,
            NULL, 0, "", HFILL}},
        { &hf_fip_flag_fpma,
          {"Fabric Provided MAC addr", "fip.flags.fpma", FT_BOOLEAN, 16,
            NULL, FIP_FL_FPMA, "", HFILL}},
        { &hf_fip_flag_spma,
          {"Server Provided MAC addr", "fip.flags.spma", FT_BOOLEAN, 16,
            NULL, FIP_FL_SPMA, "", HFILL}},
        { &hf_fip_flag_avail,
          {"Available", "fip.flags.available", FT_BOOLEAN, 16,
            NULL, FIP_FL_AVAIL, "", HFILL}},
        { &hf_fip_flag_sol,
          {"Solicited", "fip.flags.sol", FT_BOOLEAN, 16,
            NULL, FIP_FL_SOL, "", HFILL}},
        { &hf_fip_flag_fport,
          {"F_Port", "fip.flags.fport", FT_BOOLEAN, 16,
            NULL, FIP_FL_FPORT, "", HFILL}},
        { &hf_fip_desc_type,
          {"Descriptor Type", "fip.desc_type", FT_UINT8, BASE_HEX,
            VALS(&fip_desc_types), 0, "", HFILL}},
        { &hf_fip_desc_len,
          {"Descriptor Length (words)", "fip.desc_len", FT_UINT8, BASE_DEC,
            NULL, 0, "", HFILL}},
        /*
         * Various descriptor fields.
         */
        { &hf_fip_desc_pri,
          {"Priority", "fip.pri", FT_UINT8, BASE_DEC, NULL, 0, "", HFILL}},
        { &hf_fip_desc_mac,
          {"MAC Address", "fip.mac", FT_ETHER, 0, NULL, 0, "", HFILL}},
        { &hf_fip_desc_map,
          {"FC-MAP-OUI", "fip.map", FT_BYTES, BASE_HEX, NULL, 0, "", HFILL}},
        { &hf_fip_desc_name,
          {"Switch or Node Name", "fip.name", FT_STRING, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_fab_vfid,
          {"VFID Descriptor", "fip.fab.vfid", FT_STRING, BASE_DEC, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_fab_map,
          {"FC-MAP", "fip.fab.map", FT_STRING, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_fab_name,
          {"Fabric Descriptor", "fip.fab.name", FT_STRING, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_fcoe_size,
          {"Max FCoE frame size", "fip.fcoe_size", FT_UINT16, BASE_DEC, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_vn_mac,
          {"VN_Port MAC Address", "fip.vn.mac", FT_ETHER, 0, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_vn_fid,
          {"VN_Port FC_ID", "fip.vn.fc_id", FT_UINT32, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_vn_wwpn,
          {"Port Name", "fip.vn.pwwn", FT_STRING, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_fka,
          {"FKA_ADV_Period", "fip.fka", FT_UINT32, BASE_DEC, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_vend,
          {"Vendor-ID", "fip.vendor", FT_BYTES, BASE_HEX, NULL, 0, "", HFILL}},
        { &hf_fip_desc_vend_data,
          {"Vendor-specific data", "fip.vendor.data", FT_BYTES, BASE_HEX, NULL,
            0, "", HFILL}},
        { &hf_fip_desc_vlan,
          {"VLAN", "fip.vlan", FT_UINT16, BASE_DEC, NULL, 0, "", HFILL}},
        { &hf_fip_desc_unk,
          {"Unknown Descriptor", "fip.desc", FT_BYTES, BASE_HEX, NULL,
            0, "", HFILL}}
    };
    static gint *ett[] = {
        &ett_fip,
        &ett_fip_flags,
        &ett_fip_dt_pri,
        &ett_fip_dt_mac,
        &ett_fip_dt_map,
        &ett_fip_dt_name,
        &ett_fip_dt_fab,
        &ett_fip_dt_mdl,
        &ett_fip_dt_caps,
        &ett_fip_dt_vn,
        &ett_fip_dt_fka,
        &ett_fip_dt_vend,
        &ett_fip_dt_vlan,
        &ett_fip_dt_unk
    };

    /* Register the protocol name and description */
    proto_fip = proto_register_protocol("FCoE Initialization Protocol",
        "FIP", "fip");

    /* Required function calls to register the header fields and
     * subtrees used */
    proto_register_field_array(proto_fip, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

/*
 * This function name is required because a script is used to find these
 * routines and create the code that calls these routines.
 */
void
proto_reg_handoff_fip(void)
{
    dissector_handle_t fip_handle;

    fip_handle = create_dissector_handle(dissect_fip, proto_fip);
    dissector_add("ethertype", ETHERTYPE_FIP, fip_handle);
    fc_handle = find_dissector("fc");
}