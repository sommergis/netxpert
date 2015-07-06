#ifndef SPT_LEM_H
#define SPT_LEM_H

#include <lemon/smart_graph.h>
#include <lemon/dijkstra.h>
//#include "..\..\lemon-1.3\lemon\smart_graph.h"
//#include "..\..\lemon-1.3\lemon\dijkstra.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>

//Heap
#include <lemon/quad_heap.h>
//#include <lemon/dheap.h>
#include <lemon/fib_heap.h>
#include "isptree.h"

using namespace lemon;

namespace netxpert {

    template <typename T>
    class Inf {
    public:
        Inf() {}
        operator T() { return( std::numeric_limits<T>::max() ); }
    };

    //typedef FibHeap<SmartDigraph::ArcMap<double>, SmartDigraph::NodeMap<double>> FibonacciHeap;
    typedef Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>> DijkstraInternal;

    class SPT_LEM_2Heap : public ISPTree
    {
        public:

            SPT_LEM_2Heap(unsigned int nmx = 0 , unsigned int mmx = 0 , bool Drctd = true);
            /* MCFClass SPTree Interface */
            void ShortestPathTree();
            void LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
                  double *pU , double *pC , double *pDfct ,
                  unsigned int *pSn , unsigned int *pEn );
            unsigned int MCFnmax( void );
            unsigned int MCFmmax( void );
            void SetOrigin( unsigned int NewOrg );
            void SetDest( unsigned int NewDst );
            bool Reached( unsigned int NodeID );
            unsigned int* ArcPredecessors( void );
            unsigned int* Predecessors( void );
            void GetArcPredecessors ( unsigned int *outArcPrd );
            void GetPredecessors( unsigned int *outPrd );
            void GetPath ( unsigned int Dst, unsigned int *outSn, unsigned int *outEn );
            /* end of MCFClass SPTree Interface */

            void PrintResult();
            ~SPT_LEM_2Heap();

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs

        private:
            bool isDrctd;
            bool allDests;
            SmartDigraph g;

            //Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>* dijk;
            //Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>, DijkstraDefaultTraits<SmartDigraph, SmartDigraph::ArcMap<double>>>* dijk;
            DijkstraInternal* dijk;
            SmartDigraph::NodeMap<double>* distMap;
            SmartDigraph::ArcMap<double>* length;
            SmartDigraph::Node orig;
            SmartDigraph::Node dest;
            std::vector<typename SmartDigraph::Node> nodes;
            unsigned int* predecessors;
            unsigned int* arcPredecessors;
    };
}
#endif // SPT_LEM_H
