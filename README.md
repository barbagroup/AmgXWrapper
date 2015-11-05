# AmgXWrapper - A wrapper of AmgX

This wrapper wraps AmgX into a more C++ way in order to simplify the usage of AmgX, especially when using AmgX together with other libraries, like PETSc. Currently we only consider to use it with PETSc, but in the future we may make it works with other libraries.

The usage is simple. Just follow the procedure: ***initialize -> set matrix A -> solve -> finalize***. For example,

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
    int         iters = solver. getIters();    
    // get residual at specific iteration
    double      res = solver.getResidual(iters);    
    // finalization
    solver.finalize();
 
    // other codes
    ....

    return 0;
}
```


Note, this wrapper, at the beginning, is specifically designed for our CFD solver -- **PetIBM**, so it may lack some features in order to become a more general tool. We are trying to make it more general.

#### Future works:
* Add functions for the case that the values of entries in A may be changed
* Support other matrix structures other than AIJ
* Add mechanisms for debug and error handling. For example, check whether the instance is initialized if the codes are compiled under debug mode, or return error codes, etc.
