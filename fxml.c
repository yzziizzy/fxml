 

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

FXMLTag* fxmlTagDestroy(FXMLTag* t) {
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
	
	s++; // skip bracket
	
	// TODO: skip shitespace?
	
	end = strpbrk(s, " \n\r\t\v");
	
	if(!end) {
		fprintf(stderr, "FXML: could not find end of tag name.\n");
		return 2;
	}
	
	t->name_len = end - s - 1; // TODO check for off-by-one
	t->name = strndup(s, t->name_len);
	
	printf("tag name: '%s'\n", t->name);
	
	// might as well
	fxmlTagFindStartOfContent(t);
	
	return 0;
}

void fxmlTagFindStartOfContent(FXMLTag* t) {
	char* s;
	
	// find end of opening tag
	s = strchr(t->start, '>'); // BUG bounds checking
	if(!s) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlFindEndOfContent.\n");
		return;
	}
	
	if(*(s-1) == '/') {
		t->self_terminating = 1;
		t->fully_parsed = 1;
	}char* readFile(char* path, int* srcLen) {
	
	int fsize;
	char* contents;
	FILE* f;
	
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open file \"%s\"\n", path);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	rewind(f);
	
	contents = (char*)malloc(fsize + 2);
	
	fread(contents+1, sizeof(char), fsize, f);
	contents[0] = '\n';
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize + 1;
	
	return contents;
}

	
	t->content_start = s + 1;
}

FXMLTag* fxmlTagFindEndOfContent(FXMLTag* t) {
	char* s = t->content_start;
	
	if(t->fully_parsed) return;
	
	fxmlTagParseName(t);
	
	if(t->self_terminating) return;
	
	// skip all the children, recursively
	while(*s) { // BUG need better bounds checking
		int ctype;
		dbg_printf("find eoc loop: %.5s", t->name)
		
		fxmlFindNextTagStart(&s);
		
		// found it
		if(fxmlIsCloseTagFor(s, t->name)) {
			t->content_end = s;
			
			fxmlSkipTagDecl(&s);
			t->end = s;
			
			t->fully_parsed = 1;
			
			return;
		}
		
		// keep going; more children to skip
	}
	
	fprintf(stderr, "FXML: unexpected end of input in fxmlTagFindEndOfContent.\n");
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
	if(0 != strcmp(s, "<![CDATA[")) {
		fprintf(stderr, "FXML: cannot parse non CDATA tag as CDATA.\n");
		return;
	}
	
	s += strlen("<!CDATA[");
	
	e = strstr(s, "]]>");
	if(!e) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlDecodeCDATA.\n");
		return;
	}
	
	strncpy(*out, s, e - s);
	
	*out += e - s;
	*in += e - s + strlen("]]>");
}


// return a concatenated decoded dup'ed copy of the tag's top-level text content. empty string if none.
char* fxmlGetTextContents(FXMLTag* t, size_t* len) {
	
	char* in = t->content_start;
	char* buf, *out;
	size_t buflen;
	
	// allocate too much, shink later
	buflen = t->content_end - t->content_start;
	buf = out = malloc(buflen + 1);
	
	while(in < t->content_end) {
		int ctype;
		
		if(*in == '&') {
			fxmlDecodeEntity(&in, &out);
		}
		else if(*in == '<') {
			ctype = fxmlProbeTagType(in);
			if(ctype == FXML_TAG_CDATA) {
				fxmlDecodeCDATA(&in, &out);
			} 
			else {
				fxmlSkipTag(&in);
			}
		}
		else {
			*out++ = *in++;
		}
	}
	
	*out = 0;
	
}

// expects the char after the attr name
static char* extractAttrValue(char* start) {
	char* s = start;
	char* e, *out;
	char quote;
	
	//look for the next useful token
	skipWhitespace(&s);
	if(*s != '=') {
		fprintf(stderr, "FXML: malformed attribute lacks equals sign.\n");
		return strdup(""); // good enough
	}
	
	s++;
	skipWhitespace(&s);
	
	if(*s != '"' || *s != '\'') {
		fprintf(stderr, "FXML: malformed attribute lacks quotes.\n");
		return strdup(""); // good enough
	}
	
	quote = *s;
	s++;
	
	e = strchr(s, quote);
	
	out = malloc(e - s + 1);
	if(!out) {
		fprintf(stderr, "FXML: OOM in extractAttrValue.\n");
		exit(1);
	}
	
	
	// decode the attr
	while(s < e) {
		if(*s == '&') {
			fxmlDecodeEntity(&s, &out);
		}
		else {
			*out++ = *s++;
		}
	}
	
	*out = 0;
	
	
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
	
	if(*s != '"' || *s != '\'') {
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
		e = strpbrk(s, " \n\r\t\v=>");
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


// fetches first child tag of any name
FXMLTag* fxmlTagGetFirstChild(FXMLTag* t) {
	char* s;
	
	fxmlTagParseName(t);

	if(t->self_terminating) return;
	
	s = t->content_start;
	
	fxmlFindNextNormalTagStart(&s);
	if(fxmlIsCloseTagFor(s, t->name)) {
		return NULL;
	}
	
	return fxmlTagCreate(s, t);
}

//fetches the first child with the specified name, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindFirstChild(FXMLTag* t, char* name) {
	FXMLTag* c, *o;
	
	c = fxmlTagGetFirstChild(t);
	
	while(c && 0 != strcmp(c->name, name)) {
		dbg_printf("name: %s \n", c->name);
		
		o = c;
		c = fxmlTagNextSibling(c);
		fxmlTagDestroy(o);
		free(o);
	}
	dbg_printf("ended\n");
	return c;
}

// fetches the next sibling, ignoring all contents of this tag and any text between
FXMLTag* fxmlTagNextSibling(FXMLTag* t) {
	char* s;
	dbg_printf("before eoc");
	fxmlTagFindEndOfContent(t);
	dbg_printf("eoc")
	s = t->end;
	
	if(!t->parent) {
		fprintf(stderr, "FXML: cannot get siblings for root element.\n");
		return NULL;
	}
	
	fxmlFindNextNormalTagStart(&s);
	if(fxmlIsCloseTagFor(s, t->parent->name)) {
		return NULL;
	}
	
	return fxmlTagCreate(s, t);
	
}


//fetches the first child with the specified name, or null if it doesn't exist.
// may be expensive as it walks the xml tree
FXMLTag* fxmlTagFindNextSibling(FXMLTag* t, char* name) {
	FXMLTag* c, *o;
	
	c = t;
	
	while(c && 0 != strcmp(c->name, name)) {
		dbg_printf("name: %s \n", c->name);
		
		o = c;
		c = fxmlTagNextSibling(c);
		fxmlTagDestroy(o);
		free(o);
	}
	dbg_printf("ended\n");
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
		fprintf(stderr, "FXML: expected < in fxmlProbeTagType.\n");
		return FXML_TAG_UNKNOWN;
	}
	
	switch(*(start+1)) {
		case 0:
			fprintf(stderr, "FXML: unexpected end of input in fxmlProbeTagType.\n");
			return FXML_TAG_UNKNOWN;
		
		case '!':
			if(*(start+2) == '-' && *(start+3) == '-') //BUG might read past end of input. eh, you get what you deserve feeding in bad files.
				return FXML_TAG_COMMENT;
			else if(strcmp(start+1, "![CDATA[") == 0)
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
	
	// all other tags close semi-normally
	s = strchr(s, '>');
	if(!s) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlSkipTagDecl.\n");
		*start = NULL;
		return 0;
	}
	
	*start = s + 1;
	
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


// nasty recursive tag skipping fn. don't blow out the stack.
void fxmlSkipTag(char** start) {
	char* name;
	char* s = *start;
	int type;
	char* endstr;
	
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
	
	// skip the opening tag
	fxmlSkipTagDecl(&s);
	
	// skip all the children, recursively
	while(*s) { // BUG need better bounds checking
		int ctype;
		
		
		fxmlFindNextTagStart(&s);
		
		// found it
		if(fxmlIsCloseTagFor(s, name)) {
			fxmlSkipTagDecl(&s);
			*start = s;
			return;
		}
		
		// keep going; more children to skip
	}
	
	fprintf(stderr, "FXML: unexpected end of input in fxmlSkipTag.\n");
}

// checks if a given tag is a valid closing tag for a named normal xml tag
int fxmlIsCloseTagFor(char* s, char* name) {
	if(*s == 0) {
		fprintf(stderr, "FXML: unexpected end of input in fxmlIsCloseTagFor.\n");
		return 0;
	}
	
	// not a closing tag at all
	if(*(s+1) != '/') return 0;
	dbg_printf("%.5s, %.5s", s+2,name)
	if(strncmp(s+2, name, strlen(name)) == 0) return 1;
	
	return 0;
}


void fxmlFindNextTagStart(char** start) {
	char* s;
	
	s = strchr(*start, '<');
	if(s == NULL) {
		fprintf(stderr, "FXML: unexpected end of input looking for next tag start.\n");
		s += strlen(s);
	}
	
	*start = s;
}

void fxmlFindNextNormalTagStart(char** start) {
	char* s = *start;
	
	while(*s) { dbg_printf("looping");
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
	
	skipWhitespace(&s);
	
	while(*s) {
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
FXMLTag* fxmlLoadString(char* source, size_t len) {
	
	FXMLTag* t;
	char* start;
	
	// ignore any leading whitespace, declarations, doctypes, or any other bullshit.
	start = fxmlFindRoot(source);
	
	t = fxmlTagCreate(start, NULL);
	
	return t;
}



char* readFile(char* path, size_t* srcLen) {
	
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
	
	contents = (char*)malloc(fsize + 2);
	
	fread(contents+1, sizeof(char), fsize, f);
	contents[0] = '\n';
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize + 1;
	
	return contents;
}



FXMLTag* fxmlLoadFile(char* path) {
	
	char* source;
	size_t len;
	
	source = readFile(path, &len);
	
	return fxmlLoadString(source, len);
}











