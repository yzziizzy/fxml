#ifndef FXML_H_INCLUDED
#define FXML_H_INCLUDED


#include <stdint.h>


//#define dbg_printf(fmt, ...) 
#define dbg_printf(fmt, ...) printf("DBG: " fmt "\n",  ##__VA_ARGS__);


enum {
	FXML_TAG_UNKNOWN = 0,
	FXML_TAG_NORMAL,
	FXML_TAG_BANG,
	FXML_TAG_QUEST,
	FXML_TAG_COMMENT,
	FXML_TAG_CDATA,
	FXML_TAG_CLOSE
};





// only normal tags use this structure. the rest are either ignored or decoded silently.
typedef struct FXMLTag {
	struct FXMLTag* parent; // optional
	
	char* start; // the opening tag <
	char* name;
	size_t name_len;
	char* content_start; // first char after opening tag >
	char* content_end; // the < of the close tag
	char* end; // first char after the close tag >
	
	int self_terminating;
	int fully_parsed;
	
	// random info
	int depth;
	
	
} FXMLTag;


typedef struct FXMLFile {
	char* source;
	size_t slen;
	FXMLTag* root;
} FXMLFile;


// main API
FXMLFile* fxmlLoadString(char* source, size_t len);
FXMLFile* fxmlLoadFile(char* path);

// gets a dup'ed, decoded string of all top-level text content (including CDATA) concatenated together
char* fxmlGetTextContents(FXMLTag* t, size_t* len);

// fetches a dup'ed and decoded copy of a certain attribute's value, or null if nonexistent.
char* fxmlGetAttr(FXMLTag* t, char* name);
// fetches a dup'ed and decoded copy of a certain attribute's value, sliced, or null if nonexistent.
char* fxmlGetAttrSlice(FXMLTag* t, char* name, int start, int end);
// returns 0 if not exists
int64_t fxmlGetAttrInt(FXMLTag* t, char* name);
// returns 0.0 if not exist
double fxmlGetAttrDouble(FXMLTag* t, char* name);

// ffirst child of any name, or null if none.
FXMLTag* fxmlTagGetFirstChild(FXMLTag* t);

// first child of a certain name, or null if none.
FXMLTag* fxmlTagFindFirstChild(FXMLTag* t, char* name);

// next sibling of any name, or null if none left. cannot be called on the root node.
FXMLTag* fxmlTagNextSibling(FXMLTag* t, int freePrev);

// next sibling of a certain name, or null if none left. cannot be called on the root node.
FXMLTag* fxmlTagFindNextSibling(FXMLTag* t, char* name, int freePrev);

//fetches the first child with any of the specified names, or null if it doesn't exist.
FXMLTag* fxmlTagFindFirstChildArray(FXMLTag* t, char** names);

//fetches the next sibling with any of the specified names, or null if it doesn't exist.
FXMLTag* fxmlTagFindNextSiblingArray(FXMLTag* t, char** names);


// below are tag-based functions, mostly for internal use


// initializer for static allocations
void fxmlTagInit(FXMLTag* t, char* start, FXMLTag* parent);

// dynamic allocator, calls TagInit
FXMLTag* fxmlTagCreate(char* start, FXMLTag* parent);
// destructor. need to free the struct itself afterward
void fxmlTagDestroy(FXMLTag* t);

// mostly internal use. all have guards to shortcut redundant calls.
static int fxmlTagParseName(FXMLTag* t);
void fxmlTagFindStartOfContent(FXMLTag* t);
void fxmlTagFindEndOfContent(FXMLTag* t);



// below are string-based functions, mostly for internal use.

// returns the length of the tag name 
int fxmlGetTagNameLen(char* start);

// expects a pointer to the opening <
// returns 1 if a given tag is a closing tag
int fxmlIsCloseTag(char* s);

// returns an enum, listed above. mostly internal use.
int fxmlProbeTagType(char* start);

// skips the opening tag of a node or any kind
int fxmlSkipTagDecl(char** start);

// expects a pointer to the first char after the opening tag
void fxmlFindEndOfContent(char** start, char* name, int namelen);

// skips an entire node of any kind
void fxmlSkipTag(char** start);

int fxmlIsCloseTagFor(char* s, char* name, int namelen);

// finds the next opening tag of any kind
void fxmlFindNextTagStart(char** start);

// finds the next opening tag of a normal node, skipping anything else in between
void fxmlFindNextNormalTagStart(char** start);

// for loading
char* fxmlFindRoot(char* source);

void fxmlDecodeCDATA(char** in, char** out);

// not implemented. places a single ? in the output then advances past the input entity.
void fxmlDecodeEntity(char** input, char** output);




#endif // FXML_H_INCLUDED
