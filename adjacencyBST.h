#ifndef ADJACENCYBST_H
#define ADJACENCYBST_H

typedef struct reference{
	char* id;
	struct reference *left;
	struct reference *right;
}reference;

typedef struct AdjBST{
	char* id;
	struct AdjBST *left;
	struct AdjBST *right;
	reference *references;
}adjNode;

adjNode *createadjNode(char *);
reference *createReference(char *);
adjNode *insertadjNode(adjNode *, adjNode*, reference*);
void insertReference(reference *, reference*);
int countAdjNodes(adjNode*);
void printadjNodes(adjNode *);
void printReferences(reference *);
adjNode *searchAdj(adjNode*, char*);
reference *searchRefs(reference*, char*);
void writeAdjArticles(adjNode *, FILE *);
void buildAdjBST(adjNode*, MPI_File);
void deleteAdjBST(adjNode*);

int countAdjNodes(adjNode* root){
	int count = 1;
	if(root->left != NULL)
		count += countAdjNodes(root->left);
	if(root->right != NULL)
		count += countAdjNodes(root->right);
	return count;
}


reference *createReference(char* id){
	reference *temp;
	temp = (reference*)malloc(sizeof(reference));
	temp->id = malloc(sizeof(char)*20);

	for(int i = 0; i < 20; i++){
		temp->id[i] = id[i];
		if(id[i] == '\0')
			i = 20;
	}

	temp->left=temp->right=NULL;
	return temp;
}

adjNode *createadjNode(char* id){
	adjNode *temp;
	temp = (adjNode*)malloc(sizeof(adjNode));
	temp->id = malloc(sizeof(char)*20);
	temp->references = (reference*)malloc(sizeof(reference*));

	for(int i = 0; i < 20; i++){
		temp->id[i] = id[i];
		if(id[i] == '\0')
			i = 20;
	}
	temp->references = NULL;
	temp->left=temp->right=NULL;
	return temp;
}

adjNode* insertadjNode(adjNode *root,adjNode *temp, reference *reference_adjNode){
    int key = strcmp(temp->id, root->id);
    if(key < 0){
        if(root->left!=NULL){
            insertadjNode(root->left,temp, reference_adjNode);
        } else {
            root->left=temp;
            if(root->left->references == NULL)
                root->left->references = reference_adjNode;
//	        else
  //              insertReference(root->left->references, reference_adjNode);
            return root->left;
        }
    } else if(key > 0){
        if(root->right!=NULL){
            insertadjNode(root->right,temp, reference_adjNode);
        } else {
            root->right=temp;
            if(root->right->references == NULL){
                root->right->references = reference_adjNode;
            }else{
 //               insertReference(root->right->references, reference_adjNode);
            }
            return root->right;
        }
    } else if(key == 0) {
        //don't insert, only add the article
        if(root->references == NULL){
            root->references = reference_adjNode;
        }else{
            insertReference(root->references, reference_adjNode);
        }
        //free(temp);
        return root;
    }
}

void insertReference(reference *root, reference *temp){
	int key = strcmp(temp->id, root->id);
	if(key < 0){
		if(root->left != NULL)
			insertReference(root->left, temp);
		else
			root->left = temp;
	} else if(key > 0){
		if(root->right != NULL)
			insertReference(root->right, temp);
	    else 
			root->right = temp;
	} else if(key == 0){
		//do nothing, reference already exists
	}
}

void printadjNodes(adjNode *root){
	if(root != NULL){
		printf("%s ", root->id);
		printadjNodes(root->left);
		printadjNodes(root->right);
	}
}

void printReferences(reference *root){
	if(root != NULL){
		printf("%s ", root->id);
		printReferences(root->left);
		printReferences(root->right);
	}
}

adjNode* searchAdj(adjNode* root, char *id){
	adjNode *curr = root;
	int c;
	while((c=strcmp(id, curr->id)) != 0){
		if(c < 0){
			if(curr->left != NULL){
				curr = curr->left;
			} else {
				return NULL;
			}
		} else if(c > 0){
			if(curr->right != NULL){
				curr = curr->right;
			} else {
				return NULL;
			}
		}
	}
	if(c == 0){
		return curr;
	} else {
		printf("Error finding adjNode\n");
		return NULL;
	}
}

reference *searchRefs(reference* root, char* id){
	reference *curr = root;
	int c;
	while((c=strcmp(id, curr->id)) != 0){
		if(c < 0){
			if(curr->left != NULL){
				curr = curr->left;
			} else {
				return NULL;
			}
		} else if(c > 0){
			if(curr->right != NULL){
				curr = curr->right;
			} else {
				return NULL;
			}
		}
	}
	if(c == 0){
		return curr;
	} else {
		return NULL;
	}	
}

void writeReferences(reference* root, FILE *fp){
	if(root != NULL){
		fprintf(fp, "%s", root->id);
		fprintf(fp, " ");
		writeReferences(root->left, fp);
		writeReferences(root->right, fp);
	}
}

void writeAdjArticles(adjNode* root, FILE * fp){
	if(root != NULL){
		fprintf(fp, "%s", root->id);
		fprintf(fp, " ");
		writeReferences(root->references, fp);
		fprintf(fp, "\n");
		writeAdjArticles(root->left, fp);
		writeAdjArticles(root->right, fp);
	}
}

void buildAdjBST(adjNode *root, MPI_File fp){
	char *a;
	char c, d[20];

	int i, count = 0;
	int newline = 1; //bool to determine if node needs to be created
	reference *tempReference = NULL;
	adjNode *temp = NULL;
	d[0] = '\0';

	MPI_Status status;
	MPI_File_read(fp, &c, 1, MPI_CHAR, &status);
	int numRecv;
	MPI_Get_count(&status, MPI_CHAR, &numRecv);
	while(numRecv > 0){
		if(c == ' '){
			if(newline == 1){
				temp = createadjNode(d);
				newline = 0;
				for(i = 0; i < 20; i++)
					d[i] = '\0';
				count = 0;
			} else {
				tempReference = createReference(d);
				insertadjNode(root, temp, tempReference);
				for(i = 0; i < 20; i++)
					d[i] = '\0';
				count = 0;
			}
		} else if(c == '\n'){
			for(i = 0; i < 20; i++)
				d[i] = '\0';
			newline = 1;
			count = 0;
		} else {
			d[count] = c;
			count++;
			if(count > 18){
				d[19] = '\0';
				count = 0;
			}
		}
		MPI_File_read(fp, &c, 1, MPI_CHAR, &status);
		MPI_Get_count(&status, MPI_CHAR, &numRecv);
	}
}

void deleteAdjBST(adjNode* root){
	if(root != NULL){
		deleteAdjBST(root->left);
		deleteAdjBST(root->right);
		free(root);
	}
}
#endif
