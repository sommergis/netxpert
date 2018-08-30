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
#include "lemon-net.hpp"

namespace netxpert {
    ///\brief Abstract Class (Interface) for all Solvers
    class ISolver
    {
        public:
            ///\brief Default virtual destructor
            virtual ~ISolver() {}
            ///\brief Solve method that takes the Network as text representation
            ///\warning Has not been implemented in the C++ Version yet.
            virtual void Solve(std::string net) = 0;
            ///\brief Solve method that takes the Network as an object of type InternalNet
            virtual void Solve(netxpert::data::InternalNet& net)=0;
            ///\brief Gets the total optimum of the solver
            virtual const double GetOptimum() const=0;
    };
}
#endif // ISOLVER_H
