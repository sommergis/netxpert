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

#ifndef TRANSP_SIMPLE_H
#define TRANSP_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "transportation.h"

namespace netxpert {
 namespace simple {

   /**
    * \brief Simple Interface of the Transportation Solver
    *
    * \li Initialization with a JSON Config in Constructor
    * \li call of Solve() method (saves the results to the ResultDB given in the config JSON)
    * \li optional: GetOptimum() returns the overall optimum
    * \li optional: GetDistributionAsJSON() returns the optimized distribution as JSON string
    **/
    class Transportation
    {
        public:
            ///\brief Constructor
            Transportation(std::string jsonCnfg);
            ///\brief virtual empty Destructor
            virtual ~Transportation() {}
            /**
            * \brief Computes the transportation problem and saves the result.
            * \returns 0 if successful, 1 if unsuccessful
            */
            int Solve();
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Gets the computed distribution of the Transportation Solver as JSON string.
            std::string GetDistributionAsJSON();
            ///\brief Gets the computed distribution of the Transportation Solver as a vector of ExtDistributionArcs.
            std::vector<netxpert::data::ExtDistributionArc> GetDistribution();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::Transportation> solver = nullptr;
    };
  }
}
#endif // TRANSP_SIMPLE_H
