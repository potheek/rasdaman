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
#ifndef RASSERVER_CONFIG_HH
#define RASSERVER_CONFIG_HH


#include "commline/cmlparser.hh"

class Configuration
  {
    public:
      Configuration();
      
      bool parseCommandLine(int argc, char** argv);
      
      const char* getServerName();
      int         getListenPort();
      bool        isHttpServer();
      bool        isRnpServer();
      
      const char* getRasmgrHost();
      int         getRasmgrPort();      
      bool        isLogToStdOut();
      
      int         getDefaultOptimizationLevel();
      int         getMaxTransferBufferSize();
      int         getTimeout();
      const char* getDbConnectionID();
      
#ifdef RMANDEBUG
      int         getDefaultTileSize();
      int         getDefaultPCTMin();
      int         getDefaultPCTMax();
      
      int         getDefaultIndexSize();
      
      int         getDebugLevel();
      
      const char* getDefaultTileConfig();
      const char* getTilingScheme();
      const char* getIndexType();
      bool        useTileContainer();
      
#endif      
    private:
      void printHelp();
      
      void initParameters();
      void checkParameters();
         void initLogFiles();
	 void deprecated(CommandLineParameter*);

      const char* makeLogFileName(const char *srvName,const char *desExt);

      // Parameters
      CommandLineParameter *cmlHelp;
      CommandLineParameter *cmlRsn;
      CommandLineParameter *cmlPort;
      CommandLineParameter *cmlMgr;
      CommandLineParameter *cmlMgrPort;
      CommandLineParameter *cmlMgrSync;

      CommandLineParameter *cmlTransBuffer;
      CommandLineParameter *cmlTimeOut;
      CommandLineParameter *cmlMgmntInt;
      CommandLineParameter *cmlHttp;
      CommandLineParameter *cmlRnp;

      CommandLineParameter *cmlOptLevel;
      CommandLineParameter *cmlConnectStr;
      CommandLineParameter *cmlLog;

#ifdef RMANDEBUG
      CommandLineParameter *cmlTileSize;
      CommandLineParameter *cmlPctMin;
      CommandLineParameter *cmlPctMax;
      CommandLineParameter *cmlUseTC;
      CommandLineParameter *cmlTileConf;
      CommandLineParameter *cmlTiling;
      CommandLineParameter *cmlIndex;
      CommandLineParameter *cmlIndexSize;
      CommandLineParameter *cmlDbg;
      CommandLineParameter *cmlDbgLevel;
#endif
      const char* myExecutable;
      
      const char* serverName;
      int         listenPort;
      
      const char* rasmgrHost;
      int         rasmgrPort;
      
      bool        logToStdOut;
      const char* logFileName; // == 0 if stdout
      
      int         maxTransferBufferSize;
      int         timeout;
      bool        httpServ;
      bool        rnpServ;
      int         optLevel;
      const char* dbConnection;

#ifdef RMANDEBUG
      int         tileSize;
      int         pctMin;
      int         pctMax;
      bool        useTC;
      const char* tileConf;
      const char* tilingName;
      const char* indexType;
      int         indexSize;
      int         dbgLevel;
#endif      
   };

extern Configuration configuration;

#endif
