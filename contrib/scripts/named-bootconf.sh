#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# $NetBSD: named-bootconf.sh,v 1.5 1998/12/15 01:00:53 tron Exp $
#
# Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Matthias Scheler.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

if [ ${OPTIONFILE-X} = X ]; then
	WORKDIR=/tmp/`date +%s`.$$
	( umask 077 ; mkdir $WORKDIR ) || {
		echo "unable to create work directory '$WORKDIR'" >&2 
		exit 1
	}
	OPTIONFILE=$WORKDIR/options
	ZONEFILE=$WORKDIR/zones
	COMMENTFILE=$WORKDIR/comments
	export OPTIONFILE ZONEFILE COMMENTFILE
	touch $OPTIONFILE $ZONEFILE $COMMENTFILE
	DUMP=1
else
	DUMP=0
fi

while read CMD ARGS; do
	class=
	CMD=`echo "${CMD}" | tr '[A-Z]' '[a-z]'`
	case $CMD in
	\; )
		echo \# $ARGS >>$COMMENTFILE
		;;
	cache )
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			(echo ""
			cat $COMMENTFILE
			echo "zone \"$1\" {"
			echo "	type hint;"
			echo "	file \"$2\";"
			echo "};") >>$ZONEFILE
			rm -f $COMMENTFILE
			touch $COMMENTFILE
		fi
		;;
	directory )
		set - X $ARGS
		shift
		if [ $# -eq 1 ]; then
			(cat $COMMENTFILE
			echo "	directory \"$1\";") >>$OPTIONFILE
			rm -f $COMMENTFILE
			touch $COMMENTFILE

			DIRECTORY=$1
			export DIRECTORY
		fi
		;;
	forwarders )
		(cat $COMMENTFILE
		echo "	forwarders {"
		for ARG in $ARGS; do
			echo "		$ARG;"
		done
		echo "	};") >>$OPTIONFILE
		rm -f $COMMENTFILE
		touch $COMMENTFILE
		;;
	include )
		if [ "$ARGS" != "" ]; then
			(cd ${DIRECTORY-.}; cat $ARGS) | $0
		fi
		;;
	limit )
		ARGS=`echo "${ARGS}" | tr '[A-Z]' '[a-z]'`
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			cat $COMMENTFILE >>$OPTIONFILE
			case $1 in
			datasize | files | transfers-in | transfers-per-ns )
				echo "	$1 $2;" >>$OPTIONFILE
				;;
			esac
			rm -f $COMMENTFILE
			touch $COMMENTFILE
		fi
		;;
	options )
		ARGS=`echo "${ARGS}" | tr '[A-Z]' '[a-z]'`
		cat $COMMENTFILE >>$OPTIONFILE
		for ARG in $ARGS; do
			case $ARG in
			fake-iquery )
				echo "	fake-iquery yes;" >>$OPTIONFILE
				;;
			forward-only )
				echo "	forward only;" >>$OPTIONFILE
				;;
			no-fetch-glue )
				echo "	fetch-glue no;" >>$OPTIONFILE
				;;
			no-recursion )
				echo "	recursion no;" >>$OPTIONFILE
				;;
			esac
		done
		rm -f $COMMENTFILE
		touch $COMMENTFILE
		;;
	primary|primary/* )
		case $CMD in
		primary/chaos )
			class="chaos "
			;;
		primary/hs )
			class="hesiod "
			;;
		esac
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			(echo ""
			cat $COMMENTFILE
			echo "zone \"$1\" ${class}{"
			echo "	type master;"
			echo "	file \"$2\";"
			echo "};") >>$ZONEFILE
			rm -f $COMMENTFILE
			touch $COMMENTFILE
		fi
		;;
	secondary|secondary/* )
		case $CMD in
		secondary/chaos )
			class="chaos "
			;;
		secondary/hs )
			class="hesiod "
			;;
		esac
		set - X $ARGS
		shift
		if [ $# -gt 2 ]; then
			ZONE=$1
			shift
			PRIMARIES=$1
			while [ $# -gt 2 ]; do
				shift
				PRIMARIES="$PRIMARIES $1"
			done
			(echo ""
			cat $COMMENTFILE
			echo "zone \"$ZONE\" ${class}{"
			echo "	type slave;"
			echo "	file \"$2\";"
			echo "	masters {"
			for PRIMARY in $PRIMARIES; do
				echo "		$PRIMARY;"
			done
			echo "	};"
			echo "};") >>$ZONEFILE
			rm -f $COMMENTFILE
			touch $COMMENTFILE
		fi
		;;
	stub|stub/* )
		case $CMD in
		stub/chaos )
			class="chaos "
			;;
		stub/hs )
			class="hesiod "
			;;
		esac
		set - X $ARGS
		shift
		if [ $# -gt 2 ]; then
			ZONE=$1
			shift
			PRIMARIES=$1
			while [ $# -gt 2 ]; do
				shift
				PRIMARIES="$PRIMARIES $1"
			done
			(echo ""
			cat $COMMENTFILE
			echo "zone \"$ZONE\" ${class}{"
			echo "	type stub;"
			echo "	file \"$2\";"
			echo "	masters {"
			for PRIMARY in $PRIMARIES; do
				echo "		$PRIMARY;"
			done
			echo "	};"
			echo "};") >>$ZONEFILE
			rm -f $COMMENTFILE
			touch $COMMENTFILE
		fi
		;;
	slave )
		cat $COMMENTFILE >>$OPTIONFILE
		echo "	forward only;" >>$OPTIONFILE
		rm -f $COMMENTFILE
		touch $COMMENTFILE
		;;
	sortlist )
		(cat $COMMENTFILE
		echo "	topology {"
		for ARG in $ARGS; do
			case $ARG in
			*.0.0.0 )
				echo "		$ARG/8;"
				;;
			*.0.0 )
				echo "		$ARG/16;"
				;;
			*.0 )
				echo "		$ARG/24;"
				;;
			* )
				echo "		$ARG;"
				;;
			esac
		done
		echo "	};") >>$OPTIONFILE
		rm -f $COMMENTFILE
		touch $COMMENTFILE
		;;
	tcplist | xfrnets )
		(cat $COMMENTFILE
		echo "	allow-transfer {"
		for ARG in $ARGS; do
			case $ARG in
			*.0.0.0 )
				echo "		$ARG/8;"
				;;
			*.0.0 )
				echo "		$ARG/16;"
				;;
			*.0 )
				echo "		$ARG/24;"
				;;
			* )
				echo "		$ARG;"
				;;
			esac
		done
		echo "	};") >>$OPTIONFILE
		rm -f $COMMENTFILE
		touch $COMMENTFILE
		;;
	esac
done

if [ $DUMP -eq 1 ]; then
	echo ""
	echo "options {"
	cat $OPTIONFILE
	echo "};"
	cat $ZONEFILE $COMMENTFILE

	rm -f $OPTIONFILE $ZONEFILE $COMMENTFILE
	rmdir $WORKDIR
fi

exit 0
