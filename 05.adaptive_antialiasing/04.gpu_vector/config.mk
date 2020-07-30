#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# config.mk
# $Id: config.mk,v 1.1.1.3 2020/07/30 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# DEVICE: OpenCL device type [default|cpu|gpu]
#........................................................................
DEVICE	= default
#------------------------------------------------------------------------
# EQVCLR: equivalent color detection [relaxed|strict]
#........................................................................
EQVCLR	= relaxed
#------------------------------------------------------------------------
# SAMPLE: sampling method [halton|hammersley|mt19937|rand]
#........................................................................
SAMPLE	= hammersley
#------------------------------------------------------------------------
# DATA  : input data set (input/$(DATA).dat)
#........................................................................
DATA	= 001
