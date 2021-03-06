#!/usr/bin/perl
# Simple script that converts Perl test harness output to 
# Subunit
# Copyright (C) 2008 Jelmer Vernooij <jelmer@samba.org>
# Published under the GNU GPL, v3 or later

my $firstline = 1;
my $error = 0;
while(<STDIN>) {
	if ($firstline) {
		$firstline = 0;
		next;
	}
	if (/^not ok (\d+) - (.*)$/) {
		print "test: $2\n";
		print "failure: $2\n";
		$error = 1;
	} elsif (/^ok (\d+) - (.*)$/) {
		print "test: $2\n";
		print "success: $2\n";
	} elsif (/^ok (\d+)$/) {
		print "test: $1\n";
		print "success: $1\n";
	} elsif (/^ok (\d+) # skip (.*)$/) {
		print "test: $1\n";
		print "skip: $1 [\n$2\n]\n";
	} elsif (/^not ok (\d+)$/) {
		print "test: $1\n";
		print "failure: $1\n";
		$error = 1;
	} else {
		print;
	}
}
exit $error;

