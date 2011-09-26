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
 * Copyright 2003 - 2010 Peter Baumann / rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 */
package petascope.wcps.server.core;

import petascope.core.Metadata;
import java.util.Iterator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import petascope.exceptions.WCPSException;
import org.w3c.dom.*;
import petascope.exceptions.WCSException;
import petascope.exceptions.ExceptionCode;

public class Crs implements IRasNode {

    Logger log = LoggerFactory.getLogger(Crs.class);
    private String crsName;

    public Crs(Node node, XmlQuery xq) throws WCPSException {
        while ((node != null) && node.getNodeName().equals("#text")) {
            node = node.getNextSibling();
        }

        if (node != null && node.getNodeName().equals("srsName")) {
            String val = node.getTextContent();
            this.crsName = val;
            if (crsName.equals(DomainElement.IMAGE_CRS) || crsName.equals(DomainElement.WGS84_CRS)) {
                System.err.println("Found CRS: " + crsName);
            } else {
                throw new WCPSException("Invalid CRS: '" + crsName + "'");
            }
        } else {
            throw new WCPSException("Could not find a 'srsName' node !");
        }
    }

    /***
     * Converts an array of 4 coordinates (bounding box) expressed in the
     * current CRS to pixel coordinates.
     * @param u2 Left-most  X point (CRS coordinate)
     * @param u3 Right-most X point (CRS coordinate)
     * @param v2 Lower-most Y point (CRS coordinate)
     * @param v3 Upper-most Y point (CRS coordinate)
     * @return array of integers, pixel coordinates that represent the given CRS
     * coordinates.
     */
    public long[] convertToPixelCoordinates(Metadata meta, String axisName, Double u2, Double u3, Double v2, Double v3) throws WCSException {
        Wgs84Crs crs = meta.getCrs();
        long px0 = -1, px1 = -1, py0 = -1, py1 = -1;
        // Convert bounding box values to pixel coordinates
        if (crsName.equals(DomainElement.WGS84_CRS)) {
            log.trace("Converting WGS84 axis {} interval to pixel coordinates ...", axisName);
            /* Image coordinates */
            Iterator<CellDomainElement> it = meta.getCellDomainIterator();
            CellDomainElement X = it.next();
            CellDomainElement Y = it.next();
            if (X == null || Y == null) {
                log.error("Could not find the X or Y axis for coverage: " + meta.getCoverageName());
                throw new WCSException(ExceptionCode.NoApplicableCode, "Could not find the X or Y axis for coverage: " + meta.getCoverageName());
            }
            int x0 = X.getLo().intValue();
            int x1 = X.getHi().intValue();
            int y0 = Y.getLo().intValue();
            int y1 = Y.getHi().intValue();

            log.trace("Pixel Coordinates: X01 (" + x0 + "," + x1 + ") + Y01 (" + y0 + "," + y1 + ")");
            /* CRS span */
            double x2 = crs.getLow1();
            double y2 = crs.getLow2();
            double x3 = crs.getHigh1();
            double y3 = crs.getHigh2();
            log.trace("CRS Coordinates: X23 (" + x2 + "," + x3 + ") + Y23 (" + y2 + "," + y3 + ")");
            /* For WGS84, the offset = (# pixels)/(CRS span) */
            double oX = crs.getOffset1();
            double oY = crs.getOffset2();

            /* The actual conversion is below: */
            if (axisName.equals("X")) {
                px0 = Math.round((u2 - x2) / oX) + x0;
                px1 = Math.round((u3 - u2) / oX) + px0;
                log.debug("CRS Coordinates on axis X: U23 (" + u2 + "," + u3 + ")");
                log.debug("Pixel Coordinates on axis X: U01 (" + px0 + "," + px1 + ") ");
            }
            if (axisName.equals("Y")) {
                py0 = Math.round((y3 - v3) / oY) + y0;
                py1 = Math.round((v3 - v2) / oY) + py0;
                log.debug("CRS Coordinates on axis Y:  V23 (" + v2 + "," + v3 + ")");
                log.debug("Pixel Coordinates on axis Y: V01 (" + py0 + "," + py1 + ")");
            }

        }
        long[] longCoord = {px0, px1, py0, py1};

        return longCoord;
    }

    public String toRasQL() {
        return crsName;
    }

    public String getName() {
        return crsName;
    }
}