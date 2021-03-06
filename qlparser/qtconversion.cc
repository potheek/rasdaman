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
/*************************************************************
 *
 *
 * PURPOSE:
 *
 *
 * COMMENTS:
 *
 ************************************************************/

static const char rcsid[] = "@(#)qlparser, QtConversion: $Header: /home/rasdev/CVS-repository/rasdaman/qlparser/qtconversion.cc,v 1.36 2003/12/27 20:51:28 rasdev Exp $";

#include "config.h"
#include "raslib/basetype.hh"
#include "raslib/structuretype.hh"
#include "conversion/convertor.hh"
#include "conversion/convfactory.hh"

#include "qlparser/qtconversion.hh"
#include "qlparser/qtmdd.hh"

#include "mddmgr/mddobj.hh"
#include "tilemgr/tile.hh"

#include "catalogmgr/typefactory.hh"
#include "relcatalogif/typeiterator.hh"
#include "relcatalogif/structtype.hh"
#include "relcatalogif/chartype.hh"

#include <easylogging++.h>

#include <iostream>
#ifndef CPPSTDLIB
#include <ospace/string.h> // STL<ToolKit>
#else
#include <string>
using namespace std;
#endif

const QtNode::QtNodeType QtConversion::nodeType = QtNode::QT_CONVERSION;


QtConversion::QtConversion( QtOperation* newInput, QtConversionType
                            newConversionType, const char* s )
    : QtUnaryOperation( newInput ), conversionType( newConversionType ), paramStr(s)
{
    LTRACE << "QtConversion()";
}

void
QtConversion::setConversionTypeByName( string formatName )
{
    if(string("bmp") == formatName)
        conversionType = QtConversion::QT_TOBMP;
    else if(string("hdf") == formatName )
        conversionType = QtConversion::QT_TOHDF;
    else if(string("png") == formatName)
        conversionType = QtConversion::QT_TOPNG;
    else if (string("jpeg") == formatName)
        conversionType = QtConversion::QT_TOJPEG;
    else if (string("tiff") == formatName)
        conversionType = QtConversion::QT_TOTIFF;
    else if (string("vff") == formatName)
        conversionType = QtConversion::QT_TOVFF;
    else if (string("csv") == formatName)
        conversionType = QtConversion::QT_TOCSV;
    else if (string("tor") == formatName)
        conversionType = QtConversion::QT_TOTOR;
    else if (string("dem") == formatName)
        conversionType = QtConversion::QT_TODEM;
    else if(string("netcdf") == formatName )
        conversionType = QtConversion::QT_TONETCDF;
    else if(string("inv_bmp") == formatName )
        conversionType = QtConversion::QT_FROMBMP;
    else if(string("inv_hdf") == formatName )
        conversionType = QtConversion::QT_FROMHDF;
    else if(string("inv_csv") == formatName )
        conversionType = QtConversion::QT_FROMCSV;
    else if(string("inv_png") == formatName)
        conversionType = QtConversion::QT_FROMPNG;
    else if (string("inv_jpeg") == formatName)
        conversionType = QtConversion::QT_FROMJPEG;
    else if (string("inv_tiff") == formatName)
        conversionType = QtConversion::QT_FROMTIFF;
    else if (string("inv_vff") == formatName)
        conversionType = QtConversion::QT_FROMVFF;
    else if (string("inv_tor") == formatName)
        conversionType = QtConversion::QT_FROMTOR;
    else if (string("inv_dem") == formatName)
        conversionType = QtConversion::QT_FROMDEM;
    else if(string("inv_netcdf") == formatName )
        conversionType = QtConversion::QT_FROMNETCDF;
    else
        conversionType = QtConversion::QT_UNKNOWN;
}

bool
QtConversion::lookupConversionTypeByName( string formatName )
{
    return (( string("bmp")      == formatName ) || ( string("hdf")      == formatName ) ||
            ( string("png")      == formatName ) || ( string("jpeg")     == formatName ) ||
            ( string("dem")      == formatName ) || ( string("tor")      == formatName ) ||
            ( string("csv")      == formatName ) || ( string("inv_csv")  == formatName ) ||
            ( string("tiff")     == formatName ) || ( string("vff")      == formatName ) ||
            ( string("inv_bmp")  == formatName ) || ( string("inv_hdf")  == formatName ) ||
            ( string("inv_png")  == formatName ) || ( string("inv_jpeg") == formatName ) ||
            ( string("inv_dem")  == formatName ) || ( string("inv_tor")  == formatName ) ||
            ( string("inv_tiff") == formatName ) || ( string("inv_vff")  == formatName ) ||
            ( string("netcdf")   == formatName ) || ( string("inv_netcdf") == formatName ));
}

bool
QtConversion::equalMeaning( QtNode* node )
{

    bool result = false;

    if( nodeType == node->getNodeType() )
    {
        QtConversion* convNode;
        convNode = static_cast<QtConversion*>(node); // by force

        result = input->equalMeaning( convNode->getInput() );

        result &= conversionType == convNode->conversionType;
    };

    return ( result );
}

const BaseType*
constructBaseType( const r_Type *type )
{
    const BaseType *result = NULL;
    if(type->isPrimitiveType())
    {
        result = TypeFactory::mapType(type->name());
        if (!result)
        {
            LFATAL << "Error: no primitive type for name '"
                           << type->name() << "' were found";
            throw r_Error(BASETYPENOTSUPPORTED);
        }
    }
    else if(type->isStructType())
    {
        r_Structure_Type *stype = static_cast<r_Structure_Type *>(const_cast<r_Type*>(type));
        StructType *restype = new StructType("tmp_struct_name", stype->count_elements());
        for (unsigned int i = 0; i < stype->count_elements(); ++i)
        {
            try
            {
                r_Attribute attr = (*stype)[i];
                const r_Base_Type &attr_type = attr.type_of();
                restype->addElement(attr.name(), constructBaseType(&attr_type));
            }
            catch (r_Error &e)
            {
                delete restype;
                throw;
            }
        }
        TypeFactory::addTempType(restype);
        result = restype;
    }
    return result;
}


QtData*
QtConversion::evaluate( QtDataList* inputList )
{
    startTimer("QtConversion");

    QtData* returnValue = NULL;
    QtData* operand = NULL;
    r_Minterval* nullValues = NULL;

    if( conversionType == QT_UNKNOWN )
    {
        LFATAL << "Error: QtConversion::evaluate() - unknown conversion format.";
        parseInfo.setErrorNo(382);
        throw parseInfo;
    }

    operand = input->evaluate( inputList );

    if( operand )
    {
        char* typeStructure = NULL;
        Tile*   sourceTile    = NULL;

        if ( conversionType == QT_TOCSV && operand->isScalarData() )
        {
            QtScalarData* qtScalar = static_cast<QtScalarData*>(operand);
            r_Minterval dom = r_Minterval(2);
            dom << r_Sinterval(0LL, 0LL);
            dom << r_Sinterval(0LL, 0LL);
            sourceTile = new Tile( dom, qtScalar->getValueType(), qtScalar->getValueBuffer(),
                false /* use constructor which does not free the buffer pointer */ );
            typeStructure = qtScalar->getTypeStructure();
        }
        else
        {
#ifdef QT_RUNTIME_TYPE_CHECK
            if( operand->getDataType() != QT_MDD )
            {
                LERROR << "Internal error in QtConversion::evaluate() - "
                               << "runtime type checking failed (MDD).";

                // delete old operand
                if( operand ) operand->deleteRef();
                return 0;
            }
#endif

            QtMDD*  qtMDD         = static_cast<QtMDD*>(operand);
            MDDObj* currentMDDObj = qtMDD->getMDDObject();
            nullValues = currentMDDObj->getNullValues();
            vector< boost::shared_ptr<Tile> >* tiles = NULL;
            if (qtMDD->getLoadDomain().is_origin_fixed() && qtMDD->getLoadDomain().is_high_fixed())
            {
                // get relevant tiles
                tiles = currentMDDObj->intersect( qtMDD->getLoadDomain() );
            }
            else
            {
                LTRACE << "evaluate() - no tile available to convert.";
                return operand;
            }

            // check the number of tiles
            if( !tiles->size() )
            {
                LTRACE << "evaluate() - no tile available to convert.";
                return operand;
            }

            // create one single tile with the load domain
            sourceTile = new Tile( tiles, qtMDD->getLoadDomain() );

            // delete the tile vector
            delete tiles;
            tiles = NULL;

            // get type structure of the operand base type
            typeStructure = qtMDD->getCellType()->getTypeStructure();
        }

        // convert structure to r_Type
        r_Type* baseSchema = r_Type::get_any_type( typeStructure );

        free( typeStructure );
        typeStructure = NULL;
        LTRACE << "evaluate() - no tile available to convert.";
#ifdef DEBUG
        LTRACE << "Cell base type for conversion: ";
        baseSchema->print_status( RMInit::dbgOut );
#endif

        //
        // real conversion
        //

        r_convDesc convResult;
        r_Minterval tileDomain = sourceTile->getDomain();

        //convertor type
        r_Data_Format convType = r_Array;
        //result type from convertor
        r_Data_Format convFormat = r_Array;

        switch (conversionType)
        {
        case QT_TOTIFF:
            convType = r_TIFF;
            convFormat = r_TIFF;
            break;
        case QT_FROMTIFF:
            convType = r_TIFF;
            convFormat = r_Array;
            break;
        case QT_TOBMP:
            convType = r_BMP;
            convFormat = r_BMP;
            break;
        case QT_FROMBMP:
            convType = r_BMP;
            convFormat = r_Array;
            break;
        case QT_TOHDF:
            convType = r_HDF;
            convFormat = r_HDF;
            break;
        case QT_TONETCDF:
            convType = r_NETCDF;
            convFormat = r_NETCDF;
            break;
        case QT_TOCSV:
            convType = r_CSV;
            convFormat = r_CSV;
            break;
        case QT_FROMHDF:
            convType = r_HDF;
            convFormat = r_Array;
            break;
        case QT_FROMNETCDF:
            convType = r_NETCDF;
            convFormat = r_Array;
            break;
        case QT_FROMCSV:
            convType = r_CSV;
            convFormat = r_Array;
            break;
        case QT_TOJPEG:
            convType = r_JPEG;
            convFormat = r_JPEG;
            break;
        case QT_FROMJPEG:
            convType = r_JPEG;
            convFormat = r_Array;
            break;
        case QT_TOPNG:
            convType = r_PNG;
            convFormat = r_PNG;
            break;
        case QT_FROMPNG:
            convType = r_PNG;
            convFormat = r_Array;
            break;
        case QT_TOVFF:
            convType = r_VFF;
            convFormat = r_VFF;
            break;
        case QT_FROMVFF:
            convType = r_VFF;
            convFormat = r_Array;
            break;
        case QT_TOTOR:
            convType = r_TOR;
            convFormat = r_TOR;
            break;
        case QT_FROMTOR:
            convType = r_TOR;
            convFormat = r_Array;
            break;
        case QT_TODEM:
            convType = r_DEM;
            convFormat = r_DEM;
            break;
        case QT_FROMDEM:
            convType = r_DEM;
            convFormat = r_Array;
            break;
        default:
            LFATAL << "Error: QtConversion::evaluate(): unsupported format " << conversionType;
            throw r_Error(CONVERSIONFORMATNOTSUPPORTED);
            break;
        }

        r_Convertor *convertor = NULL;

        try
        {
            convertor = r_Convertor_Factory::create(convType, sourceTile->getContents(), tileDomain, baseSchema);
            if(conversionType < QT_FROMTIFF)
            {
                LTRACE << "evaluate() - convertor " << convType << " converting to " << convFormat << "...";
                convResult = convertor->convertTo(paramStr);
            }
            else
            {
                LTRACE << "evaluate() - convertor " << convType << " converting from " << convFormat << "...";
                convResult = convertor->convertFrom(paramStr);
            }
        }
        catch (r_Error &err)
        {
            delete sourceTile;
            sourceTile = NULL;
            delete baseSchema;
            baseSchema = NULL;
            // delete old operand
            if( operand ) operand->deleteRef();
            if (convertor != NULL)
            {
                delete convertor;
                convertor=NULL;
            }

            parseInfo.setErrorNo(381);
            throw parseInfo;
        }
        if (convertor != NULL)
        {
            delete convertor;
            convertor=NULL;
        }

        LTRACE << "evaluate() - ok";


        //
        // done
        //

        // delete sourceTile
        delete sourceTile;
        sourceTile = NULL;

        // create a transient tile for the compressed data
        const BaseType* myType = NULL;
        const r_Type* currStruct = NULL;
        myType = constructBaseType( convResult.destType );

        if(strcasecmp( dataStreamType.getType()->getTypeName(), myType->getTypeName()) )
        {
            //FIXME here we have to change the dataStreamType.getType(), is not char for
            //conversion from_DEF all the time, we don't know the result type until we parse the data
            MDDBaseType* mddBaseType = new MDDBaseType( "tmp", myType );
            TypeFactory::addTempType( mddBaseType );

            dataStreamType.setType( mddBaseType );
#ifdef DEBUG
            LDEBUG << " QtConversion::evaluate() for conversion " << conversionType << " real result is ";

            dataStreamType.printStatus(RMInit::logOut);
#endif
        }

        long convResultSize = static_cast<long>(convResult.destInterv.cell_count()) * static_cast<long>(myType->getSize());
        
        Tile* resultTile = new Tile( convResult.destInterv,
                                     myType,
                                     convResult.dest,
                                     convResultSize,
                                     convFormat );

        // delete destination type
        if( convResult.destType )
        {
            delete convResult.destType;
            convResult.destType=NULL;
        }

        // create a transient MDD object for the query result
        MDDBaseType* mddBaseType = static_cast<MDDBaseType*>(const_cast<Type*>(dataStreamType.getType()));
        MDDObj* resultMDD = new MDDObj( mddBaseType, convResult.destInterv, nullValues );
        resultMDD->insertTile( resultTile );

        // create a new QtMDD object as carrier object for the transient MDD object
        returnValue = new QtMDD( static_cast<MDDObj*>(resultMDD) );
        (static_cast<QtMDD*>(returnValue))->setFromConversion(true);

        // delete base type schema
        delete baseSchema;
        baseSchema = NULL;

        // delete old operand
        if( operand ) operand->deleteRef();
    }
    else
        LERROR << "Error: QtConversion::evaluate() - operand is not provided.";
    
    stopTimer();

    return returnValue;
}


void
QtConversion::printTree( int tab, ostream& s, QtChildType mode )
{
    s << SPACE_STR(static_cast<size_t>(tab)).c_str() << "QtConversion Object: ";

    switch( conversionType )
    {
    case QT_TOTIFF:
        s << "to TIFF";
        break;
    case QT_TOBMP:
        s << "to BMP";
        break;
    case QT_TOHDF:
        s << "to HDF";
        break;
    case QT_TONETCDF:
        s << "to NETCDF";
        break;
    case QT_TOCSV:
        s << "to CSV";
        break;
    case QT_TOJPEG:
        s << "to JPEG";
        break;
    case QT_TOPNG:
        s << "to PNG";
        break;
    case QT_TOVFF:
        s << "to VFF";
        break;
    case QT_TOTOR:
        s << "to TOR";
        break;
    case QT_TODEM:
        s << "to DEM";
        break;
    case QT_FROMTIFF:
        s << "from TIFF";
        break;
    case QT_FROMBMP:
        s << "from BMP";
        break;
    case QT_FROMHDF:
        s << "from HDF";
        break;
    case QT_FROMNETCDF:
        s << "from NETCDF";
        break;
    case QT_FROMCSV:
        s << "from CSV";
        break;
    case QT_FROMJPEG:
        s << "from JPEG";
        break;
    case QT_FROMPNG:
        s << "from PNG";
        break;
    case QT_FROMVFF:
        s << "from VFF";
        break;
    case QT_FROMTOR:
        s << "from TOR";
        break;
    case QT_FROMDEM:
        s << "from DEM";
        break;
    default:
        s << "unknown conversion";
        break;
    }

    s << getEvaluationTime();
    s << std::endl;

    QtUnaryOperation::printTree( tab, s, mode );
}



void
QtConversion::printAlgebraicExpression( ostream& s )
{
    s << conversionType << "(";

    if( input )
        input->printAlgebraicExpression( s );
    else
        s << "<nn>";

    s << ")";
}



const QtTypeElement&
QtConversion::checkType( QtTypeTuple* typeTuple )
{
    dataStreamType.setDataType( QT_TYPE_UNKNOWN );

    // check operand branches
    if( input )
    {

        // get input type
        const QtTypeElement& inputType = input->checkType( typeTuple );

        if( conversionType != QT_TOCSV && inputType.getDataType() != QT_MDD )
        {
            LFATAL << "Error: QtConversion::checkType() - operand is not of type MDD.";
            parseInfo.setErrorNo(380);
            throw parseInfo;
        }

        //FIXME we set for every kind of conversion the result type char
        //for conversion from_DEF we don't know the result type until we parse the data
        MDDBaseType* mddBaseType = new MDDBaseType( "Char", TypeFactory::mapType("Char") );
        TypeFactory::addTempType( mddBaseType );

        dataStreamType.setType( mddBaseType );

        if(conversionType>QT_TODEM)
        {
#ifdef DEBUG
            LINFO << "QtConversion::checkType() for conversion "
                           << conversionType << " assume the result ";
            dataStreamType.printStatus(RMInit::logOut);
#endif
        }
    }
    else
        LERROR << "Error: QtConversion::checkType() - operand branch invalid.";

    return dataStreamType;
}

std::ostream&
operator<< (std::ostream& os, QtConversion::QtConversionType type)
{
    switch( type )
    {
    case QtConversion::QT_TOTIFF:
        os << "tiff";
        break;
    case QtConversion::QT_TOBMP:
        os << "bmp";
        break;
    case QtConversion::QT_TOHDF:
        os << "hdf";
        break;
    case QtConversion::QT_TOCSV:
        os << "csv";
        break;
    case QtConversion::QT_TOJPEG:
        os << "jpeg";
        break;
    case QtConversion::QT_TOPNG:
        os << "png";
        break;
    case QtConversion::QT_TOVFF:
        os << "vff";
        break;
    case QtConversion::QT_TOTOR:
        os << "tor";
        break;
    case QtConversion::QT_TODEM:
        os << "dem";
        break;
    case QtConversion::QT_TONETCDF:
        os << "netcdf";
        break;
    case QtConversion::QT_FROMTIFF:
        os << "inv_tiff";
        break;
    case QtConversion::QT_FROMBMP:
        os << "inv_bmp";
        break;
    case QtConversion::QT_FROMHDF:
        os << "inv_hdf";
        break;
    case QtConversion::QT_FROMCSV:
        os << "inv_csv";
        break;
    case QtConversion::QT_FROMJPEG:
        os << "inv_jpeg";
        break;
    case QtConversion::QT_FROMPNG:
        os << "inv_png";
        break;
    case QtConversion::QT_FROMVFF:
        os << "inv_vff";
        break;
    case QtConversion::QT_FROMTOR:
        os << "inv_tor";
        break;
    case QtConversion::QT_FROMDEM:
        os << "inv_dem";
        break;
    case QtConversion::QT_FROMNETCDF:
        os << "inv_netcdf";
        break;
    default:
        os << "unknown Conversion";
        break;
    }

    return os;
}

