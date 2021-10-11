#include "prediction.h"

#include "../utilities/logging.h"
// used: md5 checksum
#include "../sdk/hash/md5.h"
// used: movehelper, prediction, globals interfaces
#include "../core/interfaces.h"

void CPrediction::Start(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	// @xref: "CPrediction::ProcessMovement"

	if (!pLocal->IsAlive() || I::MoveHelper == nullptr)
		return;

	bInPrediction = true;

	// start command
	*pLocal->GetCurrentCommand() = pCmd;
	pLocal->GetLastCommand() = *pCmd;

	// random_seed isn't generated in ClientMode::CreateMove yet, we must generate it ourselves
	*uPredictionRandomSeed = MD5::PseudoRandom(pCmd->iCommandNumber) & std::numeric_limits<int>::max();
	// set ourselves as a predictable entity
	*pSetPredictionEntity = pLocal;

	// backup globals
	flOldCurrentTime = I::Globals->flCurrentTime;
	flOldFrameTime = I::Globals->flFrameTime;
	iOldTickCount = I::Globals->iTickCount;

	// backup tickbase
	const int iOldTickBase = pLocal->GetTickBase();

	// backup prediction states
	const bool bOldIsFirstPrediction = I::Prediction->bIsFirstTimePredicted;
	const bool bOldInPrediction = I::Prediction->bInPrediction;

	// set corrected values
	I::Globals->flCurrentTime = TICKS_TO_TIME(GetTickbase(pCmd, pLocal));
	I::Globals->flFrameTime = I::Prediction->bEnginePaused ? 0.f : TICK_INTERVAL;
	I::Globals->iTickCount = GetTickbase(pCmd, pLocal);

	I::Prediction->bIsFirstTimePredicted = false;
	I::Prediction->bInPrediction = true;

	/* skipped weapon select and vehicle predicts */

	if (pCmd->uImpulse)
		*pLocal->GetImpulse() = pCmd->uImpulse;

	// synchronize m_afButtonForced & m_afButtonDisabled
	pCmd->iButtons |= pLocal->GetButtonForced();
	pCmd->iButtons &= ~(pLocal->GetButtonDisabled());

	// update button state
	const int iButtons = pCmd->iButtons;
	int* nPlayerButtons = pLocal->GetButtons();
	const int nButtonsChanged = iButtons ^ *nPlayerButtons;

	// synchronize m_afButtonLast
	pLocal->GetButtonLast() = *nPlayerButtons;

	// synchronize m_nButtons
	*pLocal->GetButtons() = iButtons;

	// synchronize m_afButtonPressed
	pLocal->GetButtonPressed() = iButtons & nButtonsChanged;

	// synchronize m_afButtonReleased
	pLocal->GetButtonReleased() = nButtonsChanged & ~iButtons;

	// check if the player is standing on a moving entity and adjusts velocity and basevelocity appropriately
	I::Prediction->CheckMovingGround(pLocal, I::Globals->flFrameTime);

	// copy angles from command to player
	I::Prediction->SetLocalViewAngles(pCmd->angViewPoint);

	// run prethink
	if (pLocal->PhysicsRunThink(THINK_FIRE_ALL_FUNCTIONS))
		pLocal->PreThink();

	// run think
	if (int* iNextThinkTick = pLocal->GetNextThinkTick(); *iNextThinkTick > 0 && *iNextThinkTick <= GetTickbase(pCmd, pLocal))
	{
		*iNextThinkTick = TICK_NEVER_THINK;
		pLocal->Think();
	}

	// set host player
	I::MoveHelper->SetHost(pLocal);

	// setup move
	I::Prediction->SetupMove(pLocal, pCmd, I::MoveHelper, &moveData);
	I::GameMovement->ProcessMovement(pLocal, &moveData);

	// finish move
	I::Prediction->FinishMove(pLocal, pCmd, &moveData);
	I::MoveHelper->ProcessImpacts();

	// run post think
	pLocal->PostThink();

	// restore tickbase
	pLocal->GetTickBase() = iOldTickBase;

	// restore prediction states
	I::Prediction->bInPrediction = bOldInPrediction;
	I::Prediction->bIsFirstTimePredicted = bOldIsFirstPrediction;
}

void CPrediction::End(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	if (!pLocal->IsAlive() || I::MoveHelper == nullptr)
		return;

	// reset host player
	I::MoveHelper->SetHost(nullptr);

	// restore globals
	I::Globals->flCurrentTime = flOldCurrentTime;
	I::Globals->flFrameTime = flOldFrameTime;
	I::Globals->iTickCount = iOldTickCount;

	// finish command
	*pLocal->GetCurrentCommand() = nullptr;

	// reset prediction seed
	*uPredictionRandomSeed = -1;

	// reset prediction entity
	*pSetPredictionEntity = nullptr;

	// reset move
	I::GameMovement->Reset();

	bInPrediction = false;
}

void CPrediction::RestoreEntityToPredictedFrame(int predicted_frame)
{
	/* thanks to sreb from uc
	* https://www.unknowncheats.me/forum/3208633-post13.html
	*/

	using RestoreEntityToPredictedFrameFn = void(__stdcall*)(int, int);
	static auto fn = reinterpret_cast<RestoreEntityToPredictedFrameFn>(MEM::FindPattern(CLIENT_DLL, XorStr("55 8B EC A1 ? ? ? ? 56 8B 75 08 83")));
	fn(0, predicted_frame);
}

int CPrediction::GetTickbase(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	static int iTick = 0;
	static CUserCmd* pLastCmd = nullptr;

	if (pCmd != nullptr)
	{
		// if command was not predicted - increment tickbase
		if (pLastCmd == nullptr || pLastCmd->bHasBeenPredicted)
			iTick = pLocal->GetTickBase();
		else
			iTick++;

		pLastCmd = pCmd;
	}

	return iTick;
}
