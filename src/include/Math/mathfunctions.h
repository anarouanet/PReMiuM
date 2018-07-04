/// \file mathfunctions.h
/// \author David Hastie
/// \date 19 Mar 2012
/// \brief Header file to define distributions

/// \note (C) Copyright David Hastie and Silvia Liverani, 2012.

/// PReMiuM++ is free software; you can redistribute it and/or modify it under the
/// terms of the GNU Lesser General Public License as published by the Free Software
/// Foundation; either version 3 of the License, or (at your option) any later
/// version.

/// PReMiuM++ is distributed in the hope that it will be useful, but WITHOUT ANY
/// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/// PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

/// You should have received a copy of the GNU Lesser General Public License
/// along with PReMiuM++ in the documentation directory. If not, see
/// <http://www.gnu.org/licenses/>.

/// The external linear algebra library Eigen, parts of which are included  in the
/// lib directory is released under the LGPL3+ licence. See comments in file headers
/// for details.

/// The Boost C++ header library, parts of which are included in the  lib directory
/// is released under the Boost Software Licence, Version 1.0, a copy  of which is
/// included in the documentation directory.

/// Version 3.1.3 edited by Rob Johnson, March 2017


#ifndef MATHFUNCTIONS_H_
#define MATHFUNCTIONS_H_

#include<cmath>
#include <Eigen/Eigen>
#include<iostream>
#include<fstream>
#include <Eigen/Dense>
#include<boost/math/special_functions/gamma.hpp>
#include <armadillo>
#include<string>

using std::ifstream;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;
using namespace boost::math::constants;
using namespace Eigen;
using namespace arma;

using boost::math::lgamma;

double logMultivarGammaFn(const double& x,const unsigned int& p){

	double out;
	out = 0.25*(double)(p*(p-1))*log(pi<double>());
	for(unsigned int i=1;i<=p;i++){
		out += lgamma(x+(1.0-(double)i)/2.0);
	}
	return out;
}

double logit(const double& lambda){
	return 1.0/(1.0+exp(-lambda));
}

//RJ matrix (fast) inversion function
 void invert(MatrixXd& invSigma,MatrixXd& Sigma,const unsigned int dimBlock, double noise){
	if(dimBlock>0){
		int dimSigma = Sigma.rows();
		int nBlocks = dimSigma/dimBlock;
		MatrixXd C;
		C = Sigma.block(0, 0, dimBlock, dimBlock);
		for(int i=0;i<dimBlock;i++)
			C(i,i) = C(i,i) - noise;
		double invNoise = 1.0/noise;
		MatrixXd E;
        E = nBlocks*C;
        for(int i=0;i<dimBlock;i++)
            E(i,i) = E(i,i) + noise;
		E = - invNoise * C * E.inverse();
        invSigma = E.replicate(nBlocks,nBlocks);
        for(int i=0;i<dimSigma;i++)
            invSigma(i,i) = invSigma(i,i) + invNoise;
	}else{
		invSigma = Sigma.inverse();
	}
}
//RJ GP covariance function
void GP_cov(MatrixXd& Mat,std::vector<double> L,std::vector<double> times,const unsigned int dimBlock, const string& kernel){
	double a;
  	int i,j;
  	int nTimes = times.size();

  	if(kernel.compare("SQexponential")==0){
  	  double eL0 = exp(L[0]);
  	  double eL1 = exp(L[1])*2.0;
  	  double eL2 = exp(L[2]);

  	  if(dimBlock<0){
  	    int nBlocks = nTimes/dimBlock;
  	    MatrixXd unit;
  	    unit.resize(dimBlock,dimBlock);
  	    unit.fill(0.0);

  	    for(i=1;i<dimBlock;i++){
  	      for(j=0;j<i;j++){
  	        a=-(times[i]-times[j])*(times[i]-times[j])/eL1;
  	        unit(i,j)=eL0*std::exp(a);
  	      }
  	    }
  	    unit = unit + unit.transpose() + eL0*MatrixXd::Identity(dimBlock, dimBlock);
  	    Mat = unit.replicate(nBlocks,nBlocks);
  	    for(i=0;i<nTimes;i++) Mat(i,i) = Mat(i,i) + eL2;
  	  }else{

  	    for(i=1;i<nTimes;i++){
  	      for(j=0;j<i;j++){
  	        a=-(times[i]-times[j])*(times[i]-times[j])/eL1;
  	        Mat(i,j)=eL0*std::exp(a);
  	      }
  	    }
  	    Mat = Mat + Mat.transpose();
  	    for(int i=0;i<nTimes;i++)
  	      Mat(i,i) = Mat(i,i) + eL0+eL2;
  	  }
  	}else{
  	  //[sigma_b^2 + sigma_v^2(t-l)(t-l)]^2
  	  double eL0 = exp(L[0]); //sigma_b^2
  	  double eL1 = exp(L[1]); //sigma_v^2
  	  double eL2 = exp(L[2]); // sigma_e^2
  	  double eL3 = exp(L[3]); // l

  	  for(i=1;i<nTimes;i++){
  	    for(j=0;j<i;j++){
  	      a=eL0+eL1*(times[i]-eL3)*(times[j]-eL3);
  	      Mat(i,j)=a*a;
  	    }
  	  }
  	  Mat = Mat + Mat.transpose();

  	  for(int i=0;i<nTimes;i++){
  	    a=eL0+eL1*(times[i]-eL3)*(times[i]-eL3);
  	    Mat(i,i)=a*a + eL2;
  	  }
  	}

}

//AR to obtain Sigma_permut^{-1} from Sigma^{-1} = Mat
void Permut_cov(MatrixXd& Mat,  const int start_permut, const int length_permut){
  int i,j,ii,jj,ai,aj;
  int nTimes = Mat.rows();
  MatrixXd Mat_permut;
  Mat_permut.setZero(nTimes,nTimes);

  ai=0;
  for(i=0;i<nTimes;i++){

    if(i< start_permut){
      ii=i;
    }else if(i<nTimes-length_permut){
      ii=i+length_permut;
    }else{
      ii=start_permut+ai;
      ai++;
    }

    aj=0;
    for(j=0;j<nTimes;j++){
      if(j< start_permut){
        jj=j;
      }else if(j<nTimes-length_permut){
        jj=j+length_permut;
      }else{
        jj=start_permut+aj;
        aj++;
      }
      Mat_permut(i,j)=Mat(ii,jj);
    }
  }
  Mat=Mat_permut;
}
//AR
double Get_Sigma_inv_GP_cov(MatrixXd& Mat,std::vector<double> L,std::vector<double> times,const unsigned int dimBlock, const std::vector<double> grid, const string& kernel){
  double a,eL0,eL1,eL2,eL3;
  int i,j;
  int nTimes = times.size();
  double det=0;
  //std::fstream fout("file_output.txt", std::ios::in | std::ios::out | std::ios::app);

  Mat.setZero(nTimes,nTimes);

  if(kernel.compare("SQexponential")==0){
     eL0 = exp(L[0]);
     eL1 = exp(L[1])*2.0;
     eL2 = exp(L[2]);

    for(i=1;i<nTimes;i++){
      for(j=0;j<i;j++){
        a=-(times[i]-times[j])*(times[i]-times[j])/eL1;
        Mat(i,j)=eL0*std::exp(a);
      }
    }
    Mat = Mat + Mat.transpose();
    for(int i=0;i<nTimes;i++)
      Mat(i,i) = Mat(i,i) + eL0+eL2;

  }else{

    //[sigma_b^2 + sigma_v^2(t-l)(t-l)]^2
     eL0 = exp(L[0]); //sigma_b^2
     eL1 = exp(L[1]); //sigma_v^2
     eL2 = exp(L[2]); // sigma_e^2
     eL3 = exp(L[3]); // l

    for(i=1;i<nTimes;i++){
      for(j=0;j<i;j++){
        a=eL0+eL1*(times[i]-eL3)*(times[j]-eL3);
        Mat(i,j)=a*a;
      }
    }
    Mat = Mat + Mat.transpose();

    for(int i=0;i<nTimes;i++){
      a=eL0+eL1*(times[i]-eL3)*(times[i]-eL3);
      Mat(i,j)=a*a+eL2;
    }
  }

  if(*std::max_element(std::begin(grid), std::end(grid))==0){
    //MatrixXd L_inv = Mat.llt().solve(MatrixXd::Identity(Mat.rows(), Mat.rows()));
    //det=  log(L_inv.determinant());

    //fout << "det L_inv 1 "<< det << " mat "<<log(Mat.determinant()) <<endl;
    LLT<MatrixXd> lltOfA(Mat); // compute the Cholesky decomposition of A
    MatrixXd L = lltOfA.matrixL();
    det=  2*log(L.determinant());
    //fout << "det L_inv 2 "<< det << " mat "<<log(Mat.determinant()) <<endl;
    Mat = L.inverse().transpose()*L.inverse();

  }else{

    MatrixXd Ktu;
    MatrixXd Kuu;
    Ktu.setZero(nTimes,grid.size());
    Kuu.setZero(grid.size(),grid.size());

    for(i=0;i<nTimes;i++){
      for(j=0;j<grid.size();j++){
        if(kernel.compare("SQexponential")==0){
          a=-(times[i]-grid[j])*(times[i]-grid[j])/eL1;
          Ktu(i,j)=eL0*std::exp(a);
        }else{
          a=eL0+eL1*(times[i]-eL3)*(grid[j]-eL3);
          Ktu(i,j)=a*a;
        }
      }
    }

    for(i=1;i<grid.size();i++){
      for(j=0;j<i;j++){
        if(kernel.compare("SQexponential")==0){
          a=-(grid[i]-grid[j])*(grid[i]-grid[j])/eL1;
          Kuu(i,j)=eL0*std::exp(a);
        }else{
          a=eL0+eL1*(grid[i]-eL3)*(grid[j]-eL3);
          Kuu(i,j)=a*a;
        }
      }
    }
    Kuu = Kuu + Kuu.transpose();

    for(int i=0;i<grid.size();i++){
      if(kernel.compare("SQexponential")==0){
        Kuu(i,i) = Kuu(i,i) + eL0;
      }else{
        for(int i=0;i<grid.size();i++){
          a=eL0+eL1*(grid[i]-eL3)*(grid[i]-eL3);
          Kuu(i,i)=a*a + eL2;
        }
      }
    }


    MatrixXd Qtt=Ktu*Kuu.inverse()*Ktu.transpose();
    MatrixXd Lambda=(Mat.diagonal()-Qtt.diagonal()).asDiagonal();
    MatrixXd Aut=Kuu.llt().matrixL().solve(Ktu.transpose()); //.transpose().solve(Ktu.transpose());
    Mat =Lambda.inverse()-Lambda.inverse()*Aut.transpose()*(MatrixXd::Identity(grid.size(), grid.size())+Aut*Lambda.inverse()*Aut.transpose()).inverse()*Aut*Lambda.inverse();

    det=log((MatrixXd::Identity(grid.size(), grid.size())+Aut*Lambda.inverse()*Aut.transpose()).determinant()*Lambda.determinant());
  }



  return det;
}

//AR Inverse function for block matrix with Woodbury identity
// double Inverse_woodbury(const MatrixXd& M0, const double& log_det_M0, MatrixXd& Mat,std::vector<double> times);
//
// double Inverse_woodbury(const MatrixXd& M0, const double& log_det_M0, MatrixXd& Mat){
//   std::vector<double> times;
//   return Inverse_woodbury( M0,  log_det_M0,  Mat,  times);
// }

double Inverse_woodbury(const MatrixXd& M0, const double& log_det_M0, MatrixXd& Mat,std::vector<double> times){

  int dimSigma = M0.rows();
  double log_DetPrecMat;
  MatrixXd kno;
  MatrixXd Knew;

  double a;
  int i,j;
  int nTimes = Mat.rows();

  i=M0.rows();

  if(i<nTimes){ // add one subject

    Knew.setZero(nTimes-i,nTimes-i);
    kno.setZero(nTimes-i,i);

    for(int i2=i;i2<nTimes;i2++){
      for(j=0;j<i2;j++){
        if(j<i){
          kno(i2-i,j)=Mat(i2,j);
        }else{
          Knew(i2-i,j-i)=Mat(i2,j);
        }
      }
    }

    Knew = Knew + Knew.transpose();
    for(int i2=0;i2<nTimes-i;i2++)
      Knew(i2,i2) = Mat(i+i2,i+i2);

    MatrixXd A(nTimes-i,nTimes-i);
    MatrixXd B(i,nTimes-i);
    A.setZero(nTimes-i,nTimes-i);
    B.setZero(i,nTimes-i);
    B=M0*kno.transpose();
    A=Knew-kno*B;

    if(M0.rows() == Mat.rows()){
      //error
      //foutt << " ERROR "  << M0.rows()  << " versus "<< Mat.rows() <<endl;
    }else{
      log_DetPrecMat=log_det_M0+log(A.determinant());
    }

    Mat.setZero(nTimes,nTimes);
    Mat.topRows(i)<<M0+B*A.inverse()*kno*M0, -B*A.inverse();
    Mat.bottomRows(nTimes-i)<<-A.inverse()*B.transpose(), A.inverse();

  }else{ // remove one subject i>nTimes

    Knew.setZero(i-nTimes,i-nTimes);
    kno.setZero(i-nTimes,nTimes);

    for(int i2=nTimes;i2<i;i2++){
      for(j=0;j<i2;j++){
        if(j<nTimes){
          kno(i2-nTimes,j)=M0(i2,j);
        }else{
          Knew(i2-nTimes,j-nTimes)=M0(i2,j);
        }
      }
    }


    Knew = Knew + Knew.transpose();
    for(int i2=0;i2<i-nTimes;i2++)
      Knew(i2,i2) = M0(nTimes+i2,nTimes+i2);

    MatrixXd U(i,(i-nTimes));
    U << MatrixXd::Zero(nTimes,i-nTimes), MatrixXd::Identity(i-nTimes,i-nTimes);
    MatrixXd V(i-nTimes,i);
    V << -kno, MatrixXd::Zero(i-nTimes,i-nTimes);

    MatrixXd Mnew_inv=M0.inverse(); // To improve

    MatrixXd T=(MatrixXd::Identity(i-nTimes,i-nTimes)+V*Mnew_inv*U);
    MatrixXd M1_inv(i,i);
    M1_inv.setZero(i,i);
    M1_inv=Mnew_inv - Mnew_inv*U*T.inverse()*V*Mnew_inv;

    MatrixXd U1=-V.transpose();
    MatrixXd V1=-U.transpose();

    MatrixXd M2(i,i);
    M2.setZero(i,i);
    MatrixXd T1=(MatrixXd::Identity(i-nTimes,i-nTimes)+V1*M1_inv*U1);
    M2=M1_inv - M1_inv*U1*T1.inverse()*V1*M1_inv;

    Mat.setZero(nTimes,nTimes);
    for(int i2=0;i2<nTimes;i2++){
      for(j=0;j<i2;j++){
        Mat(i2,j)=M2(i2,j);
      }
    }
    Mat = Mat + Mat.transpose();
    for(int i2=0;i2<nTimes;i2++)
      Mat(i2,i2) = M2(i2,i2);

    MatrixXd A(i-nTimes,i-nTimes);
    MatrixXd B(nTimes,i-nTimes);

    A.setZero(i-nTimes,i-nTimes);
    B.setZero(nTimes,i-nTimes);
    B=Mat*kno.transpose();
    A=Knew-kno*B;

    log_DetPrecMat= log_det_M0- log(A.determinant());
  }


  return(log_DetPrecMat);
}

double Inverse_woodbury(const MatrixXd& M0, const double& log_det_M0, MatrixXd& Mat,std::vector<double> times, MatrixXd& M0_inv){

  int dimSigma = M0.rows();
  double log_DetPrecMat;
  MatrixXd kno;
  MatrixXd Knew;

  double a;
  int i,j;
  int nTimes = Mat.rows();

  i=M0.rows();

  if(i<nTimes){ // add one subject

    Knew.setZero(nTimes-i,nTimes-i);
    kno.setZero(nTimes-i,i);

    for(int i2=i;i2<nTimes;i2++){
      for(j=0;j<i2;j++){
        if(j<i){
          kno(i2-i,j)=Mat(i2,j);
        }else{
          Knew(i2-i,j-i)=Mat(i2,j);
        }
      }
    }

    Knew = Knew + Knew.transpose();
    for(int i2=0;i2<nTimes-i;i2++)
      Knew(i2,i2) = Mat(i+i2,i+i2);

    MatrixXd A(nTimes-i,nTimes-i);
    MatrixXd B(i,nTimes-i);
    A.setZero(nTimes-i,nTimes-i);
    B.setZero(i,nTimes-i);
    B=M0*kno.transpose();
    A=Knew-kno*B;

    if(M0.rows() == Mat.rows()){
      //error
      //foutt << " ERROR "  << M0.rows()  << " versus "<< Mat.rows() <<endl;
    }else{
      log_DetPrecMat=log_det_M0+log(A.determinant());
    }

    Mat.setZero(nTimes,nTimes);
    Mat.topRows(i)<<M0+B*A.inverse()*kno*M0, -B*A.inverse();
    Mat.bottomRows(nTimes-i)<<-A.inverse()*B.transpose(), A.inverse();

  }else{ // remove one subject i>nTimes

    Knew.setZero(i-nTimes,i-nTimes);
    kno.setZero(i-nTimes,nTimes);

    for(int i2=nTimes;i2<i;i2++){
      for(j=0;j<i2;j++){
        if(j<nTimes){
          kno(i2-nTimes,j)=M0(i2,j);
        }else{
          Knew(i2-nTimes,j-nTimes)=M0(i2,j);
        }
      }
    }


    Knew = Knew + Knew.transpose();
    for(int i2=0;i2<i-nTimes;i2++)
      Knew(i2,i2) = M0(nTimes+i2,nTimes+i2);

    MatrixXd U(i,(i-nTimes));
    U << MatrixXd::Zero(nTimes,i-nTimes), MatrixXd::Identity(i-nTimes,i-nTimes);
    MatrixXd V(i-nTimes,i);
    V << -kno, MatrixXd::Zero(i-nTimes,i-nTimes);

    MatrixXd Mnew_inv=M0_inv; // To improve

    MatrixXd T=(MatrixXd::Identity(i-nTimes,i-nTimes)+V*Mnew_inv*U);
    MatrixXd M1_inv(i,i);
    M1_inv.setZero(i,i);
    M1_inv=Mnew_inv - Mnew_inv*U*T.inverse()*V*Mnew_inv;

    MatrixXd U1=-V.transpose();
    MatrixXd V1=-U.transpose();

    MatrixXd M2(i,i);
    M2.setZero(i,i);
    MatrixXd T1=(MatrixXd::Identity(i-nTimes,i-nTimes)+V1*M1_inv*U1);
    M2=M1_inv - M1_inv*U1*T1.inverse()*V1*M1_inv;

    Mat.setZero(nTimes,nTimes);
    for(int i2=0;i2<nTimes;i2++){
      for(j=0;j<i2;j++){
        Mat(i2,j)=M2(i2,j);
      }
    }
    Mat = Mat + Mat.transpose();
    for(int i2=0;i2<nTimes;i2++)
      Mat(i2,i2) = M2(i2,i2);

    MatrixXd A(i-nTimes,i-nTimes);
    MatrixXd B(nTimes,i-nTimes);

    A.setZero(i-nTimes,i-nTimes);
    B.setZero(nTimes,i-nTimes);
    B=Mat*kno.transpose();
    A=Knew-kno*B;

    log_DetPrecMat= log_det_M0- log(A.determinant());
  }


  return(log_DetPrecMat);
}


//RJ GP covariance star function
double GP_cov_star(MatrixXd& Mat,std::vector<double> L,std::vector<double> times,std::vector<double> timestar, const string& kernel){

	double a;
  	int i,j;
  	int nTimes = times.size();
  	int nStar = timestar.size();
  	for(i=0;i<nTimes;i++){
    	for(j=0;j<nStar;j++){
    	  if(kernel.compare("SQexponential")==0){
    	    a=-1.0/2.0/exp(L[1])*(times[i]-timestar[j])*(times[i]-timestar[j]);
    	    Mat(i,j)=exp(L[0])*std::exp(a);
    	  }else{
    	    a=exp(L[0])+exp(L[1])*(times[i]-exp(L[3]))*(timestar[j]-exp(L[3]));
    	    Mat(i,j)=a*a ;//+ exp(L[3]);
    	  }
    	}
  	}
  	return 0;
}

#endif /*MATHFUNCTIONS_H_*/