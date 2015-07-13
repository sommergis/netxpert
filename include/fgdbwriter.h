#ifndef FGDBWRITER_H
#define FGDBWRITER_H

#include <string>
#include "dbwriter.h"
#include "logger.h"
#include <fstream>
#include <FileGDB_API/include/FileGDBAPI.h>
#include "utils.h"
#include <boost/algorithm/string.hpp>

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
            virtual void CreateSolverResultTable(const string& _tableName);
            virtual void CreateSolverResultTable(const string& _tableName, bool dropFirst);
            virtual void OpenNewTransaction();
            void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    const geos::geom::MultiLineString& route, string _tableName,
                                    bool truncateBeforeInsert);
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

