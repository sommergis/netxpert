#ifndef ISOLINES_H
#define ISOLINES_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "sptree.h"

namespace netxpert {
 namespace simple {

    class Isolines
    {
        public:
            Isolines(std::string jsonCnfg);
            virtual ~Isolines() {}
            int Solve();
            void SetCutOffs(std::string cutOffsAsJSON);
            void SetCutOffs(std::unordered_map<netxpert::data::ExtNodeID, double> cutOffs);
            double GetOptimum();
            std::string GetShortestPathsAsJSON();
            std::vector<netxpert::data::ExtSPTreeArc> GetShortestPaths();

        private:
            double optimum;
            std::vector<netxpert::data::ExtSPTreeArc> totalSPTs;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::ShortestPathTree> solver = nullptr;
    };
  }
}
#endif // ISOLINES_H
