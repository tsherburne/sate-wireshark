/* main_filter_toolbar.h
 * Definitions for filter toolbar routines
 * Copyright 2003, Ulf Lamping <ulf.lamping@web.de>
 *
 * $Id: main_filter_toolbar.h 26899 2008-12-02 08:11:23Z jmayer $
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

#ifndef __MAIN_FILTER_TOOLBAR_H__
#define __MAIN_FILTER_TOOLBAR_H__

#define E_DFILTER_FL_KEY          "display_filter_list"
#define E_DFILTER_CM_KEY          "display_filter_combo"

extern GtkWidget *filter_toolbar_new(void);

#endif /* __MAIN_FILTER_TOOLBAR_H__ */
