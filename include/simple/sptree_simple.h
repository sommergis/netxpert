#ifndef SHORTESTPATHTREE_SIMPLE_H
#define SHORTESTPATHTREE_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "sptree.h"

namespace netxpert {
 namespace simple {

    class ShortestPathTree
    {
        public:
            ShortestPathTree(std::string jsonCnfg);
            virtual ~ShortestPathTree() {}
            int Solve();
            double GetOptimum();
            std::string GetShortestPathsAsJSON();
            std::vector<ExtSPTreeArc> GetShortestPaths();
        private:
            Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::ShortestPathTree> solver = nullptr;
    };

  }
}

#endif // SHORTESTPATHTREE_SIMPLE_H
