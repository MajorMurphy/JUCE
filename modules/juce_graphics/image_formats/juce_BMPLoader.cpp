/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

    //==============================================================================
    class BMPLoader
    {
    public:
        BMPLoader(InputStream& in)
        {
            BMPImageFormat imageFmt;
            image = imageFmt.decodeImage(in);
        }

        Image image;

    private:


        JUCE_DECLARE_NON_COPYABLE(BMPLoader)
    };





    //==============================================================================
    BMPImageFormat::BMPImageFormat() {}
    BMPImageFormat::~BMPImageFormat() {}

    String BMPImageFormat::getFormatName() { return "BMP"; }
    bool BMPImageFormat::usesFileExtension(const File& f) { return f.hasFileExtension("bmp"); }

    bool BMPImageFormat::canUnderstand(InputStream& in)
    {
        BitmapFileHeader header;
        BitmapInfoHeader info;

        if (in.read(&header, sizeof(header)) != sizeof(header))
            return false;

        if (header.type != 0x4d42)
            // first bytes expected to be 'BM'
            return false;
        
        if (in.read(&info, sizeof(info)) != sizeof(info))
            return false;

        if (info.compression != 0)
            // compression not supported here
            return false;

        if (info.bitCount != 24)
            // only 8-bit RGB supported here
            return false;


        if (info.width <= 0 || info.height <= 0)
            return false;

        return true;

    }

    Image BMPImageFormat::decodeImage(InputStream& in)
    {
        Image img;
        BitmapFileHeader header;
        BitmapInfoHeader info;
        if (in.read(&header, sizeof(header)) != sizeof(header))
            return img;

        if (header.type != 0x4d42)
            // first bytes expected to be 'BM'
            return img;

        if (in.read(&info, sizeof(info)) != sizeof(info))
            return img;

        img = Image(Image::RGB, info.width, abs(info.height), true);

        auto stride = ((((info.width * info.bitCount) + 31) & ~31) >> 3);
        info.imageSize = abs(info.height) * stride;

        MemoryBlock scanLine(stride);

        for (auto y = 0; y < abs(info.height); y++)
        {
            auto imageData = scanLine.getData();
            if (in.read(imageData, stride) != stride)
            {
                jassertfalse;
                return img;
            }
            
            for (auto x = 0; x < info.width; x++)
            {
                auto pixelPtr = (uint8_t*)imageData + x * info.bitCount / 8;
                img.setPixelAt(x,
                    info.height < 0 ? y : abs(info.height) - y -1,
                    Colour(*(pixelPtr), *(pixelPtr + 1), *pixelPtr + 2));
            }
        }
        return img;
    }

    bool BMPImageFormat::writeImageToStream(const Image& /*sourceImage*/, OutputStream& /*destStream*/)
    {
        // Not yet implemented
        jassertfalse;
        return false;
    }







} // namespace juce
