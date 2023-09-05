// IIPImage.cc 


/*  IIP fcgi server module

    Copyright (C) 2000-2014 Ruven Pillay.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "IIPImage.h"
#include <stdio.h>
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#if _MSC_VER
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <limits>

#include "openslide.h"

using namespace std;



// Swap function
void IIPImage::swap( IIPImage& first, IIPImage& second ) // nothrow
{
  // Swap the members of the two objects
  std::swap( first.imagePath, second.imagePath );
  std::swap( first.isFile, second.isFile );
  std::swap( first.suffix, second.suffix );
  std::swap( first.virtual_levels, second.virtual_levels );
  std::swap( first.format, second.format );
  std::swap( first.fileSystemPrefix, second.fileSystemPrefix );
  std::swap( first.fileNamePattern, second.fileNamePattern );
  std::swap( first.horizontalAnglesList, second.horizontalAnglesList );
  std::swap( first.verticalAnglesList, second.verticalAnglesList );
  std::swap( first.image_widths, second.image_widths );
  std::swap( first.image_heights, second.image_heights );
  std::swap( first.tile_width, second.tile_width );
  std::swap( first.tile_height, second.tile_height );
  std::swap( first.numResolutions, second.numResolutions );
  std::swap( first.bpc, second.bpc );
  std::swap( first.channels, second.channels );
  std::swap( first.sampleType, second.sampleType );
  std::swap( first.quality_layers, second.quality_layers );
  std::swap( first.colourspace, second.colourspace );
  std::swap( first.isSet, second.isSet );
  std::swap( first.currentX, second.currentX );
  std::swap( first.currentY, second.currentY );
  std::swap( first.metadata, second.metadata );
  std::swap( first.timestamp, second.timestamp );
  std::swap( first.min, second.min );
  std::swap( first.max, second.max );
}



void IIPImage::testImageType() throw(file_error)
{
  // Check whether it is a regular file
  struct stat sb;

  string path = fileSystemPrefix + imagePath;

  if( (stat(path.c_str(),&sb)==0) && S_ISREG(sb.st_mode) ){

    isFile = true;
    int dot = imagePath.find_last_of( "." );
    suffix = imagePath.substr( dot + 1, imagePath.length() );
    timestamp = sb.st_mtime;

    // Determine our file format using magic file signatures
    unsigned char header[10];
    FILE *im = fopen( path.c_str(), "rb" );
    if( im == NULL ){
      string message = "Unable to open file '" + path + "'";
      throw file_error( message );
    }

    // Read and close immediately
    int len = fread( header, 1, 10, im );
    fclose( im );

    // Make sure we were able to read enough bytes
    if( len < 10 ){
      string message = "Unable to read initial byte sequence from file '" + path + "'";
      throw file_error( message );
    }

    // Magic file signature for JPEG2000
    unsigned char j2k[10] = {0x00,0x00,0x00,0x0C,0x6A,0x50,0x20,0x20,0x0D,0x0A};

    // Magic file signatures for TIFF (See http://www.garykessler.net/library/file_sigs.html)
    unsigned char stdtiff[3] = {0x49,0x20,0x49};       // TIFF
    unsigned char lsbtiff[4] = {0x49,0x49,0x2A,0x00};  // Little Endian TIFF
    unsigned char msbtiff[4] = {0x49,0x49,0x2A,0x00};  // Big Endian TIFF
    unsigned char lbigtiff[4] = {0x4D,0x4D,0x00,0x2B}; // Little Endian BigTIFF
    unsigned char bbigtiff[4] = {0x49,0x49,0x2B,0x00}; // Big Endian BigTIFF

    // OpenSlide
__builtin_fprintf(stderr, "1\n");
    {
      const char * vendor = openslide_detect_vendor( path.c_str() );
      if ( vendor != NULL ) {
__builtin_fprintf(stderr, "2\n");

        if ( !strcmp(vendor, "generic-tiff") ) {
          // Have generic TIFF, so use iipsrv reader
__builtin_fprintf(stderr, "3\n");

          format = TIF;
          return;
        }
        // OpenSlide but not generic tiff
        format = OPENSLIDE;
__builtin_fprintf(stderr, "4\n");

        return;
      }
    }

    // BioFormats
    /*{
__builtin_fprintf(stderr, "5\n");

      BioFormatsInstance bfi = BioFormatsManager::get_new();
__builtin_fprintf(stderr, "6\n");

      int code = bfi.is_compatible( path );
      //  1 -> compatible
      //  0 -> incompatible
      // -1 -> error
      if ( code == 1 ) {
__builtin_fprintf(stderr, "7\n");

        format = BIOFORMATS;
        return;
      }
__builtin_fprintf(stderr, "8\n");

      BioFormatsManager::free( std::move(bfi) );
    }*/

    // IIPsrv builtin
    {
      if( memcmp( header, j2k, 10 ) == 0 ) {
        format = JPEG2000;
        return;
      }
      else if( memcmp( header, stdtiff, 3 ) == 0
        || memcmp( header, lsbtiff, 4 ) == 0 || memcmp( header, msbtiff, 4 ) == 0
        || memcmp( header, lbigtiff, 4 ) == 0 || memcmp( header, bbigtiff, 4 ) == 0 ){
        format = TIF;
        return;
      }
    }
__builtin_fprintf(stderr, "9\n");

    format = UNSUPPORTED;

  }
  else{

#ifdef HAVE_GLOB_H

    // Check for sequence
    glob_t gdat;
    string filename = path + fileNamePattern + "000_090.*";

    if( glob( filename.c_str(), 0, NULL, &gdat ) != 0 ){
      globfree( &gdat );
      string message = path + string( " is neither a file nor part of an image sequence" );
      throw file_error( message );
    }
    if( gdat.gl_pathc != 1 ){
      globfree( &gdat );
      string message = string( "There are multiple file extensions matching " )  + filename;
      throw file_error( message );
    }

    string tmp( gdat.gl_pathv[0] );
    globfree( &gdat );

    isFile = false;

    int dot = tmp.find_last_of( "." );
    int len = tmp.length();

    suffix = tmp.substr( dot + 1, len );
    if (suffix=="vtif" ||
        suffix=="svs" || 
        suffix=="ndpi" || 
        suffix=="mrxs" || 
        suffix=="vms" || 
        suffix=="scn" || 
        suffix=="dcm" || 
        suffix=="bif")
    	format = OPENSLIDE;
    else if (
      suffix == "v3draw" ||
      suffix == "ano" ||
        suffix == "cfg" ||
        suffix == "csv" ||
        suffix == "htm" ||
        suffix == "rec" ||
        suffix == "tim" ||
        suffix == "zpo" ||
        suffix == "tif" ||
        suffix == "dic" ||
        suffix == "dcm" ||
        suffix == "dicom" ||
        suffix == "jp2" ||
        suffix == "j2ki" ||
        suffix == "j2kr" ||
        suffix == "raw" ||
        suffix == "ima" ||
        suffix == "cr2" ||
        suffix == "crw" ||
        suffix == "jpg" ||
        suffix == "thm" ||
        suffix == "wav" ||
        suffix == "tiff" ||
        suffix == "dv" ||
        suffix == "r3d" ||
        suffix == "r3d_d3d" ||
        suffix == "log" ||
        suffix == "mvd2" ||
        suffix == "aisf" ||
        suffix == "aiix" ||
        suffix == "dat" ||
        suffix == "atsf" ||
        suffix == "tf2" ||
        suffix == "tf8" ||
        suffix == "btf" ||
        suffix == "pbm" ||
        suffix == "pgm" ||
        suffix == "ppm" ||
        suffix == "xdce" ||
        suffix == "xml" ||
        suffix == "xlog" ||
        suffix == "apl" ||
        suffix == "tnb" ||
        suffix == "mtb" ||
        suffix == "im" ||
        suffix == "mea" ||
        suffix == "res" ||
        suffix == "aim" ||
        suffix == "arf" ||
        suffix == "psd" ||
        suffix == "al3d" ||
        suffix == "gel" ||
        suffix == "am" ||
        suffix == "amiramesh" ||
        suffix == "grey" ||
        suffix == "hx" ||
        suffix == "labels" ||
        suffix == "img" ||
        suffix == "hdr" ||
        suffix == "sif" ||
        suffix == "afi" ||
        suffix == "svs" ||
        suffix == "exp" ||
        suffix == "h5" ||
        suffix == "1sc" ||
        suffix == "pic" ||
        suffix == "scn" ||
        suffix == "ims" ||
        suffix == "ch5" ||
        suffix == "vsi" ||
        suffix == "ets" ||
        suffix == "pnl" ||
        suffix == "htd" ||
        suffix == "c01" ||
        suffix == "dib" ||
        suffix == "cxd" ||
        suffix == "v" ||
        suffix == "eps" ||
        suffix == "epsi" ||
        suffix == "ps" ||
        suffix == "flex" ||
        suffix == "xlef" ||
        suffix == "fits" ||
        suffix == "fts" ||
        suffix == "dm2" ||
        suffix == "dm3" ||
        suffix == "dm4" ||
        suffix == "naf" ||
        suffix == "his" ||
        suffix == "ndpi" ||
        suffix == "ndpis" ||
        suffix == "vms" ||
        suffix == "txt" ||
        suffix == "i2i" ||
        suffix == "hed" ||
        suffix == "mod" ||
        suffix == "inr" ||
        suffix == "ipl" ||
        suffix == "ipm" ||
        suffix == "fff" ||
        suffix == "ics" ||
        suffix == "ids" ||
        suffix == "seq" ||
        suffix == "ips" ||
        suffix == "ipw" ||
        suffix == "frm" ||
        suffix == "par" ||
        suffix == "j2k" ||
        suffix == "jpf" ||
        suffix == "jpk" ||
        suffix == "jpx" ||
        suffix == "klb" ||
        suffix == "xv" ||
        suffix == "bip" ||
        suffix == "sxm" ||
        suffix == "fli" ||
        suffix == "lim" ||
        suffix == "msr" ||
        suffix == "lif" ||
        suffix == "lof" ||
        suffix == "lei" ||
        suffix == "l2d" ||
        suffix == "mnc" ||
        suffix == "stk" ||
        suffix == "nd" ||
        suffix == "scan" ||
        suffix == "vff" ||
        suffix == "mrw" ||
        suffix == "stp" ||
        suffix == "mng" ||
        suffix == "nii" ||
        suffix == "nrrd" ||
        suffix == "nhdr" ||
        suffix == "nd2" ||
        suffix == "nef" ||
        suffix == "obf" ||
        suffix == "omp2info" ||
        suffix == "oib" ||
        suffix == "oif" ||
        suffix == "pty" ||
        suffix == "lut" ||
        suffix == "oir" ||
        suffix == "sld" ||
        suffix == "spl" ||
        suffix == "liff" ||
        suffix == "top" ||
        suffix == "pcoraw" ||
        suffix == "pcx" ||
        suffix == "pict" ||
        suffix == "pct" ||
        suffix == "df3" ||
        suffix == "im3" ||
        suffix == "qptiff" ||
        suffix == "bin" ||
        suffix == "env" ||
        suffix == "spe" ||
        suffix == "afm" ||
        suffix == "sm2" ||
        suffix == "sm3" ||
        suffix == "spc" ||
        suffix == "set" ||
        suffix == "sdt" ||
        suffix == "spi" ||
        suffix == "xqd" ||
        suffix == "xqf" ||
        suffix == "db" ||
        suffix == "vws" ||
        suffix == "pst" ||
        suffix == "inf" ||
        suffix == "tfr" ||
        suffix == "ffr" ||
        suffix == "zfr" ||
        suffix == "zfp" ||
        suffix == "2fl" ||
        suffix == "tga" ||
        suffix == "pr3" ||
        suffix == "dti" ||
        suffix == "fdf" ||
        suffix == "hdf" ||
        suffix == "bif" ||
        suffix == "xys" ||
        suffix == "html" ||
        suffix == "acff" ||
        suffix == "wat" ||
        suffix == "bmp" ||
        suffix == "wpi" ||
        suffix == "czi" ||
        suffix == "lms" ||
        suffix == "lsm" ||
        suffix == "mdb" ||
        suffix == "zvi" ||
        suffix == "mrc" ||
        suffix == "st" ||
        suffix == "ali" ||
        suffix == "map" ||
        suffix == "mrcs" ||
        suffix == "jpeg" ||
        suffix == "png" ||
        suffix == "gif" ||
        suffix == "ptif"
    )
      format = BIOFORMATS;
    else if( suffix == "jp2" || suffix == "jpx" || suffix == "j2k" ) format = JPEG2000;
    else if( suffix == "ptif" || suffix == "tif" || suffix == "tiff" ) format = TIF;
    else format = UNSUPPORTED;

    updateTimestamp( tmp );

#else
    string message = path + string( " is not a regular file and no glob support enabled" );
    throw file_error( message );
#endif

  }

}

time_t IIPImage::getFileTimestamp(const string& path) throw(file_error)
{
  // Get a modification time for our image
  struct stat sb;

  if( stat( path.c_str(), &sb ) == -1 ){
    string message = string( "Unable to open file " ) + path;
    throw file_error( message );
  }
  return sb.st_mtime;
}


bool IIPImage::updateTimestamp( const string& path ) throw(file_error)
{
  
  time_t newtime = IIPImage::getFileTimestamp(path);
  double modified = difftime(newtime, timestamp);
  timestamp = newtime;
  if (modified > std::numeric_limits<double>::round_error()) return true;
  else return false;
}



const std::string IIPImage::getTimestamp()
{
  tm *t;
  const time_t tm1 = timestamp;
  t = gmtime( &tm1 );
  char strt[64];
  strftime( strt, 64, "%a, %d %b %Y %H:%M:%S GMT", t );

  return string(strt);
}



void IIPImage::measureVerticalAngles()
{
  verticalAnglesList.clear();

#ifdef HAVE_GLOB_H

  glob_t gdat;
  unsigned int i;

  string filename = fileSystemPrefix + imagePath + fileNamePattern + "000_*." + suffix;
  
  if( glob( filename.c_str(), 0, NULL, &gdat ) != 0 ){
    globfree( &gdat );
  }

  for( i=0; i < gdat.gl_pathc; i++ ){

    // Extract angle no from path name.
    int angle;
    string tmp( gdat.gl_pathv[i] );
    int len = tmp.length() - suffix.length() - 1;
    string sequence_no = tmp.substr( len-3, 3 );
    istringstream(sequence_no) >> angle;
    verticalAnglesList.push_front( angle );
  }

  verticalAnglesList.sort();

  globfree( &gdat );

#endif

}



void IIPImage::measureHorizontalAngles()
{
  horizontalAnglesList.clear();

#ifdef HAVE_GLOB_H

  glob_t gdat;
  unsigned int i;

  string filename = fileSystemPrefix + imagePath + fileNamePattern + "*_090." + suffix;

  if( glob( filename.c_str(), 0, NULL, &gdat ) != 0 ){
    globfree( &gdat );
  }

  for( i=0; i < gdat.gl_pathc; i++ ){

    // Extract angle no from path name.
    int angle;
    string tmp( gdat.gl_pathv[i] );
    int start = string(fileSystemPrefix + imagePath + fileNamePattern).length();
    int end = tmp.find_last_of("_");
    string n = tmp.substr( start, end-start );
    istringstream(n) >> angle;
    horizontalAnglesList.push_front( angle );
  }

  horizontalAnglesList.sort();

  globfree( &gdat );

#endif

}



void IIPImage::Initialise()
{
  testImageType();

  if( !isFile ){
    // Measure sequence angles
    measureHorizontalAngles();

    // Measure vertical view angles
    measureVerticalAngles();
  }
  // If it's a single value, give the view default angles of 0 and 90
  else{
    horizontalAnglesList.push_front( 0 );
    verticalAnglesList.push_front( 90 );
  }

}



const string IIPImage::getFileName( int seq, int ang )
{
  char name[1024];

  if( isFile ){
    return fileSystemPrefix+imagePath;
  }
  else{
    // The angle or spectral band indices should be a minimum of 3 digits when padded
    snprintf( name, 1024,
	      "%s%s%03d_%03d.%s", (fileSystemPrefix+imagePath).c_str(), fileNamePattern.c_str(),
	      seq, ang, suffix.c_str() );
    return string( name );
  }
}



int operator == ( const IIPImage& A, const IIPImage& B )
{
  if( A.imagePath == B.imagePath ) return( 1 );
  else return( 0 );
}



int operator != ( const IIPImage& A, const IIPImage& B )
{
  if( A.imagePath != B.imagePath ) return( 1 );
  else return( 0 );
}


