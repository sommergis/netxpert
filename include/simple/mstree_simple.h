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

#ifndef MSTREE_SIMPLE_H
#define MSTREE_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "mstree.h"

namespace netxpert {
 namespace simple {
    /**
    * \brief Simple Interface of the MinimumSpanningTree Solver
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON)
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetMinimumSpanningTreeAsJSON() returns the minimum spanning tree as JSON string
    **/
    class MinimumSpanningTree
    {
        public:
          ///\brief Constructor
          MinimumSpanningTree(std::string jsonCnfg);
          ///\brief virtual empty Destructor
          virtual ~MinimumSpanningTree() {}
          /**
          * \brief Computes the minimum spanning tree problem and saves the results.
          * \returns 0 if successful, 1 if unsuccessful
          */
          int Solve();
          ///\brief Gets the overall optimum of the solver
          const double GetOptimum() const;
          ///\brief Gets the computed minimum spanning tree as JSON string.
          ///\todo implement me
          std::string GetMinimumSpanningTreeAsJSON();
          ///\brief Gets the computed minimum spanning tree as a vector of original arcs.
          ///\todo implement me
          std::vector<netxpert::data::ExternalArc> GetMinimumSpanningTree();
        private:
          netxpert::cnfg::Config NETXPERT_CNFG;
          std::unique_ptr<netxpert::MinimumSpanningTree> solver = nullptr;
    };
  }
}

#endif // MSTREE_SIMPLE_H
