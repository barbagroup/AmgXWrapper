---
title: 'AmgXWrapper: An interface between PETSc and NVIDIA AmgX library'
tags:
  - PETSc
  - AmgX
  - multigrid
  - multi-GPU computing
authors:
 - name: Pi-Yueh Chuang
   orcid: 0000-0001-6330-2709
   affiliation: The George Washington University
 - name: Lorena A. Barba
   orcid: 0000-0001-5812-2711
   affiliation: The George Washington University
date: 02 June 2017
---

# Summary

This code provides a capability of multi-GPU computing to PETSc-based applications through NVIDIA's AmgX.
PETSc (Portable, Extensible Toolkit for Scientific Computation),
developed by Argonne National Laboratory, is a library for developing
high-performance parallel scientific software through MPI model. 
NVIDIA's AmgX is a multi-GPU linear algebra solver library which
supports multigrid preconditioners and Krylov solvers.
Because of the lack of support to modern heterogeneous platforms (GPU+CPU) in PETSc,
AmgX becomes a good option for heterogeneous computing to PETSc applications.
Nevertheless, it is not a trivial task to incorporate AmgX into PETSc
applications due to the different designs, underlying data formats, and usages between
PETSc and AmgX.
The purpose of this wrapper is to bridge PETSc and AmgX, so that PETSc application
developers can use AmgX easily and without a thorough understanding of AmgX API.
The key benefit is that when incorporating AmgX in legacy PETSc applications,
developers need only a few lines of code modification.
The wrapper also features implicit data transfer when there are mismatch numbers
of CPU cores and GPU devices on a computing node.
This allows us to exploit all possible resources on modern heterogeneous platforms.
We have presented an example of using AmgX and AmgXWrapper to accelerate an
existing PETSc-based CFD code (see reference 1).



# References
1. Chuang, Pi-Yueh, Lorena A. Barba. "Using AmgX to Accelerate PETSc-Based
CFD Codes." (2017) figshare.
[https://doi.org/10.6084/m9.figshare.5018774.v1](https://doi.org/10.6084/m9.figshare.5018774.v1)

2. Balay, Satish, Shrirang Abhyankar, Mark~F. Adams, Jed Brown, Peter Brune,
Kris Buschelman, Lisandro Dalcin, Victor Eijkhout, William~D. Gropp, Dinesh Kaushik,
Matthew~G. Knepley, Lois Curfman McInnes, Karl Rupp, Barry~F. Smith, Stefano Zampini,
Hong Zhang, and Hong Zhang. "PETSc web page." (2016) [Online]. Available:
http://www.mcs.anl.gov/petsc (visited on 07/13/2016).
