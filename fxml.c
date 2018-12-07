 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "fxml.h" 


static void skipWhitespace(char** s) {
	char* e;
	e = *s + strspn(*s, " \n\t\r\v");
	
	if(e) {
		*s = e;
		return;
	}
	
	// no whitespace until the end of the string
	*s = *s + strlen(*s);
}




void fxmlTagInit(FXMLTag* t, char* start, FXMLTag* parent) {
	
	t->start = start;
	
	if(parent) {
		t->parent = parent;
		t->depth = parent->depth + 1;
	}
	
	// put this here for now
	fxmlTagParseName(t);
}

FXMLTag* fxmlTagCreate(char* start, FXMLTag* parent) {
	
	FXMLTag* t;
	
	t = calloc(1, sizeof(*t));
	if(!t) {
		fprintf(stderr, "OOM allocating FXMLTag.\n");
		exit(2);
	}
	
	fxmlTagInit(t, start, parent);
	
	return t;
}

void fxmlTagDestroy(FXMLTag* t) {
	if(!t) return;
	
	if(t->name) free(t->name);
}

int fxmlTagParseName(FXMLTag* t) {
	char* end;
	char* s = t->start;
	
	// already parsed
	if(t->name) return 0;
	
	if(s[0] != '<') {
		fprintf(stderr, "FXML: expected opening bracket in fxmlTagParseName.\n");
		return 1;
	}
	
	//printf("h> %.5s\n", s);
	s++; // skip bracket
	//printf("i> %.5s\n", s);
	
	// TODO: skip shitespace?
	
	end = strpbrk(s, " \n\r\t\v>/");
	
	if(!end) {
		fprintf(stderr, "FXML: could not find end of tag name.\n");
		return 2;
	}
	
	t->name_len = end - s; // TODO check for off-by-one
	t->name = strndup(s, t->name_len);
	
//	dbg_printf("tag name: '%s'\n", t->name);
	
	// might as well
	fxmlTagFindStartOfContent(t);
	
	return 0;
}

void fxmlTagFindStartOfContent(FXMLTag* t) {
	char* s;
	
	// find end of opening tag
	s = strchr(t->start, '>'); // BUG bounds checking
	if(!s) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlFindStartOfContent.\n");
		return;
	}
	
	if(*(s-1) == '/') {
		t->self_terminating = 1;
		//t->fully_parsed = 1;
	}

	t->content_start = s + 1;
}

void fxmlTagFindEndOfContent(FXMLTag* t) {
	char* s = t->content_start;
//	printf("\na%d, %p> '%.27s'\n", t->fully_parsed, t->end, t->end);
	
	if(t->fully_parsed) return;
//	printf("\nb> '%.6s'\n", s);
	
	fxmlTagParseName(t);
 printf("\nc> '%.*s'\n", t->name_len, t->name);
		
	if(t->self_terminating) {
		t->content_start = NULL;
		t->content_end = NULL;
		
		s = t->start;
	//	printf("d> %.5s\n", s);
	}
	else {
		fxmlFindEndOfContent(&s, t->name, t->name_len);
		t->content_end = s;
	//	printf("e> %.5s\n", s);
	}
 printf("\nd> '%.*s'\n", t->name_len, t->name);	
	fxmlSkipTagDecl(&s);
	t->end = s;
//	printf("f> %.5s\n", s);
	
// 		namelen = fxmlGetTagNameLen(s + 1);
// 	name = s + 1;
// 	printf("\na> '%.*s'(%d)\n", namelen, tname, namelen);
	
	t->fully_parsed = 1;
}

// decodes and entity, advancing both input and output pointers.
void fxmlDecodeEntity(char** input, char** output) {
	char* s;
	
	// TODO real algorithm
	s = strchr(*input, ';');
	
	*input = s + 1;
	
	**output = '?';
	(*output)++;
}

// leaves *in on the next char after the closing sequence
void fxmlDecodeCDATA(char** in, char** out) {
	char* s = *in;
	char* e;
	
	// just in case
	if(0 != strncmp(s, "<![CDATA[", strlen("<![CDATA["))) {
		fprintf(stderr, "FXML: cannot parse non CDATA tag as CDATA.\n");
		return;
	}
	
	s += strlen("<!CDATA[") + 1;
	e = strstr(s, "]]>");
	
	if(!e) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlDecodeCDATA.\n");
		return;
	}
	
	strncpy(*out, s, e - s);
	
	*out += e - s;
	*in = e + strlen("]]>");
}


// return a concatenated decoded dup'ed copy of the tag's top-level text content. empty string if none.
char* fxmlGetTextContents(FXMLTag* t, size_t* len) {
	
	char* in = t->content_start;
	char* out, *o;
	size_t buflen;
	
	fxmlTagFindEndOfContent(t);
	
	// allocate too much, shink later
	buflen = t->content_end - t->content_start;
	out = o = malloc(buflen + 1);
	
	while(in < t->content_end) {
		int ctype;
		
		if(*in == '&') {
			fxmlDecodeEntity(&in, &o);
		}
		else if(*in == '<') {
			ctype = fxmlProbeTagType(in);
			if(ctype == FXML_TAG_CDATA) {
				fxmlDecodeCDATA(&in, &o);
			} 
			else {
				fxmlSkipTag(&in);
			}
		}
		else {
			*o++ = *in++;
		}
	}
	
	*o = 0;
	
	if(len) *len = o - out;
	
	return out;
}

// expects the char after the attr name
static char* extractAttrValue(char* start) {
	char* s = start;
	char* e, *out, *o;
	char quote;
	
	//look for the next useful token
	skipWhitespace(&s);
	if(*s != '=') {
		fprintf(stderr, "FXML: malformed attribute lacks equals sign.\n");
		return strdup(""); // good enough
	}
	s++;
	skipWhitespace(&s);
	
	if(*s != '"' && *s != '\'') {
		fprintf(stderr, "FXML: malformed attribute lacks quotes.\n");
		return strdup(""); // good enough
	}
	
	quote = *s;
	s++;
	
	e = strchr(s, quote);

	out = o = malloc(e - s + 1);
	if(!out) {
		fprintf(stderr, "FXML: OOM in extractAttrValue.\n");
		exit(1);
	}
	
	
	// decode the attr
	while(s < e) {
		if(*s == '&') {
			fxmlDecodeEntity(&s, &o);
		}
		else {
			*o++ = *s++;
		}
	}
	*o = 0;
	
	
	return out;
}

static char* findNextAttr(char* start) {
	char* s = start;
	char* e;
	char quote;
	
	//look for the next useful token
	skipWhitespace(&s);

	if(*s != '=') {
		fprintf(stderr, "FXML: malformed attribute lacks equals sign.\n");
		return s; // good enough
	}

	s++; 
	skipWhitespace(&s);

	if(*s != '"' && *s != '\'') {
		fprintf(stderr, "FXML: malformed attribute lacks quotes.\n");
		return s; // good enough
	}
	
	quote = *s;
	s++;
	
	e = strchr(s, quote);
	
	return e + 1;
}

// given a tag, return a dup'ed decoded value for an attr, or null if the attr doesn't exist
char* fxmlGetAttr(FXMLTag* t, char* name) {
	
	char* e;
	char* s = t->start + t->name_len + 1;
	
	while(*s) { 
		skipWhitespace(&s);
		
		// go past the name
		e = strpbrk(s, " \n\r\t\v=>/");
		if(*e == '>') {
			// no more attributes
			return NULL;
		}
		
		if(0 == strncmp(name, s, e - s)) {
			// this attr matches
			return extractAttrValue(e);
		}
		
		s = findNextAttr(e);
	}
	
	fprintf(stderr, "FXML: unexpected end of input in fxmlGetAttr.\n");
}

// given a tag, return a dup'ed decoded value for an attr, sliced, or null if the attr doesn't exist
char* fxmlGetAttrSlice(FXMLTag* t, char* name, int start, int end) {
	
	char* e;
	char* s = t->start + t->name_len + 1;
	
	while(*s) { 
		skipWhitespace(&s);
		
		// go past the name
		e = strpbrk(s, " \n\r\t\v=>/");
		if(*e == '>') {
			// no more attributes
			return NULL;
		}
		
		if(0 == strncmp(name, s, e - s)) {
			// this attr matches
			
			// not efficient at all. meh.
			char* v = extractAttrValue(e);
			
			int len = strlen(v);
			char* out = malloc(len + 1);
			
			int _end = end == 0 ? len : (end < 0 ? len + end : (start > end ? start : end));
			
			int cpyn = end - start; 
			
			strncpy(out, v, cpyn);
			out[cpyn] = 0;
			
			free(v);
			
			return out;
		}
		
		s = findNextAttr(e);
	}
	
	fprintf(stderr, "FXML: unexpected end of input in fxmlGetAttr.\n");
}

// returns 0 if not exists
int64_t fxmlGetAttrInt(FXMLTag* t, char* name) {
	char* c;
	int64_t i;
	
	c = fxmlGetAttr(t, name);
	if(!c) return 0;
	
	i = strtoll(c, NULL, 10);
	free(c);
	
	return i;
}

// returns 0.0 if not exists
double fxmlGetAttrDouble(FXMLTag* t, char* name) {
	char* c;
	double d;
	
	c = fxmlGetAttr(t, name);
	if(!c) return 0.0;
	
	d = strtod(c, NULL);
	free(c);
	
	return d;
}


// fetches first child tag of any name
FXMLTag* fxmlTagGetFirstChild(FXMLTag* t) {
	char* s;
	
	fxmlTagParseName(t);

	if(t->self_terminating) return;
	
	s = t->content_start;
	
	fxmlFindNextNormalTagStart(&s);
	if(fxmlIsCloseTagFor(s, t->name, -1)) {
		return NULL;
	}
	
	return fxmlTagCreate(s, t);
}

//fetches the first child with the specified name, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindFirstChild(FXMLTag* t, char* name) {
	FXMLTag* c;
	
	c = fxmlTagGetFirstChild(t);
	
	while(c && 0 != strncmp(c->name, name, c->name_len)) {
		c = fxmlTagNextSibling(c, 1);
	}

	return c;
}

// fetches the next sibling, ignoring all contents of this tag and any text between
// returns null when there are no more siblings
FXMLTag* fxmlTagNextSibling(FXMLTag* t, int freePrev) {
	char* s;
	FXMLTag* n;

	fxmlTagFindEndOfContent(t);
	s = t->end;
	
	if(!t->parent) {
		fprintf(stderr, "FXML: cannot get siblings for root element.\n");
		return NULL;
	}
	
	fxmlFindNextNormalTagStart(&s);
	if(fxmlIsCloseTag(s)) { printf("is close tag\n");
		if(freePrev) {
			fxmlTagDestroy(t);
			free(t);
		}
		return NULL;
	}
	printf("not close tag\n");
	n = fxmlTagCreate(s, t);
	if(freePrev) {
		fxmlTagDestroy(t);
		free(t);
	}
	return n;
}


//fetches the next sibling with the specified name, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindNextSibling(FXMLTag* t, char* name, int freePrev) {
	FXMLTag* c;
	
	c = fxmlTagNextSibling(t, 0); // TODO: fix possible leak on last item
//	printf("huh %p %.*s\n", c, c->name_len, c->name);
	while(c && 0 != strncmp(c->name, name, c->name_len)) {
		c = fxmlTagNextSibling(c, 1);
		printf("u> %p\n", c);
	}

	if(t && freePrev) {
		fxmlTagDestroy(t);
		free(t);
	}
	return c;
}


static int strInArray(char* test, char** array, int len) {
	char** ar = array;
	
	while(*ar) {
		if(0 == strncmp(test, *ar, len)) return 0;
	}
	
	return 1;
}


//fetches the first child with any of the specified names, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindFirstChildArray(FXMLTag* t, char** names) {
	FXMLTag* c;
	
	c = fxmlTagGetFirstChild(t);
	
	while(c && 0 != strInArray(c->name, names, c->name_len)) {
		c = fxmlTagNextSibling(c, 1);
	}

	return c;
}

//fetches the next sibling with any of the specified names, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindNextSiblingArray(FXMLTag* t, char** names) {
	FXMLTag* c;
	
	c = t;
	
	while(c && 0 != strInArray(c->name, names, c->name_len)) {
		c = fxmlTagNextSibling(c, 1);
	}

	return c;
}



/////////////////////////
///
/// raw string handling fns, mostly for creating and parsing FXMLTag structs
///
/////////////////////////


// determine what kind of tag this is
int fxmlProbeTagType(char* start) {
	if(*start != '<') {
		fprintf(stderr, "FXML: expected < in fxmlProbeTagType, got '%.5s'\n", start);
		return FXML_TAG_UNKNOWN;
	}
	
	switch(*(start+1)) {
		case 0:
			fprintf(stderr, "FXML: unexpected end of input in fxmlProbeTagType.\n");
			return FXML_TAG_UNKNOWN;
		
		case '!':
			if(*(start+2) == '-' && *(start+3) == '-') //BUG might read past end of input. eh, you get what you deserve feeding in bad files.
				return FXML_TAG_COMMENT;
			else if(strncmp(start+1, "![CDATA[", strlen("![CDATA[")) == 0)
				return FXML_TAG_CDATA;
			else
				return FXML_TAG_BANG;
		
		case '?':
			return FXML_TAG_QUEST;
			
		default:
			return FXML_TAG_NORMAL;
	}
	
	// can't reach here
}

// skips terminated tags, or to the end of a normal open tag. 
// sets start to the next char after the >
// returns 1 if the tag is self-terminating
int fxmlSkipTagDecl(char** start) {
	char* s;
	char* endstr;
	int type;
	//printf("\ns> %.5s\n", *start);
	type = fxmlProbeTagType(*start);
	
	if(type == FXML_TAG_CDATA || type == FXML_TAG_COMMENT) {
		// BUG can skip past end of string. meh, you deserve it
		*start = s + 1 + strlen("<![CDATA[");
		return 1;
	}
	if(type == FXML_TAG_COMMENT) {
		// BUG can skip past end of string. meh, you deserve it
		*start = s + 1 + strlen("<!--");
		return 1;
	}
	
// 	//printf("\nt> %.5s\n", *start);
	// all other tags close semi-normally
	s = strchr(*start, '>');
//	printf("\nu> %.5s\n", s);
	if(!s) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlSkipTagDecl.\n");
		*start = NULL;
		return 0;
	}
	
	*start = s + 1;
//	printf("\nv> %.5s\n", *start);
	
	switch(type) {
		case FXML_TAG_NORMAL:
			return *(s-1) == '/' ? 1 : 0;
		
		case FXML_TAG_BANG:
		case FXML_TAG_QUEST:
			return 1;
			
		case FXML_TAG_CDATA:
		case FXML_TAG_COMMENT:
		default:
			fprintf(stderr, "FXML: should not have reached here: fxmlSkipTagDecl.\n");
			return 2;
	}
}

// returns the length of the tag name 
int fxmlGetTagNameLen(char* start) {
	char* e = strpbrk(start, " \n\r\t\v=>/");
	return e - start;
}


// expects a pointer to the first char after the opening tag
void fxmlFindEndOfContent(char** start, char* name, int namelen) {
	char* s = *start;
	
	// skip all the children, recursively
	while(*s) { // BUG need better bounds checking
		int ctype;
		
		
		fxmlFindNextTagStart(&s);
	//printf("\nb> %.8s, '%.*s'(%d)\n", s, namelen, name, namelen);
	
		//	dbg_printf("skip %.5s", s);
		// found the closing tag
		if(fxmlIsCloseTagFor(s, name, namelen)) {
			*start = s;
	//		printf("found end tag for '%.*s'\n", namelen, name);
			//dbg_printf("skip %.5s", s);
			return;
		}
		
		// recurse
		fxmlSkipTag(&s);
		
		//int v = *((int*)(0));
		// keep going; more children to skip
	}
	
	fprintf(stderr, "FXML: unexpected end of input in fxmlFindEndOfContent.\n");
}


// assumes input points to the opening <
// nasty recursive tag skipping fn. don't blow out the stack.
void fxmlSkipTag(char** start) {
	char* name;
	int namelen;
	
	char* s = *start;
	int type;
	char* endstr;
	
	//printf("fxmlSkipTag\n");
	type = fxmlProbeTagType(*start);
	
	// easy to skip the entire tag for these
	if(type == FXML_TAG_CDATA || type == FXML_TAG_COMMENT) {
		if(type == FXML_TAG_CDATA) endstr = "]]>";
		else endstr = "-->";
		
		s = strstr(*start, endstr);
		if(!s) {
			fprintf(stderr, "FXML: unexpected end of input in fxmlSkipTag.\n");
			*start = NULL;
			return;
		}
		
		// convenient that the end sequences are the same length
		*start = s + 4;
		return;
	}

	// hopefully these SOB's don't nest. too lazy to look it up.
	if(type == FXML_TAG_QUEST || type == FXML_TAG_BANG) {
		if(type == FXML_TAG_BANG) endstr = "]>";
		else endstr = "?>";
		
		s = strstr(*start, endstr);
		if(!s) {
			fprintf(stderr, "FXML: unexpected end of input in fxmlSkipTag.\n");
			*start = NULL;
			return;
		}
		
		// convenient that the end sequences are the same length
		*start = s + 3;
		return;
	}
	
	namelen = fxmlGetTagNameLen(s + 1);
	name = s + 1;
	//printf("\na> '%.*s'(%d)\n", namelen, name, namelen);
	
	// skip the opening tag
	int selfTerm = fxmlSkipTagDecl(&s);
	if(selfTerm) {
		*start = s;
		return;
	}
	
	fxmlFindEndOfContent(&s, name, namelen);
	
	// skip closing tag
	fxmlSkipTagDecl(&s);
	
	*start = s;
}

// expects a pointer to the opening <
// returns 1 if a given tag is a closing tag
int fxmlIsCloseTag(char* s) {
	return *(s+1) == '/';
}
	
// checks if a given tag is a valid closing tag for a named normal xml tag
int fxmlIsCloseTagFor(char* s, char* name, int namelen) {
	if(*s == 0) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlIsCloseTagFor.\n");
		return 0;
	}
	
	// not a closing tag at all
	if(*(s+1) != '/') return 0;

	int closelen = fxmlGetTagNameLen(s+2);
	//printf("closelen %d %d \n", closelen, namelen);
	if(closelen != namelen) return 0;
	
	if(strncmp(s+2, name, namelen) == 0) return 1;
	
	return 0;
}


void fxmlFindNextTagStart(char** start) {
	char* s;
	
	s = strchr(*start, '<');
	if(s == NULL) {
		fprintf(stderr, "FXML: unexpected end of input looking for next tag start.\n");
		int i = *((int*)0);
		s = start + strlen(*start);
	}
	
	*start = s;
}

void fxmlFindNextNormalTagStart(char** start) {
	char* s = *start;
	
	while(*s) {
		int type;
		
		fxmlFindNextTagStart(&s);
		type = fxmlProbeTagType(s);
		
		if(type == FXML_TAG_NORMAL || type == FXML_TAG_CLOSE) break;
		
		fxmlSkipTag(&s);
	}
	
	if(*s == 0) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlFindNextNormalTagStart.\n");
	}
	
	*start = s;
}

char* fxmlFindRoot(char* source) {
	int type;
	char* s = source;
	
	while(*s) {
		skipWhitespace(&s);
		
		type = fxmlProbeTagType(s);
		
		// found it
		if(FXML_TAG_NORMAL == type) {
			return s;
		}
		
		fxmlSkipTag(&s);
	}
	
	// no root element found
	fprintf(stderr, "FXML: no root element found.\n");
	
	return NULL;
}



// len is ignored for now. source must be null-terminated
FXMLFile* fxmlLoadString(char* source, size_t len) {
	
	FXMLFile* f;
	char* start;
	
	f = calloc(1, sizeof(*f));
	f->source = source;
	f->slen = len;
	
	// ignore any leading whitespace, declarations, doctypes, or any other bullshit.
	start = fxmlFindRoot(source);
	
	f->root = fxmlTagCreate(start, NULL);
	
	return f;
}



static char* read_file(char* path, size_t* srcLen) {
	
	int fsize;
	char* contents;
	FILE* f;
	
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "FXML: Could not open file \"%s\"\n", path);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	rewind(f);
	
	contents = (char*)malloc(fsize + 1);
	
	fread(contents, sizeof(char), fsize, f);
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize;
	
	return contents;
}



FXMLFile* fxmlLoadFile(char* path) {
	
	char* source;
	size_t len;
	
	source = read_file(path, &len);
	
	return fxmlLoadString(source, len);
}












