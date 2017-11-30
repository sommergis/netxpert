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

#ifndef MCFP_SIMPLE_H
#define MCFP_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "mcflow.h"

namespace netxpert {
 namespace simple {
    /**
    * \brief Simple Interface of the MinCostFlow Solver
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON)
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetMinimumCostFlowAsJSON() returns the minimum cost flow as JSON string
    **/
    class MinCostFlow
    {
        public:
            MinCostFlow(std::string jsonCnfg);
            virtual ~MinCostFlow() {}
            int Solve();
            const double GetOptimum() const;
            std::string GetMinimumCostFlowAsJSON();
            std::vector<netxpert::data::FlowCost> GetMinimumCostFlow();

        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::MinCostFlow> solver = nullptr;
    };
 }
}
#endif // MCFP_SIMPLE_H
