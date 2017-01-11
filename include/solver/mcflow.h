#ifndef MINCOSTFLOW_H
#define MINCOSTFLOW_H

#include "isolver.h"
#include "imcflow.h"
#include "MCFSimplex.h"
#include "netsimplexlem.h"

//using namespace std;

namespace netxpert {
    /**
    * \Class Solver for the Minimum Cost Flow Problem
    */
    class MinCostFlow : public netxpert::ISolver
    {
        public:
            MinCostFlow(netxpert::cnfg::Config& cnfg);
            virtual ~MinCostFlow() {}
            void Solve(std::string net);
            void Solve(netxpert::Network& net);
            bool IsDirected;
            std::vector<netxpert::data::FlowCost> GetMinCostFlow() const;
            netxpert::cnfg::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MCFAlgorithm mcfAlgorithm);
            netxpert::data::MCFSolverStatus GetSolverStatus() const;
            double GetOptimum() const;
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

        protected:
            //visible also to derived classes
            netxpert::cnfg::Config NETXPERT_CNFG;
            double optimum = 0;
            netxpert::data::MCFSolverStatus solverStatus;
            netxpert::cnfg::MCFAlgorithm algorithm;
            std::vector<netxpert::data::FlowCost> flowCost;
            std::shared_ptr<netxpert::core::IMinCostFlow> mcf;
            void solve (netxpert::Network& net);
            bool validateNetworkData(netxpert::Network& net);
            void convertInternalNetworkToSolverData(netxpert::Network& net, std::vector<unsigned int>& sNds,
                                                    std::vector<unsigned int>& eNds, std::vector<double>& supply,
                                                    std::vector<double>& caps, std::vector<double>& costs);

        private:
            //private is only visible to MCF instance - not to derived classes (like TP)

            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak int this case even without delete in the deconstructor
            netxpert::Network* net;
    };
}
#endif // MINCOSTFLOW_H
