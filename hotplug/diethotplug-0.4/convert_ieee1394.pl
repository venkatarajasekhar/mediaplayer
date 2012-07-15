#!/usr/bin/perl
#
# Script to turn a modules.ieee1394map file into a c structure.

# Copyright (C) 2001 Greg Kroah-Hartman <greg@kroah.com>
#  
#	This program is free software; you can redistribute it and/or modify it
#	under the terms of the GNU General Public License as published by the
#	Free Software Foundation version 2 of the License.
# 
#	This program is distributed in the hope that it will be useful, but
#	WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#	General Public License for more details.
#
#	You should have received a copy of the GNU General Public License along
#	with this program; if not, write to the Free Software Foundation, Inc.,
#	675 Mass Ave, Cambridge, MA 02139, USA.
#
#
# This script is very dependent on the layout of the modules.ieee1394map file,
# so if it changes, this script needs to change.  This could probably be done
# dynamically, by looking at the first line of the file and determining how
# many variables there are.
#
# Or in other words, a real perl programmer could merge all of these scripts
# together and do it all in only 3 lines :)
# 

print "struct ieee1394_module_map {\n";
print "\tconst char * module_name;\n";
print "\tunsigned int match_flags;\n";
print "\tunsigned int vendor_id;\n";
print "\tunsigned int model_id;\n";
print "\tunsigned int specifier_id;\n";
print "\tunsigned int version;\n";
print "} __attribute__ ((packed));\n";
print "\n";
print "struct ieee1394_module_map ieee1394_module_map[] = {\n";

while (<>) {
	chomp;			# no newline
	s/#.*//;		# no comments
	s/^\s+//;		# no leading whitespace
	s/\s+$//;		# no trailing whitespace
	next unless length;	# if nothing is left, then go get some more

	# chop the line up into pieces
	@line = split();

	print "\t{\"$line[0]\", ";
	print "$line[1], $line[2], $line[3], $line[4], $line[5]";
	print "},\n";
}

print "\t{NULL}\n};\n";

