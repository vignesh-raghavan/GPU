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


__global__
void iloop(float* A, float* b, int N, float* x, float* y)
{
	extern __shared__ float shMem[];
	float* shX = shMem;
	float* shT = (float*) &shMem[N];

	int bid = blockIdx.x;
	int bstride = gridDim.x;
	int tid = threadIdx.x;
	int tstride = blockDim.x;
	int i, j, k;
	//float t;

	//printf("<<< %d, %f, %f >>>\n", N, A[0], b[0]);
	for(k = tid; k < N; k += tstride) 
	{
		shX[k] = x[k];
	}

	__syncthreads();

	for(i = bid; i < N; i += bstride)
	{
		//t = 0.0;
		shT[tid] = 0.0;
		for(j = tid; j < N; j += tstride)
		{
			shT[tid] += A[ ((N*i)+j) ] * shX[j];
		}
		//shT[tid] = t;

		__syncthreads();

		for(k = tstride/2; k > 0; k>>=1)
		{
			if(tid < k) shT[tid] += shT[ (tid+k) ];

			__syncthreads();
		}

		if(tid == 0) y[i] = shX[i] + ( (b[i] - shT[0])/(A[ ((N*i)+i) ]) );
	}
}



int main(int argc, char* argv[])
{
   float time = 0.0;
	float maxError = 0.0;
	float* d_A;
	float* d_b;
	float* d_x;
	float* d_y;
	int k;
	int blocksize;
	int numblocks;

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

   if(argc >= 2) fname = argv[1];
   else fname = "../inputs/8.txt";

   readFile(fname, &N, &iter, &A, &b);

	printf("CUDA : Parsed file %s\n", fname);

	x = (float*) malloc(N*sizeof(float));
	y = (float*) malloc(N*sizeof(float));
	
	for(i = 0; i < N; i++)
	{
		x[i] = 0.0;
		y[i] = 0.0;
	}

	if( cudaMalloc(&d_A, N*N*sizeof(float)) != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_b, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_x, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_y, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");

	if( cudaMemcpy(d_A, A, N*N*sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	if( cudaMemcpy(d_b, b, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	if( cudaMemcpy(d_y, y, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	//for(i = 0; i < N*N; i++)
	//{
	//	d_A[i] = A[i];
	//}
	//for(i = 0; i < N; i++)
	//{
	//	d_b[i] = b[i];
	//	d_x[i] = x[i];
	//	d_y[i] = y[i];
	//}

	blocksize = 1024;//fmin(1024, N);
	//numblocks = (N+blocksize-1)/blocksize;
	numblocks = 112; 
	printf("CUDA : Grid Size %d, Block size %d\n", numblocks, blocksize);

	cudaEventRecord(start);
   
	if( cudaMemcpy(d_x, x, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	for(k = 0; k < iter; k++)
	{
		iloop<<< numblocks, blocksize, ((N+blocksize)*sizeof(float)) >>>(d_A, d_b, N, d_x, d_y); // kernel launch on GPU
	   if( cudaMemcpy(d_x, d_y, N*sizeof(float)  , cudaMemcpyDeviceToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
		//if(k%100 == 0)printf("CUDA: %d\n", k);
	}
   if( cudaMemcpy(x, d_x, N*sizeof(float)  , cudaMemcpyDeviceToHost) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	
	//cudaDeviceSynchronize();
	//for(j = 0; j < N; j++) printf("CUDA : %f\n", d_y[j]);

   cudaEventRecord(stop);
   
   cudaEventSynchronize(stop);
   cudaEventElapsedTime(&time, start, stop);
	
	printf("CUDA : Done Computing Jacobi on GPU\n");
	
	//Verify : sdiff -s <output> inputs/<input>
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

	//for(i = 0; i < N; i++) x[i] = d_x[i];

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

	cudaFree(d_A);
	cudaFree(d_b);
	cudaFree(d_x);
	cudaFree(d_y);
	free(A);
	free(b);
	free(x);
	free(y);
	free(c);

   return 0;
}
