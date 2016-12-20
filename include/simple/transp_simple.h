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
