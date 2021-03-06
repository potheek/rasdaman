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
 * SOURCE: netcdf.cc
 *
 * MODULE:  conversion
 *
 * CLASSES: r_Conv_NETCDF
 *
 * COMMENTS:
 *
 * Provides functions to convert data to NETCDF and back.
 */

#include "config.h"
#ifdef HAVE_NETCDF

#include "conversion/netcdf.hh"
#include "conversion/memfs.hh"
#include "raslib/error.hh"
#include "raslib/parseparams.hh"
#include "raslib/primitivetype.hh"
#include "raslib/attribute.hh"
#include "raslib/structuretype.hh"

#include "netcdfcpp.h"
#include "raslib/odmgtypes.hh"
#include "relcatalogif/basetype.hh"
#include "relcatalogif/structtype.hh"

#include <easylogging++.h>

#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <math.h>
#include <cmath>
#include <unistd.h>

#define DEFAULT_VAR "data"
#define VARIABLE_SEPARATOR_CHAR ';'
#define VARIABLE_SEPARATOR_STR ";"
#define VARIABLE_KEY "vars"
#define VARIABLE_KEY_EQ "vars="
#define VALID_MIN "valid_min"
#define VALID_MAX "valid_max"
#define VALID_MIN_BYTE 0
#define VALID_MAX_BYTE 255
#define VALID_MIN_SHORT 0
#define VALID_MAX_SHORT 65535
#define VALID_MIN_INT 0.0
#define VALID_MAX_INT 4294967295.0

#define REMOVE_TMP_FILES \
    dataFile.close(); \
    remove(tmpFile);

using namespace std;

void r_Conv_NETCDF::initNETCDF(void)
{
    Conventions = "CF-1.4";
    Institution = "rasdaman.org, Jacobs University Bremen";

    variable = NULL;
    vars = NULL;
    if (params == NULL)
        params = new r_Parse_Params();
    params->add(VARIABLE_KEY, &variable, r_Parse_Params::param_type_string);
    varsSize = 0;
}

void r_Conv_NETCDF::getVars(void)
{
    if (variable != NULL)
    {
        char* start = strstr(variable, VARIABLE_KEY_EQ);
        char* variables = variable;
        if (start != NULL)
        {
            variables = variable + strlen(VARIABLE_KEY_EQ);
        }
        // count nr of variables in the options string
        int occ = 0;
        for (int i = 0; variables[i]; i++)
            occ += (variables[i] == VARIABLE_SEPARATOR_CHAR);
        // separate the variables and save them in 'vars'
        vars = new char*[occ + 1];
        char *result = NULL;
        result = strtok( variables, VARIABLE_SEPARATOR_STR );
        while( result != NULL )
        {
            vars[varsSize] = strdup(result);
            varsSize++;
            result = strtok( NULL, VARIABLE_SEPARATOR_STR );
        }
    }
}

/// constructor using an r_Type object. Exception if the type isn't atomic.
r_Conv_NETCDF::r_Conv_NETCDF(const char *src, const r_Minterval &interv, const r_Type *tp) throw (r_Error)
    : r_Convert_Memory(src, interv, tp, true)
{
    initNETCDF();
    /* added support for multiple variables and commented out this check -- MR 2012-oct-20
    /// ToDo: Can be hacked by dividing it to its basic component. Or by using CXX-4 which is in the development phase
    if (tp->isStructType())
    {
        LFATAL << "r_Conv_NETCD::r_Conv_NETCDF(): structured types not supported.";
        throw r_Error(r_Error::r_Error_General);
    }
    */
}

/// constructor using convert_type_e shortcut
r_Conv_NETCDF::r_Conv_NETCDF(const char *src, const r_Minterval &interv, int tp) throw (r_Error)
    : r_Convert_Memory(src, interv, tp)
{
    initNETCDF();
}


/// destructor
r_Conv_NETCDF::~r_Conv_NETCDF(void)
{
    if (variable != NULL)
    {
        delete [] variable;
        variable = NULL;
    }
    for (int i = 0; i < varsSize; i++)
    {
        if (vars[i] != NULL)
        {
            delete [] vars[i];
        }
    }
    if (vars != NULL)
    {
        delete [] vars;
        vars = NULL;
    }
}

/// convert to NETCDF
r_convDesc &r_Conv_NETCDF::convertTo(const char *options) throw (r_Error)
{
    char tmpFile[] ="/tmp/rasdaman_nctempXXXXXX";
    string dimNamePrefix = "dim_";
    int dimNo = 0;
    long *dimSizes;
    long dataSize = 1;
    size_t filesize = 0;
    FILE *fp = NULL;
    const char *src = desc.src;
    int tempFD;              // for the temp file

    if (options != NULL)
    {
        params->process(options);
        getVars();
    }

    tempFD = mkstemp(tmpFile);
    if(tempFD == -1)
    {
        LFATAL << "r_Conv_netcdf::convertTo(" << (options?options:"NULL")
                        << ") desc.srcType (" << desc.srcType->type_id()
                        << ") unable to generate a tempory file !";
        throw r_Error();
    }

    // Create the file. The Replace parameter tells netCDF to overwrite
    // this file, if it already exists.
    NcFile dataFile(tmpFile, NcFile::Replace);
    if (!dataFile.is_valid())
    {
        LFATAL << "Error: unable to open output file.";
        throw r_Error(r_Error::r_Error_General);
    }

    //Create netCDF dimensions
    dimNo = static_cast<int>(desc.srcInterv.dimension());
    dimSizes = new long[dimNo];
    const NcDim** dims;
    dims = new const NcDim*[dimNo];
    if (dimSizes == NULL)
    {
        LFATAL << "Error: out of memory.";
        REMOVE_TMP_FILES;
        if(dims != NULL)
        {
            delete[] dims;
            dims = NULL;
        }
        throw r_Error(r_Error::r_Error_MemoryAllocation);
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(dimNo); i++)
    {
        dimSizes[i] = desc.srcInterv[i].high() - desc.srcInterv[i].low() + 1;
        dataSize *= dimSizes[i];
    }

    // Add the dimensions to the netcdf file, the dimension names are
    // Dimension_0, ..,Dimension_rank-1. Add also the dimension interval
    for (int i = 0; i < dimNo; i++)
    {
        stringstream s;
        s << dimNamePrefix << i;
        NcDim* dim = dataFile.add_dim(s.str().c_str(), dimSizes[i]);
        dims[i] = dim;
    }

    // Define a netCDF variable. The type of the variable depends on rasdaman type
    char* varName = const_cast<char*>(DEFAULT_VAR);
    if (variable != NULL && varsSize == 1)
    {
        varName = variable;
    }
    if (desc.baseType != ctype_struct && varsSize > 1)
    {
        LFATAL << "Error: mismatch in #variables between query and MDD object type.";
        REMOVE_TMP_FILES;
        if(dims != NULL)
        {
            delete[] dims;
            dims = NULL;
        }
        throw r_Error(r_Error::r_Error_QueryParameterCountInvalid);
    }

    switch (desc.baseType)
    {
    case ctype_int8:
    {
        r_Octet *val = (r_Octet* &) src;
        NcVar *ncVar = dataFile.add_var(varName, ncByte, dimNo, dims);
        // Write the data to the file.
        ncVar->put(val, dimSizes);
        break;
    }
    case ctype_char:
    case ctype_uint8:
    {
        // unsigned data has to be transformed to data of 2x more bytes
        // as NetCDF only supports exporting signed data..
        // so unsigned char is transformed to short, and we add the
        // valid_min/valid_max attributes to describe the range -- DM 2012-may-24

        r_Char *val = (r_Char* &) src;
        short *data = new short[dataSize];
        if (data == NULL)
        {
            LFATAL << "Error: out of memory.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_MemoryAllocation);
        }
        for (int i = 0; i < dataSize; i++, val++)
        {
            data[i] = (short) ((r_Char) val[0]);
        }
        NcVar *ncVar = dataFile.add_var(varName, ncShort, dimNo, dims);
        ncVar->put(&data[0], dimSizes);
        ncVar->add_att(VALID_MIN, VALID_MIN_BYTE);
        ncVar->add_att(VALID_MAX, VALID_MAX_BYTE);
        delete [] data;
        break;
    }
    case ctype_int16:
    {
        r_Short *val = (r_Short* &) src;
        NcVar *ncVar = dataFile.add_var(varName, ncShort, dimNo, dims);
        ncVar->put(val, dimSizes);
        break;
    }
    case ctype_uint16:
    {
        r_UShort *val = (r_UShort* &) src;
        int *data = new int[dataSize];
        if (data == NULL)
        {
            LFATAL << "Error: out of memory.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_MemoryAllocation);
        }
        for (int i = 0; i < dataSize; i++, val++)
        {
            data[i] = (int) ((r_UShort) val[0]);
        }
        NcVar *ncVar = dataFile.add_var(varName, ncInt, dimNo, dims);
        ncVar->put(&data[0], dimSizes);
        ncVar->add_att(VALID_MIN, VALID_MIN_SHORT);
        ncVar->add_att(VALID_MAX, VALID_MAX_SHORT);
        delete [] data;
        break;
    }
    case ctype_int32:
    {
        r_Long *val = (r_Long* &) src;
        NcVar *ncVar = dataFile.add_var(varName, ncInt, dimNo, dims);
        ncVar->put(val, dimSizes);
        break;
    }
    case ctype_uint32:
    {
        // upscale unsigned long to float, as it can't be exported directly as uint
        r_ULong *val = (r_ULong* &) src;
        float *data = new float[dataSize];
        if (data == NULL)
        {
            LFATAL << "Error: out of memory.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_MemoryAllocation);
        }
        for (int i = 0; i < dataSize; i++, val++)
        {
            data[i] = (float) ((r_ULong) val[0]);
        }
        NcVar *ncVar = dataFile.add_var(varName, ncFloat, dimNo, dims);
        ncVar->put(&data[0], dimSizes);
        ncVar->add_att(VALID_MIN, VALID_MIN_INT);
        ncVar->add_att(VALID_MAX, VALID_MAX_INT);
        delete [] data;
        break;
    }
    case ctype_float32:
    {
        r_Float *val = (r_Float* &) src;
        NcVar *ncVar = dataFile.add_var(varName, ncFloat, dimNo, dims);
        ncVar->put(val, dimSizes);
        ncVar->add_att("missing_value", "NaNf");
        break;
    }
    case ctype_int64:
    case ctype_uint64:
    case ctype_float64:
    {
        r_Double *val = (r_Double* &) src;
        NcVar *ncVar = dataFile.add_var(varName, ncDouble, dimNo, dims);
        ncVar->put(val, dimSizes);
        ncVar->add_att("missing_value", "NaN");
        break;
    }
    case ctype_rgb:
    case ctype_struct:
    {
        r_Structure_Type *st = static_cast<r_Structure_Type*>(const_cast<r_Type*>(desc.srcType));
        if (st == NULL)
        {
            LFATAL << "Error: MDD object type could not be cast to struct.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_RefInvalid);
        }
        if (varsSize > static_cast<int>(st->count_elements()))
        {
            LFATAL << "Error: mismatch in #variables between query and MDD object type.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_QueryParameterCountInvalid);
        }
        int structSize = 0; // size of the struct type, used for offset computation in memcpy
        int allMatch = 0; // check if all variables have a match in struct attributes
        if (variable == NULL)
        {
            vars = new char*[st->count_elements()];
            if (vars == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                if(dims != NULL)
                {
                    delete[] dims;
                    dims = NULL;
                }
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            varsSize = 0;
        }

        for (r_Structure_Type::attribute_iterator ite(st->defines_attribute_begin()); ite != st->defines_attribute_end(); ite++)
        {
            r_Primitive_Type *pt = static_cast<r_Primitive_Type*>(const_cast<r_Base_Type*>(&(*ite).type_of()));
            structSize += static_cast<int>(pt->size());
            if (variable == NULL)
            {
                vars[varsSize] = strdup((*ite).name());
                varsSize++;
                allMatch++;
            }
            else
            {
                int i = 0;
                while ((i < varsSize) && (strcmp(vars[i], (*ite).name()) != 0))
                {
                    i++;
                }
                if (i < varsSize)
                {
                    allMatch++;
                }
            }
        }
        if (allMatch != varsSize)
        {
            LFATAL << "Error: not all specified variables are attributes of the MDD object type.";
            REMOVE_TMP_FILES;
            if(dims != NULL)
            {
                delete[] dims;
                dims = NULL;
            }
            throw r_Error(r_Error::r_Error_General);
        }

        for (int i = 0;  i < varsSize; i++)
        {
            int offset = 0; // offset of the attribute within the struct type
            r_Structure_Type::attribute_iterator iter(st->defines_attribute_begin());
            while (iter != st->defines_attribute_end() && (strcmp((*iter).name(),vars[i]) != 0))
            {
                r_Primitive_Type *pt = static_cast<r_Primitive_Type*>(const_cast<r_Base_Type*>(&(*iter).type_of()));
                offset += static_cast<int>(pt->size());
                iter++;
            }
            if (iter != st->defines_attribute_end())
            {
                varName = vars[i];
                switch ((*iter).type_of().type_id())
                {
                    case r_Type::OCTET:
                    {
                        char * buff = new char[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(char));
                        }
                        r_Octet *val = (r_Octet*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncByte, dimNo, dims);
                        ncVar->put(val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    case r_Type::CHAR:
                    {
                        char * buff = new char[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(char));
                        }
                        r_Char *val = (r_Char*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncChar, dimNo, dims);
                        ncVar->put((const char*)val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    case r_Type::USHORT:
                    case r_Type::SHORT:
                    {
                        short * buff = new short[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(short));
                        }
                        r_Short *val = (r_Short*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncShort, dimNo, dims);
                        ncVar->put(val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    case r_Type::ULONG:
                    case r_Type::LONG:
                    {
                        int * buff = new int[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(int));
                        }
                        r_Long *val = (r_Long*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncInt, dimNo, dims);
                        ncVar->put(val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    case r_Type::FLOAT:
                    {
                        float * buff = new float[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(float));
                        }
                        r_Float *val = (r_Float*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncFloat, dimNo, dims);
                        ncVar->put(val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    case r_Type::DOUBLE:
                    {
                        double * buff = new double[dataSize];
                        if (buff == NULL)
                        {
                            LFATAL << "Error: out of memory.";
                            REMOVE_TMP_FILES;
                            if(dims != NULL)
                            {
                                delete[] dims;
                                dims = NULL;
                            }
                            throw r_Error(r_Error::r_Error_MemoryAllocation);
                        }
                        for (int j = 0; j < dataSize; j++)
                        {
                            memcpy(&buff[j], desc.src + j*structSize + offset, sizeof(double));
                        }
                        r_Double *val = (r_Double*) buff;
                        NcVar *ncVar = dataFile.add_var(varName, ncDouble, dimNo, dims);
                        ncVar->put(val, dimSizes);
                        delete [] buff;
                        break;
                    }
                    default:
                    {
                        LFATAL << "Error: this type is not supported " << desc.baseType << ".";
                        REMOVE_TMP_FILES;
                        if(dims != NULL)
                        {
                            delete[] dims;
                            dims = NULL;
                        }
                        throw r_Error(r_Error::r_Error_General);
                    }
                }
            }

        }
        break;
    }
    default:
    {
        LFATAL << "Error: this type is not supported " << desc.baseType << ".";
        REMOVE_TMP_FILES;
        if(dims != NULL)
        {
            delete[] dims;
            dims = NULL;
        }
        throw r_Error(r_Error::r_Error_Conversion);
    }
    }

    dataFile.add_att("Conventions", Conventions);
    dataFile.add_att("Institution", Institution);
    dataFile.close();
    // The data file is written and closed

    // Pass the NetCDF file as a stream of char
    if ((fp = fopen(tmpFile, "rb")) == NULL)
    {
        LFATAL << "Error: unable to read back file.";
        throw r_Error(r_Error::r_Error_Conversion);
    }
    // Get the file size
    fseek(fp, 0, SEEK_END);
    filesize = static_cast<size_t>(ftell(fp));

    if ((desc.dest = (char*) mystore.storage_alloc(filesize)) == NULL)
    {
        LFATAL << "Error:  out of memory.";
        remove(tmpFile);
        throw r_Error(r_Error::r_Error_MemoryAllocation);
    }
    // Set the desc.dest content
    fseek(fp, 0, SEEK_SET);
    size_t rsize = fread(desc.dest, 1, filesize, fp);
    fclose(fp);
    // Free Memory
    remove(tmpFile);
    delete [] dimSizes;
    if(dims != NULL)
    {
        delete[] dims;
        dims = NULL;
    }
    //Set the interval and type
    desc.destInterv = r_Minterval(1);
    desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) filesize - 1);
    desc.destType = r_Type::get_any_type("char");

    return desc;
}

/// convert from NETCDF
r_convDesc &r_Conv_NETCDF::convertFrom(const char *options) throw (r_Error)
{
    size_t dataSize = 1;
    long *dimSizes = NULL;
    
    char tmpFile[] = "/tmp/rasdaman_nctempXXXXXX";
    int tmpFd;
    tmpFd = mkstemp(tmpFile);
    if(tmpFd == -1)
    {
        LFATAL << "r_Conv_NETCDF::convertFrom(" << (options?options:"NULL")
                        << ") desc.srcType (" << desc.srcType->type_id()
                        << ") unable to generate a temporary file!";
        throw r_Error(r_Error::r_Error_Conversion);
    }

    if (options != NULL)
    {
        variable = strdup(options);
        getVars();
    }

    const char* src = desc.src;
    // Just write the data to temp file.
    ofstream file;
    file.open(tmpFile);
    for (int i = 0; i < desc.srcInterv[0].high() - desc.srcInterv[0].low() + 1; i++, src++)
    {
        file << *src;
    }
    file.close();

    // Open the file. The ReadOnly parameter tells netCDF we want
    // read-only access to the file.
    NcFile dataFile(tmpFile, NcFile::ReadOnly);
    if (!dataFile.is_valid())
    {
        LFATAL << "Error: Can not open the file.";
        REMOVE_TMP_FILES;
        throw r_Error(r_Error::r_Error_Conversion);
    }

    // Number of dimensions and varaiables
    int numDims = dataFile.num_dims();
    int numVars = dataFile.num_vars();

    // Get a set of dimension names. This is used to get all the variables that are not dimensions
    vector<string> varNames;
    vector<string>::iterator it;

    // Get a set of variable names that are not dimensions. and defined by all dimensions
    for (int i = 0; i < numVars; i++)
    {
        NcVar *var = dataFile.get_var(i);
        if (var->num_dims() > 0)
        {
            if (variable == NULL)
            {
                varNames.push_back(var->name());
            }
            else
            {
                int j = 0;
                while ((j < varsSize) && (strcmp(var->name(), vars[j]) != 0))
                {
                    j++;
                }
                if (j < varsSize)
                {
                    varNames.push_back(var->name());
                }
            }
        }
    }
    if (varNames.empty())
    {
        LFATAL << "Error: no variable found to import.";
        REMOVE_TMP_FILES;
        throw r_Error(r_Error::r_Error_Conversion);
    }

    if (varsSize == 1)
    {
        string varName = vars[0];
        NcVar *var = dataFile.get_var(varName.c_str());
        numDims = var->num_dims();
        dimSizes = new long[numDims];
        if (dimSizes == NULL)
        {
            LFATAL << "Error: out of memory.";
            REMOVE_TMP_FILES;
            throw r_Error(r_Error::r_Error_MemoryAllocation);
        }
        for (int i = 0; i < numDims; i++)
        {
            NcDim *dim = var->get_dim(i);
            dimSizes[i] = dim->size();
            dataSize *= static_cast<size_t>(dim->size());
        }
        switch (var->type())
        {
        case ncByte:
        {
            ncbyte *data = new ncbyte[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            // Ignore NetCDF dim interval and assume it always starts at zero
            // ToDo: Add a parse object to allow the user to control the dim interval, i.e. start at 0 or not
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize)) == NULL)
            {
                LFATAL << "Error: out of memory";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (ncbyte));
            desc.destType = get_external_type(ctype_char);
            delete [] data;
            break;
        }
        case ncChar:
        {
            char *data = new char[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            // Ignore NetCDF dim interval and assume it always starts at zero
            // ToDo: Add a parse object to allow the user to control the dim interval, i.e. start at 0 or not
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize)) == NULL)
            {
                LFATAL << "Error: out of memory";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (char));
            desc.destType = get_external_type(ctype_char);
            delete [] data;
            break;
        }
        case ncShort:
        {
            short *data = new short[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize * sizeof (short))) == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (short));
            desc.destType = get_external_type(ctype_int16);
            delete [] data;
            break;
        }
        case ncInt:
        {
            int *data = new int[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize * sizeof (int))) == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (int));
            desc.destType = get_external_type(ctype_int32);
            delete [] data;
            break;
        }
        case ncFloat:
        {
            float *data = new float[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize * sizeof (float))) == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (float));
            desc.destType = get_external_type(ctype_float32);
            delete [] data;
            break;
        }
        case ncDouble:
        {
            double *data = new double[dataSize];
            if (data == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            var->get(&data[0], dimSizes);

            desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
            for (int i = 0; i < numDims; i++)
                desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

            if ((desc.dest = (char*) mystore.storage_alloc(dataSize * sizeof (double))) == NULL)
            {
                LFATAL << "Error: out of memory";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            memcpy(desc.dest, data, dataSize * sizeof (double));
            desc.destType = get_external_type(ctype_float64);
            delete [] data;
            break;
        }
        default:
        {
            LFATAL << "Error: this type is not supported" << desc.baseType << ".";
            REMOVE_TMP_FILES;
            throw r_Error(r_Error::r_Error_Conversion);
        }

        }
    }
    else
    {
        // if no variable is specified, try to import all
        if (variable == NULL)
        {
            vars = new char*[varNames.size()];
            if (vars == NULL)
            {
                LFATAL << "Error: out of memory.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_MemoryAllocation);
            }
            varsSize = 0;
            for (it = varNames.begin(); it != varNames.end(); it++)
            {
                vars[varsSize] = strdup((*it).c_str());
                varsSize++;
            }
        }
        desc.baseType = ctype_struct;
        size_t structSize = 0; // size of the struct type, used for offset computations in memcpy
        size_t alignSize = 1;  // alignment of size (taken from StructType::calcSize())
        size_t cellSize = 0;
        stringstream destType(stringstream::out); // build the struct type string
        destType << "struct { ";
        for (int i = 0; i < varsSize; i++)
        {
            if (find(varNames.begin(), varNames.end(), vars[i]) == varNames.end())
            {
                LFATAL << "Error: variable " << vars[i] << " not present in the file.";
                REMOVE_TMP_FILES;
                throw r_Error(r_Error::r_Error_Conversion);
            }
            NcVar *var = dataFile.get_var(vars[i]);
            if (i > 0)
                destType << ", ";
            switch (var->type())
            {
                case ncByte:
                case ncChar:
                    cellSize = sizeof(r_Char);
                    destType << "char";
                    break;
                case ncDouble:
                    cellSize = sizeof(r_Double);
                    destType << "double";
                    break;
                case ncFloat:
                    cellSize = sizeof(r_Float);
                    destType << "float";
                    break;
                case ncInt:
                    cellSize = sizeof(r_Long);
                    destType << "long";
                    break;
                case ncShort:
                    cellSize = sizeof(r_Short);
                    destType << "short";
                    break;
                default:
                {
                    LFATAL << "Error: this type is not supported." << static_cast<int>(var->type());
                    REMOVE_TMP_FILES;
                    throw r_Error(r_Error::r_Error_Conversion);
                }
            }
            structSize += cellSize;
            if (cellSize > alignSize)
                alignSize = cellSize;

            numDims = var->num_dims();
            int firstDim = 0; // get the dimensionality of first variable to import and check if all other have the same dimensionality
            if (i == 0)
            {
                firstDim = numDims;
                dimSizes = new long[numDims];
                if (dimSizes == NULL)
                {
                    LFATAL << "Error: out of memory.";
                    REMOVE_TMP_FILES;
                    throw r_Error(r_Error::r_Error_MemoryAllocation);
                }
            }
            else
            {
                if (numDims != firstDim) {
                    LFATAL << "Error: variables have different dimesionalities.";
                    REMOVE_TMP_FILES;
                    throw r_Error(r_Error::r_Error_Conversion);
                }
            }
            for (int j = 0; j < numDims; j++)
            {
                NcDim *dim = var->get_dim(j);
                if (dim->is_unlimited())
                {
                    LFATAL << "Error: unlimited dimensions can not be handled.";
                    REMOVE_TMP_FILES;
                    throw r_Error(r_Error::r_Error_Conversion);
                }
                else
                {
                    if (i == 0)
                    {
                        dimSizes[j] = dim->size();
                        dataSize *= static_cast<size_t>(dim->size());
                    }
                    else if (dim->size() != dimSizes[j])
                    {
                        LFATAL << "Error: variables have different dimesionalities.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_Conversion);
                    }
                }
            }
        }

        destType << " }";
        desc.destType = r_Type::get_any_type(destType.str().c_str());
        
        // align struct size to the member type of biggest size
        structSize = (structSize / alignSize + 1) * alignSize;
        
        if ((desc.dest = (char*) mystore.storage_alloc(dataSize * structSize)) == NULL)
        {
            LFATAL << "Error: out of memory.";
            REMOVE_TMP_FILES;
            throw r_Error(r_Error::r_Error_MemoryAllocation);
        }

        size_t offset = 0; // offset of the attribute within the struct type
        for (int i = 0; i < varsSize; i++)
        {
            NcVar *var = dataFile.get_var(vars[i]);
            switch (var->type())
            {
                case ncShort:
                {
                    short *data = new short[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(short));

                    delete [] data;
                    offset += sizeof(short);
                    break;
                }
                case ncInt:
                {
                    int *data = new int[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(int));

                    delete [] data;
                    offset += sizeof(int);
                    break;
                }
                case ncDouble:
                {
                    double *data = new double[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(double));

                    delete [] data;
                    offset += sizeof(double);
                    break;
                }
                case ncFloat:
                {
                    float *data = new float[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(float));

                    delete [] data;
                    offset += sizeof(float);
                    break;
                }
                case ncByte:
                {
                    ncbyte *data = new ncbyte[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(ncbyte));

                    delete [] data;
                    offset += sizeof(ncbyte);
                    break;
                }
                case ncChar:
                {
                    char *data = new char[dataSize];
                    if (data == NULL)
                    {
                        LFATAL << "Error: out of memory.";
                        REMOVE_TMP_FILES;
                        throw r_Error(r_Error::r_Error_MemoryAllocation);
                    }
                    var->get(&data[0], dimSizes);

                    for (unsigned int j = 0; j < dataSize; j++)
                        memcpy(desc.dest + j * structSize + offset, &data[j], sizeof(char));

                    delete [] data;
                    offset += sizeof(char);
                    break;
                }
                default:
                {
                    LFATAL << "Error: this type is not supported.";
                    REMOVE_TMP_FILES;
                    throw r_Error(r_Error::r_Error_Conversion);
                }

            }

        }

        desc.destInterv = r_Minterval(static_cast<r_Dimension>(numDims));
        for (int i = 0; i < numDims; i++)
            desc.destInterv << r_Sinterval((r_Range) 0, (r_Range) dimSizes[i] - 1);

    }
    if (desc.srcInterv.dimension() == 2)
        // this means it was explicitly specified, so we shouldn't override it
        desc.destInterv = desc.srcInterv;

    REMOVE_TMP_FILES;
    
    return desc;
}

/// cloning

r_Convertor *r_Conv_NETCDF::clone(void) const
{
    return new r_Conv_NETCDF(desc.src, desc.srcInterv, desc.baseType);
}

/// identification

const char *r_Conv_NETCDF::get_name(void) const
{
    return format_name_netcdf;
}

r_Data_Format r_Conv_NETCDF::get_data_format(void) const
{
    return r_NETCDF;
}
/// For test purpose

template <class baseType, class castType> void r_Conv_NETCDF::print(baseType* val, int bufferZise)
{

    for (int i = 0; i < bufferZise; ++i, val++)
        LINFO << (castType) val[0];
}

#endif

/* NcType is defined as enum as follows
Name            size        Comments
ncByte          1       8-bit signed integer
ncChar          1       8-bit unsigned integer
ncShort         2       signed 2 byte integer
ncInt           4       32-bit signed integer
ncLong          4       Deprecated
ncFloat         4       32-bit floating point
ncDouble        8       64-bit floating point
 */
