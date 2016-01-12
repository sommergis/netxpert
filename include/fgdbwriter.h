#ifndef FGDBWRITER_H
#define FGDBWRITER_H

#include <string>
#include "dbwriter.h"
#include "logger.h"
#include <fstream>
#include "FileGDB_API/include/FileGDBAPI.h"
#include "utils.h"
#include <algorithm>
//#include <boost/algorithm/string.hpp>

using namespace std;
using namespace geos;

namespace netxpert {

    /**
    * \Class Writes the result of NetXpert into a ESRI FileGeodatabase
    **/
    class FGDBWriter : public DBWriter
    {
        public:
            FGDBWriter( Config& netxpertConfig );
            virtual ~FGDBWriter();
            virtual void CommitCurrentTransaction();
            virtual void CreateNetXpertDB();
            virtual void CreateSolverResultTable(const std::string& _tableName, const NetXpertSolver solverType);
            virtual void CreateSolverResultTable(const std::string& _tableName, const NetXpertSolver solverType,
                                                 const bool dropFirst);
            virtual void OpenNewTransaction();
            /**
            * \Brief For saving the result arc of a built network into the netXpert result DB
            */
            void SaveNetworkBuilderArc(const std::string& extArcID, const unsigned int fromNode,
                                       const unsigned int toNode, const double cost,
                                       const double capacity, const std::string& oneway,
                                       const geos::geom::Geometry& arc,
                                       const std::string& _tableName);

            //MCF, TP
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const double capacity, const double flow, const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            //SPT, ODM
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            //MST
            void SaveResultArc(const std::string& originalArcID, const double cost,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            //Isolines
            void SaveResultArc(const std::string& orig, const double cost,
                               const double cutoff,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);

            virtual void CloseConnection();
        private:
            unique_ptr<FileGDBAPI::Geodatabase> geodatabasePtr;
            void connect();
            bool isConnected = false;
            void createTable (const std::string& _tableName, const NetXpertSolver solverType);
            void openTable(const string& _tableName);
            void dropTable (const string& _tableName);
            unique_ptr<FileGDBAPI::Table> currentTblPtr;
            const string resultMCFDefPath           = "FGDB_NETXPERT_MCF_SCHEMA.XML";
            const string resultNetBuilderDefPath    = "FGDB_NETXPERT_NETBUILD_SCHEMA.XML";
            const string resultMSTDefPath           = "FGDB_NETXPERT_MST_SCHEMA.XML";
            const string resultIsoLinesDefPath      = "FGDB_NETXPERT_ISOLINES_SCHEMA.XML";
            const string resultSPTDefPath           = "FGDB_NETXPERT_SPT_SCHEMA.XML";
            Config NETXPERT_CNFG;
    };
}


#endif // FGDBWRITER_H

