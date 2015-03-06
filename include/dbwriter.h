#ifndef DBWRITER_H
#define DBWRITER_H

#include <string>
#include <geos/geom/MultiLineString.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/CoordinateSequence.h>

using namespace std;

namespace NetXpert {
    /**
    * \Abstract Class (Interface) for Result DB for NetXpert
    **/
    class DBWriter
    {
        public:
            virtual ~DBWriter(){}
            // virtual Rückgabetyp nameDerFunktion(Variable1, Variable2, VariableN) = 0;
            // mit dem "= 0" zwingt man den Programmierer die Methode zu implementieren!!!
            // Sonst gibt es Fehler beim Kompilieren...
            virtual void CommitCurrentTransaction() = 0;
            virtual void CreateNetXpertDB() = 0;
            virtual void CreateSolverResultTable(string _tableName) = 0;
            virtual void CreateSolverResultTable(string _tableName, bool dropFirst) = 0;
            virtual void OpenNewTransaction() = 0;
            /*virtual void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString& route, string _tableName,
                                    bool truncateBeforeInsert) = 0;*/
            /*virtual void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString route, string _tableName,
                                    bool truncateBeforeInsert, SQLiteCommand cmd) = 0;*/
            virtual void CloseConnection() = 0;
    };
}


#endif // DBWRITER_H
