#ifndef ISOLVER_H
#define ISOLVER_H

#include <string>
#include "network.h"
#include "lemon-net.h"

namespace netxpert {
    /**
    * \Abstract Abstract Class (Interface) for all Solvers
    **/
    class ISolver
    {
        public:
            /** Default destructor */
            virtual ~ISolver() {}
            virtual void Solve(std::string net) = 0;
            //virtual void Solve(netxpert::Network& net) = 0;
            virtual void Solve(netxpert::InternalNet& net)=0;
            virtual const double GetOptimum() const=0;
    };
}
#endif // ISOLVER_H
