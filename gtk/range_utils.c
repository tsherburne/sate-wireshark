/* range_utils.c
 * Packet range routines (save, print, ...) for GTK things
 *
 * $Id: range_utils.c 25634 2008-06-29 15:51:43Z wmeier $
 *
 * Ulf Lamping <ulf.lamping@web.de>
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

#include <gtk/gtk.h>

#include "../globals.h"
#include "../simple_dialog.h"
#include "../packet-range.h"

#include "gtk/gui_utils.h"
#include "gtk/dlg_utils.h"
#include "gtk/range_utils.h"


#define RANGE_VALUES_KEY                "range_values"
#define RANGE_CAPTURED_BT_KEY           "range_captured_button"
#define RANGE_DISPLAYED_BT_KEY          "range_displayed_button"

#define RANGE_SELECT_ALL_KEY            "range_select_all_rb"
#define RANGE_SELECT_ALL_C_KEY          "range_select_all_c_lb"
#define RANGE_SELECT_ALL_D_KEY          "range_select_all_d_lb"
#define RANGE_SELECT_CURR_KEY           "range_select_curr_rb"
#define RANGE_SELECT_CURR_C_KEY         "range_select_curr_c_lb"
#define RANGE_SELECT_CURR_D_KEY         "range_select_curr_d_lb"
#define RANGE_SELECT_MARKED_KEY         "range_select_marked_only_rb"
#define RANGE_SELECT_MARKED_C_KEY       "range_select_marked_only_c_lb"
#define RANGE_SELECT_MARKED_D_KEY       "range_select_marked_only_d_lb"
#define RANGE_SELECT_MARKED_RANGE_KEY   "range_select_marked_range_rb"
#define RANGE_SELECT_MARKED_RANGE_C_KEY "range_select_marked_range_c_lb"
#define RANGE_SELECT_MARKED_RANGE_D_KEY "range_select_marked_range_d_lb"
#define RANGE_SELECT_USER_KEY           "range_select_user_range_rb"
#define RANGE_SELECT_USER_C_KEY         "range_select_user_range_c_lb"
#define RANGE_SELECT_USER_D_KEY         "range_select_user_range_d_lb"
#define RANGE_SELECT_USER_ENTRY_KEY     "range_select_user_range_entry"

gboolean
range_check_validity(packet_range_t *range)
{
  switch (packet_range_check(range)) {

  case CVT_NO_ERROR:
    return TRUE;

  case CVT_SYNTAX_ERROR:
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
      "The specified range of packets isn't a valid range.");
    return FALSE;

  case CVT_NUMBER_TOO_BIG:
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
      "The specified range of packets has a packet number that's too large.");
    return FALSE;

  default:
    g_assert_not_reached();
    return FALSE;
  }
}

/* update all "dynamic" things */
void
range_update_dynamics(gpointer data)
{
  packet_range_t *range;
  GtkWidget     *range_displayed_bt;
  gboolean      filtered_active;
  gint          selected_num;
  gboolean      can_select;
  gchar         label_text[100];


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);

  
  range_displayed_bt = g_object_get_data(G_OBJECT(data), RANGE_DISPLAYED_BT_KEY);
  filtered_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(range_displayed_bt));

  /* Enable saving only the displayed packets only if there *are*
     displayed packets. */
  if (range->displayed_cnt != 0)
    gtk_widget_set_sensitive(range_displayed_bt, TRUE);
  else {
    /* If saving the displayed packets is selected, select saving the
       captured packets. */
    filtered_active = FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_KEY)), FALSE);
    gtk_widget_set_sensitive(range_displayed_bt, FALSE);
  }

  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_C_KEY), !filtered_active);
  g_snprintf(label_text, sizeof(label_text), "%u", cfile.count);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_C_KEY)), label_text);
  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_D_KEY), filtered_active);
  g_snprintf(label_text, sizeof(label_text), "%u", range->displayed_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_D_KEY)), label_text);

  /* Enable saving the currently-selected packet only if there *is* a
     currently-selected packet. */
  selected_num = (cfile.current_frame) ? cfile.current_frame->num : 0;
  can_select = (selected_num != 0);
  if (can_select) {
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_KEY), TRUE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_C_KEY), !filtered_active);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_D_KEY), filtered_active);
  } else {
    /* If "save selected packet" is selected, select "save all packets". */
    if (range->process == range_process_selected) {
      range->process = range_process_all;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_KEY)), TRUE);
    }
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_C_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_D_KEY), FALSE);
  }
  /* XXX: how to update the radio button label but keep the mnemonic? */
/*g_snprintf(label_text, sizeof(label_text), "_Selected packet #%u only", selected_num);
  gtk_label_set_text(GTK_LABEL(GTK_BIN(select_curr_rb)->child), label_text);*/
  g_snprintf(label_text, sizeof(label_text), "%u", selected_num ? 1 : 0);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_C_KEY)), label_text);
  g_snprintf(label_text, sizeof(label_text), "%u", selected_num ? 1 : 0);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_CURR_D_KEY)), label_text);

  /* Enable the buttons for saving marked packets only if there *are*
     marked packets. */
  if (filtered_active)
    can_select = (range->displayed_marked_cnt != 0);
  else
    can_select = (cfile.marked_count > 0);
  if (can_select) {
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_KEY), TRUE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_C_KEY), !filtered_active);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_D_KEY), filtered_active);
  }
  else {
    /* If "save marked packet" is selected, select "save all packets". */
    if (range->process == range_process_marked) {
      range->process = range_process_all;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_KEY)), TRUE);
    }
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_C_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_D_KEY), FALSE);
  }
  g_snprintf(label_text, sizeof(label_text), "%u", cfile.marked_count);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_C_KEY)), label_text);
  g_snprintf(label_text, sizeof(label_text), "%u", range->displayed_marked_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_D_KEY)), label_text);

  /* Enable the buttons for saving the range of marked packets only if
     there *is* a range of marked packets. */
  if (filtered_active)
    can_select = (range->displayed_mark_range_cnt != 0);
  else
    can_select = (range->mark_range_cnt != 0);
  if (can_select) {
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_KEY), TRUE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_C_KEY), !filtered_active);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_D_KEY), filtered_active);
  }
  else {
    /* If "save range between first and last marked packet" is selected,
       select "save all packets". */
    if (range->process == range_process_marked_range) {
      range->process = range_process_all;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(data), RANGE_SELECT_ALL_KEY)), TRUE);
    }
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_C_KEY), FALSE);
    gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_D_KEY), FALSE);
  }
  g_snprintf(label_text, sizeof(label_text), "%u", range->mark_range_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_C_KEY)), label_text);
  g_snprintf(label_text, sizeof(label_text), "%u", range->displayed_mark_range_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_MARKED_RANGE_D_KEY)), label_text);

  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_KEY), TRUE);
  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_C_KEY), !filtered_active);
  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_D_KEY), filtered_active);
  g_snprintf(label_text, sizeof(label_text), "%u", range->user_range_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_C_KEY)), label_text);
  g_snprintf(label_text, sizeof(label_text), "%u", range->displayed_user_range_cnt);
  gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_D_KEY)), label_text);
}


static void
toggle_captured_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *bt;
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);

  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    /* They changed the state of the "captured" button. */
    range->process_filtered = FALSE;

    bt = g_object_get_data(G_OBJECT(data), RANGE_CAPTURED_BT_KEY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bt), TRUE);
    bt = g_object_get_data(G_OBJECT(data), RANGE_DISPLAYED_BT_KEY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bt), FALSE);

    range_update_dynamics(data);
  }
}

static void
toggle_filtered_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *bt;
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process_filtered = TRUE;
    bt = g_object_get_data(G_OBJECT(data), RANGE_CAPTURED_BT_KEY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bt), FALSE);
    bt = g_object_get_data(G_OBJECT(data), RANGE_DISPLAYED_BT_KEY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bt), TRUE);
    
    range_update_dynamics(data);
  }
}

static void
toggle_select_all(GtkWidget *widget, gpointer data)
{
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process = range_process_all;
    range_update_dynamics(data);
  }
}

static void
toggle_select_selected(GtkWidget *widget, gpointer data)
{
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process = range_process_selected;
    range_update_dynamics(data);
  }
}

static void
toggle_select_marked_only(GtkWidget *widget, gpointer data)
{
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process = range_process_marked;
    range_update_dynamics(data);
  }
}

static void
toggle_select_marked_range(GtkWidget *widget, gpointer data)
{
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process = range_process_marked_range;
    range_update_dynamics(data);
  }
}

static void
toggle_select_user_range(GtkWidget *widget, gpointer data)
{
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);
  
  /* is the button now active? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
    range->process = range_process_user_range;
    range_update_dynamics(data);
  }
	
  /* Make the entry widget sensitive or insensitive */
  gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_ENTRY_KEY), range->process == range_process_user_range);

  /* When selecting user specified range, then focus on the entry */
  if (range->process == range_process_user_range)
    gtk_widget_grab_focus(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_ENTRY_KEY));

}


static void
range_entry(GtkWidget *widget _U_, gpointer data)
{
  const gchar   *entry_text;
  GtkWidget     *entry;
  packet_range_t *range;


  range = g_object_get_data(G_OBJECT(data), RANGE_VALUES_KEY);  
  entry = g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_ENTRY_KEY);

  gtk_toggle_button_set_active(g_object_get_data(G_OBJECT(data), RANGE_SELECT_USER_KEY), TRUE);
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  packet_range_convert_str(range, entry_text);
  range_update_dynamics(data);
}


static void
range_entry_in_event(GtkWidget *widget _U_, GdkEventFocus *event _U_, gpointer user_data _U_)
{
    /* This event is called, if the "enter" key is pressed while the key focus (right name?) */
    /* is in the range entry field. */

    /* Calling range_entry() isn't necessary as all changes are already done while the */
    /* entry was edited. Calling it here will cause a NULL pointer exception, */
    /* so don't do: <range_entry(widget, user_data); as we did before. */

    /* What we could do here is to cause the "hosting" dialog box do whatever it */
    /* needs to do when the default button was pressed. This is difficult as we currently */
    /* don't have a concept to call the hosting dialog this way. */

    /* XXX - As we might want to put the whole range thing in it's own dialog, this would be */
    /* a much easier task than it would be today as we could simply close our own dialog. */
}


/* create a new range "widget" */
GtkWidget *range_new(packet_range_t *range)
{
  GtkWidget     *range_tb;
  GtkWidget     *captured_bt;
  GtkWidget     *displayed_bt;

  GtkWidget     *select_all_rb;
  GtkWidget     *select_all_c_lb;
  GtkWidget     *select_all_d_lb;
  GtkWidget     *select_curr_rb;
  GtkWidget     *select_curr_c_lb;
  GtkWidget     *select_curr_d_lb;
  GtkWidget     *select_marked_only_rb;
  GtkWidget     *select_marked_only_c_lb;
  GtkWidget     *select_marked_only_d_lb;
  GtkWidget     *select_marked_range_rb;
  GtkWidget     *select_marked_range_c_lb;
  GtkWidget     *select_marked_range_d_lb;
  GtkWidget     *select_user_range_rb;
  GtkWidget     *select_user_range_c_lb;
  GtkWidget     *select_user_range_d_lb;
  GtkWidget     *select_user_range_entry;
 
  GtkTooltips   *tooltips;


  /* Enable tooltips */
  tooltips = gtk_tooltips_new();

  /* range table */
  range_tb = gtk_table_new(7, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(range_tb), 5);

  /* captured button */
  captured_bt = gtk_toggle_button_new_with_mnemonic("_Captured");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), captured_bt, 1, 2, 0, 1);
  g_signal_connect(captured_bt, "toggled", G_CALLBACK(toggle_captured_cb), range_tb);
  gtk_tooltips_set_tip (tooltips,captured_bt,("Process all the below chosen packets"), NULL);

  /* displayed button */
  displayed_bt = gtk_toggle_button_new_with_mnemonic("_Displayed");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), displayed_bt, 2, 3, 0, 1);
  g_signal_connect(displayed_bt, "toggled", G_CALLBACK(toggle_filtered_cb), range_tb);
  gtk_tooltips_set_tip (tooltips,displayed_bt,("Process only the below chosen packets, which also passes the current display filter"), NULL);


  /* Process all packets */
  select_all_rb = gtk_radio_button_new_with_mnemonic_from_widget(NULL, "_All packets");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_all_rb, 0, 1, 1, 2);
  gtk_tooltips_set_tip (tooltips, select_all_rb, 
      ("Process all packets"), NULL);
  g_signal_connect(select_all_rb, "toggled", G_CALLBACK(toggle_select_all), range_tb);

  select_all_c_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_all_c_lb, 1, 2, 1, 2);
  select_all_d_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_all_d_lb, 2, 3, 1, 2);


  /* Process currently selected */
  select_curr_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(select_all_rb), "_Selected packet only");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_curr_rb, 0, 1, 2, 3);
  gtk_tooltips_set_tip (tooltips, select_curr_rb, ("Process the currently selected packet only"), NULL);
  g_signal_connect(select_curr_rb, "toggled", G_CALLBACK(toggle_select_selected), range_tb);

  select_curr_c_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_curr_c_lb, 1, 2, 2, 3);
  select_curr_d_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_curr_d_lb, 2, 3, 2, 3);


  /* Process marked packets */
  select_marked_only_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(select_all_rb), "_Marked packets only");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_only_rb, 0, 1, 3, 4);
  gtk_tooltips_set_tip (tooltips, select_marked_only_rb, ("Process marked packets only"), NULL);
  g_signal_connect(select_marked_only_rb, "toggled", G_CALLBACK(toggle_select_marked_only), range_tb);

  select_marked_only_c_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_only_c_lb, 1, 2, 3, 4);
  select_marked_only_d_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_only_d_lb, 2, 3, 3, 4);


  /* Process packet range between first and last packet */
  select_marked_range_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(select_all_rb), "From first _to last marked packet");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_range_rb, 0, 1, 4, 5);
  gtk_tooltips_set_tip (tooltips,select_marked_range_rb,("Process all packets between the first and last marker"), NULL);
  g_signal_connect(select_marked_range_rb, "toggled", G_CALLBACK(toggle_select_marked_range), range_tb);

  select_marked_range_c_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_range_c_lb, 1, 2, 4, 5);
  select_marked_range_d_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_marked_range_d_lb, 2, 3, 4, 5);


  /* Process a user specified provided packet range : -10,30,40-70,80- */
  select_user_range_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(select_all_rb), "Specify a packet _range:");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_user_range_rb, 0, 1, 5, 6);
  gtk_tooltips_set_tip (tooltips,select_user_range_rb,("Process a specified packet range"), NULL);
  g_signal_connect(select_user_range_rb, "toggled", G_CALLBACK(toggle_select_user_range), range_tb);

  select_user_range_c_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_user_range_c_lb, 1, 2, 5, 6);
  select_user_range_d_lb = gtk_label_new("?");
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_user_range_d_lb, 2, 3, 5, 6);


  /* The entry part */
  select_user_range_entry = gtk_entry_new();
  gtk_entry_set_max_length (GTK_ENTRY (select_user_range_entry), 254);
  gtk_table_attach_defaults(GTK_TABLE(range_tb), select_user_range_entry, 0, 1, 6, 7);
  gtk_tooltips_set_tip (tooltips,select_user_range_entry, 
	("Specify a range of packet numbers :     \nExample :  1-10,18,25-100,332-"), NULL);
  g_signal_connect(select_user_range_entry,"changed", G_CALLBACK(range_entry), range_tb);
  g_signal_connect(select_user_range_entry,"activate", G_CALLBACK(range_entry_in_event), range_tb);


  gtk_widget_show_all(range_tb);


  g_object_set_data(G_OBJECT(range_tb), RANGE_VALUES_KEY,               range);
  g_object_set_data(G_OBJECT(range_tb), RANGE_CAPTURED_BT_KEY,          captured_bt);
  g_object_set_data(G_OBJECT(range_tb), RANGE_DISPLAYED_BT_KEY,         displayed_bt);

  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_ALL_KEY,           select_all_rb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_ALL_C_KEY,         select_all_c_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_ALL_D_KEY,         select_all_d_lb);

  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_CURR_KEY,          select_curr_rb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_CURR_C_KEY,        select_curr_c_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_CURR_D_KEY,        select_curr_d_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_CURR_D_KEY,        select_curr_d_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_KEY,        select_marked_only_rb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_C_KEY,      select_marked_only_c_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_D_KEY,      select_marked_only_d_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_RANGE_KEY,  select_marked_range_rb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_RANGE_C_KEY,select_marked_range_c_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_MARKED_RANGE_D_KEY,select_marked_range_d_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_USER_KEY,          select_user_range_rb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_USER_C_KEY,        select_user_range_c_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_USER_D_KEY,        select_user_range_d_lb);
  g_object_set_data(G_OBJECT(range_tb), RANGE_SELECT_USER_ENTRY_KEY,    select_user_range_entry);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(captured_bt), !range->process_filtered);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(displayed_bt), range->process_filtered);

  switch(range->process) {
  case(range_process_all):
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_all_rb),  TRUE);
    break;
  case(range_process_selected):
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_curr_rb),  TRUE);
    break;
  case(range_process_marked):
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_marked_only_rb),  TRUE);
    break;
  case(range_process_marked_range):
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_marked_range_rb),  TRUE);
    break;
  case(range_process_user_range):
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_user_range_rb),  TRUE);
    break;
  default:
    g_assert_not_reached();
  }

  return range_tb;
}
