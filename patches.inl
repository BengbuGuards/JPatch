extern "C" void adadad(void)
{
    asm("VMOV S6, S0");
} // This one is used internally by myself. Helps me to get patched values.

// Moon phases
int moon_alphafunc, moon_vertexblend, moon_alphaval;
uintptr_t MoonVisual_1_BackTo, MoonVisual_2_BackTo;
void (*gglBlendFunc)(GLenum, GLclampf);
extern "C" void MoonVisual_1(void)
{
    emu_glEnable(GL_ALPHA_TEST);

    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &moon_alphafunc);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &moon_vertexblend);
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &moon_alphaval);
    
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    SET_TO(emu_glAlphaFunc, *(void**)(pGTASA + 0x6BCBF8));
    SET_TO(gglBlendFunc, pGTASA + 0x1A1504);
    emu_glAlphaFunc(GL_ALWAYS, 0.5f);
    //gglBlendFunc();
}
extern "C" void MoonVisual_2(void)
{
    emu_glAlphaFunc(GL_ALWAYS, 0.5f);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)moon_alphafunc);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)moon_vertexblend);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)moon_alphaval);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCCOLOR);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);

    //emu_glDisable(GL_ALPHA_TEST);
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
    if((*(IsWTouched*)(self->vtable() + 80))(self) != false) // IsTouched
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
            anim->m_bitsFlag |= ANIMATION_FREEZE_LAST_FRAME;
            anim->m_fBlendDelta = -1000.0f;

            CTask* task = TaskConstructor();
            TaskStartNamedAnim(task, "PARA_Land", "PARACHUTE", ANIMATION_UNLOCK_LAST_FRAME | ANIMATION_PARTIAL | ANIMATION_TRANSLATE_Y | ANIMATION_TRANSLATE_X, 10.0f, -1, true, *ms_iActiveSequence > -1, false, false);
            SetTask(&ped->m_pIntelligence->m_TaskMgr, task, TASK_PRIMARY_PRIMARY, false);
        }
    }
    PlayerInfoProcess_ParachuteAnim(self, playerNum);
}

// Unused detonator anim
DECL_HOOKv(UseDetonator, CEntity* ent)
{
    UseDetonator(ent);

    if(ent->m_nType == eEntityType::ENTITY_TYPE_PED)
    {
        CTask* task = TaskConstructor();
        TaskStartNamedAnim(task, "BOMBER", "DETONATOR", ANIMATION_UNLOCK_LAST_FRAME | ANIMATION_PARTIAL, 4.0f, -1, true, *ms_iActiveSequence > -1, false, false);
        SetTask(&((CPed*)ent)->m_pIntelligence->m_TaskMgr, task, TASK_PRIMARY_PRIMARY, false);
    }
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