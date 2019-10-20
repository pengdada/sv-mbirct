
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "mbir_ct.h"
#include "MBIRModularDefs.h"
#include "MBIRModularUtils.h"
#include "allocate.h"
#include "initialize.h"


/* Initialize image state */
void Initialize_Image(
	struct Image3D *Image,
	struct CmdLine *cmdline,
	char *ImageReconMask,
	float InitValue,
	float OutsideROIValue)
{
    int j,jz;
    int Nxy = Image->imgparams.Nx * Image->imgparams.Ny;
    int Nz = Image->imgparams.Nz;

    //fprintf(stdout, "\nInitializing Image ... \n");
    
    if(strcmp(cmdline->InitImageDataFile,"NA") == 0) /* Image file not available */
    {
        //GenConstImage(Image, InitValue);               /* generate image with uniform pixel value */
    }
    else {
        //ReadImage3D(cmdline->InitImageDataFile, Image); /* read image which has values in HU units */
        fprintf(stdout, "Note initial image feature not implemented--using constant initial condition\n");
    }

    /* Generate constant image */
    // ***move this up when projector is fixed
    for(jz=0; jz<Nz; jz++)
    for(j=0; j<Nxy; j++)
    if(ImageReconMask[j]==0)
        Image->image[jz][j] = OutsideROIValue;
    else
        Image->image[jz][j] = InitValue;

}


/* Allocate and generate Image Reconstruction mask */
char *GenImageReconMask(struct ImageParams3D *imgparams)
{
    int jx, jy, jz, Nx, Ny, Nz;
    float x_0, y_0, Deltaxy, x, y, yy, ROIRadius, R_sq, R_sq_max;
    char *ImageReconMask;
    
    Nx = imgparams->Nx;
    Ny = imgparams->Ny;
    Nz = imgparams->Nz;
    Deltaxy = imgparams->Deltaxy;
    ROIRadius = imgparams->ROIRadius;
    x_0 = -(Nx-1)*Deltaxy/2;
    y_0 = -(Ny-1)*Deltaxy/2;
    
    ImageReconMask = (char *)get_spc(Ny*Nx,sizeof(char));
    
    if (ROIRadius < 0.0)
    {
        printf("Error in GenImageReconMask : Invalid Value for Radius of Reconstruction \n");
        exit(-1);
    }
    else
    {
        R_sq_max = ROIRadius*ROIRadius;
        for (jy = 0; jy < Ny; jy++)
        {
            y = y_0 + jy*Deltaxy;
            yy = y*y;
            for (jx = 0; jx < Nx; jx++)
            {
                x = x_0 + jx*Deltaxy;
                R_sq = x*x + yy;
                if (R_sq > R_sq_max)
                    ImageReconMask[jy*Nx+jx] = 0;
                else
                    ImageReconMask[jy*Nx+jx] = 1;
            }
        }
    }
    return(ImageReconMask);
}


/* Normalize weights to sum to 1 */
/* Only neighborhood specific */
void NormalizePriorWeights3D(struct ReconParamsQGGMRF3D *reconparams)
{
    double sum;
    
    /* All neighbor weights must sum to one, assuming 8 pt neighborhood */
    sum = 4.0*reconparams->b_nearest + 4.0*reconparams->b_diag + 2.0*reconparams->b_interslice;
    
    reconparams->b_nearest /= sum;
    reconparams->b_diag /= sum;
    reconparams->b_interslice /= sum;
}

/* Wrapper to read in Image, sinogram and reconstruction parameters */
void readSystemParams(
	struct CmdLine *cmdline,
	struct ImageParams3D *imgparams,
	struct SinoParams3DParallel *sinoparams,
	struct ReconParamsQGGMRF3D *reconparams)
{
    //printf("\nReading Image, Sinogram and Reconstruction Parameters ... \n");
    
    if(ReadImageParams3D(cmdline->ImageParamsFile, imgparams)) {
        fprintf(stdout,"Error in reading image parameters \n");
        exit(-1);
    }
    if(ReadSinoParams3DParallel(cmdline->SinoParamsFile, sinoparams)) {
        fprintf(stdout,"Error in reading sinogram parameters \n");
        exit(-1);
    }
    if(ReadReconParamsQGGMRF3D(cmdline->ReconParamsFile ,reconparams)) {
        fprintf(stdout,"Error in reading reconstruction parameters \n");
        exit(-1);
    }
          
    /* Tentatively initialize weights. Remove once this is read in directly from params file */
    NormalizePriorWeights3D(reconparams);
    
    /* Print paramters */
    //printSinoParams3DParallel(sinoparams);
    //printImageParams3D(imgparams);
    //printReconParamsQGGMRF3D(reconparams);
    //fprintf(stdout,"\n");

    /* Determine and SET number of slice index digits in data files */
    int Ndigits = NumSinoSliceDigits(cmdline->SinoDataFile, sinoparams->FirstSliceNumber);
    if(Ndigits==0)
    {
        fprintf(stderr,"Error: Can't read the first data file. Looking for any one of the following:\n");
        for(int i=MBIR_MODULAR_MAX_NUMBER_OF_SLICE_DIGITS; i>0; i--)
            fprintf(stderr,"\t%s_slice%.*d.2Dsinodata\n",cmdline->SinoDataFile, i, sinoparams->FirstSliceNumber);
        exit(-1);
    }
    //printf("Detected %d slice index digits\n",Ndigits);
    sinoparams->NumSliceDigits = Ndigits;
    imgparams->NumSliceDigits = Ndigits;

}



