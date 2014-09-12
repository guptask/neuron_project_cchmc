#include "NeuronColorChannelSeparator.hpp"
#include <memory>

int main (int argc, char *argv[]) {

    std::unique_ptr<NeuronColorChannelSeparator> col_ch_separator = 
        std::unique_ptr<NeuronColorChannelSeparator>(new NeuronColorChannelSeparator(false));

    TIFFErrorHandler error_handler = TIFFSetWarningHandler(NULL);
    TIFF *in = TIFFOpen(argv[1], "r");
    if (!in) {
        TIFFError(TIFFFileName(in), "Could not open input image");
        return 0;
    }

    TIFF *out = TIFFOpen(argv[2], "w");
    if (!out) {
        TIFFError(TIFFFileName(out), "Could not open output image");
        return 0;
    }
    TIFFSetWarningHandler(error_handler);

    col_ch_separator->printChannelImage(in, out, false, false, true);
    TIFFWriteDirectory(out);

    TIFFClose(in);
    TIFFClose(out);

    return 0;
}
