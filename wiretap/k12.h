/* k12.c
*
* $Id: k12.h 21651 2007-05-02 20:09:42Z lego $
*
* Wiretap Library
* Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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

int k12_open(wtap *wth, int *err, gchar **err_info);
int k12_dump_can_write_encap(int encap);
gboolean k12_dump_open(wtap_dumper *wdh, gboolean cant_seek _U_, int *err);
int k12text_open(wtap *wth, int *err, gchar **err_info _U_);
int k12text_dump_can_write_encap(int encap);
gboolean k12text_dump_open(wtap_dumper *wdh, gboolean cant_seek _U_, int *err);
