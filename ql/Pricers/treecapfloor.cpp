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
/*! \file treecapfloor.cpp
    \brief Cap/Floor calculated using a tree

    \fullpath
    ql/Pricers/%treecapfloor.cpp
*/

// $Id$

#include "ql/Pricers/treecapfloor.hpp"
#include "ql/ShortRateModels/onefactormodel.hpp"

namespace QuantLib {

    namespace Pricers {

        using namespace Instruments;
        using namespace ShortRateModels;
        using namespace Lattices;

        TreeCapFloor::TreeCapFloor(const Handle<Model>& model, Size timeSteps) 
        : CapFloorPricer<Model>(model), timeSteps_(timeSteps) {}

        TreeCapFloor::TreeCapFloor(const Handle<Model>& model,
                                   const TimeGrid& timeGrid) 
        : CapFloorPricer<Model>(model), timeGrid_(timeGrid), timeSteps_(0) {
            tree_ = model_->tree(timeGrid);
        }

        void TreeCapFloor::update() {
            if (timeGrid_.size() > 0)
                tree_ = model_->tree(timeGrid_);
            notifyObservers();
        }

        void TreeCapFloor::calculate() const {

            QL_REQUIRE(!model_.isNull(), "TreeCapFloor: No model");
            Handle<Lattice> tree;

            if (tree_.isNull()) {
                std::list<Time> times(0);
                Size nPeriods = parameters_.startTimes.size();
                Size i;
                for (i=0; i<nPeriods; i++) {
                    times.push_back(parameters_.startTimes[i]);
                    times.push_back(parameters_.endTimes[i]);
                }
                times.sort();
                times.unique();

                TimeGrid timeGrid(times, timeSteps_);
                tree = model_->tree(timeGrid);
            } else {
                tree = tree_;
            }

            Handle<DiscretizedAsset> capfloor(
                new DiscretizedCapFloor(tree, parameters_));

            tree->initialize(capfloor, parameters_.endTimes.back());
            tree->rollback(capfloor, parameters_.startTimes.front());

            results_.value = tree->presentValue(capfloor);
        }

    }

}
