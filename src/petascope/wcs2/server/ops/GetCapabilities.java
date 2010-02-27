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
package petascope.wcs2.server.ops;

//~--- non-JDK imports --------------------------------------------------------
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import petascope.ConfigManager;

import petascope.wcps.server.core.DbMetadataSource;
import petascope.wcps.server.exceptions.ResourceException;

//~--- JDK imports ------------------------------------------------------------

import java.util.Iterator;
import petascope.wcs.server.exceptions.InvalidServiceConfigurationException;

/**
 * GetCapabilities operation for The Web Coverage Service 2.0
 *
 * @author Andrei Aiordachioaie
 */
public class GetCapabilities implements WcsOperation {

    private static Logger LOG = LoggerFactory.getLogger(GetCapabilities.class);
    /* Template XMLs for response types */
    private String GetCapabilitiesResponse;
    private DbMetadataSource meta;
    /* Other useful vars */
    private String xmlListOfCoverages;

    public GetCapabilities(DbMetadataSource metadata) throws InvalidServiceConfigurationException {
        meta = metadata;

        GetCapabilitiesResponse = ConfigManager.WCS2_GET_CAPABILITIES_TEMPLATE;
        if (GetCapabilitiesResponse == null) {
            throw new InvalidServiceConfigurationException("Could not find template file.");
        }
    }

    @Override
    public String execute(String input) {
        String output;

        // Create the output by replacing placeholders
        output = GetCapabilitiesResponse.replaceAll("\\{URL\\}",
                ConfigManager.PETASCOPE_SERVLET_URL);
        Iterator<String> it;

        try {
            it = meta.coverages().iterator();
            xmlListOfCoverages = "";
            while (it.hasNext()) {
                xmlListOfCoverages += "<wcs:id gml:id=\"" + it.next() + "\"/>";
            }
        } catch (ResourceException ex) {
            ex.printStackTrace();
        }
        output = output.replaceAll("\\{Coverages\\}", xmlListOfCoverages);
        output = output.replaceAll("\\{wcsSchemaUrl\\}", ConfigManager.WCS2_SCHEMA_URL);

        return output;
    }
}
