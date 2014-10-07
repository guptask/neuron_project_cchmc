#include "ColorChannelSeparator.hpp"

ColorChannelSeparator::ColorChannelSeparator (bool is_alpha_needed) : 
    is_alpha_needed_(is_alpha_needed) {}

void ColorChannelSeparator::apply (
        TIFF *in, TIFF *out, bool is_red, bool is_green, bool is_blue) {

    uint32_t width = 0, height = 0, long_v = 0;
    uint16_t short_v = 0, v[1] = {0};
    float float_v = 0.0;
    char *string_v = NULL;

    TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(in, TIFFTAG_IMAGELENGTH, &height);

    CopyField(TIFFTAG_SUBFILETYPE, long_v);
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    CopyField(TIFFTAG_FILLORDER, short_v);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    if (!is_alpha_needed_) {
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
    } else {
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 4);
        v[0] = EXTRASAMPLE_ASSOCALPHA;
        TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, v);
    }

    CopyField(TIFFTAG_XRESOLUTION, float_v);
    CopyField(TIFFTAG_YRESOLUTION, float_v);
    CopyField(TIFFTAG_RESOLUTIONUNIT, short_v);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_SOFTWARE, TIFFGetVersion());
    CopyField(TIFFTAG_DOCUMENTNAME, string_v);

    modifyChPrintWholeImage (in, out, is_red, is_green, is_blue);
}


void ColorChannelSeparator::modifyChPrintWholeImage (
        TIFF *in, TIFF *out, bool is_red, bool is_green, bool is_blue) {

    uint32_t *raster = NULL, width = 0, height = 0, row = 0;
    TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(in, TIFFTAG_IMAGELENGTH, &height);

    uint32_t rows_per_strip = (uint32_t) -1;
    rows_per_strip = TIFFDefaultStripSize(out, rows_per_strip);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rows_per_strip);

    raster = (uint32_t *)_TIFFmalloc(width * height * sizeof (uint32));
    if (!raster) {
        TIFFError(TIFFFileName(in), "No space for raster buffer");
        return;
    }

    /* Read the image in one chunk into an RGBA array */
    if (!TIFFReadRGBAImageOriented(in, width, height, raster, ORIENTATION_TOPLEFT, 0)) {
        _TIFFfree(raster);
        return;
    }

    int	pixel_count = width * height;
    unsigned char *src = (unsigned char *) raster;
    unsigned char *dst = (unsigned char *) raster;

    while (pixel_count > 0) {
        *(dst++) = (is_red) ? *src : 0; src++;
        *(dst++) = (is_green) ? *src : 0; src++;
        *(dst++) = (is_blue) ? *src : 0; src++;
        if (!is_alpha_needed_) {
            src++;
        } else {
            *(dst++) = *(src++);
        }
        pixel_count--;
    }

    /* Write out the result in strips */
    for (row = 0; row < height; row += rows_per_strip) {
        unsigned char *raster_strip = NULL;
        int	rows_to_write = 0, bytes_per_pixel = 0;

        if (!is_alpha_needed_) {
            raster_strip = ((unsigned char *) raster) + 3 * row * width;
            bytes_per_pixel = 3;
        } else {
            raster_strip = (unsigned char *) (raster + row * width);
            bytes_per_pixel = 4;
        }

        if (row + rows_per_strip > height) {
            rows_to_write = height - row;
        } else {
            rows_to_write = rows_per_strip;
        }

        if (TIFFWriteEncodedStrip(out, row/rows_per_strip, raster_strip,
                             bytes_per_pixel * rows_to_write * width ) == -1) {
            _TIFFfree(raster);
            return;
        }
    }
    _TIFFfree( raster );
}
