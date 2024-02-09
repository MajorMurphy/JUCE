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

#include <JuceHeader.h>
#if JUCE_LINK_LIBHEIF_CODE && !JUCE_USING_COREIMAGE_LOADER
#include <libheif/heif.h>
#endif

JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100)

juce::HEIFImageFormat::HEIFImageFormat()
{

}

juce::HEIFImageFormat::~HEIFImageFormat()
{
}

juce::String juce::HEIFImageFormat::getFormatName()
{
	return "High Efficiency Image File Format (HEIF)";
}

bool juce::HEIFImageFormat::usesFileExtension(const File& file)
{
	return file.hasFileExtension("heif;heic");
}

bool juce::HEIFImageFormat::canUnderstand([[maybe_unused]] InputStream& in)
{
#if JUCE_USING_COREIMAGE_LOADER
    in.readByte();
    in.readByte();
    in.readByte();
    in.readByte();
    in.readByte();
    uint32 ftpy = in.readInt();
    uint32 heic = in.readInt();
    return heic == 6515045 && ftpy == 1752201588;

#elif JUCE_LINK_LIBHEIF_CODE
	bool canUnderstand = false;

	juce::Image decodedImage;
	juce::MemoryBlock encodedImageData(in.getNumBytesRemaining());
	in.read(encodedImageData.getData(), encodedImageData.getSize());

	heif_context* ctx = heif_context_alloc();
	auto readResult = heif_context_read_from_memory_without_copy(ctx, encodedImageData.getData(), encodedImageData.getSize(), nullptr);

	if (readResult.code == heif_error_code::heif_error_Ok)
		canUnderstand = true;

	// clean up resources
	heif_context_free(ctx);

	return canUnderstand;
#else
	jassertfalse;
	return false;
#endif
}

inline void ABGRtoARGB(juce::uint32* x)
{	
	// Source is in format: 0xAABBGGRR
	*x = (*x & 0xFF00FF00) |		
		((*x & 0x00FF0000) >> 16) | //______RR
		((*x & 0x000000FF) << 16);  //__BB____
	// Return value is in format:  0xAARRGGBB
}

#if JUCE_USING_COREIMAGE_LOADER
 Image juce_loadWithCoreImage (InputStream&);
#endif

juce::Image juce::HEIFImageFormat::decodeImage(juce::InputStream& in)
{
#if JUCE_USING_COREIMAGE_LOADER
    return juce_loadWithCoreImage (in);
#elif JUCE_LINK_LIBHEIF_CODE
	juce::Image decodedImage;
	juce::MemoryBlock encodedImageData(in.getNumBytesRemaining());
	in.read(encodedImageData.getData(), encodedImageData.getSize());

	heif_context* ctx = heif_context_alloc();
	heif_context_read_from_memory_without_copy(ctx, encodedImageData.getData(), encodedImageData.getSize(), nullptr);

	// get a handle to the primary image
	heif_image_handle* handle = nullptr;
	heif_context_get_primary_image_handle(ctx, &handle);

	auto width = heif_image_handle_get_width(handle);
	auto height = heif_image_handle_get_height(handle);
	auto hasAlpha = heif_image_handle_has_alpha_channel(handle);

	// decode the image and convert colorspace to RGB, saved as 24bit interleaved
	heif_image* encodedImage = nullptr;
	heif_decode_image(handle, &encodedImage, heif_colorspace_RGB, hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB, nullptr);
    if(!encodedImage)
    {
        jassertfalse;
        return Image();
    }

	int stride = 0;
	const uint8_t* data = heif_image_get_plane_readonly(encodedImage, heif_channel_interleaved, &stride);
    if(stride<=0)
    {
        jassertfalse;
        return Image();
    }

	decodedImage = Image(hasAlpha ? Image::ARGB : Image::RGB, width, height, false);
	Image::BitmapData bmp(decodedImage, Image::BitmapData::writeOnly);
	for (int y = 0; y < height; y++)
	{
		auto linePtr = data + (y * stride);
		if (hasAlpha)
		{
            if(bmp.lineStride==stride)
            {
                memcpy(bmp.getLinePointer(y), linePtr, stride);
                for (int x = 0; x < width; x++)
                {
                    ABGRtoARGB((uint32*)bmp.getPixelPointer(x, y));
                }
            }
            else jassertfalse;
		}
		else
		{			
			for (int x = 0; x < width; x++)
			{
				auto dest = bmp.getPixelPointer(x, y);
				auto src = linePtr + x*3;
				dest[0] = src[2];
				dest[1] = src[1];
				dest[2] = src[0];
			}
		}
	}

	// clean up resources
	heif_image_release(encodedImage);
	heif_image_handle_release(handle);
	heif_context_free(ctx);
	return decodedImage;
#else 
	jassertfalse;
	return Image();
#endif

}

bool juce::HEIFImageFormat::writeImageToStream(const Image&, OutputStream&)
{
	jassertfalse;
	return false;
}


JUCE_END_IGNORE_WARNINGS_MSVC
