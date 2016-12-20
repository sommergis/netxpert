#ifndef ISOLVER_H
#define ISOLVER_H

#include <string>
#include "network.h"

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
            virtual void Solve(netxpert::Network& net) = 0;
            virtual double GetOptimum() const=0;
    };
}
#endif // ISOLVER_H
