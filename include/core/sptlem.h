// *** ADDED BY HEADER FIXUP ***
#include <string>
// *** END ***
#ifndef SPT_LEM_H
#define SPT_LEM_H

#include "lemon/smart_graph.h"
#include "lemon/static_graph.h"
#include "lemon/dijkstra.h"
#include "bijkstra.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>

//Heap
#include "lemon/quad_heap.h"
//#include "lemon/dheap.h"
#include "lemon/fib_heap.h"
#include "isptree.h"

using namespace lemon;

namespace netxpert {
    namespace core {

    template <typename T>
    class Inf {
    public:
        Inf() {}
        operator T() { return( std::numeric_limits<T>::max() ); }
    };

    //typedef FibHeap<SmartDigraph::ArcMap<double>, SmartDigraph::NodeMap<double>> FibonacciHeap;
    //using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;

    /**
    *  \Class Core Solver for the Shortest Path Tree Problem with binary Heap structure and
    *   Dijkstra's algorithm of LEMON.
    */
    class SPT_LEM_2Heap : public netxpert::core::ISPTree
    {
        public:
            SPT_LEM_2Heap(unsigned int nmx = 0 , unsigned int mmx = 0 , bool Drctd = true);
            SPT_LEM_2Heap (SPT_LEM_2Heap const &) {} //copy constrcutor
            virtual ~SPT_LEM_2Heap(); //deconstructor
            //void ShortestPathTree(double maxCutOff); //for isolines

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

            ISPTreePtr create () const        // Virtual constructor (creation)
            {
                return ISPTreePtr(new SPT_LEM_2Heap() );
            }
            ISPTreePtr clone () const        // Virtual constructor (copying)
            {
                return ISPTreePtr(new SPT_LEM_2Heap (*this));
            }
            /* end of MCFClass SPTree Interface */
            void GetPath ( unsigned int Dst, unsigned int *outSn, unsigned int *outEn );
            void PrintResult();

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs

        private:
			//using DijkstraInternal = Dijkstra<StaticDigraph, StaticDigraph::ArcMap<double>>;
			using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;
            bool isDrctd;
            bool allDests;

            DijkstraInternal* dijk;
            //smart graph
            SmartDigraph g;
            SmartDigraph::NodeMap<double>* distMap;
            SmartDigraph::ArcMap<double>* length;
            SmartDigraph::Node orig;
            SmartDigraph::Node dest;
            std::vector<typename SmartDigraph::Node> nodes;
            //Static graph
			/*
            StaticDigraph g;
            StaticDigraph::NodeMap<double>* distMap;
            StaticDigraph::ArcMap<double>* length;
            StaticDigraph::Node orig;
            StaticDigraph::Node dest;
            std::vector<StaticDigraph::Node> nodes;*/
            //->
            unsigned int* predecessors;
            unsigned int* arcPredecessors;
    };
} //namespace core
} //namespace netxpert

#endif // SPT_LEM_H
