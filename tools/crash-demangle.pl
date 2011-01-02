#!/usr/bin/perl -w
#
# crash-demangle.pl by Davide Libenzi (Demangle tool for iPhone crash dumps)
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

use strict;

my $i;
my $nmddir;
my %hsyms;

for ($i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq "-D") {
	$i++;
	if ($i <= $#ARGV) {
	    $nmddir = $ARGV[$i];
	}
    } elsif ($ARGV[$i] eq "-h") {

    }
}

if (!defined($nmddir)) {
    usage();
}
if (! -d $nmddir) {
    print STDERR "invalid nmdump directory: $nmddir\n";
    exit(1);
}

while (<STDIN>) {
    my $ln;
    my ($nmn, $nma, $hnm, $sym);

    $ln = $_;
    $ln =~ s/^([^\r\n]*)[\r]?\n$/$1/;
    print $ln;
    if ($ln =~ /^[0-9]+[ \t]+([a-z\._\-]+)[ \t]+(0x[0-9a-f]+)[ \t]+(0x[0-9a-f]+)[ \t]+\+[ \t]+(0x[0-9a-f]+)$/i) {
	$nmn = $1;
	$nma = $2;
	$hnm = $nmn . "@" . $nma;
	$sym = $hsyms{$hnm};
	if (!defined($sym)) {
	    $sym = getsym($nmn, $nma);
	    $hsyms{$hnm} = $sym;
	}
	print " ($nma=$sym)";
    }
    print "\n";
}

exit(0);


sub usage {
    print STDERR "use: ${0} -D NMDDIR [-h]\n";
    exit(1);
}

sub getsym {
    my ($nmn, $nma) = @_;
    my $nmf;
    my $sym;
    my @comp;

    $nmf = "$nmddir/$nmn.nmdump";
    if (! -f $nmf) {
	return "????";
    }
    $sym = qx(./tools/nma2n -f $nmf $nma);
    chomp($sym);
    @comp = split("\t", $sym);
    return $comp[1];
}

