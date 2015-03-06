#ifndef FGDBWRITER_H
#define FGDBWRITER_H

#include <string>
#include "dbwriter.h"
#include "logger.h"
#include <fstream>
#include <FileGDB_API/include/FileGDBAPI.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace geos;
using namespace FileGDBAPI;

namespace NetXpert {

    /**
    * \Class that writes the result of NetXpert into a ESRI FileGeodatabase
    **/
    class FGDBWriter : public DBWriter
    {
        public:
            FGDBWriter( Config& netxpertConfig );
            virtual ~FGDBWriter();
            virtual void CommitCurrentTransaction();
            virtual void CreateNetXpertDB();
            virtual void CreateSolverResultTable(const string _tableName);
            virtual void CreateSolverResultTable(string _tableName, bool dropFirst);
            virtual void OpenNewTransaction();
            void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString& route, string _tableName,
                                    bool truncateBeforeInsert);
            virtual void CloseConnection();
        private:
            Geodatabase* geodatabasePtr;
            void connect();
            void createTable( string _tableName);
            void openTable( string _tableName);
            void dropTable ( string _tableName);
            Table* currentTblPtr;
            const string resultTblDefPath = "FGDB_NETXPERT_RESULT_SCHEMA.XML";
            Config NETXPERT_CNFG;
    };
}


#endif // FGDBWRITER_H

