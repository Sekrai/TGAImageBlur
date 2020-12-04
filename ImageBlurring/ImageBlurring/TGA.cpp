#include "TGA.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

TGA::TGA(const char* aPath)
{
	std::fstream hFile(aPath, std::ios::in | std::ios::binary);
	if (!hFile.is_open()) { throw std::invalid_argument("File Not Found."); }

	std::uint8_t header[18] = { 0 };
	std::vector<std::uint8_t> ImageData;
	static std::uint8_t DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static std::uint8_t IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

	hFile.read(reinterpret_cast<char*>(&header), sizeof(header));

	if (!std::memcmp(DeCompressed, &header, sizeof(DeCompressed)))
	{
		myBitsPerPixel = header[16];
		myWidth = header[13] * 256 + header[12];
		myHeight = header[15] * 256 + header[14];
		mySize = ((myWidth * myBitsPerPixel + 31) / 32) * 4 * myHeight;
		myTrueColour = header[2];
		myImageDescriptor = header[17];

		if ((myBitsPerPixel != 24) && (myBitsPerPixel != 32))
		{
			hFile.close();
			throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit Image.");
		}

		ImageData.resize(mySize);
		myImageCompressedFlag = false;
		hFile.read(reinterpret_cast<char*>(ImageData.data()), mySize);
	}
	else if (!std::memcmp(IsCompressed, &header, sizeof(IsCompressed)))
	{
		myBitsPerPixel = header[16];
		myWidth = header[13] * 256 + header[12];
		myHeight = header[15] * 256 + header[14];
		mySize = ((myWidth * myBitsPerPixel + 31) / 32) * 4 * myHeight;

		if ((myBitsPerPixel != 24) && (myBitsPerPixel != 32))
		{
			hFile.close();
			throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit Image.");
		}

		PixelInfo Pixel = { 0 };
		int CurrentByte = 0;
		std::size_t CurrentPixel = 0;
		myImageCompressedFlag = true;
		std::uint8_t ChunkHeader = { 0 };
		int BytesPerPixel = (myBitsPerPixel / 8);
		ImageData.resize(myWidth * myHeight * sizeof(PixelInfo));

		do
		{
			hFile.read(reinterpret_cast<char*>(&ChunkHeader), sizeof(ChunkHeader));

			if (ChunkHeader < 128)
			{
				++ChunkHeader;
				for (int i = 0; i < ChunkHeader; ++i, ++CurrentPixel)
				{
					hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

					ImageData[CurrentByte++] = Pixel.B;
					ImageData[CurrentByte++] = Pixel.G;
					ImageData[CurrentByte++] = Pixel.R;
					if (myBitsPerPixel > 24) ImageData[CurrentByte++] = Pixel.A;
				}
			}
			else
			{
				ChunkHeader -= 127;
				hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

				for (int i = 0; i < ChunkHeader; ++i, ++CurrentPixel)
				{
					ImageData[CurrentByte++] = Pixel.B;
					ImageData[CurrentByte++] = Pixel.G;
					ImageData[CurrentByte++] = Pixel.R;
					if (myBitsPerPixel > 24) ImageData[CurrentByte++] = Pixel.A;
				}
			}
		} while (CurrentPixel < (myWidth * myHeight));
	}
	else
	{
		hFile.close();
		throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit TGA File.");
	}

	hFile.close();
	myPixels = ImageData;
}

TGA::~TGA()
{

}


bool TGA::Save(const char* aPath)
{
	std::ofstream tempFile(aPath, std::ios::out | std::ios::binary);
	if (!tempFile) return false;

	// The image header
	std::uint8_t header[18] = { 0 };
	header[2] = myTrueColour;
	header[12] = myWidth & 0xFF;
	header[13] = (myWidth >> 8) & 0xFF;
	header[14] = myHeight & 0xFF;
	header[15] = (myHeight >> 8) & 0xFF;
	header[16] = myBitsPerPixel;
	header[17] = myImageDescriptor;

	tempFile.write((const char*)header, 18);

	tempFile.write(reinterpret_cast<char*>(myPixels.data()), mySize);

	static const char footer[26] =
		"\0\0\0\0"  // no extension area
		"\0\0\0\0"  // no developer directory
		"TRUEVISION-XFILE"  // yep, this is a TGA file
		".";
	tempFile.write(footer, 26);

	tempFile.close();
	return true;

}

void TGA::Blur(const double& aRadius)						//Runs the different Box Blurs
{
	std::vector<int> tempBoxes = CalculateBoxes(aRadius, 3);

	for (int i = 0; i < tempBoxes.size(); i++)
	{
		HorizontalBoxBlur((tempBoxes[i] - 1) / 2);
		VerticalBoxBlur((tempBoxes[i] - 1) / 2);
	}
}

void TGA::GaussianBlur(const double& aRadius)																			  //2:10~ min for a max blur
{
	float rs = ceil(aRadius * 2.57);
	for (int i = 0; i < myHeight; ++i)
	{
		for (int j = 0; j < myWidth; ++j)
		{
			double r = 0, g = 0, b = 0;
			double count = 0;


			for (int iy = i - rs; iy < i + rs + 1; ++iy)
			{
				for (int ix = j - rs; ix < j + rs + 1; ++ix)
				{
					auto x = std::min(static_cast<int>(myWidth) - 1, std::max(0, ix));
					auto y = std::min(static_cast<int>(myHeight) - 1, std::max(0, iy));

					auto dsq = ((ix - j) * (ix - j)) + ((iy - i) * (iy - i));
					auto wght = std::exp(-dsq / (2.0 * aRadius * aRadius)) / (M_PI * 2.0 * aRadius * aRadius);

					PixelInfo* pixel = GetPixel(x, y);

					r += pixel->R * wght;
					g += pixel->G * wght;
					b += pixel->B * wght;
					count += wght;
				}
			}

			PixelInfo* pixel = GetPixel(j, i);
			pixel->R = std::round(r / count);
			pixel->G = std::round(g / count);
			pixel->B = std::round(b / count);
		}
	}
}

void TGA::BoxBlur(const double& aRadius)																				   //45~ seconds for a max blur
{
	double tempVal = (aRadius + aRadius + 1) * (aRadius + aRadius + 1);
	double r = 0, g = 0, b = 0;

	for (int i = 0; i < myHeight; i++)
	{
		for (int j = 0; j < myWidth; j++)
		{
			for (int iy = i - aRadius; iy < i + aRadius + 1; iy++)
			{
				for (int ix = j - aRadius; ix < j + aRadius + 1; ix++)
				{
					auto x = std::min(static_cast<int>(myWidth) - 1, std::max(0, ix));
					auto y = std::min(static_cast<int>(myHeight) - 1, std::max(0, iy));

					PixelInfo* pixel = GetPixel(x, y);

					r += pixel->R;
					g += pixel->G;
					b += pixel->B;
				}
			}

			PixelInfo* pixel = GetPixel(j, i);

			pixel->R = std::round(r / tempVal);
			pixel->G = std::round(g / tempVal);
			pixel->B = std::round(b / tempVal);
			r = 0;
			g = 0;
			b = 0;
		}
	}
}

void TGA::HorizontalBoxBlur(const double& aRadius)																			//4~ seconds for a max blur
{
	double tempVal = (aRadius + aRadius + 1);
	double r = 0, g = 0, b = 0;

	for (int i = 0; i < myHeight; i++)
	{
		for (int j = 0; j < myWidth; j++)
		{

			for (int ix = j - aRadius; ix < j + aRadius + 1; ix++)
			{
				auto x = std::min(static_cast<int>(myWidth) - 1, std::max(0, ix));

				PixelInfo* pixel = GetPixel(x, i);

				r += pixel->R;
				g += pixel->G;
				b += pixel->B;
			}


			PixelInfo* pixel = GetPixel(j, i);

			pixel->R = std::round(r / tempVal);
			pixel->G = std::round(g / tempVal);
			pixel->B = std::round(b / tempVal);
			r = 0;
			g = 0;
			b = 0;
		}
	}
}

void TGA::VerticalBoxBlur(const double& aRadius)																			//4~ seconds for a max blur
{
	double tempVal = (aRadius + aRadius + 1);
	double r = 0, g = 0, b = 0;

	for (int i = 0; i < myHeight; i++)
	{
		for (int j = 0; j < myWidth; j++)
		{
			for (int iy = i - aRadius; iy < i + aRadius + 1; iy++)
			{
				auto y = std::min(static_cast<int>(myHeight) - 1, std::max(0, iy));

				PixelInfo* pixel = GetPixel(j, y);

				r += pixel->R;
				g += pixel->G;
				b += pixel->B;

			}

			PixelInfo* pixel = GetPixel(j, i);

			pixel->R = std::round(r / tempVal);
			pixel->G = std::round(g / tempVal);
			pixel->B = std::round(b / tempVal);
			r = 0;
			g = 0;
			b = 0;
		}
	}
}


std::vector<int> TGA::CalculateBoxes(const int& aSigma, const int& aDepth)
{
	double wIdeal = std::sqrt((12 * aSigma * aSigma / aDepth) + 1);
	int wl = std::floor(wIdeal);

	if (wl % 2 == 0)
	{
		wl--;
	}

	int wu = wl + 2;

	double mIdeal = (12 * aSigma * aSigma - aDepth * wl * wl - 4 * aDepth * wl - 3 * aDepth) / (-4 * wl - 4);
	double m = std::round(mIdeal);

	std::vector<int> sizes;

	for (int i = 0; i < aDepth; i++)
	{
		sizes.push_back(i < m ? wl : wu);
	}

	return sizes;
}




PixelInfo* TGA::GetPixel(uint32_t x, uint32_t y)
{
	PixelInfo* temp = reinterpret_cast<PixelInfo*>(myPixels.data());
	return &temp[(myHeight - 1 - y) * myWidth + x];
}