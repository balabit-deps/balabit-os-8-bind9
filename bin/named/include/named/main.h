/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef NAMED_MAIN_H
#define NAMED_MAIN_H 1

/*! \file */

#ifdef ISC_MAIN_HOOK
#define main(argc, argv) bindmain(argc, argv)
#endif /* ifdef ISC_MAIN_HOOK */

/*
 * Commandline arguments for named; also referenced in win32/ntservice.c
 */
#define NAMED_MAIN_ARGS "46A:c:Cd:D:E:fFgL:M:m:n:N:p:sS:t:T:U:u:vVx:X:"

ISC_PLATFORM_NORETURN_PRE void
named_main_earlyfatal(const char *format, ...)
	ISC_FORMAT_PRINTF(1, 2) ISC_PLATFORM_NORETURN_POST;

void
named_main_earlywarning(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
named_main_setmemstats(const char *);

#endif /* NAMED_MAIN_H */
