/*
 *  NOTE: Identical to Peter Glaus' implementation, except for a single
 *  modified error statement (lowess.cpp line 68).
 *  See Artistic License 2, clause 9 (at http://www.perlfoundation.org/artistic_license_2_0).
 *  Reference version can be found at https://github.com/BitSeq/BitSeq
 *
 *  c++ implementation of Lowess weighted regression by 
 *  Peter Glaus http://www.cs.man.ac.uk/~glausp/
 *
 *
 *  Based on fortran code by Cleveland downloaded from:
 *  http://netlib.org/go/lowess.f
 *  original author:
* wsc@research.bell-labs.com Mon Dec 30 16:55 EST 1985
* W. S. Cleveland
* Bell Laboratories
* Murray Hill NJ 07974
 *  
 *  See original documentation in the .cpp file for details.
 * 
 */
#ifndef LOWESS_H
#define LOWESS_H

#include<vector>

using namespace std;

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps,
	double delta, vector<double> &ys, vector<double> &rw, vector<double> &res);

void lowess(const vector<double> &x, const vector<double> &y, double f, long nsteps, vector<double> &ys);

void lowest(const vector<double> &x, const vector<double> &y, double xs, double &ys,
	long nleft, long nright, vector<double> &w,bool userw,  vector<double> &rw, bool &ok);

#endif

