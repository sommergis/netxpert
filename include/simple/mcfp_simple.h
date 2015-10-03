#ifndef MCFP_SIMPLE_H
#define MCFP_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "mcflow.h"

namespace netxpert {
 namespace simple {

    class MinCostFlow
    {
        public:
            MinCostFlow(std::string jsonCnfg);
            int Solve();
            double GetOptimum();
            std::string GetMinimumCostFlowAsJSON();
            std::vector<netxpert::FlowCost> GetMinimumCostFlow();
        private:
            Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::MinCostFlow> solver = nullptr;
    };
 }
}
#endif // MCFP_SIMPLE_H
