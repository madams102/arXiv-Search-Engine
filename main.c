#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "mpi.h"
#include "mLib.h"
#include "bst.h"
#include "adjacencyBST.h"

void query(char** words, node* root, char** IDArr, int count, node** temp);
int getCount(char** words, node* root, node** temp);
int createAdjMatrix(char** IDArr, matrixf* M, node*, adjNode*);

int main(){
	MPI_Init(NULL, NULL);
	MPI_Comm world = MPI_COMM_WORLD;
	int nprocs, rank;
	MPI_Comm_size(world, &nprocs);
	MPI_Comm_rank(world, &rank);

	clock_t begin, end;

	if(rank == 0)
		begin = clock();
	//open necessary files for building search trees
	MPI_File wordsFildes, refsFildes;
	char* string = malloc(sizeof(char)*14);
	sprintf(string, "./txt/output%d.txt", rank);
	MPI_File_open(world, string, MPI_MODE_RDONLY, MPI_INFO_NULL, &wordsFildes);
	MPI_File_open(world, "./txt/adjacencyBST.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &refsFildes);

	//build words BST
	node *root = NULL;
	char* word = malloc(sizeof(char) * 4);
	sprintf(word, "mmm");
	root = createNode(word);
	root->articles = NULL;
	free(word);

	if(rank == 0)
		printf("Loading back-index. Please wait.\n");
	buildBST(root, wordsFildes);

	//build adjacency BST
	adjNode *rootAdj = NULL;
	char* wordAdj = malloc(sizeof(char)*20);
	sprintf(wordAdj, "1109.5215");
	rootAdj = createadjNode(wordAdj);
	rootAdj->references = NULL;
	free(wordAdj);

	if(rank == 0)
		printf("Loading Adjacency List. Please Wait.\n");
	buildAdjBST(rootAdj, refsFildes);

	MPI_Barrier(world);
	if(rank == 0){
		end = clock();
		double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("Done loading! Finished in %fs\n", time_spent);
	}

	//loop and get user response until they hit enter

	while(1){
		MPI_Barrier(world);
		char str[125];
		char *tokenized[5];
		for(int i = 0; i < 5; i++)
			tokenized[i] = malloc(sizeof(char)*25);

		if(rank == 0){	
			printf("Enter a search query (5 words or less, please)\n");
			fgets(str, 125, stdin);
			begin = clock();
			for(int i = 1; i < nprocs; i++)
				MPI_Send(str, 125, MPI_CHAR, i, 0, world);
		} else {
			MPI_Recv(str, 125, MPI_CHAR, 0, 0, world, MPI_STATUS_IGNORE);
		}

		if(str[0] == '\n')
			break;
		//tokenize the user input into seperate words	
		int index = 0, counter = 0;
		for(int i = 0; i < 125; i++){
			if(str[i] == ' '){
				tokenized[index][counter] = '\0';
				index++;
				counter = 0;
			} else if(str[i] == '\n' || str[i] == EOF){
				tokenized[index][counter] = '\0';
				i = 125;
				for(int j = index+1; j < 5; j++)
					tokenized[j][0] = '\0';
			} else {
				tokenized[index][counter] = str[i];
				counter++;
			}
		}

		//IDArr holds the results from a BST query
		char** IDArr;
		node* temp[5];
		int numResults = getCount(tokenized, root, temp);
		IDArr = (char**)malloc(sizeof(char*) * numResults);
		for(int i = 0; i < numResults; i++)
			IDArr[i] = malloc(sizeof(char) * 20);
		query(tokenized, root, IDArr, numResults, temp);

		for(int i = 0; i < 5; i++)
			free(tokenized[i]);

		//combine results into a single ID arr
		if(rank > 0){
			MPI_Send(&numResults, 1, MPI_INT, 0, 0, world);
			for(int i = 0; i < numResults; i++){
				MPI_Send(IDArr[i], 20, MPI_CHAR, 0, 0, world);
			}
		}

		//retrieve the number of results from other processors to combine into one monolithic "totalIDArr"
		int nonRootNumResults[nprocs-1];
		int total = numResults;
		char** totalIDArr;
		if(nprocs > 1 && rank == 0){
			for(int i = 1; i < nprocs; i++){
				MPI_Recv(&nonRootNumResults[i-1], 1, MPI_INT, i, 0, world, MPI_STATUS_IGNORE);
				total += nonRootNumResults[i-1];
			}
			totalIDArr = (char**)malloc(sizeof(char*)*total);
			for(int i = 0; i < total; i++)
				totalIDArr[i] = malloc(sizeof(char)*20);
			counter = 0;
			for(int i = 0; i < numResults; i++){
				int c = 0;
				while(IDArr[counter][c] != '\0' && c < 19){
					totalIDArr[counter][c] = IDArr[counter][c];
					c++;
				}
				totalIDArr[counter][c] = '\0';
				counter++;
			}
			for(int i = 1; i < nprocs; i++){
				for(int num = 0; num < nonRootNumResults[i-1]; num++){
					MPI_Recv(totalIDArr[counter], 20, MPI_CHAR, i, 0, world, MPI_STATUS_IGNORE);
					counter++;
				}
			}
		}


		int matchFound = 1;
		matrixf adjMatrix;
		int numAdjacent;
		if(rank == 0){
			//create adjacency matrix
			if(nprocs > 1){
				if(total != 0){
					initMatrixfZero(&adjMatrix, total, total);
					numAdjacent = createAdjMatrix(totalIDArr, &adjMatrix, root, rootAdj);
				} else {
					printf("No matching words found\n");
					matchFound = 0;
					numAdjacent = 0;
				}
			} else {
				if(numResults != 0){
				initMatrixfZero(&adjMatrix, numResults, numResults);
				numAdjacent = createAdjMatrix(IDArr, &adjMatrix, root, rootAdj);
				} else {
					printf("No matching words found\n");
					matchFound = 0;
					numAdjacent = 0;
				}
			}
		}

		MPI_Bcast(&numAdjacent, 1, MPI_INT, 0, world);
		//compute hub, authority, and page rank scores
		if(matchFound == 1 && numAdjacent > 10){
			matrixf hub, auth, pagerank;
			initMatrixfOne(&hub, total, 1);
			initMatrixfOne(&auth, total, 1);
			hits(&adjMatrix, &hub, &auth, nprocs, rank, world);

			if(rank == 0){
				for(int i = 0; i < total; i++){
					//printf("%f\t%f\n", hub.arr[i], auth.arr[i]);
			//		if(hub.arr[i] > 0.0001 || auth.arr[i] > 0.0001)
			//			printf("%d\t%s\n", i, totalIDArr[i]);
				}
			}

			//for page rank, adjMatrix must be stochastic
			createStochastic(&adjMatrix);

			initMatrixfOne(&pagerank, total, 1);
			pageRank(&adjMatrix, &pagerank, nprocs, rank, world);

			//output the results
			if(rank == 0){
				double max = 0;
				for(int i = 0; i < total; i++){
					if(pagerank.arr[i] > max){
						max = pagerank.arr[i];
					}
				}
				for(int i = 0; i < total; i++)
					if(pagerank.arr[i] > 0.0001)
						pagerank.arr[i] /= max;
			
				int bestResultsIndex[10];
				double bestResults[10];
				for(int i = 0; i < 10; i++){
					bestResults[i] = -1;
					bestResultsIndex[i] = -1;
				}
				double sum = 0;	
				for(int i = 0; i < total; i++){
					sum = hub.arr[i] + auth.arr[i] + pagerank.arr[i];
					//check if it matches multiple words
					for(int j = 0; j < total; j++){
						if(totalIDArr[i] == totalIDArr[j] && i != j){
							printf("MULTIPLE %s\n", totalIDArr[i]);
							sum += 10;
						}
					}
					for(int j = 0; j < 10; j++){
						if(sum > bestResults[j]){
							bestResults[j] = sum;
							bestResultsIndex[j] = i;
							j = 10;
						}
					}
				}

				//print top 10 results
				for(int i = 0; i < 10; i++){
					if(bestResultsIndex[i] != -1){
						int timesReferenced = 0;
						for(int j = 0; j < total; j++){
							if(totalIDArr[bestResultsIndex[i]] == totalIDArr[j])
								timesReferenced++;	
						}
						if(timesReferenced <= 1)
							printf("https://arxiv.org/abs/%s\n", totalIDArr[bestResultsIndex[i]]);
						else
							printf("https://arxiv.org/abs/%s (contains %d keywords)\n", totalIDArr[bestResultsIndex[i]], timesReferenced);
					}
				}
			}

		//	free(totalIDArr);
		//	free(IDArr);
			free(hub.arr);
			free(auth.arr);
			free(pagerank.arr);
		//if the adjacency matrix is too sparse (less than 10 results), just output as many ID's are referenced in the matrix
		} else if(numAdjacent <= 10 && matchFound == 1){
			if(rank == 0){
				for(int i = 0; i < total; i++){
					for(int j = 0; j < total; j++){
						if(adjMatrix.arr[i*total+j] == 1)
							printf("https://arxiv.org/abs/%s\n", totalIDArr[i]);
					}
				}
			}
		}
		//free up any memory that hasn't already been freed
		for(int i = 0; i < numResults; i++){
			free(IDArr[i]);
		}

		if(rank == 0){		
			for(int i = 0; i < total; i++)
				free(totalIDArr[i]);
		}
		if(rank == 0 && matchFound == 1)
			free(adjMatrix.arr);

		if(rank == 0){
			end = clock();
			double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
			printf("\nSearch took %fs\n\n", time_spent);
		}
	}

	deleteAdjBST(rootAdj);
	deleteBST(root);

	MPI_File_close(&wordsFildes);
	MPI_File_close(&refsFildes);
	MPI_Finalize();
	return 0;
}

//turns article ID sub-tree into an array for easier indexing
int writeToCharArr(article* root, char** IDArr, int index){
	if(root == NULL){
		return index;
	}else {
		int i = 0;
		while(root->id[i] != '\0' && root != NULL){
			IDArr[index][i] = root->id[i];
			i++;
		}
		IDArr[index][i] = '\0';
		index++;
		if(root->left != NULL)
			index = writeToCharArr(root->left, IDArr, index);
		if(root->right != NULL)
			index = writeToCharArr(root->right, IDArr, index);
		return index;
	}
}

//gets a total for how many articles will be considered given a multi-word search
int getCount(char** words, node* root, node** temp){
	int count = 0;
	for(int i = 0; i < 5; i++){
		if(words[i][0] == '\0')
			break;
		temp[i] = search(root, words[i]);
		if(temp[i] != NULL){
			count += countArticles(temp[i]->articles);
		} else{
			words[i][0] = '\0';
		}
	}
	return count;
}

//returns an array IDArr containing all ID's of papers that contain the word that was queried, actual return type is an int which holds the number of ID's in the array
void query(char** words, node* root, char** IDArr, int count, node** temp){
	if(count != 0){
		int index = 0;
		for(int i = 0; i < 5; i++){
			if(words[i][0] != '\0'){
				index = writeToCharArr(temp[i]->articles, IDArr, index);
			}
		}
	}
}

//takes the array of ID's from the query and creates an adjacency matrix from them
int createAdjMatrix(char** IDArr, matrixf *M, node* wordsRoot, adjNode* refsRoot){
	if(M->rows != M->cols){
		printf("matrix not square\n");
		return 0;
	}
	adjNode* temp;
	int adjacentIDs = 0;
	for(int i = 0; i < M->rows; i++){
		if((temp = searchAdj(refsRoot, IDArr[i])) != NULL){
			for(int j = 0; j < M->rows; j++){
				if(i != j)
					if(searchRefs(temp->references, IDArr[j]) != NULL){
						M->arr[j*M->rows + i] = 1;
					//	printf("%s\n", IDArr[j]);
						adjacentIDs++;
					}
			}
		}
		/*if(searchAdj(refsRoot, IDArr[i]) != NULL)
			printf("%d %s\n",i, IDArr[i]);
		else
			printf("%d\t%s\n",i, IDArr[i]);
			*/
	}
	return adjacentIDs;
}

