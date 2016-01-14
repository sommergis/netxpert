#ifndef MSTREE_SIMPLE_H
#define MSTREE_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "mstree.h"

namespace netxpert {
 namespace simple {

    class MinimumSpanningTree
    {
        public:
            MinimumSpanningTree(std::string jsonCnfg);
            virtual ~MinimumSpanningTree() {}
            int Solve();
            double GetOptimum();
            std::string GetMinimumSpanningTreeAsJSON();
            std::vector<netxpert::ExternalArc> GetMinimumSpanningTree();
        private:
            Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::MinimumSpanningTree> solver = nullptr;
    };
  }
}

#endif // MSTREE_SIMPLE_H
