#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "mpi.h"

//#include "mLib.h"
#include "bst.h"
//#include "pageRank.h"

#define NUM_ARTICLES 1615712

int main(){

	MPI_Init(NULL, NULL);
	MPI_Comm world = MPI_COMM_WORLD;
	int nprocs, rank;
	MPI_Comm_size(world, &nprocs);
	MPI_Comm_rank(world, &rank);

	MPI_File fd;
	MPI_File_open(world, "txt/out.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &fd);

	unsigned long num_chars = 1673324889;
	unsigned int charsPerProc = num_chars / nprocs;
	int localRange[2];
	char c;
	int seek;
	short flag;
	if(rank == 0){
		//determine the seek ranges for each processor
		localRange[0] = 0;
		localRange[1] = charsPerProc;

		for(int i = 0; i < nprocs-1; i++){
			MPI_File_seek(fd, charsPerProc*(i+1), MPI_SEEK_SET);
			seek = charsPerProc*(i+1);
			flag = 1;
			printf("%d\n", i);
			while(flag == 1){
				while(c != '+'){ //read until character is a '+'
					MPI_File_read(fd, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
					seek++;
				}
				for(int j = 0; j < 5; j++){ //make sure it's not just reading a stray '+', and indeed is '++++++'
					MPI_File_read(fd, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
					seek++;
					if(c != '+')
						j = 6;
					if(j == 4)
						flag = 0;
				}
			}

			localRange[1] = seek;
			MPI_Send(&localRange, 2, MPI_INT, i, 0, world);

			localRange[0] = localRange[1] + 1;

			if(i == nprocs-2 && nprocs >= 3){
				//send to last node
				localRange[1] = num_chars;
				MPI_Send(&localRange, 2, MPI_INT, i+1, 0, world);
			}
		}
		if(nprocs == 1){
			localRange[0] = 0;
			localRange[1] = num_chars;
			MPI_Send(&localRange, 2, MPI_INT, 0, 0, world);
		}	
	}

	int range[2];
	MPI_Recv(&range, 2, MPI_INT, 0, 0, world, MPI_STATUS_IGNORE);

	printf("Received %d %d\n", range[0], range[1]);

	//ranges are received, start entering data into bst
	node *root = NULL, *temp;
	article *rootArticle = NULL, *tempArticle;

	char* word = malloc(sizeof(char) * 25);
	char* id = malloc(sizeof(char)*20);
	
	//init root node to a string that is lexicographically "straddling the fence"
	sprintf(word, "mmm");
	temp = createNode(word);

	root = temp;
	root->articles = NULL;	

	MPI_File_seek(fd, range[0], MPI_SEEK_SET);
	seek = range[0];
	char buf = '\0';

	int index = 0;
	MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
	seek++;
	id[index++] = buf;
	clock_t begin, end;
	double time_spent;

	char unimportantWords[121][10] = {
		"a",
		"in",
		"is",
		"for",
		"the",
		"of",
		"at",
		"all",
		"from",
		"and",
		"as",
		"are",
		"with",
		"that",
		"to",
		"we",
		"use",
		"have",
		"give",
		"which",
		"may",
		"not",
		"be",
		"known",
		"has",
		"because",
		"if",
		"when",
		"how",
		"where",
		"on",
		"each",
		"an",
		"this",
		"just",
		"them",
		"been",
		"an", 
		"by",
		"but",
		"also",
		"our",
		"due",
		"those",
		"it",
		"can",
		"us",
		"these",
		"whose",
		"so",
		"get",
		"through",
		"must",
		"could",
		"their",
		"used",
		"its",
		"per",
		"data",
		"test",
		"gains",
		"showed",
		"more",
		"some",
		"idea",
		"using",
		"few",
		"large",
		"till",
		"while",
		"there",
		"exists",
		"than",
		"becoming",
		"no",
		"found",
		"like",
		"will",
		"open",
		"inside",
		"observed",
		"most",
		"least",
		"best",
		"worst",
		"open",
		"they",
		"upper",
		"two",
		"one",
		"new",
		"were",
		"nor",
		"neither",
		"toward",
		"limit",
		"present",
		"report",
		"would",
		"expect",
		"examples",
		"between",
		"same",
		"review",
		"complete",
		"other",
		"described",
		"was",
		"already",
		"shown",
		"become",
		"highly",
		"both",
		"studies",
		"via",
		"does",
		"results",
		"tau",
		"out",
		"near",
		"away"
	};

	if(rank >= 7)
	while(seek <= (range[1])){	
		begin = clock();
		//read in ID
		if(index > 23) //half baked solution to fixing "double free or corruption" error
			index = 0;
		while(buf != '\n'){
			id[index++] = buf;
			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;
		}
		id[index] = '\0';
		//printf("%s\n", id);

		index = 0;

		//skip past title and authors
		MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
		seek++;
		while(buf != '\n'){
			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;
		}

		MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
		seek++;
		while(buf != '\n'){
			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;
		}

		//printf("READING.........\n");
		//read in words and enter in bst
		MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
		seek++;
		index = 0;
		while(buf != '\n'){
			if(buf == ' ' && index < 24){
				word[index] = '\0';
				if(word[0] != '\0'){
					flag = 0;
					for(int i = 0; i < 121; i++){
						if(strcmp(word, unimportantWords[i]) == 0)
							flag = 1;
					}
					if(flag == 0 && strlen(word) > 4){
						temp = createNode(word);
						tempArticle = createArticle(id);
						node *t =insertNode(root, temp, tempArticle);
						insertArticle(t->articles, tempArticle);
					}
				}
				index = 0;
			}

			if(index >= 24){
				MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
				seek++;
				while(buf != ' '){
					MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
					seek++;
				}
			}

			if((buf >= 65 && buf <= 90) || (buf >= 97 && buf <= 122)){
				if(buf >= 65 && buf <= 90) //make all letters lowercase
					buf += 32;
				word[index++] = buf;
			} else {
				index = 0;
				word[0] = '\0';
				while(buf != ' '){
					MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
					seek++;
				}
			}

			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;

		}

		//skip past the '++++++'
		while(buf != '+'){
			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;
		}
		while(buf != '\n'){ 
			MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
			seek++;
		}
		MPI_File_read(fd, &buf, 1, MPI_CHAR, MPI_STATUS_IGNORE);
		seek++;
		
		end = clock();
		time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
		double percent = seek / (double)(range[1] - range[0]);
		if(rank == 7)
			printf("%fs\t%f percent\n", time_spent, percent); 

	}

	FILE* output;
	char* outputFileString = malloc(sizeof(char)*14);
	sprintf(outputFileString, "./output%d.txt", rank);
	output = fopen(outputFileString, "w");


	printf("%d writing\n", rank);
	if(rank >= 7)
	writeWords(root, output);
	deleteBST(root);
/*
	node* after=NULL;
	sprintf(word, "mmm");
	after = createNode(word);
	after->articles = NULL;
*/
	//buildBST(after, output);
//	printArticles(after->left->articles);

/*
	node *root=NULL, *temp, *root2=NULL;
	article *rootArticle=NULL, *tempArticle;
	char* str = malloc(sizeof(char)*20);
	char* id = malloc(sizeof(char)*20);
	sprintf(str, "sean");
	sprintf(id, "1830.3195");
	temp=createNode(str);
	tempArticle = createArticle(id);

	root = temp; //have to initialize the BST before inserting nodes. Error
				 //checking for that in the function proved to be more trouble
				 //than it was worth
	root->articles = tempArticle;

	sprintf(id, "penis-tiddies");
	tempArticle = createArticle(id);
	insertNode(root, temp, tempArticle);

	sprintf(id, "testacle-tiddies");
	tempArticle = createArticle(id);
	insertNode(root, temp, tempArticle);

	//-------------------------


	temp = createNode(str); //create a second tree to test merging
	//sprintf(str, "will");
	sprintf(id, "1111.1111");
	//temp = createNode(str);
	tempArticle = createArticle(id);
	
	root2 = temp;
	root2->articles = tempArticle;

	sprintf(id, "2222.2222");
	tempArticle = createArticle(id);
	insertNode(root2, temp, tempArticle);

	
	mergeTrees(root, root2);

	printf("preorder traversal\n");
	printNodes(root);
	printArticles(root->articles);
	printf("\n");
*/
	MPI_File_close(&fd);
	MPI_Finalize();
	return 0;
}
