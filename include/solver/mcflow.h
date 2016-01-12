#ifndef MINCOSTFLOW_H
#define MINCOSTFLOW_H

#include "isolver.h"
#include "imcflow.h"
#include "MCFSimplex.h"
#include "netsimplexlem.h"

using namespace std;

namespace netxpert {
    /**
    * \Class Solver for the Minimum Cost Flow Problem
    */
    class MinCostFlow : public ISolver
    {
        public:
            MinCostFlow(netxpert::Config& cnfg);
            virtual ~MinCostFlow();
            void Solve(std::string net);
            void Solve(netxpert::Network& net);
            bool IsDirected;
            std::vector<netxpert::FlowCost> GetMinCostFlow() const;
            netxpert::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::MCFAlgorithm mcfAlgorithm);
            netxpert::MCFSolverStatus GetSolverStatus() const;
            double GetOptimum() const;
            void SaveResults(const std::string& resultTableName, const netxpert::ColumnMap& cmap) const;

        protected:
            //visible also to derived classes
            Config NETXPERT_CNFG;
            double optimum = 0;
            netxpert::MCFSolverStatus solverStatus;
            netxpert::MCFAlgorithm algorithm;
            std::vector<netxpert::FlowCost> flowCost;
            std::shared_ptr<netxpert::IMinCostFlow> mcf;
            void solve (netxpert::Network& net);
            bool validateNetworkData(netxpert::Network& net);
            void convertInternalNetworkToSolverData(netxpert::Network& net, std::vector<unsigned int>& sNds,
                                                    std::vector<unsigned int>& eNds, std::vector<double>& supply,
                                                    std::vector<double>& caps, std::vector<double>& costs);

        private:
            //private is only visible to MCF instance - not to derived classes (like TP)
            Network* net;
    };
}
#endif // MINCOSTFLOW_H
