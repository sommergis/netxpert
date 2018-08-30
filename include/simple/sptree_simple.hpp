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

#ifndef SHORTESTPATHTREE_SIMPLE_H
#define SHORTESTPATHTREE_SIMPLE_H

#include "data.hpp"
#include "utils.hpp"
#include "lemon-net.hpp"
#include "dbhelper.hpp"
#include "sptree.hpp"

namespace netxpert {
 namespace simple {

    /**
    * \brief Simple Interface of the ShortestPathTree Solver
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON)
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetShortestPathsAsJSON() returns the shortest path(s) as JSON string
    */
    class ShortestPathTree
    {
        public:
            ///\brief Constructor
            ShortestPathTree(std::string jsonCnfg);
            ///\brief Virtual Empty destructor
            ~ShortestPathTree() {}
            /**
            * \brief Computes the shortest path tree on the given network and saves the result.
            * \returns 0 if successful, 1 if unsuccessful
            */
            int Solve();
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Gets the shortest paths of the spt solver as JSON string
            std::string GetShortestPathsAsJSON();
            ///\brief Gets the shortest paths of the spt solver with original arcs
            ///\todo implement me; problem: resolve internal ODPair to external arcIDs
            std::vector<netxpert::data::extarcid_t> GetShortestPaths();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::ShortestPathTree> solver = nullptr;
    };

  }
}

#endif // SHORTESTPATHTREE_SIMPLE_H
