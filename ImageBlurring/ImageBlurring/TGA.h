#ifndef TGA_HEADER
#define TGA_HEADER

#include <vector>
#include <fstream>
#include <string>

typedef union PixelInfo
{
    std::uint32_t Colour;
    struct
    {
        std::uint8_t R, G, B, A;
    };
} *PPixelInfo;

class TGA
{
public:
    TGA(const char* aPath);
    ~TGA();

    bool Save(const char* aPath);
    void Blur(const double& aRadius);
    void GaussianBlur(const double &aRadius);
    void BoxBlur(const double& aRadius);
    void HorizontalBoxBlur(const double& aRadius);
    void VerticalBoxBlur(const double& aRadius);

    std::vector<int> CalculateBoxes(const int& aSigma, const int& aDepth);

    PixelInfo* GetPixel(uint32_t x, uint32_t y);
    std::vector<std::uint8_t> GetPixels() { return myPixels; }
    std::uint32_t GetWidth() const { return myWidth; }
    std::uint32_t GetHeight() const { return myHeight; }
    bool HasAlphaChannel() { return myBitsPerPixel == 32; }
private:
    std::vector<std::uint8_t> myPixels;
    bool myImageCompressedFlag;
    std::uint32_t myWidth, myHeight, mySize, myBitsPerPixel, myTrueColour, myImageDescriptor;
};


#endif