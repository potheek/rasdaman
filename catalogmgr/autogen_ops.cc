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
 *
 *
 * COMMENTS:    Automaticaly generated
 *
 ************************************************************/


#include "config.h"
#include <cmath>
#include <cerrno>


OpABSCDouble::OpABSCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpABSCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = fabs(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpSQRTCDouble::OpSQRTCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpSQRTCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = sqrt(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpPOWCDouble::OpPOWCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpPOWCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = pow(convOp, exponent);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

void OpPOWCDouble::setExponent(double newExponent)
{
    exponent = newExponent;
}

OpEXPCDouble::OpEXPCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpEXPCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = exp(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpLOGCDouble::OpLOGCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpLOGCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = log10(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpLNCDouble::OpLNCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpLNCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = log(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpSINCDouble::OpSINCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpSINCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = sin(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpCOSCDouble::OpCOSCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpCOSCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = cos(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpTANCDouble::OpTANCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpTANCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = tan(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpSINHCDouble::OpSINHCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpSINHCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = sin(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpCOSHCDouble::OpCOSHCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpCOSHCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = cos(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpTANHCDouble::OpTANHCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpTANHCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = tan(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpARCSINCDouble::OpARCSINCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpARCSINCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = asin(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpARCCOSCDouble::OpARCCOSCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpARCCOSCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = acos(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

OpARCTANCDouble::OpARCTANCDouble(
    const BaseType* newResType,
    const BaseType* newOpType,
    unsigned int newResOff,
    unsigned int newOpOff)
    : UnaryOp(newResType, newOpType, newResOff, newOpOff) {}


void OpARCTANCDouble::operator()(char* res, const char* op)
{
    double convOp = *(opType->convertToCDouble(op + opOff, &convOp));
    double convRes;
    errno = 0;
    if (isNull(convOp))
    {
        convRes = convOp;
    }
    else
    {
        convRes = atan(convOp);
    }
    if(errno)
    {
        if(errno == EDOM) throw 510;
        if(errno == ERANGE) throw 511;
    }
    resType->makeFromCDouble(res + resOff, &convRes);
}

