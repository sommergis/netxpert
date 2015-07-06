#ifndef CORE_MSTLEM_H
#define CORE_MSTLEM_H

#include <lemon/smart_graph.h>
#include <lemon/kruskal.h>
#include <lemon/maps.h>
#include <stdio.h>
#include <limits.h> //UNIT_MAX
#include <string.h> //memcpy
//#include <lemon\min_cost_arborescence.h>
#include "imstree.h"

using namespace lemon;
using namespace std;

namespace netxpert {
    /**
      Core Solver for the Minimum Spanning Tree Problem
    */
    class MST_LEMON : public IMinSpanTree
    {
        public:
            MST_LEMON();
            void SolveMST();

            void LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
                    double* pU , double* pC , double* pDfct ,
                    unsigned int* pSn , unsigned int* pEn );
            unsigned int MCFnmax( void );
            unsigned int MCFmmax( void );
            void GetMST (unsigned int* outStartNodes, unsigned int* outEndNodes);
            double MSTGetF0(void);
            //void GetMST (unsigned int* outStartNodes, unsigned int[] outEndNodes);
            virtual ~MST_LEMON(void);

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs
            SmartDigraph g; //undirected
            double totalCost;
            //Kruskal<SmartDigraph,SmartDigraph::ArcMap<double>>* mst;
            SmartDigraph::ArcMap<double>* costMapPtr;
            SmartDigraph::ArcMap<bool>* edgeBoolMapPtr;
            std::vector<typename SmartDigraph::Node> nodes;
    };
}
#endif // CORE_MSTLEM_H
