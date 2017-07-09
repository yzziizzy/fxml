#!/bin/bash

echo " " 
echo "------------------------"

gcc -o fxml_test test.c fxml.c -ggdb -O0 && 
./fxml_test







