
#
# makefile for QuantLib Examples under Borland C++
#
# $Id$

.autodepend
.silent

# Options
!ifdef DEBUG
    MAKE = $(MAKE) -DDEBUG
!endif

# Primary target:
# QuantLib Examples
examples::
    cd DiscreteHedging
    $(MAKE)
    cd ..\EuropeanOption
    $(MAKE)
    cd ..\Swap
    $(MAKE)
    cd ..

# Check
check::
    cd DiscreteHedging
    $(MAKE) check
    cd ..\EuropeanOption
    $(MAKE) check
    cd ..\Swap
    $(MAKE) check
    cd ..

# Clean up
clean::
    cd DiscreteHedging
    $(MAKE) clean
    cd ..\EuropeanOption
    $(MAKE) clean
    cd ..\Swap
    $(MAKE) clean
    cd ..
