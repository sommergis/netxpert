/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef ISOLVER_H
#define ISOLVER_H

#include <string>
#include "lemon-net.h"

namespace netxpert {
    /**
    * \brief Abstract Class (Interface) for all Solvers
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
