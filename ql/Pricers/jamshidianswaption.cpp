/*
 Copyright (C) 2001, 2002 Sadruddin Rejeb

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
/*! \file jamshidianswaption.cpp
    \brief Swaption pricer using Jamshidian's decomposition

    \fullpath
    ql/Pricers/%jamshidianswaption.cpp
*/

// $Id$

#include "ql/Pricers/jamshidianswaption.hpp"
#include "ql/Solvers1D/brent.hpp"

namespace QuantLib {

    namespace Pricers {

        using namespace ShortRateModels;
        using Instruments::SwaptionParameters;

        class JamshidianSwaption::rStarFinder : public ObjectiveFunction {
          public:
            rStarFinder(const SwaptionParameters &params,
                        const Handle<OneFactorAffineModel>& model,
                        const std::vector<double>& amounts)
            : strike_(params.nominal), maturity_(params.exerciseTimes[0]),
              times_(params.fixedPayTimes), amounts_(amounts), model_(model) {
            }

            double operator()(double x) const {
                double value = strike_;
                Size size = times_.size();
                for (Size i=0; i<size; i++) {
                    double dbValue =
                        model_->discountBond(maturity_, times_[i], x);
                    value -= amounts_[i]*dbValue;
                }
                return value;
            }
          private:
            double strike_;
            Time maturity_;
            const std::vector<Time>& times_;
            const std::vector<double>& amounts_;
            const Handle<OneFactorAffineModel>& model_;
        };

        void JamshidianSwaption::calculate() const {
            QL_REQUIRE(
                parameters_.exerciseType == Exercise::European,
                "Cannot use the Jamshidian decomposition on exotic swaptions");
            Time maturity = parameters_.exerciseTimes[0];
            QL_REQUIRE(maturity==parameters_.floatingResetTimes[0],
                "Maturity must be equal to first reset date");


            std::vector<double> amounts(parameters_.fixedCoupons);
            amounts.back() += parameters_.nominal;

            rStarFinder finder(parameters_, model_, amounts);
            Solvers1D::Brent s1d = Solvers1D::Brent();
            double minStrike = -10.0;
            double maxStrike = 10.0;
            s1d.setMaxEvaluations(10000);
            s1d.setLowerBound(minStrike);
            s1d.setUpperBound(maxStrike);
            double rStar = s1d.solve(finder, 1e-8, 0.05, minStrike, maxStrike);

            Option::Type type = parameters_.payFixed?Option::Put:Option::Call;
            Size size = parameters_.fixedCoupons.size();
            double value = 0.0;
            for (Size i=0; i<size; i++) {
                double strike = model_->discountBond(maturity,
                    parameters_.fixedPayTimes[i], rStar);
                double dboValue = model_->discountBondOption(
                    type, strike, maturity, parameters_.fixedPayTimes[i]);
                value += amounts[i]*dboValue;
            }
            results_.value = value;
        }
    }

}
