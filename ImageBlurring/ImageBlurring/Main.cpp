#include <iostream>
#include "TGA.h"


int main(int argc, char* argv[])
{
	std::cout << "These are the specified Arguments:" << std::endl;


	for (int  i = 1; i < argc; i++)
	{
		std::cout << argv[i] << std::endl;
	}

	TGA tempImage = TGA("C:/Users/Andre/source/repos/ImageBlurring/JustRight.tga");


	tempImage.Blur(5.0);


	if (tempImage.Save("C:/Users/Andre/source/repos/ImageBlurring/BlurredImage.tga") == true)
	{
		std::cout << "Output Image open" << std::endl;
	}

	int temp = 0;
	std::cin >> temp;
}