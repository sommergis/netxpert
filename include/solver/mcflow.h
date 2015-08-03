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
            MinCostFlow(Config& cnfg);
            virtual ~MinCostFlow();
            void Solve(std::string net);
            void Solve(Network& net);
            bool IsDirected;
            std::vector<FlowCost> GetMinCostFlow() const;
            MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(MCFAlgorithm mcfAlgorithm);
            MCFSolverStatus GetSolverStatus() const;
            double GetOptimum() const;

        protected:
            Config NETXPERT_CNFG;
            double optimum = 0;
            MCFSolverStatus solverStatus;
            MCFAlgorithm algorithm;
            vector<FlowCost> flowCost;
            shared_ptr<IMinCostFlow> mcf;
            void solve (Network& net);
            bool validateNetworkData(Network& net);
            void convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
                                                    vector<unsigned int>& eNds, vector<double>& supply,
                                                    vector<double>& caps, vector<double>& costs);
    };
}
#endif // MINCOSTFLOW_H
