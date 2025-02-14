#include "BioFormatsImage.h"
#include "Timer.h"
#include <cmath>
#include <sstream>

#include <cstdlib>
#include <cassert>

#include <cstdio>
#include <chrono>

#include <limits>

// TODO: comment me
#define DEBUG_OSI 1

// TODO: comment me
#define DEBUG_VERBOSE 1

using namespace std;

extern std::ofstream logfile;

void BioFormatsImage::openImage() throw(file_error)
{
    fprintf(stderr, "ddBioFormatsImage.cc:  BioFormatsImage::openImage() start\n");
    string filename = getFileName(currentX, currentY);

    // get the file modification date/time.   return false if not changed, return true if change compared to the stored info.
    bool modified = updateTimestamp(filename);
    // close previous
    closeImage();

#ifdef DEBUG_OSI
    Timer timer;
    timer.start();

    logfile << "BioFormats :: openImage() :: start" << endl
            << flush;
#endif
    fprintf(stderr, "ddBioFormatsImage.cc:  continue1 \n");

    fprintf(stderr, "dddBioFormatsImage.cc entering file\n");
    if (!bfi.open(filename))
    {
        fprintf(stderr, "dddBioFormatsImage.cc cant enter file:\n");

        const char *error = bfi.get_error().c_str();
        fprintf(stderr, "dddBioFormatsImage.cc cant enter file %s:\n", error);

        logfile << "ERROR: encountered error: " << error << " while opening " << filename << " with BioFormats: " << endl
                << flush;
        throw file_error(string("Error opening '" + filename + "' with BioFormats, error " + error));
    }
    fprintf(stderr, "dddBioFormatsImage.cc entered file\n");

#ifdef DEBUG_OSI
    logfile << "BioFormats :: openImage() :: " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

#ifdef DEBUG_OSI
    logfile << "BioFormats :: openImage() :: completed " << filename << endl
            << flush;
#endif

    if (bpc == 0)
    {
        loadImageInfo(currentX, currentY);
    }

    isSet = true;
}

static unsigned int getPowerOfTwoRoundDown(unsigned int a)
{
#if (defined(__GNUC__) && __GNUC__ > 4) || (defined(__clang__) && __clang_major__ > 6)
    // works for a > 0
    return sizeof(unsigned int) * 8 - __builtin_clz(a) - 1;
#else
    unsigned int x = 0;
    while (a >> 1)
    {
        x++;
    }
    return x - 1;
#endif
}

/// given an open OSI file, get information from the image.
void BioFormatsImage::loadImageInfo(int x, int y) throw(file_error)
{
    fprintf(stderr, "ddddBioFormatsImage :: loadImageInfo starting\n");

#ifdef DEBUG_OSI
    logfile << "BioFormatsImage :: loadImageInfo()" << endl;

#endif

    int w = 0, h = 0;
    currentX = x;
    currentY = y;

    // choose power of 2 to make downsample simpler.
    int suggested_width = bfi.get_optimal_tile_width();
    if (suggested_width > 0)
    {
        suggested_width = 1 << getPowerOfTwoRoundDown(suggested_width);
    }
    if (suggested_width > 2048 || suggested_width < 128)
    {
        tile_width = 256;
    }
    else
    {
        tile_width = suggested_width;
    }

    int suggested_height = bfi.get_optimal_tile_width();
    if (suggested_height > 0)
    {
        suggested_height = 1 << getPowerOfTwoRoundDown(suggested_height);
    }
    if (suggested_height > 2048 || suggested_height < 128)
    {
        tile_height = 256;
    }
    else
    {
        tile_height = suggested_height;
    }

    w = bfi.get_size_x();
    h = bfi.get_size_y();

    if (w < 0 || h < 0)
    {
        const char *err = bfi.get_error().c_str();

        logfile << "ERROR: encountered error: " << err << " while getting level0 dim" << endl;
        throw file_error("Getting bioformats level0 dimensions: " + std::string(err));
    }

#ifdef DEBUG_OSI
    logfile << "dimensions :" << w << " x " << h << endl;
    // logfile << "comment : " << comment << endl;
#endif

#ifdef DEBUG_VERBOSE
    // PLEASE NOTE: these can differ between resolution levels
    cerr << "Parsing details" << endl;
    cerr << "Optimal: " << tile_width << " " << tile_height << endl;
    cerr << "rgbChannelCount: " << bfi.get_rgb_channel_count() << endl; // Number of colors returned with each openbytes call
    cerr << "sizeC: " << bfi.get_size_c() << endl;
    cerr << "effectiveSizeC: " << bfi.get_effective_size_c() << endl; // colors on separate planes. 1 if all on same plane
    cerr << "sizeZ: " << bfi.get_size_z() << endl;
    cerr << "sizeT: " << bfi.get_size_t() << endl;
    cerr << "ImageCount: " << bfi.get_image_count() << endl; // number of planes in series
    cerr << "isRGB: " << (int)bfi.is_rgb() << endl;          // multiple colors per openbytes plane
    cerr << "isInterleaved: " << (int)bfi.is_interleaved() << endl;
    cerr << "isInterleaved: " << (int)bfi.is_interleaved() << endl;
#endif

    fprintf(stderr, "continue info: parsing file in bioformatsimage.cc\n");

    // TODO important: these need to be moved to getnativetile
    // as these can change in the image
    // should we make custom function that communicates these efficiently with bits?
    // search for sampletype in this file perhaps
    // see TPTImage.cc#L89 to see an example of setting these per tile, not per file
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/RawTile.h#L61

    // iipsrv takes 1 or 3 only. we cant set this to 4 for internal use, iip will access that as well
    channels = 3;
    channels_internal = bfi.get_rgb_channel_count();
    if (channels_internal != 3 && channels_internal != 4)
    {
        if (channels_internal > 0)
        {
            logfile << "Unimplemented: only support 3, 4 channels, not " << channels_internal << endl;
            fprintf(stderr, "branch1 %d\n", channels_internal);
            throw file_error("Unimplemented: only support 3, 4 channels, not " + channels_internal);
        }
        else
        {
            const char *err = bfi.get_error().c_str();
            fprintf(stderr, "branch2\n");
            logfile << "Error while getting channel count: " << err << endl;
            throw file_error("Error while getting channel count: " + std::string(err));
        }
    }

    if (bfi.get_effective_size_c() != 1)
    {
        fprintf(stderr, "branch3.5 %d\n", bfi.get_effective_size_c());
        // TODO: find an example. To implement, call openBytes three times perhaps.
        // Or maybe this is when we have a RGB plus a grayscale channel?
        logfile << "Unimplemented: currently noninterleaved works if we have them on the same plane" << endl;
    }
    fprintf(stderr, "continue info50: parsing file in bioformatsimage.cc\n");

    if (bfi.is_indexed_color() && !bfi.is_false_color())
    {
        /*To implement this, get8BitLookupTable() or get16BitLookupTable()
        and then read from there. in theory, this might change between series/resolutions.

        One way is: check isfalsecolor when initializing new file in java
then do for 0th plane: check is fake color, then do get8BitLookupTable and get16bitlt, save these, and to read, if the sample format when reading bytes is same type (8 bit or 16 bit) for table index, access it. when closing file, reset table
*/
        fprintf(stderr, "False color image detected!\n");

        logfile << "Unimplemented: False color image" << endl;
        throw file_error("Unimplemented: False color image");
    }

    if (bfi.get_dimension_order().length() && bfi.get_dimension_order()[2] != 'C')
    {
        fprintf(stderr, "branch6\n");

        logfile << "Unimplemented: unfamiliar dimension order " << bfi.get_dimension_order() << endl;
        throw file_error("Unimplemented: unfamiliar dimension order " + std::string(bfi.get_dimension_order()));
    }

    // bfi.get_bytes_per_pixel actually gives bits per channel per pixel, so don't divide by channels
    int bytespc_internal = bfi.get_bytes_per_pixel();
    bpc = 8;
    colourspace = sRGB;

    /*
          if ((bpp % channels) != 0) {
        // How can this be handled? Should be very rare.
        throw file_error("Unimplemented: bad remainder when diving bits per pixel among channels. bfi.get_bits_per_pixel(): " + std::to_string(bfi.get_bits_per_pixel()) + " channels: " + std::to_string(channels));
    }
    */
    fprintf(stderr, "continue info100: parsing file in bioformatsimage.cc\n");

    if (bytespc_internal <= 0)
    {
        fprintf(stderr, "branch8\n");

        const char *err = bfi.get_error().c_str();
        logfile << "Error while getting bits per pixel: " << err << endl;
        throw file_error("Error while getting bits per pixel: " + std::string(err));
    }

#define too_big (tile_width * tile_height * bytespc_internal * bfi.get_rgb_channel_count() > bfi_communication_buffer_len)
    while (too_big)
    {
        tile_height >>= 1;
        if (!too_big)
            break;
        tile_width >>= 1;
    }
#undef too_big
    fprintf(stderr, "continue info150: parsing file in bioformatsimage.cc\n");

    // save the openslide dimensions.
    std::vector<int> bioformats_widths, bioformats_heights;
    bioformats_widths.clear();
    bioformats_heights.clear();

    int bioformats_levels = bfi.get_resolution_count();
    fprintf(stderr, "continue info170: parsing file in bioformatsimage.cc %d\n", bioformats_levels);

    if (bioformats_levels <= 0)
    {
        const char *err = bfi.get_error().c_str();
        logfile << "ERROR: encountered error: " << err << " while getting level count" << endl;
        throw file_error("ERROR: encountered error: " + std::string(err) + " while getting level count");
    }

#ifdef DEBUG_OSI
    logfile << "number of levels = " << bioformats_levels << endl;
    double tempdownsample;
#endif

    int ww, hh;
    for (int i = 0; i < bioformats_levels; i++)
    {
        fprintf(stderr, "continue info185: parsing file in bioformatsimage.cc\n");
        bfi.set_current_resolution(i);
        fprintf(stderr, "continue info190: parsing file in bioformatsimage.cc\n");

        ww = bfi.get_size_x();
        hh = bfi.get_size_y();

#ifdef DEBUG_VERBOSE
        fprintf(stderr, "resolution %d has x=%d y=%d", i, ww, hh);
#endif
        if (ww <= 0 || hh <= 0)
        {
            logfile << "ERROR: encountered error: while getting level dims for level " << i << endl;
            throw file_error("error while getting level dims for level " + i);
        }
        bioformats_widths.push_back(ww);
        bioformats_heights.push_back(hh);
#ifdef DEBUG_OSI
        tempdownsample = ((double)(w) / ww + (double)(h) / hh) / 2;
        logfile << "\tlevel " << i << "\t(w,h) = (" << ww << "," << hh << ")\tdownsample=" << tempdownsample << endl;
#endif
    }
    fprintf(stderr, "continue info200: parsing file in bioformatsimage.cc\n");

    bioformats_widths.push_back(0);
    bioformats_heights.push_back(0);

    image_widths.clear();
    image_heights.clear();

    //======== virtual levels because getTile specifies res as powers of 2.
    // precompute and store addition info about the tiles
    lastTileXDim.clear();
    lastTileYDim.clear();
    numTilesX.clear();
    numTilesY.clear();

    bioformats_level_to_use.clear();
    bioformats_downsample_in_level.clear();
    unsigned int bf_level = 0;               // which layer bioformats provides us
    unsigned int bf_downsample_in_level = 1; // how much to scale internally that layer

    // store the original size.
    image_widths.push_back(w);
    image_heights.push_back(h);
    lastTileXDim.push_back(w % tile_width);
    lastTileYDim.push_back(h % tile_height);
    // As far as I understand, this arithmetic means that
    // last remainder has at least 0, at most n-1 columns/rows
    // where n is tile_width or tile_height:
    numTilesX.push_back((w + tile_width - 1) / tile_width);
    numTilesY.push_back((h + tile_height - 1) / tile_height);
    bioformats_level_to_use.push_back(bf_level);
    bioformats_downsample_in_level.push_back(bf_downsample_in_level);

    // what if there are ~~openslide~~ bioformats levels with dim smaller than this?

    // populate at 1/2 size steps
    while ((w > 256) || (h > 256))
    {
        // need a level that has image completely inside 1 tile.
        // (stop with both w and h less than tile_w/h,  previous iteration divided by 2.

        w >>= 1; // divide by 2 and floor. losing 1 pixel from higher res.
        h >>= 1;

        // compare to next level width and height. if the calculated res is smaller, next level is better for supporting the w/h
        // so switch level.
        if (w <= bioformats_widths[bf_level + 1] && h <= bioformats_heights[bf_level + 1])
        {
            ++bf_level;
            bf_downsample_in_level = 1; // just went to next smaller level, don't downsample internally yet.

            // Handle duplicate levels
            while (w <= bioformats_widths[bf_level + 1] && h <= bioformats_heights[bf_level + 1])
            {
                ++bf_level;
            }
        }
        else
        {
            bf_downsample_in_level <<= 1; // next one, downsample internally by 2.
        }
        bioformats_level_to_use.push_back(bf_level);
        bioformats_downsample_in_level.push_back(bf_downsample_in_level);

        image_widths.push_back(w);
        image_heights.push_back(h);
        lastTileXDim.push_back(w % tile_width);
        lastTileYDim.push_back(h % tile_height);
        numTilesX.push_back((w + tile_width - 1) / tile_width);
        numTilesY.push_back((h + tile_height - 1) / tile_height);

#ifdef DEBUG_VERBOSE
        cerr << "downsamplein levels:" << endl;
        for (auto i : bioformats_downsample_in_level)
            cerr << i << " ";
        cerr << "\n";

        cerr << "numtilex:" << endl;
        for (auto i : numTilesX)
            cerr << i << " ";
        cerr << "\n";
#endif

#ifdef DEBUG_OSI
        logfile << "Create virtual layer : " << w << "x" << h << std::endl;
#endif
    }

#ifdef DEBUG_OSI
    for (int t = 0; t < image_widths.size(); t++)
    {
        logfile << "virtual level " << t << " (w,h)=(" << image_widths[t] << "," << image_heights[t] << "),";
        logfile << " (last_tw,last_th)=(" << lastTileXDim[t] << "," << lastTileYDim[t] << "),";
        logfile << " (ntx,nty)=(" << numTilesX[t] << "," << numTilesY[t] << "),";
        logfile << " os level=" << bioformats_level_to_use[t] << " downsample from bf_level=" << bioformats_downsample_in_level[t] << endl;
    }
#endif

    numResolutions = numTilesX.size();

    // only support bpp of 8 (255 max), and 3 channels
    min.assign(channels, 0.0f);
    max.assign(channels, (float)(1 << bpc) - 1.0f);
    fprintf(stderr, "continue info500: parsing file in bioformatsimage.cc\n");
}

/// Overloaded function for closing a TIFF image
void BioFormatsImage::closeImage()
{
#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif

    fprintf(stderr, "Called bfi.close in BioFormatsImage::closeImage\n");
    bfi.close();

#ifdef DEBUG_OSI
    logfile
        << "BioFormats :: closeImage() :: " << timer.getTime() << " microseconds" << endl;
#endif
}

/// Overloaded function for getting a particular tile
/** \param x horizontal sequence angle (for microscopy, ignored.)
    \param y vertical sequence angle (for microscopy, ignored.)
    \param r resolution - specified as -log_2(mag factor), where mag_factor ~= highest res width / target res width.  0 to numResolutions - 1.
    \param l number of quality layers to decode - for jpeg2000
    \param t tile number  (within the resolution level.)	specified as a sequential number = y * width + x;
 */
RawTilePtr BioFormatsImage::getTile(int seq, int ang, unsigned int iipres, int layers, unsigned int tile) throw(file_error)
{

#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif
#ifdef DEBUG_VERBOSE
    fprintf(stderr, "BioFormats::getTile called: iipres %d numResolutions %d\n", iipres, numResolutions);
#endif

    if (iipres > (numResolutions - 1))
    {
        ostringstream tile_no;
        tile_no << "BioFormats :: Asked for non-existant resolution: " << iipres;
        throw file_error(tile_no.str());
        return 0;
    }

    // res is specified in opposite order from openslide virtual image levels: image level 0 has highest res,
    // image level nRes-1 has res of 0.
    uint32_t osi_level = numResolutions - 1 - iipres;

#ifdef DEBUG_VERBOSE
    fprintf(stderr, "tile requested: %u is_zoom/osi_level: %d", tile, osi_level);
#endif

#ifdef DEBUG_OSI
    logfile << "BioFormats :: getTile() :: res=" << iipres << " tile= " << tile << " is_zoom= " << osi_level << endl;

#endif

    //======= get the dimensions in pixels and num tiles for the current resolution
    /*
        int64_t layer_width = 0;
        int64_t layer_height = 0;
        bfi.set_current_resolution(osi_level);
        layer_width = bfi.get_size_x();
        layer_height = bfi.get_size_y();

    #ifdef DEBUG_VERBOSE
        fprintf(stderr, "layer: %d layer_width: %d layer_height: %d", layers, layer_width, layer_height);
    #endif
    */
    // Calculate the number of tiles in each direction
    size_t ntlx = numTilesX[osi_level];
    size_t ntly = numTilesY[osi_level];

    if (tile >= ntlx * ntly)
    {
        ostringstream tile_no;
        tile_no << "BioFormatsImage :: Asked for non-existant tile: " << tile;
        throw file_error(tile_no.str());
    }

    // tile x.
    size_t tx = tile % ntlx;
    size_t ty = tile / ntlx;

    RawTilePtr ttt = getCachedTile(tx, ty, iipres);

#ifdef DEBUG_OSI
    logfile << "BioFormats :: getTile() :: total " << timer.getTime() << " microseconds" << endl
            << flush;
    logfile << "TILE RENDERED" << std::endl;
#endif
    return ttt; // return cached instance.  TileManager's job to copy it..
}

/**
 * check if cache has tile.
 *    if yes, return it.
 *    if not,
 *       if a native layer, getNativeTile,
 *       else call halfsampleAndComposeTile
 * @param res  	iipsrv's resolution id.  openslide's level is inverted from this.
 */
RawTilePtr BioFormatsImage::getCachedTile(const size_t tilex, const size_t tiley, const uint32_t iipres)
{

#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif

    assert(tileCache);

    // check if cache has tile
    uint32_t osi_level = numResolutions - 1 - iipres;
    uint32_t tid = tiley * numTilesX[osi_level] + tilex;
    RawTilePtr ttt = tileCache->getObject(TileCache::getIndex(getImagePath(), iipres, tid, 0, 0, UNCOMPRESSED, 0));

    // if cache has file, return it
    if (ttt)
    {
#ifdef DEBUG_OSI
        logfile << "BioFormats :: getCachedTile() :: Cache Hit " << tilex << "x" << tiley << "@" << iipres << " osi tile bounds: " << numTilesX[osi_level] << "x" << numTilesY[osi_level] << " " << timer.getTime() << " microseconds" << endl
                << flush;
#endif

        return ttt;
    }
    // else caches does not have it.
#ifdef DEBUG_OSI
    logfile << "BioFormats :: getCachedTile() :: Cache Miss " << tilex << "x" << tiley << "@" << iipres << " osi tile bounds: " << numTilesX[osi_level] << "x" << numTilesY[osi_level] << " " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

    // is this a native layer?
    if (bioformats_downsample_in_level[osi_level] == 1)
    {
        logfile << "nativelayer";
        // supported by native openslide layer
        // tile manager will cache if needed
        return getNativeTile(tilex, tiley, iipres);
    }
    else
    {
        logfile << "nonnative layer";

        // not supported by native openslide layer, so need to compose from next level up,
        return halfsampleAndComposeTile(tilex, tiley, iipres);

        // tile manager will cache this one.
    }
}
#pragma GCC optimize("O3")
/**
 * read from file, color convert, store in cache, and return tile.
 *
 * @param res  	iipsrv's resolution id.  openslide's level is inverted from this.
 */
RawTilePtr BioFormatsImage::getNativeTile(const size_t tilex, const size_t tiley, const uint32_t iipres)
{

#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif

#ifdef DEBUG_VERBOSE
    fprintf(stderr, "BioFormatsImage::getNativeTile began;\ngetNativeTile params: tilex: %lu tiley: %lu iipres: %u\n", tilex, tiley, iipres);
#endif

    // compute the parameters (i.e. x and y offsets, w/h, and bestlayer to use.
    uint32_t osi_level = numResolutions - 1 - iipres;

    // find the next layer to downsample to desired zoom level z
    //
    uint32_t bestLayer = bioformats_level_to_use[osi_level];
#ifdef DEBUG_VERBOSE
    fprintf(stderr, "Best layer: %u\n", bestLayer);
#endif

    size_t ntlx = numTilesX[osi_level];
    size_t ntly = numTilesY[osi_level];
#ifdef DEBUG_VERBOSE
    fprintf(stderr, "ntlx %lu ntly %lu\n", ntlx, ntly);
    cerr << "tile requested: " << tilex << " "  << tiley << endl;
#endif

    // compute the correct width and height
    size_t tw = tile_width;
    size_t th = tile_height;
#ifdef DEBUG_VERBOSE
    fprintf(stderr, "tilewidth %lu tileheight %lu\n", tw, th);
#endif

    // Get the width and height for last row and column tiles
    size_t rem_x = this->lastTileXDim[osi_level];
    size_t rem_y = this->lastTileYDim[osi_level];

    // Alter the tile size if it's in the rightmost column
    if ((tilex == ntlx - 1) && (rem_x != 0))
    {
        tw = rem_x;
    }
    // Alter the tile size if it's in the bottom row
    if ((tiley == ntly - 1) && (rem_y != 0))
    {
        th = rem_y;
    }

    if (tilex > ntlx || tiley > ntly)
    {
        cerr << "Inexistant tile!";
        throw file_error("inexistant");
    }

        // create the RawTile object
        RawTilePtr rt(new RawTile(tiley * ntlx + tilex, iipres, 0, 0, tw, th, 3, bpc));

    // compute the size, etc
    rt->dataLength = tw * th * 3 * sizeof(unsigned char);
    rt->filename = getImagePath();
    rt->timestamp = timestamp;

    int allocate_length = rt->dataLength;

    if (!bfi.set_current_resolution(bestLayer))
    {
        auto s = string("FATAL : bad resolution: " + std::to_string(bestLayer) + " rather than up to " + std::to_string(bfi.get_resolution_count() - 1));
        logfile << s;
        throw file_error(s);
    }

    // We currently don't assume that this is the same among all resolutions
    // Is it?
    char should_reduce_channels_from_4to3 = 0;
    int channels = bfi.get_rgb_channel_count();
    if (channels != 3 && channels != 4)
    {
        throw file_error("Channels not 3 or 4: " + std::to_string(channels));
    }
    if (channels == 4)
    {
        should_reduce_channels_from_4to3 = 1;
        allocate_length = tw * th * 4 * sizeof(unsigned char);
    }

    // These must be called after bfi.set_current_resolution
    // It's sometimes different between resolutions

    int bytespc_internal = bfi.get_bytes_per_pixel();
    int should_interleave = !bfi.is_interleaved();

    // https://github.com/ome/bioformats/blob/metadata54/components/formats-api/src/loci/formats/FormatTools.java#L76
    int pixel_type = bfi.get_pixel_type();

    int should_remove_sign = 1;
    // added float and double, because they are handled specially
    if (pixel_type == 1 || pixel_type == 3 || pixel_type == 5 || pixel_type == 6 || pixel_type == 7 || pixel_type == 8)
    {
        should_remove_sign = 0;
    }

    int should_convert_from_float = pixel_type == 6;
    int should_convert_from_double = pixel_type == 7;
    int should_convert_from_bit = pixel_type == 8;

    // TODO: sampletype can be implemented here, for float support
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/RawTile.h#L118
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/IIPImage.h#L123
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/RawTile.h#L181

    // new a block ...
    // relying on delete [] to do the right thing.
    rt->data = new unsigned char[allocate_length];
    rt->memoryManaged = 1; // allocated data, so use this flag to indicate that it needs to be cleared on destruction
                           // rawtile->padded = false;
#ifdef DEBUG_OSI
    logfile << "Allocating tw * th * channels * sizeof(char) : " << tw << " * " << th << " * " << channels << " * sizeof(char) " << endl
            << flush;
#endif

#ifdef DEBUG_VERBOSE
    fprintf(stderr, "after remainder process: tilewidth %lu tileheight %lu\n", tw, th);
    cerr << "Serving such tile: width " << rt->width << " height " << rt->height << " channels " << rt->channels << " bpc " << rt->bpc << "\n";
#endif

#ifdef DEBUG_VERBOSE
    cerr << "Parsing details FOR TILE" << endl;
    cerr << "Optimal: " << tile_width << " " << tile_height << endl;
    cerr << "rgbChannelCount: " << bfi.get_rgb_channel_count() << endl; // Number of colors returned with each openbytes call
    cerr << "sizeC: " << bfi.get_size_c() << endl;
    cerr << "effectiveSizeC: " << bfi.get_effective_size_c() << endl; // colors on separate planes. 1 if all on same plane
    cerr << "sizeZ: " << bfi.get_size_z() << endl;
    cerr << "sizeT: " << bfi.get_size_t() << endl;
    cerr << "ImageCount: " << bfi.get_image_count() << endl; // number of planes in series
    cerr << "isRGB: " << (int)bfi.is_rgb() << endl;          // multiple colors per openbytes plane
    cerr << "isInterleaved: " << (int)bfi.is_interleaved() << endl;
    cerr << "isInterleaved: " << (int)bfi.is_interleaved() << endl;
#endif

    // READ FROM file

    //======= next compute the x and y coordinates (top left corner) in level 0 coordinates
    //======= expected by bfi.open_bytes.
    int tx0 = tilex * tile_width;
    int ty0 = tiley * tile_height;

#ifdef DEBUG_VERBOSE
    cerr << "bfi.open_bytes params: " << bestLayer << " " << tx0 << " " << ty0 << " " << tw << " " << th << std::endl;
    cerr << "downsample in level " << bioformats_downsample_in_level[osi_level] << endl;

    cerr << "this layer has resolution x=" << bfi.get_size_x() << " y=" << bfi.get_size_y() << endl;
#endif

    if (!rt->data)
        throw file_error(string("FATAL : BioFormatsImage read_region => allocation memory ERROR"));

    /*char *test = (char*) "/images/pngtest1.png";
    cerr << "but, instead, callin bfi.bfinternal_deleteme\n" << test << endl;

    if (bfi.bfinternal_deleteme(test) < 0)
    {
        cerr << "couldn't simulate - check path?\n";
    }
    cerr << "returned from there\n";
    // end BREAK*/

    cerr << "calling bfi.open_bytes\n";

    // https://stackoverflow.com/questions/31657511/chrono-the-difference-between-two-points-in-time-in-milliseconds
    auto start = std::chrono::high_resolution_clock::now();
    int bytes_received = bfi.open_bytes(tx0, ty0, tw, th);
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = finish - start;
    milliseconds += elapsed.count();
    cerr << "Milliseconds: " << milliseconds << endl;
    if (bytes_received < 0)
    {
        cerr << "bfi.open_bytes got an error! expected this many bytes: " << (channels_internal * bytespc_internal * tw * th) << " but got " << bfi.get_error().c_str();
        const char *error = bfi.get_error().c_str();
        logfile << "ERROR: encountered error: " << error << " while reading region exact at  " << tx0 << "x" << ty0 << " dim " << tw << "x" << th << " with BioFormats: " << error << endl;
        throw file_error("ERROR: encountered error: " + std::string(error) + " while reading region exact at " + std::to_string(tx0) + "x" + std::to_string(ty0) + " dim " + std::to_string(tw) + "x" + std::to_string(th) + " with BioFormats: " + std::string(error));
    }
    cerr << "bfi.open_bytes returned with success for this many bytes: "
         << bytes_received << std::endl;
    cerr << "channels_internal " << channels_internal << " bytespc_internal"
         << bytespc_internal << " tw th " << tw << " " << th << std::endl;

    if (bytes_received != channels_internal * bytespc_internal * tw * th)
    {
        fprintf(stderr, "got an unexpected number of bytes\n");
        throw file_error("ERROR: expected len " + std::to_string(channels * bpc / 8 * tw * th) + " but got " + std::to_string(bytes_received));
    }
    // Note: please don't copy anything more than
    // bytes_received when it's positive

    /*
    Summary of next lines:

    var signed = ...
    var bit = ...

    if (float) {
    bswap if needed (reinterpret cast)
    reinterpret same space, now fill with cast to int after scale 0 to 1
    signed = false
    } else if (double) {
    …
    }

    // int cases
    if (internalbpc != 8) {

    // picking:
    // move to be consecutive bytes, bpc = 8

    } else if (bit) {
        scale
    }

    if (interleave) {
     interleave, discard alpha
    } else {
    copy, either skip alpha or not
    ]

    if (signed) {
        read as signed, add -int_min, read as unsigned
    }

    */

    unsigned char *data_out = (unsigned char *)rt->data;
    int pixels = rt->width * rt->height;

    // Truncate to 8 bits
    // 1 for pick last byte, 0 for pick first byte.
    // if data in le -> pick last, data be -> pick first.
    // but there are two branches - if we cast from double/float
    // the data's endianness depends on platform, otherwise
    // on file format, so from bfi.is_little_endian
    int pick_byte;
    unsigned char *buf = (unsigned char *)bfi.communication_buffer();

// The common case for float and double branches, change later otherwise
#if !defined(__BYTE_ORDER) || __BYTE_ORDER == __LITTLE_ENDIAN
    pick_byte = 1;
#else
    pick_byte = 0;
#endif

    if (should_convert_from_float)
    {
        cerr << "endianness of file is little: " << (int)bfi.is_little_endian() << endl;
        if (bfi.is_little_endian() != pick_byte)
        {
            cerr << "swapping\n";
            unsigned int *buf_as_int = (unsigned int *)buf;
            for (int i = 0; i < pixels * channels_internal; i++)
            {
                buf_as_int[i] = ((buf_as_int[i] & 0xFF000000) >> 24) | ((buf_as_int[i] & 0xFF0000) >> 8) | ((buf_as_int[i] & 0xFF00) << 8) | (buf_as_int[i] & 0xFF) << 24;
            }
        }

        float *buf_as_float = (float *)buf;

        for (int i = 0; i < pixels * channels_internal; i++)
        {
            buf[i] = (unsigned char)(buf_as_float[i] * 255.0);
        }
        bytespc_internal = 1;
    }
    else if (should_convert_from_double)
    {
        if (bfi.is_little_endian() != pick_byte)
        {
            unsigned long int *buf_as_long_int = (unsigned long int *)buf;
            for (int i = 0; i < pixels * channels_internal; i++)
            {
                buf_as_long_int[i] = ((buf_as_long_int[i] >> 56) & 0xFFlu) | ((buf_as_long_int[i] >> 40) & 0xFF00lu) | ((buf_as_long_int[i] >> 24) & 0xFF0000lu) | ((buf_as_long_int[i] >> 8) & 0xFF000000lu) | ((buf_as_long_int[i] << 8) & 0xFF00000000lu) | ((buf_as_long_int[i] << 24) & 0xFF0000000000lu) | ((buf_as_long_int[i] << 40) & 0xFF000000000000lu) | ((buf_as_long_int[i] << 56) & 0xFF00000000000000lu);
            }
        }

        double *buf_as_double = (double *)buf;

        for (int i = 0; i < pixels * channels_internal; i++)
        {
            buf[i] = (unsigned char)((float)(buf_as_double[i]) * 255.0);
        }
        bytespc_internal = 1;
    }

    // int cases
    if (bytespc_internal != 1)
    {
        pick_byte = bfi.is_little_endian();

        int coefficient = bytespc_internal;
        int offset = pick_byte ? (coefficient - 1) : 0;
        fprintf(stderr, "coeff %d offset %d", coefficient, offset);
        for (int i = 0; i < pixels * channels_internal; i++)
        {
            // Unnecessary copy rather than considering these offset and coefficient
            // variables when we'll already copy, but this allows readability
            // and not making the common 8-bit reading slower
            buf[i] = buf[coefficient * i + offset];
        }

        bytespc_internal = 1;
    }
    else if (should_convert_from_bit)
    {
        for (int i = 0; i < pixels * channels_internal; i++)
        {
            // 0 -> 0
            // 1 -> 255
            buf[i] = (0 - buf[i]);
        }
    }

    if (should_interleave)
    {
        char *red = bfi.communication_buffer();
        char *green = &bfi.communication_buffer()[pixels];
        char *blue = &bfi.communication_buffer()[2 * pixels];

        for (int i = 0; i < pixels; i++)
        {
            data_out[3 * i] = red[i];
        }
        for (int i = 0; i < pixels; i++)
        {
            data_out[3 * i + 1] = green[i];
        }
        for (int i = 0; i < pixels; i++)
        {
            data_out[3 * i + 2] = blue[i];
        }
    }
    else
    {
        if (should_reduce_channels_from_4to3)
        {
            for (int i = 0; i < pixels; i++)
            {
                // This can be optimized I think with uint64 operations?
                // Depends on compiler
                data_out[3 * i] = bfi.communication_buffer()[4 * i];
                data_out[3 * i + 1] = bfi.communication_buffer()[4 * i + 1];
                data_out[3 * i + 2] = bfi.communication_buffer()[4 * i + 2];
            }
        }
        else
        {
            memcpy(rt->data, bfi.communication_buffer(), bytes_received);
        }
    }

    if (should_remove_sign)
    {
        for (int i = 0; i < pixels * 3; i++)
        {
            data_out[i] += 128;
        }
    }

#ifdef DEBUG_OSI
    logfile << "BioFormats :: getNativeTile() :: read_region() :: " << tilex << "x" << tiley << "@" << iipres << " " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

#ifdef DEBUG_OSI
    timer.start();
#endif

    // TODO: If we have to do color conversion, here is a good place
    // For OpenSlide:
    // COLOR CONVERT in place BGRA->RGB conversion
    // this->bgra2rgb(reinterpret_cast<uint8_t *>(rt->data), tw, th);

#ifdef DEBUG_OSI
    logfile << "BioFormats :: color conversions :: " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

    // and return it.
    return rt;
}

/**
 * @detail  return from the local cache a tile.
 *          The tile may be native (directly from file),
 *                previously downsampled and cached,
 *                downsampled from existing cached tile (and cached now for later use), or
 *                downsampled from native tile (from file.  both native and downsampled tiles will be cached for later use)
 *
 *  note that tilex and tiley can be found by multiplying by 2 raised to power of the difference in levels.
 *  2 versions - direct and recursive.  direct should have slightly lower latency.

 * This function
 * automatically downsample a region in the missing zoom level z, if needed.
 * Note that z is not the openslide layer, but the desired zoom level, because
 * the slide may not have all the layers that correspond to all the
 * zoom levels. The number of layers is equal or less than the number of
 * zoom levels in an equivalent zoomify format.
 * This downsampling method simply does area averaging. If interpolation is desired,
 * an image processing library could be used.

 * go to next level in size, get 4 tiles, downsample and compose.
 *
 * call 4x (getCachedTile at next res, downsample, compose),
 * store in cache, and return tile.  (causes recursion, stops at native layer or in cache.)
 */
RawTilePtr BioFormatsImage::halfsampleAndComposeTile(const size_t tilex, const size_t tiley, const uint32_t iipres)
{
#ifdef DEBUG_VERBOSE
    cerr << "BioFormatsImage::halfsampleAndComposeTile called\n";
#endif
    // not in cache and not a native tile, so create one from higher sampling.
#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif

    // compute the parameters (i.e. x and y offsets, w/h, and bestlayer to use.
    uint32_t osi_level = numResolutions - 1 - iipres;

#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsampleAndComposeTile() :: zoom=" << osi_level << " from " << (osi_level - 1) << endl;
#endif

    size_t ntlx = numTilesX[osi_level];
    size_t ntly = numTilesY[osi_level];

    // compute the correct width and height
    size_t tw = tile_width;
    size_t th = tile_height;

    // Get the width and height for last row and column tiles
    size_t rem_x = this->lastTileXDim[osi_level];
    size_t rem_y = this->lastTileYDim[osi_level];

    // Alter the tile size if it's in the rightmost column
    if ((tilex == ntlx - 1) && (rem_x != 0))
    {
        tw = rem_x;
    }
    // Alter the tile size if it's in the bottom row
    if ((tiley == ntly - 1) && (rem_y != 0))
    {
        th = rem_y;
    }

    // allocate raw tile.
    RawTilePtr rt(new RawTile(tiley * ntlx + tilex, iipres, 0, 0, tw, th, channels, bpc));

    // compute the size, etc
    rt->dataLength = tw * th * 3;
    rt->filename = getImagePath();
    rt->timestamp = timestamp;

    // new a block that is larger for openslide library to directly copy in.
    // then do color operations.  relying on delete [] to do the right thing.
    rt->data = new unsigned char[rt->dataLength];
    rt->memoryManaged = 1; // allocated data, so use this flag to indicate that it needs to be cleared on destruction
                           // rawtile->padded = false;
#ifdef DEBUG_OSI
    logfile << "Allocating tw * th * channels * sizeof(char) : " << tw << " * " << th << " * " << channels << " * sizeof(char) " << endl
            << flush;
#endif

    // new iipres  - next res.  recall that larger res corresponds to higher mag, with largest res being max resolution.
    uint32_t tt_iipres = iipres + 1;
    RawTilePtr tt;
    // temp storage.
    uint8_t *tt_data = new uint8_t[(tile_width >> 1) * (tile_height >> 1) * channels];
    size_t tt_out_w, tt_out_h;

    // uses 4 tiles to create new.
    for (int j = 0; j < 2; ++j)
    {

        size_t tty = tiley * 2 + j;

        if (tty >= numTilesY[osi_level - 1])
            break; // at edge, this may not be a 2x2 block.

        for (int i = 0; i < 2; ++i)
        {
            // compute new tile x and y and iipres.
            size_t ttx = tilex * 2 + i;
            if (ttx >= numTilesX[osi_level - 1])
                break; // at edge, this may not be a 2x2 block.

#ifdef DEBUG_OSI
            logfile << "BioFormats :: halfsampleAndComposeTile() :: call getCachedTile " << endl
                    << flush;
#endif

            // get the tile
            tt = getCachedTile(ttx, tty, tt_iipres);

            if (tt)
            {

                // cache the next res tile

#ifdef DEBUG_OSI
                timer.start();
#endif

                // cache it
                tileCache->insert(tt); // copy is made?

#ifdef DEBUG_OSI
                logfile << "BioFormats :: halfsampleAndComoseTile() :: cache insert res " << tt_iipres << " " << ttx << "x" << tty << " :: " << timer.getTime() << " microseconds" << endl
                        << flush;
#endif

                // downsample into a temp storage.
                halfsample_3(reinterpret_cast<uint8_t *>(tt->data), tt->width, tt->height,
                             tt_data, tt_out_w, tt_out_h);

                // compose into raw tile.  note that tile 0,0 in a 2x2 block always have size tw/2 x th/2
                compose(tt_data, tt_out_w, tt_out_h, (tile_width / 2) * i, (tile_height / 2) * j,
                        reinterpret_cast<uint8_t *>(rt->data), tw, th);
            }
#ifdef DEBUG_OSI
            logfile << "BioFormats :: halfsampleAndComposeTile() :: called getCachedTile " << endl
                    << flush;
#endif
        }
    }
    delete[] tt_data;
#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsampleAndComposeTile() :: downsample " << osi_level << " from " << (osi_level - 1) << " :: " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

    // and return it.
    return rt;
}

/**
 * performs 1/2 size downsample on rgb 24bit images
 * @details 	usingg the following property,
 * 					(a + b) / 2 = ((a ^ b) >> 1) + (a & b)
 * 				which does not cause overflow.
 *
 * 				mask with 0xFEFEFEFE to revent underflow during bitshift, allowing 4 compoents
 * 				to be processed concurrently in same register without SSE instructions.
 *
 * 				output size is computed the same way as the virtual level dims - round down
 * 				if not even.
 *
 * @param in
 * @param in_w
 * @param in_h
 * @param out
 * @param out_w
 * @param out_h
 * @param downSamplingFactor      always power of 2.
 */
// TODO: just use opensldie code?
void BioFormatsImage::halfsample_3(const uint8_t *in, const size_t in_w, const size_t in_h,
                                   uint8_t *out, size_t &out_w, size_t &out_h)
{
#ifdef DEBUG_VERBOSE
    cerr << "BioFormats::halfsample_3 called\n";
#endif

#ifdef DEBUG_OSI
    logfile
        << "BioFormats :: halfsample_3() :: start :: in " << (void *)in << " out " << (void *)out << endl
        << flush;
    logfile << "                            :: in wxh " << in_w << "x" << in_h << endl
            << flush;
#endif

    // do one 1/2 sample run
    out_w = in_w >> 1;
    out_h = in_h >> 1;

    if ((out_w == 0) || (out_h == 0))
    {
        logfile << "BioFormats :: halfsample_3() :: ERROR: zero output width or height " << endl
                << flush;
        return;
    }

    if (!(in))
    {
        logfile << "BioFormats :: halfsample_3() :: ERROR: null input " << endl
                << flush;
        return;
    }

    uint8_t const *row1, *row2;
    uint8_t *dest = out; // if last recursion, put in out, else do it in place

#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsample_3() :: top " << endl
            << flush;
#endif

    // walk through all pixels in output, except last.
    size_t max_h = out_h - 1,
           max_w = out_w;
    size_t inRowStride = in_w * channels;
    size_t inRowStride2 = 2 * inRowStride;
    size_t inColStride2 = 2 * channels;
    // skip last row, as the very last dest element may have overflow.
    for (size_t j = 0; j < max_h; ++j)
    {
        // move row pointers forward 2 rows at a time - in_w may not be multiple of 2.
        row1 = in + j * inRowStride2;
        row2 = row1 + inRowStride;

        for (size_t i = 0; i < max_w; ++i)
        {
            *(reinterpret_cast<uint32_t *>(dest)) = halfsample_kernel_3(row1, row2);
            // output is contiguous.
            dest += channels;
            row1 += inColStride2;
            row2 += inColStride2;
        }
    }

#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsample_3() :: last row " << endl
            << flush;
#endif

    // for last row, skip the last element
    row1 = in + max_h * inRowStride2;
    row2 = row1 + inRowStride;

    --max_w;
    for (size_t i = 0; i < max_w; ++i)
    {
        *(reinterpret_cast<uint32_t *>(dest)) = halfsample_kernel_3(row1, row2);
        dest += channels;
        row1 += inColStride2;
        row2 += inColStride2;
    }

#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsample_3() :: last one " << endl
            << flush;
#endif

    // for last pixel, use memcpy to avoid writing out of bounds.
    uint32_t v = halfsample_kernel_3(row1, row2);
    memcpy(dest, reinterpret_cast<uint8_t *>(&v), channels);

    // at this point, in has been averaged and stored .
    // since we stride forward 2 col and rows at a time, we don't need to worry about overwriting an unread pixel.
#ifdef DEBUG_OSI
    logfile << "BioFormats :: halfsample_3() :: done" << endl
            << flush;
#endif
}

// in is contiguous, out will be when done.
// TODO: use openslide code?
void BioFormatsImage::compose(const uint8_t *in, const size_t in_w, const size_t in_h,
                              const size_t &xoffset, const size_t &yoffset,
                              uint8_t *out, const size_t &out_w, const size_t &out_h)
{
#ifdef DEBUG_VERBOSE
    fprintf(stderr, "compose called. channels %d\n", channels);
#endif
#ifdef DEBUG_OSI
    logfile << "BioFormats :: compose() :: start " << endl
            << flush;
#endif

    if ((in_w == 0) || (in_h == 0))
    {
#ifdef DEBUG_OSI
        logfile << "BioFormats :: compose() :: zero width or height " << endl
                << flush;
#endif
        return;
    }
    if (!(in))
    {
#ifdef DEBUG_OSI
        logfile << "BioFormats :: compose() :: nullptr input " << endl
                << flush;
#endif
        return;
    }

    if (out_h < yoffset + in_h)
    {
        logfile << "COMPOSE ERROR: out_h, yoffset, in_h: " << out_h << "," << yoffset << "," << in_h << endl;
        assert(out_h >= yoffset + in_h);
    }
    if (out_w < xoffset + in_w)
    {
        logfile << "COMPOSE ERROR: out_w, xoffset, in_w: " << out_w << "," << xoffset << "," << in_w << endl;
        assert(out_w >= xoffset + in_w);
    }

    size_t dest_stride = out_w * channels;
    size_t src_stride = in_w * channels;

    uint8_t *dest = out + yoffset * dest_stride + xoffset * channels;
    uint8_t const *src = in;

    for (int k = 0; k < in_h; ++k)
    {
        memcpy(dest, src, in_w * channels);
        dest += dest_stride;
        src += src_stride;
    }

#ifdef DEBUG_OSI
    logfile << "BioFormats :: compose() :: start " << endl
            << flush;
#endif
}
