#!/bin/bash
#
# COPYRIGHT
#   Copyright (c) 2006 Bob Tanner <tanner@real-time.com>
#
# LICENSE
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the Free
#   Software Foundation version 2
#
#   This program is distributed in the hope that it will be useful, but WITHOUT
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#   more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc., 59
#   Temple Place, Suite 330, Boston, MA 02111-1307 USA.
#

# Scan these locations for KEYDEF entries. First entry is my personal
# key, the second is Quozl's, and the third is fake, used as an
# example of how to add other scan locations
#
SCAN="/home/tanner/.netrek/key-debian.def \
	/home/quozl/.netrek/key-debian.def \
	/home/a/ahn/src/netrek/keydef \
     "

# Look for a key 
#
for key in ${SCAN}; do
    if [ -r $key ]; then
        echo $key
	exit 0
    fi
done

# If we get here we didn't have a key, so do the default
#
echo "sample_key.def"

exit 0
