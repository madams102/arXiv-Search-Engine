#ifndef BST_H
#define BST_H

/*
typedef struct ReferencesBST{
	char* id;
	struct ReferencesBST *left;
	struct ReferencesBST *right;
}ref;
*/

typedef struct ArticleBST{
	char* id;
	struct ArticleBST *left;
	struct ArticleBST *right;
	//ref *references;	//search tree of references
}article;

typedef struct BST
{
	char* keyword;
	struct BST *left;
	struct BST *right;
	article *articles; //search tree of articles
}node;
 
node *createNode();
article *createArticle();
node* insertNode(node *,node *, article *);
void insertArticle(article *root, article *new_node);
void printNodes(node *);
void printArticles(article *);
int countNodes(node *);
int countArticles(article*);
node* search(node* root, char *word);
void mergeTrees(node *, node*);
void mergeArticles(article *, article*);
//void mergeTreesParallel(node*, node*, MPI_Comm, int, int);
void writeWords(node *, FILE *);
void writeArticles(article *, FILE *);
void buildBST(node*, MPI_File);
void deleteBST(node*);

int countNodes(node* root){
	int count = 1;
	if(root->left != NULL)
		count += countNodes(root->left);
	if(root->right != NULL)
		count += countNodes(root->right);
	return count;
}

int countArticles(article* root){
	int count = 1;
	if(root->left != NULL)
		count += countArticles(root->left);
	if(root->right != NULL)
		count += countArticles(root->right);
	return count;
}

node* search(node* root, char *keyword){
	node* curr = root;
	int c;
	while((c=strcmp(keyword, curr->keyword)) != 0){
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
		printf("Error finding node\n");
		return NULL;
	}
}

void buildBST(node *root, MPI_File fp){
	char *a;
	char c, d[20];
	int i, count = 0;
	int newline = 1; //bool to determine if node needs to be created
	article *tempArticle = NULL;
	node *temp = NULL;
	d[0] = '\0';

	MPI_Status status;
	MPI_File_read(fp, &c, 1, MPI_CHAR, &status);
	int numRecv;
	MPI_Get_count(&status, MPI_CHAR, &numRecv);
	while(numRecv > 0){
		if(c == ' '){
			if(newline == 1){
				temp = createNode(d);
				//printf("%s\n", d);
				temp->articles = NULL;
				newline = 2;
				for(i = 0; i < 20; i++)
					d[i] = '\0';
				count = 0;
			} else if(newline == 2){
				tempArticle = createArticle(d);
				insertNode(root, temp, tempArticle);
				newline = 0;
				count = 0;
			} else {
				tempArticle = createArticle(d);
				//insertNode(root, temp, tempArticle);
				insertArticle(temp->articles, tempArticle);
				//printArticles(temp->articles);
				for(i = 0; i < 20; i++)
					d[i] = '\0';
				count = 0;
			}
		} else if(c == '\n'){
			//tempArticle = createArticle(d);
			//printf("Creating article %s\n", d);
			//insertNode(root, temp, tempArticle);
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

//inorder traversal of articles
void writeArticles(article *root, FILE *fp){
	if(root != NULL){
		fprintf(fp, "%s", root->id);
		fprintf(fp, " ");
		writeArticles(root->left, fp);
		writeArticles(root->right, fp);
	}
}

// inorder traversal of words
void writeWords(node *root, FILE *fp){
	if(root != NULL){
		fprintf(fp, "%s", root->keyword);
		fprintf(fp, " ");
		writeArticles(root->articles, fp);
		fprintf(fp, "\n");	
		writeWords(root->left, fp);
		writeWords(root->right, fp);
	}
}


article *createArticle(char* id){
	article *temp;
	temp = (article*)malloc(sizeof(article));
	temp->id = malloc(sizeof(char)*20);
	
	for(int i = 0; i < 20; i++){
		temp->id[i] = id[i];
		if(id[i] == '\0')
			i = 20;
	}

	temp->left=temp->right=NULL;
	return temp;
}

node *createNode(char* str){
	node *temp;
	temp=(node*)malloc(sizeof(node));
	temp->keyword = malloc(sizeof(char)*25);
	temp->articles = (article*)malloc(sizeof(article*));

	for(int i = 0; i < 25; i++){
		temp->keyword[i] = str[i];
		if(str[i] == '\0')
			i = 25;
	}
	temp->articles = NULL;
	temp->left=temp->right=NULL;
	return temp;
}
 
node* insertNode(node *root,node *temp, article *article_node){
	int key = strcmp(temp->keyword, root->keyword);
	if(key < 0){
		if(root->left!=NULL){
			return insertNode(root->left,temp, article_node);
		} else {
			root->left=temp;	
			if(root->left->articles == NULL){
				root->left->articles = article_node;
			}else{
	//			insertArticle(root->left->articles, article_node);
			}
			return root->left;
		}
	} else if(key > 0){
		if(root->right!=NULL){
			return insertNode(root->right,temp, article_node);
		} else {
			root->right=temp;
			if(root->right->articles == NULL){
				root->right->articles = article_node;
			}else{
	//			insertArticle(root->right->articles, article_node);
			}
			return root->right;
		}
	} else if(key == 0) {
		//don't insert, only add the article
		if(root->articles == NULL){
			root->articles = article_node;
		}else{
	//		insertArticle(root->articles, article_node);	
		}
		return root;
	}
}


void insertArticle(article *root, article *temp){
	int key = strcmp(temp->id, root->id);
	if(key < 0){
		if(root->left!=NULL)
			insertArticle(root->left,temp);
		else
			root->left=temp;	
	} else if(key > 0){
		if(root->right!=NULL){
			insertArticle(root->right,temp);
		}else{
			root->right=temp;
		}
	} else if(key == 0) {
		//do nothing, article already exists
		//free(temp);
	}

}
 
void printNodes(node *root){
	if(root!=NULL){
		printf("%s ",root->keyword);
		printNodes(root->left);
		printNodes(root->right);
	}
}

void printArticles(article *root){
	if(root != NULL){
		printf("%s ", root->id);
		printArticles(root->left);
		printArticles(root->right);
	}
}

void mergeTrees(node *major, node *minor){
	node *temp;
	if(minor != NULL){
	//	temp = insertNode(major, minor, minor->articles);
		//mergeArticles(temp->articles, minor->articles);
		mergeTrees(major, minor->left);
		mergeTrees(major, minor->right);
	}
}

void mergeArticles(article *root, article* A){
	if(A != NULL){
		insertArticle(root, A);
		mergeArticles(root, A->left);
		mergeArticles(root, A->right);
	}
}


void mergeTreesParallel(node* major, node* minor, MPI_Comm world, int nprocs, int rank){
	node* temp;
	MPI_Win node_window;
	MPI_Win_allocate_shared(sizeof(node*), sizeof(node*), MPI_INFO_NULL, world, &temp, &node_window);
}

void deleteBST(node* root){
	if(root!=NULL){
		deleteBST(root->left);
		deleteBST(root->right);
		free(root);
	}
}
#endif
