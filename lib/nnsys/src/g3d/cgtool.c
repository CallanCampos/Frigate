#include "nnsys/g3d.h"

// NitroSystem's G3D computer graphics tool support: what a model is drawn with depends on the tool that made it, so a
// model's scaling rule and texture matrix mode pick from these tables. The functions are in basic.c, maya.c, si3d.c,
// 3dsmax.c and xsi.c. The file's name is a guess from the tables. The two tables of three come out in the reverse of
// their declaration order

NNSG3dFuncJointScale NNS_G3D_JOINT_SCALE_FUNCS[3] = { func_02069f94, func_0206a044, NNSi_G3dGetJointScaleSi3d };
NNSG3dFuncJointFlush NNS_G3D_JOINT_FLUSH_FUNCS[3] = { func_02069f40, func_02069fbc, func_0206a714 };
NNSG3dFuncTexMatrix NNS_G3D_TEX_MATRIX_FUNCS[4] = { func_0206a5cc, func_0206aa5c, func_0206b124, func_0206b74c };
