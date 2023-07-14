#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <dlfcn.h>
#include <vector>
#include <cctype>

#define GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#include "GTASA_STRUCTS.h"

MYMODCFG(net.rusjj.jpatch, JPatch, 1.4.3, RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.1)
END_DEPLIST()

union ScriptVariables
{
    int      i;
    float    f;
    uint32_t u;
    void*    p;
};

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////     Saves     ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
uintptr_t pGTASA, pSC;
void* hGTASA, *hSC;
static constexpr float ar43 = 4.0f / 3.0f;
static constexpr float fMagic = 50.0f / 30.0f;
static constexpr int nMaxScriptSprites = 384; // Changing it wont make it bigger.

bool bIsRWReady = false;
float fEmergencyVehiclesFix;
CSprite2d** pNewScriptSprites = new CSprite2d*[nMaxScriptSprites] {NULL}; // 384*4=1536 0x600
void* pNewIntroRectangles = new void*[15*nMaxScriptSprites] {NULL}; // 384*60=23040 0x5A00
CRegisteredShadow* asShadowsStored_NEW;
CStaticShadow* aStaticShadows_NEW;

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////     Vars      ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define GetTimeStep() (*ms_fTimeStep)
#define GetTimeStepInSeconds() (*ms_fTimeStep / 50.0f)

CPlayerInfo* WorldPlayers;
CIntVector2D* windowSize;
CCamera* TheCamera;
RsGlobalType* RsGlobal;
MobileMenu *gMobileMenu;
CWidget** m_pWidgets;
ScriptVariables* ScriptParams;
CLinkList<AlphaObjectInfo>* m_alphaList;
CPool<CCutsceneObject>** pObjectPool;
CZoneInfo** m_pCurrZoneInfo;
CWeaponInfo* aWeaponInfo;
int keys[538];
bool *ms_bIsPlayerOnAMission;
int *DETAILEDWATERDIST;
int *ms_nNumGang;
CPolyBunch* aPolyBunches;
CBaseModelInfo** ms_modelInfoPtrs;
CRGBA* ms_vehicleColourTable;

CTaskComplexSequence* ms_taskSequence;
CRunningScript** pActiveScripts;
float *ms_fTimeStep, *ms_fFOV, *game_FPS, *CloudsRotation, *WeatherWind, *fSpriteBrightness, *m_f3rdPersonCHairMultX, *m_f3rdPersonCHairMultY, *ms_fAspectRatio;
void *g_surfaceInfos;
unsigned int *m_snTimeInMilliseconds;
int *lastDevice, *NumberOfSearchLights, *ms_numAnimBlocks, *RasterExtOffset, *detailTexturesStorage, *textureDetail, *ms_iActiveSequence;
bool *bDidWeProcessAnyCinemaCam, *bRunningCutscene, *bProcessingCutscene;
uint32_t *CloudsIndividualRotation, *m_ZoneFadeTimer, *ms_memoryUsed, *ms_memoryAvailable;
static uint32_t CCheat__m_aCheatHashKeys[] = { 
   0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 
   // WEAPON4, TIMETRAVEL, SCRIPTBYPASS, SHOWMAPPINGS 
   0x5A1B5E9A, 0x00000000, 0x00000000, 0x00000000, 
   // INVINCIBILITY, SHOWTAPTOTARGET, SHOWTARGETING 
   0x7B64E263, 0x00000000, 0x00000000, 
   0xEECCEA2B, 
   0x42AF1E28, 0x555FC201, 0x2A845345, 0xE1EF01EA, 
   0x771B83FC, 0x5BF12848, 0x44453A17, 0x00000000, 
   0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 
   0x00000000, 0x43DB914E, 0xDBC0DD65, 0x00000000, 
   0xD08A30FE, 0x37BF1B4E, 0xB5D40866, 0xE63B0D99, 
   0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x00000000, 
   0x00000000, 0x0D5C6A4E, 0x00000000, 0x00000000, 
   0x66516EBC, 0x4B137E45, 0x00000000, 0x00000000, 
   0x3A577325, 0xD4966D59, 
   // THEGAMBLER 
   0x00000000, 
   0x5FD1B49D, 0xA7613F99, 
   0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866, 
   0x2EF877DB, 0x2781E797, 0x2BC1A045, 0xB2AFE368, 
   0x00000000, 0x00000000, 0x1A5526BC, 0xA48A770B, 
   0x00000000, 0x00000000, 0x00000000, 0x7F80B950, 
   0x6C0FA650, 0xF46F2FA4, 0x70164385, 0x00000000, 
   0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE, 
   0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x00000000, 
   0x00000000, 0xF53EF5A5, 0xF2AA0C1D, 0xF36345A8, 
   0x00000000, 0xB7013B1B, 0x00000000, 0x31F0C3CC, 
   0x00000000, 0x00000000, 0x00000000, 0x00000000, 
   0x00000000, 0xF01286E9, 0xA841CC0A, 0x31EA09CF, 
   0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC, 
   0x86988DAE, 0x2BDD2FA1, 0x00000000, 0x00000000, 
   0x00000000, 0x00000000, 0x00000000, 0x00000000, 
   0x00000000, 0x00000000, 0x00000000, 0x00000000, 
   0x00000000, 0x00000000, 0x00000000, 
}; // VitaPort

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////     Funcs     ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int ret0(int a, ...) { return 0; } // Generic
int ret1(int a, ...) { return 1; } // Generic
void RedirectToRegister(unsigned char reg, uintptr_t addr, uintptr_t to)
{
    if(!addr) return;
    uintptr_t hook[2] = {(uintptr_t)(0x10000000*reg + 0x0000F8DF), to}; // idk
    if(addr & 1)
    {
        addr &= ~1;
        if (addr & 2)
        {
            aml->PlaceNOP(addr, 1); 
            addr += 2;
        }
        //hook[0] = 0x10000000*reg + 0x0000F8DF;
    }
    aml->Write(addr, (uintptr_t)hook, sizeof(hook));
}

float fAspectCorrection = 0.0f, fAspectCorrectionDiv = 0.0f;
CRGBA OwnHudColour[HUD_COLOUR_COUNT];
DECL_HOOK(bool, InitRenderWare)
{
    if(!InitRenderWare()) return false;
    
    bIsRWReady = true;
    
    OwnHudColour[HUD_COLOUR_RED] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_GREEN] = CRGBA(54, 104, 44);
    OwnHudColour[HUD_COLOUR_DARK_BLUE] = CRGBA(50, 60, 127);
    OwnHudColour[HUD_COLOUR_LIGHT_BLUE] = CRGBA(172, 203, 241);
    OwnHudColour[HUD_COLOUR_LIGHT_GRAY] = CRGBA(225, 225, 225);
    OwnHudColour[HUD_COLOUR_WHITE] = CRGBA(255, 255, 255);
    OwnHudColour[HUD_COLOUR_BLACK] = CRGBA(0, 0, 0);
    OwnHudColour[HUD_COLOUR_GOLD] = CRGBA(144, 98, 16);
    OwnHudColour[HUD_COLOUR_PURPLE] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_DARK_GRAY] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_DARK_RED] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_DARK_GREEN] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_CREAM] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_NIGHT_BLUE] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_BLUE] = CRGBA(180, 25, 29);
    OwnHudColour[HUD_COLOUR_YELLOW] = CRGBA(180, 25, 29);

    return true;
}
#define fAspectCorrection (*ms_fAspectRatio - ar43)
#define fAspectCorrectionDiv (*ms_fAspectRatio / ar43)
/*DECL_HOOKv(CalculateAspectRatio)
{
    CalculateAspectRatio();
    fAspectCorrection = (*ms_fAspectRatio - ar43);
    fAspectCorrectionDiv = *ms_fAspectRatio / ar43;
}*/
void (*BrightLightsInit)();
void (*BrightLightsRender)();
void (*emu_glEnable)(GLenum);
void (*emu_glDisable)(GLenum);
void (*emu_glAlphaFunc)(GLenum, GLclampf);
bool (*IsOnAMission)();
void (*AddToCheatString)(char);
void (*RwRenderStateSet)(RwRenderState, void*);
void (*RwRenderStateGet)(RwRenderState, void*);
void (*ClearPedWeapons)(CPed*);
eBulletFxType (*GetBulletFx)(void* self, unsigned int surfaceId);
void (*LIB_PointerGetCoordinates)(int, int*, int*, float*);
bool (*Touch_IsDoubleTapped)(WidgetIDs, bool doTapEffect, int idkButBe1);
bool (*Touch_IsHeldDown)(WidgetIDs, int idkButBe1);
void (*SetCameraDirectlyBehindForFollowPed)(CCamera*);
void (*RestoreCamera)(CCamera*);
CVehicle* (*FindPlayerVehicle)(int playerId, bool unk);
CPlayerPed* (*FindPlayerPed)(int playerId);
void (*PhysicalApplyForce)(CPhysical* self, CVector force, CVector point, bool updateTurnSpeed);
char* (*GetFrameNodeName)(RwFrame*);
int (*SpriteCalcScreenCoors)(const RwV3d& posn, RwV3d* out, float* w, float* h, bool checkMaxVisible, bool checkMinVisible);
void (*WorldRemoveEntity)(CEntity*);
void (*SetFontColor)(CRGBA* clr);
bool (*ProcessVerticalLine)(const CVector& origin, float distance, CColPoint& outColPoint, CEntity*& outEntity, bool buildings, bool vehicles, bool peds, bool objects, bool dummies, bool doSeeThroughCheck, CStoredCollPoly* outCollPoly);
void (*CreateEntityRwObject)(CEntity*);
void (*RequestModel)(int id, int prio);
bool (*RemoveLeastUsedModel)(unsigned int streamingFlags);
void (*LoadAllRequestedModels)(bool bOnlyPriorityRequests);
void (*AddAnimBlockRef)(int animBlock);
void (*TimerStop)();
void (*TimerUpdate)();
void (*GetTouchPosition)(CVector2D*, int cachedPosNum);
bool (*StoreStaticShadow)(uint32_t id, uint8_t type, RwTexture* texture, CVector* posn, float frontX, float frontY, float sideX, float sideY, int16_t intensity, uint8_t red, uint8_t green, uint8_t blue, float zDistane, float scale, float drawDistance, bool temporaryShadow, float upDistance);
void (*TransformPoint)(RwV3d& point, const CSimpleTransform& placement, const RwV3d& vecPos);
CAnimBlendAssociation* (*RpAnimBlendClumpGetAssociation)(RpClump*, const char*);
CAnimBlendAssociation* (*RpAnimBlendClumpGetAssociationU)(RpClump*, uint32_t);
CObject* (*CreateObject)(int mdlIdx, bool create);
C2dEffect* (*Get2dEffect)(CBaseModelInfo*, int);
void (*RwFrameForAllObjects)(RwFrame*, RwObject* (*)(RwObject*, void*), void*);
RwObject* (*GetCurrentAtomicObjectCB)(RwObject*, void*);
void (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, void*), void*);
void (*SetComponentAtomicAlpha)(RpAtomic*, int);
void (*ApplyMoveForce)(CPhysical*,float,float,float);
bool (*GetWaterLevel)(CVector, float&, bool, CVector*);
CTask* (*GetTaskSwim)(CPedIntelligence*);
CTask* (*TaskConstructor)();
void (*TaskStartNamedAnim)(CTask*, const char* animName, const char* animGroupName, uint32_t animFlags, float blendDelta, int32_t endTime, bool bDontInterrupt, bool bRunInSequence, bool bOffsetPed, bool bHoldLastFrame);
void (*SetTask)(CTaskManager*, CTask*, int, bool);
bool (*TaskComplexSequenceAddTask)(CTaskComplexSequence*, CTask*);
CAnimBlendAssociation* (*BlendAnimation)(RpClump* clump, AssocGroupId groupId, AnimationId animId, float clumpAssocBlendData);

inline void TransformFromObjectSpace(CEntity* self, CVector& outPos, const CVector& offset)
{
    if(self->m_matrix)
    {
        outPos = *self->m_matrix * offset;
        return;
    }
    TransformPoint((RwV3d&)outPos, self->m_placement, (RwV3d&)offset);
}
inline CVector TransformFromObjectSpace(CEntity* ent, const CVector& offset)
{
    auto result = CVector();
    if (ent->m_matrix) {
        result = *ent->m_matrix * offset;
        return result;
    }

    TransformPoint((RwV3d&)result, ent->m_placement, (RwV3d&)offset);
    return result;
}
inline void BumpStreamingMemory(int megabytes)
{
    *ms_memoryAvailable += megabytes * 1024 * 1024;
}

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////     Hooks     ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
extern "C" void adadad(void)
{
    asm("VMOV S6, S0");
} // This one is used internally by myself. Helps me to get patched values.

// Moon phases
int moon_alphafunc, moon_vertexblend, moon_alphaval;
uintptr_t MoonVisual_1_BackTo;
extern "C" void MoonVisual_1(void)
{
    //emu_glEnable(GL_ALPHA_TEST);

    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &moon_alphafunc);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &moon_vertexblend);
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &moon_alphaval);
    
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    //SET_TO(emu_glAlphaFunc, *(void**)(pGTASA + 0x6BCBF8));
    //emu_glAlphaFunc(GL_GREATER, 0.5f);
}
__attribute__((optnone)) __attribute__((naked)) void MoonVisual_1_Inject(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl MoonVisual_1\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (MoonVisual_1_BackTo));
}
uintptr_t MoonVisual_2_BackTo;
extern "C" void MoonVisual_2(void)
{
    //emu_glAlphaFunc(GL_GREATER, 0.5f);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)moon_alphafunc);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)moon_vertexblend);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)moon_alphaval);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);

    //emu_glDisable(GL_ALPHA_TEST);
}
__attribute__((optnone)) __attribute__((naked)) void MoonVisual_2_Inject(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl MoonVisual_2\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (MoonVisual_2_BackTo));
}

// FOV
DECL_HOOKv(SetFOV, float factor, bool unused)
{
    if(TheCamera->m_WideScreenOn)
    {
        *ms_fFOV = factor;
    }
    else
    {
        SetFOV(factor, unused);
    }
}

// Limit particles
uintptr_t AddBulletImpactFx_BackTo;
unsigned int nextHeavyParticleTick = 0;
eBulletFxType nLimitWithSparkles = BULLETFX_NOTHING;
extern "C" eBulletFxType AddBulletImpactFx(unsigned int surfaceId)
{
    eBulletFxType nParticlesType = GetBulletFx(g_surfaceInfos, surfaceId);
    if(nParticlesType == BULLETFX_SAND || nParticlesType == BULLETFX_DUST)
    {
        if(nextHeavyParticleTick < *m_snTimeInMilliseconds)
        {
            nextHeavyParticleTick = *m_snTimeInMilliseconds + 100;
        }
        else
        {
            return nLimitWithSparkles;
        }
    }
    return nParticlesType;
}
__attribute__((optnone)) __attribute__((naked)) void AddBulletImpactFx_Inject(void)
{
    asm volatile(
        "mov r12, r3\n"
        "push {r0-r7,r9-r11}\n"
        "mov r0, r12\n"
        "bl AddBulletImpactFx\n"
        "mov r8, r0\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r7,r9-r11}\n"
        "mov r9, r1\n"
        "mov r4, r2\n"
        "bx r12\n"
    :: "r" (AddBulletImpactFx_BackTo));
}

// AimingRifleWalkFix
DECL_HOOKv(ControlGunMove, void* self, CVector2D* vec2D)
{
    float save = *ms_fTimeStep; *ms_fTimeStep = fMagic;
    ControlGunMove(self, vec2D);
    *ms_fTimeStep = save;
}

// Water physics fix
// https://github.com/gta-reversed/gta-reversed-modern/blob/163ddc6ab22181004afd57e017618d9e3953a734/source/game_sa/Tasks/TaskTypes/TaskSimpleSwim.cpp#L440
#define SWIMSPEED_FIX
DECL_HOOKv(ProcessSwimmingResistance, CTaskSimpleSwim* task, CPed* ped)
{
    float fSubmergeZ = -1.0f;
    CVector vecPedMoveSpeed{};

    switch (task->m_nSwimState)
    {
        case SWIM_TREAD:
        case SWIM_SPRINT:
        case SWIM_SPRINTING: {
            float fAnimBlendSum = 0.0f;
            float fAnimBlendDifference = 1.0f;

            CAnimBlendAssociation* animSwimBreast = RpAnimBlendClumpGetAssociationU(ped->m_pRwClump, ANIM_ID_SWIM_BREAST);
            if (animSwimBreast) {
                fAnimBlendSum = 0.4f * animSwimBreast->m_fBlendAmount;
                fAnimBlendDifference = 1.0f - animSwimBreast->m_fBlendAmount;
            }

            CAnimBlendAssociation* animSwimCrawl = RpAnimBlendClumpGetAssociationU(ped->m_pRwClump, ANIM_ID_SWIM_CRAWL);
            if (animSwimCrawl) {
                fAnimBlendSum += 0.2f * animSwimCrawl->m_fBlendAmount;
                fAnimBlendDifference -= animSwimCrawl->m_fBlendAmount;
            }
            if (fAnimBlendDifference < 0.0f) {
                fAnimBlendDifference = 0.0f;
            }

            fSubmergeZ = fAnimBlendDifference * 0.55f + fAnimBlendSum;

            vecPedMoveSpeed =  ped->m_vecAnimMovingShiftLocal.x * ped->GetRight();
            vecPedMoveSpeed += ped->m_vecAnimMovingShiftLocal.y * ped->GetForward();
            break;
        }
        case SWIM_DIVE_UNDERWATER: {
            vecPedMoveSpeed =  ped->m_vecAnimMovingShiftLocal.x * ped->GetRight();
            vecPedMoveSpeed += ped->m_vecAnimMovingShiftLocal.y * ped->GetForward();

            auto animSwimDiveUnder = RpAnimBlendClumpGetAssociationU(ped->m_pRwClump, ANIM_ID_SWIM_DIVE_UNDER);
            if (animSwimDiveUnder) {
                vecPedMoveSpeed.z = animSwimDiveUnder->m_fCurrentTime / animSwimDiveUnder->m_pAnimBlendHierarchy->m_fTotalTime * 
                #ifndef SWIMSPEED_FIX
                    -0.1f;
                #else
                    (-0.1f * (*ms_fTimeStep / fMagic));
                #endif
            }
            break;
        }
        case SWIM_UNDERWATER_SPRINTING: {
            vecPedMoveSpeed   =  ped->m_vecAnimMovingShiftLocal.x * ped->GetRight();
            vecPedMoveSpeed   += cosf(task->m_fRotationX) * ped->m_vecAnimMovingShiftLocal.y * ped->GetForward();
            vecPedMoveSpeed.z += (sinf(task->m_fRotationX) * ped->m_vecAnimMovingShiftLocal.y + 0.01f)
            #ifdef SWIMSPEED_FIX
                / (*ms_fTimeStep / fMagic)
            #endif
            ;
            break;
        }
        case SWIM_BACK_TO_SURFACE: {
            auto animClimb = RpAnimBlendClumpGetAssociationU(ped->m_pRwClump, ANIM_ID_CLIMB_JUMP);
            if (!animClimb)
                animClimb = RpAnimBlendClumpGetAssociationU(ped->m_pRwClump, ANIM_ID_SWIM_JUMPOUT);

            if (animClimb) {
                if (animClimb->m_pAnimBlendHierarchy->m_fTotalTime > animClimb->m_fCurrentTime &&
                    (animClimb->m_fBlendAmount >= 1.0f || animClimb->m_fBlendDelta > 0.0f)
                ) {
                    float fMoveForceZ = GetTimeStep() * ped->m_fMass * 0.3f * 0.008f;
                    ApplyMoveForce(ped, 0.0f, 0.0f, fMoveForceZ);
                }
            }
            return;
        }
        default: {
            return;
        }
    }

    float fTheTimeStep = powf(0.9f, GetTimeStep());
    vecPedMoveSpeed *= (1.0f - fTheTimeStep)
    #ifdef SWIMSPEED_FIX
        * (fMagic / *ms_fTimeStep)
    #endif
    ;
    ped->m_vecMoveSpeed *= fTheTimeStep;
    #ifdef SWIMSPEED_FIX
        if(ped->IsPlayer()) vecPedMoveSpeed *= 1.25f;
    #endif
    ped->m_vecMoveSpeed += vecPedMoveSpeed;

    auto& pedPos = ped->GetPosition();
    bool bUpdateRotationX = true;
    CVector vecCheckWaterLevelPos = GetTimeStep() * ped->m_vecMoveSpeed + pedPos;
    float fWaterLevel = 0.0f;
    if (!GetWaterLevel(vecCheckWaterLevelPos, fWaterLevel, true, NULL)) {
        fSubmergeZ = -1.0f;
        bUpdateRotationX = false;
    } else {
        if (task->m_nSwimState != SWIM_UNDERWATER_SPRINTING || task->m_fStateChanger < 0.0f) {
            bUpdateRotationX = false;
        } else {
            if (pedPos.z + 0.65f > fWaterLevel && task->m_fRotationX > 0.7854f) {
                task->m_nSwimState = SWIM_TREAD;
                task->m_fStateChanger = 0.0f;
                bUpdateRotationX = false;
            }
        }
    }

    if (bUpdateRotationX) {
        if (task->m_fRotationX >= 0.0f) {
            if (pedPos.z + 0.65f <= fWaterLevel) {
                if (task->m_fStateChanger <= 0.001f)
                    task->m_fStateChanger = 0.0f;
                else
                    task->m_fStateChanger *= 0.95f;
            } else {
                float fMinimumSpeed = 0.05f * 0.5f;
                if (task->m_fStateChanger > fMinimumSpeed) {
                    task->m_fStateChanger *= 0.95f;
                }
                if (task->m_fStateChanger < fMinimumSpeed) {
                    task->m_fStateChanger += GetTimeStepInSeconds() / 10.0f;
                    task->m_fStateChanger = std::min(fMinimumSpeed, task->m_fStateChanger);
                }
                task->m_fRotationX += GetTimeStep() * task->m_fStateChanger;
                fSubmergeZ = (0.55f - 0.2f) * (task->m_fRotationX * 4.0f / PI) * 0.75f + 0.2f;
            }
        } else {
            if (pedPos.z - sin(task->m_fRotationX) + 0.65f <= fWaterLevel) {
                if (task->m_fStateChanger > 0.001f)
                    task->m_fStateChanger *= 0.95f;
                else
                    task->m_fStateChanger = 0.0f;
            } else {
                task->m_fStateChanger += GetTimeStepInSeconds() / 10.0f;
                task->m_fStateChanger = std::min(task->m_fStateChanger, 0.05f);
            }
            task->m_fRotationX += GetTimeStep() * task->m_fStateChanger;
        }
    }

    if (fSubmergeZ > 0.0f) {
        fWaterLevel -= fSubmergeZ + pedPos.z;
        float fTimeStepMoveSpeedZ = fWaterLevel / GetTimeStep();
        float fTimeStep = GetTimeStep() * 0.1f;
        fTimeStepMoveSpeedZ = std::clamp(fTimeStepMoveSpeedZ, -fTimeStep, fTimeStep);
        fTimeStepMoveSpeedZ -= ped->m_vecMoveSpeed.z;
        fTimeStep = GetTimeStepInSeconds();
        fTimeStepMoveSpeedZ = std::clamp(fTimeStepMoveSpeedZ, -fTimeStep, fTimeStep);
        ped->m_vecMoveSpeed.z += fTimeStepMoveSpeedZ;
    }

    if (pedPos.z < -69.0f) {
        pedPos.z = -69.0f;
        ped->m_vecMoveSpeed.z = std::max(ped->m_vecMoveSpeed.z, 0.0f);
    }
}

uintptr_t ProcessBuoyancy_BackTo;
extern "C" float ProcessBuoyancy_Patch(CPhysical* physical)
{
    if (physical->m_nType == eEntityType::ENTITY_TYPE_PED)
    {
        CPed* ped = (CPed*)physical;
        if (ped->IsPlayer()) // we only need this for player, due to swim bug
        {
            return (1.0f + ((*ms_fTimeStep / fMagic) / 1.5f)) * (*ms_fTimeStep / fMagic);
        }
    }
    return *ms_fTimeStep;
}
__attribute__((optnone)) __attribute__((naked)) void ProcessBuoyancy_Inject(void)
{
    asm volatile(
        "MOV R0, R4\n"
        "BL ProcessBuoyancy_Patch\n"
        "PUSH {R0}\n"
        "VLDR S0, [R4,#0x6C]\n"
        "VLDR S2, [R4,#0xBC]\n"
        "VMUL.F32 S0, S2, S0\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
    :: "r" (ProcessBuoyancy_BackTo));
    asm volatile("VMOV S2, R0\nBX R12\n");
}

// Madd Dogg's Mansion Basketball glitch
DECL_HOOK(ScriptHandle, GenerateNewPickup_MaddDogg, float x, float y, float z, int16_t modelId, ePickupType pickupType, int ammo, int16_t moneyPerDay, bool isEmpty, const char* msg)
{
    if(modelId == 1277 && x == 1263.05f && y == -773.67f && z == 1091.39f)
    {
        return GenerateNewPickup_MaddDogg(1291.2f, -798.0f, 1089.39f, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
    }
    return GenerateNewPickup_MaddDogg(x, y, z, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
}

// Fix Star bribe which is missing minus sign in X coordinate and spawns inside the building
DECL_HOOK(ScriptHandle, GenerateNewPickup_SFBribe, float x, float y, float z, int16_t modelId, ePickupType pickupType, int ammo, int16_t moneyPerDay, bool isEmpty, const char* msg)
{
    if(modelId == 1247 && (int)x == -2120 && (int)y == 96)
    {
        return GenerateNewPickup_SFBribe(2120.0f, y, z, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
    }
    return GenerateNewPickup_SFBribe(x, y, z, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
}

// Fix a Rifle weapon pickup that is located inside the stadium wall since beta
DECL_HOOK(ScriptHandle, GenerateNewPickup_SFRiflePickup, float x, float y, float z, int16_t modelId, ePickupType pickupType, int ammo, int16_t moneyPerDay, bool isEmpty, const char* msg)
{
    if(modelId == 357 && x == -2094.0f && y == -488.0f)
    {
        return GenerateNewPickup_SFRiflePickup(x, -490.2f, z, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
    }
    return GenerateNewPickup_SFRiflePickup(x, y, z, modelId, pickupType, ammo, moneyPerDay, isEmpty, msg);
}

// Do not drop-off jetpack in air
DECL_HOOKv(DropJetPackTask, void* task, CPed* ped)
{
    if(!ped->m_PedFlags.bIsStanding) return;
    DropJetPackTask(task, ped);
}

// Immediately leave the car
DECL_HOOK(bool, CanPedStepOutCar, CVehicle* self, bool bIgnoreSpeedUpright)
{
    float lookupAngle = self->m_matrix->at.z;
    if((lookupAngle <= 0.1f && lookupAngle >= -0.1f) ||
       (self->m_nVehicleType != eVehicleType::VEHICLE_TYPE_BOAT && !bIgnoreSpeedUpright))
    {
        if(fabs(self->m_vecMoveSpeed.z) > 0.05f || self->m_vecMoveSpeed.Magnitude2D() >= 0.01f) return false;
    }
    return true;
}

// Died penalty
uintptr_t DiedPenalty_BackTo;
extern "C" void DiedPenalty(void)
{
    if(WorldPlayers[0].m_nMoney > 0)
    {
        WorldPlayers[0].m_nMoney = (WorldPlayers[0].m_nMoney - 100) < 0 ? 0 : (WorldPlayers[0].m_nMoney - 100);
    }
    ClearPedWeapons(WorldPlayers[0].m_pPed);
}
__attribute__((optnone)) __attribute__((naked)) void DiedPenalty_Inject(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl DiedPenalty\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (DiedPenalty_BackTo));
}

// Emergency Vehicles
uintptr_t EmergencyVeh_BackTo;
__attribute__((optnone)) __attribute__((naked)) void EmergencyVeh_Inject(void)
{
    asm volatile(
        "push {r0}\n");
    asm volatile(
        "vmov s0, %0\n"
    :: "r" (fEmergencyVehiclesFix));
    asm volatile(
        "mov r12, %0\n"
        "pop {r0}\n"
        "bx r12\n"
    :: "r" (EmergencyVeh_BackTo));
}
DECL_HOOKv(SetFOV_Emergency, float factor, bool unused)
{
    // Someone is using broken mods
    // So here is the workaround + a little value clamping
    if(factor < 5.0f)
    {
        fEmergencyVehiclesFix = 70.0f / 5.0f;
    }
    else if(factor > 170.0f)
    {
        fEmergencyVehiclesFix = 70.0f / 170.0f;
    }
    else
    {
        fEmergencyVehiclesFix = 70.0f / factor;
    }
    SetFOV_Emergency(factor, unused);
}

// Marker fix
DECL_HOOKv(PlaceRedMarker_MarkerFix, bool canPlace)
{
    if(canPlace)
    {
        int x, y;
        LIB_PointerGetCoordinates(*lastDevice, &x, &y, NULL);
        if(y > 0.85f * RsGlobal->maximumHeight &&
           x > ((float)RsGlobal->maximumWidth - 0.7f * RsGlobal->maximumHeight)) return;
    }
    PlaceRedMarker_MarkerFix(canPlace);
}

// SkimmerPlaneFix
// Changed the way it works, because ms_fTimeStep cannot be the same at the mod start (it is 0 at the mod start anyway)
uintptr_t SkimmerWaterResistance_BackTo;
extern "C" float SkimmerWaterResistance_Patch(void)
{
    return 30.0f * (*ms_fTimeStep / fMagic);
}
__attribute__((optnone)) __attribute__((naked)) void SkimmerWaterResistance_Inject(void)
{
    asm volatile(
        "vpush {s0-s2}\n"
        "bl SkimmerWaterResistance_Patch\n"
        "vpop {s0-s2}\n"
        "vmov.f32 s4, r0\n");
    asm volatile(
        "mov r12, %0\n"
        "vadd.f32 s2, s2, s8\n"
        "bx r12\n"
    :: "r" (SkimmerWaterResistance_BackTo));
}

// Cinematic camera
bool toggledCinematic = false;
DECL_HOOKv(PlayerInfoProcess_Cinematic, CPlayerInfo* info, int playerNum)
{
    PlayerInfoProcess_Cinematic(info, playerNum);

    // Do it for the local player only.
    if(info == &WorldPlayers[0])
    {
        if(!*bRunningCutscene &&
           info->m_pPed->m_pVehicle != NULL &&
           info->m_pPed->m_nPedState == PEDSTATE_DRIVING)
        {
            if(info->m_pPed->m_pVehicle->m_nVehicleType != VEHICLE_TYPE_TRAIN &&
               Touch_IsDoubleTapped(WIDGETID_CAMERAMODE, true, 1))
            {
                toggledCinematic = !TheCamera->m_bEnabledCinematicCamera;
                TheCamera->m_bEnabledCinematicCamera = toggledCinematic;

                memset(m_pWidgets[WIDGETID_CAMERAMODE]->tapTimes, 0, sizeof(float)*10); // CWidget::ClearTapHistory in a better way
            }
        }
        else
        {
            if(toggledCinematic)
            {
                TheCamera->m_bEnabledCinematicCamera = false;
                *bDidWeProcessAnyCinemaCam = false;
                if(!*bRunningCutscene &&
                   TheCamera->m_pTargetEntity == NULL)
                {
                    RestoreCamera(TheCamera);
                    SetCameraDirectlyBehindForFollowPed(TheCamera);
                }
                m_pWidgets[WIDGETID_CAMERAMODE]->enabled = false;
                toggledCinematic = false;
            }
        }
    }
}

// SWAT
uintptr_t GetCarGunFired_BackTo; // For our usage only
uintptr_t GetCarGunFired_BackTo1, GetCarGunFired_BackTo2; // For optimization?
extern "C" void GetCarGunFired_Patch(void)
{
    CVehicle* veh = FindPlayerVehicle(-1, false);
    if(veh != NULL && (veh->m_nModelIndex == 407 || veh->m_nModelIndex == 601 ||
                       veh->m_nModelIndex == 430))
    {
        GetCarGunFired_BackTo = GetCarGunFired_BackTo1;
    }
    else
    {
        GetCarGunFired_BackTo = GetCarGunFired_BackTo2;
    }
}
__attribute__((optnone)) __attribute__((naked)) void GetCarGunFired_Inject(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl GetCarGunFired_Patch\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (GetCarGunFired_BackTo));
}

// Fuzzy seek (im lazy to patch so lets just do this instead (because we need to inject the code))
DECL_HOOK(int, mpg123_param, void* mh, int key, long val, int ZERO, double fval)
{
    // 0x2000 = MPG123_SKIP_ID3V2
    // 0x200  = MPG123_FUZZY
    // 0x100  = MPG123_SEEKBUFFER
    // 0x40   = MPG123_GAPLESS
    return mpg123_param(mh, key, val | (0x2000 | 0x200 | 0x100 | 0x40), ZERO, fval);
}

// Fix water cannon
DECL_HOOKv(WaterCannonRender, void* self)
{
    float save = *ms_fTimeStep; *ms_fTimeStep = fMagic;
    WaterCannonRender(self);
    *ms_fTimeStep = save;
}
DECL_HOOKv(WaterCannonUpdate, void* self, int frames)
{
    float save = *ms_fTimeStep; *ms_fTimeStep = fMagic;
    WaterCannonUpdate(self, frames);
    *ms_fTimeStep = save;
}

// Moving objs (opcode 034E)
uintptr_t ProcessCommands800To899_BackTo;
extern "C" void ProcessCommands800To899_Patch(void)
{
    float scale = 30.0f / *game_FPS;
    ScriptParams[4].f *= scale;
    ScriptParams[5].f *= scale;
    ScriptParams[6].f *= scale;
}
__attribute__((optnone)) __attribute__((naked)) void ProcessCommands800To899_Inject(void)
{
    asm volatile(
        "mla r9, r1, r2, r0\n"
        "push {r0-r11}\n"
        "vpush {s0-s6}\n"
        "bl ProcessCommands800To899_Patch\n");
    asm volatile(
        "mov r12, %0\n"
        "vpop {s0-s6}\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (ProcessCommands800To899_BackTo));
}

// PhysicalApplyCollision
uintptr_t PhysicalApplyCollision_BackTo;
extern "C" void PhysicalApplyCollision_Patch(CPhysical* self, CVector force, CVector point, bool updateTurnSpeed)
{
    force *= *ms_fTimeStep / fMagic;
    PhysicalApplyForce(self, force, point, updateTurnSpeed);
}
__attribute__((optnone)) __attribute__((naked)) void PhysicalApplyCollision_Inject(void)
{
    asm volatile(
        "mov r0, r9\n"
        "ldr.w r11, [sp,#0xE0-0xAC]\n" // SP fixed
        "bl PhysicalApplyCollision_Patch\n");
    asm volatile(
        "mov r0, %0\n"
        "bx r0\n"
    :: "r" (PhysicalApplyCollision_BackTo));
}

// Car Slowdown Fix
float *mod_HandlingManager_off4;
DECL_HOOKv(ProcessVehicleWheel, CVehicle* self, CVector& wheelFwd, CVector& wheelRight, CVector& wheelContactSpeed, CVector& wheelContactPoint,
        int32_t wheelsOnGround, float thrust, float brake, float adhesion, int8_t wheelId, float* wheelSpeed, void* wheelState, uint16_t wheelStatus)
{
    float save = *mod_HandlingManager_off4; *mod_HandlingManager_off4 = 0.9f * (*ms_fTimeStep / fMagic);
    ProcessVehicleWheel(self, wheelFwd, wheelRight, wheelContactSpeed, wheelContactPoint, wheelsOnGround, thrust, brake, adhesion, wheelId, wheelSpeed, wheelState, wheelStatus);
    *mod_HandlingManager_off4 = save;
}

// Heli rotor
float *fRotorFinalSpeed, *fRotor1Speed, *fRotor2Speed;
DECL_HOOKv(Heli_ProcessFlyingStuff, CHeli* self)
{
    *fRotor1Speed = 0.00454545454f * *fRotorFinalSpeed * (*ms_fTimeStep / fMagic);
    *fRotor2Speed = 3.0f * *fRotor1Speed;
    Heli_ProcessFlyingStuff(self);
}

DECL_HOOKv(PossiblyRemoveVehicle_Re3, CVehicle* veh)
{
    if(veh->vehicleFlags.bIsLocked) return;
    PossiblyRemoveVehicle_Re3(veh);
}

DECL_HOOKv(CloudsUpdate_Re3)
{
    float s = sinf(TheCamera->m_fOrientation - 0.85f);

    *CloudsRotation += *WeatherWind * s * 0.0025f * (*ms_fTimeStep / fMagic);
    *CloudsIndividualRotation += (*WeatherWind * *ms_fTimeStep + 0.3f * (*ms_fTimeStep / fMagic)) * 60.0f;
}

uint32_t* ThreadLaunch_GagNameSet;
DECL_HOOK(void*, OS_ThreadLaunch, void* threadFn, void* a1, uint32_t a2, char const* threadName, int a3, int threadPrio)
{
    bool noName = threadName == NULL || threadName[0] == 0;
    if(noName)
    {
        *ThreadLaunch_GagNameSet = 0xBF00BF00;
    }
    void* ret = OS_ThreadLaunch(threadFn, a1, a2, threadName, a3, threadPrio);
    if(noName)
    {
        *ThreadLaunch_GagNameSet = 0xED86F72A;
    }
    return ret;
}

// Free objects pool
DECL_HOOK(CObject*, Object_New, uint32_t size)
{
    auto objPool = (*pObjectPool);
    auto obj = objPool->New();
    if (!obj)
    {
        int size = objPool->GetSize();
        for (int i = 0; i < size; ++i)
        {
            auto existing = objPool->GetAt(i);
            if (existing && existing->m_nObjectType == OBJECT_TEMPORARY)
            {
                int32_t handle = objPool->GetIndex(existing);
                WorldRemoveEntity(existing);
                delete existing;
                obj = objPool->New(handle);
                return obj;
            }
        }
    }
    return obj;
}

// Colored zone names
uintptr_t ColoredZoneNames_BackTo;
extern "C" void ColoredZoneNames_Patch(void)
{
    CRGBA fontColor((*m_pCurrZoneInfo)->ZoneColor.red, (*m_pCurrZoneInfo)->ZoneColor.green, (*m_pCurrZoneInfo)->ZoneColor.blue, (unsigned char)(*m_ZoneFadeTimer * 0.255f));
    SetFontColor(&fontColor);
}
__attribute__((optnone)) __attribute__((naked)) void ColoredZoneNames_Inject(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl ColoredZoneNames_Patch\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (ColoredZoneNames_BackTo));
}

// Find Ground Z detects objects
#define FINDGROUNDZ_DIST 2000
#define NOTFOUND_RETURN 20.0f
DECL_HOOK(float, FindGroundZ2D, float x, float y)
{
    CColPoint colp;
    CEntity* hitEnt;
    return ProcessVerticalLine(CVector(x, y, (float)(FINDGROUNDZ_DIST)), -(float)(FINDGROUNDZ_DIST), colp, hitEnt, true, false, false,
           (*bRunningCutscene || TheCamera->m_WideScreenOn || TheCamera->m_pTargetEntity != NULL || !TheCamera->m_vecGameCamPosFixed.IsZero()) ? false : true, true, false, NULL) ? colp.m_vecPoint.z : NOTFOUND_RETURN;
}
DECL_HOOK(float, FindGroundZ3D, float x, float y, float z, bool* result, CEntity** ent)
{
    if(z <= 0)
    {
        if(result) *result = false;
        if(ent) *ent = NULL;
        return 0.0f;
    }
    CColPoint colp;
    CEntity* hitEnt;
    if(ProcessVerticalLine(CVector(x, y, z), z > FINDGROUNDZ_DIST ? -(float)(FINDGROUNDZ_DIST) : -z, colp, hitEnt, true, false, false, 
      (*bRunningCutscene || TheCamera->m_WideScreenOn || TheCamera->m_pTargetEntity != NULL || !TheCamera->m_vecGameCamPosFixed.IsZero()) ? false : true, true, false, NULL))
    {
        if(result) *result = true;
        if(ent) *ent = hitEnt;
        return colp.m_vecPoint.z;
    }
    else
    {
        if(result) *result = false;
        if(ent) *ent = NULL;
        return 0.0f;
    }
}

// Road Reflections
uintptr_t RoadReflections_BackTo;
__attribute__((optnone)) __attribute__((naked)) void RoadReflections_Inject(void)
{
    asm volatile(
        "vmul.f32 s0, s0, s28\n"
        "vmul.f32 s17, s2, s0\n"
        "vdiv.f32 s2, s17, s16\n"
        "ldr r0, [sp, #0x128-0x74+0x8]\n" // SP fixed
    );
    asm volatile(
        "push {r0}\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0}\n"
        "bx r12\n"
    :: "r" (RoadReflections_BackTo));
}

// Heli rotor blur
// https://gtaforums.com/topic/703439-san-andreas-moon-and-rotor-blades-fix-for-pc/
// Doesnt work :)
// peepo found a way. So, actually, R* disabled it. LOL
#define SMOOTHING_PERCENT 0.3f
#define SMOOTHED_VALUE(_VAL)    (_VAL<SMOOTHING_PERCENT ? 0 : (_VAL / (1.01f - SMOOTHING_PERCENT)))
uint16_t nRotorMdlIgnore;
uintptr_t RotorBlurRender_BackTo1, RotorBlurRender_BackTo2;
extern "C" void RotorBlurRender_Patch(RpAtomic* atomic, CAutomobile* ent)
{
    if(!atomic) return;
    if(ent->m_nModelIndex == nRotorMdlIgnore) SetComponentAtomicAlpha(atomic, 0);
    else
    {
        int rotorVal;
        if(ent->m_nVehicleType == VEHICLE_TYPE_PLANE)
            rotorVal = 255.0f * SMOOTHED_VALUE(((CPlane*)ent)->m_fEngineSpeed / 0.2f);
        else
            rotorVal = 255.0f * SMOOTHED_VALUE(((CHeli*)ent)->m_aWheelAngularVelocity[1] / 0.220484f);
            
        if(rotorVal > 255) rotorVal = 255;
        else if(rotorVal < 0) rotorVal = 0;
        SetComponentAtomicAlpha(atomic, rotorVal);
    }
}
__attribute__((optnone)) __attribute__((naked)) void RotorBlurRender_Inject1(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0]\n"
        "LDR R1, [SP, #16]\n"
        "BL RotorBlurRender_Patch\n"
        "POP {R0-R11}\n"
    );
    asm volatile(
        "PUSH {R0}\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (RotorBlurRender_BackTo1));
}
__attribute__((optnone)) __attribute__((naked)) void RotorBlurRender_Inject2(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0]\n"
        "LDR R1, [SP, #16]\n"
        "BL RotorBlurRender_Patch\n"
        "POP {R0-R11}\n"
    );
    asm volatile(
        "PUSH {R0}\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (RotorBlurRender_BackTo2));
}
DECL_HOOKv(HeliRender, CHeli* self)
{
    HeliRender(self);
}
DECL_HOOKv(PlaneRender, CPlane* self)
{
    HeliRender((CHeli*)self);
}

// LODs
#define SKIP_START_FRAMES 3
#define ANIMBLOCK_OFFSET  25575

uintptr_t LoadScene_BackTo, InitPools_BackTo, InitPools2_BackTo;
std::vector<CEntity*> g_aLODs;
std::vector<int> g_aPeds;
bool bPreloadLOD, bPreloadAnim, bPreloadPed = false;
bool bUnloadUnusedModels, bDynStreamingMem, bDontUnloadInCutscenes;
float fRemoveUnusedStreamMemPercentage, fDynamicStreamingMemPercentage; int nRemoveUnusedInterval; unsigned int lastTimeRemoveUnused = 0;
DECL_HOOKv(GameProcess)
{
    GameProcess();

    static char nLoadChecks = SKIP_START_FRAMES+1;
    if(nLoadChecks != 0) // Ignore first SKIP_START_FRAMES frames
    {
        --nLoadChecks;
        if(nLoadChecks == 1)
        {
            // Dont break some engine calculations because of that
            TimerStop();

            if(bPreloadLOD)
            {
                int size = g_aLODs.size();
                CEntity* ent = NULL;
                RwObject* obj = NULL;
                for(int i = 0; i < size; ++i)
                {
                    ent = g_aLODs[i];
                    obj = ent->m_pRwObject;

                    RequestModel(ent->m_nModelIndex, STREAMING_MISSION_REQUIRED);
                    if(obj == NULL) CreateEntityRwObject(g_aLODs[i]);
                }
                LoadAllRequestedModels(false);
            }
            if(bPreloadAnim)
            {
                for(int i = 1; i < *ms_numAnimBlocks; ++i)
                {
                    RequestModel(i + ANIMBLOCK_OFFSET, STREAMING_MISSION_REQUIRED);
                    AddAnimBlockRef(i);
                }
                LoadAllRequestedModels(false);
            }
            if(bPreloadPed) // Crashing, maybe in the future sometime
            {
                int size = g_aPeds.size();
                for(uint16_t i = 0; i < size; ++i)
                {
                    // The game doesnt want to load models 290-299?
                    if(g_aPeds[i] > 289) continue;
                    RequestModel(g_aPeds[i], STREAMING_MISSION_REQUIRED);
                }
                LoadAllRequestedModels(false);
            }

            // Resume all game timers
            TimerUpdate();
            nLoadChecks = 0;
        }
        return;
    }
    else if(bUnloadUnusedModels || bDynStreamingMem)
    {
        float memUsedPercent = (float)*ms_memoryUsed / (float)*ms_memoryAvailable;
        if(!bDontUnloadInCutscenes || !*bRunningCutscene)
        {
            if(bUnloadUnusedModels && memUsedPercent >= fRemoveUnusedStreamMemPercentage)
            {
                int removeUnusedIntervalMsTweaked;
                if (memUsedPercent >= 0.95f) removeUnusedIntervalMsTweaked = (int)(nRemoveUnusedInterval * 0.5f);
                else removeUnusedIntervalMsTweaked = nRemoveUnusedInterval;

                if ((*m_snTimeInMilliseconds - lastTimeRemoveUnused) > removeUnusedIntervalMsTweaked)
                {
                    RemoveLeastUsedModel(STREAMING_NONE);
                    lastTimeRemoveUnused = *m_snTimeInMilliseconds;
                }
            }
        }
        if(bDynStreamingMem && memUsedPercent >= fDynamicStreamingMemPercentage)
        {
            BumpStreamingMemory(32);
        }
    }
}
DECL_HOOK(CBaseModelInfo*, AddPedModel, int id)
{
    g_aPeds.push_back(id);
    return AddPedModel(id);
}
/* FROM MY PROJECT ReSAMP */
struct CSAPool
{
    void*     objects;
    uint8_t*  flags;
    uint32_t  count;
    uint32_t  top;
    uint32_t  initialized;
};
static CSAPool* AllocatePool(size_t count, size_t size)
{
    CSAPool *p = new CSAPool;
    p->objects = new char[size*count];
    p->flags = new uint8_t[count];
    p->count = count;
    p->top = 0xFFFFFFFF;
    p->initialized = 1;
    for (size_t i = 0; i < count; ++i)
    {
        p->flags[i] |= 0x80;
        p->flags[i] &= 0x80;
    }
    return p;
}
/* FROM MY PROJECT ReSAMP */
extern "C" void InitPools_Patch()
{
    *(CSAPool**)(aml->GetSym(hGTASA, "_ZN6CPools16ms_pColModelPoolE")) = AllocatePool(50000,  0x30);
}
__attribute__((optnone)) __attribute__((naked)) void InitPools_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "BL InitPools_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (InitPools_BackTo));
}
extern "C" void InitPools2_Patch()
{
    *(CSAPool**)(aml->GetSym(hGTASA, "_ZN6CPools15ms_pVehiclePoolE")) =AllocatePool(2000,  2604);
}
__attribute__((optnone)) __attribute__((naked)) void InitPools2_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "BL InitPools2_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (InitPools2_BackTo));
}
extern "C" void LoadScene_Patch(CEntity* ent)
{
    g_aLODs.push_back(ent);
}
__attribute__((optnone)) __attribute__((naked)) void LoadScene_Inject(void)
{
    asm volatile(
        "LDRSH.W R2, [R6,#0x26]\n"
        "LDRB.W R1, [R0,#0x38]\n"
        "MOV R12, R6\n"
        "push {r0-r10}\n"
        "MOV R0, R12\n"
        "bl LoadScene_Patch\n");
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r10}\n"
        "LDR.W R11, [R9,R2,LSL#2]\n"
        "bx r12\n"
    :: "r" (LoadScene_BackTo));
}

// Colorpicker
typedef bool (*IsWTouched)(CWidget*);
DECL_HOOK(float, GetColorPickerValue, CWidgetRegionColorPicker* self)
{
    static float prevVal = 0.0f;
    if((*(IsWTouched*)(self->vtable + 80))(self) != false) // IsTouched
    {
        CVector2D v; GetTouchPosition(&v, self->cachedPosNum);
        
        float left = self->screenRect.left * 0.7f;
        float right = self->screenRect.right * 0.7f;

        float bottom = self->screenRect.bottom * 0.8f;
        float top = self->screenRect.top * 0.8f;

        if(v.x < left || v.x > right || v.y < top || v.y > bottom) return prevVal;

        float ret = (int)(
            8.0f * ((v.x - left) / (right - left)) + 8 * (int)(
            8.0f * ((v.y - top) / (bottom - top))) );
        prevVal = ret;

        return ret;
    }
    return 0.0f;
}

// Light shadows from poles
uintptr_t ProcessLightsForEntity_BackTo;
float fLightDist = 40.0f;
CVector vecEffCenterTmp;
extern "C" void ProcessLightsForEntity_Patch(CEntity* ent, C2dEffect* eff, int effectNum, int bDoLight, CVector vecEffPos)
{
    if(bDoLight && eff->light.m_fShadowSize != 0)
    {
        //logger->Info("eff pos %f %f %f", vecEffCenterTmp.x, vecEffCenterTmp.y, vecEffCenterTmp.z);
        /*if(ent->m_nModelIndex == 1226)
        {
            // But why does it shift a bit while closer than 40.0F units?
            vecEffCenterTmp = ent->GetPosition();
        }
        else*/
        {
            //vecEffCenterTmp = TransformFromObjectSpace(ent, eff->m_vecPosn);
        }
        
        //vecEffCenterTmp = *vecEffPos;
        
        float intensity = ((float)eff->light.m_nShadowColorMultiplier / 255.0f) * 0.1f * *fSpriteBrightness;
        float zDist = eff->light.m_nShadowZDistance ? eff->light.m_nShadowZDistance : 15.0f;
        StoreStaticShadow((uint32_t)ent + effectNum, 2, eff->light.m_pShadowTex, &vecEffCenterTmp, eff->light.m_fShadowSize, 0.0f, 0.0f, -eff->light.m_fShadowSize,
                           128, intensity * eff->light.m_color.red, intensity * eff->light.m_color.green, intensity * eff->light.m_color.blue, zDist, 1.0f, fLightDist, false, 0.0f);
    }
}
__attribute__((optnone)) __attribute__((naked)) void ProcessLightsForEntity_Inject(void)
{
    asm volatile(
        "MOV R0, R9\n"
        
        //"ADD R12, SP, #0x190-0x90\n"
        
        "PUSH {R1-R11}\n"
        "LDR R1, [SP, #28]\n"
        "LDR R2, [SP, #20]\n"
        "LDR R3, [SP, #12]\n"
        //"LDR R4, [SP, #16]\n"
        //"ADD R4, SP, #0xC0+44\n"
        
        //"LDMIA R12, {R4-R6}\n"
        
        //"ADD R4, SP, #4\n"
        "BL ProcessLightsForEntity_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R1-R11}\n"
        "LDR.W R10, [SP,#0x150-0x98]\n"
        "BX R12\n"
    :: "r" (ProcessLightsForEntity_BackTo));
}
DECL_HOOKv(AddLight, unsigned char a1, CVector a2, CVector a3, float a4, float a5, float a6, float a7, unsigned char a8, bool a9, CEntity* a10)
{
    // This is going to be a workaround
    // because i dont freakin understand
    // why does this stupid TransformPoint
    // is failed and a position in stack
    // is wrong, like 1 million Z. WTF
    vecEffCenterTmp = a2;
    AddLight(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

// Green-ish detail texture
#define GREEN_TEXTURE_ID 14
inline void* GetDetailTexturePtr(int texId)
{
    return *(void**)(**(int**)(*detailTexturesStorage + 4 * (texId-1)) + *RasterExtOffset);
}

DECL_HOOKv(emu_TextureSetDetailTexture, void* texture, unsigned int tilingScale)
{
    if(texture == NULL)
    {
        emu_TextureSetDetailTexture(NULL, 0);
        return;
    }
    if(texture == GetDetailTexturePtr(GREEN_TEXTURE_ID))
    {
        *textureDetail = 0;
        emu_TextureSetDetailTexture(NULL, 0);
        return;
    }
    emu_TextureSetDetailTexture(texture, tilingScale);
    *textureDetail = 1;
}

// Static shadows
DECL_HOOKv(RenderStaticShadows, bool a1)
{
    for(int i = 0; i < 0xFF; ++i)
    {
        aStaticShadows_NEW[i].m_bRendered = false;
    }
    RenderStaticShadows(a1);
}
DECL_HOOKv(InitShadows)
{
    static CPolyBunch bunchezTail[1024];
    
    InitShadows();
    for(int i = 0; i < 1023; ++i)
    {
        bunchezTail[i].pNext = &bunchezTail[i+1];
    }
    bunchezTail[1023].pNext = NULL;
    aPolyBunches[360-1].pNext = &bunchezTail[0];
}

// ClimbDie
DECL_HOOK(bool, ClimbProcessPed, CTask* self, CPed* target)
{
    float save = *ms_fTimeStep; *ms_fTimeStep = fMagic;
    bool ret = ClimbProcessPed(self, target);
    *ms_fTimeStep = save;
    return ret;
}

// Driving school bikes
DECL_HOOK(int, CreateCarGenerator, float x, float y, float z, float angle, int32_t modelId, int16_t color1, int16_t color2, uint8_t forceSpawn, uint8_t alarmChances,
                                   uint8_t doorLockChances, uint16_t minDelay, uint16_t maxDelay, uint8_t iplId, bool ignorePopulationLimit)
{
    if((modelId == 463 || modelId == 521 || modelId == 522) && z == 10.1203f && (int)x == 1174)
    {
        if(y == 1364.832f)
            return CreateCarGenerator(1175.76, y, z, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, iplId, ignorePopulationLimit);
        if(y == 1366.479f)
            return CreateCarGenerator(1175.999, 1368.4, z, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, iplId, ignorePopulationLimit);
        if(y == 1368.359f)
            return CreateCarGenerator(1175.967, 1372.0, z, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, iplId, ignorePopulationLimit);
    }
    return CreateCarGenerator(x, y, z, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, iplId, ignorePopulationLimit);
}

// Fixing a crosshair by very stupid math
float fWideScreenWidthScale, fWideScreenHeightScale;
DECL_HOOKv(DrawCrosshair)
{
    static constexpr float XSVal = 1024.0f / 1920.0f; // prev. 0.530, now it's 0.533333..3
    static constexpr float YSVal = 768.0f / 1920.0f; // unchanged :p

    CPlayerPed* player = WorldPlayers[0].m_pPed;
    if(player->m_Weapons[player->m_byteCurrentWeaponSlot].m_nType == WEAPON_COUNTRYRIFLE)
    {
        // Weirdo logic but ok
        float save1 = *m_f3rdPersonCHairMultX; *m_f3rdPersonCHairMultX = 0.530f - 0.84f * ar43 * 0.01115f; // 0.01125f;
        float save2 = *m_f3rdPersonCHairMultY; *m_f3rdPersonCHairMultY = 0.400f + 0.84f * ar43 * 0.038f; // 0.03600f;
        DrawCrosshair();
        *m_f3rdPersonCHairMultX = save1; *m_f3rdPersonCHairMultY = save2;
        return;
    }

    float save1 = *m_f3rdPersonCHairMultX; *m_f3rdPersonCHairMultX = 0.530f - fAspectCorrection * 0.01115f; // 0.01125f;
    float save2 = *m_f3rdPersonCHairMultY; *m_f3rdPersonCHairMultY = 0.400f + fAspectCorrection * 0.038f; // 0.03600f;
    DrawCrosshair();
    *m_f3rdPersonCHairMultX = save1; *m_f3rdPersonCHairMultY = save2;
}

DECL_HOOKv(CalculateAspectRatio_CrosshairFix)
{
    CalculateAspectRatio_CrosshairFix();

    fWideScreenWidthScale = 640.0f / (*ms_fAspectRatio * 448.0f);
    fWideScreenHeightScale = 448.0 / 448.0f;
}

// Cheats!
DECL_HOOKv(DoCheats, int self, int keyNum)
{
    // Suppress
    //DoCheats(self, keyNum);
}
DECL_HOOKv(KBEvent, bool pushed, int keyNum, int ctrl_or_shift, int alwaysZero)
{
    KBEvent(pushed, keyNum, ctrl_or_shift, alwaysZero);
    
    char key = 0;
    
    if(keyNum >= 17 && keyNum <= 26) key = keyNum + 31; // numbas
    else if(keyNum >= 27 && keyNum <= 52) key = keyNum + 70; // symbalz
    
    if(!pushed && key != 0) AddToCheatString(toupper(key));
}

// Fix crash while loading the save file
DECL_HOOKv(EntMdlNoCreate, CEntity *self, uint32_t mdlIdx)
{
    RequestModel(mdlIdx, STREAMING_PRIORITY_REQUEST);
    LoadAllRequestedModels(true);
    EntMdlNoCreate(self, mdlIdx);
}

// Taxi lights
void (*SetTaxiLight)(CAutomobile*, bool);
CAutomobile* pLastPlayerTaxi = NULL;
DECL_HOOKv(AutomobileRender, CAutomobile* self)
{
    AutomobileRender(self);
    
    if(self->m_nModelIndex == 420 ||
       self->m_nModelIndex == 438)
    {
        CPlayerPed* p = FindPlayerPed(-1);
        
        if(self->vehicleFlags.bEngineOn &&
           self->m_pDriver &&
           self->m_nNumPassengers == 0 &&
           self->m_fHealth > 0)
        {
            SetTaxiLight(self, true);
        }
        else
        {
            SetTaxiLight(self, false);
        }
    }
}

// Interior radar
DECL_HOOKv(DrawRadar, void* self)
{
    CPlayerPed* p = FindPlayerPed(-1);
    if(!p || p->m_nInterior == 0 || IsOnAMission())
        DrawRadar(self);
}

// I got the moneeeey
uintptr_t DrawMoney_BackTo;
extern "C" const char* DrawMoney_Patch(int isPositive)
{
    static const char* positiveT = "$%08d";
    static const char* negativeT = "-$%07d";
    
    return isPositive ? positiveT : negativeT;
}
__attribute__((optnone)) __attribute__((naked)) void DrawMoney_Inject(void)
{
    asm volatile(
        "LDR R5, [R0]\n"
        "SMLABB R0, R1, R11, R5\n"
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #8]\n"
        "BL DrawMoney_Patch\n"
        "STR R0, [SP, #4]\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (DrawMoney_BackTo));
}

// Hud colours
DECL_HOOK(int, GetIntColour, void*, eHudColours i)
{
    CRGBA& clr = OwnHudColour[i];
    return ((clr.b) | 0xFF) + (((clr.g) << 16) | 0xFF) + (((clr.r) << 24) | 0xFF);
}
DECL_HOOK(CRGBA, GetRGBA, CRGBA& ret, void* self, eHudColours i) // how the arguments look like that?!
{
    return (ret = OwnHudColour[i]);
}

// Para dmg anim fix
DECL_HOOKv(ComputeDamageAnim, uintptr_t self, CPed* victim, bool a2)
{
    bool bNeedFix = *(eWeaponType*)(self + 24) == WEAPON_PARACHUTE;
    
    if(bNeedFix) *(eWeaponType*)(self + 24) = WEAPON_UNARMED;
    ComputeDamageAnim(self, victim, a2);
    if(bNeedFix) *(eWeaponType*)(self + 24) = WEAPON_PARACHUTE;
}

// MixSets-SA: HostileGangs
bool bFakeMission = false, bSaveStat;
DECL_HOOKv(ProcessPedGroups)
{
    bFakeMission = true;
    bSaveStat = *ms_bIsPlayerOnAMission;
    *ms_bIsPlayerOnAMission = false;
    ProcessPedGroups();
    *ms_bIsPlayerOnAMission = bSaveStat;
    bFakeMission = false;
}
DECL_HOOK(bool, PedGroups_IsOnAMission)
{
    return bFakeMission || PedGroups_IsOnAMission();
}

// Peepo: Fix traffic lights
DECL_HOOKv(TrFix_RenderEffects)
{
    TrFix_RenderEffects();
    BrightLightsRender();
}
DECL_HOOKv(TrFix_InitGame2nd, const char* a1)
{
    TrFix_InitGame2nd(a1);
    BrightLightsInit();
}

// Food Eating Fix
DECL_HOOKv(PlayerInfoProcess_Food, CPlayerInfo* info, int playerNum)
{
    PlayerInfoProcess_Food(info, playerNum);
    if(IsOnAMission()) return;

    RpClump* pedcl = info->m_pPed->m_pRwClump;
    CAnimBlendAssociation* assoc;
    static bool active = false;
    static CObject* obj;
    int mdlIdx = 2880;
    
    if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "VEND_EAT_P")) != NULL)
    {
      success_eating:
        float fProgress = assoc->m_fCurrentTime / assoc->m_pAnimBlendHierarchy->m_fTotalTime;
        if(fProgress <= 0.8f && !active)
        {
            active = true;
            
            obj = CreateObject(mdlIdx, true);
            obj->GetPosition() = info->m_pPed->GetPosition();
        }
        else if(fProgress > 0.8f && active) goto end_food_anim;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "VEND_EAT1_P")) != NULL)
    {
        
        goto success_eating;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "VEND_Drink_P")) != NULL)
    {
        
        goto success_eating;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "VEND_Drink1_P")) != NULL)
    {
        
        goto success_eating;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "EAT_Burger")) != NULL)
    {
        
        goto success_eating;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "EAT_Chicken")) != NULL)
    {
        
        goto success_eating;
    }
    else if((assoc = RpAnimBlendClumpGetAssociation(pedcl, "EAT_Pizza")) != NULL)
    {
        
        goto success_eating;
    }
    else
    {
        if(active)
        {
          end_food_anim:
            active = false;
            
            
        }
    }
}

// Max loading splashes
int DoRand(int max)
{
    srand(time(NULL));
    return (int)(((double)rand() / (double)RAND_MAX) * max);
}
uintptr_t LoadSplashes_BackTo;
char mobilescDone[16];
int mobilescCount = 7;
extern "C" void LoadSplashes_Patch(void)
{
    snprintf(mobilescDone, sizeof(mobilescDone), "mobilesc%d", DoRand(mobilescCount));
}
__attribute__((optnone)) __attribute__((naked)) void LoadSplashes_Inject(void)
{
    asm volatile(
        //"ADD R5, SP, 0x58+0x40\n"
        "ADDS R0, R4, #4\n"
        "PUSH {R0,R2-R11}\n"
        "BL LoadSplashes_Patch\n");
    asm volatile(
        "MOV R1, %0\n"
    :: "r" (&mobilescDone));
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0,R2-R11}\n"
        "BX R12\n"
    :: "r" (LoadSplashes_BackTo));
}

// Peds count calculating is so much wrong for gangs!
// This glitch is resulting in a very rare gang members appearing
uintptr_t PedCountCalc_BackTo1, PedCountCalc_BackTo2;
extern "C" int PedCountCalc_Patch1(uint32_t pedType)
{
    return --ms_nNumGang[pedType - PED_TYPE_GANG1];
}
extern "C" int PedCountCalc_Patch2(uint32_t pedType)
{
    return ++ms_nNumGang[pedType - PED_TYPE_GANG1];
}
__attribute__((optnone)) __attribute__((naked)) void PedCountCalc_Inject1(void)
{
    asm volatile(
        "BL PedCountCalc_Patch1\n"
        "PUSH {R0}\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (PedCountCalc_BackTo1));
}
__attribute__((optnone)) __attribute__((naked)) void PedCountCalc_Inject2(void)
{
    asm volatile(
        "BL PedCountCalc_Patch2\n"
        "PUSH {R0}\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (PedCountCalc_BackTo2));
}

// Force DXT
DECL_HOOKv(LoadTexDBThumbs, const char* dbName, int unk, TextureDatabaseFormat format)
{
    if(format == DF_Default)
    {
        LoadTexDBThumbs(dbName, unk, DF_DXT);
    }
    else
    {
        LoadTexDBThumbs(dbName, unk, format);
    }
}

// SunGlare
void (*DoSunGlare)(CVehicle*);
DECL_HOOKv(RenderVehicle_SunGlare, CVehicle* self)
{
    RenderVehicle_SunGlare(self);
    DoSunGlare(self);
}

// Smaller grenade collision
#define GRENADE_NEW_RADIUS 0.18f
CColModel colGrenade, *ms_colModelWeapon;
uintptr_t LoadWeaponObject_BackTo;
void (*ColModelAllocateData)(CColModel*, int, int, int, int, int, bool);
void (*ColSphereSet)(CColSphere*, float, CVector&, uint8_t, uint8_t, uint8_t);
DECL_HOOKv(InitTempColModels)
{
    InitTempColModels();

    colGrenade.m_pColData = NULL;
    colGrenade.m_level = 0;
    colGrenade.m_hasCollisionVolumes = 0;
    colGrenade.m_useSingleAlloc = 0;
    colGrenade.m_deleteUncompressed = 0;

    colGrenade.m_boxBound.m_vecMin = CVector(-GRENADE_NEW_RADIUS, -GRENADE_NEW_RADIUS, -GRENADE_NEW_RADIUS);
    colGrenade.m_boxBound.m_vecMax = CVector( GRENADE_NEW_RADIUS,  GRENADE_NEW_RADIUS,  GRENADE_NEW_RADIUS);
    colGrenade.m_sphereBound.m_vecCenter = CVector(0, 0, 0);
    colGrenade.m_sphereBound.m_flRadius = GRENADE_NEW_RADIUS;

    ColModelAllocateData(&colGrenade, 1, 0, 0, 0, 0, false);
    ColSphereSet(&colGrenade.m_pColData->m_pSphereArray[0], colGrenade.m_sphereBound.m_flRadius * 0.75f, colGrenade.m_sphereBound.m_vecCenter, 56, 0, 255);
}
extern "C" CColModel* LoadWeaponObject_Patch(int mdlId)
{
    switch(mdlId)
    {
        case 342:
        case 343:
        case 344:
        case 363:
            return &colGrenade;

        default:
            return ms_colModelWeapon;
    }
}
__attribute__((optnone)) __attribute__((naked)) void LoadWeaponObject_Inject(void)
{
    asm volatile(
        "LDR R0, [SP,#0x1C]\n"
        //"PUSH {R3-R11}\n"
        "BL LoadWeaponObject_Patch\n"
        "MOV R1, R0\n");
    asm volatile(
        "MOV R12, %0\n"
        //"POP {R3-R11}\n"
        "MOVS R2, #0\n"
        "MOV R0, R6\n"
        "BX R12\n"
    :: "r" (LoadWeaponObject_BackTo));
}

// Slippery floor!
uintptr_t IceFloor_BackTo1, IceFloor_BackTo2;
extern "C" uintptr_t IceFloor_Patch(CPed* p)
{
    if(p->IsDead())
    {
        CPhysical* groundphy = p->m_pGroundPhysical;
        if(!(!groundphy || groundphy->m_pAttachedTo || (groundphy->m_nPhysicalFlags & 0xC) == 4))
        {
            return IceFloor_BackTo2; // continue code execution
        }
    }
    return IceFloor_BackTo1; // skip the code
}
__attribute__((optnone)) __attribute__((naked)) void IceFloor_Inject(void)
{
    asm volatile(
        "PUSH {R0-R3, R5-R11}\n"
        "VPUSH {S0}\n"
        "MOV R0, R4\n"
        "BL IceFloor_Patch\n");
    asm volatile(
        "MOV R12, R0\n"
        "VPOP {S0}\n"
        "POP {R0-R3, R5-R11}\n"
        "BX R12\n");
}

// Can now use a gun!
float* float_4DD9E8;
DECL_HOOKv(TaskSimpleUseGunSetMoveAnim, CTask* task, CPed* ped)
{
    *float_4DD9E8 = (fMagic) * (0.1f / *ms_fTimeStep);
    TaskSimpleUseGunSetMoveAnim(task, ped);
}

// Spread fix
float *fPlayerAimRotRate;
DECL_HOOK(bool, FireInstantHit, CWeapon *self, CEntity *a2, CVector *a3, CVector *a4, CEntity *a5, CVector *a6, CVector *a7, int a8, int a9)
{
    *fPlayerAimRotRate = (rand() * 2.0f * M_PI) / (float)RAND_MAX;
    return FireInstantHit(self, a2, a3, a4, a5, a6, a7, a8, a9);
}

// IS_CHAR_DEAD fix
DECL_HOOK(bool, RunningScript_IsPedDead, CRunningScript* script, CPed* ped)
{
    if(ped->m_fHealth <= 0.0f && ped->m_Weapons[ped->m_byteCurrentWeaponSlot].m_nType != eWeaponType::WEAPON_PARACHUTE) return true;
    return RunningScript_IsPedDead(script, ped);
}

// Sprint button after aiming and dropping to the water
DECL_HOOK(bool, IsTargetingActiveForPlayer, CCamera* self, CPlayerPed* p)
{
    if(p)
    {
        if(GetTaskSwim(p->m_pIntelligence)) return false; // Fixed part
        if(p->m_pTarget || p->m_pPlayerData->m_bFreeAiming) return true;
    }
    switch(self->PlayerWeaponMode.Mode)
    {
        case 0x07:
        case 0x08:
        case 0x22:
        case 0x2D:
        case 0x2E:
        case 0x33:
        case 0x41:
            return true;
    }
    return false;
}
DECL_HOOK(bool, IsTargetingActive, CCamera* self)
{
    CPlayerPed* p = FindPlayerPed(-1);
    return HookOf_IsTargetingActiveForPlayer(self, p);
}

// ParaLand Anim fix
DECL_HOOKv(PlayerInfoProcess_ParachuteAnim, CPlayerInfo* self, int playerNum)
{
    CPed* ped = self->m_pPed;
    if(ped->m_Weapons[ped->m_byteCurrentWeaponSlot].m_nType == eWeaponType::WEAPON_PARACHUTE)
    {
        auto anim = RpAnimBlendClumpGetAssociation(ped->m_pRwClump, "FALL_front");
        if(anim != NULL)
        {
            CTask* task = TaskConstructor();
            TaskStartNamedAnim(task, "PARA_Land", "PARACHUTE", 0x08 | 0x10 | 0x40 | 0x80, 6.0f, -1, true, *ms_iActiveSequence > -1, false, false);
            SetTask(&ped->m_pIntelligence->m_TaskMgr, task, TASK_PRIMARY_EVENT_RESPONSE_TEMP, false);
        }
    }
    PlayerInfoProcess_ParachuteAnim(self, playerNum);
}

// Boat radio animation
uintptr_t BoatRadio_BackTo;
extern "C" CAnimBlendAssociation* BoatRadio_Patch(RpClump* p)
{
    if(RpAnimBlendClumpGetAssociationU(p, ANIM_ID_DRIVE_BOAT)) return NULL;
    return BlendAnimation(p, ANIM_GROUP_DEFAULT, ANIM_ID_CAR_TUNE_RADIO, 4.0);
}
__attribute__((optnone)) __attribute__((naked)) void BoatRadio_Inject(void)
{
    asm volatile(
        "BL BoatRadio_Patch\n"
        "PUSH {R0}\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0}\n"
        "BX R12\n"
    :: "r" (BoatRadio_BackTo));
}

// MixSets
uintptr_t DuckAnyWeapon_BackTo1, DuckAnyWeapon_BackTo2;
extern "C" uintptr_t DuckAnyWeapon_Patch(CWeaponInfo* info, CPed* ped)
{
    if(info->m_nWeaponFire == WEAPON_FIRE_MELEE ||
       /*info->m_nWeaponFire == WEAPON_FIRE_USE ||
       (ped->m_Weapons[ped->m_byteCurrentWeaponSlot].m_nType | 2) == 43 ||
       (info->bCrouchFire ||*/ // original code
       info->m_eAnimGroup != ANIM_GROUP_FLAME || info->m_eAnimGroup != ANIM_GROUP_ROCKET) // MIX
        return DuckAnyWeapon_BackTo1;
    return DuckAnyWeapon_BackTo2;
}
__attribute__((optnone)) __attribute__((naked)) void DuckAnyWeapon_Inject(void)
{
    asm volatile(
        "MOV R0, R5\n"
        "MOV R1, R4\n"
        "BL DuckAnyWeapon_Patch\n"
        "BX R0\n");
}

/* Broken below */
/* Broken below */
/* Broken below */
/* Broken below */
/* Broken below */

// Camera/sniper zooming patch
uintptr_t CamZoomProc_BackTo, FixSniperZoomingDistance_BackTo, FixSniperZoomingDistance2_BackTo;
extern "C" void CamZoomProc_Patch(CCam* cam)
{
    cam->FOV *= fAspectCorrectionDiv;
}
__attribute__((optnone)) __attribute__((naked)) void CamZoomProc_Inject(void)
{
    asm volatile(
        "VSTR S0, [R0]\n"
        "PUSH {R0-R11}\n"
        "VPUSH {S0-S2}\n"
        "LDR R0, [SP, #0x10]\n"
        "BL CamZoomProc_Patch\n");
    asm volatile(
        "VPOP {S0-S2}\n"
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "ADD.W R0, R1, #0x80\n"
        "BX R12\n"
    :: "r" (CamZoomProc_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void FixSniperZoomingDistance_Inject(void)
{
    asm volatile(
        "PUSH {R0}\n"
        "VPUSH {S0-S2}\n");
    asm volatile(
        "VMOV.F32 S4, %0\n"
    :: "r" (10.0f / fAspectCorrectionDiv));
    asm volatile(
        "VPOP {S0-S2}\n"
        "MOV R12, %0\n"
        "POP {R0}\n"
        "VMIN.F32 D1, D1, D2\n"
        "VMUL.F32 S2, S6, S2\n"
        "BX R12\n"
    :: "r" (FixSniperZoomingDistance_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void FixSniperZoomingDistance2_Inject(void)
{
    asm volatile(
        "PUSH {R0}\n"
        "VPUSH {S0-S2}\n");
    asm volatile(
        "VMOV.F32 S2, %0\n"
    :: "r" (12.0f / fAspectCorrectionDiv));
    asm volatile(
        "VPOP {S0-S2}\n"
        "MOV R12, %0\n"
        "POP {R0}\n"
        "LDRH R2, [R4,#0xE]\n"
        "VMIN.F32 D16, D0, D3\n"
        "BX R12\n"
    :: "r" (FixSniperZoomingDistance2_BackTo));
}

// Opcode 08F8
DECL_HOOKv(CheckForStatsMessage, bool unk)
{
    if(*bProcessingCutscene) return;

    CheckForStatsMessage(unk);
}

// Components clr fix
RpMaterial* SetCompColorCB(RpMaterial* mat, void* data)
{
    mat->color = *(RwRGBA*)data;
    return mat;
}
void SetComponentColor(CVehicle* self, RwFrame* frame)
{
    RpAtomic* atomic = NULL;
    RpGeometry* geometry = NULL;
    RwFrameForAllObjects(frame, GetCurrentAtomicObjectCB, &atomic);
    if(atomic)
    {
        geometry = atomic->geometry;
        geometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
        
        // CRGBA col;
        // RpGeometryForAllMaterials(self->m_pRwAtomic->geometry, GetCarColorCB, &col);
        // RpGeometryForAllMaterials(geometry, SetCompColorCB, &clr);
        // logger->Info("RpMaterialList %d", geometry->matList.numMaterials);
        
        
        //geometry->matList.materials[0]->color = *(RwRGBA*)&ms_vehicleColourTable[self->m_nPrimaryColor];
        //geometry->matList.materials[1]->color = *(RwRGBA*)&ms_vehicleColourTable[self->m_nSecondaryColor];
    }
}
DECL_HOOKv(ChooseVehicleColour, CVehicleModelInfo* self, uint8_t& prim, uint8_t& sec, uint8_t& tert, uint8_t& quat, int32_t variationShift)
{
    ChooseVehicleColour(self, prim, sec, tert, quat, variationShift);
    
    self->m_nCurrentPrimaryColor = prim;
    self->m_nCurrentSecondaryColor = sec;
    self->m_nCurrentTertiaryColor = tert;
    self->m_nCurrentQuaternaryColor = quat;
}
DECL_HOOKv(SetComponentVisibility, CAutomobile* self, RwFrame* nodeFrame, eAtomicComponentFlag visibility)
{
    CVehicleModelInfo* vi = (CVehicleModelInfo*)ms_modelInfoPtrs[self->m_nModelIndex];
    vi->m_currentColor = self->m_color;
        /*vi->m_nCurrentPrimaryColor = self->m_nPrimaryColor;
        vi->m_nCurrentSecondaryColor = self->m_nSecondaryColor;
        vi->m_nCurrentTertiaryColor = self->m_nTertiaryColor;
        vi->m_nCurrentQuaternaryColor = self->m_nQuaternaryColor;*/
    SetComponentVisibility(self, nodeFrame, visibility);
    
            
    /*if(visibility != ATOMIC_IS_DAM_STATE) return;
    
    eSomeMaxs compNum = (eSomeMaxs)-1;
    for(uint8_t i = 0; i < MAX_CAR_NODES; ++i)
    {
        if(nodeFrame == self->m_CarNodes[i]) { compNum = (eSomeMaxs)i; break; }
    }
    if(compNum == (eSomeMaxs)-1) return;
    
    switch(compNum)
    {
        default: break;
        
        case CAR_DOOR_RF:
        case CAR_DOOR_RR:
        case CAR_DOOR_LF:
        case CAR_DOOR_LR:
            SetComponentColor(self, nodeFrame);
            break;
    }*/
}
DECL_HOOKv(PreRenderCar, CAutomobile* self)
{
    PreRenderCar(self);
    
    RwFrame* node = NULL;
    for(int i = CAR_DOOR_RF; i <= CAR_DOOR_LR; ++i)
    {
        node = self->m_CarNodes[i];
        //logger->Info("car 0x%08X, comp 0x%X 0x%08X", self, i, node);
        if(!node) continue;
        
        //CRGBA clr(128,128,255);
        SetComponentColor(self, node);
    }
}

// Social Club
DECL_HOOK(void*, SC_EnterSocial)
{
    logger->Info("SC_EnterSocial 0x%08X", SC_EnterSocial());
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////     Funcs     ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
extern "C" const char* OnUpdaterURLRequested()
{
    return "https://raw.githubusercontent.com/AndroidModLoader/JPatch/main/updater.txt";
}
extern "C" void OnModLoad()
{
    logger->SetTag("JPatch");
    
    //aml->PatchForThumb(true); // Auto but may be enabled to be sure
    
    cfg->Bind("Author", "", "About")->SetString("[-=KILL MAN=-]"); cfg->ClearLast();
    cfg->Bind("IdeasFrom", "", "About")->SetString("MTA:SA Team, re3 contributors, JuniorDjjr, ThirteenAG, Blackbird88, 0x416c69, Whitetigerswt, XMDS, Peepo"); cfg->ClearLast();
    cfg->Bind("Discord", "", "About")->SetString("https://discord.gg/2MY7W39kBg"); cfg->ClearLast();
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/AndroidModLoader/JPatch"); cfg->ClearLast();
    cfg->Save();

    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    
    pSC = aml->GetLib("libSCAnd.so");
    hSC = aml->GetLibHandle("libSCAnd.so");

    // Functions Start //
    SET_TO(AddToCheatString,        aml->GetSym(hGTASA, "_ZN6CCheat16AddToCheatStringEc"));
    SET_TO(RwRenderStateSet,        aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(RwRenderStateGet,        aml->GetSym(hGTASA, "_Z16RwRenderStateGet13RwRenderStatePv"));
    SET_TO(ClearPedWeapons,         aml->GetSym(hGTASA, "_ZN4CPed12ClearWeaponsEv"));
    SET_TO(GetBulletFx,             aml->GetSym(hGTASA, "_ZN14SurfaceInfos_c11GetBulletFxEj"));
    SET_TO(LIB_PointerGetCoordinates, aml->GetSym(hGTASA, "_Z25LIB_PointerGetCoordinatesiPiS_Pf"));
    SET_TO(Touch_IsDoubleTapped,    aml->GetSym(hGTASA, "_ZN15CTouchInterface14IsDoubleTappedENS_9WidgetIDsEbi"));
    SET_TO(Touch_IsHeldDown,        aml->GetSym(hGTASA, "_ZN15CTouchInterface10IsHeldDownENS_9WidgetIDsEi"));
    SET_TO(SetCameraDirectlyBehindForFollowPed, aml->GetSym(hGTASA, "_ZN7CCamera48SetCameraDirectlyBehindForFollowPed_CamOnAStringEv"));
    SET_TO(RestoreCamera,           aml->GetSym(hGTASA, "_ZN7CCamera7RestoreEv"));
    SET_TO(FindPlayerVehicle,       aml->GetSym(hGTASA, "_Z17FindPlayerVehicleib"));
    SET_TO(FindPlayerPed,           aml->GetSym(hGTASA, "_Z13FindPlayerPedi"));
    SET_TO(PhysicalApplyForce,      aml->GetSym(hGTASA, "_ZN9CPhysical10ApplyForceE7CVectorS0_b"));
    SET_TO(GetFrameNodeName,        aml->GetSym(hGTASA, "_Z16GetFrameNodeNameP7RwFrame"));
    SET_TO(SpriteCalcScreenCoors,   aml->GetSym(hGTASA, "_ZN7CSprite15CalcScreenCoorsERK5RwV3dPS0_PfS4_bb"));
    SET_TO(WorldRemoveEntity,       aml->GetSym(hGTASA, "_ZN6CWorld6RemoveEP7CEntity"));
    SET_TO(SetFontColor,            aml->GetSym(hGTASA, "_ZN5CFont8SetColorE5CRGBA"));
    SET_TO(ProcessVerticalLine,     aml->GetSym(hGTASA, "_ZN6CWorld19ProcessVerticalLineERK7CVectorfR9CColPointRP7CEntitybbbbbbP15CStoredCollPoly"));
    SET_TO(CreateEntityRwObject,    aml->GetSym(hGTASA, "_ZN7CEntity14CreateRwObjectEv"));
    SET_TO(RequestModel,            aml->GetSym(hGTASA, "_ZN10CStreaming12RequestModelEii"));
    SET_TO(LoadAllRequestedModels,  aml->GetSym(hGTASA, "_ZN10CStreaming22LoadAllRequestedModelsEb"));
    SET_TO(AddAnimBlockRef,         aml->GetSym(hGTASA, "_ZN12CAnimManager15AddAnimBlockRefEi"));
    SET_TO(TimerStop,               aml->GetSym(hGTASA, "_ZN6CTimer4StopEv"));
    SET_TO(TimerUpdate,             aml->GetSym(hGTASA, "_ZN6CTimer6UpdateEv"));
    SET_TO(RemoveLeastUsedModel,    aml->GetSym(hGTASA, "_ZN10CStreaming20RemoveLeastUsedModelEj"));
    SET_TO(GetTouchPosition,        aml->GetSym(hGTASA, "_ZN15CTouchInterface16GetTouchPositionEi"));
    SET_TO(StoreStaticShadow,       aml->GetSym(hGTASA, "_ZN8CShadows17StoreStaticShadowEjhP9RwTextureP7CVectorffffshhhfffbf"));
    SET_TO(TransformPoint,          aml->GetSym(hGTASA, "_Z14TransformPointR5RwV3dRK16CSimpleTransformRKS_"));
    SET_TO(IsOnAMission,            aml->GetSym(hGTASA, "_ZN11CTheScripts18IsPlayerOnAMissionEv"));
    SET_TO(emu_glEnable,            aml->GetSym(hGTASA, "_Z12emu_glEnablej"));
    SET_TO(emu_glDisable,           aml->GetSym(hGTASA, "_Z13emu_glDisablej"));
    SET_TO(BrightLightsInit,        aml->GetSym(hGTASA, "_ZN13CBrightLights4InitEv"));
    SET_TO(BrightLightsRender,      aml->GetSym(hGTASA, "_ZN13CBrightLights6RenderEv"));
    SET_TO(CreateObject,            aml->GetSym(hGTASA, "_ZN7CObject6CreateEib"));
    SET_TO(Get2dEffect,             aml->GetSym(hGTASA, "_ZN14CBaseModelInfo11Get2dEffectEi"));
    SET_TO(RpAnimBlendClumpGetAssociation,aml->GetSym(hGTASA, "_Z30RpAnimBlendClumpGetAssociationP7RpClumpPKc"));
    SET_TO(RpAnimBlendClumpGetAssociationU,aml->GetSym(hGTASA, "_Z30RpAnimBlendClumpGetAssociationP7RpClumpj"));
    SET_TO(RwFrameForAllObjects,    aml->GetSym(hGTASA, "_Z20RwFrameForAllObjectsP7RwFramePFP8RwObjectS2_PvES3_"));
    SET_TO(GetCurrentAtomicObjectCB,aml->GetSym(hGTASA, "_Z24GetCurrentAtomicObjectCBP8RwObjectPv"));
    SET_TO(RpGeometryForAllMaterials,aml->GetSym(hGTASA, "_Z25RpGeometryForAllMaterialsP10RpGeometryPFP10RpMaterialS2_PvES3_"));
    SET_TO(SetComponentAtomicAlpha, aml->GetSym(hGTASA, "_ZN8CVehicle23SetComponentAtomicAlphaEP8RpAtomici"));
    SET_TO(ApplyMoveForce,          aml->GetSym(hGTASA, "_ZN9CPhysical14ApplyMoveForceE7CVector"));
    SET_TO(GetWaterLevel,           aml->GetSym(hGTASA, "_ZN11CWaterLevel13GetWaterLevelEfffPfbP7CVector"));
    SET_TO(GetTaskSwim,             aml->GetSym(hGTASA, "_ZNK16CPedIntelligence11GetTaskSwimEv"));
    SET_TO(TaskConstructor,         aml->GetSym(hGTASA, "_ZN5CTasknwEj"));
    SET_TO(TaskStartNamedAnim,      aml->GetSym(hGTASA, "_ZN23CTaskSimpleRunNamedAnimC2EPKcS1_ifibbbb"));
    SET_TO(SetTask,                 aml->GetSym(hGTASA, "_ZN12CTaskManager7SetTaskEP5CTaskib"));
    SET_TO(TaskComplexSequenceAddTask, aml->GetSym(hGTASA, "_ZN20CTaskComplexSequence7AddTaskEP5CTask"));
    SET_TO(BlendAnimation,          aml->GetSym(hGTASA, "_ZN12CAnimManager14BlendAnimationEP7RpClump12AssocGroupId11AnimationIdf"));
    HOOKPLT(InitRenderWare,         pGTASA + 0x66F2D0);
    //HOOK(CalculateAspectRatio,      aml->GetSym(hGTASA, "_ZN5CDraw20CalculateAspectRatioEv"));
    // Functions End   //
    
    // Variables Start //
    SET_TO(ms_taskSequence,         aml->GetSym(hGTASA, "_ZN14CTaskSequences15ms_taskSequenceE"));
    SET_TO(pActiveScripts,          aml->GetSym(hGTASA, "_ZN11CTheScripts14pActiveScriptsE"));
    SET_TO(ms_fTimeStep,            aml->GetSym(hGTASA, "_ZN6CTimer12ms_fTimeStepE"));
    SET_TO(WorldPlayers,            *(void**)(pGTASA + 0x6783C8)); // Patched CWorld::Players will work now!
    SET_TO(ms_fFOV,                 aml->GetSym(hGTASA, "_ZN5CDraw7ms_fFOVE"));
    SET_TO(game_FPS,                aml->GetSym(hGTASA, "_ZN6CTimer8game_FPSE"));
    SET_TO(TheCamera,               aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(RsGlobal,                aml->GetSym(hGTASA, "RsGlobal"));
    SET_TO(g_surfaceInfos,          aml->GetSym(hGTASA, "g_surfaceInfos"));
    SET_TO(m_snTimeInMilliseconds,  aml->GetSym(hGTASA, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(gMobileMenu,             aml->GetSym(hGTASA, "gMobileMenu"));
    SET_TO(NumberOfSearchLights,    aml->GetSym(hGTASA, "_ZN5CHeli20NumberOfSearchLightsE"));
    SET_TO(lastDevice,              aml->GetSym(hGTASA, "lastDevice"));
    SET_TO(m_pWidgets,              *(void**)(pGTASA + 0x67947C)); // Patched CTouchInterface::m_pWidgets will work now!
    SET_TO(bDidWeProcessAnyCinemaCam, aml->GetSym(hGTASA, "bDidWeProcessAnyCinemaCam"));
    SET_TO(bRunningCutscene,        aml->GetSym(hGTASA, "_ZN12CCutsceneMgr10ms_runningE"));
    SET_TO(bProcessingCutscene,     aml->GetSym(hGTASA, "_ZN12CCutsceneMgr21ms_cutsceneProcessingE"));
    SET_TO(ScriptParams,            *(void**)(pGTASA + 0x676F7C)); // Patched ScriptParams will work now!
    SET_TO(m_alphaList,             aml->GetSym(hGTASA, "_ZN18CVisibilityPlugins11m_alphaListE"));
    SET_TO(CloudsRotation,          aml->GetSym(hGTASA, "_ZN7CClouds13CloudRotationE"));
    SET_TO(CloudsIndividualRotation, aml->GetSym(hGTASA, "_ZN7CClouds18IndividualRotationE"));
    SET_TO(WeatherWind,             aml->GetSym(hGTASA, "_ZN8CWeather4WindE"));
    SET_TO(pObjectPool,             *(void**)(pGTASA + 0x676BBC));
    SET_TO(m_pCurrZoneInfo,         aml->GetSym(hGTASA, "_ZN9CPopCycle15m_pCurrZoneInfoE"));
    SET_TO(m_ZoneFadeTimer,         aml->GetSym(hGTASA, "_ZN4CHud15m_ZoneFadeTimerE"));
    SET_TO(ms_numAnimBlocks,        aml->GetSym(hGTASA, "_ZN12CAnimManager16ms_numAnimBlocksE"));
    SET_TO(ms_memoryUsed,           aml->GetSym(hGTASA, "_ZN10CStreaming13ms_memoryUsedE"));
    SET_TO(ms_memoryAvailable,      aml->GetSym(hGTASA, "_ZN10CStreaming18ms_memoryAvailableE"));
    SET_TO(fSpriteBrightness,       pGTASA + 0x966590);
    SET_TO(detailTexturesStorage,   aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime14detailTexturesE") + 8); // pGTASA + 0x6BD1D8
    SET_TO(textureDetail,           aml->GetSym(hGTASA, "textureDetail"));
    SET_TO(ms_iActiveSequence,      aml->GetSym(hGTASA, "_ZN14CTaskSequences18ms_iActiveSequenceE"));
    SET_TO(RasterExtOffset,         aml->GetSym(hGTASA, "RasterExtOffset"));
    SET_TO(m_f3rdPersonCHairMultX,  aml->GetSym(hGTASA, "_ZN7CCamera22m_f3rdPersonCHairMultXE"));
    SET_TO(m_f3rdPersonCHairMultY,  aml->GetSym(hGTASA, "_ZN7CCamera22m_f3rdPersonCHairMultYE"));
    SET_TO(ms_fAspectRatio,         aml->GetSym(hGTASA, "_ZN5CDraw15ms_fAspectRatioE"));
    SET_TO(aWeaponInfo,             aml->GetSym(hGTASA, "aWeaponInfo"));
    SET_TO(windowSize,              aml->GetSym(hGTASA, "windowSize"));
    SET_TO(ms_bIsPlayerOnAMission,  aml->GetSym(hGTASA, "_ZN10CPedGroups22ms_bIsPlayerOnAMissionE"));
    SET_TO(DETAILEDWATERDIST,       aml->GetSym(hGTASA, "DETAILEDWATERDIST"));
    SET_TO(ms_nNumGang,             aml->GetSym(hGTASA, "_ZN11CPopulation11ms_nNumGangE"));
    SET_TO(aPolyBunches,            aml->GetSym(hGTASA, "_ZN8CShadows12aPolyBunchesE"));
    SET_TO(ms_modelInfoPtrs,        aml->GetSym(hGTASA, "_ZN10CModelInfo16ms_modelInfoPtrsE"));
    SET_TO(ms_vehicleColourTable,   aml->GetSym(hGTASA, "_ZN17CVehicleModelInfo21ms_vehicleColourTableE"));
    // Variables End //
    
    // Animated textures
    if(cfg->GetBool("EnableAnimatedTextures", true, "Visual"))
    {
        aml->Write(aml->GetSym(hGTASA, "RunUVAnim"), (uintptr_t)"\x01", 1);
    }

    // Vertex weight
    if(cfg->GetBool("FixVertexWeight", true, "Visual"))
    {
        aml->Write(pGTASA + 0x1C8064, (uintptr_t)"\x01", 1);
        aml->Write(pGTASA + 0x1C8082, (uintptr_t)"\x01", 1);
    }

    // Fix moon!
    // War Drum moment: cannot get Alpha testing to work
    /*if(cfg->GetBool("MoonPhases", true, "Visual"))
    {
        //aml->Write(pGTASA + 0x1AF5C2, (uintptr_t)"\x4F\xF0\x00\x03", 4);
        MoonVisual_1_BackTo = pGTASA + 0x59ED90 + 0x1;
        MoonVisual_2_BackTo = pGTASA + 0x59EE4E + 0x1;
        aml->Redirect(pGTASA + 0x59ED80 + 0x1, (uintptr_t)MoonVisual_1_Inject);
        aml->Redirect(pGTASA + 0x59EE36 + 0x1, (uintptr_t)MoonVisual_2_Inject);
    };*/

    // Fix sky multitude
    if(cfg->GetBool("FixSkyMultitude", true, "Visual"))
    {
        aml->Unprot(pGTASA + 0x59FB8C, 2*sizeof(float));
        *(float*)(pGTASA + 0x59FB8C) = -10.0f;
        *(float*)(pGTASA + 0x59FB90) =  10.0f;
    }

    // Fix vehicles backlights light state
    if(cfg->GetBool("FixCarsBacklightLightState", true, "Visual"))
    {
        aml->Write(pGTASA + 0x591272, (uintptr_t)"\x02", 1);
        aml->Write(pGTASA + 0x59128E, (uintptr_t)"\x02", 1);
    }

    // Limit sand/dust particles on bullet impact (they are EXTREMELY dropping FPS)
    if(cfg->GetBool("LimitSandDustBulletParticles", true, "Visual"))
    {
        AddBulletImpactFx_BackTo = pGTASA + 0x36478E + 0x1;
        aml->Redirect(pGTASA + 0x36477C + 0x1, (uintptr_t)AddBulletImpactFx_Inject);
        if(cfg->GetBool("LimitSandDustBulletParticlesWithSparkles", false, "Visual"))
        {
            nLimitWithSparkles = BULLETFX_SPARK;
        }
    }

    // Do not set primary color to the white on vehicles paintjob
    if(cfg->GetBool("PaintJobDontSetPrimaryToWhite", true, "Visual"))
    {
        aml->PlaceNOP(pGTASA + 0x582328, 2);
    }

    // Fix walking while rifle-aiming
    if(cfg->GetBool("FixAimingWalkRifle", true, "Gameplay"))
    {
        HOOKPLT(ControlGunMove, pGTASA + 0x66F9D0);
    }

    // Fix water physics
    if(cfg->GetBool("FixWaterPhysics", true, "Gameplay"))
    {
        HOOKPLT(ProcessSwimmingResistance, pGTASA + 0x66E584);
        ProcessBuoyancy_BackTo = pGTASA + 0x56F946 + 0x1;
        aml->Redirect(pGTASA + 0x56F930 + 0x1, (uintptr_t)ProcessBuoyancy_Inject);
    }

    // Fix stealable items sucking
    if(cfg->GetBool("ClampObjectToStealDist", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x40B162, (uintptr_t)"\xB7\xEE\x00\x0A", 4);
    }

    // Fix broken basketball minigame by placing the save icon away from it
    if(cfg->GetBool("MaddDoggMansionSaveFix", true, "SCMFixes"))
    {
        HOOKPLT(GenerateNewPickup_MaddDogg, pGTASA + 0x674DE4);
    }

    // Fix broken basketball minigame by placing the save icon away from it
    if(cfg->GetBool("FixStarBribeInSFBuilding", true, "SCMFixes"))
    {
        HOOKPLT(GenerateNewPickup_SFBribe, pGTASA + 0x674DE4);
    }

    // Fix rifle pickup that stuck inside the stadium
    if(cfg->GetBool("FixSFStadiumRiflePickup", true, "SCMFixes"))
    {
        HOOKPLT(GenerateNewPickup_SFRiflePickup, pGTASA + 0x674DE4);
    }

    // Remove jetpack leaving on widget press while in air?
    if(cfg->GetBool("DisableDropJetPackInAir", true, "Gameplay"))
    {
        HOOKPLT(DropJetPackTask, pGTASA + 0x675AA8);
    }

    // Dont stop the car before leaving it
    if(cfg->GetBool("ImmediatelyLeaveTheCar", true, "Gameplay"))
    {
        HOOK(CanPedStepOutCar, aml->GetSym(hGTASA, "_ZNK8CVehicle16CanPedStepOutCarEb"));
    }

    // Bring back penalty when CJ dies!
    if(cfg->GetBool("WeaponPenaltyIfDied", true, "Gameplay"))
    {
        DiedPenalty_BackTo = pGTASA + 0x3088E0 + 0x1;
        aml->Redirect(pGTASA + 0x3088BE + 0x1, (uintptr_t)DiedPenalty_Inject);
    }

    // Fix emergency vehicles
    if(cfg->GetBool("FixEmergencyVehicles", true, "Gameplay"))
    {
        EmergencyVeh_BackTo = pGTASA + 0x3DD88C + 0x1;
        aml->Redirect(pGTASA + 0x3DD87A + 0x1, (uintptr_t)EmergencyVeh_Inject);
        HOOKPLT(SetFOV_Emergency, pGTASA + 0x673DDC);
        aml->Write(pGTASA + 0x3DD8A0, (uintptr_t)"\xB0\xEE\x42\x1A", 4); // WarDumbs are multiplying it by 0.8? Reasonable for 2013 but why didnt they remove that in 2.00?
        aml->Write(pGTASA + 0x3DD8A4, (uintptr_t)"\xB0\xEE\x40\x2A", 4); // Same thing but by 0.875... Cringe.
    }

    // Fix cutscene FOV (disabled by default right now, causes the camera being too close on ultrawide screens)
    if(cfg->GetBool("FixCutsceneFOV", false, "Visual"))
    {
        HOOKPLT(SetFOV, pGTASA + 0x673DDC);
    }

    // Fix red marker that cannot be placed in a menu on ultrawide screens
    // Kinda trashy fix...
    if(cfg->GetBool("FixRedMarkerUnplaceable", true, "Gameplay"))
    {
        aml->Unprot(pGTASA + 0x2A9E60, sizeof(float));
        *(float*)(pGTASA + 0x2A9E60) /= 1.2f;
        aml->Write(pGTASA + 0x2A9D42, (uintptr_t)"\x83\xEE\x0C\x3A", 4);
        HOOKPLT(PlaceRedMarker_MarkerFix, pGTASA + 0x6702C8);
    }

    // Dont set player on fire when he's on burning BMX (MTA:SA)
    if(cfg->GetBool("DontBurnPlayerOnBurningBMX", true, "Gameplay"))
    {
        aml->PlaceB(pGTASA + 0x3F1ECC + 0x1, pGTASA + 0x3F1F24 + 0x1);
    }

    // Increase the number of vehicles types (not actual vehicles) that can be loaded at once (MTA:SA)
    // Causes crash and completely useless
    //if(cfg->GetBool("DesiredNumOfCarsLoadedBuff", true, "Gameplay"))
    //{
    //    *(unsigned char*)(aml->GetSym(hGTASA, "_ZN10CStreaming24desiredNumVehiclesLoadedE")) = 50; // Game hardcoded to 50 max (lazy to fix crashes for patches below)
    //    aml->PlaceNOP(pGTASA + 0x46BE1E, 1);
    //    aml->PlaceNOP(pGTASA + 0x47269C, 2);
    //}

    // THROWN projectiles throw more accurately (MTA:SA)
    if(cfg->GetBool("ThrownProjectilesAccuracy", true, "Gameplay"))
    {
        aml->PlaceB(pGTASA + 0x5DBBC8 + 0x1, pGTASA + 0x5DBD0C + 0x1);
    }

    // Disable call to FxSystem_c::GetCompositeMatrix in CAEFireAudioEntity::UpdateParameters 
    // that was causing a crash - spent ages debugging, the crash happens if you create 40 or 
    // so vehicles that catch fire (upside down) then delete them, repeating a few times.
    // (MTA:SA)
    if(cfg->GetBool("GetCompositeMatrixFixPossibleCrash", true, "Gameplay"))
    {
        aml->PlaceNOP(pGTASA + 0x395E6A, 7);
    }

    // Disable setting the occupied's vehicles health to 75.0f when a burning ped enters it (MTA:SA)
    if(cfg->GetBool("DontGiveCarHealthFromBurningPed", true, "Gameplay"))
    {
        aml->PlaceNOP(pGTASA + 0x3F1CAC, 0xD);
    }

    // Increase intensity of vehicle tail light corona (MTA:SA)
    // Is this even working on Android?
    if(cfg->GetBool("IncreaseTailLightIntensity", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x591016, (uintptr_t)"\xF0", 1);
    }

    // Cinematic vehicle camera on double tap
    if(cfg->GetBool("CinematicCameraOnDoubleTap", true, "Gameplay"))
    {
        HOOKPLT(PlayerInfoProcess_Cinematic, pGTASA + 0x673E84);
    }
    
    // Fix Skimmer plane ( https://github.com/XMDS )
    if (cfg->GetBool("SkimmerPlaneFix", true, "Gameplay"))
    {
        SkimmerWaterResistance_BackTo = pGTASA + 0x589ADC + 0x1;
        aml->Redirect(pGTASA + 0x589AD4 + 0x1, (uintptr_t)SkimmerWaterResistance_Inject);
    }

    // Buff streaming
    if(cfg->GetBool("BuffStreamingMem", true, "Gameplay"))
    {
        int wantsMB = cfg->GetInt("BuffStreamingMem_CountMB", 512, "Gameplay");
        if(wantsMB >= 20)
        {
            aml->PlaceNOP(pGTASA + 0x46BE18, 1);
            aml->PlaceNOP(pGTASA + 0x47272A, 2);
            aml->PlaceNOP(pGTASA + 0x472690, 2);
            if(*ms_memoryAvailable < wantsMB*1024*1024)
            {
                *ms_memoryAvailable = wantsMB * 1024 * 1024;
            }
        }
    }

    // Buff streaming memory (dynamic)
    bDynStreamingMem = cfg->GetBool("DynamicStreamingMem", true, "Gameplay");
    fDynamicStreamingMemPercentage = 0.001f * cfg->GetInt("DynamicStreamingMem_Percentage", 80, "Gameplay");

    // Buff planes max height
    if(cfg->GetBool("BuffPlanesMaxHeight", true, "Gameplay"))
    {
        float* heights;
        aml->Unprot(pGTASA + 0x585674, sizeof(float)*7);
        SET_TO(heights, pGTASA + 0x585674);
        for(int i = 0; i < 7; ++i)
        {
            heights[i] *= 1.25f;
        }
    }

    // Buff jetpack max height
    if(cfg->GetBool("BuffJetpackMaxHeight", true, "Gameplay"))
    {
        aml->Unprot(pGTASA + 0x5319D0, sizeof(float));
        *(float*)(pGTASA + 0x5319D0) *= 2.0f;
    }

    // 44100 Hz Audio support (without a mod OpenAL Update) (is this working?)
    if(cfg->GetBool("Allow44100HzAudio", true, "Gameplay"))
    {
        aml->Unprot(pGTASA + 0x613E0A, sizeof(int));
        *(int*)(pGTASA + 0x613E0A) = 44100;
    }

    // Disable GTA vehicle detachment at rotation awkwardness
    if(cfg->GetBool("FixVehicleDetachmentAtRot", true, "Visual"))
    {
        aml->PlaceB(pGTASA + 0x407344 + 0x1, pGTASA + 0x407016 + 0x1);
    }

    // Bring back missing "Shoot" button for S.W.A.T. when we dont have a weapon. WarDrum forgot about it.
    if(cfg->GetBool("FixMissingShootBtnForSWAT", true, "Gameplay"))
    {
        GetCarGunFired_BackTo1 = pGTASA + 0x3F99E8 + 0x1;
        GetCarGunFired_BackTo2 = pGTASA + 0x3F9908 + 0x1;
        aml->Redirect(pGTASA + 0x3F99C4 + 0x1, (uintptr_t)GetCarGunFired_Inject);
    }

    // Just a fuzzy seek. Tell MPG123 to not load useless data.
    if(cfg->GetBool("FuzzySeek", true, "Gameplay"))
    {
        HOOKPLT(mpg123_param, pGTASA + 0x66F3D4);
    }

    // Fix water cannon on a high fps
    if(cfg->GetBool("FixHighFPSWaterCannons", true, "Gameplay"))
    {
        HOOKPLT(WaterCannonRender, pGTASA + 0x67432C);
        HOOKPLT(WaterCannonUpdate, pGTASA + 0x6702EC);
    }

    // Fix moving objects on a high fps (through the scripts)
    if(cfg->GetBool("FixHighFPSOpcode034E", true, "SCMFixes"))
    {
        ProcessCommands800To899_BackTo = pGTASA + 0x347866 + 0x1;
        aml->Redirect(pGTASA + 0x346E84 + 0x1, (uintptr_t)ProcessCommands800To899_Inject);
    }

    // Fix pushing force
    if(cfg->GetBool("FixPhysicalPushForce", true, "Gameplay"))
    {
        PhysicalApplyCollision_BackTo = pGTASA + 0x402B72 + 0x1;
        aml->Redirect(pGTASA + 0x402B68 + 0x1, (uintptr_t)PhysicalApplyCollision_Inject);
    }

    // Can now rotate the camera inside the heli/plane?
    // https://github.com/TheOfficialFloW/gtasa_vita/blob/6417775e182b0c8b789cc9a0c1161e6f1b43814f/loader/main.c#L736
    if(cfg->GetBool("UnstuckHeliCamera", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x3C0866, (uintptr_t)"\x00\x20\x00\xBF", 4);
        aml->Write(pGTASA + 0x3C1518, (uintptr_t)"\x00\x20\x00\xBF", 4);
        aml->Write(pGTASA + 0x3C198A, (uintptr_t)"\x00\x20\x00\xBF", 4);
        aml->Write(pGTASA + 0x3FC462, (uintptr_t)"\x00\x20\x00\xBF", 4);
        aml->Write(pGTASA + 0x3FC754, (uintptr_t)"\x00\x20\x00\xBF", 4);
    }

    // Classic CJ shadow
    if(cfg->GetBool("FixClassicCJShadow", true, "Gameplay"))
    {
        aml->PlaceNOP(pGTASA + 0x5B86C4, 7);
    }

    // Car Slowdown Fix
    if(cfg->GetBool("FixCarSlowdownHighFPS", true, "Gameplay"))
    {
        SET_TO(mod_HandlingManager_off4, (*(uintptr_t*)(pGTASA + 0x6777C8)) + 4); // FLA
        HOOKPLT(ProcessVehicleWheel, pGTASA + 0x66FC7C);
    }

    // Heli rotor speed fix
    if(cfg->GetBool("FixHeliRotorSpeedHighFPS", true, "Visual"))
    {
        aml->Unprot(pGTASA + 0x572604, sizeof(float)*5);
        SET_TO(fRotorFinalSpeed, pGTASA + 0x572604);
        SET_TO(fRotor1Speed, pGTASA + 0x572610);
        SET_TO(fRotor2Speed, pGTASA + 0x572608);
        HOOK(Heli_ProcessFlyingStuff, aml->GetSym(hGTASA, "_ZN5CHeli21ProcessFlyingCarStuffEv"));
    }

    // Give more space for opcodes 038D+038F
    if(cfg->GetBool("FixOpcodes038D/F", true, "SCMFixes"))
    {
        aml->Unprot(pGTASA + 0x678EAC, sizeof(void*));
        *(uintptr_t*)(pGTASA + 0x678EAC) = (uintptr_t)pNewScriptSprites;
        aml->Unprot(pGTASA + 0x67915C, sizeof(void*));
        *(uintptr_t*)(pGTASA + 0x67915C) = (uintptr_t)pNewIntroRectangles;

        aml->Write(pGTASA + 0x327E6E, (uintptr_t)"\xB6\xF5\xC0\x6F", 4); // CMissionCleanup::Process
        aml->Write(pGTASA + 0x328298, (uintptr_t)"\xB4\xF5\xC0\x6F", 4); // CTheScripts::RemoveScriptTextureDictionary
        aml->Write(pGTASA + 0x32A638, (uintptr_t)"\xB4\xF5\xC0\x6F", 4); // CTheScripts::Init
        aml->Write(pGTASA + 0x1A3736, (uintptr_t)"\x40\xF2\xFC\x54", 4); // sub_1A3730
        aml->Write(pGTASA + 0x1A37F4, (uintptr_t)"\xB5\xF5\xC0\x6F", 4); // sub_1A3750
        
        aml->Write(pGTASA + 0x329E6C, (uintptr_t)"\xB8\xF5\xB4\x4F", 4); // CTheScripts::DrawScriptSpritesAndRectangles
        aml->Write(pGTASA + 0x32A5FC, (uintptr_t)"\xB6\xF5\xB4\x4F", 4); // CTheScripts::Init
        aml->Write(pGTASA + 0x32B040, (uintptr_t)"\xB2\xF5\xB4\x4F", 4); // CTheScripts::Process
    }

    // RE3: Fix R* optimization that prevents peds to spawn
    if(cfg->GetBool("Re3_PedSpawnDeoptimize", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x3F40E8, (uintptr_t)"\x03", 1);
    }

    // RE3: Make cars and peds to not despawn when you look away
    if(cfg->GetBool("Re3_ExtOffscreenDespRange", true, "Gameplay"))
    {
        aml->PlaceB(pGTASA + 0x2EC660 + 0x1, pGTASA + 0x2EC6D6 + 0x1); // Vehicles
        aml->PlaceB(pGTASA + 0x4CE4EA + 0x1, pGTASA + 0x4CE55C + 0x1); // Peds
    }

    // RE3: Do not remove locked cars
    if(cfg->GetBool("Re3_DontRemoveLockedCars", true, "Gameplay"))
    {
        HOOKPLT(PossiblyRemoveVehicle_Re3, pGTASA + 0x6736E4);
    }

    // RE3: Correct clouds rotating speed
    if(cfg->GetBool("Re3_CloudsRotationHighFPS", true, "Visual"))
    {
        HOOKPLT(CloudsUpdate_Re3, pGTASA + 0x670358);
    }

    // RE3: multiple instances of semaphore fix
    if(cfg->GetBool("Re3_CdStreamMultipleInst", true, "Gameplay"))
    {
        aml->Unprot(pGTASA + 0x26C0DA, 4);
        aml->Write(pGTASA + 0x2C97DC, (uintptr_t)"\x00\x23", 2);
        SET_TO(ThreadLaunch_GagNameSet, pGTASA + 0x26C0DA);
        HOOKPLT(OS_ThreadLaunch, pGTASA + 0x66E8A0);
    }

    // RE3: Fix a lil mistake in AskForObjectToBeRenderedInGlass
    if(cfg->GetBool("Re3_InGlassRenderedPlus1", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5AC61C, (uintptr_t)"\x1F", 1);
    }

    // RE3: Free the space for an object in a pool by deleting temp objects if there is no space
    if(cfg->GetBool("Re3_FreePlaceInObjectPool", true, "Gameplay"))
    {
        HOOKPLT(Object_New, pGTASA + 0x6726EC);
    }

    // Lower threads sleeping timer
    if(cfg->GetBool("LowerThreadsSleeping", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x1D248E, (uintptr_t)"\x08", 1);
        aml->Write(pGTASA + 0x266D3A, (uintptr_t)"\x08", 1);
        aml->Write(pGTASA + 0x26706E, (uintptr_t)"\x08", 1);
        aml->Write(pGTASA + 0x26FDCC, (uintptr_t)"\x08", 1);
    }

    // Dont kill peds when jacking their car, monster!
    if(cfg->GetBool("DontKillPedsOnCarJacking", true, "Gameplay"))
    {
        aml->PlaceB(pGTASA + 0x4F5FC4 + 0x1, pGTASA + 0x4F5FD6 + 0x1);
    }

    // Colored zone names are back
    if(cfg->GetBool("ColoredZoneNames", true, "Visual"))
    {
        ColoredZoneNames_BackTo = pGTASA + 0x438404 + 0x1;
        aml->Redirect(pGTASA + 0x4383D6 + 0x1, (uintptr_t)ColoredZoneNames_Inject);
    }

    // Bigger max count of peds
    if(cfg->GetBool("BuffMaxPedsCount", true, "Gameplay"))
    {
        *(int*)aml->GetSym(hGTASA, "_ZN11CPopulation20MaxNumberOfPedsInUseE") = 0x23;
        aml->Write(pGTASA + 0x3F4DE0, (uintptr_t)"\x23", 1);
        aml->Write(pGTASA + 0x4CC284, (uintptr_t)"\x23", 1);
        aml->Write(pGTASA + 0x4CCBE0, (uintptr_t)"\x23", 1);
        aml->Write(pGTASA + 0x4CCBEA, (uintptr_t)"\x1C", 1);
    }

    // Bigger max count of cars
    if(cfg->GetBool("BuffMaxCarsCount", true, "Gameplay"))
    {
        *(int*)aml->GetSym(hGTASA, "_ZN8CCarCtrl20MaxNumberOfCarsInUseE") = 0x14;
        aml->Write(pGTASA + 0x3F4DD2, (uintptr_t)"\x14", 1);
    }

    // Frick your "improved characters models", War Dumb
    if(cfg->GetBool("FixPedSpecInShaders", true, "Visual"))
    {
        aml->Write(pGTASA + 0x1CE2F0, (uintptr_t)"\x40\x46\x00\xBF", 4);
        aml->Write(pGTASA + 0x1CEDC4, (uintptr_t)"\x40\xF2\x40\x60", 4);
        aml->Write(pGTASA + 0x1CEF1A, (uintptr_t)"\x58\x46\x00\xBF", 4);
        aml->PlaceB(pGTASA + 0x1CF5C8 + 0x1, pGTASA + 0x1CF658 + 0x1);
    }

    // Tells "FindGroundZ" functions that we need "can teleport on objects" too
    if(cfg->GetBool("IncludeObjectsForFindZ", true, "Gameplay"))
    {
        HOOKPLT(FindGroundZ2D, pGTASA + 0x66EDB8);
        HOOKPLT(FindGroundZ3D, pGTASA + 0x67022C);
    }

    // RE3: Road reflections
    if(cfg->GetBool("Re3_WetRoadsReflections", true, "Visual"))
    {
        RoadReflections_BackTo = pGTASA + 0x5A2EA4 + 0x1;
        aml->Redirect(pGTASA + 0x5A2E94 + 0x1, (uintptr_t)RoadReflections_Inject);
    }

    // Helicopter's rotor blur
    bool heliblur = cfg->GetBool("HeliRotorBlur", true, "Visual");
    bool planeblur = cfg->GetBool("PlaneRotorBlur", false, "Visual");
    if(heliblur || planeblur)
    {
        aml->Redirect(pGTASA + 0x572D6C + 0x1, (uintptr_t)RotorBlurRender_Inject1);
        aml->Redirect(pGTASA + 0x572D8C + 0x1, (uintptr_t)RotorBlurRender_Inject2);
        RotorBlurRender_BackTo1 = pGTASA + 0x572D76 + 0x1;
        RotorBlurRender_BackTo2 = pGTASA + 0x572D96 + 0x1;
        
        if(heliblur)
        {
            HOOK(HeliRender, aml->GetSym(hGTASA, "_ZN5CHeli6RenderEv"));
            nRotorMdlIgnore = cfg->GetBool("HeliRotorBlur_NotMaverick", true, "Visual") ? 487 : 0;
        }
        // Glitchy planes
        if(planeblur) HOOK(PlaneRender, aml->GetSym(hGTASA, "_ZN6CPlane6RenderEv"));
    }

    /* ImprovedStreaming by ThirteenAG & Junior_Djjr */
    /* ImprovedStreaming by ThirteenAG & Junior_Djjr */
    /* ImprovedStreaming by ThirteenAG & Junior_Djjr */
    
    // Preload LOD models
    bPreloadLOD = cfg->GetBool("IS_PreloadLODs", false, "Gameplay");
    bPreloadAnim = cfg->GetBool("IS_PreloadAnims", false, "Gameplay");
    bPreloadPed = cfg->GetBool("IS_PreloadPeds", false, "Gameplay");
    if(bPreloadLOD || bPreloadAnim || bPreloadPed || bDynStreamingMem)
    {
        HOOKPLT(GameProcess, pGTASA + 0x66FE58);
        if(bPreloadLOD)
        {
            LoadScene_BackTo = pGTASA + 0x4691E2 + 0x1;
            aml->Redirect(pGTASA + 0x4691D6 + 0x1, (uintptr_t)LoadScene_Inject);
        }
        if(bPreloadPed)
        {
            HOOKPLT(AddPedModel, pGTASA + 0x675D98);
            aml->Redirect(pGTASA + 0x40CC20 + 0x1, (uintptr_t)InitPools_Inject);
            InitPools_BackTo = pGTASA + 0x40CC8A + 0x1;
        }
    }
    bUnloadUnusedModels = cfg->GetBool("IS_UnloadUnusedModels", true, "Gameplay");
    if(bUnloadUnusedModels)
    {
        bDontUnloadInCutscenes = !cfg->GetBool("IS_UnloadUnusedModels_InCutscene", false, "Gameplay");
        fRemoveUnusedStreamMemPercentage = 0.001f * cfg->GetInt("IS_UnloadUnusedModels_Percentage", 80, "Gameplay");
        nRemoveUnusedInterval = cfg->GetInt("IS_UnloadUnusedModels_Interval", 60, "Gameplay");

        if(fRemoveUnusedStreamMemPercentage < 0.001f ||
           fRemoveUnusedStreamMemPercentage > 0.99f  ||
           nRemoveUnusedInterval < 0)
        {
            bUnloadUnusedModels = false;
        }
    }

    // Fix color picker widget
    if(cfg->GetBool("FixColorPicker", true, "Visual"))
    {
        HOOKPLT(GetColorPickerValue, pGTASA + 0x6645C4);
    }

    // Bigger distance for light coronas
    if(cfg->GetBool("BuffDistForLightCoronas", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5A4960, (uintptr_t)"\x00\x22\xC4\xF2\xC8\x32", 6); // CEntity::ProcessLightsForEntity
        aml->Write(pGTASA + 0x362EC6, (uintptr_t)"\x00\x20\xC4\xF2\xC8\x30", 6); // CTrafficLights::DisplayActualLight
        aml->Write(pGTASA + 0x56585E, (uintptr_t)"\x00\x21\xC4\xF2\xF0\x21", 6); // CBike::PreRender
        aml->Write(pGTASA + 0x5658FC, (uintptr_t)"\x00\x20\xC4\xF2\xF0\x20", 6); // CBike::PreRender
        aml->Write(pGTASA + 0x573826, (uintptr_t)"\x00\x20\xC4\xF2\x96\x30", 6); // CHeli::SearchLightCone
        aml->Unprot(pGTASA + 0x55BAD0, sizeof(float)); *(float*)(pGTASA + 0x55BAD0) = 300.0f; // CAutomobile::PreRender
    }

    // Bigger distance for light shadows
    if(cfg->GetBool("BuffDistForLightShadows", true, "Visual"))
    {
        aml->Write(pGTASA + 0x36311C, (uintptr_t)"\xC4\xF2\xF0\x21", 4); // CTrafficLights::DisplayActualLight, 40 -> 120
        aml->Write(pGTASA + 0x3F1996, (uintptr_t)"\xC4\xF2\xF0\x21", 4); // CFireManager::Update, 40 -> 120
        fLightDist = 120.0f; // For thingies below \/
    }

    // Bring back light shadows from poles!
    if(cfg->GetBool("BackPolesLightShadow", true, "Visual"))
    {
        ProcessLightsForEntity_BackTo = pGTASA + 0x5A4DA8 + 0x1;
        aml->Redirect(pGTASA + 0x5A4578 + 0x1, (uintptr_t)ProcessLightsForEntity_Inject);
        HOOK(AddLight, aml->GetSym(hGTASA, "_ZN12CPointLights8AddLightEh7CVectorS0_ffffhbP7CEntity"));
    }

    // Fix greenish detail tex
    if(cfg->GetBool("FixGreenTextures", true, "Visual"))
    {
        aml->PlaceNOP(pGTASA + 0x1B00B0, 5); // Dont set textureDetail variable! We'll handle it by ourselves!
        HOOK(emu_TextureSetDetailTexture, aml->GetSym(hGTASA, "_Z27emu_TextureSetDetailTexturePvj"));
    }

    // Bring back light shadows from poles!
    if(cfg->GetBool("BuffStaticShadowsCount", true, "Gameplay"))
    {
        // Static shadows?
        asShadowsStored_NEW = new CRegisteredShadow[0xFF]; memset(asShadowsStored_NEW, 0, sizeof(CRegisteredShadow) * 0xFF);
        aStaticShadows_NEW = new CStaticShadow[0xFF] {0}; memset(aStaticShadows_NEW, 0, sizeof(CStaticShadow) * 0xFF);
        aml->Write(pGTASA + 0x677BEC, (uintptr_t)&asShadowsStored_NEW, sizeof(void*));
        aml->Write(pGTASA + 0x6798EC, (uintptr_t)&aStaticShadows_NEW, sizeof(void*));
        
        // Registered Shadows:
        // CShadows::StoreShadowToBeRendered
        aml->Write(pGTASA + 0x5B929A, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B92C0, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B92E6, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B930A, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B932E, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B9358, (uintptr_t)"\xFE", 1);
        // CShadows::StoreShadowToBeRendered (2nd arg is RwTexture*)
        aml->Write(pGTASA + 0x5B9444, (uintptr_t)"\xFE", 1);
        // CShadows::StoreShadowForVehicle
        aml->Write(pGTASA + 0x5B9BD4, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5B9B2A, (uintptr_t)"\xFE", 1);
        // CShadows::StoreShadowForPedObject
        aml->Write(pGTASA + 0x5B9F62, (uintptr_t)"\xFE", 1);
        // CShadows::StoreRealTimeShadow
        aml->Write(pGTASA + 0x5BA29E, (uintptr_t)"\xFE", 1);
        // CShadows::RenderExtraPlayerShadows
        aml->Write(pGTASA + 0x5BDDBA, (uintptr_t)"\xFE", 1);
        aml->Write(pGTASA + 0x5BDD5A, (uintptr_t)"\xFE", 1);

        // Static Shadows:
        // CShadows::StoreStaticShadow
        aml->Write(pGTASA + 0x5B8E28, (uintptr_t)"\xFF", 1);
        aml->Write(pGTASA + 0x5B88C6, (uintptr_t)"\xB1\xF5\x7F\x5F", 4); // CMP.W R1, #16320
        // CShadows::RenderStaticShadows
        aml->Write(pGTASA + 0x5BB898, (uintptr_t)"\xFF", 1);
        aml->Write(pGTASA + 0x5BB8AA, (uintptr_t)"\xFF", 1);
        // CShadows::UpdateStaticShadows
        aml->Write(pGTASA + 0x5BD2EC, (uintptr_t)"\xFF", 1);
        
        HOOK(RenderStaticShadows, aml->GetSym(hGTASA, "_ZN8CShadows19RenderStaticShadowsEb"));
        HOOK(InitShadows, aml->GetSym(hGTASA, "_ZN8CShadows4InitEv"));
    }

    // Move shadows closer to the ground
    if(cfg->GetBool("MoveShadowsToTheGround", true, "Visual"))
    {
        float fH = cfg->GetFloat("MoveShadowsToTheGround_Height", 0.02f, "Visual");
        aml->Unprot(pGTASA + 0x5A224C, sizeof(float)); *(float*)(pGTASA + 0x5A224C) = -fH;
        aml->Unprot(pGTASA + 0x5B3ED4, sizeof(float)); *(float*)(pGTASA + 0x5B3ED4) = fH;
        aml->Unprot(pGTASA + 0x5BB80C, sizeof(float)); *(float*)(pGTASA + 0x5BB80C) = fH;
        aml->Unprot(pGTASA + 0x5BC188, sizeof(float)); *(float*)(pGTASA + 0x5BC188) = fH;
        aml->Unprot(pGTASA + 0x5E03D0, sizeof(float)); *(float*)(pGTASA + 0x5E03D0) = fH;
    }

    // Radar
    if(cfg->GetBool("FixRadarStreaming", true, "Visual"))
    {
        aml->PlaceB(pGTASA + 0x44313A + 0x1, pGTASA + 0x443146 + 0x1);
    }

    // texture2D bias? In theory, this thing (below) is giving better FPS + better textures
    if(cfg->GetBool("NoShaderTextureBias", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5EAB20 + 52, (uintptr_t)"      ", 6);
        aml->Write(pGTASA + 0x5EAB94 + 52, (uintptr_t)"      ", 6);
        aml->Write(pGTASA + 0x5EAC96 + 75, (uintptr_t)"      ", 6);
        aml->Write(pGTASA + 0x5EABE8 + 53, (uintptr_t)"      ", 6);
        aml->Write(pGTASA + 0x5EABE8 + 99, (uintptr_t)"      ", 6);
    }

    // Sweet's roof is not that tasty anymore
    if(cfg->GetBool("FixClimbDying", true, "Gameplay"))
    {
        HOOKPLT(ClimbProcessPed, pGTASA + 0x66CC28);
    }

    // For new save only, fixes 3 bikes spawn that are inside each other
    if(cfg->GetBool("FixDrivingSchoolBikesSpawn", true, "SCMFixes"))
    {
        HOOKPLT(CreateCarGenerator, pGTASA + 0x672808);
    }

    // Fixing a crosshair position by very stupid math
    if(cfg->GetBool("FixCrosshair", true, "Visual"))
    {
        HOOKPLT(DrawCrosshair, pGTASA + 0x672880);
        //HOOK(CalculateAspectRatio_CrosshairFix, aml->GetSym(hGTASA, "_ZN5CDraw20CalculateAspectRatioEv"));
    }

    // Fixed cheats
    if(cfg->GetBool("FixCheats", true, "Gameplay"))
    {
        HOOKPLT(DoCheats, pGTASA + 0x675458);
        HOOKPLT(KBEvent, pGTASA + 0x6709B8);
        aml->Write(aml->GetSym(hGTASA, "_ZN6CCheat16m_aCheatHashKeysE"), (uintptr_t)CCheat__m_aCheatHashKeys, sizeof(CCheat__m_aCheatHashKeys));
    }
    
    // Save file loading crash fix
    if(cfg->GetBool("FixSaveLoadingCrash1", true, "Gameplay"))
    {
        HOOKPLT(EntMdlNoCreate, pGTASA + 0x673844);
    }
    
    // Fix Adjustable.cfg loading?
    // UPD: Introduced another glitch, so its unfixed. yet.
    // UD2: Fixed with a much better way. But another glitch arrived with X-coord shifting
    if(cfg->GetBool("FixAdjustableSizeLowering", true, "Visual"))
    {
        aml->Unprot(pGTASA + 0x28260C, sizeof(float)); *(float*)(pGTASA + 0x28260C) = 5.0f;
    }
    
    // Taxi lights (obviously)
    if(cfg->GetBool("TaxiLights", true, "Visual"))
    {
        SET_TO(SetTaxiLight, aml->GetSym(hGTASA, "_ZN11CAutomobile12SetTaxiLightEb"));
        HOOK(AutomobileRender, aml->GetSym(hGTASA, "_ZN11CAutomobile6RenderEv"));
    }
    
    // Minimap in interiors? Hell nah!
    if(cfg->GetBool("NoInteriorRadar", true, "Visual"))
    {
        HOOKPLT(DrawRadar, pGTASA + 0x66F618);
    }
    
    // Money have 8 digits now? Exciting!
    if(cfg->GetBool("PCStyledMoney", false, "Visual"))
    {
        DrawMoney_BackTo = pGTASA + 0x2BD260 + 0x1;
        aml->Redirect(pGTASA + 0x2BD258 + 0x1, (uintptr_t)DrawMoney_Inject);
    }
    
    // Idk how to fix it, yet
    //if(cfg->GetBool("ReColors", true, "Visual"))
    //{
    //    HOOK(GetIntColour, aml->GetSym(hGTASA, "_ZN11CHudColours12GetIntColourEh"));
    //    HOOK(GetRGBA, aml->GetSym(hGTASA, "_ZN11CHudColours7GetRGBAEh"));
    //}
    
    // Country. Rifle. Is. 3rd. Person.
    if(cfg->GetBool("FixCountryRifleAim", true, "Gameplay"))
    {
        // YES, THATS EXTREMELY EASY TO FIX, LMAO
        aml->PlaceNOP(pGTASA + 0x5378C0, 3);
        //aml->Write(pGTASA + 0x5378C0, (uintptr_t)"\xFF", 1);
        aml->Write(pGTASA + 0x53813C, (uintptr_t)"\xFF", 1); // 
    }
    
    // Haha, no gejmpat!!1
    if(cfg->GetBool("ForceTouchControls", false, "Gameplay"))
    {
        aml->Redirect(aml->GetSym(hGTASA, "_ZN4CHID12GetInputTypeEv"), (uintptr_t)ret0);
    }
    
    // Fix ped conversations are gone
    if(cfg->GetBool("FixPedConversation", true, "Gameplay"))
    {
        aml->PlaceB(pGTASA + 0x301BFE + 0x1, pGTASA + 0x301C0E + 0x1);
    }
    
    // Equipped parachute attacked anim fix
    if(cfg->GetBool("EquippedParaAttackAnimFix", true, "Visual"))
    {
        HOOK(ComputeDamageAnim, aml->GetSym(hGTASA, "_ZN12CEventDamage17ComputeDamageAnimEP4CPedb"));
    }
    
    // MixSets-SA: Hostile gangs
    if(cfg->GetBool("MIX_HostileGangs", true, "Gameplay"))
    {
        HOOKPLT(ProcessPedGroups, pGTASA + 0x670164);
        HOOKPLT(PedGroups_IsOnAMission, pGTASA + 0x670CDC);
    }
    
    // Water Quadrant
    int dist = cfg->GetInt("DetailedWaterDrawDistance", 48 * 5, "Visual");
    if(dist > 0)
    {
        if(dist < 24) dist = 24;
        *DETAILEDWATERDIST = dist;
    }
    
    // Peepo: Fix traffic lights
    if(cfg->GetBool("FixTrafficLights", true, "Visual"))
    {
        HOOK(TrFix_RenderEffects, aml->GetSym(hGTASA, "_Z13RenderEffectsv"));
        HOOK(TrFix_InitGame2nd, aml->GetSym(hGTASA, "_ZN5CGame5Init2EPKc"));
    }
    
    if(cfg->GetBool("DisableCloudSaves", false, "Gameplay"))
    {
        aml->Write(aml->GetSym(hGTASA, "UseCloudSaves"), (uintptr_t)"\x00", 1);
    }
    
    // Always show wanted stars even if we're not breakin the law
    if(cfg->GetBool("AlwaysDrawWantedStars", false, "Visual"))
    {
        aml->PlaceB(pGTASA + 0x2BDF82 + 0x1, pGTASA + 0x2BDFA4 + 0x1);
    }
    
    // Max mobilesc0,mobilesc1,...,mobilesc### for us
    mobilescCount = cfg->GetInt("MaxLoadingScreens", 7, "Visual");
    if(mobilescCount > 0 && mobilescCount != 7)
    {
        LoadSplashes_BackTo = pGTASA + 0x43AD00 + 0x1;
        aml->Redirect(pGTASA + 0x43ACEC + 0x1, (uintptr_t)LoadSplashes_Inject);
    }
    
    // A mistake by R* that overwrites "total num of X peds"
    if(cfg->GetBool("FixGangsCounterOverflow", true, "Gameplay"))
    {
        PedCountCalc_BackTo1 = pGTASA + 0x4D0CC2 + 0x1;
        PedCountCalc_BackTo2 = pGTASA + 0x4D0D0A + 0x1;
        aml->Redirect(pGTASA + 0x4D0CAE + 0x1, (uintptr_t)PedCountCalc_Inject1);
        aml->Redirect(pGTASA + 0x4D0CF6 + 0x1, (uintptr_t)PedCountCalc_Inject2);
    }
    
    // Some kind of "Sprint Everywhere"
    if(cfg->GetBool("SprintOnAnySurface", true, "Gameplay"))
    {
        aml->Redirect(aml->GetSym(hGTASA, "_ZN14SurfaceInfos_c12CantSprintOnEj"), (uintptr_t)ret0);
    }
    
    // An improved ForceDXT
    if(cfg->GetBool("ForceLoadDXT", false, "Gameplay"))
    {
        HOOK(LoadTexDBThumbs, aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat"));
    }
    
    // Vehicle sun glare
    if(cfg->GetBool("VehicleSunGlare", true, "Visual"))
    {
        SET_TO(DoSunGlare, aml->GetSym(hGTASA, "_ZN8CVehicle10DoSunGlareEv"));
        HOOK(RenderVehicle_SunGlare, aml->GetSym(hGTASA, "_ZN8CVehicle6RenderEv"));
    }
        
    aml->Unprot(pGTASA + 0x3C51E8, sizeof(float));
    *(float*)(pGTASA + 0x3C51E8) = 10000.0f * (1.0f / cfg->GetFloat("CameraZoomingSpeed", 2.5f, "Gameplay"));
    
    aml->Unprot(pGTASA + 0x3C51F0, sizeof(float));
    *(float*)(pGTASA + 0x3C51F0) = cfg->GetFloat("MinimalCameraZoomingFOV", 70.0f, "Gameplay");
    
    // Removes "plis give us 5 stars plis plis"
    if(cfg->GetBool("RemoveAskingToRate", true, "Others"))
    {
        aml->PlaceB(pGTASA + 0x345E9C + 0x1, pGTASA + 0x345EAE + 0x1);
    }
    
    // Remove "ExtraAirResistance" flag
    if(cfg->GetBool("NoExtraAirResistanceFlag", true, "Gameplay"))
    {
        aml->Redirect(aml->GetSym(hGTASA, "_ZN10CCullZones29DoExtraAirResistanceForPlayerEv"), (uintptr_t)ret0);
    }
    
    // Allow all radio tracks to be played!
    if(cfg->GetBool("NoRadioCuts", true, "Gameplay"))
    {
        aml->Redirect(aml->GetSym(hGTASA, "_Z14IsRemovedTracki"), (uintptr_t)ret0);
        aml->Redirect(pGTASA + 0x3A152A + 0x1, pGTASA + 0x3A1602 + 0x1); // QueueUpTracksForStation
        aml->Redirect(pGTASA + 0x3A35F6 + 0x1, pGTASA + 0x3A369A + 0x1); // ChooseMusicTrackIndex
        aml->Redirect(pGTASA + 0x3A37C2 + 0x1, pGTASA + 0x3A385E + 0x1); // ChooseIdentIndex
        aml->Redirect(pGTASA + 0x3A3A1E + 0x1, pGTASA + 0x3A3AA2 + 0x1); // ChooseAdvertIndex
        aml->Redirect(pGTASA + 0x3A4374 + 0x1, pGTASA + 0x3A4416 + 0x1); // ChooseTalkRadioShow
        aml->Redirect(pGTASA + 0x3A44D6 + 0x1, pGTASA + 0x3A4562 + 0x1); // ChooseDJBanterIndexFromList
    }
    
    // Make the grenade's collision smaller. Will this nade fit in this hole now?
    if(cfg->GetBool("SmallerGrenadeCollision", true, "Gameplay"))
    {
        HOOK(InitTempColModels, aml->GetSym(hGTASA, "_ZN14CTempColModels10InitialiseEv"));
        aml->Redirect(pGTASA + 0x4697CA + 0x1, (uintptr_t)LoadWeaponObject_Inject);
        LoadWeaponObject_BackTo = pGTASA + 0x4697D4 + 0x1;
        SET_TO(ColModelAllocateData, aml->GetSym(hGTASA, "_ZN9CColModel12AllocateDataEiiiiib"));
        SET_TO(ColSphereSet, aml->GetSym(hGTASA, "_ZN10CColSphere3SetEfRK7CVectorhhh"));
        SET_TO(ms_colModelWeapon, aml->GetSym(hGTASA, "_ZN14CTempColModels17ms_colModelWeaponE"));
    }
    
    // Fixes a weird glitch from there: https://github.com/multitheftauto/mtasa-blue/issues/1123
    if(cfg->GetBool("MTA_FixProjectiles", true, "Gameplay"))
    {
        aml->Redirect(pGTASA + 0x5D991E + 0x1, pGTASA + 0x5D9A04 + 0x1);
    }
    
    // Fixes a very slippery floor?
    if(cfg->GetBool("MTA_FixIceFloor", true, "Gameplay"))
    {
        //aml->Redirect(pGTASA + 0x4A1C3C + 0x1, pGTASA + 0x4A1C4E + 0x1);

        IceFloor_BackTo1 = pGTASA + 0x4A1C4E + 0x1; // skip
        IceFloor_BackTo2 = pGTASA + 0x4A1CA6 + 0x1; // continue
        aml->Redirect(pGTASA + 0x4A1C3C + 0x1, (uintptr_t)IceFloor_Inject);
    }

    // Can now use a gun!
    if(cfg->GetBool("HighFPSAimingWalkingFix", true, "Gameplay"))
    {
        aml->Unprot(pGTASA + 0x4DD9E8, sizeof(float));
        SET_TO(float_4DD9E8, pGTASA + 0x4DD9E8);
        HOOK(TaskSimpleUseGunSetMoveAnim, aml->GetSym(hGTASA, "_ZN17CTaskSimpleUseGun11SetMoveAnimEP4CPed"));
    }

    // AllowLicensePlatesForAllCars
    if(cfg->GetBool("AllowLicensePlatesForAllCars", true, "Visual"))
    {
        aml->PlaceNOP(pGTASA + 0x5822E6, 3);
        aml->PlaceNOP(pGTASA + 0x582398, 3);
    }

    // Game is checking if HP is < 1.0 but it may be lower!
    if(cfg->GetBool("AllowCrouchWith1HP", true, "Gameplay"))
    {
        aml->Write(pGTASA + 0x54316C, (uintptr_t)"\xB4", 1);
    }

    // Show muzzle flash for the last bullet in magazine
    if(cfg->GetBool("MuzzleFlashForLastBullet", true, "Visual"))
    {
        aml->PlaceNOP(pGTASA + 0x4DDCCA, 10);
    }

    // Renders shadows on all surfaces -> disables a flag
    if(cfg->GetBool("DrawShadowsOnAllSurfaces", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5BAE23, (uintptr_t)"\x00", 1);
    }

    // Spread fix
    if(cfg->GetBool("WeaponSpreadFix", true, "Gameplay"))
    {
        SET_TO(fPlayerAimRotRate, aml->GetSym(hGTASA, "fPlayerAimRotRate"));
        HOOK(FireInstantHit, aml->GetSym(hGTASA, "_ZN7CWeapon14FireInstantHitEP7CEntityP7CVectorS3_S1_S3_S3_bb"));
    }

    // Fixes farclip glitch with wall (wardumb be like)
    if(cfg->GetBool("FixFogWall", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5EB9D9, (uintptr_t)"\x31\x2E\x30\x30", 4);
    }

    // Fixes Corona sprites stretching at foggy weather
    if(cfg->GetBool("FixCoronasStretching", true, "Visual"))
    {
        aml->Write(pGTASA + 0x5A27EC, (uintptr_t)"\xB0\xEE\x44\x0A", 4);
    }

    // JuniorDjjr: Fix IS_CHAR_DEAD returning false even if health is 0.0
    if(cfg->GetBool("FixIsCharDead0HP", true, "SCMFixes"))
    {
        HOOKPLT(RunningScript_IsPedDead, pGTASA + 0x670D10);
    }

    // Sprint button after aiming and dropping to the water
    if(cfg->GetBool("FixSprintButtonSwimDisappear", true, "Gameplay"))
    {
        HOOKPLT(IsTargetingActiveForPlayer, pGTASA + 0x6708F0);
        HOOKPLT(IsTargetingActive, pGTASA + 0x674718);
    }

    // Guess by the name
    if(cfg->GetBool("FixParachuteLandingAnim", true, "Visual"))
    {
        HOOKPLT(PlayerInfoProcess_ParachuteAnim, pGTASA + 0x673E84);
    }

    // FX particles distance multiplier!
    float fxMultiplier = cfg->GetFloat("FxDistanceMult", 2.5f, "Visual");
    if(fxMultiplier != 1 && fxMultiplier > 0.1)
    {
        aml->Unprot(pGTASA + 0x368C7C, sizeof(float)); *(float*)(pGTASA + 0x368C7C) = 0.015625 * fxMultiplier;
        aml->Unprot(pGTASA + 0x36EC34, sizeof(float)); *(float*)(pGTASA + 0x36EC34) = 0.00390625 * fxMultiplier;
    }

    // A long awaiting radio animation... Why do you need it so bad?
    if(cfg->GetBool("BoatRadioAnimationFix", true, "Visual"))
    {
        BoatRadio_BackTo = pGTASA + 0x58C230 + 0x1;
        aml->Redirect(pGTASA + 0x58C224 + 0x1, (uintptr_t)BoatRadio_Inject);
    }

    // MixSets' DuckAnyWeapon
    if(cfg->GetBool("MIX_DuckAnyWeapon", true, "Gameplay"))
    {
        DuckAnyWeapon_BackTo1 = pGTASA + 0x543714 + 0x1; // ret 1
        DuckAnyWeapon_BackTo2 = pGTASA + 0x54376A + 0x1; // ret 0
        aml->Redirect(pGTASA + 0x5436EC + 0x1, (uintptr_t)DuckAnyWeapon_Inject);
    }

    // Fix camera zooming
    /*if(cfg->GetBool("FixCameraSniperZoomDist", true, "Gameplay"))
    {
        CamZoomProc_BackTo = pGTASA + 0x3C5160 + 0x1;
        aml->Redirect(pGTASA + 0x3C5158 + 0x1, (uintptr_t)CamZoomProc_Inject);

        FixSniperZoomingDistance_BackTo = pGTASA + 0x3C5060 + 0x1;
        aml->Redirect(pGTASA + 0x3C5054 + 0x1, (uintptr_t)FixSniperZoomingDistance_Inject);
        FixSniperZoomingDistance2_BackTo = pGTASA + 0x3C513C + 0x1;
        aml->Redirect(pGTASA + 0x3C5132 + 0x1, (uintptr_t)FixSniperZoomingDistance2_Inject);
    }*/
    
    // Fixes an opcode 08F8 which controls if the game should show you "Updated stats mate!"
    /*if(cfg->GetBool("FixOpcode08F8", true, "SCMFixes"))
    {
        HOOK(CheckForStatsMessage, aml->GetSym(hGTASA, "_ZN6CStats20CheckForStatsMessageEb"));
    }*/

    // Undone and so disabled
    /*if(cfg->GetBool("DamagedComponentsColorFix", true, "Visual"))
    {
        //HOOK(ChooseVehicleColour, aml->GetSym(hGTASA, "_ZN17CVehicleModelInfo19ChooseVehicleColourERhS0_S0_S0_i"));
        //HOOKPLT(SetComponentVisibility, pGTASA + 0x66ED8C);
        HOOK(PreRenderCar, aml->GetSym(hGTASA, "_ZN11CAutomobile9PreRenderEv"));
    }*/
    
    // JuniorDjjr, W.I.P.
    /*if(cfg->GetBool("FoodEatingModelFix", true, "Gameplay"))
    {
        HOOKPLT(PlayerInfoProcess_Food, pGTASA + 0x673E84);
    }*/
    
    // Michelle date: CTaskSimpleCarSetPedInAsPassenger?

    // No SetClumpAlpha for ped (probably fixes the problem with player's lighting when holding a weapon)
    //if(cfg->GetBool("FixPlayerLighting", true, "Visual"))
    //{
    //    aml->PlaceNOP(pGTASA + 0x4A2622 + 0x1, 2);
    //}
}