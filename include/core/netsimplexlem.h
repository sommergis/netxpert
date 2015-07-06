#ifndef NETSIMPLEXLEM_H
#define NETSIMPLEXLEM_H

#include <lemon/smart_graph.h>
#include <lemon/network_simplex.h>
#include <stdio.h>
#include <limits.h>
#include <string.h> //memcpy

namespace netxpert {

    class NetworkSimplex
    {
        public:
            NetworkSimplex(void);

            /* MCFClass Interface */
            void SolveMCF();
            void LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
                    double *pU , double *pC , double *pDfct ,
                    unsigned int *pSn , unsigned int *pEn );
            unsigned int MCFnmax( void );
            unsigned int MCFmmax( void );
            double MCFGetFO( void );
            void MCFArcs(unsigned int* startNodes, unsigned int* endNodes);
            void MCFCosts(double* costs);
            void MCFGetX( double* flow );
            int MCFGetStatus();
            void CheckPSol();
            void CheckDSol();
            /* end of MCFClass Interface */

            void PrintResult();
            ~NetworkSimplex(void);

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs

        private:
            lemon::SmartDigraph g;
            // Graphtyp, flow typ, cost typ
            lemon::NetworkSimplex<lemon::SmartDigraph, int, double>* nsimplexPtr;
            lemon::SmartDigraph::ArcMap<double>* costsPtr;
            lemon::SmartDigraph::ArcMap<double>* capacityPtr;
            lemon::SmartDigraph::NodeMap<double>* supplyPtr;
            lemon::SmartDigraph::ArcMap<double>* flowPtr;
            std::vector<typename lemon::SmartDigraph::Node> nodes;
            //double* flowX;
            lemon::NetworkSimplex<lemon::SmartDigraph,int,double>::ProblemType status;
    };
}
#endif // NETSIMPLEXLEM_H
