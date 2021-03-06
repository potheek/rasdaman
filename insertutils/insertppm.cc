/*
* This file is part of rasdaman community.
*
* Rasdaman community is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Rasdaman community is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009 Peter Baumann /
rasdaman GmbH.
*
* For more information please see <http://www.rasdaman.org>
* or contact Peter Baumann via <baumann@rasdaman.com>.
*/
/**
 * SOURCE: insertppm.cc
 *
 * MODULE: insertutils
 *
 * PURPOSE:
 *   read one or more ppm image file(s) and create a rasdaman
 *   object out of its contents.
 *   If one file is provided, then a 2-D object will be generated.
 *   If several files are provided then a 3-D object will be
 *   generated; to this end, all files have to agree on pixel
 *   type and x/y extent.
 *   If the collection doesn't exist previously, it is created.
 *   If it does exist, new objects will be created in it
 *   (obviously requiring thet the collection type is appropriate).
 *
 * COMMENTS:
 * - Has to be compiled with the following options:
 *
 *     -D__STDC__ -DSYSV -I/usr/local/dist/include
 *     -L/usr/local/dist/lib -lppm -lpgm -lpbm
 *
 *   The defines are necessary so that HP CC can understand the
 *   ppm.h include correctly.
 * - needs PPM package installed.
 *
*/

static const char rcsid[] = "@(#)cachetamgr,readppm: $Header: /home/rasdev/CVS-repository/rasdaman/insertutils/insertppm.cc,v 1.67 2006/01/17 07:36:25 rasdev Exp $";

#include "config.h"
#ifdef EARLY_TEMPLATE
#define __EXECUTABLE__
#ifdef __GNUG__
#include "raslib/template_inst.hh"
#endif
#endif

using namespace std;

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <getopt.h>
#include <list>

#include "raslib/rminit.hh"
#include "rasodmg/marray.hh"
#include "rasodmg/ref.hh"
#include "rasodmg/set.hh"
#include "rasodmg/database.hh"
#include "rasodmg/partinsert.hh"
#include "raslib/type.hh"
#include "raslib/odmgtypes.hh"
#include "raslib/error.hh"

#include "globals.hh"

// this is for inhouse debug macros; not needed for standalone compilation
#if defined(RMANDEBUG) || defined(DEBUG)
#define DEBUG_MAIN
#include "debug.hh"
#endif

#include "raslib/log_config.hh"
#include <easylogging++.h>

extern "C" {
#include <ppm.h>
}

// program exit codes:
#define EXIT_OK     0
#define EXIT_ERROR  -1
#define EXIT_USAGE  -2

// pixel type names
#define PIXEL_TYPE_BOOL     "bin"
#define PIXEL_TYPE_GREY     "grey"
#define PIXEL_TYPE_COLOR    "color"
#define PIXEL_TYPE_UNSIGNED "unsigned"

// pixel sizes in byte:
#define PIXSIZE_BW  sizeof(char)
#define PIXSIZE_GREY    sizeof(char)
#define PIXSIZE_COL sizeof(RGBPixel)
#define PIXSIZE_UNSIGNED    sizeof(unsigned short) // for unsigned 16 bit images

// internal pixel type indicator
typedef enum
{
    PIXEL_BOOL,
    PIXEL_GREY,
    PIXEL_COLOR,
    PIXEL_UNSIGNED
}
PixelType;

// the color image type:
typedef struct
{
    unsigned char red, green, blue;
} RGBPixel;

// --- default values --------------------------------------------
#define DEFAULT_TYPE    "color"
#undef DEFAULT_PORT
#define DEFAULT_PORT    "7001"
#define DEFAULT_HOST    "localhost"
#define DEFAULT_USER    "rasguest"
#define DEFAULT_PASSWD  "rasguest"
#define DEFAULT_VERBOSE false
#define DEFAULT_DB  "RASBASE"
#define DEFAULT_TR_FMT  "Array"
#define DEFAULT_ST_FMT  "Array"
#define DEFAULT_TILE_LO 0
#define DEFAULT_TILE_HI 31
#define DEFAULT_PIXSIZE PIXSIZE_COL
#define DEFAULT_PIXEL_TYPE  PIXEL_COLOR

// --- options --------------------------------------------
// long option names
#define OPTION_HELP "help"
#define OPTION_COLLECTION "collection"
#define OPTION_TYPE "type"
#define OPTION_RESCALE "rescale"
#define OPTION_TILE "tiled"
#define OPTION_SERVER "server"
#define OPTION_PORT "port"
#define OPTION_DATABASE "database"
#define OPTION_USER "user"
#define OPTION_PASSWD "passwd"
#define OPTION_TRANSFERFORMAT "transferformat"
#define OPTION_TRANSFERFORMATPARAMS "transferformatparams"
#define OPTION_STORAGEFORMAT "storageformat"
#define OPTION_STORAGEFORMATPARAMS "storageformatparams"
#define OPTION_VERBOSE "verbose"

// long option descriptor
static struct option long_options[] =
{
    { OPTION_HELP,                 0, 0, 0 },
    { OPTION_COLLECTION,           1, 0, 0 },
    { OPTION_TYPE,                 1, 0, 0 },
    { OPTION_RESCALE,              0, 0, 0 },
    { OPTION_TILE,                 1, 0, 0 },
    { OPTION_SERVER,               1, 0, 0 },
    { OPTION_PORT,                 1, 0, 0 },
    { OPTION_DATABASE,             1, 0, 0 },
    { OPTION_USER,                 1, 0, 0 },
    { OPTION_PASSWD,               1, 0, 0 },
    { OPTION_TRANSFERFORMAT,       1, 0, 0 },
    { OPTION_TRANSFERFORMATPARAMS, 1, 0, 0 },
    { OPTION_STORAGEFORMAT,        1, 0, 0 },
    { OPTION_STORAGEFORMATPARAMS,  1, 0, 0 },
    { OPTION_VERBOSE,              0, 0, 0 },
    { 0, 0, 0, 0 }
};

// short option descriptor
const char *short_options = "hc:t:rs:d:u:p:v";

// --- verbose output --------------------------------------------
// verbosity flag
static bool verbose = DEFAULT_VERBOSE;

// conditional log output
#define INFO if (verbose) cerr

// --- globals --------------------------------------------
// global variables to make readRow work (not nice, I know)
static FILE* readppm_fp=NULL;
static int readppm_colsP=0;      // number of columns
static int readppm_rowsP=0;      // number of rows
static pixval readppm_maxvalP=0; // maximum pixel value (-> pixel depth)
static int readppm_formatP=0;

// --- functions --------------------------------------------

r_Minterval readHeader( const char *fName, bool mdd3d, unsigned int numSlices );

void readImage( char* contents, const char *fName, bool mdd3d, PixelType pixType, bool rescale, r_Range slicenum, r_Range slicepos ) throw (r_Error);

void openImage( const char *fName, bool mdd3d );

void readRows(char* contents, r_Range startRow, r_Range numRows, PixelType pixType, bool rescale);

void createMarray(const r_Minterval &dom, r_Ref<r_GMarray> &mddPtr, PixelType pixType);

void printUsage(const char* name);


/*
 * calculate size of image
 * for a 3D image, numSlices is used to determine the z axis extent
 * preconditions:
 * - fname!=NULL, \0 terminated
 * - imgSize contains valid 2D or 3D minterval, depending on mdd3d
 */
r_Minterval readHeader( const char *fName, bool mdd3d, unsigned int numSlices )
{
    FILE* fp=NULL;
    int colsP=0;         // number of columns
    int rowsP=0;         // number of row
    pixval maxvalP=0;    // maximum pixel value (-> pixel depth)
    int formatP=0;
    r_Minterval result;

    // check if the file exists (pm_openr() just dumps on error, leaving db open)
    std::fstream inFile( fName, std::ios::in );
    if(!inFile)
    {
        cerr << "Error: cannot find input file " << fName << endl;
        throw r_Error( r_Error::r_Error_General );
    }

    fp = pm_openr( fName );
    if(!fp)
    {
        cerr << "Error: cannot find input file " << fName << endl;
        throw r_Error( r_Error::r_Error_General );
    }

    // initialize colsP and rowsP to create MDD
    ppm_readppminit( fp, &colsP, &rowsP, &maxvalP, &formatP );
    pm_close(fp);

    if(mdd3d)
    {
        // domain of MDD
        result = r_Minterval(3)
                 << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(numSlices - 1))
                 << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(colsP - 1))
                 << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(rowsP - 1));
    }
    else
    {
        // domain of MDD
        result = r_Minterval(2)
                 << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(colsP) - 1)
                 << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(rowsP) - 1);
    }

    INFO << "header says: " << result << endl;
    return result;
}

/*
 * read one image file
 * preconditions:
 * - fname!=NULL, \0 terminated
 */
#pragma GCC diagnostic ignored "-Wsign-conversion"
void readImage(char* contents, const char *fName,__attribute__ ((unused))  bool mdd3d, PixelType pixType, bool rescale, __attribute__ ((unused)) r_Range slicenum = 0, r_Range slicepos = 0 ) throw (r_Error)
{
    FILE* fp=NULL;      // input image file
    int colsP=0;        // number of columns
    int rowsP=0;        // number of rows
    pixval maxvalP;     // maximum pixel value (-> pixel depth)
    pixval newMaxvalP = 255;
    pixel** pixelsP=NULL;   // all pixels in the image
    pixel aPixel;       // one pixel
    pixel scaledPixel;  // pixel scaled to 0 to 255

    // open PPM file
    fp = pm_openr(fName);
    if(!fp)
    {
        cerr << "iError: cannot find input file " << fName << endl;
        throw r_Error( r_Error::r_Error_General );
    }

    // read it (also initializes information about picture)
    pixelsP = ppm_readppm( fp, &colsP, &rowsP, &maxvalP );

    unsigned long long pos;
    // iterate through all pixels
    int i=0, j=0;
    for(i = 0; i < rowsP; i++)
    {
        for(j = 0; j < colsP; j++)
        {
            // read pixel
            aPixel = pixelsP[i][j];
            // scale pixel to 0-255
            if(rescale)
                PPM_DEPTH( scaledPixel, aPixel, maxvalP, newMaxvalP );
            else
                scaledPixel = aPixel;
            pos = static_cast<unsigned long long>(slicepos*colsP*rowsP + j*rowsP + i);

            switch (pixType)
            {
            case PIXEL_BOOL:
                contents[pos*PIXSIZE_BW] = PPM_GETB( scaledPixel ) == 0 ? 0 : 1;
                break;
            case PIXEL_GREY: // create Char out of pixel (just take blue)
                contents[pos*PIXSIZE_GREY] = PPM_GETB( scaledPixel );
                break;
            case PIXEL_COLOR: // create ULong out of pixel colors
                contents[pos*PIXSIZE_COL + 0] = PPM_GETR( scaledPixel );
                contents[pos*PIXSIZE_COL + 1] = PPM_GETG( scaledPixel );
                contents[pos*PIXSIZE_COL + 2] = PPM_GETB( scaledPixel );
                break;
            case PIXEL_UNSIGNED:
                contents[pos*PIXSIZE_UNSIGNED + 0] = static_cast<char>(0xFF & PPM_GETB( scaledPixel ) );
                contents[pos*PIXSIZE_UNSIGNED + 1] = static_cast<char>(PPM_GETB( scaledPixel ) >> 8 );
                break;
            default:
                cerr << "Error: unknown pixel type code: " << static_cast<int>(pixType) << endl;
                break;
            }
        }
    }

    // close PPM and free memory
    pm_close(fp);
    ppm_freearray( pixelsP, rowsP );
}
#pragma GCC diagnostic warning "-Wsign-conversion"

/*
 * open the image file; no reading is done
 * preconditions:
 * - fname!=NULL, \0 terminated
 */
void openImage( const char *fName, __attribute__ ((unused)) bool mdd3d )
{
    // open PPM file
    readppm_fp = pm_openr( fName );
    if(!readppm_fp)
    {
        cerr << "Error: cannot find input file " << fName  << endl;
        throw r_Error( r_Error::r_Error_General );
    }

    ppm_readppminit( readppm_fp, &readppm_colsP, &readppm_rowsP, &readppm_maxvalP, &readppm_formatP );
}

/*
 * read numRows rows in an image file
 */
void readRows(char* contents, __attribute__ ((unused)) r_Range startRow, r_Range numRows, PixelType pixType, bool rescale)
{
    pixel aPixel;       // one pixel
    pixel scaledPixel;  // pixel scaled to range 0..255
    pixel* row=NULL;    // all pixels in one row
    unsigned int currRow=0; // current row #
    unsigned long long pos;
    row = new pixel[readppm_colsP];

    for(currRow = 0; currRow < numRows; currRow++)
    {
        // read a row
        ppm_readppmrow(readppm_fp, row, readppm_colsP, readppm_maxvalP, readppm_formatP);

        // iterate through all pixels in this row
        for( int i = 0; i < readppm_colsP; i++ )
        {
            // read pixel
            aPixel = row[i];

            // scale pixel to 0-255
            if(rescale)
                PPM_DEPTH( scaledPixel, aPixel, readppm_maxvalP, 255 );
            else
                scaledPixel = aPixel;

            pos = static_cast<unsigned long long>(i*numRows + currRow);

            switch (pixType)
            {
            case PIXEL_BOOL:
                contents[pos*PIXSIZE_BW] = (PPM_GETB(scaledPixel)==0) ? 0 : 1;
                break;
            case PIXEL_GREY: // create Bool out of pixel (just take blue)
                contents[pos*PIXSIZE_GREY] = PPM_GETB( scaledPixel );
                break;
            case PIXEL_COLOR: // create ULong out of pixel colors
                contents[pos*PIXSIZE_COL + 0] = PPM_GETR( scaledPixel );
                contents[pos*PIXSIZE_COL + 1] = PPM_GETG( scaledPixel );
                contents[pos*PIXSIZE_COL + 2] = PPM_GETB( scaledPixel );
                break;
            case PIXEL_UNSIGNED:
                contents[pos*PIXSIZE_UNSIGNED + 0] = static_cast<char>(0xFF & PPM_GETB( scaledPixel ) );
                contents[pos*PIXSIZE_UNSIGNED + 1] = static_cast<char>(PPM_GETB( scaledPixel ) >> 8 );
                break;
            default:
                cerr << "Error: unknown pixel type code: " << static_cast<int>(pixType) << endl;
                break;
            }
        }
    }

    delete [] row;
    row=NULL;
}

/*
 * create a transient mdd with domain dom
 */
void createMarray(const r_Minterval &dom, r_Ref<r_GMarray> &mddPtr, PixelType pixType)
{
    switch (pixType)
    {
    case PIXEL_BOOL:
        INFO << "creating MDD of type bool and extent " << dom << endl;
        mddPtr = static_cast<r_GMarray*>(new r_Marray<r_Boolean>(dom));
        break;
    case PIXEL_GREY: // create Bool out of pixel (just take blue)
        INFO << "creating MDD of type grey and extent " << dom << endl;
        mddPtr = static_cast<r_GMarray*>(new r_Marray<r_Char>(dom));
        break;
    case PIXEL_COLOR: // create ULong out of pixel colors
        INFO << "creating MDD of type color and extent " << dom << endl;
        mddPtr = static_cast<r_GMarray*>(new r_Marray<RGBPixel>(dom));
        break;
    case PIXEL_UNSIGNED: // create Ushort
        INFO << "creating MDD of type unsigned and extent " << dom << endl;
        mddPtr = static_cast<r_GMarray*>(new r_Marray<r_UShort>(dom));
        break;
    default:
        cerr << "Error: unknown pixel type code: " << static_cast<int>(pixType) << endl;
        break;
    }
}


/*
 * print usage text, using program name 'name'
 * preconditions:
 * - name != NULL, name \0 terminated
 */
void
printUsage(const char* name)
{
    cout << "Usage: " << name << " [-h|--help|-c <name>|--collection <name>] [options] files..." << endl;
    cout << "Options:"<< endl;
    cout << " -h, --help                      this help" << endl;
    cout << " -c, --collection <name>         collection to store image; will be created if nonexistent (required)" << endl;
    cout << " -t, --type <t>                  store as pixel type t where t is one of: bin, grey, color, unsigned (default: " << DEFAULT_TYPE << ")" << endl;
    cout << " -r, --rescale                   rescale pixel values to 8 bit = 0:255 (default: no rescale)" << endl;
    cout << " --tiled <t>                     store object in tiles of size t where t = [min0:max0,min1:max1,...,minn:maxn] (default: 1 tile)" << endl;
    cout << " -s, --server <name>             server host (default: " << DEFAULT_HOST << ")" << endl;
    cout << " --port <p>                      server port (default: " << DEFAULT_PORT << ")" << endl;
    cout << " -d, --database <name>           database name (default: " << DEFAULT_DB << ")" << endl;
    cout << " -u, --user <name>               user name (default: " << DEFAULT_USER << ")" << endl;
    cout << " -p, --passwd <passwd>           password (default: " << DEFAULT_PASSWD << ")" << endl << endl;
    cout << " --transferformat <fmt>          transfer format. default: " << DEFAULT_TR_FMT << ")" << endl;
    cout << " --transferformatparams <params> transfer format parameters.(default: '')" << endl;
    cout << " --storageformat <fmt>           storage format (default: " << DEFAULT_ST_FMT << ")" << endl;
    cout << " --storageformatparams <params>  storage format parameters.(default: '')" << endl;
    cout << " -v, --verbose                   be verbose (default: be brief)" << endl;
    cout << "Note: If one file is passed, then a 2D object will be generated. " << endl
         << "      If several files are passed, then a 3D object will be generated (in the sequence passed)," << endl
         << "      In this case all files must have same pixel type and x/y extent." << endl;
}

_INITIALIZE_EASYLOGGINGPP

int
main( int argc, char** argv )
{
    // Default logging configuration
    LogConfiguration defaultConf(CONFDIR, CLIENT_LOG_CONF);
    defaultConf.configClientLogging();

    const char   *prog = argv[0];               // our name
    int           retval = EXIT_OK;             // program exit code

    PixelType     pixType = DEFAULT_PIXEL_TYPE;     // image pixel type
    const char   *typeString = DEFAULT_TYPE;        // image pixel type
    r_Bytes       typeSize=DEFAULT_PIXSIZE;         // pixel size [bytes]
    const char   *tileString = NULL;            // transfer format string
    const char   *transferFormatString = DEFAULT_TR_FMT;    // transfer format string
    r_Data_Format transferFormat=r_Array;       // transfer format used
    const char   *transferFormatParams = "";        // transfer format parameters
    const char   *storageFormatString = DEFAULT_ST_FMT; // storage format string
    r_Data_Format storageFormat=r_Array;            // storage format used
    char         *storageFormatParams = const_cast<char*>("");          // storage format parameters

    const char   *collName=NULL;                // name of collection

    r_Minterval   imgSize;                  // size of image
    r_Minterval   tileSize;                 // size of tiles

    const char   *dbName=DEFAULT_DB;            // name of database
    const char   *serverName=DEFAULT_HOST;          // name of RasDaMan server host
    const char   *serverPortString = DEFAULT_PORT;      // string parameter for port
    unsigned int serverPort=0;                  // port of RasDaMan server
    const char   *userName=DEFAULT_USER;            // name of RasDaMan user
    const char   *passwd=DEFAULT_PASSWD;            // password of RasDaMan user

    list<string>  fileNameList;             // input file name list

    bool mdd3d = false;                         // flag for 3D MDD
    bool tiled = false;                         // flag for tiling
    bool rescale = false;                       // flag for rescaling pixel values to 0:255

    r_Database database;                    // rasdaman database object, for connection

    // --- collect cmd line parameters --------------------------------------------------

    int c;
    int digit_optind = 0;

    if (argc==1)    // no params at all?
    {
        printUsage( prog );
        return EXIT_USAGE;
    }

    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        c = getopt_long (argc, argv, short_options, long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 0: // long option
            if (strcmp(long_options[option_index].name,OPTION_HELP)==0)
            {
                printUsage( prog );
                return EXIT_USAGE;
            }
            else if (strcmp(long_options[option_index].name,OPTION_COLLECTION)==0)
                collName = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_TYPE)==0)
                typeString = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_RESCALE)==0)
                rescale = true;
            else if (strcmp(long_options[option_index].name,OPTION_TILE)==0)
                tileString = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_SERVER)==0)
                serverName = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_PORT)==0)
                serverPortString = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_DATABASE)==0)
                dbName = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_USER)==0)
                userName = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_PASSWD)==0)
                passwd = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_TRANSFERFORMAT)==0)
                transferFormatString = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_TRANSFERFORMATPARAMS)==0)
                transferFormatParams = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_STORAGEFORMAT)==0)
                storageFormatString = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_STORAGEFORMATPARAMS)==0)
                storageFormatParams = optarg;
            else if (strcmp(long_options[option_index].name,OPTION_VERBOSE)==0)
                verbose = true;
            else
            {
                cerr << "Error: invalid option: " << long_options[option_index].name << endl;
                return EXIT_ERROR;
            }
            break;

        case 'h':
            printUsage( prog );
            return EXIT_USAGE;
            break;
        case 'c':
            if (!optarg)
            {
                cerr << "Error: missing collection name." << endl;
                return EXIT_ERROR;
            }
            collName = optarg;
            break;
        case 't':
            if (!optarg)
            {
                cerr << "Error: missing type." << endl;
                return EXIT_ERROR;
            }
            typeString = optarg;
            break;
        case 'r':
            rescale = true;
            break;
        case 's':
            if (!optarg)
            {
                cerr << "Error: missing server name." << endl;
                return EXIT_ERROR;
            }
            serverName = optarg;
            break;
        case 'd':
            if (!optarg)
            {
                cerr << "Error: missing database name." << endl;
                return EXIT_ERROR;
            }
            dbName = optarg;
            break;
        case 'u':
            if (!optarg)
            {
                cerr << "Error: missing user name." << endl;
                return EXIT_ERROR;
            }
            userName = optarg;
            break;
        case 'p':
            if (!optarg)
            {
                cerr << "Error: missing password." << endl;
                return EXIT_ERROR;
            }
            passwd = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        default:
            printf ("Error: illegal option letter %c\n", c);
            break;
        }
    } // while

    // get non-option arguments (ie, file names)
    if (optind < argc)
    {
        while (optind < argc)
            fileNameList.push_back(string(argv[optind++]));
        mdd3d = (fileNameList.size() > 1);
    }
    else
    {
        cerr << "Error: no input file name provided." << endl;
        return EXIT_ERROR;
    }

    // check mandatory parameters
    if (collName==NULL)
    {
        cerr << "Error: missing collection name; use option '--" << OPTION_COLLECTION << "'." << endl;
        return EXIT_ERROR;
    }

    // transform parameters where necessary
    serverPort = strtol(serverPortString, (char **)NULL, 10);
    if (errno==ERANGE || errno==EINVAL)
    {
        cerr << "Error: port not an integer number: " << serverPortString << endl;
        return EXIT_ERROR;
    }
    if( tileString )
    {
        try
        {
            tileSize = r_Minterval(tileString);
            tiled = true;
        }
        catch (r_Error e)
        {
            cerr << "Error " << e.get_errorno() << ": " << e.what() << endl;
            return EXIT_ERROR;
        }
    }
    else
    {
        // use default tile sizes
        if(mdd3d)
            tileSize = r_Minterval(3)
                       << r_Sinterval(static_cast<r_Range>(DEFAULT_TILE_LO), static_cast<r_Range>(DEFAULT_TILE_HI))
                       << r_Sinterval(static_cast<r_Range>(DEFAULT_TILE_LO), static_cast<r_Range>(DEFAULT_TILE_HI))
                       << r_Sinterval(static_cast<r_Range>(DEFAULT_TILE_LO), static_cast<r_Range>(DEFAULT_TILE_HI));
        else
            tileSize = r_Minterval(2)
                       << r_Sinterval(static_cast<r_Range>(DEFAULT_TILE_LO), static_cast<r_Range>(DEFAULT_TILE_HI))
                       << r_Sinterval(static_cast<r_Range>(DEFAULT_TILE_LO), static_cast<r_Range>(DEFAULT_TILE_HI));
        INFO << "tileSize = " << tileSize << endl;
    }
    transferFormat = get_data_format_from_name( transferFormatString );
    if(transferFormat == r_Data_Format_NUMBER)
    {
        cerr << "Error: Invalid transfer format '" << transferFormatString << "'" << endl;
        return EXIT_ERROR;
    }
    storageFormat = get_data_format_from_name( storageFormatString );
    if(storageFormat == r_Data_Format_NUMBER)
    {
        cerr << "Error: Invalid storage format '" << storageFormatString << "'" << endl;
        return EXIT_ERROR;
    }
    if (typeString!=NULL)
    {
        if (strcmp(typeString,PIXEL_TYPE_BOOL)==0)
            pixType = PIXEL_BOOL;
        else if (strcmp(typeString,PIXEL_TYPE_GREY)==0)
            pixType = PIXEL_GREY;
        else if (strcmp(typeString,PIXEL_TYPE_COLOR)==0)
            pixType = PIXEL_COLOR;
        else if (strcmp(typeString,PIXEL_TYPE_UNSIGNED)==0)
            pixType = PIXEL_UNSIGNED;
        else
        {
            cerr << "Error: illegal pixel type: " << typeString << endl;
            return EXIT_ERROR;
        }
    }

    INFO << prog << " v2.0 rasdaman PPM image insert utility" << endl;

    INFO << OPTION_COLLECTION << " = " << collName << ", ";
    INFO << OPTION_TYPE << " = " << typeString << ", ";
    INFO << "3d = " << mdd3d << ", ";
    INFO << OPTION_RESCALE << " = " << rescale << ", ";
    INFO << OPTION_TILE << " = " << tileSize << endl;
    INFO << OPTION_SERVER << " = " << serverName << ", ";
    INFO << OPTION_PORT << " = " << serverPort << ", ";
    INFO << OPTION_DATABASE << " = " << dbName << ", ";
    INFO << OPTION_USER << " = " << userName << ", ";
    INFO << OPTION_PASSWD << " = " << passwd << endl;
    INFO << OPTION_TRANSFERFORMAT << " = " << transferFormatString << ", ";
    INFO << OPTION_TRANSFERFORMATPARAMS << " = " << transferFormatParams << ", ";
    INFO << OPTION_STORAGEFORMAT << " = " << storageFormatString << ", ";
    INFO << OPTION_STORAGEFORMATPARAMS << " = " << storageFormatParams << endl;
    INFO << OPTION_VERBOSE << " = " << verbose << endl;
    INFO << "file list = " ;
    for ( list<string>::iterator from = fileNameList.begin(); from != fileNameList.end(); ++from )
        INFO << *from << " ";
    INFO << endl;

    // --- action --------------------------------------------------
    try
    {
        INFO << "connecting to " << serverName << ":" << serverPort << "..." << flush;
        database.set_servername(serverName, static_cast<int>(serverPort));
        database.set_useridentification(userName, passwd);
        database.open( dbName );
        // note: partial insert implicitly opens/closes TA

        // determine MDD object's parameters
        const char *mddTypeName=NULL;
        const char *collTypeName=NULL;
        switch (pixType)
        {
        case PIXEL_BOOL:
            collTypeName = mdd3d ? "BoolSet3" : "BoolSet";
            mddTypeName  = mdd3d ? "BoolCube" : "BoolImage";
            typeSize = PIXSIZE_BW;
            break;
        case PIXEL_GREY:
            collTypeName = mdd3d ? "GreySet3" : "GreySet";
            mddTypeName  = mdd3d ? "GreyCube" : "GreyImage";
            typeSize = PIXSIZE_GREY;
            break;
        case PIXEL_COLOR:
            collTypeName = mdd3d ? "RGBSet3" : "RGBSet";
            mddTypeName  = mdd3d ? "RGBCube" : "RGBImage";
            typeSize = PIXSIZE_COL;
            break;
        case PIXEL_UNSIGNED:
            collTypeName = mdd3d ? "UShortSet3" : "UShortSet";
            mddTypeName  = mdd3d ? "UShortCube" : "UShortImage";
            typeSize = PIXSIZE_UNSIGNED;
            break;

        default:
            cerr << "Error: unknown pixel type code: " << static_cast<int>(pixType) << endl;
            throw r_Error( r_Error::r_Error_General );
        }
        INFO << "mdd type = " << mddTypeName << ", set type = " << collTypeName << endl;

        // read header information for imgSize
        imgSize = readHeader(fileNameList.front().c_str(),mdd3d, fileNameList.size());  // we know we have at least 1 elem in list
        if(!tiled)
            tileSize = imgSize; // if not tiled then tile size = image size

        // Create a partial insert object with regular tiling
        r_Partial_Insert pins(database, collName, mddTypeName, collTypeName, tileSize, typeSize);

        r_Ref<r_GMarray> mddPtr;
        r_Minterval cacheDom;

        // iterate through all the pictures
        if(mdd3d)
        {
            cout << "creating 3D object of extent " << imgSize << " from: " << flush;

            r_Range numImgs = imgSize[0].high()+1; // X
            r_Range imgRows = imgSize[2].high()+1; // Y
            r_Range imgCols = imgSize[1].high()+1; // Z
            r_Range tileX = tileSize[0].high()+1;

            // create domain of first cache
            cacheDom = r_Minterval(3) << r_Sinterval(static_cast<r_Range>(0), tileX - 1)
                       << r_Sinterval(static_cast<r_Range>(0), imgCols - 1)
                       << r_Sinterval(static_cast<r_Range>(0), imgRows - 1);

            createMarray(cacheDom, mddPtr, pixType);
            char *contents = new char[cacheDom.cell_count() * typeSize];
            mddPtr->set_array(contents);

            unsigned int k = 0;
            list<string>::iterator currentFileName = fileNameList.begin();
            while ( k < fileNameList.size() )
            {
                cout << (*currentFileName).c_str() << "..." << flush;
                INFO << "#" << k << " " << cacheDom << "..." << flush;

                readImage(contents, (*currentFileName).c_str(), true, pixType, rescale, k, k%tileX);

                if (k % tileX == tileX-1 || k == fileNameList.size()-1)
                {
                    if (pins.update(mddPtr.ptr(), transferFormat, transferFormatParams, storageFormat, storageFormatParams) != 0)
                        throw r_Error( r_Error::r_Error_General );

                    mddPtr.destroy();

                    // we are done:
                    if(k == fileNameList.size()-1)
                        break;

                    // create domain of next tile
                    cacheDom = r_Minterval(3)
                               << r_Sinterval( static_cast<r_Range>(k + 1),
                                               static_cast<r_Range>(k+tileX > static_cast<int>(fileNameList.size()-1) ? fileNameList.size()-1 : static_cast<unsigned long long>(k+tileX)) )
                               << r_Sinterval(static_cast<r_Range>(0), imgCols - 1)
                               << r_Sinterval(static_cast<r_Range>(0), imgRows - 1);

                    createMarray(cacheDom, mddPtr, pixType);
                    contents = mddPtr->get_array();
                }

                ++currentFileName, ++k;
            }
        }
        else
        {
            r_Range imgCols = imgSize[0].high()+1;
            r_Range imgRows = imgSize[1].high()+1;
            r_Range tileRows = tileSize[0].high()+1;
            const char *fName = fileNameList.front().c_str();   // we know we have 1 element in list

            cout << "creating 2D object of extent " << imgSize << " from " << fName << "..." << flush;

            openImage(fName,false);     // open image for reading

            for(r_Range k = 0; k < imgRows; k += tileRows )
            {
                // create domain of next tile
                cacheDom =  r_Minterval(2)
                            << r_Sinterval(static_cast<r_Range>(0), imgCols - 1)
                            << r_Sinterval(k, k + min(tileRows-1, imgRows-k-1));

                INFO << "#" << k << " " << cacheDom << "..." << flush;

                createMarray(cacheDom, mddPtr, pixType);
                char *contents = new char[cacheDom.cell_count() * typeSize];
                mddPtr->set_array(contents);

                readRows(contents, k, min(tileRows, imgRows - k), pixType, rescale);

                if (pins.update(mddPtr.ptr(), transferFormat, transferFormatParams, storageFormat, storageFormatParams) != 0)
                    throw r_Error( r_Error::r_Error_General );

                mddPtr.destroy();
            }
        }
        database.close();

        cout << "ok" << endl;
    }
    catch(r_Error &err)
    {
        cerr << err.what() << endl;
        // emergency abort/close, ignoring any eventual further exception
        try
        {
            database.close();
        }
        catch(...)
            { }
        retval = EXIT_ERROR;
    }
    catch (...)
    {
        cout << "Panic: unexpected exception." << endl;
    }

    INFO << argv[0] << " done." << endl;
    return retval;
}
