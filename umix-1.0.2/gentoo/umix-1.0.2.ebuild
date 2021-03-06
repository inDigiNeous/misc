# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/umix//umix/gentoo/umix-1.0.2.ebuild,v 1.1 2003/08/30 15:31:14 sakari Exp $

S=${WORKDIR}/${P}
DESCRIPTION="Program for adjusting soundcard volumes"
SRC_URI="mirror://sourceforge/${PN}/${P}.tar.gz"
HOMEPAGE="http://umix.sf.net"

SLOT="0"
KEYWORDS="x86"
LICENSE="GPL-2"
IUSE="ncurses oss"

DEPEND="ncurses? ( >=sys-libs/ncurses-5.2 )"

src_compile() {
	local myconf
	use ncurses || myconf="--disable-ncurses"
	use oss || myconf="${myconf} --disable-oss"
	econf ${myconf} || die
	emake || die
}

src_install() {
	make DESTDIR=${D} install
	dodoc AUTHORS ChangeLog NEWS README TODO
}
