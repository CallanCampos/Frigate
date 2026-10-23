#include "nnsys/g3d.h"

// NitroSystem's G3D computer graphics tool support: what a model is drawn with depends on the tool that made it, so a
// model's scaling rule and texture matrix mode pick from these tables. The functions are in basic.c, maya.c, si3d.c,
// 3dsmax.c and xsi.c. The file and the tables have NitroSystem's names, as ntrtwl's reconstruction of NitroSystem has
// them, and g3d.h notes swan's. The two tables of three come out in the reverse of their declaration order

NNSG3dGetJointScale NNS_G3dGetJointScale_FuncArray[3] = { NNSi_G3dGetJointScaleBasic, NNSi_G3dGetJointScaleMaya,
                                                          NNSi_G3dGetJointScaleSi3d };
NNSG3dSendJointSRT NNS_G3dSendJointSRT_FuncArray[3] = { NNSi_G3dSendJointSRTBasic, NNSi_G3dSendJointSRTMaya,
                                                        NNSi_G3dSendJointSRTSi3d };
NNSG3dSendTexSRT NNS_G3dSendTexSRT_FuncArray[4] = { NNSi_G3dSendTexSRTMaya, NNSi_G3dSendTexSRTSi3d,
                                                    NNSi_G3dSendTexSRT3dsMax, NNSi_G3dSendTexSRTXsi };
