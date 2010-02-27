/*
 * This file is part of PetaScope.
 *
 * PetaScope is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * PetaScope is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PetaScope. If not, see <http://www.gnu.org/licenses/>.
 *
 * For more information please see <http://www.PetaScope.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 *
 * Copyright 2009 Jacobs University Bremen, Peter Baumann.
 */
package petascope.wcps.server.core;

import petascope.wcps.server.exceptions.InvalidCrsException;
import petascope.wcps.server.exceptions.WCPSException;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.w3c.dom.*;

public class ExtendCoverageExpr implements IRasNode, ICoverageInfo {

    private List<DimensionIntervalElement> axisList;
    private CoverageExpr coverageExprType;
    private CoverageInfo coverageInfo;
    private String[] dim;
    private int dims;
    private DimensionIntervalElement elem;

    public ExtendCoverageExpr(Node node, XmlQuery xq) throws WCPSException, InvalidCrsException {

        Node child, axisNode;
        String nodeName;

        axisList = new ArrayList<DimensionIntervalElement>();

        child = node.getFirstChild();
        while (child != null) {
            nodeName = child.getNodeName();

            if (nodeName.equals("#text")) {
                child = child.getNextSibling();
                continue;
            }

            try {
                System.err.println("Trying out an CoverageExprType group...");
                coverageExprType = new CoverageExpr(node, xq);
                coverageInfo = coverageExprType.getCoverageInfo();
                child = child.getNextSibling();
                continue;
            } catch (WCPSException e) {
                System.err.println("This was no CoverageExprType: " + nodeName);
            }

            try {
                // Start a new axis and save it
                elem = new DimensionIntervalElement(node, xq, coverageInfo);
                axisList.add(elem);
                child = elem.getNextNode();
                continue;
            } catch (WCPSException e) {
                System.err.println("This was no Dimension Interval ELement: " + child.getNodeName());
            }

            // else unknown element
            throw new WCPSException("Unknown node for ExtendCoverage expression:" + child.getNodeName());
        }

        dims = coverageInfo.getNumDimensions();
        dim = new String[dims];

        for (int j = 0; j < dims; ++j) {
            dim[j] = "*:*";
        }


        Iterator<DimensionIntervalElement> i = axisList.iterator();
        DimensionIntervalElement axis;
        int axisId;
        int axisLo, axisHi;

        while (i.hasNext()) {
            axis = i.next();
            axisId = coverageInfo.getDomainIndexByName(axis.getAxisName());
            System.out.println("Axis ID: " + axisId);
            System.out.println("Axis name: " + axis.getAxisName());
            System.out.print("Axis coords: ");

            axisLo = Integer.parseInt(axis.getLowCoord());
            axisHi = Integer.parseInt(axis.getHighCoord());
            dim[axisId] = axisLo + ":" + axisHi;
            coverageInfo.setCellDimension(
                    axisId,
                    new CellDomainElement(
                    BigInteger.valueOf(axisLo), BigInteger.valueOf(axisHi)));
        }


    }

    public CoverageInfo getCoverageInfo() {
        return coverageInfo;
    }

    public String toRasQL() {
        String result = "extend(" + coverageExprType.toRasQL() + ",[";

        for (int j = 0; j < dims; ++j) {
            if (j > 0) {
                result += ",";
            }

            result += dim[j];
        }

        result += "])";

        return result;
    }
}
