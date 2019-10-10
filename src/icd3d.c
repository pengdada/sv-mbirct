
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "MBIRModularDefs.h"
#include "MBIRModularUtils.h"
#include "icd3d.h"

	
float ICDStep3D(struct ReconParamsQGGMRF3D reconparams, float THETA1, float THETA2,float tempV, float *neighbors,float pow_sigmaX_p,float pow_sigmaX_q,float pow_T_qmp)	
{
	int i, n, Nxy, NViewsTimesNChannels, XYPixelIndex, SliceIndex;
	float UpdatedVoxelValue;
    	/*
    	float adjustedTheta1=THETA1;
    	float adjustedTheta2=THETA2;
   	*/
    	/* theta1 and theta2 must be further adjusted according to Prior Model */
    	/* Step can be skipped if merely ML estimation (no prior model) is followed rather than MAP estimation */
    	float ratio=QGGMRF3D_UpdateICDParams(reconparams,tempV,neighbors,THETA1,THETA2,pow_sigmaX_p,pow_sigmaX_q,pow_T_qmp);
	
    	/* Calculate Updated Pixel Value */
    	UpdatedVoxelValue = tempV - ratio ;
    
	return UpdatedVoxelValue;
}

/* ICD update with the QGGMRF prior model */
/* Prior and neighborhood specific */
float QGGMRF3D_UpdateICDParams(struct ReconParamsQGGMRF3D reconparams,float tempV, float *neighbors,float THETA1,float THETA2,float pow_sigmaX_p,float pow_sigmaX_q,float pow_T_qmp)
{
    int j; /* Neighbor relative position to Pixel being updated */
    float sum1_Nearest=0, sum1_Diag=0, sum1_Interslice=0; /* for theta1 calculation */
    float sum2_Nearest=0, sum2_Diag=0, sum2_Interslice=0; /* for theta2 calculation */
    float b_nearest, b_diag, b_interslice;
    
    b_nearest=reconparams.b_nearest;
    b_diag=reconparams.b_diag;
    b_interslice = reconparams.b_interslice;

    float delta[10];
    float SurrogateCoeff[10];
    #pragma vector aligned											
    for (j = 0; j < 10; j++)
        delta[j] = tempV - neighbors[j];
    
    for (j = 0; j < 10; j++)
        SurrogateCoeff[j] = QGGMRF_SurrogateCoeff(delta[j],reconparams,pow_sigmaX_p,pow_sigmaX_q,pow_T_qmp);
    
    #pragma vector aligned											
    for (j = 0; j < 10; j++)
    {        
        if (j < 4)
        {
            sum1_Nearest += (SurrogateCoeff[j] * delta[j]);
            sum2_Nearest += SurrogateCoeff[j];
        }
        else if (j >= 4 && j<8)
        {
            sum1_Diag += (SurrogateCoeff[j] * delta[j]);
            sum2_Diag += SurrogateCoeff[j];
        }
        else
        {
            sum1_Interslice += (SurrogateCoeff[j] * delta[j]);
            sum2_Interslice += SurrogateCoeff[j];
        }        
        
    }
    
    THETA1 +=  (b_nearest * sum1_Nearest + b_diag * sum1_Diag + b_interslice * sum1_Interslice) ;
    THETA2 +=  (b_nearest * sum2_Nearest + b_diag * sum2_Diag + b_interslice * sum2_Interslice) ;
    return THETA1/THETA2;
    
}


/* the potential function of the QGGMRF prior model.  p << q <= 2 */
float QGGMRF_Potential(float delta, struct ReconParamsQGGMRF3D *reconparams)
{
    float p, q, T, SigmaX;
    float temp, GGMRF_Pot;
    
    p = reconparams->p;
    q = reconparams->q;
    T = reconparams->T;
    SigmaX = reconparams->SigmaX;
    
    GGMRF_Pot = pow(fabs(delta),p)/(p*pow(SigmaX,p));
    temp = pow(fabs(delta/(T*SigmaX)), q-p);
    
    return ( GGMRF_Pot * temp/(1.0+temp) );
}

/* Quadratic Surrogate Function for the log(prior model) */
/* For a given convex potential function rho(delta) ... */
/* The surrogate function defined about a point "delta_p", Q(delta ; delta_p), is given by ... */
/* Q(delta ; delta_p) = a(delta_p) * (delta^2/2), where the coefficient a(delta_p) is ... */
/* a(delta_p) = [ rho'(delta_p)/delta_p ]   ... */
/* for the case delta_current is Non-Zero and rho' is the 1st derivative of the potential function */
/* Return this coefficient a(delta_p) */
/* Prior-model specific, independent of neighborhood */

float QGGMRF_SurrogateCoeff(float delta, struct ReconParamsQGGMRF3D reconparams,float pow_sigmaX_p,float pow_sigmaX_q,float pow_T_qmp)
{
    float p, q, T, SigmaX, qmp;
    float num, denom, temp;
    float fabs_delta;
    
    p = reconparams.p;
    q = reconparams.q;
    T = reconparams.T;
    SigmaX = reconparams.SigmaX;
    qmp = q - p;
    fabs_delta=(float)fabs(delta);
    
    /* Refer to Chapter 7, MBIR Textbook by Prof Bouman, Page 151 */
    /* Table on Quadratic surrogate functions for different prior models */
    
    if (delta == 0.0)
    return 2.0/( p*pow_sigmaX_q*pow_T_qmp ) ; /* rho"(0) */
    
    temp = powf(fabs_delta/(T*SigmaX), qmp);
    num = (q/p + temp) * powf(fabs_delta,p-2) * temp;
    denom = pow_sigmaX_p * (1.0+temp) * (1.0+temp);
    
    return num/denom;
}


void ExtractNeighbors3D(
	float *neighbors,
	int jx,
	int jy,
	float *image,
	struct ImageParams3D imgparams)
{
    int plusx, minusx, plusy, minusy;
    int Nx,Ny;
    //int jz,Nz;
    
    Nx = imgparams.Nx;
    Ny = imgparams.Ny;
    //Nz = imgparams.Nz;
    
    plusx = jx + 1;
    plusx = ((plusx < Nx) ? plusx : 0);
    minusx = jx - 1;
    minusx = ((minusx < 0) ? (Nx-1) : minusx);
    plusy = jy + 1;
    plusy = ((plusy < Ny) ? plusy : 0);
    minusy = jy - 1;
    minusy = ((minusy < 0) ? (Ny-1) : minusy);
    
    neighbors[0] = image[jy*Nx+plusx];
    neighbors[1] = image[jy*Nx+minusx];
    neighbors[2] = image[plusy*Nx+jx];
    neighbors[3] = image[minusy*Nx+jx];

    neighbors[4] = image[plusy*Nx+plusx];
    neighbors[5] = image[plusy*Nx+minusx];
    neighbors[6] = image[minusy*Nx+plusx];
    neighbors[7] = image[minusy*Nx+minusx];
}


