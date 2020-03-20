#ifndef MLIB_H
#define MLIB_H

#define INDEX(n,m,i,j) m*i + j
#define ACCESS(A,i,j) A->arr[INDEX(A->rows, A->cols, i, j)]

typedef struct matrix{
	int rows, cols;
	int* arr;
}matrix;

typedef struct matrixf{
	int rows, cols;
	double* arr;
}matrixf;

void initMatrix(struct matrix* A, int r, int c){
	A->rows = r;
	A->cols = c;
	A->arr = malloc(r*c*sizeof(int));
	int i, j;
	for(i = 0; i < r*c; i++)
		A->arr[i] = rand() % 100 + 1;
}

void initMatrixf(struct matrixf* A, int r, int c){
	A->rows = r;
	A->cols = c;
	A->arr = malloc(r*c*sizeof(double));
	int i;
	for(i = 0; i < r*c; i++)
		A->arr[i] = rand() % 100 + 1;
}

void initMatrixfZero(struct matrixf* A, int r, int c){
	A->cols = c;
	A->rows = r;
	A->arr = malloc(r*c*sizeof(double));
	for(int i = 0; i < r*c; i++)
		A->arr[i] = 0;
}

void initMatrixfOne(struct matrixf* A, int r, int c){
	A->cols = c;
	A->rows = r;
	A->arr = malloc(r*c*sizeof(double));
	for(int i = 0; i < r*c; i++)
		A->arr[i] = 1;
}




void printMatrix(struct matrix* A){
	int i, j;
	for(i = 0; i < A->rows; i++){
		for(j = 0; j < A->cols; j++){
			printf("%d ", A->arr[i*A->cols+j]);
		}
		puts("");
	}
	printf("\n");
}

void printMatrixf(struct matrixf* A){
	int i, j;
	for(i = 0; i < A->rows; i++){
		for(j = 0; j < A->cols; j++){
			//printf("%.2f ", A->arr[i*A->cols+j]);
			if(A->arr[i*A->cols+j] == 1)
				printf("%d\t%d\n", i, j);
		}
		puts("");
	}
	printf("\n");
}

void printMatrixVectorf(struct matrixf* A, struct matrixf* vec){
	int i, j;
	for(i = 0; i < A->rows; i++){
		for(j = 0; j < A->cols; j++){
			printf("%.2f ", A->arr[i*A->cols + j]);
		}
		printf(" | %.2f\n", vec->arr[i]);
	}
	printf("\n");
}

struct matrixf transposef(struct matrixf* A){
	struct matrixf T;
	initMatrixf(&T, A->cols, A->rows);
	int i, j, count, index;
	count = 0;
	for(i = 0; i < A->cols; i++){
		for(j = 0; j < A->rows; j++){
			index = i + (A->cols * j);
			T.arr[count] = A->arr[index];
			count++;
		}
	}
	return T;
}

struct matrix transpose(struct matrix* A){
	struct matrix T;
	initMatrix(&T, A->cols, A->rows);
	int i, j, count, index;
	count = 0;
	for(i = 0; i < A->cols; i++){
		for(j = 0; j < A->rows; j++){
			index = i + (A->cols * j);
			T.arr[count] = A->arr[index];
			count++;
		}
	}
	return T;
}

//helper function for matrixMult, multiplies a vector by a matrix, returns a vector 
void vecMatrixMult(int* returnVec, int* vec, struct matrix* B){
	for(int i = 0; i < B->rows; i++){
		returnVec[i] = 0;
		for(int j = 0; j < B->cols; j++){
			returnVec[i] += vec[j] * B->arr[(i*B->cols)+j];
		}
	}
}

//helper function for matrixMultf, multiplies a vector by a matrix, returns a vector 
void vecMatrixMultf(double* returnVec, double* vec, struct matrixf* B){
	for(int i = 0; i < B->rows; i++){
		returnVec[i] = 0;
		for(int j = 0; j < B->cols; j++){
			returnVec[i] += vec[j] * B->arr[(i*B->cols)+j];
		}
	}
}

void vecMatrixMultfParallel(double* returnVec, double* vec, struct matrixf* B, int nprocs, int rank, MPI_Comm world){
	int div = B->rows / nprocs;
	int rem = B->rows % nprocs;
	MPI_Request request;
	if(rank != 0){
		for(int i = (rank-1)*div; i < ((rank-1)*div)+div; i++){
			double dotProduct = 0;
			for(int j = 0; j < B->cols; j++)
				dotProduct += vec[j] * B->arr[(i*B->cols)+j];
			MPI_Isend(&dotProduct, 1, MPI_DOUBLE, 0, 0, world, &request);
		}
	}
	if(rank == 0){
		for(int i = (nprocs-1)*div; i < (nprocs-1)*div + div + rem; i++){
			returnVec[i] = 0;
			for(int j = 0; j < B->cols; j++)
				returnVec[i] += vec[j] * B->arr[(i*B->cols)+j];
		}
		for(int i = 1; i < nprocs; i++){
			for(int j = 0; j < div; j++){
				MPI_Recv(&returnVec[((i-1)*div)+j], 1, MPI_DOUBLE, i, 0, world, MPI_STATUS_IGNORE);
			}
		}
	}

	MPI_Bcast(returnVec, B->cols, MPI_DOUBLE, 0, world);
}


void matrixMultf(struct matrixf* A, struct matrixf* Bsave, struct matrixf* C, int np, int r, MPI_Comm w){
	printf("A rows %d != Bsave cols %d\n", A->rows, Bsave->cols);
	printf("A rows %d != C cols %d\n", A->rows, C->cols);
	if(A->rows != Bsave->cols || A->rows != C->cols){
		printf("Matrices not of right size for multiplication!\n");
		return;
	}

	MPI_Comm world = w;
	int nprocs = np;
	int me = r;

	//process 0 init
	if(me == 0){
		struct matrixf B;
		initMatrixf(&B, Bsave->rows, Bsave->cols);
		B = transposef(Bsave);

		puts("");
		
		//send B to procs
		for(int i = 1; i < nprocs; i++){
			MPI_Send(&B.rows, 1, MPI_INT, i, 2, world);
			MPI_Send(&B.cols, 1, MPI_INT, i, 2, world);
			MPI_Send(&B.arr[0], B.rows*B.cols, MPI_DOUBLE, i, 2, world);
		}

		//send divisible rows to procs
		int divRows = A->rows / nprocs;
		for(int sendTo = 1; sendTo < nprocs; sendTo++){ //process to send to, do not need to send to root node
			MPI_Send(&divRows, 1, MPI_INT, sendTo, INT_MAX-1, world);
			for(int i = 0; i < divRows; i++){//row to send
				MPI_Send(&A->arr[A->cols*((sendTo*divRows)+i)], A->cols, MPI_DOUBLE, sendTo, 0, world);
			}
		}
	
		//send terminating message to signify that all divisible cols are sent
		for(int i = 1; i < nprocs; i++){
			MPI_Send(&i, 1, MPI_INT, i, INT_MAX, world);
		}	
		
		//distribute remainder rows to procs (should be no more than one sent to each process)
		int remRows = A->rows % nprocs;
		if(remRows >= nprocs){
			printf("biggo error with distributing remaining rows\n");
			return;
		}
		for(int i = 1; i < remRows; i++){ // do not need to send to root node, hence starting at 1
			MPI_Send(&A->arr[A->cols*((A->rows-remRows)+i)], A->cols, MPI_DOUBLE, i, 1, world);
		}

		//send a second terminating message to signify that sending is 100% complete
		for(int i = 0; i < nprocs; i++)
			MPI_Send(&i, 1, MPI_INT, i, INT_MAX-2, world);
		
		//calculate 0's part of the matrix
		double* vector = malloc(A->rows*sizeof(double));
		for(int i = 0; i < divRows; i++){
			vecMatrixMultf(vector, &A->arr[A->cols*i], &B);
			for(int j = 0; j < A->rows; j++)
				C->arr[(i*A->rows) + j] = vector[j];
		}

		//printMatrix(&C);

		//receive the rest of the rows from procs
		MPI_Status status;
		for(int i = 1; i < nprocs; i++)
			for(int j = 0; j < divRows; j++){
				MPI_Recv(vector, A->rows, MPI_DOUBLE, i, 0, world, MPI_STATUS_IGNORE);
				for(int k = 0; k < A->rows; k++){
					C->arr[((i*divRows+j)*A->rows) + k] = vector[k];

				}
			}

		//calculate 0's remainder part of the matrix
		if(remRows != 0){
			vecMatrixMultf(vector, &A->arr[A->cols*(A->rows - remRows)], &B);
			for(int j = 0; j < A->rows; j++)
				C->arr[(A->rows-remRows)*A->rows+ j] = vector[j];
		}		
		
		for(int i = 1; i < remRows; i++){
			MPI_Recv(vector, A->rows, MPI_DOUBLE, i, 1, world, MPI_STATUS_IGNORE);
			for(int j = 0; j < A->rows; j++){
				C->arr[((A->rows-remRows+i)*A->rows + j)] = vector[j];		
			}
		}
		//printMatrix(&C);

	} else { //end proccess 0 init
	
		MPI_Status status;
	

		//receive B
		int rowsB, colsB;
		MPI_Recv(&rowsB, 1, MPI_INT, 0, 2, world, MPI_STATUS_IGNORE);
		MPI_Recv(&colsB, 1, MPI_INT, 0, 2, world, MPI_STATUS_IGNORE);
		struct matrixf BT;
		initMatrixf(&BT, rowsB, colsB);
		MPI_Recv(&BT.arr[0], rowsB*colsB, MPI_DOUBLE, 0, 2, world, MPI_STATUS_IGNORE);

		double* returnVec = malloc(BT.rows * sizeof(double));

		//length of vectors
		int vecLength = Bsave->rows;
		double* vector = malloc(vecLength * sizeof(double));
	
		//get number of vectors 
		int numVectors;
		MPI_Probe(0, MPI_ANY_TAG, world, &status);

		short flag = 0;
		if(status.MPI_TAG == INT_MAX){ // in the case it only receives a remainder
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX, world, MPI_STATUS_IGNORE); //receive to stop from blocking
			numVectors = 1;
			flag = 1;

		} else {
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-1, world, MPI_STATUS_IGNORE);

			for(int i = 0; i < numVectors; i++){
				MPI_Recv(vector, vecLength, MPI_DOUBLE, 0, 0, world, MPI_STATUS_IGNORE);
				vecMatrixMultf(returnVec, vector, &BT);
				MPI_Send(returnVec, BT.rows, MPI_DOUBLE, 0, 0, world);
			}
			
		}

		if(flag == 0){
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX, world, MPI_STATUS_IGNORE); // receive if haven't already done so above
			flag = 0;
		}
		MPI_Probe(0, MPI_ANY_TAG, world, &status);
		if(status.MPI_TAG == INT_MAX-2){
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-2, world, MPI_STATUS_IGNORE); //receie to stop from blocking
			flag = 1;
		} else {	
			MPI_Recv(vector, vecLength, MPI_DOUBLE, 0, MPI_ANY_TAG, world, MPI_STATUS_IGNORE);
			vecMatrixMultf(returnVec, vector, &BT);
			MPI_Send(returnVec, BT.rows, MPI_DOUBLE, 0, 1, world);	
		}
		if(flag == 0)
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-2, world, MPI_STATUS_IGNORE); //receive if haven't already done so (for the second terminating message)

		//send terminating message signifying that all the returnVectors were sent
		MPI_Send(&numVectors, 1, MPI_INT, 0, INT_MAX, world);
	}
}
/*
void matrixMult(struct matrix* A, struct matrix* B, struct matrix* C){
	if(A->rows != B->cols){
		printf("Matrices not of right size for multiplication!\n");
		return;
	}

	MPI_Comm world = MPI_COMM_WORLD;

	int nprocs, me;
	MPI_Comm_size(world, &nprocs);
	MPI_Comm_rank(world, &me);


	//process 0 init
	if(me == 0){
		*B = transpose(B);

		//printMatrix(A);
		puts("");
		//printMatrix(B);
		//send B to procs
		for(int i = 1; i < nprocs; i++){
			MPI_Send(&B->rows, 1, MPI_INT, i, 2, world);
			MPI_Send(&B->cols, 1, MPI_INT, i, 2, world);
			MPI_Send(B->arr, B->rows*B->cols, MPI_INT, i, 2, world);
		}

		//send divisible rows to procs
		int divRows = A->rows / nprocs;
		for(int sendTo = 1; sendTo < nprocs; sendTo++){ //process to send to, do not need to send to root node
			MPI_Send(&divRows, 1, MPI_INT, sendTo, INT_MAX-1, world);
			for(int i = 0; i < divRows; i++){//row to send
				MPI_Send(&A->arr[A->cols*((sendTo*divRows)+i)], A->cols, MPI_INT, sendTo, 0, world);
			}
		}
	
		//send terminating message to signify that all divisible cols are sent
		for(int i = 1; i < nprocs; i++){
			MPI_Send(&i, 1, MPI_INT, i, INT_MAX, world);
		}	
		
		//distribute remainder rows to procs (should be no more than one sent to each process)
		int remRows = A->rows % nprocs;
		if(remRows >= nprocs){
			printf("biggo error with distributing remaining rows\n");
			return;
		}
		for(int i = 1; i < remRows; i++){ // do not need to send to root node, hence starting at 1
			MPI_Send(&A->arr[A->cols*((A->rows-remRows)+i)], A->cols, MPI_INT, i, 1, world);
		}

		//send a second terminating message to signify that sending is 100% complete
		for(int i = 0; i < nprocs; i++)
			MPI_Send(&i, 1, MPI_INT, i, INT_MAX-2, world);
		
		//calculate 0's part of the matrix
		struct matrix C;
		initMatrix(&C, A->rows, B->rows);
		int* vector = malloc(A->rows*sizeof(int));
		for(int i = 0; i < divRows; i++){
			vecMatrixMult(vector, &A->arr[A->cols*i], B);
			for(int j = 0; j < A->rows; j++)
				C.arr[(i*A->rows) + j] = vector[j];
		}

		//printMatrix(&C);

		//receive the rest of the rows from procs
		MPI_Status status;
		for(int i = 1; i < nprocs; i++)
			for(int j = 0; j < divRows; j++){
				MPI_Recv(vector, A->rows, MPI_INT, i, 0, world, MPI_STATUS_IGNORE);
				for(int k = 0; k < A->rows; k++){
					C.arr[((i*divRows+j)*A->rows) + k] = vector[k];

				}
			}

		//calculate 0's remainder part of the matrix
		if(remRows != 0){
			vecMatrixMult(vector, &A->arr[A->cols*(A->rows - remRows)], B);
			for(int j = 0; j < A->rows; j++)
				C.arr[(A->rows-remRows)*A->rows+ j] = vector[j];
		}		
		
		for(int i = 1; i < remRows; i++){
			MPI_Recv(vector, A->rows, MPI_INT, i, 1, world, MPI_STATUS_IGNORE);
			for(int j = 0; j < A->rows; j++){
				C.arr[((A->rows-remRows+i)*A->rows + j)] = vector[j];		
			}
		}
		//printMatrix(&C);

	} else { //end proccess 0 init
	
		MPI_Status status;
	

		//receive B
		int rowsB, colsB;
		MPI_Recv(&rowsB, 1, MPI_INT, 0, 2, world, MPI_STATUS_IGNORE);
		MPI_Recv(&colsB, 1, MPI_INT, 0, 2, world, MPI_STATUS_IGNORE);
		struct matrix BT;
		initMatrix(&BT, rowsB, colsB);
		MPI_Recv(&BT.arr[0], rowsB*colsB, MPI_INT, 0, 2, world, MPI_STATUS_IGNORE);

		int* returnVec = malloc(BT.rows * sizeof(int));

		//length of vectors
		int vecLength = B->rows;
		int* vector = malloc(vecLength * sizeof(int));
	
		//get number of vectors 
		int numVectors;
		MPI_Probe(0, MPI_ANY_TAG, world, &status);

		short flag = 0;
		if(status.MPI_TAG == INT_MAX){ // in the case it only receives a remainder
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX, world, MPI_STATUS_IGNORE); //receive to stop from blocking
			numVectors = 1;
			flag = 1;

		} else {
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-1, world, MPI_STATUS_IGNORE);

			for(int i = 0; i < numVectors; i++){
				MPI_Recv(vector, vecLength, MPI_INT, 0, 0, world, MPI_STATUS_IGNORE);
				vecMatrixMult(returnVec, vector, &BT);
				MPI_Send(returnVec, BT.rows, MPI_INT, 0, 0, world);
			}
			
		}

		if(flag == 0){
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX, world, MPI_STATUS_IGNORE); // receive if haven't already done so above
			flag = 0;
		}
		MPI_Probe(0, MPI_ANY_TAG, world, &status);
		if(status.MPI_TAG == INT_MAX-2){
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-2, world, MPI_STATUS_IGNORE); //receie to stop from blocking
			flag = 1;
		} else {	
			MPI_Recv(vector, vecLength, MPI_INT, 0, MPI_ANY_TAG, world, MPI_STATUS_IGNORE);
			vecMatrixMult(returnVec, vector, &BT);
			MPI_Send(returnVec, BT.rows, MPI_INT, 0, 1, world);	
		}
		if(flag == 0)
			MPI_Recv(&numVectors, 1, MPI_INT, 0, INT_MAX-2, world, MPI_STATUS_IGNORE); //receive if haven't already done so (for the second terminating message)

		//send terminating message signifying that all the returnVectors were sent
		MPI_Send(&numVectors, 1, MPI_INT, 0, INT_MAX, world);
	}
}
*/

void norm(double* b_hat, double* b, int len){
    //determines the length of an array, or size of vector in this case
    //magnitude of b
    double b_mag = 0;
    for(int i=0; i<len; i++){
        //want the square root of all elements in b squared
        //e.g.:     sqrt(b_1^2 + b_2^2+ ... + b_n^2)
        b_mag += (b[i] * b[i]);
    }

    //v_hat = 1/b_mag, represents the 'unit' by which b is being normalized
    double v_hat = 0;
    v_hat = 1/sqrt(b_mag);

    //multiply b*v_hat to find normalized vector b_hat
    for( int i=0; i<len; i++){
        b_hat[i] = b[i] * v_hat;
    }
}


/*
. Implement a first-pass attempt to calculate the eigenvector corresponding to the largest eigenvector of
a matrix. You may use the naıve power-method discussed in lecture:
(a) Let x be an all 1 vector in the appropriate dimension
(b) Perform update: x ← Ax
(c) Normalize: x ← x/kxk2
.
(d) From this, x will converge quite quickly to the eigenvector corresponding to the largest eigenvalue.
You may obtain an estimate of this eigenvalue by computing kAxk2
/kxk2
.*/void powerMethod(struct matrixf* M, double *retX, int nprocs, int rank, MPI_Comm world){
	double* x = malloc(sizeof(double) * M->rows);
	double* x_i = malloc(sizeof(double)*M->rows);
	double* x_hat = malloc(sizeof(double) * M->rows);
	//retX = realloc(retX, sizeof(double) * M->rows);



	for(int i=0; i<M->rows; i++){ x[i] = 1; }

	float delta = 1.0;
    float epsilon = .00001;

	while(delta > epsilon){
		vecMatrixMultfParallel(x_i, x, M, nprocs, rank, world);
		//vecMatrixMultf(x_i, x, M);
		if(rank == 0)
			norm(x_hat, x_i, M->cols);
		MPI_Bcast(x_hat, M->cols, MPI_DOUBLE, 0, world);
		
		for(int i=0; i<M->rows; i++){
			x[i] = x_hat[i];
		}
		delta = delta - x_hat[0];
		if(delta < 0)
			delta *= -1;
		if(delta <= epsilon){
			delta = 0;
			for(int i = 0; i < M->rows; i++){
				retX[i] = x_hat[i];
				printf("%f\n", retX[i]);
			}
		} else {
			delta = x_hat[0];
		}
	}	
	
	free(x);
	free(x_i);
	free(x_hat);
	
	/* GOAL FOR M=[84 87 78; 16 94 36; 87 93 50;] VIA OCTAVE
		0.70031
	    0.33370
	    0.63104
	*/
}

void normalize(matrixf* A){
	int i, j;
	double add = 0;
	for(i = 0; i < A->rows; i++){
		for(j = 0; j < A->cols; j++){
			double flan = A->arr[i*A->cols+j];
			flan = flan*flan;
			add = add + flan;
		}
	}
	add = sqrt(add);
	for(i = 0; i < A->rows; i++){
		for(j = 0; j < A->cols; j++){
			A->arr[i*A->cols+j] = A->arr[i*A->cols+j]/add;
		}
	}
}

double totalMatrix(matrixf* A){
	int i;
    double add = 0;
	for(i = 0; i < A->rows; i++){
		add = add + A->arr[i*A->cols];
	}
}

void hitsMatrixMult(matrixf* A, matrixf *B, matrixf *C, int nprocs, int rank, MPI_Comm world){
	if(rank == 0 && (A->cols != B->rows && A->cols != C->rows && B->cols != C->cols != 1)){
		printf("A %d %d\n", A->rows, A->cols);
		printf("B %d %d\n", B->rows, B->cols);
		printf("C %d %d\n", C->rows, C->cols);
		printf("Matrices not of right size\n");
		return;
	}
	
	if(rank == 0){
		int div = A->rows / nprocs;
		int rem = A->rows % nprocs;
		
		for(int i = 1; i < nprocs; i++){
			MPI_Send(&A->cols, 1, MPI_INT, i, 0, world);
			MPI_Send(&div, 1, MPI_INT, i, 0, world);
			MPI_Send(B->arr, B->rows, MPI_DOUBLE, i, 0, world);
		}
		
		double* cLocal = malloc(sizeof(double)*div+rem);
		MPI_Request request;
		for(int i = 0; i < div; i++){
			for(int p = 1; p < nprocs; p++){
				MPI_Isend(&A->arr[((div*(p-1))+i)*A->cols], A->rows, MPI_DOUBLE, p, 0, world, &request);
			}
			//compute local part
			double sum = 0;
			for(int j = 0; j < A->cols; j++){
				sum += A->arr[((div*(nprocs-1))+i)*A->cols+j] * B->arr[j];
			}
			cLocal[i] = sum;
		}
		//compute remainder
		for(int i = 0; i < rem; i++){
			double sum = 0;
			for(int j = 0; j < A->cols; j++){
				sum += A->arr[((div*nprocs)+i)*A->cols+j] * B->arr[j];
			}
			cLocal[i+div] = sum;
		}
		//collect cPartial from other ranks
		for(int i = 1; i < nprocs; i++){
			MPI_Recv(&C->arr[div*(i-1)], div, MPI_DOUBLE, i, 0, world, MPI_STATUS_IGNORE);
		}
		//append cLocal to C array
		for(int i = 0; i < div+rem; i++){
			C->arr[div*(nprocs-1)+i] = cLocal[i];
		}

		free(cLocal);
	} else {
		int div, cols;
		MPI_Recv(&cols, 1, MPI_INT, 0, 0, world, MPI_STATUS_IGNORE);
		MPI_Recv(&div, 1, MPI_INT, 0, 0, world, MPI_STATUS_IGNORE);

		double* bLocal = malloc(sizeof(double)*cols);
		MPI_Recv(bLocal, cols, MPI_DOUBLE, 0, 0, world, MPI_STATUS_IGNORE);
		double* row = malloc(sizeof(double)*cols);
		double* cPartial = malloc(sizeof(double)*div);

		//calculate all of rows and store in cPartial 
		for(int i = 0; i < div; i++){
			MPI_Recv(row, cols, MPI_DOUBLE, 0, 0, world, MPI_STATUS_IGNORE);
			double sum = 0;
			for(int j = 0; j < cols; j++){
				sum += row[j] * bLocal[j];
			}
			cPartial[i] = sum;
		}
		//send cPartial back to root
		MPI_Send(cPartial, div, MPI_DOUBLE, 0, 0, world);
	
		free(bLocal);
		free(row);
		free(cPartial);
	}
}

//matrix B and C are Nx1 matrices
void hits(matrixf* A, matrixf *hub, matrixf *auth, int nprocs, int rank, MPI_Comm world){
	double curr, former = 0, result;
	struct matrixf T = transposef(A);
	while(1){
		hitsMatrixMult(&T, hub, auth, nprocs, rank, world);
		hitsMatrixMult(A, auth, hub, nprocs, rank, world);

		if(rank == 0){
			normalize(hub);
			normalize(auth);
			curr = totalMatrix(auth);		
		}
		MPI_Bcast(&curr, 1, MPI_DOUBLE, 0, world);
		result = curr - former;
		//printf("result %f\n", result);
		if(result < 0)
			result *= -1;
		if((result) < .001){
			return;
		}
		former = curr;
	}	
}

void createStochastic(matrixf* A){
	for(int i = 0; i < A->cols; i++){
		int sum = 0;
		for(int j = 0; j < A->rows; j++){
			if(A->arr[j*A->rows+i] > 0)
				sum++;
		}
		if(sum != 0){
			for(int j = 0; j < A->rows; j++)
				if(A->arr[j*A->rows+i] > 0.0001){
					A->arr[j*A->rows+i] /= sum;
				}
		}
	}
}

//returns an nx1 matrix (which is a "vector")
void pageRank(matrixf* A, matrixf* ret, int nprocs, int rank, MPI_Comm world){
	if(rank == 0){
		int sum = 0;
		for(int i = 0; i < A->rows; i++){
			if(ret->arr[i] > 0.0001)
				sum++;
		}
		for(int i = 0; i < A->rows; i++)
			if(ret->arr[i] > 0.0001 && sum != 0){
				ret->arr[i] /= sum;
			}
	}	
	int size;
	if(rank == 0)
		size = A->rows;
	MPI_Bcast(&size, 1, MPI_INT, 0, world);

	matrixf temp;
	temp.rows = size;
	temp.cols = 1;
	temp.arr = malloc(sizeof(double)*size);
	if(rank == 0)
		for(int i = 0; i < size; i++)
			temp.arr[i] = ret->arr[i];

	double curr, former = 0, result;

	while(1){
		hitsMatrixMult(A, &temp, ret, nprocs, rank, world);
		if(rank == 0){
			for(int i = 0; i < size; i++)
				temp.arr[i] = ret->arr[i];
			normalize(&temp);
			curr = totalMatrix(&temp);
		}
		MPI_Bcast(&curr, 1, MPI_DOUBLE, 0, world);
		result = curr - former;
		//printf("result %f\n", result);
		if(result < 0)
			result *= -1;
		if(result < .001){
			return;
		}
		former = curr;
	}
	free(temp.arr);
}

#endif
