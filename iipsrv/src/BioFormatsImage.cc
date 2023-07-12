#include "BioFormatsImage.h"
#include "Timer.h"
#include <cmath>
#include <sstream>

#include <cstdlib>
#include <cassert>

#include <cstdio>

#include <limits>

#define throw(a)

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

    if (graal_thread == NULL)
    {
        logfile << "ERROR: can't open " << filename << " with BioFormats" << endl
                << flush;
        throw file_error(string("Error opening '" + filename + "' with BioFormats"));
    }

    fprintf(stderr, "dddBioFormatsImage.cc entering file\n");
    if (!bf_open(graal_thread, (char *)filename.c_str()))
    {
        fprintf(stderr, "dddBioFormatsImage.cc cant enter file:\n");

        const char *error = bf_get_error(graal_thread);

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

/// given an open OSI file, get information from the image.
void BioFormatsImage::loadImageInfo(int x, int y) throw(file_error)
{

#ifdef DEBUG_OSI
    logfile << "BioFormatsImage :: loadImageInfo()" << endl;

    if (!graal_thread)
    {
        logfile << "Not ready" << endl;
    }
#endif

    int w = 0, h = 0;
    currentX = x;
    currentY = y;

    const char *prop_val;

    // choose power of 2 to make downsample simpler.
    tile_width = bf_get_optimal_tile_width(graal_thread);
    if (tile_width < 0 || tile_width & (tile_width - 1))
    {
        tile_width = 256;
    }

    tile_height = bf_get_optimal_tile_height(graal_thread);
    if (tile_height < 0 || tile_height & (tile_height - 1))
    {
        tile_height = 256;
    }

    w = bf_get_size_x(graal_thread);
    h = bf_get_size_y(graal_thread);

    if (w < 0 || h < 0)
    {
        const char *err = bf_get_error(graal_thread);

        logfile << "ERROR: encountered error: " << err << " while getting level0 dim" << endl;
        throw "Getting bioformats level0 dimensions: " + std::string(err);
    }

#ifdef DEBUG_OSI
    logfile << "dimensions :" << w << " x " << h << endl;
    // logfile << "comment : " << comment << endl;
#endif

    channels = bf_get_rgb_channel_count(graal_thread);

    if (channels != 3)
    {
        // TODO: Allow RGBA
        if (channels > 0)
        {
            logfile << "Unimplemented: only support 3 channels, not " << channels << endl;
            throw "Unimplemented: only support 3 channels, not " + channels;
        }
        else
        {
            const char *err = bf_get_error(graal_thread);
            logfile << "Error while getting channel count: " << err << endl;
            throw "Error while getting channel count: " + std::string(err);
        }
    }

    if (!bf_is_interleaved(graal_thread))
    {
        // TODO: find an example file, add java code to open all planes then
        // interleave them before giving them to C. Or do it in C.
        // Maybe code for the case that bpc is a multiple of 8, reject otherwise
        logfile << "Unimplemented: iipsrv requires uninterleaved" << endl;
        throw "Unimplemented: iipsrv requires uninterleaved";
    }

    if (!bf_is_little_endian(graal_thread))
    {
        // TODO: note somewhere in the class whether to swap
        // and when getting tiles, swap
        logfile << "Unimplemented: endian swapping" << endl;
        throw "Unimplemented: endian swapping";
    }

    if (bf_is_floating_point(graal_thread))
    {
        // TODO. It should be easier to handle this when bf_floating_point_is_normalized == true
        logfile << "Unimplemented: floating point reading" << endl;
        throw "Unimplemented: floating point reading";
    }

    if (bf_get_dimension_order(graal_thread) && bf_get_dimension_order(graal_thread)[2] != 'C')
    {
        logfile << "Unimplemented: unfamiliar dimension order " << bf_get_dimension_order(graal_thread) << endl;
        throw "Unimplemented: unfamiliar dimension order " + std::string(bf_get_dimension_order(graal_thread));
    }

    bpc = bf_get_bits_per_pixel(graal_thread) / channels;
    colourspace = sRGB;

    if (bpc != 8)
    {
        // TODO: downsample or keep as is
        throw "Unimplemented: bpc " + std::to_string(bpc) + "is not 8";
    }

    if (bpc <= 0)
    {
        const char *err = bf_get_error(graal_thread);
        logfile << "Error while getting bits per pixel: " << err << endl;
        throw "Error while getting bits per pixel: " + std::string(err);
    }

    // save the openslide dimensions.
    std::vector<int> bioformats_widths, bioformats_heights;
    bioformats_widths.clear();
    bioformats_heights.clear();

    int bioformats_levels = bf_get_resolution_count(graal_thread);
    if (bioformats_levels <= 0)
    {
        const char *err = bf_get_error(graal_thread);
        logfile << "ERROR: encountered error: " << err << " while getting level count" << endl;
        throw "ERROR: encountered error: " + std::string(err) + " while getting level count";
    }

#ifdef DEBUG_OSI
    logfile << "number of levels = " << bioformats_levels << endl;
    double tempdownsample;
#endif

    int ww, hh;
    for (int i = 0; i < bioformats_levels; i++)
    {
        bf_set_current_resolution(graal_thread, i);
        ww = bf_get_size_x(graal_thread);
        hh = bf_get_size_y(graal_thread);
        if (ww <= 0 || hh <= 0)
        {
            logfile << "ERROR: encountered error: while getting level dims for level " << i << endl;
            throw "error while getting level dims for level " + i;
        }
        bioformats_widths.push_back(ww);
        bioformats_heights.push_back(hh);
#ifdef DEBUG_OSI
        tempdownsample = ((double)(w) / ww + (double)(h) / hh) / 2;
        logfile << "\tlevel " << i << "\t(w,h) = (" << ww << "," << hh << ")\tdownsample=" << tempdownsample << endl;
#endif
    }

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
    while ((w > tile_width) || (h > tile_height))
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
}

/// Overloaded function for closing a TIFF image
void BioFormatsImage::closeImage()
{
#ifdef DEBUG_OSI
    Timer timer;
    timer.start();
#endif
    fprintf(stderr, "Calling bfclose.cc in BioFormatsImage::closeImage\n");
    bf_close(graal_thread, 0);
    fprintf(stderr, "Called bfclose.cc in BioFormatsImage::closeImage\n");

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
    fprintf(stderr, "BioFormats::getTile called: iipres %d numResolutions %d\m", iipres, numResolutions);
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

    int64_t layer_width = 0;
    int64_t layer_height = 0;
    bf_set_current_resolution(graal_thread, layers);
    layer_width = bf_get_size_x(graal_thread);
    layer_height = bf_get_size_y(graal_thread);

#ifdef DEBUG_VERBOSE
    fprintf(stderr, "layer: %d layer_width: %d layer_height: %d", layers, layer_width, layer_height);
#endif

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

    /*if (!graal_thread)
    {
        // TODO: should we check if file really opened here?
        // currently, we're checking if graal initialized only
        // to avoid performance penalty
        logfile << "bioformats image not yet loaded " << endl;
    }*/
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

    // create the RawTile object
    RawTilePtr rt(new RawTile(tiley * ntlx + tilex, iipres, 0, 0, tw, th, channels, bpc));

    // compute the size, etc
    rt->dataLength = tw * th * channels * sizeof(unsigned char);
    rt->filename = getImagePath();
    rt->timestamp = timestamp;

    // TODO: sampletype can be implemented here, for float support
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/RawTile.h#L118
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/IIPImage.h#L123
    // https://github.com/camicroscope/iipImage/blob/030c8df59938089d431902f56461c32123298494/iipsrv/src/RawTile.h#L181

    // new a block ...
    // relying on delete [] to do the right thing.
    rt->data = new unsigned char[tw * th * channels * sizeof(unsigned char)];
    rt->memoryManaged = 1; // allocated data, so use this flag to indicate that it needs to be cleared on destruction
                           // rawtile->padded = false;
#ifdef DEBUG_OSI
    logfile << "Allocating tw * th * channels * sizeof(char) : " << tw << " * " << th << " * " << channels << " * sizeof(char) " << endl
            << flush;
#endif

    // READ FROM file

    //======= next compute the x and y coordinates (top left corner) in level 0 coordinates
    //======= expected by bf_open_bytes.
    // TODOOO doubt: Do we need shift here?
    int tx0 = (tilex * tile_width) << osi_level; // same as multiply by z power of 2
    int ty0 = (tiley * tile_height) << osi_level;

    bf_set_current_resolution(graal_thread, bestLayer);
#ifdef DEBUG_VERBOSE
    cerr << "bf_open_bytes params: " << bestLayer << tx0 << ty0 << tw << th;
#endif
    if (!bf_open_bytes(graal_thread, tx0, ty0, tw, th))
    {
        const char *error = bf_get_error(graal_thread);
        logfile << "ERROR: encountered error: " << error << " while reading region exact at  " << tx0 << "x" << ty0 << " dim " << tw << "x" << th << " with BioFormats: " << error << endl;
        throw "ERROR: encountered error: " + std::string(error) + " while reading region exact at " + std::to_string(tx0) + "x" + std::to_string(ty0) + " dim " + std::to_string(tw) + "x" + std::to_string(th) + " with BioFormats: " + std::string(error);
    }

#ifdef DEBUG_OSI
    logfile << "BioFormats :: getNativeTile() :: read_region() :: " << tilex << "x" << tiley << "@" << iipres << " " << timer.getTime() << " microseconds" << endl
            << flush;
#endif

    if (!rt->data)
        throw string("FATAL : BioFormatsImage read_region => allocation memory ERROR");

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
    rt->dataLength = tw * th * channels;
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
    fprintf(stderr, "compose called\n");
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
