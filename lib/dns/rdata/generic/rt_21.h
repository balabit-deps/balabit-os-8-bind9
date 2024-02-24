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

#ifndef GENERIC_RT_21_H
#define GENERIC_RT_21_H 1

/*!
 *  \brief Per RFC1183 */

typedef struct dns_rdata_rt {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t preference;
	dns_name_t host;
} dns_rdata_rt_t;

#endif /* GENERIC_RT_21_H */
