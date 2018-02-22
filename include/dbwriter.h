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

    namespace io {
    /**
    * \brief Pure virtual (i.e. abstract) class for the output DB
    **/
    class DBWriter
    {
        public:
            ///\brief Virtual empty Destructor
            virtual ~DBWriter(){}
            ///\brief Commits the current transaction
            virtual void CommitCurrentTransaction() = 0;
            ///\brief Creates the netxpert result database
            virtual void CreateNetXpertDB() = 0;
            ///\brief Creates the solver result table, but drops the table first if it exists
            virtual void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType,
                                                 bool dropFirst = false) = 0;
            ///\brief Opens a new transaction
            virtual void OpenNewTransaction() = 0;
            ///\brief Closes database connection
            virtual void CloseConnection() = 0;
    };
} //namespace io
} //namespace netxpert


#endif // DBWRITER_H
