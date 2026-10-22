#ifndef POKEBW2_SPL_SPL_H
#define POKEBW2_SPL_SPL_H

#include "types.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

// Nintendo's SPL particle library (lib/spl), decompiled from this game's code: a manager of emitters made from a
// resource file's particle definitions. The public functions keep the names pret's Platinum decompilation gives
// them. The fields named unk* are ones the library never reads

// The library's doubly linked lists, of emitters and of particles, which start with these links
typedef struct SPLNode {
    struct SPLNode *next;
    struct SPLNode *prev;
} SPLNode;

typedef struct {
    SPLNode *first;
    int count;
    SPLNode *last;
} SPLList;

void SPLList_PushFront(SPLList *list, SPLNode *node);
SPLNode *SPLList_PopFront(SPLList *list);
SPLNode *SPLList_Erase(SPLList *list, SPLNode *node);

typedef void *(*SPLAllocFunc)(u32 size);
typedef u32 (*SPLTexVRAMAllocFunc)(u32 size, BOOL is4x4comp);
typedef u32 (*SPLPalVRAMAllocFunc)(u32 size, BOOL is4pltt);

typedef struct SPLEmitter SPLEmitter;

typedef struct SPLParticle {
    struct SPLParticle *next;
    struct SPLParticle *prev;
    // Relative to the emitter, which was at emitterPos when the particle was emitted
    VecFx32 position;
    VecFx32 velocity;
    u16 rotation;
    s16 angularVelocity;
    u16 lifeTime;
    u16 age;
    // 0x10000 over the loop and life times, to map ages to [0, 255]
    u16 loopTimeFactor;
    u16 lifeTimeFactor;
    u8 texture;
    // Added to the life rate of a looping particle, so that particles emitted together do not animate together
    u8 lifeRateOffset;
    u16 baseAlpha : 5;
    u16 animAlpha : 5;
    u16 polygonId : 6;
    fx32 baseScale;
    fx16 animScale;
    GXRgb color;
    VecFx32 emitterPos;
} SPLParticle;

typedef void (*SPLEmitterCallback)(SPLEmitter *emitter);
typedef void (*SPLEmitterUpdateCallback)(SPLEmitter *emitter, u32 type);
typedef void (*SPLBehaviorFunc)(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);

// Where an emitter's particles start, relative to it. The circles and cylinders lie in the plane of the emitter's
// cross axes, and the hemispheres on the side of it their normal points to
enum {
    SPL_EMISSION_POINT,
    SPL_EMISSION_SPHERE_SURFACE,
    SPL_EMISSION_CIRCLE_BORDER,
    // Spread evenly around the circle
    SPL_EMISSION_CIRCLE_BORDER_UNIFORM,
    SPL_EMISSION_SPHERE,
    SPL_EMISSION_CIRCLE,
    SPL_EMISSION_CYLINDER_SURFACE,
    SPL_EMISSION_CYLINDER,
    SPL_EMISSION_HEMISPHERE_SURFACE,
    SPL_EMISSION_HEMISPHERE,
};

// How particles are drawn: as billboards facing the camera, or polygons in the plane of the emitter's cross axes,
// either way stretched along their velocity if directional
enum {
    SPL_DRAW_BILLBOARD,
    SPL_DRAW_DIRECTIONAL_BILLBOARD,
    SPL_DRAW_POLYGON,
    SPL_DRAW_DIRECTIONAL_POLYGON,
    SPL_DRAW_DIRECTIONAL_POLYGON_CENTER,
};

// The axes a particle's scale animation scales
enum {
    SPL_SCALE_ANIM_DIR_XY,
    SPL_SCALE_ANIM_DIR_X,
    SPL_SCALE_ANIM_DIR_Y,
};

// The axis that the circles and cylinders of emission are around
enum {
    SPL_AXIS_Z,
    SPL_AXIS_Y,
    SPL_AXIS_X,
    SPL_AXIS_EMITTER,
};

typedef struct {
    u32 emissionType : 4;
    u32 drawType : 2;
    u32 emissionAxis : 2;
    u32 hasScaleAnim : 1;
    u32 hasColorAnim : 1;
    u32 hasAlphaAnim : 1;
    u32 hasTextureAnim : 1;
    u32 hasRotation : 1;
    u32 randomInitAngle : 1;
    // Deletes the emitter once it has emitted for its life time and its particles are gone. The manager hands out no
    // pointer to such an emitter
    u32 autoTerminate : 1;
    // Moves the particles with the emitter after they are emitted
    u32 followEmitter : 1;
    u32 hasChildResource : 1;
    // The axis polygons turn about with their rotation: y, or the diagonal (1, 1, 1)
    u32 polygonRotAxis : 2;
    // The plane polygons are drawn in: xy, or xz
    u32 polygonReferencePlane : 1;
    // Starts each particle's looped animations at a random point
    u32 randomLoopOffset : 1;
    u32 drawChildrenFirst : 1;
    u32 hideParent : 1;
    // Draws the particles relative to the emitter's base position, translating them to it
    u32 relativeToBasePos : 1;
    // The behaviors the resource has, in this order after its animations and child resource
    u32 hasGravityBehavior : 1;
    u32 hasRandomBehavior : 1;
    u32 hasMagnetBehavior : 1;
    u32 hasSpinBehavior : 1;
    u32 hasCollisionPlaneBehavior : 1;
    u32 hasConvergenceBehavior : 1;
    // Gives the particles, or the children, the manager's fixed polygon ID rather than a new one each
    u32 fixedPolygonID : 1;
    u32 childFixedPolygonID : 1;
} SPLResourceFlags;

typedef struct {
    SPLResourceFlags flags;
    VecFx32 emitterBasePos;
    fx32 emissionCount;
    fx32 radius;
    fx32 length;
    VecFx16 axis;
    // The particles' color, which the color animation fades to and from
    GXRgb color;
    fx32 initVelPositionAmplifier;
    fx32 initVelAxisAmplifier;
    fx32 baseScale;
    // The width of the particles over their height
    fx16 aspectRatio;
    // The frames before the emitter starts
    u16 startDelay;
    // The range of the particles' angular velocities
    s16 minRotation;
    s16 maxRotation;
    u16 initAngle;
    u8 unk3a[2];
    // How long the emitter emits for, or 0 for as long as it lives
    u16 emitterLifeTime;
    u16 particleLifeTime;
    // How much of their scale, life time and initial speed particles may lose at random, out of 255
    u8 scaleVariance;
    u8 lifeTimeVariance;
    u8 velocityVariance;
    u8 unk43;
    u8 emissionInterval;
    u8 baseAlpha;
    // The part of their velocity particles keep each frame, as 384 more than this out of 512
    u8 airResistance;
    u8 texture;
    u32 loopTime : 8;
    // How much directional billboards stretch along their velocity as it turns across the view
    u32 dirStretch : 16;
    // The texture repeats over a particle, as a power of two
    u32 textureRepeatShiftS : 2;
    u32 textureRepeatShiftT : 2;
    // Which of the particles' axes the scale animation scales
    u32 scaleAnimDir : 3;
    // Points directional polygons back along their position rather than along their velocity
    u32 dirFromPosition : 1;
    u32 flipTextureS : 1;
    u32 flipTextureT : 1;
    u32 : 30;
    // Where the polygon is drawn relative to the particle, in its own units
    fx16 polygonX;
    fx16 polygonY;
    u32 unk54;
} SPLResourceHeader;

// The animations of a particle's scale, color, alpha and texture over its life, which runs from 0 to 255. Each fades
// in from a start value until in, holds until out, and fades to its end value from there

typedef struct {
    fx16 start;
    fx16 mid;
    fx16 end;
    u8 in;
    u8 out;
    // Animates over the loop time rather than the life time
    u16 loop : 1;
    u16 : 15;
    u16 padding;
} SPLScaleAnim;

// Fades from start to the resource's color between in and peak, and from that to end between peak and out, or
// changes at those points if not interpolated
typedef struct {
    GXRgb start;
    GXRgb end;
    u8 in;
    u8 peak;
    u8 out;
    u8 padding;
    // Starts each particle at one of the three colors, at random
    u16 randomStart : 1;
    u16 loop : 1;
    u16 interpolate : 1;
    u16 : 13;
    u16 padding2;
} SPLColorAnim;

typedef struct {
    u16 start : 5;
    u16 mid : 5;
    u16 end : 5;
    u16 : 1;
    // How much of the alpha a particle may lose at random, out of 255
    u16 randomRange : 8;
    u16 loop : 1;
    u16 : 7;
    u8 in;
    u8 out;
    u16 padding2;
} SPLAlphaAnim;

// Shows each texture for step of the particle's life
typedef struct {
    u8 textures[8];
    u32 count : 8;
    u32 step : 8;
    // Gives each particle one of the textures at random
    u32 randomStart : 1;
    u32 loop : 1;
    u32 : 14;
} SPLTextureAnim;

// What a child particle takes from its parent's rotation
enum {
    SPL_CHILD_ROTATION_NONE,
    SPL_CHILD_ROTATION_ANGLE,
    SPL_CHILD_ROTATION_ANGLE_AND_VELOCITY,
};

// The particles each particle emits
typedef struct {
    // Applies the resource's behaviors to the children too
    u16 hasBehaviors : 1;
    u16 hasScaleAnim : 1;
    u16 hasAlphaAnim : 1;
    u16 rotationType : 2;
    u16 followEmitter : 1;
    // Gives the children their own color, rather than their parent's
    u16 useChildColor : 1;
    u16 drawType : 2;
    u16 polygonRotAxis : 2;
    u16 polygonReferencePlane : 1;
    u16 : 4;
    // The children's speed, up to which their velocities differ at random from their parent's
    fx16 randomInitVelMag;
    // The scale a child particle shrinks or grows to over its life
    fx16 endScale;
    u16 lifeTime;
    // The parts of their parent's velocity and scale that children start with, out of 256 and 64
    u8 velocityRatio;
    u8 scaleRatio;
    GXRgb color;
    u8 emissionCount;
    // Particles emit children every emissionInterval frames from emissionDelay out of 256 of their lives on
    u8 emissionDelay;
    u8 emissionInterval;
    u8 texture;
    u32 textureRepeatShiftS : 2;
    u32 textureRepeatShiftT : 2;
    u32 flipTextureS : 1;
    u32 flipTextureT : 1;
    u32 dirFromPosition : 1;
    u32 : 25;
} SPLChildResource;

// A force on an emitter's particles: the function applying it, and its parameters
typedef struct {
    SPLBehaviorFunc applyFunc;
    const void *object;
} SPLBehavior;

typedef struct {
    SPLResourceHeader *header;
    SPLScaleAnim *scaleAnim;
    SPLColorAnim *colorAnim;
    SPLAlphaAnim *alphaAnim;
    SPLTextureAnim *textureAnim;
    SPLChildResource *childResource;
    SPLBehavior *behaviors;
    u16 behaviorCount;
} SPLResource;

typedef struct {
    VecFx16 magnitude;
    u16 padding;
} SPLGravityBehavior;

// Every applyInterval frames, a random force of up to magnitude either way
typedef struct {
    VecFx16 magnitude;
    u16 applyInterval;
} SPLRandomBehavior;

typedef struct {
    VecFx32 target;
    fx16 force;
    u16 padding;
} SPLMagnetBehavior;

typedef struct {
    u16 angle;
    u16 axis;
} SPLSpinBehavior;

// A plane at height y that particles crossing it die at or bounce off
typedef struct {
    fx32 y;
    fx16 elasticity;
    u16 type : 2;
} SPLCollisionPlaneBehavior;

enum {
    SPL_COLLISION_KILL,
    SPL_COLLISION_BOUNCE,
};

// Pulls particles' positions straight towards the target
typedef struct {
    VecFx32 target;
    fx16 force;
    u16 padding;
} SPLConvergenceBehavior;

typedef union {
    u32 all;
    struct {
        // Asks the manager to delete the emitter once its particles are gone
        u32 terminate : 1;
        u32 stopEmission : 1;
        u32 paused : 1;
        u32 hidden : 1;
        // Set once the emitter may emit
        u32 started : 1;
        u32 : 27;
    };
} SPLEmitterState;

// The kinds of call of an emitter's update callback, before and after the update
enum {
    SPL_EMITTER_CALLBACK_FRONT,
    SPL_EMITTER_CALLBACK_BACK,
};

struct SPLEmitter {
    SPLEmitter *next;
    SPLEmitter *prev;
    SPLList particles;
    SPLList childParticles;
    SPLResource *resource;
    SPLEmitterState state;
    VecFx32 position;
    VecFx32 velocity;
    VecFx32 particleInitVelocity;
    u16 age;
    fx16 emissionCountFractional;
    VecFx16 axis;
    u16 initAngle;
    fx32 emissionCount;
    fx32 radius;
    fx32 length;
    fx32 initVelPositionAmplifier;
    fx32 initVelAxisAmplifier;
    fx32 baseScale;
    u16 particleLifeTime;
    GXRgb color;
    // Overrides the collision plane behavior's height, unless FX32_MIN
    fx32 collisionPlaneHeight;
    fx16 textureS;
    fx16 textureT;
    fx16 childTextureS;
    fx16 childTextureT;
    u32 emissionInterval : 8;
    u32 baseAlpha : 8;
    u32 updateCycle : 3;
    u32 reserved : 13;
    VecFx16 crossAxis1;
    VecFx16 crossAxis2;
    SPLEmitterUpdateCallback updateCallback;
    void *userDataPtr;
    u32 unk98;
};

typedef struct {
    SPLEmitter *first;
    int count;
    SPLEmitter *last;
} SPLEmitterList;

// A texture's TEXIMAGE_PARAM fields
typedef struct {
    u32 format : 4;
    u32 s : 4;
    u32 t : 4;
    u32 repeat : 2;
    u32 flip : 2;
    u32 palColor0 : 1;
    // Uses the VRAM of texture sharedTexID rather than uploading its own
    u32 useSharedTexture : 1;
    u32 sharedTexID : 8;
    u32 : 6;
} SPLTextureParam;

// A texture in the resource file: this header, the texture data, then the palette
typedef struct {
    u32 signature;
    SPLTextureParam param;
    u32 textureSize;
    u32 paletteOffset;
    u32 paletteSize;
    u32 unk14;
    u32 unk18;
    // The size of the header, texture and palette, to the next texture
    u32 resourceSize;
} SPLTextureResource;

// The resource file's header, followed by the resources and the textures
typedef struct {
    u32 signature;
    u32 version;
    u16 resourceCount;
    u16 textureCount;
    u32 unkC;
    u32 resourceSize;
    u32 textureSize;
    u32 textureOffset;
    u32 unk1C;
} SPLFileHeader;

// A texture of the resource file, once uploaded to VRAM
typedef struct {
    const SPLTextureResource *data;
    u32 texAddr;
    u32 palAddr;
    SPLTextureParam param;
    u16 width;
    u16 height;
} SPLTexture;

typedef struct SPLManager {
    SPLAllocFunc alloc;
    SPLEmitterList activeEmitters;
    SPLEmitterList inactiveEmitters;
    SPLList unusedParticles;
    SPLResource *resources;
    SPLTexture *textures;
    u16 resourceCount;
    u16 textureCount;
    u16 maxEmitters;
    u16 maxParticles;
    // Particles take polygon IDs from min to max in turn, or the fixed one
    u32 minPolygonID : 6;
    u32 maxPolygonID : 6;
    u32 currentPolygonID : 6;
    u32 fixPolygonID : 6;
    // Draws the emitters from the first created to the newest, instead of the newest first (emitters are pushed to
    // the front of the active list)
    u32 drawOldestFirst : 1;
    u32 reserved : 7;
    // Ored into the particles' polygon attributes
    u32 polygonAttr;
    // The emitter being drawn
    SPLEmitter *drawEmitter;
    const MtxFx43 *viewMatrix;
    // Counts 0 and 1, for emitters updated every other frame
    u16 frame;
    u16 padding;
} SPLManager;

// The behaviors' functions, which tell a behavior's kind
void SPLBehavior_ApplyGravity(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);
void SPLBehavior_ApplyRandom(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);
void SPLBehavior_ApplyMagnet(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);
void SPLBehavior_ApplySpin(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);
void SPLBehavior_ApplyCollisionPlane(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);
void SPLBehavior_ApplyConvergence(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter);

SPLManager *SPLManager_New(SPLAllocFunc alloc, u16 maxEmitters, u16 maxParticles, u16 fixPolyID, u16 minPolyID,
                           u16 maxPolyID);
void SPLManager_LoadResources(SPLManager *mgr, const void *data);
// Upload the resource's textures and palettes to VRAM, with the default VRAM managers or the given allocators
BOOL SPLManager_UploadTextures(SPLManager *mgr);
BOOL SPLManager_UploadTexturesEx(SPLManager *mgr, SPLTexVRAMAllocFunc vramAlloc);
BOOL SPLManager_UploadPalettes(SPLManager *mgr);
BOOL SPLManager_UploadPalettesEx(SPLManager *mgr, SPLPalVRAMAllocFunc vramAlloc);
void SPLManager_Update(SPLManager *mgr);
void SPLManager_Draw(SPLManager *mgr, const MtxFx43 *viewMatrix);
SPLEmitter *SPLManager_CreateEmitter(SPLManager *mgr, int resourceID, const VecFx32 *pos);
SPLEmitter *SPLManager_CreateEmitterWithCallback(SPLManager *mgr, int resourceID, SPLEmitterCallback initCallback);
void SPLManager_DeleteEmitter(SPLManager *mgr, SPLEmitter *emitter);
void SPLManager_DeleteAllEmitters(SPLManager *mgr);

#endif // POKEBW2_SPL_SPL_H
