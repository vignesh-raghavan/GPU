# SERIAL CPU / PARALLEL GPU CODE FOR JACOBI ITERATIVE SOLVER.

To compile in CUDA,

nvcc -o object filename.cu

To run the simulation,

./stepN.sh > output
  
To launch visual profiler,

CUDA_VISIBLE_DEVICES=0 nvvp
