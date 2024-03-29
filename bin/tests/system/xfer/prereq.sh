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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

if $PERL -e 'use Net::DNS;' 2>/dev/null; then
  if $PERL -e 'use Net::DNS; die if ($Net::DNS::VERSION >= 0.69 && $Net::DNS::VERSION <= 0.74);' 2>/dev/null; then
    :
  else
    echo_i "Net::DNS versions 0.69 to 0.74 have bugs that cause this test to fail: please update." >&2
    exit 1
  fi
else
  echo_i "This test requires the Net::DNS library." >&2
  exit 1
fi

if ! $PERL -e 'use Digest::HMAC;' 2>/dev/null; then
  echo_i "This test requires the Digest::HMAC Perl module." >&2
  exit 1
fi
