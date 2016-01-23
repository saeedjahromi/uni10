/****************************************************************************
*  @file uni10_lapack_cpu.cpp
*  @license
*    Universal Tensor Network Library
*    Copyright (c) 2013-2014
*    National Taiwan University
*    National Tsing-Hua University
*
*    This file is part of Uni10, the Universal Tensor Network Library.
*
*    Uni10 is free software: you can redistribute it and/or modify
*    it under the terms of the GNU Lesser General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    Uni10 is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Lesser General Public License for more details.
*
*    You should have received a copy of the GNU Lesser General Public License
*    along with Uni10.  If not, see <http://www.gnu.org/licenses/>.
*  @endlicense
*  @brief Implementation file for the ARPACK wrappers
*  @author Chen-Yen Lai
*  @date 2016-01-22
*  @since 0.1.0
*
*****************************************************************************/
#include <iostream>
#ifdef MKL
    #define MKL_Complex8 std::complex<float>
    #define MKL_Complex16 std::complex<double>
    #include "mkl.h"
#else
    #include <uni10/numeric/uni10_lapack_wrapper.h>
#endif
#include <uni10/numeric/uni10_arpack_wrapper.h>

bool arpackEigh(double* A, double* psi, size_t n, size_t& max_iter,
    double& eigVal, double* eigVec, bool ongpu, double err_tol, int nev){
    /* Arpack diagonalization - Real type
          Please see
          https://github.com/opencollab/arpack-ng/blob/master/SRC/dsaupd.f
          for details of dsaupd.
          https://github.com/opencollab/arpack-ng/blob/master/SRC/dseupd.f
          for details of dseupd.
    */
    int dim = n;
    int ido = 0; // ido: reverse communication parameter, must be zero on first iteration
    char bmat = 'I';// bmat: standard eigenvalue problem A*x=lambda*x
    char which[] = {'S','A'};// which: calculate the smallest real part eigenvalue
    double *resid = new double[dim];// If info != 0, RESID contains the initial residual vector, possibly from a previous run.
    memcpy(resid, psi, dim * sizeof(double));// use input psi as initial guess
    int ncv = 42;// the number of columns in v: the number of lanczos vector.
    if( dim < ncv )
        ncv = dim;
    int ldv = dim;
    double *v = new double[ldv*ncv];
    int *iparam = new int[11];
    iparam[0] = 1;       // Specifies the shift strategy (1->exact)
    iparam[2] = max_iter;// Maximum number of iterations
    iparam[6] = 1;       // Sets the mode of dsaupd.
    int *ipntr = new int[11];
    double *workd = new double[3*dim];
    int lworkl = ncv*(ncv+8);// LWORKL must be at least NCV**2 + 8*NCV .
    double *workl = new double[lworkl];
    int info = 1;
    // Parameters for zgemv
    double alpha = 1.0e0;
    double beta = 0.0e0;
    int inc = 1;
    dsaupd_(&ido, &bmat, &dim, &which[0], &nev, &err_tol, resid, &ncv, v, &ldv,
            iparam, ipntr, workd, workl, &lworkl, &info);
    while( ido != 99 ){
        /* Matrix-Vector product here */
        dgemv((char*)"T", &dim, &dim, &alpha, A, &dim, workd+ipntr[0]-1, &inc, &beta,
              workd+ipntr[1]-1, &inc);
        // mvprod(workd+ipntr[0]-1, workd+ipntr[1]-1, 0.0e0);
        dsaupd_(&ido, &bmat, &dim, &which[0], &nev, &err_tol, resid, &ncv, v, &ldv,
                iparam, ipntr, workd, workl, &lworkl, &info);
    }
    if( info < 0 )
        std::cerr << "Error with dsaupd, info = " << info << std::endl;
    else if ( info == 1 )
        std::cerr << "Maximum number of Lanczos iterations reached." << std::endl;
    else if ( info == 3 )
        std::cerr << "No shifts could be applied during implicit Arnoldi update," <<
                     "try increasing NCV." << std::endl;
    int rvec = 1;// rvec = 0 : calculate only eigenvalue
    char howmny = 'A';// how many eigenvectors to calculate: 'A' => nev eigenvectors
    int *select;// when howmny == 'A', this is used as workspace to reorder the eigenvectors
    select = new int[ncv];
    double *d = new double[nev];
    double *z = new double[dim*nev];
    double sigma;
    dseupd_(&rvec, &howmny, select, d, z, &ldv, &sigma, &bmat, &dim, which, &nev,
            &err_tol, resid, &ncv, v, &ldv, iparam, ipntr, workd, workl, &lworkl, &info);
    if ( info != 0 )
        std::cerr << "Error with dseupd, info = " << info << std::endl;
    eigVal = d[0];
    memcpy(eigVec, z, dim * sizeof(double));
    delete [] resid;
    delete [] v;
    delete [] iparam;
    delete [] ipntr;
    delete [] workd;
    delete [] workl;
    delete [] d;
    delete [] z;
    delete [] select;
    if( info == 0 ) return 1;
    else return 0;
}

bool arpackEigh(std::complex<double>* A, std::complex<double>* psi, size_t n,
    size_t& max_iter, double& eigVal, std::complex<double>* eigVec, bool ongpu,
    double err_tol, int nev){
    int dim = n;
    int ido = 0;
    char bmat = 'I';
    char which[] = {'S','R'};// smallest real part
    std::complex<double> *resid = new std::complex<double>[dim];
    memcpy(resid, psi, dim * sizeof(std::complex<double>));
    int ncv = 42;
    if( dim < ncv )
        ncv = dim;
    int ldv = dim;
    std::complex<double> *v = new std::complex<double>[ldv*ncv];
    int *iparam = new int[11];
    iparam[0] = 1;
    iparam[2] = max_iter;
    iparam[6] = 1;
    int *ipntr = new int[14];
    std::complex<double> *workd = new std::complex<double>[3*dim];
    int lworkl = 3*ncv*(ncv+2);// LWORKL must be at least 3*NCV**2 + 5*NCV.*/
    std::complex<double> *workl = new std::complex<double>[lworkl];
    double *rwork = new double[ncv];
    int info = 1;
    // Parameters for zgemv
    std::complex<double> alpha = 1.0e0;
    std::complex<double> beta = 0.0e0;
    int inc = 1;
    // Begin iterations
    znaupd_(&ido, &bmat, &dim, &which[0], &nev, &err_tol, resid, &ncv, v, &ldv,
            iparam, ipntr, workd, workl, &lworkl, rwork, &info);
    while( ido != 99 ){
        zgemv((char*)"T", &dim, &dim, &alpha, A, &dim, workd+ipntr[0]-1, &inc, &beta,
              workd+ipntr[1]-1, &inc);
        // mvprod(workd+ipntr[0]-1, workd+ipntr[1]-1, 0.0e0);
        znaupd_(&ido, &bmat, &dim, &which[0], &nev, &err_tol, resid, &ncv, v, &ldv,
                iparam, ipntr, workd, workl, &lworkl, rwork, &info);
    }
    if( info < 0 )
        std::cerr << "Error with znaupd, info = " << info << std::endl;
    else if ( info == 1 )
        std::cerr << "Maximum number of Lanczos iterations reached." << std::endl;
    else if ( info == 3 )
        std::cerr << "No shifts could be applied during implicit Arnoldi update," <<
                     " try increasing NCV." << std::endl;
    int rvec = 1;
    char howmny = 'A';
    int *select = new int[ncv];
    std::complex<double> *d = new std::complex<double>[nev+1];
    std::complex<double> *z = new std::complex<double>[dim*nev];
    std::complex<double> sigma;
    std::complex<double> *workev = new std::complex<double>[3*ncv];
    zneupd_(&rvec, &howmny, select, d, z, &ldv, &sigma, workev,
            &bmat, &dim, &which[0], &nev, &err_tol, resid, &ncv, v, &ldv, iparam, ipntr,
            workd, workl, &lworkl, rwork, &info);
    if ( info != 0 )
        std::cerr << "Error with dneupd, info = " << info << std::endl;
    eigVal = d[0].real();
    memcpy(eigVec, z, dim * sizeof(std::complex<double>));
    delete [] workev;
    delete [] z;
    delete [] d;
    delete [] select;
    delete [] rwork;
    delete [] workl;
    delete [] workd;
    delete [] ipntr;
    delete [] iparam;
    delete [] v;
    delete [] resid;
    if( info == 0 ) return 1;
    else return 0;
}