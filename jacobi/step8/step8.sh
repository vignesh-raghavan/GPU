#!/bin/sh

CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/1024.8.o    ./jacobi8 ../inputs/8.txt    >> 1024.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/1024.256.o  ./jacobi8 ../inputs/256.txt  >> 1024.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/1024.1024.o ./jacobi8 ../inputs/1024.txt >> 1024.o
CUDA_VISIBLE_DEVICES=1 nvprof --log-file nvprof/1024.5120.o ./jacobi8 ../inputs/5120.txt >> 1024.o
