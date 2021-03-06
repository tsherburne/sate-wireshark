/* capture_prefs.c
 * Dialog box for capture preferences
 *
 * $Id: prefs_capture.c 28741 2009-06-15 16:45:57Z gerald $
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
#include "config.h"
#endif

#ifdef HAVE_LIBPCAP

#include <string.h>
#include <gtk/gtk.h>

#include <epan/prefs.h>

#include "../globals.h"
#include "../simple_dialog.h"
#include "../capture-pcap-util.h"
#include "../capture_ui_utils.h"
#include "../capture.h"

#include "gtk/prefs_capture.h"
#include "gtk/prefs_dlg.h"
#include "gtk/gui_utils.h"
#include "gtk/dlg_utils.h"
#include "gtk/main.h"
#include "gtk/main_welcome.h"
#include "gtk/help_dlg.h"
#include "gtk/stock_icons.h"
#include <epan/strutil.h>


#define DEVICE_KEY		"device"
#define PROM_MODE_KEY		"prom_mode"
#define PCAP_NG_KEY		"pcap_ng"
#define CAPTURE_REAL_TIME_KEY	"capture_real_time"
#define AUTO_SCROLL_KEY		"auto_scroll"
#define SHOW_INFO_KEY           "show_info"

#define CAPTURE_TABLE_ROWS 6

#define IFOPTS_CALLER_PTR_KEY	"ifopts_caller_ptr"
#define IFOPTS_DIALOG_PTR_KEY	"ifopts_dialog_ptr"
#define IFOPTS_TABLE_ROWS 2
#define IFOPTS_LIST_COLS  5
#define IFOPTS_MAX_DESCR_LEN 128
#define IFOPTS_IF_NOSEL -1

/* interface options dialog */
static GtkWidget *cur_list, *if_dev_lb, *if_name_lb, *if_linktype_cb, *if_descr_te, *if_hide_cb;
static GtkTreeSelection *if_selection;	/* current interface row selected */
static int num_linktypes;
static gboolean interfaces_info_nochange;  /* TRUE to ignore Interface Options Properties */
                                           /*  widgets "changed" callbacks.               */

static void ifopts_edit_cb(GtkWidget *w, gpointer data);
static void ifopts_edit_ok_cb(GtkWidget *w, gpointer parent_w);
static void ifopts_edit_destroy_cb(GtkWidget *win, gpointer data);
static void ifopts_edit_ifsel_cb(GtkTreeSelection *selection, gpointer data);
static void ifopts_edit_linktype_changed_cb(GtkComboBox *ed, gpointer udata);
static void ifopts_edit_descr_changed_cb(GtkEditable *ed, gpointer udata);
static void ifopts_edit_hide_changed_cb(GtkToggleButton *tbt, gpointer udata);
static void ifopts_options_add(GtkListStore *list_store, if_info_t *if_info);
static void ifopts_options_free(gchar *text[]);
static void ifopts_if_liststore_add(void);
static void ifopts_write_new_linklayer(void);
static void ifopts_write_new_descr(void);
static void ifopts_write_new_hide(void);

GtkWidget*
capture_prefs_show(void)
{
	GtkWidget	*main_tb, *main_vb;
	GtkWidget	*if_cb, *if_lb, *promisc_cb, *pcap_ng_cb, *sync_cb, *auto_scroll_cb, *show_info_cb;
	GtkWidget	*ifopts_lb, *ifopts_bt;
	GList		*if_list, *combo_list;
	int		err;
	int		row = 0;
	GtkTooltips	*tooltips = gtk_tooltips_new();

	/* Main vertical box */
	main_vb = gtk_vbox_new(FALSE, 7);
	gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);

	/* Main table */
	main_tb = gtk_table_new(CAPTURE_TABLE_ROWS, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(main_vb), main_tb, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(main_tb), 10);
	gtk_table_set_col_spacings(GTK_TABLE(main_tb), 15);
	gtk_widget_show(main_tb);

	/* Default device */
	if_lb = gtk_label_new("Default interface:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_lb), 1.0f, 0.5f);
	gtk_widget_show(if_lb);

	if_cb = gtk_combo_new();
	/*
	 * XXX - what if we can't get the list?
	 */
	if_list = capture_interface_list(&err, NULL);
	combo_list = build_capture_combo_list(if_list, FALSE);
	free_interface_list(if_list);
	if (combo_list != NULL) {
		gtk_combo_set_popdown_strings(GTK_COMBO(if_cb), combo_list);
		free_capture_combo_list(combo_list);
	}
	if (prefs.capture_device)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(if_cb)->entry),
		    prefs.capture_device);
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_cb, 1, 2, row, row+1);
	gtk_tooltips_set_tip(tooltips, GTK_COMBO(if_cb)->entry,
	    "The default interface to be captured from.", NULL);
	gtk_widget_show(if_cb);
	g_object_set_data(G_OBJECT(main_vb), DEVICE_KEY, if_cb);
	row++;

	/* Interface properties */
	ifopts_lb = gtk_label_new("Interfaces:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), ifopts_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(ifopts_lb), 1.0f, 0.5f);
	gtk_widget_show(ifopts_lb);

	ifopts_bt = gtk_button_new_from_stock(WIRESHARK_STOCK_EDIT);
	gtk_tooltips_set_tip(tooltips, ifopts_bt,
	    "Open a dialog box to set various interface options.", NULL);
	g_signal_connect(ifopts_bt, "clicked", G_CALLBACK(ifopts_edit_cb), NULL);
	gtk_table_attach_defaults(GTK_TABLE(main_tb), ifopts_bt, 1, 2, row, row+1);
	row++;

	/* Promiscuous mode */
	promisc_cb = create_preference_check_button(main_tb, row++,
	    "Capture packets in promiscuous mode:", NULL,
	    prefs.capture_prom_mode);
	gtk_tooltips_set_tip(tooltips, promisc_cb,
	    "Usually a network card will only capture the traffic sent to its own network address. "
	    "If you want to capture all traffic that the network card can \"see\", mark this option. "
	    "See the FAQ for some more details of capturing packets from a switched network.", NULL);
	g_object_set_data(G_OBJECT(main_vb), PROM_MODE_KEY, promisc_cb);

	/* Pcap-NG format */
	pcap_ng_cb = create_preference_check_button(main_tb, row++,
	    "Capture packets in pcap-ng format:", NULL,
	    prefs.capture_pcap_ng);
	gtk_tooltips_set_tip(tooltips, pcap_ng_cb,
	     "Capture packets in the next-generation capture file format. "
	     "This is still experimental.", NULL);
	g_object_set_data(G_OBJECT(main_vb), PCAP_NG_KEY, pcap_ng_cb);

	/* Real-time capture */
	sync_cb = create_preference_check_button(main_tb, row++,
	    "Update list of packets in real time:", NULL,
	    prefs.capture_real_time);
	gtk_tooltips_set_tip(tooltips, sync_cb,
        "Update the list of packets while capture is in progress. "
        "Don't use this option if you notice packet drops.", NULL);
	g_object_set_data(G_OBJECT(main_vb), CAPTURE_REAL_TIME_KEY, sync_cb);

	/* Auto-scroll real-time capture */
	auto_scroll_cb = create_preference_check_button(main_tb, row++,
	    "Automatic scrolling in live capture:", NULL,
	    prefs.capture_auto_scroll);
	gtk_tooltips_set_tip(tooltips, auto_scroll_cb,
        "Automatic scrolling of the packet list while live capture is in progress. ", NULL);
	g_object_set_data(G_OBJECT(main_vb), AUTO_SCROLL_KEY, auto_scroll_cb);

	/* Show capture info dialog */
	show_info_cb = create_preference_check_button(main_tb, row++,
	    "Hide capture info dialog:", NULL,
	    !prefs.capture_show_info);
	gtk_tooltips_set_tip(tooltips, show_info_cb,
	    "Hide the capture info dialog while capturing. ", NULL);
	g_object_set_data(G_OBJECT(main_vb), SHOW_INFO_KEY, show_info_cb);

	/* Show 'em what we got */
	gtk_widget_show_all(main_vb);

	return(main_vb);
}

void
capture_prefs_fetch(GtkWidget *w)
{
	GtkWidget *if_cb, *promisc_cb, *pcap_ng_cb, *sync_cb, *auto_scroll_cb, *show_info_cb;
	gchar	*if_text;

	if_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), DEVICE_KEY);
	promisc_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), PROM_MODE_KEY);
	pcap_ng_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), PCAP_NG_KEY);
	sync_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), CAPTURE_REAL_TIME_KEY);
	auto_scroll_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), AUTO_SCROLL_KEY);
        show_info_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), SHOW_INFO_KEY);

	if (prefs.capture_device != NULL) {
		g_free(prefs.capture_device);
		prefs.capture_device = NULL;
	}
	if_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(if_cb)->entry)));
	/* Strip out white space */
	g_strstrip(if_text);
	/* If there was nothing but white space, treat that as an
	   indication that the user doesn't want to wire in a default
	   device, and just wants the first device in the list chosen. */
	if (*if_text == '\0') {
		g_free(if_text);
		if_text = NULL;
	}
	prefs.capture_device = if_text;

	prefs.capture_prom_mode = GTK_TOGGLE_BUTTON (promisc_cb)->active;

	prefs.capture_pcap_ng = GTK_TOGGLE_BUTTON (pcap_ng_cb)->active;

	prefs.capture_real_time = GTK_TOGGLE_BUTTON (sync_cb)->active;

	prefs.capture_auto_scroll = GTK_TOGGLE_BUTTON (auto_scroll_cb)->active;

	prefs.capture_show_info = !(GTK_TOGGLE_BUTTON (show_info_cb)->active);
}

void
capture_prefs_apply(GtkWidget *w _U_)
{
}

void
capture_prefs_destroy(GtkWidget *w)
{
	GtkWidget *caller = gtk_widget_get_toplevel(w);
	GtkWidget *dlg;

	/* Is there an interface descriptions dialog associated with this
	   Preferences dialog? */
	dlg = g_object_get_data(G_OBJECT(caller), IFOPTS_DIALOG_PTR_KEY);

	if (dlg != NULL) {
		/* Yes.  Destroy it. */
		window_destroy(dlg);
	}
}

/*
 * Create an edit interface options dialog.
 */
enum
{
	DEVICE_COLUMN,
	DESC_COLUMN,
	DEF_LINK_LAYER_COLUMN,
	COMMENT_COLUMN,
	HIDE_COLUMN,
	DLT_COLUMN,
	N_COLUMN /* The number of columns */
};

static void
ifopts_edit_cb(GtkWidget *w, gpointer data _U_)
{
	GtkWidget	  *ifopts_edit_dlg, *cur_scr_win, *main_hb, *main_tb,
			  *cur_opts_fr, *ed_opts_fr, *main_vb,
			  *if_linktype_lb, *if_descr_lb, *if_hide_lb,
			  *bbox, *ok_bt, *cancel_bt, *help_bt;

	GtkListStore	  *list_store;
	GtkWidget	  *list;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
 	GtkTreeView       *list_view;
	GtkTreeSelection  *selection;

	int row = 0;

	GtkWidget   *caller   = gtk_widget_get_toplevel(w);
	GtkTooltips *tooltips = gtk_tooltips_new();

	/* Has an edit dialog box already been opened for that top-level
	   widget? */
	ifopts_edit_dlg = g_object_get_data(G_OBJECT(caller), IFOPTS_DIALOG_PTR_KEY);
	if (ifopts_edit_dlg != NULL) {
		/* Yes.  Just re-activate that dialog box. */
		reactivate_window(ifopts_edit_dlg);
		return;
	}

	/* create a new dialog */
	ifopts_edit_dlg = dlg_conf_window_new("Wireshark: Preferences: Interface Options");
	gtk_window_set_default_size(GTK_WINDOW(ifopts_edit_dlg), DEF_WIDTH, 340);

	main_vb = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
	gtk_container_add(GTK_CONTAINER(ifopts_edit_dlg), main_vb);
	gtk_widget_show(main_vb);

	/* create current options frame */
	cur_opts_fr = gtk_frame_new("Interfaces");
	gtk_container_add(GTK_CONTAINER(main_vb), cur_opts_fr);
	gtk_widget_show(cur_opts_fr);

	/* create a scrolled window to pack the current options TreeView widget into */
	cur_scr_win = scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(cur_scr_win), 3);
	gtk_container_add(GTK_CONTAINER(cur_opts_fr), cur_scr_win);
	gtk_widget_show(cur_scr_win);

	/*
	 * Create current options TreeView.
	 */
	list_store = gtk_list_store_new(N_COLUMN,	/* Total number of columns XXX	*/
					G_TYPE_STRING,	/* Device			*/
					G_TYPE_STRING,	/* Description			*/
					G_TYPE_STRING,	/* Default link-layer		*/
					G_TYPE_STRING,	/* Comment			*/
					G_TYPE_STRING,	/* Hide?			*/
					G_TYPE_INT);	/* Dlt 				*/

	list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

	list_view = GTK_TREE_VIEW(list);

	/* The view now holds a reference.  We can get rid of our own reference */
	g_object_unref (G_OBJECT (list_store));

	/* 
	 * Create the first column packet, associating the "text" attribute of the
	 * cell_renderer to the first column of the model 
	 */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Device", renderer, 
							   "text", DEVICE_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 230);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

	column = gtk_tree_view_column_new_with_attributes ("Description", renderer, 
							   "text", DESC_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 260);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

	column = gtk_tree_view_column_new_with_attributes ("Default link-layer", renderer, 
							   "text", DEF_LINK_LAYER_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 260);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

	column = gtk_tree_view_column_new_with_attributes ("Comment", renderer, 
							   "text", COMMENT_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 100);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

	column = gtk_tree_view_column_new_with_attributes ("Hide?", renderer, 
							   "text", HIDE_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 40);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

#if 0
	/* Don't show the DLT column */
	column = gtk_tree_view_column_new_with_attributes ("DLT", renderer, 
							   "text", DLT_COLUMN, 
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 40);
	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);
#endif
	/* Setup the selection handler */
	selection = gtk_tree_view_get_selection(list_view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	cur_list = list;
	gtk_container_add(GTK_CONTAINER(cur_scr_win), cur_list);

	if_selection = selection;

	g_signal_connect (G_OBJECT (selection), "changed", /* select_row */
			  G_CALLBACK (ifopts_edit_ifsel_cb),
			  NULL);

	gtk_widget_show(cur_list);

	/* add interface names to cell */
	ifopts_if_liststore_add();

	/* create edit options frame */
	ed_opts_fr = gtk_frame_new("Properties");
	gtk_box_pack_start(GTK_BOX(main_vb), ed_opts_fr, FALSE, FALSE, 0);
	gtk_widget_show(ed_opts_fr);

	main_hb = gtk_hbox_new(TRUE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(main_hb), 3);
	gtk_container_add(GTK_CONTAINER(ed_opts_fr), main_hb);
	gtk_widget_show(main_hb);

	/* table to hold description text entry and hide button */
	main_tb = gtk_table_new(IFOPTS_TABLE_ROWS, 4, FALSE);
	gtk_box_pack_start(GTK_BOX(main_hb), main_tb, TRUE, FALSE, 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_tb), 10);
	gtk_table_set_col_spacings(GTK_TABLE(main_tb), 10);
	gtk_widget_show(main_tb);

	if_dev_lb = gtk_label_new("Device:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_dev_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_dev_lb), 1.0f, 0.5f);
	gtk_widget_show(if_dev_lb);

	if_dev_lb = gtk_label_new("");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_dev_lb, 1, 2, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_dev_lb), 0.0f, 0.5f);
	gtk_widget_show(if_dev_lb);
	row++;

	if_name_lb = gtk_label_new("Description:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_name_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_name_lb), 1.0f, 0.5f);
	gtk_widget_show(if_name_lb);

	if_name_lb = gtk_label_new("");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_name_lb, 1, 2, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_name_lb), 0.0f, 0.5f);
	gtk_widget_show(if_name_lb);
	row++;

	if_linktype_lb = gtk_label_new("Default link-layer header type:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_linktype_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_linktype_lb), 1.0f, 0.5f);
	gtk_widget_show(if_linktype_lb);

	if_linktype_cb = gtk_combo_box_new_text();
	num_linktypes = 0;
	interfaces_info_nochange = FALSE;
	g_signal_connect(if_linktype_cb, "changed", G_CALLBACK(ifopts_edit_linktype_changed_cb),
			cur_list);
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_linktype_cb, 1, 2, row, row+1);
	gtk_widget_show(if_linktype_cb);
	row++;

	/* create interface description label and text entry */
	if_descr_lb = gtk_label_new("Comment:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_descr_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_descr_lb), 1.0f, 0.5f);
	gtk_widget_show(if_descr_lb);

	if_descr_te = gtk_entry_new();
	g_signal_connect(if_descr_te, "changed", G_CALLBACK(ifopts_edit_descr_changed_cb),
			cur_list);
	gtk_entry_set_max_length(GTK_ENTRY(if_descr_te), IFOPTS_MAX_DESCR_LEN);
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_descr_te, 1, 2, row, row+1);
	gtk_widget_show(if_descr_te);
	row++;

	/* create "hide interface" label and button */
	if_hide_lb = gtk_label_new("Hide interface?:");
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_hide_lb, 0, 1, row, row+1);
	gtk_misc_set_alignment(GTK_MISC(if_hide_lb), 1.0f, 0.5f);
	gtk_widget_show(if_hide_lb);

	if_hide_cb = gtk_check_button_new();
	g_signal_connect(if_hide_cb, "toggled", G_CALLBACK(ifopts_edit_hide_changed_cb),
			cur_list);
	gtk_table_attach_defaults(GTK_TABLE(main_tb), if_hide_cb, 1, 2, row, row+1);
	gtk_widget_show(if_hide_cb);
        row++;

	/* button row: OK and Cancel buttons */
	bbox = dlg_button_row_new(GTK_STOCK_OK, GTK_STOCK_CANCEL, GTK_STOCK_HELP, NULL);
	gtk_box_pack_start(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);
	gtk_widget_show(bbox);

	ok_bt = g_object_get_data(G_OBJECT(bbox), GTK_STOCK_OK);
	gtk_tooltips_set_tip(tooltips, ok_bt,
			     "Save changes and exit dialog", NULL);
	g_signal_connect(ok_bt, "clicked", G_CALLBACK(ifopts_edit_ok_cb), ifopts_edit_dlg);

	cancel_bt = g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CANCEL);
	gtk_tooltips_set_tip(tooltips, cancel_bt,
			     "Cancel and exit dialog", NULL);
        window_set_cancel_button(ifopts_edit_dlg, cancel_bt, window_cancel_button_cb);

	help_bt = g_object_get_data(G_OBJECT(bbox), GTK_STOCK_HELP);
	g_signal_connect(help_bt, "clicked", G_CALLBACK(topic_cb), 
			 (gpointer)HELP_CAPTURE_INTERFACE_OPTIONS_DIALOG);
	gtk_tooltips_set_tip (tooltips, help_bt, "Show topic specific help", NULL);

	gtk_widget_grab_default(ok_bt);

	g_signal_connect(ifopts_edit_dlg, "delete_event", G_CALLBACK(window_delete_event_cb),
                 NULL);
	/* Call a handler when we're destroyed, so we can inform
	   our caller, if any, that we've been destroyed. */
	g_signal_connect(ifopts_edit_dlg, "destroy", G_CALLBACK(ifopts_edit_destroy_cb), NULL);

	/* Set the key for the new dialog to point to our caller. */
	g_object_set_data(G_OBJECT(ifopts_edit_dlg), IFOPTS_CALLER_PTR_KEY, caller);
	/* Set the key for the caller to point to us */
	g_object_set_data(G_OBJECT(caller), IFOPTS_DIALOG_PTR_KEY, ifopts_edit_dlg);

	gtk_widget_show(ifopts_edit_dlg); /* triggers ifopts_edit_ifsel_cb() with the  */
                                          /*  "interfaces" TreeView first row selected */
	window_present(ifopts_edit_dlg);
}

/*
 * User selected "OK". Create/write preferences strings.
 */
static void
ifopts_edit_ok_cb(GtkWidget *w _U_, gpointer parent_w)
{
	if (if_selection){ /* XXX: Cannot be NULL ?? */
		/* create/write new interfaces link-layer string */
		ifopts_write_new_linklayer();

		/* create/write new interfaces description string */
		ifopts_write_new_descr();

		/* create/write new "hidden" interfaces string */
		ifopts_write_new_hide();
	}

	/* Update welcome page */
	welcome_if_panel_reload ();

	/* Now nuke this window. */
	gtk_grab_remove(GTK_WIDGET(parent_w));
	window_destroy(GTK_WIDGET(parent_w));
}

static void
ifopts_edit_destroy_cb(GtkWidget *win, gpointer data _U_)
{
	GtkWidget *caller;

	/* Get the widget that requested that we be popped up, if any.
	   (It should arrange to destroy us if it's destroyed, so
	   that we don't get a pointer to a non-existent window here.) */
	caller = g_object_get_data(G_OBJECT(win), IFOPTS_CALLER_PTR_KEY);

	if (caller != NULL) {
		/* Tell it we no longer exist. */
                g_object_set_data(G_OBJECT(caller), IFOPTS_DIALOG_PTR_KEY, NULL);
	}
}

static gint
ifopts_description_to_val (const char *if_name, const char *descr) 
{
	GList *lt_list;
	int dlt = -1;

	lt_list = capture_pcap_linktype_list(if_name, NULL);
	if (lt_list != NULL) {
		GList  *lt_entry;
		/* XXX: Code skips first entry because that's the default ??? */
		for (lt_entry = g_list_next(lt_list); lt_entry != NULL; lt_entry = g_list_next(lt_entry)) {
			data_link_info_t *dli_p = lt_entry->data;
			if (dli_p->description) {
				if (strcmp(dli_p->description, descr) == 0) {
					dlt = dli_p->dlt;
					break;
				}
			} else {
				if (strcmp(dli_p->name, descr) == 0) {
					dlt = dli_p->dlt;
					break;
				}
			}
		}
		free_pcap_linktype_list(lt_list);
	}
	return dlt;
}

/*
 * Interface selected callback; update displayed widgets.
 */
static void
ifopts_edit_ifsel_cb(GtkTreeSelection	*selection _U_,
		     gpointer		 data _U_)
{
	GtkTreeIter       iter;
	GtkTreeModel	 *model;
	gchar            *desc, *comment, *hide, *text;
	gchar            *if_name, *linktype;
	GList 		 *lt_list;
	gint              selected = 0;

	/* Get list_store data for currently selected interface */
	if (!gtk_tree_selection_get_selected (if_selection, &model, &iter)){
		return;
	}
	gtk_tree_model_get(model, &iter, 
			   DEVICE_COLUMN,  &if_name,
			   DESC_COLUMN,    &desc,
			   DEF_LINK_LAYER_COLUMN, &linktype,
			   COMMENT_COLUMN, &comment,
			   HIDE_COLUMN,    &hide,
			   -1);

	/* display  the interface device from current interfaces selection */
	gtk_label_set_text(GTK_LABEL(if_dev_lb), if_name);

	/* display the interface name from current interfaces selection */
	gtk_label_set_text(GTK_LABEL(if_name_lb), desc);

	/* Ignore "changed" callbacks while we update the Properties widgets */
	interfaces_info_nochange = TRUE;

	/* display the link-layer header type from current interfaces selection */
        /*  -- remove old linktype list (if any) from the ComboBox */
	while (num_linktypes > 0) {
		num_linktypes--;
		gtk_combo_box_remove_text (GTK_COMBO_BOX(if_linktype_cb), num_linktypes);
	}

        /*  -- build and add to the ComboBox a linktype list for the current interfaces selection */
	lt_list = capture_pcap_linktype_list(if_name, NULL);
	if (lt_list != NULL) {
		GList *lt_entry;
		for (lt_entry = lt_list; lt_entry != NULL; lt_entry = g_list_next(lt_entry)) {
			data_link_info_t *dli_p = lt_entry->data;
			text = (dli_p->description != NULL) ? dli_p->description : dli_p->name;
			if (strcmp(linktype, text) == 0) {
				selected = num_linktypes;
			}
			gtk_combo_box_append_text(GTK_COMBO_BOX(if_linktype_cb), text);
			num_linktypes++;
		}
		gtk_widget_set_sensitive(if_linktype_cb, num_linktypes >= 2);
		gtk_combo_box_set_active(GTK_COMBO_BOX(if_linktype_cb), selected);
		free_pcap_linktype_list(lt_list);
	}

	/* display the interface description from current interfaces selection */
	gtk_entry_set_text(GTK_ENTRY(if_descr_te), comment);

	/* display the "hide interface" button state from current interfaces selection */
	if (strcmp("Yes", hide) == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(if_hide_cb), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(if_hide_cb), FALSE);

	interfaces_info_nochange = FALSE;

	g_free(if_name);
	g_free(desc);
	g_free(linktype);
	g_free(comment);
	g_free(hide);
}

/*
 * Link-layer entry changed callback; update list_store for currently selected interface.
 */
static void
ifopts_edit_linktype_changed_cb(GtkComboBox *cb, gpointer udata)
{
	gchar        *ifnm, *text;
	gint          linktype;
	GtkTreeModel *list_model;
#if ! GTK_CHECK_VERSION(2,6,0)
	GtkTreeIter   iter;
	GtkTreeModel *model;
#endif
	GtkTreeIter   list_iter;
	GtkListStore *list_store;

	if (interfaces_info_nochange)
		return;

	if (if_selection == NULL)  /* XXX: Cannot be NULL ?? */
		return;

	if (!gtk_tree_selection_get_selected (if_selection, &list_model, &list_iter)){
		return;
	}
	
	gtk_tree_model_get(list_model, &list_iter, 
		DEVICE_COLUMN, &ifnm,
		-1);

	/* get current description text and set value in list_store for currently selected interface */
#if GTK_CHECK_VERSION(2,6,0)
	text = gtk_combo_box_get_active_text(cb);
	if (text) {
#else
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb), &iter)) {
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
		gtk_tree_model_get(model, &iter, 0, &text, -1);
#endif
		linktype = ifopts_description_to_val(ifnm, text);
		list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (udata))); /* Get store */
		gtk_list_store_set  (list_store, &list_iter,
				     DEF_LINK_LAYER_COLUMN, text,
				     DLT_COLUMN, linktype,
				     -1);
		g_free(text);
	}
}

/*
 * Comment text entry changed callback; update list_store for currently selected interface.
 */
static void
ifopts_edit_descr_changed_cb(GtkEditable *ed, gpointer udata)
{
	gchar        *text;
	GtkTreeModel *list_model;
	GtkTreeIter   list_iter;
	GtkListStore *list_store;

	if (interfaces_info_nochange)
		return;

	if (if_selection == NULL) /* XXX: Cannot be NULL ?? */
		return;

	if (!gtk_tree_selection_get_selected (if_selection, &list_model, &list_iter)){
		return;
	}

	/* get current description text and set value in list_store for currently selected interface */
	text = gtk_editable_get_chars(GTK_EDITABLE(ed), 0, -1);
	/* replace any reserved formatting characters "()," with spaces */
	g_strdelimit(text, "(),", ' ');

	list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (udata))); /* Get store */
	gtk_list_store_set  (list_store, &list_iter,
			     COMMENT_COLUMN, text,
			     -1);

	g_free(text);
}

/*
 * Hide toggle button changed callback; update list_store for currently selected interface .
 */
static void
ifopts_edit_hide_changed_cb(GtkToggleButton *tbt, gpointer udata)
{
	GtkTreeModel *list_model;
	GtkTreeIter   list_iter;
	GtkListStore *list_store;

	if (interfaces_info_nochange)
		return;

	if (if_selection == NULL) /* XXX: Cannot be NULL ?? */
		return;

	if (!gtk_tree_selection_get_selected (if_selection, &list_model, &list_iter)){
		return;
	}

	list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (udata))); /* Get store */
	/* get "hide" button state and set text in list_store for currently selected interface */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tbt)) == TRUE)
		gtk_list_store_set  (list_store, &list_iter,
				     HIDE_COLUMN, "Yes",
				     -1);
	else
		gtk_list_store_set  (list_store, &list_iter,
				     HIDE_COLUMN, "No",
				     -1);
}

/*
 * Add any saved interface options that apply to interfaces ListStore.
 *
 * NOTE:
 *		Interfaces that have been removed from the machine or disabled and
 *		no longer apply are ignored. Therefore, if the user subsequently
 *		selects "OK", the options for these interfaces are lost (they're
 *		lost permanently if "Save" is selected).
 */
static void
ifopts_options_add(GtkListStore *list_store, if_info_t *if_info)
{
	gchar	*p;
	gchar	*ifnm;
	gchar	*desc;
	gchar	*pr_descr;
	gchar	*text[] = { NULL, NULL, NULL, NULL, NULL };
	GList   *lt_list;
	gint     linktype;
	GtkTreeIter  iter;

	/* set device name text */
	text[0] = g_strdup(if_info->name);

	/* set OS description */
	if (if_info->description != NULL)
		text[1] = g_strdup(if_info->description);
	else
		text[1] = g_strdup("");

	/* set default link-layer header type */
	linktype = capture_dev_user_linktype_find(if_info->name);
	lt_list = capture_pcap_linktype_list(if_info->name, NULL);
	if (lt_list != NULL) {
		GList  *lt_entry;
		for (lt_entry = lt_list; lt_entry != NULL; lt_entry = g_list_next(lt_entry)) {
			data_link_info_t *dli_p = lt_entry->data;
			/* If we have no previous link-layer header type we use the first one */
			if (linktype == -1 || linktype == dli_p->dlt) {
				if (dli_p->description) {
					text[2] = g_strdup(dli_p->description);
				} else {
					text[2] = g_strdup(dli_p->name);
				}
				break;
			}
		}
		free_pcap_linktype_list(lt_list);
	}
	/* if we have no link-layer */
	if (text[2] == NULL)
		text[2] = g_strdup("");

	/* add interface descriptions */
	if (prefs.capture_devices_descr != NULL) {
		/* create working copy of device descriptions */
		pr_descr = g_strdup(prefs.capture_devices_descr);

		/* if we find a description for this interface */
		if ((ifnm = strstr(pr_descr, if_info->name)) != NULL) {
			p = ifnm;
			while (*p != '\0') {
				/* found left parenthesis, start of description */
				if (*p == '(') {
					p++;
					/* if syntax error */
					if ((*p == '\0') || (*p == ',') || (*p == '(') || (*p == ')'))
						break;

					/* save pointer to beginning of description */
					desc = p;
					p++;
					/* skip to end of description */
					while (*p != '\0') {
						/* if syntax error */
						if ((*p == ',') || (*p == '('))
							break;

						/* end of description */
						else if (*p == ')') {
							/* terminate and set description text */
							*p = '\0';
							text[3] = g_strdup(desc);
							break;
						}
						p++;
					}
					/* get out */
					break;
				} else
					p++;
			}
		}

		g_free(pr_descr);
	}

	/* if we have no description */
	if (text[3] == NULL)
		text[3] = g_strdup("");

	/* check if interface is "hidden" */
	if ((prefs.capture_devices_hide != NULL) &&
	    (strstr(prefs.capture_devices_hide, if_info->name) != NULL))
		text[4] = g_strdup("Yes");
	else
		text[4] = g_strdup("No");

	/* add row to ListStore */

#if GTK_CHECK_VERSION(2,6,0)
	gtk_list_store_insert_with_values( list_store , &iter, G_MAXINT,
#else
	gtk_list_store_append  (list_store, &iter);
	gtk_list_store_set  (list_store, &iter,
#endif
			     DEVICE_COLUMN,  text[0],
			     DESC_COLUMN,    text[1],
			     DEF_LINK_LAYER_COLUMN, text[2],
			     COMMENT_COLUMN, text[3],
			     HIDE_COLUMN,    text[4],
			     DLT_COLUMN,     linktype,
			     -1);

	ifopts_options_free(text);
}

static void
ifopts_options_free(gchar *text[])
{
	gint i;

	for (i=0; i < IFOPTS_LIST_COLS; i++) {
		if (text[i] != NULL) {
			g_free(text[i]);
			text[i] = NULL;
		}
	}
}

/*
 * Add all interfaces to interfaces ListStore.
 */
static void
ifopts_if_liststore_add(void)
{
	GList	*if_list, *ifl_p;
	int	 err;
	gchar	*err_str;

	if_list = capture_interface_list(&err, &err_str);  /* if_list = ptr to first element of list (or NULL) */
	if (if_list == NULL) {
		if (err != NO_INTERFACES_FOUND) {
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_str);
		}
		g_free(err_str);
		return;
	}

	/* We have an interface list.                            */
	/* add OS description + interface name text to ListStore */
	ifl_p = if_list;
	for (ifl_p = if_list; ifl_p != NULL; ifl_p = g_list_next(ifl_p)) {
		/* should never happen, but just in case */
		if ((ifl_p->data) == NULL)
			continue;
		/* fill current options ListStore with current preference values */
		ifopts_options_add(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (cur_list))),
				   (if_info_t *)ifl_p->data);
	}
	free_interface_list(if_list);
}

/*
 * Create/write new interfaces link-layer string based on current CList.
 * Put it into the preferences value.
 */
static void
ifopts_write_new_linklayer(void)
{
	GtkListStore	*store;
	GtkTreeIter	 iter;
	GtkTreeModel 	*model;

	gboolean	 more_items = TRUE, first_if = TRUE;  /* flag to check if first in list */
	gchar		*ifnm;
	gint		 linktype;
	gchar		*tmp_linklayer;
	gchar		*new_linklayer;

	/* new preferences interfaces link-layer string */
	new_linklayer = g_malloc0(MAX_VAL_LEN);

	/* get link-layer for each row (interface) */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cur_list));
	store = GTK_LIST_STORE(model);
	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter) ) {

		while (more_items) {
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
					   DEVICE_COLUMN, &ifnm,
					   DLT_COLUMN,    &linktype,
					   -1);

			if (linktype == -1){
				more_items = gtk_tree_model_iter_next (model,&iter);
				continue;
			}

			if (first_if != TRUE) {
				g_strlcat (new_linklayer, ",", MAX_VAL_LEN);
			}
			/*
			 * create/cat interface link-layer to new string
			 * (leave space for parens, comma and terminator)
			 */			
			tmp_linklayer = g_strdup_printf("%s(%d)", ifnm, linktype);
			g_strlcat(new_linklayer, tmp_linklayer, MAX_VAL_LEN);
			g_free(tmp_linklayer);
			g_free(ifnm);
			/* set first-in-list flag to false */
			first_if = FALSE;
			more_items = gtk_tree_model_iter_next (model,&iter);
		}

		/* write new link-layer string to preferences */
		if (strlen(new_linklayer) > 0) {
			g_free(prefs.capture_devices_linktypes);
			prefs.capture_devices_linktypes = new_linklayer;
		}
		/* no link-layers */
		else {
			g_free(prefs.capture_devices_linktypes);
			g_free(new_linklayer);
			prefs.capture_devices_linktypes = NULL;
		}
	}
}

/*
 * Create/write new interfaces description string based on current CList.
 * Put it into the preferences value.
 */
static void
ifopts_write_new_descr(void)
{
	GtkListStore	*store;
	GtkTreeIter	 iter;
	GtkTreeModel	*model;
	gboolean	 more_items = TRUE;
	gboolean	 first_if = TRUE;	/* flag to check if first in list */
	gchar		*ifnm;
	gchar		*desc;
	gchar		*tmp_descr;
	gchar		*new_descr;

	/* new preferences interfaces description string */
	new_descr = g_malloc0(MAX_VAL_LEN);

	/* get description for each row (interface) */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cur_list));
	store = GTK_LIST_STORE(model);
	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter) ) {
		while (more_items) {
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
				DEVICE_COLUMN, &ifnm,
				COMMENT_COLUMN, &desc,
				-1);

			/* if no description, skip this interface */
			if (strlen(desc) == 0){
				more_items = gtk_tree_model_iter_next (model,&iter);
				continue;
			}
			/*
			 * create/cat interface description to new string
			 * (leave space for parens, comma and terminator)
			 */
			if (first_if != TRUE) {
				g_strlcat (new_descr, ",", MAX_VAL_LEN);
			}

			tmp_descr = g_strdup_printf("%s(%s)", ifnm, desc);
			g_strlcat(new_descr, tmp_descr, MAX_VAL_LEN);
			g_free(tmp_descr);

			/* set first-in-list flag to false */
			first_if = FALSE;
			more_items = gtk_tree_model_iter_next (model,&iter);
		}

		/* write new description string to preferences */
		if (strlen(new_descr) > 0) {
			g_free(prefs.capture_devices_descr);
			prefs.capture_devices_descr = new_descr;
		}
		/* no descriptions */
		else {
			g_free(prefs.capture_devices_descr);
			g_free(new_descr);
			prefs.capture_devices_descr = NULL;
		}
	}
}

/*
 * Create/write new "hidden" interfaces string based on current CList.
 * Put it into the preferences value.
 */
static void
ifopts_write_new_hide(void)
{
	GtkListStore 	*store;
	GtkTreeIter 	 iter;
	GtkTreeModel 	*model;
	gboolean	 more_items = TRUE;
	gint		 first_if = TRUE;	/* flag to check if first in list */
	gchar		*ifnm;
	gchar		*hide;
	gchar		*new_hide;

	/* new preferences "hidden" interfaces string */
	new_hide = g_malloc0(MAX_VAL_LEN);

	/* get "hide" flag text for each row (interface) */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cur_list));
	store = GTK_LIST_STORE(model);
	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter) ) {
		while (more_items) {
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
					   DEVICE_COLUMN, &ifnm,
					   HIDE_COLUMN,   &hide,
					   -1);

			/* if flag text is "No", skip this interface */
			if (strcmp("No", hide) == 0){
				more_items = gtk_tree_model_iter_next (model,&iter);
				continue;
			}

			/*
			 * create/cat interface to new string
			 */
			if (first_if != TRUE)
				g_strlcat (new_hide, ",", MAX_VAL_LEN);
			g_strlcat (new_hide, ifnm, MAX_VAL_LEN);

			/* set first-in-list flag to false */
			first_if = FALSE;
			more_items = gtk_tree_model_iter_next (model,&iter);
		}

		/* write new "hidden" string to preferences */
		if (strlen(new_hide) > 0) {
			g_free(prefs.capture_devices_hide);
			prefs.capture_devices_hide = new_hide;
		}
		/* no "hidden" interfaces */
		else {
			g_free(prefs.capture_devices_hide);
			g_free(new_hide);
			prefs.capture_devices_hide = NULL;
		}
	}
}

#endif /* HAVE_LIBPCAP */
