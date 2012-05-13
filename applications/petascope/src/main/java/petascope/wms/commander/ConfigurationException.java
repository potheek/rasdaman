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
/** ***********************************************************
 *
 * SOURCE: ConfigurationException.java
 *
 * PACKAGE: petascope.wms.commander
 * CLASS: ConfigurationException
 *
 * CHANGE HISTORY (append further entries):
 * when         who       what
 * ----------------------------------------------------------
 * 2007-jan-05  PB        created
 *
 * Copyright (C) 2007 Dr. Peter Baumann
 *
 *********************************************************** */

/*
 * RCS:
 *   $RCSfile: ConfigurationException.java,v $ $Revision: 1.1 $ $State: Exp $
 *   $Locker:  $
 */

package petascope.wms.commander;

/**
 * ConfigurationException: error evaluating configuration file
 */
public class ConfigurationException extends CommanderException
{
    private static final long serialVersionUID = 1L;
    
    /**
     * Constructor for ConfigurationException
     * @param msg String
     */
    public ConfigurationException( String msg )
    {
        super( msg );
    }

}
