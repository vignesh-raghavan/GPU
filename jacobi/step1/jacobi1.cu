#include <cuda.h>
#include <stdio.h>
#include <math.h>

void readFile(char* fname, int* N, int* iter, float** A, float** b)
{
   FILE *fp;
   char buf[100];

   int i, j;

   fp = fopen(fname, "r");
   if(!fp)
   {
      *N = 0;
      *iter = 0;
      printf("Stale File Handle\n");
      return;
   }

   if(fscanf(fp, "%s", buf) > 0) *N = atoi(buf);
   if(fscanf(fp, "%s", buf) > 0) *iter = atoi(buf);
   printf("N = %d\nIterations = %d\n", *N, *iter);

   *b = (float*) malloc(*N*sizeof(float));
   *A = (float*) malloc((*N)*(*N)*sizeof(float));
   for(i = 0; i < *N; i++)
   {
      for(j = 0; j < *N; j++)
      {
         fscanf(fp, "%s", buf);
         (*A)[ ((*N*i)+j) ] = (float)atoi(buf);
      }
   }
   fscanf(fp, "%s", buf); // Ignore the "solution" in the text
   for(i = 0; i < *N; i++)
   {
      fscanf(fp, "%s", buf);
      (*b)[i] = (float)atoi(buf);
   }

   fclose(fp);
}


void jacobi(float** A, float** b, int* N, int* iter, float** x)
{
   int i, j, k;
   float t;
   float *y;

   *x = (float*) malloc(*N*sizeof(float));
   y = (float*) malloc(*N*sizeof(float));
   for(i = 0; i < *N; i++)
   {
      (*x)[i] = 0.0; // Initial Guess
   }

   for(k = 0; k < *iter; k++)
   {
      for(i = 0; i < *N; i++)
      {
         t = 0;
         for(j = 0; j < *N; j++)
         {
            if(i != j)
            {
               t += (((*A)[ ((*N*i)+j) ]) * ((*x)[j]));
            }
         }
         y[i] = (((*b)[i]) - t)/((*A)[ ((*N*i)+i) ]);
         //printf("k %02d i %02d t %f y %f\n", k, i, t, y[i]);
      }

      for(i = 0; i < *N; i++)
      {
         (*x)[i] = y[i];
      }
   }
   //free(y);
}




void iloop(float* A, float* b, int N, float* x, float* y)
{
	float t;

	for(int i = 0; i < N; i++)
	{
		t = 0.0;
		for(int j = 0; j < N; j++)
		{
			if(i != j)
			{
				t = t + (( A[ ((N*i)+j) ] ) * x[j]);
			}
		}
		y[i] = ((b[i] - t)/( A[ ((N*i)+i) ]));
	}
}






int main(int argc, char* argv[])
{
   float time = 0.0;
	float maxError = 0.0;
	float* M;
	int k;

   float* A;
   float* b;
   float* x;
	float* y;
   float* c;
   char* fname;
   int N, iter, i, j;

   cudaEvent_t start, stop;
   cudaEventCreate(&start);
   cudaEventCreate(&stop);

   if(argc == 2) fname = argv[1];
   else fname = "../inputs/8.txt";

   readFile(fname, &N, &iter, &A, &b);

	printf("CUDA : Parsed file%s\n", fname);

	x = (float*) malloc(N*sizeof(float));
	y = (float*) malloc(N*sizeof(float));

	for(i = 0; i < N; i++)
	{
		x[i] = 0.0;
		y[i] = 0.0;
	}

	cudaEventRecord(start);
   
	//jacobi(&A, &b, &N, &iter, &x);

	for(k = 0; k < iter; k++)
	{
		iloop(A, b, N, x, y);
		for(j = 0; j < N; j++) x[j] = y[j];
	}

   cudaEventRecord(stop);
   
   cudaEventSynchronize(stop);
   cudaEventElapsedTime(&time, start, stop);
	
	printf("CUDA : Done Computing Jacobi on CPU\n");

   //for(i = 0; i < N; i++)
   //{
   //   for(j = 0; j < N; j++)
   //   {
   //      printf("%d", (int)A[i][j]);
   //      if(j < N-1) printf(" ");
   //   }
   //   printf("\n");
   //}

   //for(i = 0; i < N; i++)
   //{
   //   printf("%d\n", (int)b[i]);
   //}
   //printf("\n\n");

   //for(i = 0; i < N; i++)
   //{
   //   printf("%f\n", x[i]);
   //}
   //printf("\n\n");

   c = (float*) malloc(N*sizeof(float));

   for(i = 0; i < N; i++)
   {
      c[i] = 0;
      for(j = 0; j < N; j++)
      {
         c[i] += A[ ((N*i)+j) ] * x[j];
      }
      //printf("%0.2f\n", c[i]);
		maxError = fmax(maxError, fabs(c[i] - b[i]));
   }

   printf("\nCUDA : Time %f ms\n", time);
	printf("CUDA : MaxError = %f\n\n\n", maxError);

	free(A);
	free(b);
	free(x);
	free(y);
	free(c);

   return 0;
}
