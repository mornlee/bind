/*
 * Copyright (C) 1999, 2000  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: compress.c,v 1.39 2000/12/29 19:38:50 bwelling Exp $ */

#define DNS_NAME_USEINLINE 1

#include <config.h>

#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/rbt.h>
#include <dns/result.h>

#define CCTX_MAGIC	0x43435458U	/* CCTX */
#define VALID_CCTX(x)	ISC_MAGIC_VALID(x, CCTX_MAGIC)

#define DCTX_MAGIC	0x44435458U	/* DCTX */
#define VALID_DCTX(x)	ISC_MAGIC_VALID(x, DCTX_MAGIC)

/***
 ***	Compression
 ***/

isc_result_t
dns_compress_init(dns_compress_t *cctx, int edns, isc_mem_t *mctx) {
	unsigned int i;

	REQUIRE(cctx != NULL);
	REQUIRE(mctx != NULL);

	cctx->allowed = 0;
	cctx->rdata = 0;
	cctx->edns = edns;
	for (i = 0; i < DNS_COMPRESS_TABLESIZE; i++)
		cctx->table[i] = NULL;
	cctx->mctx = mctx;
	cctx->count = 0;
	cctx->magic = CCTX_MAGIC;
	return (ISC_R_SUCCESS);
}

void
dns_compress_invalidate(dns_compress_t *cctx) {
	dns_compressnode_t *node;
	unsigned int i;

	REQUIRE(VALID_CCTX(cctx));

	cctx->magic = 0;
	if (cctx->table != NULL) {
		for (i = 0; i < DNS_COMPRESS_TABLESIZE; i++) {
			while (cctx->table[i] != NULL) {
				node = cctx->table[i];
				cctx->table[i] = cctx->table[i]->next;
				if (node->count < DNS_COMPRESS_INITIALNODES)
					continue;
				isc_mem_put(cctx->mctx, node,
					    sizeof(dns_compressnode_t));
			}
		}
	}
	cctx->allowed = 0;
	cctx->rdata = 0;
	cctx->edns = -1;
}

void
dns_compress_setmethods(dns_compress_t *cctx, unsigned int allowed) {
	REQUIRE(VALID_CCTX(cctx));

	cctx->allowed = allowed;
}

unsigned int
dns_compress_getmethods(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));
	return (cctx->allowed);
}

int
dns_compress_getedns(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));
	return (cctx->edns);
}

/*
 * Find the longest match of name in the table.
 * If match is found return ISC_TRUE. prefix, suffix and offset are updated.
 * If no match is found return ISC_FALSE.
 */
isc_boolean_t
dns_compress_findglobal(dns_compress_t *cctx, dns_name_t *name,
			dns_name_t *prefix, dns_name_t *suffix,
			isc_uint16_t *offset)
{
	dns_name_t tname, nname;
	dns_compressnode_t *node = NULL;
	unsigned int labels, hash, n;

	REQUIRE(VALID_CCTX(cctx));
	REQUIRE(dns_name_isabsolute(name) == ISC_TRUE);
	REQUIRE(offset != NULL);

	labels = dns_name_countlabels(name);
	INSIST(labels > 0);

	dns_name_init(&tname, NULL);
	dns_name_init(&nname, NULL);

	for (n = 0; n < labels - 1; n++) {
		dns_name_getlabelsequence(name, n, labels - n, &tname);
		hash = dns_name_hash(&tname, ISC_FALSE) %
		       DNS_COMPRESS_TABLESIZE;
		for (node = cctx->table[hash]; node != NULL; node = node->next)
		{
			dns_name_fromregion(&nname, &node->r);
			if (dns_name_equal(&nname, &tname))
				break;
		}
		if (node != NULL)
			break;
	}

	/*
	 * If node == NULL, we found no match at all.
	 */
	if (node == NULL)
		return (ISC_FALSE);

	dns_name_clone(&tname, suffix);
	dns_name_getlabelsequence(name, 0, n, prefix);
	*offset = node->offset;
	return (ISC_TRUE);
}

void
dns_compress_add(dns_compress_t *cctx, dns_name_t *name, isc_uint16_t offset) {
	dns_name_t tname, nname;
	dns_label_t label;
	unsigned int start;
	unsigned int n;
	unsigned int hash;
	dns_compressnode_t *node;

	REQUIRE(VALID_CCTX(cctx));
	REQUIRE(dns_name_isabsolute(name));

	dns_name_init(&tname, NULL);
	dns_name_init(&nname, NULL);

	n = dns_name_countlabels(name);
	start = 0;
	while (n > 1) {
		if (offset >= 0x4000)
			break;
		dns_name_getlabelsequence(name, start, n, &tname);
		hash = dns_name_hash(&tname, ISC_FALSE) %
		       DNS_COMPRESS_TABLESIZE;
		/*
		 * Look for the name in the hash bucket.  If it's there,
		 * we're done.
		 */
		for (node = cctx->table[hash]; node != NULL; node = node->next)
		{
			dns_name_fromregion(&nname, &node->r);
			if (dns_name_equal(&nname, &tname))
				return;
		}
		/*
		 * It's not there.  Create a new node and add it.
		 */
		if (cctx->count < DNS_COMPRESS_INITIALNODES)
			node = &cctx->initialnodes[cctx->count];
		else {
			node = isc_mem_get(cctx->mctx,
					   sizeof(dns_compressnode_t));
			if (node == NULL)
				return;
		}
		node->count = cctx->count++;
		node->offset = offset;
		dns_name_toregion(&tname, &node->r);
		node->next = cctx->table[hash];
		cctx->table[hash] = node;
		dns_name_getlabel(&tname, 0, &label);
		offset += label.length;
		start++;
		n--;
	}
}

void
dns_compress_rollback(dns_compress_t *cctx, isc_uint16_t offset) {
	unsigned int i;
	dns_compressnode_t *node;

	REQUIRE(VALID_CCTX(cctx));

	for (i = 0; i < DNS_COMPRESS_TABLESIZE; i++) {
		node = cctx->table[i];
		while (node != NULL && node->offset >= offset) {
			cctx->table[i] = node->next;
			if (node->count >= DNS_COMPRESS_INITIALNODES)
				isc_mem_put(cctx->mctx, node,
					    sizeof(dns_compressnode_t));
			cctx->count--;
			node = cctx->table[i];
		}
	}
}

/***
 ***	Decompression
 ***/

void
dns_decompress_init(dns_decompress_t *dctx, int edns,
		    dns_decompresstype_t type) {

	REQUIRE(dctx != NULL);
	REQUIRE(edns >= -1 && edns <= 255);

	dctx->allowed = DNS_COMPRESS_NONE;
	dctx->edns = edns;
	dctx->type = type;
	dctx->rdata = 0;
	dctx->magic = DCTX_MAGIC;
}

void
dns_decompress_invalidate(dns_decompress_t *dctx) {

	REQUIRE(VALID_DCTX(dctx));

	dctx->magic = 0;
}

void
dns_decompress_setmethods(dns_decompress_t *dctx, unsigned int allowed) {

	REQUIRE(VALID_DCTX(dctx));

	switch (dctx->type) {
	case DNS_DECOMPRESS_ANY:
		dctx->allowed = DNS_COMPRESS_ALL;
		break;
	case DNS_DECOMPRESS_NONE:
		dctx->allowed = DNS_COMPRESS_NONE;
		break;
	case DNS_DECOMPRESS_STRICT:
		dctx->allowed = allowed;
		break;
	}
}

unsigned int
dns_decompress_getmethods(dns_decompress_t *dctx) {

	REQUIRE(VALID_DCTX(dctx));

	return (dctx->allowed);
}

int
dns_decompress_edns(dns_decompress_t *dctx) {

	REQUIRE(VALID_DCTX(dctx));

	return (dctx->edns);
}

dns_decompresstype_t
dns_decompress_type(dns_decompress_t *dctx) {

	REQUIRE(VALID_DCTX(dctx));

	return (dctx->type);
}
