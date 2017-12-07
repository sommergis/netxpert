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

#ifndef ODMATRIX_SIMPLE_H
#define ODMATRIX_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "odmatrix.h"

namespace netxpert {
 namespace simple {
    /**
    * \brief Simple Interface of the OriginDestinationMatrix Solver
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON)
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetODMatrixAsJSON() returns the origin destination matrix as JSON string
    **/
    class OriginDestinationMatrix
    {
        public:
            OriginDestinationMatrix(std::string jsonCnfg);
            ~OriginDestinationMatrix() {}
            int Solve();
            int Solve(bool parallel);
            double GetOptimum();
            std::string GetODMatrixAsJSON();
            std::vector<netxpert::data::ExtSPTreeArc> GetODMatrix();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::OriginDestinationMatrix> solver = nullptr;
    };
  }
}
#endif // ODMATRIX_SIMPLE_H
