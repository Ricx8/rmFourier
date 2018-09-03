#include "rmFourier.h"

static PF_PixelFloat
*getXY32(PF_EffectWorld &def, int x, int y) {
	return (PF_PixelFloat*)((char*)def.data +
		(y * def.rowbytes) +
		(x * sizeof(PF_PixelFloat)));

}

PF_Err
normalizeImg(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	outP->red = inP->red / siP->rMax;
	outP->green = inP->green / siP->rMax;
	outP->blue = inP->blue / siP->rMax;

	return err;
}

PF_Err
fftShift(
	void *refcon,
	A_long threadNum,
	A_long yL,
	A_long numOfIterations)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long	hHalf = siP->inHeight / 2,
		wHalf = siP->inWidth / 2,
		yL2 = yL + hHalf;

	for (A_long xL = 0; xL < siP->inWidth; xL++) {
		A_long xL2 = xL + wHalf;

		if (xL2 >= siP->inWidth) xL2 = xL2 - siP->inWidth;
		if (yL2 >= siP->inHeight) yL2 = yL2 - siP->inHeight;

		unsigned long srcPPixel = (yL2 * siP->inWidth) + xL2;
		unsigned long dstPPixel = (yL * siP->inWidth) + xL;
		PF_PixelFloat *srcPixel = (PF_PixelFloat*)((char*)siP->copy_worldP->data + (srcPPixel * sizeof(PF_PixelFloat)));
		PF_PixelFloat *dstPixel = (PF_PixelFloat*)((char*)siP->output_worldP->data + (dstPPixel * sizeof(PF_PixelFloat)));
	
		*dstPixel = *srcPixel;
	}

	return err;
}

PF_Err
ifftShift(
	void *refcon,
	A_long threadNum,
	A_long yL,
	A_long numOfIterations)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long	hHalf = siP->inHeight / 2,
			wHalf = siP->inWidth / 2,
			yL2 = yL - hHalf;

	for (A_long xL = 0; xL < siP->inWidth; xL++) {
		A_long xL2 = xL - wHalf;

		if (xL2 < 0) xL2 = xL2 + siP->inWidth;
		if (yL2 < 0) yL2 = yL2 + siP->inHeight;

		unsigned long srcPPixel = (yL2 * siP->inWidth) + xL2;
		unsigned long dstPPixel = (yL * siP->inWidth) + xL;
		PF_PixelFloat *srcPixel = (PF_PixelFloat*)((char*)siP->input_worldP->data + (srcPPixel * sizeof(PF_PixelFloat)));
		PF_PixelFloat *dstPixel = (PF_PixelFloat*)((char*)siP->output_worldP->data + (dstPPixel * sizeof(PF_PixelFloat)));

		*dstPixel = *srcPixel;
	}

	return err;
}

PF_Err
pixelToVector(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	A_long currentIndex = (yL * siP->inWidth) + xL;
	siP->imgVectorR[currentIndex].real(inP->red);
	siP->imgVectorG[currentIndex].real(inP->green);
	siP->imgVectorB[currentIndex].real(inP->blue);

	return err;
}

PF_Err
vectorToPixel(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->inWidth) + xL;

	PF_FpShort finalR, finalG, finalB;

	if (!siP->inverseCB) {
		if (!siP->fftPhase) {
			finalR = log(1 + abs(siP->imgVectorR[currentIndex]));
			finalG = log(1 + abs(siP->imgVectorG[currentIndex]));
			finalB = log(1 + abs(siP->imgVectorB[currentIndex]));

			if (finalR > siP->rMax) siP->rMax = finalR;
			if (finalR > siP->gMax) siP->gMax = finalG;
			if (finalR > siP->bMax) siP->bMax = finalB;
		}
		else {
			finalR = atan2(siP->imgVectorR[currentIndex].imag(), siP->imgVectorR[currentIndex].real());
			finalG = atan2(siP->imgVectorG[currentIndex].imag(), siP->imgVectorG[currentIndex].real());
			finalB = atan2(siP->imgVectorB[currentIndex].imag(), siP->imgVectorB[currentIndex].real());
		}
		
	}
	else {
		finalR = abs(siP->imgVectorR[currentIndex]);
		finalG = abs(siP->imgVectorG[currentIndex]);
		finalB = abs(siP->imgVectorB[currentIndex]);

		if (finalR > siP->rMax) siP->rMax = finalR;
		if (finalR > siP->gMax) siP->gMax = finalG;
		if (finalR > siP->bMax) siP->bMax = finalB;
	}

	outP->alpha = 1;
	outP->red = finalR;
	outP->green = finalG;
	outP->blue = finalB;

	return err;
}

PF_Err
fftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	siP->tmpCount = siP->tmpCount + 1;
	if (threadNum > siP->tmpMax) siP->tmpMax = threadNum;

	std::vector<std::complex<double>> currentRowVecR, currentRowVecG, currentRowVecB;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		currentRowVecR.push_back(siP->imgVectorR[currentIndex]);
		currentRowVecG.push_back(siP->imgVectorG[currentIndex]);
		currentRowVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentRowVecR, siP);
	fft::transform(currentRowVecG, siP);
	fft::transform(currentRowVecB, siP);

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
	}

	return err;
}

PF_Err
fftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	std::vector<std::complex<double>> currentColVecR, currentColVecG, currentColVecB;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentColVecR, siP);
	fft::transform(currentColVecG, siP);
	fft::transform(currentColVecB, siP);

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}

PF_Err
ifftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	siP->tmpCount = siP->tmpCount + 1;
	if (threadNum > siP->tmpMax) siP->tmpMax = threadNum;

	std::vector<std::complex<double>> currentRowVecR, currentRowVecG, currentRowVecB;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double>	invR = exp(imaginaryI * double(pixelPointerAt->red)),   
								invG = exp(imaginaryI * double(pixelPointerAt->green)),
								invB = exp(imaginaryI * double(pixelPointerAt->blue));  

		pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double>	tmpB = exp(pixelPointerAt->blue) - 1;

		invR = std::complex<double>(exp(pixelPointerAt->red) - 1) * invR;
		invG = std::complex<double>(exp(pixelPointerAt->green) - 1) * invG;
		invB = tmpB * invB;

		currentRowVecR.push_back(invR);
		currentRowVecG.push_back(invG);
		currentRowVecB.push_back(invB);
	}

	fft::inverseTransform(currentRowVecR, siP);
	fft::inverseTransform(currentRowVecG, siP);
	fft::inverseTransform(currentRowVecB, siP);

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
	}

	return err;
}

PF_Err
ifftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	std::vector<std::complex<double>> currentColVecR, currentColVecG, currentColVecB;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::inverseTransform(currentColVecR, siP);
	fft::inverseTransform(currentColVecG, siP);
	fft::inverseTransform(currentColVecB, siP);

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}

PF_Err
tmpRender16(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel16 	*inP,
	PF_Pixel16 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	*outP = *inP;

	return err;
}

PF_Err
tmpRender8(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel8 	*inP,
	PF_Pixel8 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	*outP = *inP;

	return err;
}

void preTransform(size_t vSize, void *refcon) {
	const double M_PI = 3.14159265358979323846;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	// Length variables
	size_t n = vSize;//vec.size();

	if ((n & (n - 1)) == 0) {  // If is power of 2
		if (n != siP->expTable.size()) {
			siP->expTable.clear();
			siP->transformType = 1;
			siP->levels = 0;  // Compute levels = floor(log2(n))
			for (size_t temp = n; temp > 1U; temp >>= 1)
				siP->levels++;
			if (static_cast<size_t>(1U) << siP->levels != n)
				throw "Length is not a power of 2";

			// Trignometric table
			for (size_t i = 0; i < n / 2; i++)
				siP->expTable.push_back(std::exp(std::complex<double>(0, -2 * M_PI * i / n)));
		}
	}
	else{ 
		if (n != siP->expTable.size()) {
			siP->transformType = 2;
			siP->m = 1;
			while (siP->m / 2 <= n) {
				if (siP->m > SIZE_MAX / 2)
					throw "Vector too large";
				siP->m *= 2;
			}

			siP->expTable.clear();

			// Trignometric table
			for (size_t i = 0; i < n; i++) {
				unsigned long long temp = static_cast<unsigned long long>(i) * i;
				temp %= static_cast<unsigned long long>(n) * 2;
				double angle = M_PI * temp / n;
				// Less accurate alternative if long long is unavailable: double angle = M_PI * i * i / n;
				siP->expTable.push_back(std::exp(std::complex<double>(0, -angle)));
			}

			siP->bv.resize(siP->m);
			siP->bv[0] = siP->expTable[0];
			for (size_t i = 1; i < n; i++)
				siP->bv[i] = siP->bv[siP->m - i] = std::conj(siP->expTable[i]);

			siP->preBluestein = false;

			// ------------ Trignometric table of convolution ------------
			siP->convExpTable.clear();
			siP->convLevels = 0;  // Compute levels = floor(log2(n))
			for (size_t temp = siP->m; temp > 1U; temp >>= 1)
				siP->convLevels++;
			if (static_cast<size_t>(1U) << siP->convLevels != siP->m)
				throw "Length is not a power of 2";

			// Trignometric table
			for (size_t i = 0; i < siP->m / 2; i++)
				siP->convExpTable.push_back(std::exp(std::complex<double>(0, -2 * M_PI * i / siP->m)));
		}
	}
}