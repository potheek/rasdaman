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
package petascope.wcps.grammar;

/**
 * MetaDataExpr
 * Creation date: (3/3/2003 2:28:43 AM)
 * @author: mattia parigiani, Sorin Stancu-Mara, Andrei Aiordachioaie
 */
public class MetaDataExpr implements IParseTreeNode {

    CoverageExpr expr;
    String field;
    String function;
    IParseTreeNode param;

    public MetaDataExpr(DomainExpr dom) {
        function = "domain";
        param = dom;
    }

    // Identifier, ImageCRS, ImageCRSDomain, CrsSet, NullSet
    public MetaDataExpr(String op, CoverageExpr expr) {
        function = op;
        this.expr = expr;
    }

    // ImageCRSDomain, interpolationDefault, interpolationSet
    public MetaDataExpr(String op, CoverageExpr expr, String str) {
        function = op;
        this.expr = expr;
        this.field = str;
    }

    public String toXML() {
        String result = "";

        if (function.equalsIgnoreCase("imageCrsDomain")) {
            result += "<imageCrsDomain>";
            result += expr.toXML();

            if (field != null) {
                result += "<axis>" + field + "</axis>";
            }

            result += "</imageCrsDomain>";
        } else if (function.equalsIgnoreCase("domain")) {
            result = "<DomainMetadata>" + param.toXML() + "</DomainMetadata>";
        } else if (function.equalsIgnoreCase("interpolationDefault")) {
            result += "<interpolationDefault>";
            result += expr.toXML();
            result += "<name>" + param + "</param>";
            result += "</interpolationDefault>";
        } else if (function.equalsIgnoreCase("interpolationSet")) {
            result += "<interpolationSet>";
            result += expr.toXML();
            result += "<name>" + param + "</param>";
            result += "</interpolationSet>";
        } else if (function.equalsIgnoreCase("identifier")
                || function.equalsIgnoreCase("imageCRS") || function.equalsIgnoreCase("crsSet")
                || function.equalsIgnoreCase("nullSet")) {
            result += "<" + function + ">";
            result += expr.toXML();
            result += "</" + function + ">";
        } else {
            System.err.println("Unknown MetadataExpr operation: " + function);
        }

        return result;
    }
}
