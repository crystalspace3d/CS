/*
    QR_SOLVE - Least Squares Solution of a Linear System A*x=b 
    http://people.sc.fsu.edu/~jburkardt/cpp_src/qr_solve/qr_solve.html

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
void daxpy ( int n, double da, double dx[], int incx, double dy[], int incy );
double ddot ( int n, double dx[], int incx, double dy[], int incy );
double dnrm2 ( int n, double x[], int incx );
void dqrank ( double a[], int lda, int m, int n, double tol, int *kr, 
  int jpvt[], double qraux[] );
void dqrdc ( double a[], int lda, int n, int p, double qraux[], int jpvt[], 
  double work[], int job );
int dqrls ( double a[], int lda, int m, int n, double tol, int *kr, double b[], 
  double x[], double rsd[], int jpvt[], double qraux[], int itask );
void dqrlss ( double a[], int lda, int m, int n, int kr, double b[], double x[], 
  double rsd[], int jpvt[], double qraux[] );
int dqrsl ( double a[], int lda, int n, int k, double qraux[], double y[], 
  double qy[], double qty[], double b[], double rsd[], double ab[], int job );
void drot ( int n, double x[], int incx, double y[], int incy, double c,
  double s );
void drotg ( double *sa, double *sb, double *c, double *s );
void dscal ( int n, double sa, double x[], int incx );
int dsvdc ( double a[], int lda, int m, int n, double s[], double e[], 
  double u[], int ldu, double v[], int ldv, double work[], int job );
void dswap ( int n, double x[], int incx, double y[], int incy );
double *normal_solve ( int m, int n, double a[], double b[], int *flag );
double *qr_solve ( int m, int n, double a[], double b[] );
double *svd_solve ( int m, int n, double a[], double b[] );
