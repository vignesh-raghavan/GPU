# SERIAL CPU / PARALLEL GPU CODE FOR JACOBI ITERATIVE SOLVER.

To compile in CUDA,

nvcc -o <object filename> <filename.cu>

To run the simulation,

./<stepN.sh>
>
<output filename>
  
To launch visual profiler,

CUDA_VISIBLE_DEVICES=<GPU number> nvvp
