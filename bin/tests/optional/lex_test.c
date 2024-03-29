/*
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

/*! \file */

#include <isc/commandline.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/util.h>

isc_mem_t *mctx;
isc_lex_t *lex;

isc_lexspecials_t specials;

static void
print_token(isc_token_t *tokenp, FILE *stream) {
	switch (tokenp->type) {
	case isc_tokentype_unknown:
		fprintf(stream, "UNKNOWN");
		break;
	case isc_tokentype_string:
		fprintf(stream, "STRING %.*s",
			(int)tokenp->value.as_region.length,
			tokenp->value.as_region.base);
		break;
	case isc_tokentype_number:
		fprintf(stream, "NUMBER %lu", tokenp->value.as_ulong);
		break;
	case isc_tokentype_qstring:
		fprintf(stream, "QSTRING \"%.*s\"",
			(int)tokenp->value.as_region.length,
			tokenp->value.as_region.base);
		break;
	case isc_tokentype_eol:
		fprintf(stream, "EOL");
		break;
	case isc_tokentype_eof:
		fprintf(stream, "EOF");
		break;
	case isc_tokentype_initialws:
		fprintf(stream, "INITIALWS");
		break;
	case isc_tokentype_special:
		fprintf(stream, "SPECIAL %c", tokenp->value.as_char);
		break;
	case isc_tokentype_nomore:
		fprintf(stream, "NOMORE");
		break;
	default:
		FATAL_ERROR(__FILE__, __LINE__, "Unexpected type %d",
			    tokenp->type);
	}
}

int
main(int argc, char *argv[]) {
	isc_token_t token;
	isc_result_t result;
	int quiet = 0;
	int c;
	int masterfile = 1;
	int stats = 0;
	unsigned int options = 0;
	int done = 0;

	while ((c = isc_commandline_parse(argc, argv, "qmcs")) != -1) {
		switch (c) {
		case 'q':
			quiet = 1;
			break;
		case 'm':
			masterfile = 1;
			break;
		case 'c':
			masterfile = 0;
			break;
		case 's':
			stats = 1;
			break;
		}
	}

	isc_mem_create(&mctx);
	RUNTIME_CHECK(isc_lex_create(mctx, 256, &lex) == ISC_R_SUCCESS);

	if (masterfile) {
		/* Set up to lex DNS master file. */

		specials['('] = 1;
		specials[')'] = 1;
		specials['"'] = 1;
		isc_lex_setspecials(lex, specials);
		options = ISC_LEXOPT_DNSMULTILINE | ISC_LEXOPT_ESCAPE |
			  ISC_LEXOPT_EOF | ISC_LEXOPT_QSTRING |
			  ISC_LEXOPT_NOMORE;
		isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);
	} else {
		/* Set up to lex DNS config file. */

		specials['{'] = 1;
		specials['}'] = 1;
		specials[';'] = 1;
		specials['/'] = 1;
		specials['"'] = 1;
		specials['!'] = 1;
		specials['*'] = 1;
		isc_lex_setspecials(lex, specials);
		options = ISC_LEXOPT_EOF | ISC_LEXOPT_QSTRING |
			  ISC_LEXOPT_NUMBER | ISC_LEXOPT_NOMORE;
		isc_lex_setcomments(lex, (ISC_LEXCOMMENT_C |
					  ISC_LEXCOMMENT_CPLUSPLUS |
					  ISC_LEXCOMMENT_SHELL));
	}

	RUNTIME_CHECK(isc_lex_openstream(lex, stdin) == ISC_R_SUCCESS);

	while ((result = isc_lex_gettoken(lex, options, &token)) ==
		       ISC_R_SUCCESS &&
	       !done)
	{
		if (!quiet) {
			char *name = isc_lex_getsourcename(lex);
			print_token(&token, stdout);
			printf(" line = %lu file = %s\n",
			       isc_lex_getsourceline(lex),
			       (name == NULL) ? "<none>" : name);
		}
		if (token.type == isc_tokentype_eof) {
			isc_lex_close(lex);
		}
		if (token.type == isc_tokentype_nomore) {
			done = 1;
		}
	}
	if (result != ISC_R_SUCCESS) {
		printf("Result: %s\n", isc_result_totext(result));
	}

	isc_lex_close(lex);
	isc_lex_destroy(&lex);
	if (!quiet && stats) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	return (0);
}
