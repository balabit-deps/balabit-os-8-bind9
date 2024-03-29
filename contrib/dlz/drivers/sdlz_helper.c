/*
 * Copyright (C) 2002 Stichting NLnet, Netherlands, stichting@nlnet.nl.
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 1999-2001, 2016  Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/log.h>
#include <dns/result.h>

#include <dlz/sdlz_helper.h>

/*
 * sdlz helper methods
 */

/*%
 * properly destroys a querylist by de-allocating the
 * memory for each query segment, and then the list itself
 */

static void
destroy_querylist(isc_mem_t *mctx, query_list_t **querylist) {
	query_segment_t *tseg = NULL;
	query_segment_t *nseg = NULL;

	REQUIRE(mctx != NULL);

	/* if query list is null, nothing to do */
	if (*querylist == NULL) {
		return;
	}

	/* start at the top of the list */
	nseg = ISC_LIST_HEAD(**querylist);
	while (nseg != NULL) { /* loop, until end of list */
		tseg = nseg;
		/*
		 * free the query segment's text string but only if it
		 * was really a query segment, and not a pointer to
		 * %zone%, or %record%, or %client%
		 */
		if (tseg->sql != NULL && tseg->direct) {
			isc_mem_free(mctx, tseg->sql);
		}
		/* get the next query segment, before we destroy this one. */
		nseg = ISC_LIST_NEXT(nseg, link);
		/* deallocate this query segment. */
		isc_mem_put(mctx, tseg, sizeof(query_segment_t));
	}
	/* deallocate the query segment list */
	isc_mem_put(mctx, *querylist, sizeof(query_list_t));
}

/*% constructs a query list by parsing a string into query segments */
static isc_result_t
build_querylist(isc_mem_t *mctx, const char *query_str, char **zone,
		char **record, char **client, query_list_t **querylist,
		unsigned int flags) {
	isc_result_t result;
	bool foundzone = false;
	bool foundrecord = false;
	bool foundclient = false;
	char *free_me = NULL;
	char *temp_str = NULL;
	query_list_t *tql;
	query_segment_t *tseg = NULL;
	char *last = NULL;

	REQUIRE(querylist != NULL && *querylist == NULL);
	REQUIRE(mctx != NULL);

	/* if query string is null, or zero length */
	if (query_str == NULL || strlen(query_str) < 1) {
		if ((flags & SDLZH_REQUIRE_QUERY) == 0) {
			/* we don't need it were ok. */
			return (ISC_R_SUCCESS);
		} else {
			/* we did need it, PROBLEM!!! */
			return (ISC_R_FAILURE);
		}
	}

	/* allocate memory for query list */
	tql = isc_mem_get(mctx, sizeof(query_list_t));

	/* initialize the query segment list */
	ISC_LIST_INIT(*tql);

	/* make a copy of query_str so we can chop it up */
	free_me = temp_str = isc_mem_strdup(mctx, query_str);
	/* couldn't make a copy, problem!! */
	if (temp_str == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	/* loop through the string and chop it up */
	for (;;) {
		/*
		 * Split string into tokens at '$'.
		 */
		const char *sql = strtok_r(temp_str, "$", &last);
		if (sql == NULL) {
			break;
		}
		temp_str = NULL;

		/* allocate memory for tseg */
		tseg = isc_mem_get(mctx, sizeof(query_segment_t));
		tseg->sql = NULL;
		tseg->direct = false;
		/* initialize the query segment link */
		ISC_LINK_INIT(tseg, link);
		/* append the query segment to the list */
		ISC_LIST_APPEND(*tql, tseg, link);

		tseg->sql = isc_mem_strdup(mctx, sql);
		/* tseg->sql points directly to a string. */
		tseg->direct = true;
		tseg->strlen = strlen(tseg->sql);

		/* check if we encountered "$zone$" token */
		if (strcasecmp(tseg->sql, "zone") == 0) {
			/*
			 * we don't really need, or want the "zone"
			 * text, so get rid of it.
			 */
			isc_mem_free(mctx, tseg->sql);
			/* set tseg->sql to in-direct zone string */
			tseg->sql = zone;
			tseg->strlen = 0;
			/* tseg->sql points in-directly to a string */
			tseg->direct = false;
			foundzone = true;
			/* check if we encountered "$record$" token */
		} else if (strcasecmp(tseg->sql, "record") == 0) {
			/*
			 * we don't really need, or want the "record"
			 * text, so get rid of it.
			 */
			isc_mem_free(mctx, tseg->sql);
			/* set tseg->sql to in-direct record string */
			tseg->sql = record;
			tseg->strlen = 0;
			/* tseg->sql points in-directly points to a string */
			tseg->direct = false;
			foundrecord = true;
			/* check if we encountered "$client$" token */
		} else if (strcasecmp(tseg->sql, "client") == 0) {
			/*
			 * we don't really need, or want the "client"
			 * text, so get rid of it.
			 */
			isc_mem_free(mctx, tseg->sql);
			/* set tseg->sql to in-direct record string */
			tseg->sql = client;
			tseg->strlen = 0;
			/* tseg->sql points in-directly points to a string */
			tseg->direct = false;
			foundclient = true;
		}
	}

	/* we don't need temp_str any more */
	isc_mem_free(mctx, free_me);
	/*
	 * add checks later to verify zone and record are found if
	 * necessary.
	 */

	/* if this query requires %client%, make sure we found it */
	if (((flags & SDLZH_REQUIRE_CLIENT) != 0) && (!foundclient)) {
		/* Write error message to log */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Required token $client$ not found.");
		result = ISC_R_FAILURE;
		goto flag_fail;
	}

	/* if this query requires %record%, make sure we found it */
	if (((flags & SDLZH_REQUIRE_RECORD) != 0) && (!foundrecord)) {
		/* Write error message to log */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Required token $record$ not found.");
		result = ISC_R_FAILURE;
		goto flag_fail;
	}

	/* if this query requires %zone%, make sure we found it */
	if (((flags & SDLZH_REQUIRE_ZONE) != 0) && (!foundzone)) {
		/* Write error message to log */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Required token $zone$ not found.");
		result = ISC_R_FAILURE;
		goto flag_fail;
	}

	/* pass back the query list */
	*querylist = (query_list_t *)tql;

	/* return success */
	return (ISC_R_SUCCESS);

cleanup:
	/* get rid of free_me */
	if (free_me != NULL) {
		isc_mem_free(mctx, free_me);
	}

flag_fail:
	/* get rid of what was build of the query list */
	destroy_querylist(mctx, &tql);
	return (result);
}

/*%
 * build a query string from query segments, and dynamic segments
 * dynamic segments replace where the tokens %zone%, %record%, %client%
 * used to be in our queries from named.conf
 */
char *
sdlzh_build_querystring(isc_mem_t *mctx, query_list_t *querylist) {
	query_segment_t *tseg = NULL;
	unsigned int length = 0;
	char *qs = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(querylist != NULL);

	/* start at the top of the list */
	tseg = ISC_LIST_HEAD(*querylist);
	while (tseg != NULL) {
		/*
		 * if this is a query segment, use the
		 * precalculated string length
		 */
		if (tseg->direct) {
			length += tseg->strlen;
		} else { /* calculate string length for dynamic segments. */
			length += strlen(*(char **)tseg->sql);
		}
		/* get the next segment */
		tseg = ISC_LIST_NEXT(tseg, link);
	}

	/* allocate memory for the string */
	qs = isc_mem_allocate(mctx, length + 1);

	*qs = 0;
	/* start at the top of the list again */
	tseg = ISC_LIST_HEAD(*querylist);
	while (tseg != NULL) {
		if (tseg->direct) {
			/* query segments */
			strcat(qs, tseg->sql);
		} else {
			/* dynamic segments */
			strcat(qs, *(char **)tseg->sql);
		}
		/* get the next segment */
		tseg = ISC_LIST_NEXT(tseg, link);
	}

	return (qs);
}

/*% constructs a sql dbinstance (DBI) */
isc_result_t
sdlzh_build_sqldbinstance(isc_mem_t *mctx, const char *allnodes_str,
			  const char *allowxfr_str, const char *authority_str,
			  const char *findzone_str, const char *lookup_str,
			  const char *countzone_str, dbinstance_t **dbi) {
	isc_result_t result;
	dbinstance_t *db = NULL;

	REQUIRE(dbi != NULL && *dbi == NULL);
	REQUIRE(mctx != NULL);

	/* allocate and zero memory for driver structure */
	db = isc_mem_get(mctx, sizeof(dbinstance_t));
	memset(db, 0, sizeof(dbinstance_t));
	db->dbconn = NULL;
	db->client = NULL;
	db->record = NULL;
	db->zone = NULL;
	db->mctx = NULL;
	db->query_buf = NULL;
	db->allnodes_q = NULL;
	db->allowxfr_q = NULL;
	db->authority_q = NULL;
	db->findzone_q = NULL;
	db->countzone_q = NULL;
	db->lookup_q = NULL;

	/* attach to the memory context */
	isc_mem_attach(mctx, &db->mctx);

	/* initialize the reference count mutex */
	isc_mutex_init(&db->instance_lock);

	/* build the all nodes query list */
	result = build_querylist(mctx, allnodes_str, &db->zone, &db->record,
				 &db->client, &db->allnodes_q,
				 SDLZH_REQUIRE_ZONE);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build all nodes query list");
		goto cleanup;
	}

	/* build the allow zone transfer query list */
	result = build_querylist(mctx, allowxfr_str, &db->zone, &db->record,
				 &db->client, &db->allowxfr_q,
				 SDLZH_REQUIRE_ZONE | SDLZH_REQUIRE_CLIENT);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build allow xfr query list");
		goto cleanup;
	}

	/* build the authority query, query list */
	result = build_querylist(mctx, authority_str, &db->zone, &db->record,
				 &db->client, &db->authority_q,
				 SDLZH_REQUIRE_ZONE);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build authority query list");
		goto cleanup;
	}

	/* build findzone query, query list */
	result = build_querylist(mctx, findzone_str, &db->zone, &db->record,
				 &db->client, &db->findzone_q,
				 SDLZH_REQUIRE_ZONE);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build find zone query list");
		goto cleanup;
	}

	/* build countzone query, query list */
	result = build_querylist(mctx, countzone_str, &db->zone, &db->record,
				 &db->client, &db->countzone_q,
				 SDLZH_REQUIRE_ZONE);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build count zone query list");
		goto cleanup;
	}

	/* build lookup query, query list */
	result = build_querylist(mctx, lookup_str, &db->zone, &db->record,
				 &db->client, &db->lookup_q,
				 SDLZH_REQUIRE_RECORD);
	/* if unsuccessful, log err msg and cleanup */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "Could not build lookup query list");
		goto cleanup;
	}

	/* pass back the db instance */
	*dbi = (dbinstance_t *)db;

	/* return success */
	return (ISC_R_SUCCESS);

cleanup:
	/* destroy whatever was build of the db instance */
	destroy_sqldbinstance(db);
	/* return failure */
	return (ISC_R_FAILURE);
}

void
sdlzh_destroy_sqldbinstance(dbinstance_t *dbi) {
	isc_mem_t *mctx;

	/* save mctx for later */
	mctx = dbi->mctx;

	/* destroy any query lists we created */
	destroy_querylist(mctx, &dbi->allnodes_q);
	destroy_querylist(mctx, &dbi->allowxfr_q);
	destroy_querylist(mctx, &dbi->authority_q);
	destroy_querylist(mctx, &dbi->findzone_q);
	destroy_querylist(mctx, &dbi->countzone_q);
	destroy_querylist(mctx, &dbi->lookup_q);

	/* get rid of the mutex */
	(void)isc_mutex_destroy(&dbi->instance_lock);

	/* return, and detach the memory */
	isc_mem_putanddetach(&mctx, dbi, sizeof(dbinstance_t));
}

char *
sdlzh_get_parameter_value(isc_mem_t *mctx, const char *input, const char *key) {
	int keylen;
	char *keystart;
	char value[255];
	int i;

	if (key == NULL || input == NULL || strlen(input) < 1) {
		return (NULL);
	}

	keylen = strlen(key);

	if (keylen < 1) {
		return (NULL);
	}

	keystart = strstr(input, key);

	if (keystart == NULL) {
		return (NULL);
	}

	REQUIRE(mctx != NULL);

	for (i = 0; i < 255; i++) {
		value[i] = keystart[keylen + i];
		if (value[i] == ' ' || value[i] == '\0') {
			value[i] = '\0';
			break;
		}
	}

	return (isc_mem_strdup(mctx, value));
}
