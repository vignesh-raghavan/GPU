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
void iloop(float* A, float* b, int N, float* x, float* y, int offset, int streamsize)
{
	float t;

	int index = offset + (blockIdx.x * blockDim.x) +  threadIdx.x;
	int stride = (gridDim.x * blockDim.x);

	//printf("<<< %d, %d, %d >>>\n", index, (offset+streamsize), stride);

	for(int i = index; i < (offset + streamsize); i += stride)
	{
		t = 0.0;
		for(int j = 0; j < N; j++)
		{
			if(i != j)
			{
				t = t + (( A[ ((N*i)+j) ] ) * x[j]);
			}
		}
		y[i] = ((b[i] - t)/(A[ ((N*i)+i) ]));
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
	int nstreams;
	int streamsize;
	int devID;

   float* A;
   float* b;
   float* x;
	float* y;
   float* c;
   char* fname;
   int N, iter, i, j, M;

   cudaEvent_t start, stop;
   cudaEventCreate(&start);
   cudaEventCreate(&stop);

   if(argc >= 2) fname = argv[1];
   else fname = "../inputs/8.txt";
   if(argc >= 3) M = atoi(argv[2]);
   else M = 32;

	//if(argc >= 3) devID = atoi(argv[2]);
	//else devID = 1;

   //cudaDeviceProp prop;
	//if( cudaGetDeviceProperties(&prop, devID) != cudaSuccess) printf("CUDA : Error with Device Properties\n");
	//else printf("Device : %d\n", prop.id);
	//if( cudaSetDevice(devID) != cudaSuccess ) printf("CUDA : Error with Device ID\n");
	
	
	readFile(fname, &N, &iter, &A, &b);

	printf("CUDA : Parsed file %s\n", fname);

	x = (float*) malloc(N*sizeof(float));
	y = (float*) malloc(N*sizeof(float));

	memset(x, 0.0, N*sizeof(float));
	memset(y, 0.0, N*sizeof(float));
	//for(i = 0; i < N; i++)
	//{
	//	x[i] = 0.0;
	//	y[i] = 0.0;
	//}

	if( cudaMalloc(&d_A, N*N*sizeof(float)) != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_b, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_x, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");
	if( cudaMalloc(&d_y, N*sizeof(float))   != cudaSuccess ) printf("CUDA : memory allocation error!\n");

	if( cudaMemcpy(d_A, A, N*N*sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	if( cudaMemcpy(d_b, b, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	if( cudaMemcpy(d_y, y, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");

	nstreams = 2;
	streamsize = N/nstreams;
	blocksize = (int) fmin(M, streamsize);
	numblocks = streamsize/blocksize;
	//numblocks = (N+blocksize-1)/blocksize;
	printf("CUDA : Streams %d, Grid Size %d, Block size %d\n", nstreams, numblocks, blocksize);
 
	cudaStream_t streams[nstreams];
   for(i = 0; i < nstreams; i++)
	{
		if( cudaStreamCreate(&streams[i]) != cudaSuccess ) printf("CUDA : Error with Stream Creation\n");
	}
	
	cudaEventRecord(start);
   
	if( cudaMemcpy(d_x, x, N*sizeof(float)  , cudaMemcpyHostToDevice) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	for(k = 0; k < iter/2; k++)
	{
		for(i = 0; i < nstreams; i++)
		{
			int offset = i*streamsize;
			iloop<<< numblocks, blocksize, 0, streams[i] >>>(d_A, d_b, N, d_x, d_y, offset, streamsize); // kernel launch on GPU
			iloop<<< numblocks, blocksize, 0, streams[i] >>>(d_A, d_b, N, d_y, d_x, offset, streamsize); // kernel launch on GPU
		}
	}
   if( cudaMemcpy(x, d_x, N*sizeof(float)  , cudaMemcpyDeviceToHost) != cudaSuccess ) printf("CUDA : memory copy error!\n");
	
   cudaEventRecord(stop);
   
   cudaEventSynchronize(stop);
   cudaEventElapsedTime(&time, start, stop);
	
	printf("CUDA : Done Computing Jacobi on GPU\n");
	
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

	for(i = 0; i < nstreams; i++)
	{
	   if( cudaStreamDestroy(streams[i]) != cudaSuccess ) printf("CUDA : Error with Stream Deletion\n");
	}

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
