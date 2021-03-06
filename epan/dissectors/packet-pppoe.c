/* packet-pppoe.c
 * Routines for PPP Over Ethernet (PPPoE) packet disassembly (RFC2516)
 * Up to date with http://www.iana.org/assignments/pppoe-parameters (2008-04-30)
 *
 * $Id: packet-pppoe.c 28220 2009-05-01 07:17:10Z jake $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/strutil.h>
#include <epan/etypes.h>
#include <epan/prefs.h>
#include <epan/expert.h>

static int proto_pppoed = -1;

/* Common to session and discovery protocols */
static gint hf_pppoe_version = -1;
static gint hf_pppoe_type = -1;
static gint hf_pppoe_code = -1;
static gint hf_pppoe_session_id = -1;
static gint hf_pppoe_payload_length = -1;

/* Discovery protocol fields */
static gint hf_pppoed_tags = -1;
static gint hf_pppoed_tag = -1;
static gint hf_pppoed_tag_length = -1;
static gint hf_pppoed_tag_unknown_data = -1;
static gint hf_pppoed_tag_service_name = -1;
static gint hf_pppoed_tag_ac_name = -1;
static gint hf_pppoed_tag_host_uniq = -1;
static gint hf_pppoed_tag_ac_cookie = -1;
static gint hf_pppoed_tag_vendor_id = -1;
static gint hf_pppoed_tag_vendor_unspecified = -1;
static gint hf_pppoed_tag_credits = -1;
static gint hf_pppoed_tag_credits_fcn = -1;
static gint hf_pppoed_tag_credits_bcn = -1;
static gint hf_pppoed_tag_metrics = -1;
static gint hf_pppoed_tag_metrics_r = -1;
static gint hf_pppoed_tag_metrics_rlq = -1;
static gint hf_pppoed_tag_metrics_resource = -1;
static gint hf_pppoed_tag_metrics_latency = -1;
static gint hf_pppoed_tag_metrics_curr_drate = -1;
static gint hf_pppoed_tag_metrics_max_drate = -1;
static gint hf_pppoed_tag_mdr_units = -1;
static gint hf_pppoed_tag_cdr_units = -1;
static gint hf_pppoed_tag_seq_num = -1;
static gint hf_pppoed_tag_cred_scale = -1;
static gint hf_pppoed_tag_relay_session_id = -1;
static gint hf_pppoed_tag_hurl = -1;
static gint hf_pppoed_tag_motm = -1;
static gint hf_pppoed_tag_max_payload = -1;
static gint hf_pppoed_tag_ip_route_add = -1;
static gint hf_pppoed_tag_service_name_error = -1;
static gint hf_pppoed_tag_ac_system_error = -1;
static gint hf_pppoed_tag_generic_error = -1;

/* Session protocol fields */
static gint hf_pppoes_tags = -1;
static gint hf_pppoes_tag = -1;
static gint hf_pppoes_tag_credits = -1;
static gint hf_pppoes_tag_credits_fcn = -1;
static gint hf_pppoes_tag_credits_bcn = -1;

/* Session protocol fields */

static gint ett_pppoed = -1;
static gint ett_pppoed_tags = -1;

static int proto_pppoes = -1;

static gint ett_pppoes = -1;
static gint ett_pppoes_tags = -1;

/* PPPoE parent fields */

static int proto_pppoe = -1;
static gint ett_pppoe = -1;


/* Handle for calling for ppp dissector to handle session data */
static dissector_handle_t ppp_handle;


/* Preference for showing discovery tag values and lengths */
static gboolean global_pppoe_show_tags_and_lengths = FALSE;


#define PPPOE_CODE_SESSION    0x00
#define PPPOE_CODE_PADO       0x07
#define PPPOE_CODE_PADI       0x09
#define PPPOE_CODE_PADG       0x0a
#define PPPOE_CODE_PADC       0x0b
#define PPPOE_CODE_PADQ       0x0c
#define PPPOE_CODE_PADR       0x19
#define PPPOE_CODE_PADS       0x65
#define PPPOE_CODE_PADT       0xa7
#define PPPOE_CODE_PADM       0xd3
#define PPPOE_CODE_PADN       0xd4

#define PPPOE_TAG_EOL         0x0000
#define PPPOE_TAG_SVC_NAME    0x0101
#define PPPOE_TAG_AC_NAME     0x0102
#define PPPOE_TAG_HOST_UNIQ   0x0103
#define PPPOE_TAG_AC_COOKIE   0x0104
#define PPPOE_TAG_VENDOR      0x0105
#define PPPOE_TAG_CREDITS     0x0106
#define PPPOE_TAG_METRICS     0x0107
#define PPPOE_TAG_SEQ_NUM     0x0108
#define PPPOE_TAG_CRED_SCALE  0x0109
#define PPPOE_TAG_RELAY_ID    0x0110
#define PPPOE_TAG_HURL        0x0111
#define PPPOE_TAG_MOTM        0x0112
#define PPPOE_TAG_MAX_PAYLD   0x0120
#define PPPOE_TAG_IP_RT_ADD   0x0121
#define PPPOE_TAG_SVC_ERR     0x0201
#define PPPOE_TAG_AC_ERR      0x0202
#define PPPOE_TAG_GENERIC_ERR 0x0203

#define PPPOE_CDR_MASK        0x06
#define PPPOE_MDR_MASK        0x18
#define PPPOE_RCV_ONLY_MASK   0x01

#define PPPOE_SCALE_KBPS      0x00
#define PPPOE_SCALE_MBPS      0x01
#define PPPOE_SCALE_GBPS      0x02
#define PPPOE_SCALE_TBPS      0x03


static const value_string code_vals[] = {
		{PPPOE_CODE_SESSION, "Session Data"                             },
		{PPPOE_CODE_PADO, "Active Discovery Offer (PADO)"               },
		{PPPOE_CODE_PADI, "Active Discovery Initiation (PADI)"          },
		{PPPOE_CODE_PADG, "Active Discovery Session-Grant (PADG)"       },
		{PPPOE_CODE_PADC, "Active Discovery Session-Credit Resp.(PADC)" },
		{PPPOE_CODE_PADQ, "Active Discovery Quality (PADQ)"             },
		{PPPOE_CODE_PADR, "Active Discovery Request (PADR)"             },
		{PPPOE_CODE_PADS, "Active Discovery Session-confirmation (PADS)"},
		{PPPOE_CODE_PADT, "Active Discovery Terminate (PADT)"           },
		{PPPOE_CODE_PADM, "Active Discovery Message (PADM)"             },
		{PPPOE_CODE_PADN, "Active Discovery Network (PADN)"             },
		{0,               NULL                                          }
};


static const value_string tag_vals[] = {
		{PPPOE_TAG_EOL,        "End-Of-List"       },
		{PPPOE_TAG_SVC_NAME,   "Service-Name"      },
		{PPPOE_TAG_AC_NAME,    "AC-Name"           },
		{PPPOE_TAG_HOST_UNIQ,  "Host-Uniq"         },
		{PPPOE_TAG_AC_COOKIE,  "AC-Cookie"         },
		{PPPOE_TAG_VENDOR,     "Vendor-Specific"   },
		{PPPOE_TAG_CREDITS,    "Credits"           },
		{PPPOE_TAG_METRICS,    "Metrics"           },
		{PPPOE_TAG_SEQ_NUM,    "Sequence Number"    },
		{PPPOE_TAG_CRED_SCALE, "Credit Scale Factor"},
		{PPPOE_TAG_RELAY_ID,   "Relay-Session-Id"  },
		{PPPOE_TAG_HURL,       "HURL"              },
		{PPPOE_TAG_MOTM,       "MOTM"              },
		{PPPOE_TAG_MAX_PAYLD,  "PPP-Max-Payload"   },
		{PPPOE_TAG_IP_RT_ADD,  "IP Route Add"      },
		{PPPOE_TAG_SVC_ERR,    "Service-Name-Error"},
		{PPPOE_TAG_AC_ERR,     "AC-System-Error"   },
		{PPPOE_TAG_GENERIC_ERR,"Generic-Error"     },
		{0,                    NULL                }
};

const value_string datarate_scale_vals[] = {
                {PPPOE_SCALE_KBPS,	"kilobits per second"},
                {PPPOE_SCALE_MBPS,	"megabits per second"},
                {PPPOE_SCALE_GBPS,	"gigabits per second"},
                {PPPOE_SCALE_TBPS,	"terabits per second"},
		{0,			NULL                 }
};


/* Dissect discovery protocol tags */
static void
dissect_pppoe_tags(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree,
                   int payload_length)
{
	guint16 poe_tag;
	guint16 poe_tag_length;
	int tagstart;
        guint16 poe_rsv = 0;

	proto_tree  *pppoe_tree;
	proto_item  *ti;

	/* Start Decoding Here. */
	if (tree)
	{
		/* Create tags subtree */
		ti = proto_tree_add_item(tree, hf_pppoed_tags, tvb, offset, payload_length-6, FALSE);
		pppoe_tree = proto_item_add_subtree(ti, ett_pppoed_tags);

		tagstart = offset;

		/* Loop until all data seen or End-Of-List tag found */
		while (tagstart <= payload_length-2)
		{
			poe_tag = tvb_get_ntohs(tvb, tagstart);
			poe_tag_length = tvb_get_ntohs(tvb, tagstart + 2);

			/* Tag value and data length */
			if (global_pppoe_show_tags_and_lengths)
			{
				proto_tree_add_item(pppoe_tree, hf_pppoed_tag, tvb, tagstart, 2, FALSE);
				proto_tree_add_item(pppoe_tree, hf_pppoed_tag_length, tvb, tagstart+2, 2, FALSE);
			}

			/* Show tag data */
			switch (poe_tag)
			{
				case PPPOE_TAG_SVC_NAME:
					if (poe_tag_length > 0)
					{
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_service_name, tvb,
						                    tagstart+4, poe_tag_length, FALSE);
					}
					break;
				case PPPOE_TAG_AC_NAME:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_ac_name, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					/* Show AC-Name in info column */
					if (check_col(pinfo->cinfo, COL_INFO))
					{
						col_append_fstr(pinfo->cinfo, COL_INFO, " AC-Name='%s'",
						               tvb_get_ephemeral_string(tvb, tagstart+4, poe_tag_length));
					}
					break;
				case PPPOE_TAG_HOST_UNIQ:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_host_uniq, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_AC_COOKIE:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_ac_cookie, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_VENDOR:
					if (poe_tag_length >= 4)
					{
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_vendor_id, tvb,
											tagstart+4, 4, FALSE);
					}
					if (poe_tag_length > 4)
					{
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_vendor_unspecified, tvb,
						                    tagstart+4+4, poe_tag_length-4, FALSE);
					}
					break;
				case PPPOE_TAG_CREDITS:
					if (poe_tag_length == 4)
					{
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_credits_fcn, tvb,
						                    tagstart+4, 2, FALSE);
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_credits_bcn, tvb,
						                    tagstart+6, 2, FALSE);
					} else {
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_credits, tvb,
						                    tagstart+4, poe_tag_length, FALSE);
					}
					break;
				case PPPOE_TAG_METRICS:
					if (poe_tag_length == 10)
					{
                                                poe_rsv = tvb_get_ntohs(tvb, tagstart+4);

                                                proto_tree_add_item(pppoe_tree, hf_pppoed_tag_mdr_units, tvb,
                                                                    tagstart+4, 2, FALSE);
                                                proto_tree_add_item(pppoe_tree, hf_pppoed_tag_cdr_units, tvb,
                                                                    tagstart+4, 2, FALSE);
                                                proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_r, tvb,
						                    tagstart+4, 2, FALSE);
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_rlq, tvb,
						                    tagstart+6, 1, FALSE);
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_resource, tvb,
						                    tagstart+7, 1, FALSE);
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_latency, tvb,
						                    tagstart+8, 2, FALSE);

                                                /* CDR */
						ti = proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_curr_drate, tvb,
                                                                         tagstart+10, 2, FALSE);

                                                switch ((poe_rsv & PPPOE_CDR_MASK) >> 1)
                                                {
                                                case (PPPOE_SCALE_KBPS):
                                                    proto_item_append_text(ti, " kbps");
                                                    break;
                                                case (PPPOE_SCALE_MBPS):
                                                    proto_item_append_text(ti, " mbps");
                                                    break;
                                                case (PPPOE_SCALE_GBPS):
                                                    proto_item_append_text(ti, " gbps");
                                                    break;
                                                case (PPPOE_SCALE_TBPS):
                                                    proto_item_append_text(ti, " tbps");
                                                    break;
                                                }

                                                /* MDR */
						ti = proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics_max_drate, tvb,
						                    tagstart+12, 2, FALSE);

                                                switch ((poe_rsv & PPPOE_MDR_MASK) >> 3)
                                                {
                                                case (PPPOE_SCALE_KBPS):
                                                    proto_item_append_text(ti, " kbps");
                                                    break;
                                                case (PPPOE_SCALE_MBPS):
                                                    proto_item_append_text(ti, " mbps");
                                                    break;
                                                case (PPPOE_SCALE_GBPS):
                                                    proto_item_append_text(ti, " gbps");
                                                    break;
                                                case (PPPOE_SCALE_TBPS):
                                                    proto_item_append_text(ti, " tbps");
                                                    break;
                                                }

					} else {
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_metrics, tvb,
						                    tagstart+4, poe_tag_length, FALSE);
					}
					break;
				case PPPOE_TAG_SEQ_NUM:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_seq_num, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
                                case PPPOE_TAG_CRED_SCALE:
                                        proto_tree_add_item(pppoe_tree, hf_pppoed_tag_cred_scale, tvb,
                                                            tagstart+4, poe_tag_length, FALSE);
                                        break;
				case PPPOE_TAG_RELAY_ID:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_relay_session_id, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_HURL:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_hurl, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_MOTM:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_motm, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_MAX_PAYLD:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_max_payload, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_IP_RT_ADD:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_ip_route_add, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;

				/* These error tag values should be interpreted as a utf-8 unterminated
				   strings. */
				case PPPOE_TAG_SVC_ERR:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_service_name_error, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_AC_ERR:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_ac_system_error, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;
				case PPPOE_TAG_GENERIC_ERR:
					proto_tree_add_item(pppoe_tree, hf_pppoed_tag_generic_error, tvb,
					                    tagstart+4, poe_tag_length, FALSE);
					break;

				/* Get out if see end-of-list tag */
				case PPPOE_TAG_EOL:
					return;

				default:
					if (poe_tag_length > 0 )
					{
						/* Presumably unknown tag;
						   show tag value if we didn't
						   do it above */
						if (!global_pppoe_show_tags_and_lengths)
						{
							proto_tree_add_item(pppoe_tree, hf_pppoed_tag, tvb, tagstart, 2, FALSE);
							proto_tree_add_item(pppoe_tree, hf_pppoed_tag_length, tvb, tagstart+2, 2, FALSE);
						}
						proto_tree_add_item(pppoe_tree, hf_pppoed_tag_unknown_data, tvb,
						                    tagstart+4, poe_tag_length, FALSE);
					}
			}

			tagstart += (4 + poe_tag_length);
		}
	}
}


/* Discovery protocol, i.e. PPP session not yet established */
static void dissect_pppoed(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint8  pppoe_code;
	guint16 reported_payload_length;

	proto_tree  *pppoe_tree = NULL;
	proto_item  *ti;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
	{
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "PPPoED");
	}
	if (check_col(pinfo->cinfo, COL_INFO))
	{
		col_clear(pinfo->cinfo, COL_INFO);
	}

	/* Start Decoding Here. */
	pppoe_code = tvb_get_guint8(tvb, 1);

	if (check_col(pinfo->cinfo, COL_INFO))
	{
		col_append_str(pinfo->cinfo, COL_INFO, val_to_str(pppoe_code, code_vals, "Unknown"));
	}

	/* Read length of payload */
	reported_payload_length = tvb_get_ntohs(tvb, 4);

	if (tree)
	{
		ti = proto_tree_add_item(tree, proto_pppoed, tvb, 0, reported_payload_length+6, FALSE);
		pppoe_tree = proto_item_add_subtree(ti, ett_pppoed);

		/* Dissect fixed fields */
		proto_tree_add_item(pppoe_tree, hf_pppoe_version, tvb, 0, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_type, tvb, 0, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_code, tvb, 1, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_session_id, tvb, 2, 2, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_payload_length, tvb, 4, 2, FALSE);
	}

	/* Now dissect any tags */
	if (reported_payload_length > 0)
	{
		dissect_pppoe_tags(tvb, pinfo, 6, pppoe_tree, 6+reported_payload_length);
	}

}

void proto_register_pppoed(void)
{
	static hf_register_info hf[] =
	{
		/* Discovery tag fields */
		{ &hf_pppoed_tags,
			{ "PPPoE Tags", "pppoed.tags", FT_NONE, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag,
			{ "Tag", "pppoed.tag", FT_UINT16, BASE_HEX,
				 VALS(tag_vals), 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_length,
			{ "Tag Length", "pppoed.tag_length", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_unknown_data,
			{ "Unknown Data", "pppoed.tag.unknown_data", FT_BYTES, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_service_name,
			{ "Service-Name", "pppoed.tags.service_name", FT_STRING, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_ac_name,
			{ "AC-Name", "pppoed.tags.ac_name", FT_STRING, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_host_uniq,
			{ "Host-Uniq", "pppoed.tags.host_uniq", FT_BYTES, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_ac_cookie,
			{ "AC-Cookie", "pppoed.tags.ac_cookie", FT_BYTES, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_vendor_id,
			{ "Vendor id", "pppoed.tags.vendor_id", FT_UINT32, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_vendor_unspecified,
			{ "Vendor unspecified", "pppoed.tags.vendor_unspecified", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_credits,
			{ "Credits", "pppoed.tags.credits", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_credits_fcn,
			{ "FCN", "pppoed.tags.credits.fcn", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_credits_bcn,
			{ "BCN", "pppoed.tags.credits.bcn", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics,
			{ "Metrics", "pppoed.tags.metrics", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_r,
			{ "Receive Only", "pppoed.tags.metrics.r", FT_BOOLEAN, 16,
				 NULL, PPPOE_RCV_ONLY_MASK, "", HFILL
			}
		},
		{ &hf_pppoed_tag_mdr_units,
			{ "MDR Units", "pppoed.tags.metrics.mdr_units", FT_UINT16, BASE_HEX,
				 VALS(datarate_scale_vals), PPPOE_MDR_MASK, "", HFILL
			}
		},
		{ &hf_pppoed_tag_cdr_units,
			{ "CDR Units", "pppoed.tags.metrics.cdr_units", FT_UINT16, BASE_HEX,
				 VALS(datarate_scale_vals), PPPOE_CDR_MASK, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_rlq,
			{ "Relative Link Quality", "pppoed.tags.metrics.rlq", FT_UINT8, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_resource,
			{ "Resource", "pppoed.tags.metrics.resource", FT_UINT8, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_latency,
			{ "Latency", "pppoed.tags.metrics.latency", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_curr_drate,
			{ "Curr. datarate", "pppoed.tags.metrics.curr_drate", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_metrics_max_drate,
			{ "Max. datarate", "pppoed.tags.metrics.max_drate", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_seq_num,
			{ "Sequence Number", "pppoed.tags.seq_num", FT_UINT16, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_cred_scale,
			{ "Credit Scale Factor", "pppoed.tags.credit_scale", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_relay_session_id,
			{ "Relay-Session-Id", "pppoed.tags.relay_session_id", FT_BYTES, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_hurl,
			{ "HURL", "pppoed.tags.hurl", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_motm,
			{ "MOTM", "pppoed.tags.motm", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_max_payload,
			{ "PPP Max Palyload", "pppoed.tags.max_payload", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_ip_route_add,
			{ "IP Route Add", "pppoed.tags.ip_route_add", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_service_name_error,
			{ "Service-Name-Error", "pppoed.tags.service_name_error", FT_STRING, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_ac_system_error,
			{ "AC-System-Error", "pppoed.tags.ac_system_error", FT_STRING, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoed_tag_generic_error,
			{ "Generic-Error", "pppoed.tags.generic_error", FT_STRING, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		}
	};

	static gint *ett[] = {
		&ett_pppoed,
		&ett_pppoed_tags
	};

	module_t *pppoed_module;

	/* Register protocol and fields */
	proto_pppoed = proto_register_protocol("PPP-over-Ethernet Discovery",
	                                       "PPPoED", "pppoed");
	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_pppoed, hf, array_length(hf));

	/* Preference setting */
	pppoed_module = prefs_register_protocol(proto_pppoed, NULL);
	prefs_register_bool_preference(pppoed_module, "show_tags_and_lengths",
	                               "Show tag values and lengths",
	                               "Show values of tags and lengths of data fields",
	                               &global_pppoe_show_tags_and_lengths);
}

void proto_reg_handoff_pppoed(void)
{
	dissector_handle_t pppoed_handle;

	pppoed_handle = create_dissector_handle(dissect_pppoed, proto_pppoed);
	dissector_add("ethertype", ETHERTYPE_PPPOED, pppoed_handle);
}


/* Session protocol, i.e. PPP session established */
static void dissect_pppoes(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint8  pppoe_code;
	guint16 pppoe_session_id;
	guint16 reported_payload_length;
	guint16 poe_tag;
	guint16 poe_tag_length;
	gint    actual_payload_length;
	gint    length, reported_length;
	gint    credit_offset = 0, tagstart = 0;

	proto_tree  *pppoe_tree;
	proto_item  *ti = NULL;
	tvbuff_t    *next_tvb;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
	{
		col_set_str(pinfo->cinfo,COL_PROTOCOL, "PPPoES");
	}
	if (check_col(pinfo->cinfo,COL_INFO))
	{
		col_clear(pinfo->cinfo,COL_INFO);
	}

	/* Start Decoding Here. */
	pppoe_code = tvb_get_guint8(tvb, 1);

	if (check_col(pinfo->cinfo,COL_INFO))
	{
		col_add_str(pinfo->cinfo, COL_INFO,
		             val_to_str(pppoe_code, code_vals, "Unknown"));
	}

	pppoe_session_id = tvb_get_ntohs(tvb, 2);
	reported_payload_length = tvb_get_ntohs(tvb, 4);
	actual_payload_length = tvb_reported_length_remaining(tvb, 6);

	if (tree)
	{
		ti = proto_tree_add_item(tree, proto_pppoes, tvb, 0, 6, FALSE);
		pppoe_tree = proto_item_add_subtree(ti, ett_pppoe);

		proto_tree_add_item(pppoe_tree, hf_pppoe_version, tvb, 0, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_type, tvb, 0, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_code, tvb, 1, 1, FALSE);
		proto_tree_add_item(pppoe_tree, hf_pppoe_session_id, tvb, 2, 2, FALSE);
		ti = proto_tree_add_item(pppoe_tree, hf_pppoe_payload_length, tvb, 4, 2, FALSE);


		if (PPPOE_TAG_CREDITS == tvb_get_ntohs(tvb, 6))
		{
			tagstart = 6;
			poe_tag = tvb_get_ntohs(tvb, tagstart);
			poe_tag_length = tvb_get_ntohs(tvb, tagstart + 2);

			/* Create tags subtree */
			ti = proto_tree_add_item(pppoe_tree, hf_pppoes_tags, tvb, tagstart, 8, FALSE);
			pppoe_tree = proto_item_add_subtree(ti, ett_pppoes_tags);

			/* Show tag data */
			if (poe_tag_length == 4)
			{
				proto_tree_add_item(pppoe_tree, hf_pppoes_tag_credits_fcn, tvb,
					tagstart+4, 2, FALSE);
				proto_tree_add_item(pppoe_tree, hf_pppoes_tag_credits_bcn, tvb,
					tagstart+6, 2, FALSE);
			} else {
				proto_tree_add_item(pppoe_tree, hf_pppoed_tag_credits, tvb,
					tagstart+4, poe_tag_length, FALSE);
			}

			credit_offset = 8;
		}
	}

	/*
	 * The only reason why the payload length from the header
	 * should differ from the remaining data in the packet
	 * would be if the total packet length, including Ethernet
	 * CRC, were < 64 bytes, so that padding was required.
	 *
	 * That means that you have 14 bytes of Ethernet header,
	 * 4 bytes of FCS, and fewer than 46 bytes of PPPoE packet.
	 *
	 * If that's not the case, we report a difference between
	 * the payload length in the packet, and the amount of
	 * data following the PPPoE header, as an error.
	 */
	if (tvb_reported_length(tvb) > 46) {
		/*
		 * Be forgiving about a possible trailing FCS.
		 *
		 * XXX - this dissector currently doesn't know
		 * whether any extra data past the end of the PPP
		 * payload is an FCS or not.
		 *
		 * If we know that we have an FCS, or that we don't
		 * have an FCS, we should have been handed a tvbuff
		 * without the FCS, and we should just do the strict
		 * length check.
		 *
		 * If we don't know whether we have an FCS, then:
		 *
		 *   if this isn't over Ethernet - the "E" in "PPPoE"
		 *   nonwithstanding, it can also run on top of 802.11,
		 *   for example - there's no trailer, so any data
		 *   past the payload length is either an FCS or
		 *   bogus;
		 *
		 *   if this is over Ethernet, there shouldn't be
		 *   a trailer, as the packet is long enough not to
		 *   require a trailer, as per the above;
		 *
		 * so perhaps we should assume that if we have exactly
		 * 4 bytes of extra information, it's an FCS, otherwise
		 * it's not.
		 *
		 * Perhaps we need to have a routine to call to
		 * do all the length checking, etc., and call it
		 * from here and from other dissectors where the
		 * protocol has a length field, or have a way to
		 * tell the dissector that called us which field
		 * has the length field and have *that* dissector
		 * do the length checking and add the expert info
		 * to the length field, *after* it does all the
		 * FCS heuristics.
		 */
	        if ((reported_payload_length != actual_payload_length) &&
                ((reported_payload_length + 4) != actual_payload_length)) {
                    proto_item_append_text(ti, " [incorrect, should be %u]",
                        actual_payload_length);
                    expert_add_info_format(pinfo, ti, PI_MALFORMED,
                        PI_WARN, "Possible bad payload length %u != %u",
                        reported_payload_length, actual_payload_length);
		}
	}

	/*
	 * Construct a tvbuff containing the PPP packet.
	 */
	length = tvb_length_remaining(tvb, 6);
	reported_length = tvb_reported_length_remaining(tvb, 6);
	DISSECTOR_ASSERT(length >= 0);
	DISSECTOR_ASSERT(reported_length >= 0);
	if (length > reported_length)
		length = reported_length;
	if ((guint)length > reported_payload_length)
		length = reported_payload_length;
	if ((guint)reported_length > reported_payload_length)
		reported_length = reported_payload_length;
	next_tvb = tvb_new_subset(tvb,(6 + credit_offset),
				(length - credit_offset),
				(reported_length - credit_offset));
	call_dissector(ppp_handle,next_tvb,pinfo,tree);
}

void proto_register_pppoes(void)
{

	static hf_register_info hf[] =
	{
		{ &hf_pppoes_tags,
			{ "PPPoE Tags", "pppoes.tags", FT_NONE, BASE_NONE,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoes_tag,
			{ "Tag", "pppoes.tag", FT_UINT16, BASE_HEX,
				 VALS(tag_vals), 0x0, "", HFILL
			}
		},
		{ &hf_pppoes_tag_credits,
			{ "Credits", "pppoes.tags.credits", FT_BYTES, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoes_tag_credits_fcn,
			{ "FCN", "pppoes.tags.credits.fcn", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoes_tag_credits_bcn,
			{ "BCN", "pppoes.tags.credits.bcn", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		}
	};

	static gint *ett[] = {
		&ett_pppoes,
		&ett_pppoes_tags
	};

	/* Register protocol */
	proto_pppoes = proto_register_protocol("PPP-over-Ethernet Session", "PPPoES", "pppoes");

	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_pppoes, hf, array_length(hf));
}

void proto_register_pppoe(void)
{
	static hf_register_info hf[] =
	{
		/* These fields common to discovery and session protocols */
		{ &hf_pppoe_version,
			{ "Version", "pppoe.version", FT_UINT8, BASE_DEC,
				 NULL, 0xf0, "", HFILL
			}
		},
		{ &hf_pppoe_type,
			{ "Type", "pppoe.type", FT_UINT8, BASE_DEC,
				 NULL, 0x0f, "", HFILL
			}
		},
		{ &hf_pppoe_code,
			{ "Code", "pppoe.code", FT_UINT8, BASE_HEX,
				 VALS(code_vals), 0x0, "", HFILL
			}
		},
		{ &hf_pppoe_session_id,
			{ "Session ID", "pppoe.session_id", FT_UINT16, BASE_HEX,
				 NULL, 0x0, "", HFILL
			}
		},
		{ &hf_pppoe_payload_length,
			{ "Payload Length", "pppoe.payload_length", FT_UINT16, BASE_DEC,
				 NULL, 0x0, "", HFILL
			}
		}
	};

	static gint *ett[] = {
		&ett_pppoe
	};

	/* Register protocol */
	proto_pppoe = proto_register_protocol("PPP-over-Ethernet", "PPPoE", "pppoe");

	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_pppoe, hf, array_length(hf));

}

void proto_reg_handoff_pppoes(void)
{
	dissector_handle_t pppoes_handle  =
	    create_dissector_handle(dissect_pppoes, proto_pppoes);
	dissector_add("ethertype", ETHERTYPE_PPPOES, pppoes_handle);

	/* Get a handle for the PPP dissector */
	ppp_handle = find_dissector("ppp");
}
