

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fxml.h"



int main(int argc, char* argv[]) {
	
	FXMLTag* root, *child, *bro, *sis, *bro2;
	
	
	
	root = fxmlLoadFile("./sample.xml");
	
	child = fxmlTagGetFirstChild(root);
	printf("got first child: %s \n", child->name);
	
	bro = fxmlTagFindFirstChild(root, "brother");
	printf("got first brother: %s \n", bro->name);
	
	sis = fxmlTagNextSibling(bro);
	printf("got brother's next sibling (sister): %s \n", sis->name);
	
	bro2 = fxmlTagFindNextSibling(bro, "brother");
	printf("got brother's next brother: %s \n", bro2->name);
	
	fxmlTagDestroy(root);
	free(root);
	
	
	return 0;
}
