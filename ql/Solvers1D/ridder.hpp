
/*
 Copyright (C) 2000, 2001, 2002 RiskMap srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it under the
 terms of the QuantLib license.  You should have received a copy of the
 license along with this program; if not, please email ferdinando@ametrano.net
 The license is also available online at http://quantlib.org/html/license.html

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file ridder.hpp
    \brief Ridder 1-D solver

    \fullpath
    ql/Solvers1D/%ridder.hpp
*/

// $Id$

#ifndef quantlib_solver1d_ridder_h
#define quantlib_solver1d_ridder_h

#include <ql/solver1d.hpp>

namespace QuantLib {

    namespace Solvers1D {

        //! %Ridder 1-D solver
        class Ridder : public Solver1D {
          private:
            double solve_(const ObjectiveFunction& f, double xAccuracy) const;
        };

    }

}

#endif
