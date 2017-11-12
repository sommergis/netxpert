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
  /**
  * \brief Simple variants of all netXpert solvers
  **/
 namespace simple {

    class MinimumSpanningTree
    {
        public:
            MinimumSpanningTree(std::string jsonCnfg);
            virtual ~MinimumSpanningTree() {}
            int Solve();
            double GetOptimum();
            std::string GetMinimumSpanningTreeAsJSON();
            std::vector<netxpert::data::ExternalArc> GetMinimumSpanningTree();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::MinimumSpanningTree> solver = nullptr;
    };
  }
}

#endif // MSTREE_SIMPLE_H
