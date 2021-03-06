/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2004, 2005 StatPro Italia srl
 Copyright (C) 2007, 2012 Ferdinando Ametrano
 Copyright (C) 2007, 2009 Piter Dias

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "bonds.hpp"
#include "utilities.hpp"
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/instruments/bonds/floatingratebond.hpp>
#include <ql/instruments/bonds/zerocouponbond.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/calendars/brazil.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/business252.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <ql/time/schedule.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/cashflows/couponpricer.hpp>
#include <ql/cashflows/cashflows.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/pricingengines/bond/bondfunctions.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;
using boost::shared_ptr;

namespace {

    struct CommonVars {
        // common data
        Calendar calendar;
        Date today;
        Real faceAmount;

        // cleanup
        SavedSettings backup;

        // setup
        CommonVars() {
            calendar = TARGET();
            today = calendar.adjust(Date::todaysDate());
            Settings::instance().evaluationDate() = today;
            faceAmount = 1000000.0;
        }
    };

}


void BondTest::testYield() {

    BOOST_MESSAGE("Testing consistency of bond price/yield calculation...");

    CommonVars vars;

    Real tolerance = 1.0e-7;
    Size maxEvaluations = 100;

    Integer issueMonths[] = { -24, -18, -12, -6, 0, 6, 12, 18, 24 };
    Integer lengths[] = { 3, 5, 10, 15, 20 };
    Natural settlementDays = 3;
    Real coupons[] = { 0.02, 0.05, 0.08 };
    Frequency frequencies[] = { Semiannual, Annual };
    DayCounter bondDayCount = Thirty360();
    BusinessDayConvention accrualConvention = Unadjusted;
    BusinessDayConvention paymentConvention = ModifiedFollowing;
    Real redemption = 100.0;

    Rate yields[] = { 0.03, 0.04, 0.05, 0.06, 0.07 };
    Compounding compounding[] = { Compounded, Continuous };

    for (Size i=0; i<LENGTH(issueMonths); i++) {
      for (Size j=0; j<LENGTH(lengths); j++) {
        for (Size k=0; k<LENGTH(coupons); k++) {
          for (Size l=0; l<LENGTH(frequencies); l++) {
            for (Size n=0; n<LENGTH(compounding); n++) {

              Date dated = vars.calendar.advance(vars.today,
                                                 issueMonths[i], Months);
              Date issue = dated;
              Date maturity = vars.calendar.advance(issue,
                                                    lengths[j], Years);

              Schedule sch(dated, maturity,
                           Period(frequencies[l]), vars.calendar,
                           accrualConvention, accrualConvention,
                           DateGeneration::Backward, false);

              FixedRateBond bond(settlementDays, vars.faceAmount, sch,
                                 std::vector<Rate>(1, coupons[k]),
                                 bondDayCount, paymentConvention,
                                 redemption, issue);

              for (Size m=0; m<LENGTH(yields); m++) {

                Real price = BondFunctions::cleanPrice(bond, yields[m],
                                                 bondDayCount,
                                                 compounding[n],
                                                 frequencies[l]);
                Rate calculated = BondFunctions::yield(bond, price,
                                             bondDayCount, compounding[n],
                                             frequencies[l],
                                             Date(),
                                             tolerance, maxEvaluations);

                if (std::fabs(yields[m]-calculated) > tolerance) {
                  // the difference might not matter
                  Real price2 = BondFunctions::cleanPrice(bond, calculated,
                                                    bondDayCount,
                                                    compounding[n],
                                                    frequencies[l]);
                  if (std::fabs(price-price2)/price > tolerance) {
                      BOOST_FAIL("\nyield recalculation failed:"
                          "\n    issue:     " << issue <<
                          "\n    maturity:  " << maturity <<
                          "\n    coupon:    " << io::rate(coupons[k]) <<
                          "\n    frequency: " << frequencies[l] <<
                          "\n    yield:   " << io::rate(yields[m]) <<
                          (compounding[n] == Compounded ?
                                " compounded" : " continuous") <<
                          std::setprecision(7) <<
                          "\n    price:   " << price <<
                          "\n    yield': " << io::rate(calculated) <<
                          "\n    price': " << price2);
                  }
                }
              }
            }
          }
        }
      }
    }
}

void BondTest::testAtmRate() {

    BOOST_MESSAGE("Testing consistency of bond price/atmRate calculation...");

    CommonVars vars;

    Real tolerance = 1.0e-7;

    Integer issueMonths[] = { -24, -18, -12, -6, 0, 6, 12, 18, 24 };
    Integer lengths[] = { 3, 5, 10, 15, 20 };
    Natural settlementDays = 3;
    Real coupons[] = { 0.02, 0.05, 0.08 };
    Frequency frequencies[] = { Semiannual, Annual };
    DayCounter bondDayCount = Thirty360();
    BusinessDayConvention accrualConvention = Unadjusted;
    BusinessDayConvention paymentConvention = ModifiedFollowing;
    Real redemption = 100.0;
    Handle<YieldTermStructure> disc(flatRate(vars.today,0.03,Actual360()));
    shared_ptr<PricingEngine> bondEngine(new DiscountingBondEngine(disc));

    for (Size i=0; i<LENGTH(issueMonths); i++) {
      for (Size j=0; j<LENGTH(lengths); j++) {
        for (Size k=0; k<LENGTH(coupons); k++) {
          for (Size l=0; l<LENGTH(frequencies); l++) {
            Date dated = vars.calendar.advance(vars.today,
                                               issueMonths[i], Months);
            Date issue = dated;
            Date maturity = vars.calendar.advance(issue,
                                                  lengths[j], Years);

            Schedule sch(dated, maturity,
                         Period(frequencies[l]), vars.calendar,
                         accrualConvention, accrualConvention,
                         DateGeneration::Backward, false);

            FixedRateBond bond(settlementDays, vars.faceAmount, sch,
                             std::vector<Rate>(1, coupons[k]),
                             bondDayCount, paymentConvention,
                             redemption, issue);

            bond.setPricingEngine(bondEngine);
            Real price = bond.cleanPrice();
            Rate calculated = BondFunctions::atmRate(bond,
                                                     **disc,
                                                     bond.settlementDate(),
                                                     price);

            if (std::fabs(coupons[k]-calculated) > tolerance) {
                  BOOST_FAIL("\natm rate recalculation failed:"
                      "\n today:           " << vars.today <<
                      "\n settlement date: " << bond.settlementDate() <<
                      "\n issue:           " << issue <<
                      "\n maturity:        " << maturity <<
                      "\n coupon:          " << io::rate(coupons[k]) <<
                      "\n frequency:       " << frequencies[l] <<
                      "\n clean price:     " << price <<
                      "\n dirty price:     " << price + bond.accruedAmount() <<
                      "\n atm rate:        " << io::rate(calculated));
            }
          }
        }
      }
    }
}

void BondTest::testZspread() {

    BOOST_MESSAGE("Testing consistency of bond price/z-spread calculation...");

    CommonVars vars;

    Real tolerance = 1.0e-7;
    Size maxEvaluations = 100;

    Handle<YieldTermStructure> discountCurve(
                                       flatRate(vars.today,0.03,Actual360()));

    Integer issueMonths[] = { -24, -18, -12, -6, 0, 6, 12, 18, 24 };
    Integer lengths[] = { 3, 5, 10, 15, 20 };
    Natural settlementDays = 3;
    Real coupons[] = { 0.02, 0.05, 0.08 };
    Frequency frequencies[] = { Semiannual, Annual };
    DayCounter bondDayCount = Thirty360();
    BusinessDayConvention accrualConvention = Unadjusted;
    BusinessDayConvention paymentConvention = ModifiedFollowing;
    Real redemption = 100.0;

    Spread spreads[] = { -0.01, -0.005, 0.0, 0.005, 0.01 };
    Compounding compounding[] = { Compounded, Continuous };

    for (Size i=0; i<LENGTH(issueMonths); i++) {
      for (Size j=0; j<LENGTH(lengths); j++) {
        for (Size k=0; k<LENGTH(coupons); k++) {
          for (Size l=0; l<LENGTH(frequencies); l++) {
            for (Size n=0; n<LENGTH(compounding); n++) {

              Date dated = vars.calendar.advance(vars.today,
                                                 issueMonths[i], Months);
              Date issue = dated;
              Date maturity = vars.calendar.advance(issue,
                                                    lengths[j], Years);

              Schedule sch(dated, maturity,
                           Period(frequencies[l]), vars.calendar,
                           accrualConvention, accrualConvention,
                           DateGeneration::Backward, false);

              FixedRateBond bond(settlementDays, vars.faceAmount, sch,
                                 std::vector<Rate>(1, coupons[k]),
                                 bondDayCount, paymentConvention,
                                 redemption, issue);

              for (Size m=0; m<LENGTH(spreads); m++) {

                Real price = BondFunctions::cleanPrice(bond, *discountCurve,
                                                   spreads[m],
                                                   bondDayCount,
                                                   compounding[n],
                                                   frequencies[l]);
                Spread calculated = BondFunctions::zSpread(bond, price,
                                                           *discountCurve,
                                                           bondDayCount,
                                                           compounding[n],
                                                           frequencies[l],
                                                           Date(),
                                                           tolerance,
                                                           maxEvaluations);

                if (std::fabs(spreads[m]-calculated) > tolerance) {
                  // the difference might not matter
                  Real price2 = BondFunctions::cleanPrice(bond, *discountCurve,
                                                      calculated,
                                                      bondDayCount,
                                                      compounding[n],
                                                      frequencies[l]);
                  if (std::fabs(price-price2)/price > tolerance) {
                      BOOST_FAIL("\nZ-spread recalculation failed:"
                          "\n    issue:     " << issue <<
                          "\n    maturity:  " << maturity <<
                          "\n    coupon:    " << io::rate(coupons[k]) <<
                          "\n    frequency: " << frequencies[l] <<
                          "\n    Z-spread:  " << io::rate(spreads[m]) <<
                          (compounding[n] == Compounded ?
                                " compounded" : " continuous") <<
                          std::setprecision(7) <<
                          "\n    price:     " << price <<
                          "\n    Z-spread': " << io::rate(calculated) <<
                          "\n    price':    " << price2);
                  }
                }
              }
            }
          }
        }
      }
    }
}



void BondTest::testTheoretical() {

    BOOST_MESSAGE("Testing theoretical bond price/yield calculation...");

    CommonVars vars;

    Real tolerance = 1.0e-7;
    Size maxEvaluations = 100;

    Size lengths[] = { 3, 5, 10, 15, 20 };
    Natural settlementDays = 3;
    Real coupons[] = { 0.02, 0.05, 0.08 };
    Frequency frequencies[] = { Semiannual, Annual };
    DayCounter bondDayCount = Actual360();
    BusinessDayConvention accrualConvention = Unadjusted;
    BusinessDayConvention paymentConvention = ModifiedFollowing;
    Real redemption = 100.0;

    Rate yields[] = { 0.03, 0.04, 0.05, 0.06, 0.07 };

    for (Size j=0; j<LENGTH(lengths); j++) {
      for (Size k=0; k<LENGTH(coupons); k++) {
        for (Size l=0; l<LENGTH(frequencies); l++) {

            Date dated = vars.today;
            Date issue = dated;
            Date maturity = vars.calendar.advance(issue, lengths[j], Years);

            shared_ptr<SimpleQuote> rate(new SimpleQuote(0.0));
            Handle<YieldTermStructure> discountCurve(flatRate(vars.today,
                                                              rate,
                                                              bondDayCount));

            Schedule sch(dated, maturity,
                         Period(frequencies[l]), vars.calendar,
                         accrualConvention, accrualConvention,
                         DateGeneration::Backward, false);

            FixedRateBond bond(settlementDays, vars.faceAmount, sch,
                               std::vector<Rate>(1, coupons[k]),
                               bondDayCount, paymentConvention,
                               redemption, issue);

            shared_ptr<PricingEngine> bondEngine(
                                    new DiscountingBondEngine(discountCurve));
            bond.setPricingEngine(bondEngine);

            for (Size m=0; m<LENGTH(yields); m++) {

                rate->setValue(yields[m]);

                Real price = BondFunctions::cleanPrice(bond, yields[m],
                                                 bondDayCount, Continuous,
                                                 frequencies[l]);
                Real calculatedPrice = bond.cleanPrice();

                if (std::fabs(price-calculatedPrice) > tolerance) {
                    BOOST_FAIL("price calculation failed:" <<
                        "\n    issue:     " << issue <<
                        "\n    maturity:  " << maturity <<
                        "\n    coupon:    " << io::rate(coupons[k]) <<
                        "\n    frequency: " << frequencies[l] <<
                        "\n    yield:  " << io::rate(yields[m]) <<
                        std::setprecision(7) <<
                        "\n    expected:    " << price <<
                        "\n    calculated': " << calculatedPrice <<
                        "\n    error':      " << price-calculatedPrice);
                }

                Rate calculatedYield = BondFunctions::yield(bond, calculatedPrice,
                    bondDayCount, Continuous, frequencies[l],
                    bond.settlementDate(),
                    tolerance, maxEvaluations);
                if (std::fabs(yields[m]-calculatedYield) > tolerance) {
                    BOOST_FAIL("yield calculation failed:" <<
                        "\n    issue:     " << issue <<
                        "\n    maturity:  " << maturity <<
                        "\n    coupon:    " << io::rate(coupons[k]) <<
                        "\n    frequency: " << frequencies[l] <<
                        "\n    yield:  " << io::rate(yields[m]) <<
                        std::setprecision(7) <<
                        "\n    price:  " << price <<
                        "\n    yield': " << io::rate(calculatedYield));
                }
            }
        }
      }
    }
}


void BondTest::testCached() {

    BOOST_MESSAGE(
        "Testing bond price/yield calculation against cached values...");

    CommonVars vars;

    // with implicit settlement calculation:

    Date today(22, November, 2004);
    Settings::instance().evaluationDate() = today;

    Calendar bondCalendar = NullCalendar();
    DayCounter bondDayCount = ActualActual(ActualActual::ISMA);
    Natural settlementDays = 1;

    Handle<YieldTermStructure> discountCurve(flatRate(today,0.03,Actual360()));

    // actual market values from the evaluation date

    Frequency freq = Semiannual;
    Schedule sch1(Date(31, October, 2004),
                  Date(31, October, 2006), Period(freq), bondCalendar,
                  Unadjusted, Unadjusted, DateGeneration::Backward, false);

    FixedRateBond bond1(settlementDays, vars.faceAmount, sch1,
                        std::vector<Rate>(1, 0.025),
                        bondDayCount, ModifiedFollowing,
                        100.0, Date(1, November, 2004));

    shared_ptr<PricingEngine> bondEngine(
                                    new DiscountingBondEngine(discountCurve));
    bond1.setPricingEngine(bondEngine);

    Real marketPrice1 = 99.203125;
    Rate marketYield1 = 0.02925;

    Schedule sch2(Date(15, November, 2004),
                  Date(15, November, 2009), Period(freq), bondCalendar,
                  Unadjusted, Unadjusted, DateGeneration::Backward, false);

    FixedRateBond bond2(settlementDays, vars.faceAmount, sch2,
                        std::vector<Rate>(1, 0.035),
                        bondDayCount, ModifiedFollowing,
                        100.0, Date(15, November, 2004));

    bond2.setPricingEngine(bondEngine);

    Real marketPrice2 = 99.6875;
    Rate marketYield2 = 0.03569;

    // calculated values

    Real cachedPrice1a = 99.204505, cachedPrice2a = 99.687192;
    Real cachedPrice1b = 98.943393, cachedPrice2b = 101.986794;
    Rate cachedYield1a = 0.029257,  cachedYield2a = 0.035689;
    Rate cachedYield1b = 0.029045,  cachedYield2b = 0.035375;
    Rate cachedYield1c = 0.030423,  cachedYield2c = 0.030432;

    // check
    Real tolerance = 1.0e-6;
    Real price, yield;

    price = BondFunctions::cleanPrice(bond1, marketYield1,
                             bondDayCount, Compounded, freq);
    if (std::fabs(price-cachedPrice1a) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price
                   << "\n    expected:   " << cachedPrice1a
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << price-cachedPrice1a);
    }

    price = bond1.cleanPrice();
    if (std::fabs(price-cachedPrice1b) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price
                   << "\n    expected:   " << cachedPrice1b
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << price-cachedPrice1b);
    }

    yield = BondFunctions::yield(bond1, marketPrice1, bondDayCount, Compounded, freq);
    if (std::fabs(yield-cachedYield1a) > tolerance) {
        BOOST_FAIL("\nfailed to reproduce cached compounded yield:" <<
                   std::setprecision(4) <<
                   "\n    calculated: " << io::rate(yield) <<
                   "\n    expected:   " << io::rate(cachedYield1a) <<
                   "\n    tolerance:  " << io::rate(tolerance) <<
                   "\n    error:      " << io::rate(yield-cachedYield1a));
    }

    yield = BondFunctions::yield(bond1, marketPrice1, bondDayCount, Continuous, freq);
    if (std::fabs(yield-cachedYield1b) > tolerance) {
        BOOST_FAIL("failed to reproduce cached continuous yield:"
                   << std::setprecision(4)
                   << "\n    calculated: " << io::rate(yield)
                   << "\n    expected:   " << io::rate(cachedYield1b)
                   << "\n    tolerance:  " << io::rate(tolerance)
                   << "\n    error:      " << io::rate(yield-cachedYield1b));
    }

    yield = BondFunctions::yield(bond1, bond1.cleanPrice(), bondDayCount, Continuous, freq, bond1.settlementDate());
    if (std::fabs(yield-cachedYield1c) > tolerance) {
        BOOST_FAIL("failed to reproduce cached continuous yield:"
                   << std::setprecision(4)
                   << "\n    calculated: " << io::rate(yield)
                   << "\n    expected:   " << io::rate(cachedYield1c)
                   << "\n    tolerance:  " << io::rate(tolerance)
                   << "\n    error:      " << io::rate(yield-cachedYield1c));
    }


    price = BondFunctions::cleanPrice(bond2, marketYield2, bondDayCount, Compounded, freq);
    if (std::fabs(price-cachedPrice2a) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price
                   << "\n    expected:   " << cachedPrice2a
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << price-cachedPrice2a);
    }

    price = bond2.cleanPrice();
    if (std::fabs(price-cachedPrice2b) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price
                   << "\n    expected:   " << cachedPrice2b
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << price-cachedPrice2b);
    }

    yield = BondFunctions::yield(bond2, marketPrice2, bondDayCount, Compounded, freq);
    if (std::fabs(yield-cachedYield2a) > tolerance) {
        BOOST_FAIL("failed to reproduce cached compounded yield:"
                   << std::setprecision(4)
                   << "\n    calculated: " << io::rate(yield)
                   << "\n    expected:   " << io::rate(cachedYield2a)
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << io::rate(yield-cachedYield2a));
    }

    yield = BondFunctions::yield(bond2, marketPrice2, bondDayCount, Continuous, freq);
    if (std::fabs(yield-cachedYield2b) > tolerance) {
        BOOST_FAIL("failed to reproduce cached continuous yield:"
                   << std::setprecision(4)
                   << "\n    calculated: " << io::rate(yield)
                   << "\n    expected:   " << io::rate(cachedYield2b)
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << io::rate(yield-cachedYield2b));
    }

    yield = BondFunctions::yield(bond2, bond2.cleanPrice(), bondDayCount, Continuous, freq, bond2.settlementDate());
    if (std::fabs(yield-cachedYield2c) > tolerance) {
        BOOST_FAIL("failed to reproduce cached continuous yield:"
                   << std::setprecision(4)
                   << "\n    calculated: " << io::rate(yield)
                   << "\n    expected:   " << io::rate(cachedYield2c)
                   << "\n    tolerance:  " << tolerance
                   << "\n    error:      " << io::rate(yield-cachedYield2c));
    }

    // with explicit settlement date:

    Schedule sch3(Date(30,November,2004),
                  Date(30,November,2006), Period(freq),
                  UnitedStates(UnitedStates::GovernmentBond),
                  Unadjusted, Unadjusted, DateGeneration::Backward, false);

    FixedRateBond bond3(settlementDays, vars.faceAmount, sch3,
                        std::vector<Rate>(1, 0.02875),
                        ActualActual(ActualActual::ISMA),
                        ModifiedFollowing,
                        100.0, Date(30,November,2004));

    bond3.setPricingEngine(bondEngine);

    Rate marketYield3 = 0.02997;

    Date settlementDate = Date(30,November,2004);
    Real cachedPrice3 = 99.764874;

    price = BondFunctions::cleanPrice(bond3, marketYield3,
                             bondDayCount, Compounded, freq, settlementDate);
    if (std::fabs(price-cachedPrice3) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price << ""
                   << "\n    expected:   " << cachedPrice3 << ""
                   << "\n    error:      " << price-cachedPrice3);
    }

    // this should give the same result since the issue date is the
    // earliest possible settlement date

    Settings::instance().evaluationDate() = Date(22,November,2004);

    price = BondFunctions::cleanPrice(bond3, marketYield3, bondDayCount, Compounded, freq);
    if (std::fabs(price-cachedPrice3) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:"
                   << QL_FIXED
                   << "\n    calculated: " << price << ""
                   << "\n    expected:   " << cachedPrice3 << ""
                   << "\n    error:      " << price-cachedPrice3);
    }
}



void BondTest::testCachedZero() {

    BOOST_MESSAGE("Testing zero-coupon bond prices against cached values...");

    CommonVars vars;

    Date today(22,November,2004);
    Settings::instance().evaluationDate() = today;

    Natural settlementDays = 1;

    Handle<YieldTermStructure> discountCurve(flatRate(today,0.03,Actual360()));

    Real tolerance = 1.0e-6;

    // plain

    ZeroCouponBond bond1(settlementDays,
                         UnitedStates(UnitedStates::GovernmentBond),
                         vars.faceAmount,
                         Date(30,November,2008),
                         ModifiedFollowing,
                         100.0, Date(30,November,2004));

    shared_ptr<PricingEngine> bondEngine(
                                    new DiscountingBondEngine(discountCurve));
    bond1.setPricingEngine(bondEngine);

    Real cachedPrice1 = 88.551726;

    Real price = bond1.cleanPrice();
    if (std::fabs(price-cachedPrice1) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice1 << "\n"
                   << "    error:      " << price-cachedPrice1);
    }

    ZeroCouponBond bond2(settlementDays,
                         UnitedStates(UnitedStates::GovernmentBond),
                         vars.faceAmount,
                         Date(30,November,2007),
                         ModifiedFollowing,
                         100.0, Date(30,November,2004));

    bond2.setPricingEngine(bondEngine);

    Real cachedPrice2 = 91.278949;

    price = bond2.cleanPrice();
    if (std::fabs(price-cachedPrice2) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice2 << "\n"
                   << "    error:      " << price-cachedPrice2);
    }

    ZeroCouponBond bond3(settlementDays,
                         UnitedStates(UnitedStates::GovernmentBond),
                         vars.faceAmount,
                         Date(30,November,2006),
                         ModifiedFollowing,
                         100.0, Date(30,November,2004));

    bond3.setPricingEngine(bondEngine);

    Real cachedPrice3 = 94.098006;

    price = bond3.cleanPrice();
    if (std::fabs(price-cachedPrice3) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice3 << "\n"
                   << "    error:      " << price-cachedPrice3);
    }
}


void BondTest::testCachedFixed() {

    BOOST_MESSAGE("Testing fixed-coupon bond prices against cached values...");

    CommonVars vars;

    Date today(22,November,2004);
    Settings::instance().evaluationDate() = today;

    Natural settlementDays = 1;

    Handle<YieldTermStructure> discountCurve(flatRate(today,0.03,Actual360()));

    Real tolerance = 1.0e-6;

    // plain

    Schedule sch(Date(30,November,2004),
                 Date(30,November,2008), Period(Semiannual),
                 UnitedStates(UnitedStates::GovernmentBond),
                 Unadjusted, Unadjusted, DateGeneration::Backward, false);

    FixedRateBond bond1(settlementDays, vars.faceAmount, sch,
                        std::vector<Rate>(1, 0.02875),
                        ActualActual(ActualActual::ISMA),
                        ModifiedFollowing,
                        100.0, Date(30,November,2004));

    shared_ptr<PricingEngine> bondEngine(
                                    new DiscountingBondEngine(discountCurve));
    bond1.setPricingEngine(bondEngine);

    Real cachedPrice1 = 99.298100;

    Real price = bond1.cleanPrice();
    if (std::fabs(price-cachedPrice1) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice1 << "\n"
                   << "    error:      " << price-cachedPrice1);
    }

    // varying coupons

    std::vector<Rate> couponRates(4);
    couponRates[0] = 0.02875;
    couponRates[1] = 0.03;
    couponRates[2] = 0.03125;
    couponRates[3] = 0.0325;

    FixedRateBond bond2(settlementDays, vars.faceAmount, sch,
                          couponRates,
                          ActualActual(ActualActual::ISMA),
                          ModifiedFollowing,
                          100.0, Date(30,November,2004));

    bond2.setPricingEngine(bondEngine);

    Real cachedPrice2 = 100.334149;

    price = bond2.cleanPrice();
    if (std::fabs(price-cachedPrice2) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice2 << "\n"
                   << "    error:      " << price-cachedPrice2);
    }

    // stub date

    Schedule sch3(Date(30,November,2004),
                  Date(30,March,2009), Period(Semiannual),
                  UnitedStates(UnitedStates::GovernmentBond),
                  Unadjusted, Unadjusted, DateGeneration::Backward, false,
                  Date(), Date(30,November,2008));

    FixedRateBond bond3(settlementDays, vars.faceAmount, sch3,
                          couponRates, ActualActual(ActualActual::ISMA),
                          ModifiedFollowing,
                          100.0, Date(30,November,2004));

    bond3.setPricingEngine(bondEngine);

    Real cachedPrice3 = 100.382794;

    price = bond3.cleanPrice();
    if (std::fabs(price-cachedPrice3) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice3 << "\n"
                   << "    error:      " << price-cachedPrice3);
    }
}


void BondTest::testCachedFloating() {

    BOOST_MESSAGE("Testing floating-rate bond prices against cached values...");

    CommonVars vars;

    Date today(22,November,2004);
    Settings::instance().evaluationDate() = today;

    Natural settlementDays = 1;

    Handle<YieldTermStructure> riskFreeRate(flatRate(today,0.025,Actual360()));
    Handle<YieldTermStructure> discountCurve(flatRate(today,0.03,Actual360()));

    shared_ptr<IborIndex> index(new USDLibor(6*Months, riskFreeRate));
    Natural fixingDays = 1;

    Real tolerance = 1.0e-6;

    shared_ptr<IborCouponPricer> pricer(new
        BlackIborCouponPricer(Handle<OptionletVolatilityStructure>()));

    // plain

    Schedule sch(Date(30,November,2004),
                 Date(30,November,2008),
                 Period(Semiannual),
                 UnitedStates(UnitedStates::GovernmentBond),
                 ModifiedFollowing, ModifiedFollowing,
                 DateGeneration::Backward, false);

    FloatingRateBond bond1(settlementDays, vars.faceAmount, sch,
                           index, ActualActual(ActualActual::ISMA),
                           ModifiedFollowing, fixingDays,
                           std::vector<Real>(), std::vector<Spread>(),
                           std::vector<Rate>(), std::vector<Rate>(),
                           false,
                           100.0, Date(30,November,2004));

    shared_ptr<PricingEngine> bondEngine(
                                     new DiscountingBondEngine(riskFreeRate));
    bond1.setPricingEngine(bondEngine);

    setCouponPricer(bond1.cashflows(),pricer);

    #if defined(QL_USE_INDEXED_COUPON)
    Real cachedPrice1 = 99.874645;
    #else
    Real cachedPrice1 = 99.874646;
    #endif


    Real price = bond1.cleanPrice();
    if (std::fabs(price-cachedPrice1) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice1 << "\n"
                   << "    error:      " << price-cachedPrice1);
    }

    // different risk-free and discount curve

    FloatingRateBond bond2(settlementDays, vars.faceAmount, sch,
                           index, ActualActual(ActualActual::ISMA),
                           ModifiedFollowing, fixingDays,
                           std::vector<Rate>(), std::vector<Spread>(),
                           std::vector<Rate>(), std::vector<Rate>(),
                           false,
                           100.0, Date(30,November,2004));

    shared_ptr<PricingEngine> bondEngine2(
                                    new DiscountingBondEngine(discountCurve));
    bond2.setPricingEngine(bondEngine2);

    setCouponPricer(bond2.cashflows(),pricer);

    #if defined(QL_USE_INDEXED_COUPON)
    Real cachedPrice2 = 97.955904;
    #else
    Real cachedPrice2 = 97.955904;
    #endif

    price = bond2.cleanPrice();
    if (std::fabs(price-cachedPrice2) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice2 << "\n"
                   << "    error:      " << price-cachedPrice2);
    }

    // varying spread

    std::vector<Rate> spreads(4);
    spreads[0] = 0.001;
    spreads[1] = 0.0012;
    spreads[2] = 0.0014;
    spreads[3] = 0.0016;

    FloatingRateBond bond3(settlementDays, vars.faceAmount, sch,
                           index, ActualActual(ActualActual::ISMA),
                           ModifiedFollowing, fixingDays,
                           std::vector<Real>(), spreads,
                           std::vector<Rate>(), std::vector<Rate>(),
                           false,
                           100.0, Date(30,November,2004));

    bond3.setPricingEngine(bondEngine2);

    setCouponPricer(bond3.cashflows(),pricer);

    #if defined(QL_USE_INDEXED_COUPON)
    Real cachedPrice3 = 98.495458;
    #else
    Real cachedPrice3 = 98.495459;
    #endif

    price = bond3.cleanPrice();
    if (std::fabs(price-cachedPrice3) > tolerance) {
        BOOST_FAIL("failed to reproduce cached price:\n"
                   << QL_FIXED
                   << "    calculated: " << price << "\n"
                   << "    expected:   " << cachedPrice3 << "\n"
                   << "    error:      " << price-cachedPrice3);
    }
}

void BondTest::testBrazilianCached() {

    BOOST_MESSAGE(
        "Testing Brazilian public bond prices against Andima cached values...");

    CommonVars vars;

    Natural settlementDays = 1;
    Real faceAmount = 1000.0;
    Real redemption = 100.0;
    Date today(6,June,2007);
    Date issueDate(1,January,2007);

    // The tolerance is high because Andima truncate yields
    Real tolerance = 1.0e-4;

    // Reset evaluation date
    Settings::instance().evaluationDate() = today;

    // NTN-F maturity dates
    std::vector<Date> maturityDates(6);
    maturityDates[0] = Date(1,January,2008);
    maturityDates[1] = Date(1,January,2010);
    maturityDates[2] = Date(1,July,2010);
    maturityDates[3] = Date(1,January,2012);
    maturityDates[4] = Date(1,January,2014);
    maturityDates[5] = Date(1,January,2017);

    // Andima NTN-F yields
    std::vector<Rate> yields(6);
    yields[0] = 0.114614;
    yields[1] = 0.105726;
    yields[2] = 0.105328;
    yields[3] = 0.104283;
    yields[4] = 0.103218;
    yields[5] = 0.102948;

    // Andima NTN-F prices
    std::vector<Rate> prices(6);
    prices[0] = 1034.63031372;
    prices[1] = 1030.09919487;
    prices[2] = 1029.98307160;
    prices[3] = 1028.13585068;
    prices[4] = 1028.33383817;
    prices[5] = 1026.19716497;

    std::vector<InterestRate> couponRates(1);
    couponRates[0] = InterestRate(0.1, Thirty360(), Compounded,Annual);

    for (Size bondIndex = 0; bondIndex < maturityDates.size(); bondIndex++) {

        InterestRate yield(yields[bondIndex],
                           Business252(Brazil()),
                           Compounded, Annual);

        Schedule schedule(Date(1,January,2007),
                          maturityDates[bondIndex], Period(Semiannual),
                          Brazil(Brazil::Settlement),
                          Unadjusted, Unadjusted,
                          DateGeneration::Backward, false);

        FixedRateBond bond(settlementDays,
            faceAmount,
            schedule,
            couponRates,
            Following,
            redemption,
            issueDate);

        Real cachedPrice = prices[bondIndex];
        Real price = faceAmount *
            (BondFunctions::cleanPrice(bond, yield.rate(), yield.dayCounter(),
                                 yield.compounding(), yield.frequency(),
                                 today) + bond.accruedAmount(today)) / 100.0;

        if (std::fabs(price-cachedPrice) > tolerance) {
            BOOST_ERROR("failed to reproduce Andima cached price:\n"
                        << QL_FIXED
                        << "    calculated: " << price << "\n"
                        << "    expected:   " << cachedPrice << "\n"
                        << "    error:      " << price-cachedPrice  << "\n"
                        );
        }
    }
}



test_suite* BondTest::suite() {
    test_suite* suite = BOOST_TEST_SUITE("Bond tests");

    suite->add(QUANTLIB_TEST_CASE(&BondTest::testYield));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testAtmRate));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testZspread));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testTheoretical));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testCached));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testCachedZero));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testCachedFixed));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testCachedFloating));
    suite->add(QUANTLIB_TEST_CASE(&BondTest::testBrazilianCached));
    return suite;
}

