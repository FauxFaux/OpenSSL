/* crypto/asn1/asn1_par.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include "cryptlib.h"
#include <openssl/buffer.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

static int asn1_print_info(BIO *bp, int tag, int xclass,int constructed,
	int indent);
static int asn1_parse2(BIO *bp, unsigned char **pp, long length,
	int offset, int depth, int indent);
static int asn1_print_info(BIO *bp, int tag, int xclass, int constructed,
	     int indent)
	{
	static const char fmt[]="%-18s";
	static const char fmt2[]="%2d %-15s";
	char str[128];
	const char *p,*p2=NULL;

	if (constructed & V_ASN1_CONSTRUCTED)
		p="cons: ";
	else
		p="prim: ";
	if (BIO_write(bp,p,6) < 6) goto err;
	if (indent)
		{
		if (indent > 128) indent=128;
		memset(str,' ',indent);
		if (BIO_write(bp,str,indent) < indent) goto err;
		}

	p=str;
	if ((xclass & V_ASN1_PRIVATE) == V_ASN1_PRIVATE)
		sprintf(str,"priv [ %d ] ",tag);
	else if ((xclass & V_ASN1_CONTEXT_SPECIFIC) == V_ASN1_CONTEXT_SPECIFIC)
		sprintf(str,"cont [ %d ]",tag);
	else if ((xclass & V_ASN1_APPLICATION) == V_ASN1_APPLICATION)
		sprintf(str,"appl [ %d ]",tag);
	else if ((tag == V_ASN1_EOC) /* && (xclass == V_ASN1_UNIVERSAL) */)
		p="EOC";
	else if (tag == V_ASN1_BOOLEAN)
		p="BOOLEAN";
	else if (tag == V_ASN1_INTEGER)
		p="INTEGER";
	else if (tag == V_ASN1_ENUMERATED)
		p="ENUMERATED";
	else if (tag == V_ASN1_BIT_STRING)
		p="BIT STRING";
	else if (tag == V_ASN1_OCTET_STRING)
		p="OCTET STRING";
	else if (tag == V_ASN1_NULL)
		p="NULL";
	else if (tag == V_ASN1_OBJECT)
		p="OBJECT";
	else if (tag == V_ASN1_SEQUENCE)
		p="SEQUENCE";
	else if (tag == V_ASN1_SET)
		p="SET";
	else if (tag == V_ASN1_PRINTABLESTRING)
		p="PRINTABLESTRING";
	else if (tag == V_ASN1_T61STRING)
		p="T61STRING";
	else if (tag == V_ASN1_IA5STRING)
		p="IA5STRING";
	else if (tag == V_ASN1_UTCTIME)
		p="UTCTIME";

	/* extras */
	else if (tag == V_ASN1_NUMERICSTRING)
		p="NUMERICSTRING";
	else if (tag == V_ASN1_VIDEOTEXSTRING)
		p="VIDEOTEXSTRING";
	else if (tag == V_ASN1_GENERALIZEDTIME)
		p="GENERALIZEDTIME";
	else if (tag == V_ASN1_GRAPHICSTRING)
		p="GRAPHICSTRING";
	else if (tag == V_ASN1_VISIBLESTRING)
		p="VISIBLESTRING";
	else if (tag == V_ASN1_GENERALSTRING)
		p="GENERALSTRING";
	else if (tag == V_ASN1_UNIVERSALSTRING)
		p="UNIVERSALSTRING";
	else if (tag == V_ASN1_BMPSTRING)
		p="BMPSTRING";
	else
		p2="(unknown)";
		
	if (p2 != NULL)
		{
		if (BIO_printf(bp,fmt2,tag,p2) <= 0) goto err;
		}
	else
		{
		if (BIO_printf(bp,fmt,p) <= 0) goto err;
		}
	return(1);
err:
	return(0);
	}

int ASN1_parse(BIO *bp, unsigned char *pp, long len, int indent)
	{
	return(asn1_parse2(bp,&pp,len,0,0,indent));
	}

static int asn1_parse2(BIO *bp, unsigned char **pp, long length, int offset,
	     int depth, int indent)
	{
	unsigned char *p,*ep,*tot,*op,*opp;
	long len;
	int tag,xclass,ret=0;
	int nl,hl,j,r;
	ASN1_OBJECT *o=NULL;
	ASN1_OCTET_STRING *os=NULL;
	/* ASN1_BMPSTRING *bmp=NULL;*/

	p= *pp;
	tot=p+length;
	op=p-1;
	while ((p < tot) && (op < p))
		{
		op=p;
		j=ASN1_get_object(&p,&len,&tag,&xclass,length);
#ifdef LINT
		j=j;
#endif
		if (j & 0x80)
			{
			if (BIO_write(bp,"Error in encoding\n",18) <= 0)
				goto end;
			ret=0;
			goto end;
			}
		hl=(p-op);
		length-=hl;
		/* if j == 0x21 it is a constructed indefinite length object */
		if (BIO_printf(bp,"%5ld:",(long)offset+(long)(op- *pp))
			<= 0) goto end;

		if (j != (V_ASN1_CONSTRUCTED | 1))
			{
			if (BIO_printf(bp,"d=%-2d hl=%ld l=%4ld ",
				depth,(long)hl,len) <= 0)
				goto end;
			}
		else
			{
			if (BIO_printf(bp,"d=%-2d hl=%ld l=inf  ",
				depth,(long)hl) <= 0)
				goto end;
			}
		if (!asn1_print_info(bp,tag,xclass,j,(indent)?depth:0))
			goto end;
		if (j & V_ASN1_CONSTRUCTED)
			{
			ep=p+len;
			if (BIO_write(bp,"\n",1) <= 0) goto end;
			if (len > length)
				{
				BIO_printf(bp,
					"length is greater than %ld\n",length);
				ret=0;
				goto end;
				}
			if ((j == 0x21) && (len == 0))
				{
				for (;;)
					{
					r=asn1_parse2(bp,&p,(long)(tot-p),
						offset+(p - *pp),depth+1,
						indent);
					if (r == 0) { ret=0; goto end; }
					if ((r == 2) || (p >= tot)) break;
					}
				}
			else
				while (p < ep)
					{
					r=asn1_parse2(bp,&p,(long)len,
						offset+(p - *pp),depth+1,
						indent);
					if (r == 0) { ret=0; goto end; }
					}
			}
		else if (xclass != 0)
			{
			p+=len;
			if (BIO_write(bp,"\n",1) <= 0) goto end;
			}
		else
			{
			nl=0;
			if (	(tag == V_ASN1_PRINTABLESTRING) ||
				(tag == V_ASN1_T61STRING) ||
				(tag == V_ASN1_IA5STRING) ||
				(tag == V_ASN1_VISIBLESTRING) ||
				(tag == V_ASN1_UTCTIME) ||
				(tag == V_ASN1_GENERALIZEDTIME))
				{
				if (BIO_write(bp,":",1) <= 0) goto end;
				if ((len > 0) &&
					BIO_write(bp,(char *)p,(int)len)
					!= (int)len)
					goto end;
				}
			else if (tag == V_ASN1_OBJECT)
				{
				opp=op;
				if (d2i_ASN1_OBJECT(&o,&opp,len+hl) != NULL)
					{
					if (BIO_write(bp,":",1) <= 0) goto end;
					i2a_ASN1_OBJECT(bp,o);
					}
				else
					{
					if (BIO_write(bp,":BAD OBJECT",11) <= 0)
						goto end;
					}
				}
			else if (tag == V_ASN1_BOOLEAN)
				{
				int ii;

				opp=op;
				ii=d2i_ASN1_BOOLEAN(NULL,&opp,len+hl);
				if (ii < 0)
					{
					if (BIO_write(bp,"Bad boolean\n",12))
						goto end;
					}
				BIO_printf(bp,":%d",ii);
				}
			else if (tag == V_ASN1_BMPSTRING)
				{
				/* do the BMP thang */
				}
			else if (tag == V_ASN1_OCTET_STRING)
				{
				int i,printable=1;

				opp=op;
				os=d2i_ASN1_OCTET_STRING(NULL,&opp,len+hl);
				if (os != NULL)
					{
					opp=os->data;
					for (i=0; i<os->length; i++)
						{
						if ((	(opp[i] < ' ') &&
							(opp[i] != '\n') &&
							(opp[i] != '\r') &&
							(opp[i] != '\t')) ||
							(opp[i] > '~'))
							{
							printable=0;
							break;
							}
						}
					if (printable && (os->length > 0))
						{
						if (BIO_write(bp,":",1) <= 0)
							goto end;
						if (BIO_write(bp,(char *)opp,
							os->length) <= 0)
							goto end;
						}
					ASN1_OCTET_STRING_free(os);
					os=NULL;
					}
				}
			else if (tag == V_ASN1_INTEGER)
				{
				ASN1_INTEGER *bs;
				int i;

				opp=op;
				bs=d2i_ASN1_INTEGER(NULL,&opp,len+hl);
				if (bs != NULL)
					{
					if (BIO_write(bp,":",1) <= 0) goto end;
					if (bs->type == V_ASN1_NEG_INTEGER)
						if (BIO_write(bp,"-",1) <= 0)
							goto end;
					for (i=0; i<bs->length; i++)
						{
						if (BIO_printf(bp,"%02X",
							bs->data[i]) <= 0)
							goto end;
						}
					if (bs->length == 0)
						{
						if (BIO_write(bp,"00",2) <= 0)
							goto end;
						}
					}
				else
					{
					if (BIO_write(bp,"BAD INTEGER",11) <= 0)
						goto end;
					}
				ASN1_INTEGER_free(bs);
				}
			else if (tag == V_ASN1_ENUMERATED)
				{
				ASN1_ENUMERATED *bs;
				int i;

				opp=op;
				bs=d2i_ASN1_ENUMERATED(NULL,&opp,len+hl);
				if (bs != NULL)
					{
					if (BIO_write(bp,":",1) <= 0) goto end;
					if (bs->type == V_ASN1_NEG_ENUMERATED)
						if (BIO_write(bp,"-",1) <= 0)
							goto end;
					for (i=0; i<bs->length; i++)
						{
						if (BIO_printf(bp,"%02X",
							bs->data[i]) <= 0)
							goto end;
						}
					if (bs->length == 0)
						{
						if (BIO_write(bp,"00",2) <= 0)
							goto end;
						}
					}
				else
					{
					if (BIO_write(bp,"BAD ENUMERATED",11) <= 0)
						goto end;
					}
				ASN1_ENUMERATED_free(bs);
				}

			if (!nl) 
				{
				if (BIO_write(bp,"\n",1) <= 0) goto end;
				}
			p+=len;
			if ((tag == V_ASN1_EOC) && (xclass == 0))
				{
				ret=2; /* End of sequence */
				goto end;
				}
			}
		length-=len;
		}
	ret=1;
end:
	if (o != NULL) ASN1_OBJECT_free(o);
	if (os != NULL) ASN1_OCTET_STRING_free(os);
	*pp=p;
	return(ret);
	}
