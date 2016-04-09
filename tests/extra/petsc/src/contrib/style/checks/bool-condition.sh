#!/bin/bash

# Checks for compliance with 
# Rule: 'Do not use if (rank == 0) or if (v == PETSC_NULL) or if (flg == PETSC_TRUE) or if (flg == PETSC_FALSE)'
# Note: Comparison with PetscScalar should still be (val == 0) in order to be compatible with complex.

# Steps:
# - find lines with the patterns mentioned above. 
# 

grep -n -H "rank ==\s*0)\|\s*==\s*NULL)\|==\s*PETSC_TRUE)\|==\s*PETSC_FALSE)" "$@"

