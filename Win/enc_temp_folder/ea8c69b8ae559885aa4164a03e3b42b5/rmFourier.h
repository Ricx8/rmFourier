#pragma once

#ifndef RMFOURIER_H
#define RMFOURIER_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	
	// Define the next two lines in Windows, because an error with thread and VS
	#define INT64_MAX    _I64_MAX
	#define INTMAX_MAX   INT64_MAX

	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "Smart_Utils.h"
#include "AEFX_SuiteHelper.h"
#include "FftComplex.hpp"

#include <string>
#include <vector>
#include <complex>
#include <ratio>
#include <thread>

const std::string   strName = "rmFourier",
					strDescription = "RM skeleton for ctrl plugind";

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


// RM-NOTE: parameter UI order
enum {
	RMFOURIER_INPUT = 0,
	RMFOURIER_INVERSE_FFT,
	RMFOURIER_NUM_PARAMS
};

// RM-NOTE: parameter disk order
enum {
	INVERSE_FFT_DISK_ID = 1,
};

typedef struct {
	PF_ProgPtr							ref;
	PF_SampPB							samp_pb;
	PF_InData							in_data;
	PF_Boolean							no_opB;
	PF_EffectWorld						*tmpOutput;

	bool								inverseCB;
	std::vector<std::complex<double>>	tmpVectorR,
										imgVectorR,
										*imgRedDataVector;////, imgGreenDataVector, imgBlueDataVector, 
										//finalImgGreenDataVector;
	unsigned int						fftState; // 0= Columns, 1= Rows 
	double								rMax;
	A_long								imgWidth, imgHeight;
} rmFourierInfo;


extern "C" {
	DllExport	
	PF_Err 
	EntryPointFunc(	
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra) ;
}

static PF_PixelFloat
*getXY32(PF_EffectWorld &def, int x, int y);

PF_EffectWorld tmpFourier(PF_EffectWorld inWorld);

PF_Err
pushPixelToVector(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
normalizeImg(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
circularShift(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
pixelToVector(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
vectorToPixel(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

void transformRow(std::vector<std::complex<double>> *imgDataVec, A_long row, A_long imgWidth);
void transformColumn(std::vector<std::complex<double>> *imgDataVec, A_long col, A_long imgWidth, A_long imgHeight);

#endif // RMFOURIER_H