#ifndef IMINCOSTFLOW_H
#define IMINCOSTFLOW_H

namespace netxpert {
    /**
    * \Class Abstract Class (Interface) for all Minimum Cost Flow Problem Solvers in netxpert core.
    */
    class IMinCostFlow
    {
        public:
            virtual ~IMinCostFlow(){}
            /* start of MCFClass MCFsimplex Interface */
            virtual void MCFArcs(unsigned int* outStartNodes, unsigned int* outEndNodes)=0;
            virtual void LoadNet(unsigned int nmx, unsigned int mmx, unsigned int pn, unsigned int pm, double* pU,
                            double* pC, double* pDfct, unsigned int* pSn, unsigned int* pEn)=0;
            virtual unsigned int MCFmmax()=0;
            virtual unsigned int MCFnmax()=0;
            virtual void SolveMCF()=0;
            virtual double MCFGetFO()=0;
            virtual int MCFGetStatus()=0;
            virtual void MCFCosts(double* outCosts)=0;
            virtual void MCFGetX(double* outFlow)=0;
            virtual void CheckPSol()=0;
            virtual void CheckDSol()=0;
            /* end of MCFClass MCFsimplex Interface */
    };
}
#endif // IMINCOSTFLOW_H
