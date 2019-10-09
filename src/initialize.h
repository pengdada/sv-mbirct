#ifndef _INITIALIZE_H_
#define _INITIALIZE_H_

struct CmdLineMBIR{
    char SinoParamsFile[256];
    char ImageParamsFile[256];
    char ReconParamsFile[256];
    char SinoDataFile[256];
    char SinoWeightsFile[256];
    char ReconImageDataFile[256]; /* output */
    char SysMatrixFile[256];
    char InitImageDataFile[256]; /* optional input */
};


void Initialize_Image(struct Image3D *Image, struct CmdLineMBIR *cmdline, char *ImageReconMask, float InitValue, float OutsideROIValue);
char *GenImageReconMask(struct ImageParams3D *imgparams);
void readSystemParams_MBIR(struct CmdLineMBIR *cmdline, struct ImageParams3D *imgparams, struct SinoParams3DParallel *sinoparams, struct ReconParamsQGGMRF3D *reconparams);
void NormalizePriorWeights3D(struct ReconParamsQGGMRF3D *reconparams);
void readCmdLineMBIR(int argc, char *argv[], struct CmdLineMBIR *cmdline);
void PrintCmdLineUsage_MBIR(char *ExecFileName);
int CmdLineHelp_MBIR(char *string);

#endif
