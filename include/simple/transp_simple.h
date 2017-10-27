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

    class Transportation
    {
        public:
            Transportation(std::string jsonCnfg);
            virtual ~Transportation() {}
            int Solve();
            double GetOptimum();
            std::string GetDistributionAsJSON();
            std::vector<netxpert::data::ExtDistributionArc> GetDistribution();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::Transportation> solver = nullptr;
    };
  }
}
#endif // TRANSP_SIMPLE_H
