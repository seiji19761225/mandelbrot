#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# config.mk
# $Id: config.mk,v 1.1.1.5 2018/09/11 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# code configurations
#------------------------------------------------------------------------
# BOOL_T: bool_t data type [bool|int]
#........................................................................
BOOL_T	= int
#------------------------------------------------------------------------
# EQVCLR: equivalent color detection [relaxed|strict]
#........................................................................
EQVCLR	= relaxed
#------------------------------------------------------------------------
# REAL_T: floating point data type [fp64|fp80|fp128]
#........................................................................
REAL_T	= fp64
#------------------------------------------------------------------------
# SAMPLE: sampling method [halton|hammersley|mt19937|rand]
#........................................................................
SAMPLE	= hammersley
#------------------------------------------------------------------------
# VECTOR: vector length (0:scalar kernel, 1:OpenMP SIMD, >=2:vector kernel)
#........................................................................
VECTOR	= 0
#------------------------------------------------------------------------
# DATA  : input data set [input/$(DATA).dat]
#........................................................................
DATA	= benchmark