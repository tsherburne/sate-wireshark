/* export_object_dicom.c
 * $Id: export_object_dicom.c 26413 2008-10-11 14:25:02Z etxrab $
 * Routines for tracking & saving objects found in DICOM streams
 * See also: export_object.c / export_object.h for common code
 * Copyright 2008, David Aggeler <david_aggeler@hispeed.ch>
 * 
 * $Id: export_object_dicom.c 26413 2008-10-11 14:25:02Z etxrab $
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <epan/dissectors/packet-dcm.h>

#include <epan/packet.h>
#include <epan/tap.h>

#include "gtk/export_object.h"

static int
eo_dicom_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt _U_,
	       const void *data)
{
	export_object_list_t *object_list = tapdata;
	const dicom_eo_t *eo_info = data;
	export_object_entry_t *entry;

	if (eo_info) { /* We have data waiting for us */
		/* These values will be freed when the Export Object window is closed.
		   Therefore, strings and buffers must be copied
		*/
		entry = g_malloc(sizeof(export_object_entry_t));

		entry->pkt_num = pinfo->fd->num;
		entry->hostname = g_strdup(eo_info->hostname);
		entry->content_type = g_strdup(eo_info->content_type);
		entry->filename = g_strdup(g_path_get_basename(eo_info->filename));
		entry->payload_len  = eo_info->payload_len;
		entry->payload_data = g_memdup(eo_info->payload_data, eo_info->payload_len);

		object_list->entries =
			g_slist_append(object_list->entries, entry);

		return 1; /* State changed - window should be redrawn */
	} else {
		return 0; /* State unchanged - no window updates needed */
	}
}

void
eo_dicom_cb(GtkWidget *widget _U_, gpointer data _U_)
{
	export_object_window("dicom_eo", "DICOM", eo_dicom_packet);
}
