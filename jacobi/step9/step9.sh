#!/bin/sh

rm -f 32.o 64.o 128.o 256.o
rm -rf nvprof
mkdir nvprof

CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/32.8    ./jacobi9 ../inputs/8.txt    32 >> 32.o 
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/32.256  ./jacobi9 ../inputs/256.txt  32 >> 32.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/32.1024 ./jacobi9 ../inputs/1024.txt 32 >> 32.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/32.5120 ./jacobi9 ../inputs/5120.txt 32 >> 32.o

CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/64.8    ./jacobi9 ../inputs/8.txt    64 >> 64.o 
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/64.256  ./jacobi9 ../inputs/256.txt  64 >> 64.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/64.1024 ./jacobi9 ../inputs/1024.txt 64 >> 64.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/64.5120 ./jacobi9 ../inputs/5120.txt 64 >> 64.o

CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/128.8    ./jacobi9 ../inputs/8.txt    128 >> 128.o 
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/128.256  ./jacobi9 ../inputs/256.txt  128 >> 128.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/128.1024 ./jacobi9 ../inputs/1024.txt 128 >> 128.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/128.5120 ./jacobi9 ../inputs/5120.txt 128 >> 128.o

CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/256.8    ./jacobi9 ../inputs/8.txt    256 >> 256.o 
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/256.256  ./jacobi9 ../inputs/256.txt  256 >> 256.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/256.1024 ./jacobi9 ../inputs/1024.txt 256 >> 256.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/256.5120 ./jacobi9 ../inputs/5120.txt 256 >> 256.o
