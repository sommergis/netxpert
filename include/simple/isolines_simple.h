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

#ifndef ISOLINES_H
#define ISOLINES_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "sptree.h"

namespace netxpert {
  namespace simple {
    /**
    * \brief Simple Interface of the Isolines Solver
    * \warning EXPERIMENTAL!
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON
    * \li call SetCutOffs() to set the distance cut off values
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetShortestPathsAsJSON() returns the shortest paths as JSON string
    **/
    class Isolines
    {
        public:
            Isolines(std::string jsonCnfg);
            virtual ~Isolines() {}
            int Solve();
            void SetCutOffs(std::string cutOffsAsJSON);
            void SetCutOffs(std::unordered_map<netxpert::data::ExtNodeID, double> cutOffs);
            const double GetOptimum() const;
            std::string GetShortestPathsAsJSON();
            std::vector<netxpert::data::ExtSPTreeArc> GetShortestPaths();

        private:
            double optimum;
            std::vector<netxpert::data::ExtSPTreeArc> totalSPTs;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::ShortestPathTree> solver = nullptr;
    };
  }
}
#endif // ISOLINES_H
