# Makefile.am
# Automake file for Wireshark documentation
#
# $Id: Makefile.am 25660 2008-07-02 05:08:43Z guy $
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# We include dependencies on ../config.h in order to
# capture when $(VERSION) changes.

wireshark-tmp.pod: $(srcdir)/wireshark.pod $(top_builddir)/AUTHORS-SHORT-FORMAT
	cat $(srcdir)/wireshark.pod $(top_builddir)/AUTHORS-SHORT-FORMAT > wireshark-tmp.pod

../ws.css: ../docbook/ws.css
	cp ../docbook/ws.css ../

../wireshark.1: wireshark-tmp.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	wireshark-tmp.pod | sed 's/WIRESHARK-TMP/WIRESHARK/' > ../wireshark.1

../wireshark.html: wireshark-tmp.pod ../config.h ../ws.css
	$(POD2HTML)						\
	--title="The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css						\
	--noindex						\
	wireshark-tmp.pod > ../wireshark.html

../tshark.1: tshark.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/tshark.pod > ../tshark.1

../tshark.html: tshark.pod ../config.h ../ws.css
	$(POD2HTML)							\
	--title="tshark - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/tshark.pod > ../tshark.html

../wireshark-filter.4: wireshark-filter.pod ../config.h
	$(POD2MAN)					\
	--section=4					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	wireshark-filter.pod > ../wireshark-filter.4

../wireshark-filter.html: wireshark-filter.pod ../config.h ../ws.css
	$(POD2HTML)								\
	--title="wireshark-filter - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css								\
	--noindex								\
	wireshark-filter.pod > ../wireshark-filter.html

wireshark-filter.pod: wireshark-filter.pod.template  ../tshark
	WIRESHARK_RUN_FROM_BUILD_DIRECTORY=1 ../tshark -G fields | $(PERL) $(srcdir)/dfilter2pod.pl $(srcdir)/wireshark-filter.pod.template > wireshark-filter.pod

../capinfos.1: capinfos.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/capinfos.pod > ../capinfos.1

../capinfos.html: capinfos.pod ../config.h ../ws.css
	$(POD2HTML)							\
	--title="capinfos - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/capinfos.pod > ../capinfos.html

../editcap.1: editcap.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/editcap.pod > ../editcap.1

../editcap.html: editcap.pod ../config.h ../ws.css
	$(POD2HTML)							\
	--title="editcap - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/editcap.pod > ../editcap.html

../idl2wrs.1: idl2wrs.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/idl2wrs.pod > ../idl2wrs.1

../idl2wrs.html: idl2wrs.pod ../config.h ../ws.css
	$(POD2HTML)							\
	--title="idl2wrs - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/idl2wrs.pod > ../idl2wrs.html

../mergecap.1: mergecap.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/mergecap.pod > ../mergecap.1

../mergecap.html: mergecap.pod ../config.h ../ws.css
	$(POD2HTML)							\
	--title="mergecap - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/mergecap.pod > ../mergecap.html

../text2pcap.1: text2pcap.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/text2pcap.pod > ../text2pcap.1

../text2pcap.html: text2pcap.pod ../config.h ../ws.css
	$(POD2HTML)                     \
	--title="text2pcap - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/text2pcap.pod > ../text2pcap.html

../dumpcap.1: dumpcap.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/dumpcap.pod > ../dumpcap.1

../dumpcap.html: dumpcap.pod ../config.h ../ws.css
	$(POD2HTML)                     \
	--title="dumpcap - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/dumpcap.pod > ../dumpcap.html

../rawshark.1: rawshark.pod ../config.h
	$(POD2MAN)					\
	--center="The Wireshark Network Analyzer"	\
	--release=$(VERSION)				\
	$(srcdir)/rawshark.pod > ../rawshark.1

../rawshark.html: rawshark.pod ../config.h ../ws.css
	$(POD2HTML)                     \
	--title="rawshark - The Wireshark Network Analyzer $(VERSION)"	\
	--css=ws.css							\
	--noindex							\
	$(srcdir)/rawshark.pod > ../rawshark.html

CLEANFILES =		\
	wireshark-filter.pod \
	wireshark-tmp.pod \
	../wireshark-filter.4 \
	../wireshark-filter.html \
	../wireshark.1	\
	../wireshark.html	\
	../capinfos.1	\
	../capinfos.html	\
	../editcap.1	\
	../editcap.html	\
	../mergecap.1	\
	../mergecap.html	\
	../tshark.1	\
	../tshark.html	\
	../text2pcap.1	\
	../text2pcap.html	\
	../dumpcap.1	\
	../dumpcap.html	\
	../idl2wrs.1	\
	../idl2wrs.html	\
	../rawshark.1	\
	../rawshark.html

DISTCLEANFILES =	\
	pod2htmi.tmp	\
	pod2htmd.tmp	\
	*~

MAINTAINERCLEANFILES = \
	Makefile.in

EXTRA_DIST = \
	Makefile.nmake	\
	README.binarytrees	\
	README.capture	\
	README.design	\
	README.developer	\
	README.display_filter	\
	README.idl2wrs	\
	README.malloc	\
	README.packaging	\
	README.plugins	\
	README.regression   \
	README.request_response_tracking	\
	README.stats_tree	\
	README.tapping	\
	README.xml-output	\
	capinfos.pod		\
	dfilter2pod.pl	\
	dumpcap.pod		\
	editcap.pod		\
	eproto2sgml		\
	idl2wrs.pod		\
	mergecap.pod	\
	randpkt.txt		\
	rawshark.pod	\
	sgml.doc.template	\
	text2pcap.pod	\
	tshark.pod		\
	wireshark-filter.pod.template \
	wireshark.pod
