#!/usr/bin/perl -w
#
# $Id:$

use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_s /;

getopts('f:s:');

my $file   = $opt_f || die "Specify a pids file.";
my $signal = $opt_s || 15;

#------------------------------------------------------------------------------

open(FH, "< $file") or die $!;
while (my $line = <FH>) {
    
    if ( $line =~ /pid="(\d+)"/ ) {
	my $pid = $1;
	if ( $pid > 0 ) {
	    print "kill $pid\n";
	    system( "kill -$signal $pid" );
	}
    }
}

__END__

