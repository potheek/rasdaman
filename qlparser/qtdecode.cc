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
 * MERCHANTrABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include "qlparser/gdalincludes.hh"
#include "relcatalogif/structtype.hh"
#include "config.h"
#include "debug.hh"
#include <float.h>

#include "catalogmgr/typefactory.hh"
#include "qlparser/qtdecode.hh"
#include "qlparser/qtmdd.hh"
#include "qlparser/qtmintervaldata.hh"
#include "qlparser/typeresolverutil.hh"
#include "qlparser/gdaldataconverter.hh"

#include "raslib/error.hh"
#include "raslib/primitivetype.hh"
#include "raslib/structuretype.hh"
#include "catalogmgr/ops.hh"
#include "relcatalogif/basetype.hh"
#include "tilemgr/tile.hh"
#include "mddmgr/mddobj.hh"

#include "qlparser/qtoncstream.hh"
#include "qlparser/qtexecute.hh"

#include <easylogging++.h>

#include <iostream>
#ifndef CPPSTDLIB
#include <ospace/string.h>
#else
#include <string>
using namespace std;
#endif

#ifndef DATA_CHUNK_SIZE
#define DATA_CHUNK_SIZE 10000 //no of bytes to be written in a file
#endif

const QtNode::QtNodeType QtDecode::nodeType = QtNode::QT_DECODE;

QtDecode::QtDecode(QtOperation* newInput) throw (r_Error)
: QtUnaryOperation(newInput)
{
	GDALAllRegister();
	format = NULL;
	gdalParams = NULL;
}

QtDecode::QtDecode(QtOperation* newInput, char* formatArg, char* gdalParamsArg) throw (r_Error) :
QtUnaryOperation(newInput), format(formatArg)
{
	GDALAllRegister();
	initGdalParamas(gdalParamsArg);
}

QtData* QtDecode::evaluate(QtDataList* inputList) throw (r_Error)
{
	startTimer("QtDecode");

	QtData* operand = NULL;

	operand = input->evaluate(inputList);

	if (operand)
	{
		Tile* sourceTile = NULL;

		// Perform the actual evaluation
		QtMDD* qtMDD = static_cast<QtMDD*>(operand);
		MDDObj* currentMDDObj = qtMDD->getMDDObject();
		vector< boost::shared_ptr<Tile> >* tiles = NULL;
		if (qtMDD->getLoadDomain().is_origin_fixed() && qtMDD->getLoadDomain().is_high_fixed())
		{
			// get relevant tiles
			tiles = currentMDDObj->intersect(qtMDD->getLoadDomain());
		} else
		{
			LTRACE << "evaluate() - no tile available to convert.";
			return operand;
		}

		// check the number of tiles
		if (!tiles->size())
		{
			LTRACE << "evaluate() - no tile available to convert.";
			return operand;
		}

		// create one single tile with the load domain
		sourceTile = new Tile(tiles, qtMDD->getLoadDomain());

		// delete the tile vector
		delete tiles;
		tiles = NULL;

		const char TMPFILE_TEMPLATE[] = "/tmp/rasdaman-XXXXXX";
		char tmpFileName[sizeof(TMPFILE_TEMPLATE)];
		strcpy(tmpFileName, TMPFILE_TEMPLATE);
		createTemporaryImageFile(tmpFileName, sourceTile);

		GDALDataset *poDataset;
		poDataset = static_cast<GDALDataset*>(GDALOpen(tmpFileName, GA_ReadOnly));

		if (poDataset == NULL)
		{
                    LFATAL << "QtDecode::evaluate() - failed opening file with GDAL, eror: " << CPLGetLastErrorMsg();
                    unlink(tmpFileName);
                    throw r_Error(r_Error::r_Error_FeatureNotSupported);
		}

		/*if the format is specified as the second parameter of decode()
		  then we pass the gdal parameters and create a new gdal data set*/
		if (format != NULL)
		{
			GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(format);
			if (driver == NULL)
			{
                            LFATAL << "QtDecode::evaluateMDD - Error: Unsupported format: " << format;
                            throw r_Error(r_Error::r_Error_FeatureNotSupported);
			}
			poDataset = driver->CreateCopy(tmpFileName, poDataset, FALSE, gdalParams, NULL, NULL);
		}

		int width = poDataset->GetRasterXSize();
		int height = poDataset->GetRasterYSize();

		BaseType* baseType = TypeResolverUtil::getBaseType(poDataset);
		/*WARNING: GDALDataConverter::getTileCells() closes the GDAL dataset*/
        char* fileContents;
        r_Bytes dataSize;

        GDALDataConverter::getTileCells(poDataset, /* out */ dataSize, /* out */ fileContents);
		unlink(tmpFileName);

		//TODO-GM: Add support for 1D and 3D files
		r_Minterval mddDomain = r_Minterval(2) << r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(width) - 1)
				<< r_Sinterval(static_cast<r_Range>(0), static_cast<r_Range>(height) - 1);

        Tile* resultTile = new Tile(mddDomain, baseType, fileContents, dataSize, r_Array);
		MDDDimensionType* mddDimensionType = new MDDDimensionType("tmp_dim_type_name", baseType, 2);
		TypeFactory::addTempType(mddDimensionType);

		MDDObj* resultMDD = new MDDObj(mddDimensionType, resultTile->getDomain());
		resultMDD->insertTile(resultTile);

		QtMDD* returnValue = new QtMDD(static_cast<MDDObj*>(resultMDD));
        returnValue->setFromConversion(true);

		if (operand)
		{
			operand->deleteRef();
        }

		return returnValue;
	} else
	{
		LERROR << "Error: QtDecode::evaluate() - operand is not provided.";
	}

	stopTimer();
	return NULL;
}

void QtDecode::createTemporaryImageFile(char* tmpFileName, Tile* sourceTile)
{
	int fd = mkstemp(tmpFileName);
	if (fd < 1)
	{
		LFATAL << "QtDecode::evaluateMDD - Error: Creation of temp file failed with error:\n" << strerror(errno);
		throw r_Error(r_Error::r_Error_General);
	}

	r_Bytes lg = 0;
	r_Bytes fileSize = sourceTile->getSize();

	char* buff = sourceTile->getContents();

	//write chunks of data in the file until no chunks are left
	while (lg + DATA_CHUNK_SIZE < fileSize)
	{
		lg += write(fd, buff + lg, DATA_CHUNK_SIZE);
	}
	ssize_t wsize = write(fd, buff + lg, fileSize - lg);
	close(fd);
}

void QtDecode::initGdalParamas(char* params)
{
	gdalParams = CSLTokenizeString2(params, PARAM_SEPARATOR, CSLT_STRIPLEADSPACES |
			CSLT_STRIPENDSPACES);

}

const QtTypeElement& QtDecode::checkType(QtTypeTuple* typeTuple)
{
    dataStreamType.setDataType(QT_TYPE_UNKNOWN);

	// check operand branches
	if (input)
	{
		// get input types
		const QtTypeElement& inputType = input->checkType(typeTuple);
#ifdef DEBUG
                LTRACE << "Class..: QtDecode";
                LTRACE << "Operand: ";
                inputType.printStatus(RMInit::dbgOut);
#endif

		if (inputType.getDataType() != QT_MDD)
		{
			LFATAL << "Error: QtDecode::evaluate() - operand must be an MDD.";
			parseInfo.setErrorNo(353);
			throw parseInfo;
		}

        /*
         * we set for every kind of conversion the result type char for conversion because we
         * don't know the result type until we parse the data
        */
        MDDBaseType* mddBaseType = new MDDBaseType( "Char", TypeFactory::mapType("Char") );
        TypeFactory::addTempType( mddBaseType );
        dataStreamType.setType( mddBaseType );

    } else
		LERROR << "Error: QtDecode::checkType() - operand branch invalid.";

	return dataStreamType;
}

void QtDecode::printTree(int tab, std::ostream& s, QtChildType mode)
{
	s << SPACE_STR(static_cast<size_t>(tab)).c_str() << "QtDecode Object: " << getEvaluationTime() << endl;

	QtUnaryOperation::printTree(tab, s, mode);
}

QtNode::QtNodeType QtDecode::getNodeType() const
{
	return nodeType;
}

QtDecode::~QtDecode()
{
}
