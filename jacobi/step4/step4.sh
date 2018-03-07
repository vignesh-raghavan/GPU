#!/bin/sh

CUDA_VISIBLE_DEVICES=1 ./jacobi4 ../inputs/8.txt
CUDA_VISIBLE_DEVICES=1 ./jacobi4 ../inputs/256.txt
CUDA_VISIBLE_DEVICES=1 ./jacobi4 ../inputs/1024.txt
CUDA_VISIBLE_DEVICES=1 ./jacobi4 ../inputs/5120.txt
