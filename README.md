# AmgXWrapper

<a style="border-width:0" href="https://doi.org/10.21105/joss.00280">
    <img src="https://joss.theoj.org/papers/10.21105/joss.00280/status.svg" alt="DOI badge" >
</a>
<a style="border-width:0" href="https://www.doi2bib.org/bib/10.21105%2Fjoss.00280">
    <img src="https://img.shields.io/badge/Cite%20AmgXWrapper-bibtex-blue.svg">
</a>

AmgXWrapper simplifies the usage of AmgX when using AmgX together with PETSc.
A unique feature is that when the number of MPI processes is greater than the 
number of GPU devices, this wrapper will do the system consolidation/data 
scattering/data gathering automatically. So there's always only one MPI process
using each GPU and no resource competition. Though we currently only support PETSc,
we hope this wrapper can work with other libraries in the future.

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
    // initialize the instance with communicator, executation mode, and config file
    solver.initialize(comm, mode, file);    
    // set matrix A. Currently it only accept PETSc AIJ matrix
    solver.setA(A);    
    // solve. x and rhs are PETSc vectors. unkns will be the final result in the end
    solver.solve(unks, rhs);    
    // get number of iterations
    int         iters;
    solver.getIters(iters);    
    // get residual at the last iteration
    double      res;
    solver.getResidual(iters, res);    
    // finalization
    solver.finalize();
 
    // other codes
    ....

    return 0;
}
```

Note, this wrapper was originally designed for our CFD solver -- 
**[PetIBM](https://github.com/barbagroup/PetIBM)**, so it may lack some features 
for other kinds of PETSc-based applications. We are trying to make it more general.

## Document

1. [Dependencies](doc/dependencies.md)
2. [Build and install](doc/install.md)
3. [Usage](doc/usage.md)
4. [Test](doc/test.md)
5. [Examples](example/README.md)

## Feature: system consolidation when the number of MPI processes is greater than number of GPUs

If a user ever tries to use AmgX directly without this wrapper, and when 
multiple MPI processes are sharing a GPU device, AmgX does not deliver 
a satisfying performance. One feature of this wrapper is to resolve this problem.

Our solution is always to allow only one MPI process to call AmgX on each GPU.
In other words, only certain MPI processes can talk to GPUs when the number
of MPI processes is greater than the number of GPUs. The wrapper will consolidate
matrix. Data on other processes will be gathered to the processes that can talk
to AmgX before solving, and then scattered back afterward.

For example, if there are 18 MPI processes and 6 GPUs (12 MPI processes 
and 2 GPUs on node 1, and 6 MPI processes and 4 GPUs on node 2), only the rank 0, 6, 
12, 14, 16, 17 can talk to GPUs and will call AmgX solvers. Data on rank 1-5 
will be gathered to rank 0; data on rank 7-11 will go to rank 6; data on rank 
13 and 15 go to rank 12 and 14, respectively; and both rank 16 and 17 enjoy a 
single GPU by itself. This causes some penalties on computing time because of 
communications between processes. However, the overall performance is much 
better than calling AmgX solvers from all processes directly.

The redistribution method may be naive, but it's working. We are satisfied with
the performance so far.

## Limitation

* CPU version of AmgX solver (i.e., `hDDI` mode in AmgX or `AmgX_CPU` option
  in our Poisson example) is not supported in this wrapper. Using PETSc's (or
  third-party's, like Hypre) solvers and preconditioners may be a better choice
  for CPU capability.
* Due to our PETSc applications only use AIJ format for sparse matrices, 
  AmgXWrapper currently only supports AIJ format.

## Future work

* Add support of updating entry values of a matrix that is already on CUDA devices.
* Support other matrix formats other than AIJ.
* Add mechanisms for debugging and error handling.

## Contributing

To contribute to AmgXWrapper, see [CONTRIBUTING](CONTRIBUTING.md).

## How to cite

To cite this code, please use the citation or the BibTeX entry below.

> Pi-Yueh Chuang, & Lorena A. Barba (2017). AmgXWrapper: An interface between PETSc and the NVIDIA AmgX library. _J. Open Source Software_, **2**(16):280, [doi:10.21105/joss.00280](http://dx.doi.org/10.21105/joss.00280)


```console
@article{Chuang2017,
  doi = {10.21105/joss.00280},
  url = {https://doi.org/10.21105/joss.00280},
  year  = {2017},
  month = {aug},
  publisher = {The Open Journal},
  volume = {2},
  number = {16},
  author = {Pi-Yueh Chuang and Lorena A. Barba},
  title = {{AmgXWrapper}: An interface between {PETSc} and the {NVIDIA} {AmgX} library},
  journal = {The Journal of Open Source Software}
}
```

## Reference

[1] Chuang, Pi-Yueh; Barba, Lorena A. (2017): Using AmgX to Accelerate PETSc-Based
CFD Codes. figshare.
[https://doi.org/10.6084/m9.figshare.5018774.v1](https://doi.org/10.6084/m9.figshare.5018774.v1)
