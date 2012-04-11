/** ***********************************************************
 *
 * SOURCE: CommanderException.java
 *
 * PACKAGE: petascope.wms.commander
 * CLASS: CommanderException
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
 *   $RCSfile: CommanderException.java,v $ $Revision: 1.1 $ $State: Exp $
 *   $Locker:  $
 */

package petascope.wms.commander;

/**
 * This is the superclass of all exceptions.
 **/
public class CommanderException extends Exception
{
    private static final long serialVersionUID = 1L;
    
    // --- instance variables -----------------------------------------
    
    /**
     * The error message.
     **/
    protected String msg = null;
    
    /**
     * Supported number of parameters.
     **/
    protected int nop = 3;
    
    /**
     * Parameters for the error message.
     **/
    protected String[] p;
    
    // --- instance methods -------------------------------------------
    
    /**
     * Standard constructor getting the error message.
     * @param message String
     **/
    public CommanderException(String message)
    {
        super();
        msg = message;
        printStackTrace();
        p = new String[nop];
        for (int i=0; i<nop; i++)
            p[i] = null;
    }
    
    /**
     * Constructor getting the error message and 1 parameter.
     * @param message String
     * @param param1 String
     **/
    public CommanderException(String message, String param1)
    {
        super();
        msg = message;
        printStackTrace();
        p = new String[nop];
        p[0] = param1;
        for (int i=1; i<nop; i++)
            p[i] = null;
    }
    
    /**
     * Constructor getting the error message and 2 parameters.
     * @param message String
     * @param param1 String
     * @param param2 String
     **/
    public CommanderException(String message, String param1, String param2)
    {
        super();
        msg = message;
        printStackTrace();
        p = new String[nop];
        p[0] = param1;
        p[1] = param2;
        for (int i=2; i<nop; i++)
            p[i] = null;
    }
    
    /**
     * Constructor getting the error message and 3 parameters.
     * @param message String
     * @param param1 String
     * @param param2 String
     * @param param3 String
     **/
    public CommanderException(String message, String param1, String param2, String param3)
    {
        super();
        msg = message;
        printStackTrace();
        p = new String[nop];
        p[0] = param1;
        p[1] = param2;
        p[2] = param3;
    }
    
    /**
     * Substitutes parameters and returns the error message.
     * @return String
     **/
    public String getMessage()
    {
        StringBuffer buf = new StringBuffer(msg);
        for (int i=1; i<=nop; i++)
        {
            int index = msg.indexOf("$"+i);
            if(index != -1 && p[i-1] != null)
                buf.replace(index,index+2,p[i-1]);
            msg = buf.toString();
        }
        return msg;
    }
    

}

