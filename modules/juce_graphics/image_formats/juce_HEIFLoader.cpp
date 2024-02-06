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
#include <libheif/heif.h>


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

bool juce::HEIFImageFormat::canUnderstand(InputStream& in)
{
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
}

inline void ABGRtoARGB(juce::uint32* x)
{	
	// Source is in format: 0xAABBGGRR
	*x = (*x & 0xFF00FF00) |		
		((*x & 0x00FF0000) >> 16) | //______RR
		((*x & 0x000000FF) << 16);  //__BB____
	// Return value is in format:  0xAARRGGBB
}

juce::Image juce::HEIFImageFormat::decodeImage(juce::InputStream& in)
{
	juce::Image decodedImage;
	juce::MemoryBlock encodedImageData(in.getNumBytesRemaining());
	in.read(encodedImageData.getData(), encodedImageData.getSize());

	heif_context* ctx = heif_context_alloc();
	heif_context_read_from_memory_without_copy(ctx, encodedImageData.getData(), encodedImageData.getSize(), nullptr);

	// get a handle to the primary image
	heif_image_handle* handle;
	heif_context_get_primary_image_handle(ctx, &handle);

	auto width = heif_image_handle_get_width(handle);
	auto height = heif_image_handle_get_height(handle);
	auto hasAlpha = heif_image_handle_has_alpha_channel(handle);

	// decode the image and convert colorspace to RGB, saved as 24bit interleaved
	heif_image* encodedImage;
	heif_decode_image(handle, &encodedImage, heif_colorspace_RGB, hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB, nullptr);

	int stride;
	const uint8_t* data = heif_image_get_plane_readonly(encodedImage, heif_channel_interleaved, &stride);


	decodedImage = Image(hasAlpha ? Image::ARGB : Image::RGB, width, height, false);
	Image::BitmapData bmp(decodedImage, Image::BitmapData::writeOnly);
	for (int y = 0; y < height; y++)
	{
		auto linePtr = data + (y * stride);
		if (hasAlpha)
		{
			memcpy_s(bmp.getLinePointer(y), bmp.lineStride, linePtr, stride);
			for (int x = 0; x < width; x++)
			{
				ABGRtoARGB((uint32*)bmp.getPixelPointer(x, y));
			}
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
}

bool juce::HEIFImageFormat::writeImageToStream(const Image&, OutputStream&)
{
	return false;
}


