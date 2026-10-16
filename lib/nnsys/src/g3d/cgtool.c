#include "nnsys/g3d.h"

// NitroSystem's G3D computer graphics tool support: what a model is drawn with depends on the tool that made it, so a
// model's scaling rule and texture matrix mode pick from these tables. The functions are in basic.c, maya.c, si3d.c,
// 3dsmax.c and xsi.c. The file's name is a guess from the tables. The two tables of three come out in the reverse of
// their declaration order

NNSG3dFuncJointScale NNS_G3D_JOINT_SCALE_FUNCS[3] = { NNSi_G3dGetJointScaleBasic, NNSi_G3dGetJointScaleMaya,
                                                      NNSi_G3dGetJointScaleSi3d };
NNSG3dFuncJointFlush NNS_G3D_JOINT_FLUSH_FUNCS[3] = { NNSi_G3dSendJointSRTBasic, NNSi_G3dSendJointSRTMaya,
                                                      NNSi_G3dSendJointSRTSi3d };
NNSG3dFuncTexMatrix NNS_G3D_TEX_MATRIX_FUNCS[4] = { NNSi_G3dSendTexSRTMaya, NNSi_G3dSendTexSRTSi3d,
                                                    NNSi_G3dSendTexSRT3dsMax, NNSi_G3dSendTexSRTXsi };
