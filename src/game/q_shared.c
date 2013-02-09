/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

// os x game bundles have no standard library links, and the defines are not always defined!

#ifdef MACOS_X
int qmax(int x, int y) {
	return (((x) > (y)) ? (x) : (y));
}

int qmin(int x, int y) {
	return (((x) < (y)) ? (x) : (y));
}
#endif

float Com_Clamp(float min, float max, float value) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

void COM_FixPath(char *pathname) {
	while (*pathname) {
		if (*pathname == '\\') {
			*pathname = '/';
		}
		pathname++;
	}
}

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath(char *pathname) {
	char *last;

	last = pathname;
	while (*pathname) {
		if (*pathname == '/') {
			last = pathname + 1;
		}
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(const char *in, char *out) {
	while (*in && *in != '.') {
		*out++ = *in++;
	}
	*out = 0;
}

/*
============
COM_StripExtension2
a safer version
============
*/
void COM_StripExtension2(const char *in, char *out, int destsize) {
	int len = 0;

	while (len < destsize - 1 && *in && *in != '.') {
		*out++ = *in++;
		len++;
	}
	*out = 0;
}

void COM_StripFilename(char *in, char *out) {
	char *end;

	Q_strncpyz(out, in, strlen(in) + 1);
	end  = COM_SkipPath(out);
	*end = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char *path, int maxSize, const char *extension) {
	char oldPath[MAX_QPATH];
	char *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path) {
		if (*src == '.') {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz(oldPath, path, sizeof (oldPath));
	Com_sprintf(path, maxSize, "%s%s", oldPath, extension);
}

//============================================================================
/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
qboolean COM_BitCheck(const int array[], int bitNum) {
	int i;

	i = 0;
	while (bitNum > 31) {
		i++;
		bitNum -= 32;
	}

	return ((array[i] & (1 << bitNum)) != 0);        // (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet(int array[], int bitNum) {
	int i;

	i = 0;
	while (bitNum > 31) {
		i++;
		bitNum -= 32;
	}

	array[i] |= (1 << bitNum);
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear(int array[], int bitNum) {
	int i;

	i = 0;
	while (bitNum > 31) {
		i++;
		bitNum -= 32;
	}

	array[i] &= ~(1 << bitNum);
}
//============================================================================

/*
============================================================================

PARSING

============================================================================
*/

static char com_token[MAX_TOKEN_CHARS];
static char com_parsename[MAX_TOKEN_CHARS];
static int  com_lines;

static int  backup_lines;
static char *backup_text;

void COM_BeginParseSession(const char *name) {
	com_lines = 0;
	Com_sprintf(com_parsename, sizeof (com_parsename), "%s", name);
}

void COM_BackupParseSession(char **data_p) {
	backup_lines = com_lines;
	backup_text  = *data_p;
}

void COM_RestoreParseSession(char **data_p) {
	com_lines = backup_lines;
	*data_p   = backup_text;
}

void COM_SetCurrentParseLine(int line) {
	com_lines = line;
}

int COM_GetCurrentParseLine(void) {
	return com_lines;
}

char *COM_Parse(char **data_p) {
	return COM_ParseExt(data_p, qtrue);
}

void COM_ParseError(char *format, ...) {
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof (string), format, argptr);
	va_end(argptr);

	Com_Printf("ERROR: %s, line %d: %s\n", com_parsename, com_lines, string);
}

void COM_ParseWarning(char *format, ...) {
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof (string), format, argptr);
	va_end(argptr);

	Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, com_lines, string);
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
static char *SkipWhitespace(char *data, qboolean *hasNewLines) {
	int c;

	while ((c = *data) <= ' ') {
		if (!c) {
			return NULL;
		}
		if (c == '\n') {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress(char *data_p) {
	char     *datai, *datao;
	int      c, size;
	qboolean ws = qfalse;

	size  = 0;
	datai = datao = data_p;
	if (datai) {
		while ((c = *datai) != 0) {
			if (c == 13 || c == 10) {
				*datao = c;
				datao++;
				ws = qfalse;
				datai++;
				size++;
				// skip double slash comments
			} else if (c == '/' && datai[1] == '/') {
				while (*datai && *datai != '\n') {
					datai++;
				}
				ws = qfalse;
				// skip /* */ comments
			} else if (c == '/' && datai[1] == '*') {
				datai += 2; // Arnout: skip over '/*'
				while (*datai && (*datai != '*' || datai[1] != '/')) {
					datai++;
				}
				if (*datai) {
					datai += 2;
				}
				ws = qfalse;
			} else {
				if (ws) {
					*datao = ' ';
					datao++;
				}
				*datao = c;
				datao++;
				datai++;
				ws = qfalse;
				size++;
			}
		}
	}
	*datao = 0;
	return size;
}

char *COM_ParseExt(char **data_p, qboolean allowLineBreaks) {
	int      c           = 0, len;
	qboolean hasNewLines = qfalse;
	char     *data;

	data         = *data_p;
	len          = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data) {
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	COM_BackupParseSession(data_p);

	while (1) {
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data) {
			*data_p = NULL;
			return com_token;
		}
		if (hasNewLines && !allowLineBreaks) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/') {
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*') {
			data += 2;
			while (*data && (*data != '*' || data[1] != '/')) {
				data++;
			}
			if (*data) {
				data += 2;
			}
		} else {
			break;
		}
	}

	// handle quoted strings
	if (c == '\"') {
		data++;
		while (1) {
			c = *data++;
			if (c == '\\' && *(data) == '\"') {
				// Arnout: string-in-string
				if (len < MAX_TOKEN_CHARS) {
					com_token[len] = '\"';
					len++;
				}
				data++;

				while (1) {
					c = *data++;

					if (!c) {
						com_token[len] = 0;
						*data_p        = ( char * ) data;
						break;
					}
					if ((c == '\\' && *(data) == '\"')) {
						if (len < MAX_TOKEN_CHARS) {
							com_token[len] = '\"';
							len++;
						}
						data++;
						c = *data++;
						break;
					}
					if (len < MAX_TOKEN_CHARS) {
						com_token[len] = c;
						len++;
					}
				}
			}
			if (c == '\"' || !c) {
				com_token[len] = 0;
				*data_p        = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == '\n') {
			com_lines++;
		}
	} while (c > 32);

	if (len == MAX_TOKEN_CHARS) {
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}




/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken(char **buf_p, char *match) {
	char *token;

	token = COM_Parse(buf_p);
	if (strcmp(token, match) != 0) {
		Com_Error(ERR_DROP, "MatchToken: %s != %s", token, match);
	}
}

/*
=================
SkipBracedSection_Depth

=================
*/
void SkipBracedSection_Depth(char **program, int depth) {
	char *token;

	do {
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0) {
			if (token[0] == '{') {
				depth++;
			} else if (token[0] == '}') {
				depth--;
			}
		}
	} while (depth && *program);
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection(char **program) {
	char *token;
	int  depth;

	depth = 0;
	do {
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0) {
			if (token[0] == '{') {
				depth++;
			} else if (token[0] == '}') {
				depth--;
			}
		}
	} while (depth && *program);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine(char **data) {
	char *p;
	int  c;

	p = *data;
	while ((c = *p++) != 0) {
		if (c == '\n') {
			com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix(char **buf_p, int x, float *m) {
	char *token;
	int  i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i]  = atof(token);
	}

	COM_MatchToken(buf_p, ")");
}

void Parse2DMatrix(char **buf_p, int y, int x, float *m) {
	int i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix(buf_p, x, m + i * x);
	}

	COM_MatchToken(buf_p, ")");
}

void Parse3DMatrix(char **buf_p, int z, int y, int x, float *m) {
	int i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix(buf_p, y, x, m + i * x * y);
	}

	COM_MatchToken(buf_p, ")");
}


/*
===============
Com_ParseInfos
===============
*/
int Com_ParseInfos(char *buf, int max, char infos[][MAX_INFO_STRING]) {
	const char *token;
	int        count;
	char       key[MAX_TOKEN_CHARS];

	count = 0;

	while (1) {
		token = COM_Parse(&buf);
		if (!token[0]) {
			break;
		}
		if (strcmp(token, "{") != 0) {
			Com_Printf("Missing { in info file\n");
			break;
		}

		if (count == max) {
			Com_Printf("Max infos exceeded\n");
			break;
		}

		infos[count][0] = 0;
		while (1) {
			token = COM_Parse(&buf);
			if (!token[0]) {
				Com_Printf("Unexpected end of info file\n");
				break;
			}
			if (!strcmp(token, "}")) {
				break;
			}
			Q_strncpyz(key, token, sizeof (key));

			token = COM_ParseExt(&buf, qfalse);
			if (!token[0]) {
				token = "<NULL>";
			}
			Info_SetValueForKey(infos[count], key, token);
		}
		count++;
	}

	return count;
}

/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint(int c) {
	if (c >= 0x20 && c <= 0x7E) {
		return (1);
	}
	return (0);
}

int Q_islower(int c) {
	if (c >= 'a' && c <= 'z') {
		return (1);
	}
	return (0);
}

int Q_isupper(int c) {
	if (c >= 'A' && c <= 'Z') {
		return (1);
	}
	return (0);
}

int Q_isalpha(int c) {
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		return (1);
	}
	return (0);
}

int Q_isnumeric(int c) {
	if (c >= '0' && c <= '9') {
		return (1);
	}
	return (0);
}

int Q_isalphanumeric(int c) {
	if (Q_isalpha(c) ||
	    Q_isnumeric(c)) {
		return(1);
	}
	return (0);
}

int Q_isforfilename(int c) {
	if ((Q_isalphanumeric(c) || c == '_') && c != ' ') {       // space not allowed in filename
		return(1);
	}
	return (0);
}

char *Q_strrchr(const char *string, int c) {
	char cc = c;
	char *s;
	char *sp = (char *)0;

	s = (char *)string;

	while (*s) {
		if (*s == cc) {
			sp = s;
		}
		s++;
	}
	if (cc == 0) {
		sp = s;
	}

	return sp;
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz(char *dest, const char *src, int destsize) {
	if (!src) {
		Com_Error(ERR_FATAL, "Q_strncpyz: NULL src");
	}
	if (destsize < 1) {
		Com_Error(ERR_FATAL, "Q_strncpyz: destsize < 1");
	}

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

int Q_stricmpn(const char *s1, const char *s2, int n) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;       // strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;       // strings are equal
}

int Q_strncmp(const char *s1, const char *s2, int n) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;       // strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;       // strings are equal
}

int Q_stricmp(const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn(s1, s2, 99999) : -1;
}

char *Q_strlwr(char *s1) {
	char *s;

	for (s = s1; *s; ++s) {
		if (('A' <= *s) && (*s <= 'Z')) {
			*s -= 'A' - 'a';
		}
	}

	return s1;
}

char *Q_strupr(char *s1) {
	char *cp;

	for (cp = s1 ; *cp ; ++cp) {
		if (('a' <= *cp) && (*cp <= 'z')) {
			*cp += 'A' - 'a';
		}
	}

	return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat(char *dest, int size, const char *src) {
	int l1;

	l1 = strlen(dest);
	if (l1 >= size) {
		Com_Error(ERR_FATAL, "Q_strcat: already overflowed");
	}
	Q_strncpyz(dest + l1, src, size - l1);
}


int Q_PrintStrlen(const char *string) {
	int        len;
	const char *p;

	if (!string) {
		return 0;
	}

	len = 0;
	p   = string;
	while (*p) {
		if (Q_IsColorString(p)) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr(char *string) {
	char *d;
	char *s;
	int  c;

	s = string;
	d = string;
	while ((c = *s) != 0) {
		if (Q_IsColorString(s)) {
			s++;
		} else if (c >= 0x20 && c <= 0x7E) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

// strips whitespaces and bad characters
qboolean Q_isBadDirChar(char c) {
	char badchars[] = { ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\'', '\"', '\0' };
	int  i;

	for (i = 0; badchars[i] != '\0'; i++) {
		if (c == badchars[i]) {
			return qtrue;
		}
	}

	return qfalse;
}

char *Q_CleanDirName(char *dirname) {
	char *d;
	char *s;

	s = dirname;
	d = dirname;

	// clear trailing .'s
	while (*s == '.') {
		s++;
	}

	while (*s != '\0') {
		if (!Q_isBadDirChar(*s)) {
			*d++ = *s;
		}
		s++;
	}
	*d = '\0';

	return dirname;
}

void QDECL Com_Error(int level, const char *error, ...) {
	va_list argptr;
	char    text[1024];

	// Nico, silent GCC
	level = level;

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof (text), error, argptr);
	va_end(argptr);

# ifdef CGAMEDLL
	CG_Error("%s", text);
# elif defined GAMEDLL
	G_Error("%s", text);
# else
	trap_Error(va("%s", text));
#endif
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof (text), msg, argptr);
	va_end(argptr);

# ifdef CGAMEDLL
	CG_Printf("%s", text);
# elif defined GAMEDLL
	G_Printf("%s", text);
# else
	trap_Print(va("%s", text));
#endif
}

void QDECL Com_sprintf(char *dest, int size, const char *fmt, ...) {
	int     ret;
	va_list argptr;

	va_start(argptr, fmt);
	ret = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	// Nico, as I changed Q_vsnprintf, i need to change the test here
	// if ( ret == -1 ) {
	//	Com_Printf( "Com_sprintf: overflow of %i bytes buffer\n", size );
	if (ret >= size) {
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, ret + 1);
	}
}

// Nico, secured version from etlegacy
char *QDECL va(char *format, ...) {
	va_list     argptr;
	static char string[2][32000]; // in case va is called by nested functions
	static int  index = 0;
	char        *buf;

	buf = string[index & 1];
	index++;

	va_start(argptr, format);
	Q_vsnprintf(buf, sizeof (*string), format, argptr);
	va_end(argptr);

	return buf;
}

/*
=============
TempVector

(SA) this is straight out of g_utils.c around line 210

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float *tv(float x, float y, float z) {
	static int    index;
	static vec3_t vecs[8];
	float         *v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v     = vecs[index];
	index = (index + 1) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey(const char *s, const char *key) {
	char        pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE];   // use two buffers so compares
	                                        // work without stomping on each other
	static int valueindex = 0;
	char       *o;

	if (!s || !key) {
		return "";
	}

	if (strlen(s) >= BIG_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring [%s] [%s]", s, key);
	}

	valueindex ^= 1;
	if (*s == '\\') {
		s++;
	}
	while (1) {
		o = pkey;
		while (*s != '\\') {
			if (!*s) {
				return "";
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s) {
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey)) {
			return value[valueindex];
		}

		if (!*s) {
			break;
		}
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair(const char **head, char *key, char *value) {
	char       *o;
	const char *s;

	s = *head;

	if (*s == '\\') {
		s++;
	}
	key[0]   = 0;
	value[0] = 0;

	o = key;
	while (*s != '\\') {
		if (!*s) {
			*o    = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while (*s != '\\' && *s) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey(char *s, const char *key) {
	char *start;
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char *o;

	if (strlen(s) >= MAX_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring [%s] [%s]", s, key);
	}

	if (strchr(key, '\\')) {
		return;
	}

	while (1) {
		start = s;
		if (*s == '\\') {
			s++;
		}
		o = pkey;
		while (*s != '\\') {
			if (!*s) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s) {
			if (!*s) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey)) {
			// rain - arguments to strcpy must not overlap
			//strcpy (start, s);	// remove this part
			memmove(start, s, strlen(s) + 1);     // remove this part
			return;
		}

		if (!*s) {
			return;
		}
	}

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big(char *s, const char *key) {
	char *start;
	char pkey[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];
	char *o;

	if (strlen(s) >= BIG_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_RemoveKey_Big: oversize infostring [%s] [%s]", s, key);
	}

	if (strchr(key, '\\')) {
		return;
	}

	while (1) {
		start = s;
		if (*s == '\\') {
			s++;
		}
		o = pkey;
		while (*s != '\\') {
			if (!*s) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s) {
			if (!*s) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey)) {
			strcpy(start, s);    // remove this part
			return;
		}

		if (!*s) {
			return;
		}
	}

}




/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate(const char *s) {
	if (strchr(s, '\"')) {
		return qfalse;
	}
	if (strchr(s, ';')) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey(char *s, const char *key, const char *value) {
	char newi[MAX_INFO_STRING];

	if (strlen(s) >= MAX_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value);
	}

	if (strchr(key, '\\') || strchr(value, '\\')) {
		Com_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';')) {
		Com_Printf("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"')) {
		Com_Printf("Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey(s, key);
	if (!value || !strlen(value)) {
		return;
	}

	Com_sprintf(newi, sizeof (newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > MAX_INFO_STRING) {
		Com_Printf("Info string length exceeded\n");
		return;
	}

	strcat(s, newi);
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big(char *s, const char *key, const char *value) {
	char newi[BIG_INFO_STRING];

	if (strlen(s) >= BIG_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value);
	}

	if (strchr(key, '\\') || strchr(value, '\\')) {
		Com_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';')) {
		Com_Printf("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"')) {
		Com_Printf("Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey_Big(s, key);
	if (!value || !strlen(value)) {
		return;
	}

	Com_sprintf(newi, sizeof (newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > BIG_INFO_STRING) {
		Com_Printf("BIG Info string length exceeded\n");
		return;
	}

	strcat(s, newi);
}
//====================================================================
