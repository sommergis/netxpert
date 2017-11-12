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

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "sptree.h"

namespace netxpert {
  /**
  * \brief Simple variants of all netXpert solvers
  **/
 namespace simple {

    class ShortestPathTree
    {
        public:
            ShortestPathTree(std::string jsonCnfg);
            ~ShortestPathTree() {}
            int Solve();
            const double GetOptimum() const;
            std::string GetShortestPathsAsJSON();
            std::vector<netxpert::data::ExtSPTreeArc> GetShortestPaths();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::ShortestPathTree> solver = nullptr;
    };

  }
}

#endif // SHORTESTPATHTREE_SIMPLE_H
