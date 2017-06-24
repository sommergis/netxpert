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
