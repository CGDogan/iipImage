/*
 * File:   BioFormats.h
 * Based on OpenSlideImage.h
 *
 *
 */

#ifndef BIOFORMATSIMAGE_H
#define BIOFORMATSIMAGE_H

#include "IIPImage.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <inttypes.h>
#include <iostream>
#include <fstream>

#include "Cache.h"

#include "graal/bfbridge.h"
#include "graal/graal_isolate.h"

#define throw(a)

class BioFormatsImage : public IIPImage
{
private:
    graal_isolate_t *graal_isolate = NULL;
    graal_isolatethread_t *graal_thread = NULL;

    TileCache *tileCache;

    std::vector<size_t> numTilesX, numTilesY;
    std::vector<size_t> lastTileXDim, lastTileYDim;
    std::vector<int> bioformats_level_to_use, bioformats_downsample_in_level;

    // Unimplemented methods in line with OpenslideImage.h:
    //    void read(...);
    //    void downsample_region(...);

    /**
     * @brief get cached tile.
     * @detail  return from the local cache a tile.
     *          The tile may be native (directly from file),
     *                previously cached,
     *                or downsampled (recursively called.
     *  note that tilex and tiley can be found by multiplying by 2 raised to power of the difference in levels.
     *  2 versions - direct and recursive.  direct should have slightly lower latency.
     *
     * @param tilex
     * @param tiley
     * @param res
     * @return
     */
    /// check if cache has tile.  if yes, return it.  if not, and is a native layer, getNativeTile, else call halfsampleAndComposeTile
    RawTilePtr getCachedTile(const size_t tilex, const size_t tiley, const uint32_t iipres);

    /// read from file, color convert, store in cache, and return tile.
    RawTilePtr getNativeTile(const size_t tilex, const size_t tiley, const uint32_t iipres);

    /// call 4x (getCachedTile at next res, downsample, compose), store in cache, and return tile.  (causes recursion, stops at native layer or in cache.)
    RawTilePtr halfsampleAndComposeTile(const size_t tilex, const size_t tiley, const uint32_t iipres);

    /// average 2 rows, then average 2 pixels
    inline uint32_t halfsample_kernel_3(const uint8_t *r1, const uint8_t *r2)
    {
        uint64_t a = *(reinterpret_cast<const uint64_t *>(r1));
        uint64_t c = *(reinterpret_cast<const uint64_t *>(r2));
        uint64_t t1 = (((a ^ c) & 0xFEFEFEFEFEFEFEFE) >> 1) + (a & c);
        uint32_t t2 = t1 & 0x0000000000FFFFFF;
        uint32_t t3 = (t1 >> 24) & 0x0000000000FFFFFF;
        return (((t3 ^ t2) & 0xFEFEFEFE) >> 1) + (t3 & t2);
    }

    /// downsample by 2
    void halfsample_3(const uint8_t *in, const size_t in_w, const size_t in_h,
                      uint8_t *out, size_t &out_w, size_t &out_h);

    void compose(const uint8_t *in, const size_t in_w, const size_t in_h,
                 const size_t &xoffset, const size_t &yoffset,
                 uint8_t *out, const size_t &out_w, const size_t &out_h);

    /// Constructor
    BioFormatsImage() : IIPImage()
    {
        int code = graal_create_isolate(NULL, &graal_isolate, &graal_thread);
        if (code != 0)
        {
            throw "graal_create_isolate: " + code;
        }
    };

public:
    /// Constructor
    /** \param path image path
     */
    BioFormatsImage(const std::string &path, TileCache *tile_cache) : IIPImage(path), tileCache(tile_cache)
    {
        BioFormatsImage();
        // TODO set tile width on loadimage, not here
    };

    /// Copy Constructor

    /** \param image IIPImage object
     */
    BioFormatsImage(const IIPImage &image, TileCache *tile_cache) : IIPImage(image), tileCache(tile_cache)
    {
        BioFormatsImage();
    };

    /** \param image IIPImage object
     */
    explicit BioFormatsImage(const BioFormatsImage &image) : IIPImage(image),
                                                             // TODO Copy everything including JVM pointers for moving here.
                                                             // osr(image.osr), tileCache(image.tileCache),
                                                             graal_isolate(image.graal_isolate),
                                                             graal_thread(image.graal_thread),
                                                             numTilesX(image.numTilesX),
                                                             numTilesY(image.numTilesY),
                                                             lastTileXDim(image.lastTileXDim),
                                                             lastTileYDim(image.lastTileYDim),
                                                             bioformats_level_to_use(image.bioformats_level_to_use),
                                                             bioformats_downsample_in_level(image.bioformats_downsample_in_level){};
    /// Destructor

    virtual ~BioFormatsImage()
    {
        if (graal_thread)
        {
            closeImage();
            graal_tear_down_isolate(graal_thread);
        }
    };

    virtual void openImage() throw(file_error);

    /// Overloaded function for loading image information
    /** \param x horizontal sequence angle
        \param y vertical sequence angle
     */
    virtual void loadImageInfo(int x, int y) throw(file_error);

    virtual void closeImage();

    /// Overloaded function for getting a particular tile
    /** \param x horizontal sequence angle
        \param y vertical sequence angle
        \param r resolution
        \param l number of quality layers to decode
        \param t tile number
     */
    virtual RawTilePtr getTile(int x, int y, unsigned int r, int l, unsigned int t) throw(file_error);

    // Unimplemented with OpenSlideImage.h:
    //	virtual RawTile getRegion(...);

    //  void readProperties(...);
};

#endif /* BIOFORMATSIMAGE_H */
