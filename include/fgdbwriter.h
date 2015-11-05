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
            virtual void CreateSolverResultTable(const std::string&_tableName);
            virtual void CreateSolverResultTable(const std::string& _tableName, const bool dropFirst);
            virtual void OpenNewTransaction();
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const double capacity, const double flow, const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            virtual void CloseConnection();
        private:
            unique_ptr<FileGDBAPI::Geodatabase> geodatabasePtr;
            void connect();
            bool isConnected = false;
            void createTable( const string& _tableName);
            void openTable( const string& _tableName);
            void dropTable ( const string& _tableName);
            unique_ptr<FileGDBAPI::Table> currentTblPtr;
            const string resultTblDefPath = "FGDB_NETXPERT_RESULT_SCHEMA.XML";
            Config NETXPERT_CNFG;
    };
}


#endif // FGDBWRITER_H

