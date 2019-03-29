# Test

To test the code, use the [Poisson example](../example/poisson). This example 
solves a Poisson equation which we already know the exact solution.
After running this example, the error norms of the numerical solution will be
compared against the exact solution. By doing grid convergence tests, one can 
also check the order of convergence. To solve other Poisson equations, users are
free to modify the hard-coded equation in the source code of functions
`generateRHS` and `generateExt`.
