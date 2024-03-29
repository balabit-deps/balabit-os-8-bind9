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

now=$($PERL -e 'print time()."\n";')
for keyfile in K*.key; do
  inactive=$($SETTIME -upI $keyfile | awk '{print $2}')
  if [ "$inactive" = UNSET ]; then
    continue
  elif [ "$inactive" -lt "$now" ]; then
    echo_d "inactive date is in the past"
    ret=1
  fi
done
