#!/bin/sh
#
# fill-nmdir.sh by Davide Libenzi (`nm` symbol files generator for iPhone)
# Copyright (C) 2008  Davide Libenzi
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Davide Libenzi <davidel@xmailserver.org>
#

if [ $# -lt 1 ]; then
    echo "Use: $0 HEAVENLY"
    exit 1
fi
if [ ! -d "$1" ]; then
    echo "Not a directory: $1"
    exit 1
fi
if [ ! -d "$1/System/Library/Frameworks" ]; then
    echo "Invalid heavenly directory (missing frameworks dir): $1"
    exit 1
fi
if [ ! -d "$1/usr/lib" ]; then
    echo "Invalid heavenly directory (missing /usr/lib): $1"
    exit 1
fi

EXBN=`dirname $0`

echo ">>> Exporting frameworks ..."
$EXBN/frmk-nmexport.sh "$1/System/Library/Frameworks"

for f in $1/usr/lib/*.dylib; do
    BN=`basename $f`
    echo "Exporting $f -> $BN ..."
    arm-apple-darwin9-nm -n $f > $BN.nmdump
done

