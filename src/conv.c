/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include "system4.h"
#include "alice.h"

static char *convert_text(iconv_t cd, const char *str)
{
	size_t inbytesleft = strlen(str);
	char *inbuf = (char*)str;

	size_t outbuf_size = inbytesleft;
	size_t outbytesleft = outbuf_size;
	char *outbuf = xmalloc(outbuf_size+1);
	char *outptr = outbuf;

	while (inbytesleft) {
		if (iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft) == (size_t)-1 && errno != E2BIG) {
			if (*current_file_name)
				ALICE_ERROR("%s:%lu: iconv: %s", *current_file_name, *current_line_nr, strerror(errno));
			else
				ALICE_ERROR("iconv: %s", strerror(errno));
		}
		// reallocate outbuf
		size_t out_index = outbuf_size - outbytesleft;
		outbytesleft += outbuf_size;
		outbuf_size *= 2;
		outbuf = xrealloc(outbuf, outbuf_size+1);
		outptr = outbuf + out_index;
	}
	*outptr = '\0';

	return outbuf;
}

static const char *input_encoding = "CP932";
static const char *output_encoding = "UTF-8";
static iconv_t output_conv = (iconv_t)-1;
static iconv_t utf8_conv = (iconv_t)-1;
static iconv_t output_utf8_conv = (iconv_t)-1;

void set_input_encoding(const char *enc)
{
	input_encoding = enc;
}

void set_output_encoding(const char *enc)
{
	output_encoding = enc;
}

char *conv_output(const char *str)
{
	if (output_conv == (iconv_t)-1 && (output_conv = iconv_open(output_encoding, input_encoding)) == (iconv_t)-1)
		ALICE_ERROR("iconv_open: %s", strerror(errno));
	return convert_text(output_conv, str);
}

char *conv_utf8(const char *str)
{
	if (utf8_conv == (iconv_t)-1 && (utf8_conv = iconv_open("UTF-8", input_encoding)) == (iconv_t)-1)
		ALICE_ERROR("iconv_open: %s", strerror(errno));
	return convert_text(utf8_conv, str);
}

char *conv_output_utf8(const char *str)
{
	if (output_utf8_conv == (iconv_t)-1 && (output_utf8_conv = iconv_open("UTF-8", output_encoding)) == (iconv_t)-1)
		ALICE_ERROR("iconv_open: %s", strerror(errno));
	return convert_text(output_utf8_conv, str);
}
