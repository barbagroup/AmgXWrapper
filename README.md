# AmgXWrapper

This wrapper simplifies the usage of AmgX when using AmgX together with PETSc.
Though we currently only support PETSc, we hope this wrapper can work with other libraries in the future.

The usage is simple. 
Just follow the procedure: ***initialize -> set matrix A -> solve -> finalize***. 

For example,

```c++
int main(int argc, char **argv)
{
    // initialize matrix A, RHS, etc using PETSc
    ...

    // create an instance of the solver wrapper
    AmgXSolver    solver;
    // initialize the instance
    solver.initialize(...);    
    // set matrix A. Currently it only accept PETSc AIJ matrix
    solver.setA(A);    
    // solve. x and rhs are PETSc vectors. unkns will be the final result in the end
    solver.solve(unks, rhs);    
    // get number of iterations
    int         iters = solver.getIters();    
    // get residual at specific iteration
    double      res = solver.getResidual(iters);    
    // finalization
    solver.finalize();
 
    // other codes
    ....

    return 0;
}
```


Note, this wrapper was originally designed for our CFD solver -- **[PetIBM](https://github.com/barbagroup/PetIBM)**,
so it may lack some features for other kinds of PETSc-based applications.
We are trying to make it more general.

## Update: 

### 01/08/2016

Now the wrapper has better performance when there are more MPI processes than the number of GPUs.

For old version,
it had poor performance when we have multiple MPI processes calling AmgX on a single GPU. 
These MPI processes would compete for GPU resource, which is not good.
The solution is to always allow only one MPI process to call AmgX on each GPU.
So, in order to do this, 
this wrapper will now only call AmgX at some of MPI processes when there are more MPI processes than the number of GPUs. 
Data on other processes will be gathered to those calling AmgX before solving, and then scattered back after.

For example, 
if there are totally 18 MPI processes and 6 GPUs (12 MPI processes and 2 GPUs on node 1, and 6 MPI ranks and 4 GPUs on node 2), 
only the rank 0, 6, 12, 14, 16, 17 will call AmgX solvers.
Data on rank 1-5 will be gathered to rank 0; 
data on rank 7-11 will go to rank 6; 
data on rank 13 and 15 go to rank 12 and 14, respectively; 
and both rank 16 and 17 enjoys a single GPU by itself.
This causes some penalties on computing time because of communications between processes.
However, the overall performance is much better comparing to calling AmgX solvers at ALL processes directly.

The redistribution method may be naive, but it's working.
And we have been satisfied with the performance so far.

## Future work:
* Add support of updating entry values of the matrix A that is already on CUDA device.
* Support other matrix format other than AIJ.
* Add mechanisms for debugging and error handling.

## Reference
[1] Chuang, Pi-Yueh; Barba, Lorena A. (2017): Using AmgX to Accelerate PETSc-Based CFD Codes. figshare. [https://doi.org/10.6084/m9.figshare.5018774.v1](https://doi.org/10.6084/m9.figshare.5018774.v1)
