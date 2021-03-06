#!/usr/bin/perl -w
#
# unix2dos.pl - convert UNIX line endings (\n) in DOS line endings (\r\n)
# 
# $Id: unix2dos.pl 18197 2006-05-21 05:12:17Z sahlberg $
#
# Copyright (c) 2004, Olivier Biot
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

use strict;

while (<STDIN>) {
	if($_ !~ /\r\n/) {
		$_ =~ s/\n/\r\n/;
	}
	print $_;
}
1;
