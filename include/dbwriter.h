/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef DBWRITER_H
#define DBWRITER_H

#include <string>
#include "data.h"
#include "geos/geom/MultiLineString.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/CoordinateSequence.h"

namespace netxpert {
    /**
    * \brief Input/Output of netXpert
    **/
    namespace io {
    /**
    * \brief Abstract Class (Interface) for the output DB
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
            virtual void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType) = 0;
            virtual void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType,
                                                 const bool dropFirst) = 0;
            virtual void OpenNewTransaction() = 0;
            /*virtual void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString& route, string _tableName,
                                    bool truncateBeforeInsert) = 0;*/
            /*virtual void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString route, string _tableName,
                                    bool truncateBeforeInsert, SQLiteCommand cmd) = 0;*/
            virtual void CloseConnection() = 0;
    };
} //namespace io
} //namespace netxpert


#endif // DBWRITER_H
