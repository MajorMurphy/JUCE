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
/**
    Base-class for codecs that can read and write image file formats such
    as PNG, JPEG, etc.

    This class also contains static methods to make it easy to load images
    from files, streams or from memory.

    @see Image, ImageCache

    @tags{Graphics}
*/
class JUCE_API  ImageFileFormat
{
protected:
    //==============================================================================
    /** Creates an ImageFormat. */
    ImageFileFormat() = default;

public:
    /** Destructor. */
    virtual ~ImageFileFormat() = default;

    //==============================================================================
    /** Returns a description of this file format.

        E.g. "JPEG", "PNG"
    */
    virtual String getFormatName() = 0;

    /** Returns true if the given stream seems to contain data that this format understands.

        The format class should only read the first few bytes of the stream and sniff
        for header bytes that it understands.

        Note that this will advance the stream and leave it in a new position, so if you're
        planning on re-using it, you may want to rewind it after calling this method.

        @see decodeImage
    */
    virtual bool canUnderstand (InputStream& input) = 0;

    /** Returns true if this format uses the file extension of the given file. */
    virtual bool usesFileExtension (const File& possibleFile) = 0;

    /** Tries to decode and return an image from the given stream.

        This will be called for an image format after calling its canUnderStand() method
        to see if it can handle the stream.

        @param input    the stream to read the data from. The stream will be positioned
                        at the start of the image data (but this may not necessarily
                        be position 0)
        @returns        the image that was decoded, or an invalid image if it fails.
        @see loadFrom
    */
    virtual Image decodeImage (InputStream& input) = 0;

    //==============================================================================
    /** Attempts to write an image to a stream.

        To specify extra information like encoding quality, there will be appropriate parameters
        in the subclasses of the specific file types.

        @returns        true if it nothing went wrong.
    */
    virtual bool writeImageToStream (const Image& sourceImage,
                                     OutputStream& destStream) = 0;

    //==============================================================================
    /** Tries the built-in formats to see if it can find one to read this stream.
        There are currently built-in decoders for PNG, JPEG and GIF formats.
        The object that is returned should not be deleted by the caller.
        @see canUnderstand, decodeImage, loadFrom
    */
    static ImageFileFormat* findImageFormatForStream (InputStream& input);

    /** Looks for a format that can handle the given file extension.
        There are currently built-in formats for PNG, JPEG and GIF formats.
        The object that is returned should not be deleted by the caller.
    */
    static ImageFileFormat* findImageFormatForFileExtension (const File& file);

    //==============================================================================
    /** Tries to load an image from a stream.

        This will use the findImageFormatForStream() method to locate a suitable
        codec, and use that to load the image.

        @returns        the image that was decoded, or an invalid image if it fails.
    */
    static Image loadFrom (InputStream& input);

    /** Tries to load an image from a file.

        This will use the findImageFormatForStream() method to locate a suitable
        codec, and use that to load the image.

        @returns        the image that was decoded, or an invalid image if it fails.
    */
    static Image loadFrom (const File& file);

    /** Tries to load an image from a block of raw image data.

        This will use the findImageFormatForStream() method to locate a suitable
        codec, and use that to load the image.

        @returns        the image that was decoded, or an invalid image if it fails.
    */
    static Image loadFrom (const void* rawData,
                           size_t numBytesOfData);
};

//==============================================================================
/**
    A subclass of ImageFileFormat for reading and writing PNG files.

    @see ImageFileFormat, JPEGImageFormat

    @tags{Graphics}
*/
class JUCE_API  PNGImageFormat  : public ImageFileFormat
{
public:
    //==============================================================================
    PNGImageFormat();
    ~PNGImageFormat() override;

    //==============================================================================
    String getFormatName() override;
    bool usesFileExtension (const File&) override;
    bool canUnderstand (InputStream&) override;
    Image decodeImage (InputStream&) override;
    bool writeImageToStream (const Image&, OutputStream&) override;
};


//==============================================================================
/**
    A subclass of ImageFileFormat for reading and writing JPEG files.

    @see ImageFileFormat, PNGImageFormat

    @tags{Graphics}
*/
class JUCE_API  JPEGImageFormat  : public ImageFileFormat
{
public:
    //==============================================================================
    JPEGImageFormat();
    ~JPEGImageFormat() override;

    //==============================================================================
    /** Specifies the quality to be used when writing a JPEG file.

        @param newQuality  a value 0 to 1.0, where 0 is low quality, 1.0 is best, or
                           any negative value is "default" quality
    */
    void setQuality (float newQuality);

    //==============================================================================
    String getFormatName() override;
    bool usesFileExtension (const File&) override;
    bool canUnderstand (InputStream&) override;
    Image decodeImage (InputStream&) override;
    bool writeImageToStream (const Image&, OutputStream&) override;

private:
    float quality;
};

//==============================================================================
/**
    A subclass of ImageFileFormat for reading BMP files.

    @see ImageFileFormat, PNGImageFormat, JPEGImageFormat

    @tags{Graphics}
*/
class JUCE_API  BMPImageFormat : public ImageFileFormat
{
public:
    //==============================================================================
    BMPImageFormat();
    ~BMPImageFormat() override;

    //==============================================================================
    String getFormatName() override;
    bool usesFileExtension(const File&) override;
    bool canUnderstand(InputStream&) override;
    Image decodeImage(InputStream&) override;
    bool writeImageToStream(const Image&, OutputStream&) override;

private:

#ifdef __GNUC__
#define PACKED( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#elif defined _MSC_VER
#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else 
#define PACKED( __Declaration__ ) __Declaration__
#endif

    PACKED(struct BitmapInfoHeader {
        uint32 size = sizeof(BitmapInfoHeader); // size of structure in bytes
        int32  width = 0; // pixel width
        int32  height = 0; // pixel height
        int16  planes = 1; // always 1
        int16  bitCount; // bits per pixel
        uint32 compression = 0; // leave set to 0, since no compression is supported here
        uint32 imageSize = 0; // size of image in bytes. Can be left 0 for uncompressed images.
        int32  pixelsPerMeterX = 0; // pixels per meter horizontally
        int32  pixelsPerMeterY = 0; // pixels per meter vertically
        uint32 colorsUsed = 0; // not relevent since we won't support color tables
        uint32 colorsImportant = 0; // not relevent since we won't support color tables
    };)

        PACKED(struct BitmapFileHeader {
        uint16  type;// = 0x4d42; // always 0x4d42 'BM'
        uint32 fileSize;// = 0; // filesize in bytes
        uint16  reserved1;// = 0; // must be 0
        uint16  reserved2;// = 0; // must be 0
        uint32 offset;// = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);  // offset to image data
    };)
#undef PACKED
};

//==============================================================================
/**
    A subclass of ImageFileFormat for reading GIF files.

    @see ImageFileFormat, PNGImageFormat, JPEGImageFormat

    @tags{Graphics}
*/
class JUCE_API  GIFImageFormat  : public ImageFileFormat
{
public:
    //==============================================================================
    GIFImageFormat();
    ~GIFImageFormat() override;

    //==============================================================================
    String getFormatName() override;
    bool usesFileExtension (const File&) override;
    bool canUnderstand (InputStream&) override;
    Image decodeImage (InputStream&) override;
    bool writeImageToStream (const Image&, OutputStream&) override;
};

//==============================================================================
/**
    A subclass of ImageFileFormat for reading and writing WEBP files.
    Currently just a wrapper for executing dwebp

    @see ImageFileFormat, WEBPImageFormat
*/
class JUCE_API  WEBPImageFormat : public ImageFileFormat
{
public:
    //==============================================================================
    WEBPImageFormat();
    ~WEBPImageFormat() override;

    //==============================================================================
    /** Specifies the quality to be used when writing a WEBP file.

        @param shouldBeLossless         boolean true for lossless encoding.
        @param newQuality               a value 0 to 1.0, where 0 is low quality, 1.0 is best, or
                                        any negative value is "default" quality.  Ignored if lossless.
    */
    void setQuality(bool shouldBeLossless, float newQuality = 0.85f);

    //==============================================================================
    String getFormatName() override;
    bool usesFileExtension(const File&) override;
    bool canUnderstand(InputStream&) override;
    Image decodeImage(InputStream&) override;
    bool writeImageToStream(const Image& sourceImage, OutputStream& destStream) override;

private:
    bool lossless = true;
    float quality;
};

} // namespace juce
