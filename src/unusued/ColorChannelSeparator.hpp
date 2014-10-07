#ifndef NEURON_COLOR_CHANNEL_SEPARATOR_HPP
#define NEURON_COLOR_CHANNEL_SEPARATOR_HPP

/* Separate out the neuron color channels 
   and print out the tiff images separately
 */

#include <iostream>
#include <stdint.h>
#include "tiffio.h"

#define CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)


class ColorChannelSeparator {

public:
    ColorChannelSeparator (bool is_alpha_needed);

    void apply (TIFF *in, TIFF *out, bool is_red, bool is_green, bool is_blue);

private:
    void modifyChPrintWholeImage (TIFF *in, TIFF *out, 
                            bool is_red, bool is_green, bool is_blue);

    bool is_alpha_needed_ = false;

};

#endif
