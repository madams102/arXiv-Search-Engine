#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "mpi.h"
#include "mLib.h"
#include "adjacencyBST.h"

#define ARTICLES 1354753

int main(){
	int fildes;
	fildes = open("./txt/arxiv-citations.txt", O_RDONLY);
	//outdes = open("./txt/adjacencyBST.txt", O_WRONLY | O_CREAT, 0666);
	FILE* outdes = fopen("./txt/adjacencyBST.txt", "w");

	//init BST
	node* root = NULL, *temp;
	reference *rootReference = NULL, *tempReference;

	char* id = malloc(sizeof(char) * 20);
	char* refId = malloc(sizeof(char) * 20);

	sprintf(id, "1109.5215"); //I suspect that this will balance it alright...

	temp = createNode(id);

	root = temp;
	root->references = NULL;

	char c;
	int index, r;
	for(int i = 0; i < ARTICLES; i++){
		//read id
		index = 0;
		read(fildes, &c, 1);
		while(c != '\n'){
			id[index++] = c;
			read(fildes, &c, 1);
		}
		id[index] = '\0';
		temp = createNode(id);
		//printf("%d ID %s\n",i,  id);

		//read through -----
		read(fildes, &c, 1);
		while(c != '\n'){
			read(fildes, &c, 1);
		}

		read(fildes, &c, 1);
		while(c != '+'){
			index = 0;
			while(c != '\n'){
				refId[index++] = c;
				read(fildes, &c, 1);
			}
			refId[index] = '\0';
			//printf("%d REF %s\n", i, refId);
			//printf("%d\n", i);
			tempReference = createReference(refId);
			insertNode(root, temp, tempReference);
			read(fildes, &c, 1);
		}
		 
		//read through +++++
		r = 1;
		while(c == '+' && r > 0){
			r = read(fildes, &c, 1);
		}
	}

	writeArticles(root, outdes);
/*
	char* s = malloc(sizeof(char)*20);
	sprintf(s, "alg-geom/5289431");
	temp = createNode(s);
	sprintf(s, "alg-geom/9412017");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);

	sprintf(s, "alg-geom/3333333");
	temp = createNode(s);
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);

	sprintf(s, "alg-geom/8888888");
	temp = createNode(s);
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);
	
	
	sprintf(s, "alg-geom/9412017");
	temp = createNode(s);
	sprintf(s, "alg-geom/5289431");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);
	sprintf(s, "alg-geom/2222222");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);

	sprintf(s, "alg-geom/7777777");
	temp = createNode(s);
	sprintf(s, "alg-geom/9412017");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);
	sprintf(s, "alg-geom/5289431");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);
	sprintf(s, "alg-geom/9134874");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);

	sprintf(s, "alg-geom/9134874");
	temp = createNode(s);
	sprintf(s, "alg-geom/7777777");
	tempReference = createReference(s);
	insertNode(root, temp, tempReference);
	


	node** nodeArray = malloc(sizeof(node*)*3);

	sprintf(s, "alg-geom/7777777");
	nodeArray[0] = search(root,s);

	sprintf(s, "alg-geom/9412017");
	nodeArray[1] = search(root, s);

	sprintf(s, "alg-geom/5289431");
	nodeArray[2] = search(root, s);
	//TODO make a function that queries this and returns a node array with size
	matrix adjMat;
	initMatrix(&adjMat, 3, 3);
	createAdjMatrix(nodeArray, &adjMat);

	printMatrix(&adjMat);
*/
/*
	//printNodes(root);
	sprintf(s, "alg-geom/9412017");
	node* find = search(root, s);

	printReferences(find->references);
	printf("\n");
	sprintf(s, "alg-geom/7777777");
	find = search(root, s);

	printReferences(find->references);
	printf("\n");
	*/
	return 0;
}
