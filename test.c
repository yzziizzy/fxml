

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fxml.h"



int main(int argc, char* argv[]) {
	
	FXMLTag* root, *child, *bro, *sis, *bro2;
	char* bro2name, *sisht, *cont;
	
	
	root = fxmlLoadFile("./sample.xml");
	
	child = fxmlTagGetFirstChild(root);
	printf("got first child: %s \n", child->name);
	
	cont = fxmlGetTextContents(child, NULL);
	printf("child text contents: %s\n", cont);
	
	bro = fxmlTagFindFirstChild(root, "brother");
	printf("got first brother: %s \n", bro->name);
	
	sis = fxmlTagNextSibling(bro);
	sisht = fxmlGetAttr(sis, "height");
	printf("got brother's next sibling (sister): %s height: %s\n", sis->name, sisht);
	
	bro2 = fxmlTagFindNextSibling(bro, "brother");
	bro2name = fxmlGetAttr(bro2, "name");
	printf("got brother's next brother: %s name: %s\n", bro2->name, bro2name);
	
	
	fxmlTagDestroy(root);
	free(root);
	
	
	return 0;
}
