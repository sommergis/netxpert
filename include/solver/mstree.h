#ifndef MINSPANTREE_H
#define MINSPANTREE_H

#include "isolver.h"
#include "imstree.h"
#include "mstlem.h"

using namespace std;

namespace netxpert {
    /**
    * \Class Solver for the Minimum Spanning Tree Problem
    */
    class MinimumSpanningTree : public ISolver
    {
        public:
            /** Default constructor */
            MinimumSpanningTree(Config& cnfg);

            /** Default destructor */
            virtual ~MinimumSpanningTree();

            void Solve(string net);
            void Solve(Network& net);

            MSTAlgorithm GetAlgorithm();
            void SetAlgorithm(MSTAlgorithm mstAlgorithm);

            double GetOptimum();
            vector<InternalArc> GetMinimumSpanningTree() const;

        private:
            vector<InternalArc> minimumSpanTree;
            MSTAlgorithm algorithm;
            shared_ptr<IMinSpanTree> mst;
            vector<InternalArc> solve (Network& net);
            bool validateNetworkData(Network& net);
            void convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
                                                    vector<unsigned int>& eNds, vector<double>& supply,
                                                    vector<double>& caps, vector<double>& costs);
    };
}
#endif // MINSPANTREE_H
