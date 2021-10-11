// used: random_device, mt19937, uniform_int_distribution
#include <random>

#include "misc.h"
// used: global variables
#include "../global.h"
// used: cheat variables
#include "../core/variables.h"
// used: convar interface
#include "../core/interfaces.h"
// used: angle-vector calculations
#include "../utilities/math.h"
// used: hotkeys for movement features
#include "../utilities/inputsystem.h"
// used: prediction restore func for edgebug
#include "../features/prediction.h"

void CMiscellaneous::Run(CUserCmd* pCmd, CBaseEntity* pLocal, bool& bSendPacket)
{
	if (!pLocal->IsAlive())
		return;

	PrePred(pCmd, pLocal);

	// @credits: a8pure c:
	if (C::Get<bool>(Vars.bMiscNoCrouchCooldown))
		pCmd->iButtons |= IN_BULLRUSH;

	if (C::Get<bool>(Vars.bMiscBunnyHop))
		BunnyHop(pCmd, pLocal);

	if (C::Get<bool>(Vars.bMiscAutoStrafe))
		AutoStrafe(pCmd, pLocal);

	if (C::Get<bool>(Vars.bMiscRevealRanks) && pCmd->iButtons & IN_SCORE)
		I::Client->DispatchUserMessage(CS_UM_ServerRankRevealAll, 0U, 0, nullptr);
}

void CMiscellaneous::Event(IGameEvent* pEvent, const FNV1A_t uNameHash)
{
	if (!I::Engine->IsInGame())
		return;
}

void CMiscellaneous::MovementCorrection(CUserCmd* pCmd, const QAngle& angOldViewPoint) const
{
	static CConVar* cl_forwardspeed = I::ConVar->FindVar(XorStr("cl_forwardspeed"));

	if (cl_forwardspeed == nullptr)
		return;

	static CConVar* cl_sidespeed = I::ConVar->FindVar(XorStr("cl_sidespeed"));

	if (cl_sidespeed == nullptr)
		return;

	static CConVar* cl_upspeed = I::ConVar->FindVar(XorStr("cl_upspeed"));

	if (cl_upspeed == nullptr)
		return;

	// get max speed limits by convars
	const float flMaxForwardSpeed = cl_forwardspeed->GetFloat();
	const float flMaxSideSpeed = cl_sidespeed->GetFloat();
	const float flMaxUpSpeed = cl_upspeed->GetFloat();

	Vector vecForward = { }, vecRight = { }, vecUp = { };
	M::AngleVectors(angOldViewPoint, &vecForward, &vecRight, &vecUp);

	// we don't attempt on forward/right roll, and on up pitch/yaw
	vecForward.z = vecRight.z = vecUp.x = vecUp.y = 0.f;

	vecForward.NormalizeInPlace();
	vecRight.NormalizeInPlace();
	vecUp.NormalizeInPlace();

	Vector vecOldForward = { }, vecOldRight = { }, vecOldUp = { };
	M::AngleVectors(pCmd->angViewPoint, &vecOldForward, &vecOldRight, &vecOldUp);

	// we don't attempt on forward/right roll, and on up pitch/yaw
	vecOldForward.z = vecOldRight.z = vecOldUp.x = vecOldUp.y = 0.f;

	vecOldForward.NormalizeInPlace();
	vecOldRight.NormalizeInPlace();
	vecOldUp.NormalizeInPlace();

	const float flPitchForward = vecForward.x * pCmd->flForwardMove;
	const float flYawForward = vecForward.y * pCmd->flForwardMove;
	const float flPitchSide = vecRight.x * pCmd->flSideMove;
	const float flYawSide = vecRight.y * pCmd->flSideMove;
	const float flRollUp = vecUp.z * pCmd->flUpMove;

	// solve corrected movement
	const float x = vecOldForward.x * flPitchSide + vecOldForward.y * flYawSide + vecOldForward.x * flPitchForward + vecOldForward.y * flYawForward + vecOldForward.z * flRollUp;
	const float y = vecOldRight.x * flPitchSide + vecOldRight.y * flYawSide + vecOldRight.x * flPitchForward + vecOldRight.y * flYawForward + vecOldRight.z * flRollUp;
	const float z = vecOldUp.x * flYawSide + vecOldUp.y * flPitchSide + vecOldUp.x * flYawForward + vecOldUp.y * flPitchForward + vecOldUp.z * flRollUp;

	// clamp and apply corrected movement
	pCmd->flForwardMove = std::clamp(x, -flMaxForwardSpeed, flMaxForwardSpeed);
	pCmd->flSideMove = std::clamp(y, -flMaxSideSpeed, flMaxSideSpeed);
	pCmd->flUpMove = std::clamp(z, -flMaxUpSpeed, flMaxUpSpeed);
}

void CMiscellaneous::PastedMovementCorrection(CUserCmd* cmd, QAngle old_angles, QAngle wish_angle)
{// pasted from only god knows where
	if (old_angles.x != wish_angle.x || old_angles.y != wish_angle.y || old_angles.z != wish_angle.z)
	{
		Vector wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

		auto viewangles = old_angles;
		auto movedata = Vector(cmd->flForwardMove, cmd->flSideMove, cmd->flUpMove);
		viewangles.Normalize();

		if (!(G::pLocal->GetFlags() & FL_ONGROUND) && viewangles.z != 0.f)
			movedata.y = 0.f;

		M::AngleVectors(wish_angle, &wish_forward, &wish_right, &wish_up);
		M::AngleVectors(viewangles, &cmd_forward, &cmd_right, &cmd_up);

		auto v8 = sqrt(wish_forward.x * wish_forward.x + wish_forward.y * wish_forward.y), v10 = sqrt(wish_right.x * wish_right.x + wish_right.y * wish_right.y), v12 = sqrt(wish_up.z * wish_up.z);

		Vector wish_forward_norm(1.0f / v8 * wish_forward.x, 1.0f / v8 * wish_forward.y, 0.f),
			wish_right_norm(1.0f / v10 * wish_right.x, 1.0f / v10 * wish_right.y, 0.f),
			wish_up_norm(0.f, 0.f, 1.0f / v12 * wish_up.z);

		auto v14 = sqrt(cmd_forward.x * cmd_forward.x + cmd_forward.y * cmd_forward.y), v16 = sqrt(cmd_right.x * cmd_right.x + cmd_right.y * cmd_right.y), v18 = sqrt(cmd_up.z * cmd_up.z);

		Vector cmd_forward_norm(1.0f / v14 * cmd_forward.x, 1.0f / v14 * cmd_forward.y, 1.0f / v14 * 0.0f),
			cmd_right_norm(1.0f / v16 * cmd_right.x, 1.0f / v16 * cmd_right.y, 1.0f / v16 * 0.0f),
			cmd_up_norm(0.f, 0.f, 1.0f / v18 * cmd_up.z);

		auto v22 = wish_forward_norm.x * movedata.x, v26 = wish_forward_norm.y * movedata.x, v28 = wish_forward_norm.z * movedata.x, v24 = wish_right_norm.x * movedata.y, v23 = wish_right_norm.y * movedata.y, v25 = wish_right_norm.z * movedata.y, v30 = wish_up_norm.x * movedata.z, v27 = wish_up_norm.z * movedata.z, v29 = wish_up_norm.y * movedata.z;

		Vector correct_movement;
		correct_movement.x = cmd_forward_norm.x * v24 + cmd_forward_norm.y * v23 + cmd_forward_norm.z * v25 + (cmd_forward_norm.x * v22 + cmd_forward_norm.y * v26 + cmd_forward_norm.z * v28) + (cmd_forward_norm.y * v30 + cmd_forward_norm.x * v29 + cmd_forward_norm.z * v27);
		correct_movement.y = cmd_right_norm.x * v24 + cmd_right_norm.y * v23 + cmd_right_norm.z * v25 + (cmd_right_norm.x * v22 + cmd_right_norm.y * v26 + cmd_right_norm.z * v28) + (cmd_right_norm.x * v29 + cmd_right_norm.y * v30 + cmd_right_norm.z * v27);
		correct_movement.z = cmd_up_norm.x * v23 + cmd_up_norm.y * v24 + cmd_up_norm.z * v25 + (cmd_up_norm.x * v26 + cmd_up_norm.y * v22 + cmd_up_norm.z * v28) + (cmd_up_norm.x * v30 + cmd_up_norm.y * v29 + cmd_up_norm.z * v27);

		static CConVar* cl_forwardspeed = I::ConVar->FindVar(XorStr("cl_forwardspeed"));

		if (cl_forwardspeed == nullptr)
			return;

		static CConVar* cl_sidespeed = I::ConVar->FindVar(XorStr("cl_sidespeed"));

		if (cl_sidespeed == nullptr)
			return;

		static CConVar* cl_upspeed = I::ConVar->FindVar(XorStr("cl_upspeed"));

		if (cl_upspeed == nullptr)
			return;
		// get max speed limits by convars
		const float flMaxForwardSpeed = cl_forwardspeed->GetFloat();
		const float flMaxSideSpeed = cl_sidespeed->GetFloat();
		const float flMaxUpSpeed = cl_upspeed->GetFloat();

		correct_movement.x = std::clamp(correct_movement.x, -flMaxForwardSpeed, flMaxForwardSpeed);
		correct_movement.y = std::clamp(correct_movement.y, -flMaxSideSpeed, flMaxSideSpeed);
		correct_movement.z = std::clamp(correct_movement.z, -flMaxUpSpeed, flMaxUpSpeed);

		cmd->flForwardMove = correct_movement.x;
		cmd->flSideMove = correct_movement.y;
		cmd->flUpMove = correct_movement.z;

		cmd->iButtons &= ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);
		if (cmd->flSideMove != 0.0) {
			if (cmd->flSideMove <= 0.0)
				cmd->iButtons |= IN_MOVELEFT;
			else
				cmd->iButtons |= IN_MOVERIGHT;
		}

		if (cmd->flForwardMove != 0.0) {
			if (cmd->flForwardMove <= 0.0)
				cmd->iButtons |= IN_BACK;
			else
				cmd->iButtons |= IN_FORWARD;
		}
	}
}

void CMiscellaneous::AutoPistol(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	if (!pLocal->IsAlive())
		return;

	CBaseCombatWeapon* pWeapon = pLocal->GetWeapon();

	if (pWeapon == nullptr)
		return;

	const short nDefinitionIndex = pWeapon->GetItemDefinitionIndex();
	const CCSWeaponData* pWeaponData = I::WeaponSystem->GetWeaponData(nDefinitionIndex);

	// check for pistol and attack
	if (pWeaponData == nullptr || pWeaponData->bFullAuto || pWeaponData->nWeaponType != WEAPONTYPE_PISTOL || !(pCmd->iButtons & IN_ATTACK))
		return;

	if (pLocal->CanShoot(static_cast<CWeaponCSBase*>(pWeapon)))
		pCmd->iButtons |= IN_ATTACK;
	else
		pCmd->iButtons &= ~IN_ATTACK;
}

void CMiscellaneous::FakeLag(CBaseEntity* pLocal, bool& bSendPacket)
{
	if (!pLocal->IsAlive())
		return;

	INetChannel* pNetChannel = I::ClientState->pNetChannel;

	if (pNetChannel == nullptr)
		return;

	static CConVar* sv_maxusrcmdprocessticks = I::ConVar->FindVar(XorStr("sv_maxusrcmdprocessticks"));

	if (sv_maxusrcmdprocessticks == nullptr)
		return;

	/*
	 * @note: get max available ticks to choke
	 * 2 ticks reserved for server info else player can be stacked
	 * while antiaiming and fakelag is disabled choke only 1 tick
	 */
	const int iMaxCmdProcessTicks = C::Get<bool>(Vars.bMiscFakeLag) ? sv_maxusrcmdprocessticks->GetInt() - 2 :
		C::Get<bool>(Vars.bAntiAim) ? 1 : 0;

	// choke
	bSendPacket = I::ClientState->nChokedCommands >= iMaxCmdProcessTicks;
}

void CMiscellaneous::JumpBug(CBaseEntity* pLocal, CUserCmd* pCmd)
{
	static bool bShouldJumpNext = false;

	if (!IPT::IsKeyDown(C::Get<int>(Vars.iMiscJumpbugKey)))
		return;

	if (bShouldJumpNext)
	{
		pCmd->iButtons |= IN_JUMP;
		bShouldJumpNext = false;
		return;
	}

	if (!(iFlagsBackup & FL_ONGROUND) && (pLocal->GetFlags() & FL_ONGROUND))
	{
		pCmd->iButtons |= IN_DUCK;
		pCmd->iButtons &= ~IN_JUMP;
		bShouldJumpNext = true;
	}
}

void CMiscellaneous::EdgeBug(CBaseEntity* pLocal, CUserCmd* pCmd, QAngle& angView)
{// this method of doing it is kinda autistic and I know the code is a bit of clusterfuck but whatever
	if (!IPT::IsKeyDown(C::Get<int>(Vars.iMiscEdgebugKey)))
		return;

	if (iFlagsBackup & FL_ONGROUND)
		return; // imagine trying to edgebug while we on the ground lol (protip: u cunt)
	
	bShouldEdgebug = flZVelBackup < -flBugSpeed && round(pLocal->GetVelocity().z) == -round(flBugSpeed) && pLocal->GetMoveType() != MOVETYPE_LADDER;
	if (bShouldEdgebug) // we literally boutta bug on da edge lol
		return;

	int nCommandsPredicted = I::Prediction->Split->nCommandsPredicted;

	// backup original stuff that we change so we can restore later if no edgebug detek
	QAngle angViewOriginal = angView;
	QAngle angCmdViewOriginal = pCmd->angViewPoint;
	int iButtonsOriginal = pCmd->iButtons;
	Vector vecMoveOriginal;
	vecMoveOriginal.x = pCmd->flSideMove;
	vecMoveOriginal.y = pCmd->flForwardMove;

	// static static static static static
	static Vector vecMoveLastStrafe;
	static QAngle angViewLastStrafe;
	static QAngle angViewOld = angView;
	static QAngle angViewDeltaStrafe;
	static bool bAppliedStrafeLast = false;
	if (!bAppliedStrafeLast)
	{// we didn't strafe last time so it's safe to update these, if we did strafe we don't want to change them ..
		angViewLastStrafe = angView;
		vecMoveLastStrafe = vecMoveOriginal;
		angViewDeltaStrafe = (angView - angViewOld);
		angViewDeltaStrafe;
	}
	bAppliedStrafeLast = false;
	angViewOld = angView;

	for (int t = 0; t < 4; t++)
	{
		static int iLastType;
		if (iLastType)
		{
			t = iLastType;
			iLastType = 0;
		}
		CPrediction::Get().RestoreEntityToPredictedFrame(nCommandsPredicted - 1); // reset player to before engine prediction was ran (-1 because this whole function is only called after pred in cmove)
		if (iButtonsOriginal & IN_DUCK && t < 2) // if we already unducking then don't unduck pusi
			t = 2;
		bool bApplyStrafe = !(t % 2); // t == 0 || t == 2
		bool bApplyDuck = t > 1;

		// set base cmd values that we need
		pCmd->angViewPoint = angViewLastStrafe;
		pCmd->iButtons = iButtonsOriginal;
		pCmd->flSideMove = vecMoveLastStrafe.x;
		pCmd->flForwardMove = vecMoveLastStrafe.y;
		
		for (int i = 0; i < C::Get<int>(Vars.iMiscEdgebugPredAmnt); i++)
		{
			// apply cmd changes for prediction
			if (bApplyDuck)
				pCmd->iButtons |= IN_DUCK;
			else
				pCmd->iButtons &= ~IN_DUCK;
			if (bApplyStrafe)
			{
				pCmd->angViewPoint += angViewDeltaStrafe;
				pCmd->angViewPoint.Normalize();
				pCmd->angViewPoint.Clamp();
			}
			else
			{
				pCmd->flSideMove = 0.f;
				pCmd->flForwardMove = 0.f;
			}
			// run prediction
			CPrediction::Get().Start(pCmd, pLocal);
			CPrediction::Get().End(pCmd, pLocal);
			// check if we'd edgebug
			bShouldEdgebug = flZVelBackup < -flBugSpeed && round(pLocal->GetVelocity().z) == -round(flBugSpeed) && pLocal->GetMoveType() != MOVETYPE_LADDER;
			flZVelBackup = pLocal->GetVelocity().z;
			/* where da feet */
			wheredafeetat[t] = pLocal->GetOrigin();
			greenfoot[t] = bShouldEdgebug;
			/* da feet at */
			if (bShouldEdgebug)
			{// !!!EDGEBUG DETEKTET!!!
				iEdgebugButtons = pCmd->iButtons; // backup iButtons state
				if (bApplyStrafe)
				{// if we hit da bug wit da strafe we gotta make sure we strafe right
					bAppliedStrafeLast = true;
					angView = (angViewLastStrafe + angViewDeltaStrafe);
					angView.Normalize();
					angView.Clamp();
					angViewLastStrafe = angView;
					pCmd->flSideMove = vecMoveLastStrafe.x;
					pCmd->flForwardMove = vecMoveLastStrafe.y;
				}
				/* restore angViewPoint back to what it was, we only modified it for prediction purposes
				*  we use movefix instead of changing angViewPoint directly 
				*  so we have pSilent pEdgebug (perfect-silent; prediction-edgebug) ((lol))
				*/
				pCmd->angViewPoint = angCmdViewOriginal;
				iLastType = t;
				return;
			}

			if (pLocal->GetFlags() & FL_ONGROUND || pLocal->GetMoveType() == MOVETYPE_LADDER)
				break;
		}
	}

	/* if we got this far in the function then we won't hit an edgebug in any of the predicted scenarios
	*  so we gotta restore everything back to what it was originally
	*/
	pCmd->angViewPoint = angCmdViewOriginal;
	angView = angViewOriginal;
	pCmd->iButtons = iButtonsOriginal;
	pCmd->flSideMove = vecMoveOriginal.x;
	pCmd->flForwardMove = vecMoveOriginal.y;
}

void CMiscellaneous::BunnyHop(CUserCmd* pCmd, CBaseEntity* pLocal) const
{
	static CConVar* sv_autobunnyhopping = I::ConVar->FindVar(XorStr("sv_autobunnyhopping"));

	if (sv_autobunnyhopping->GetBool())
		return;

	if (pLocal->GetMoveType() == MOVETYPE_LADDER || pLocal->GetMoveType() == MOVETYPE_NOCLIP || pLocal->GetMoveType() == MOVETYPE_OBSERVER)
		return;

	std::random_device randomDevice;
	std::mt19937 generate(randomDevice());
	const std::uniform_int_distribution<> chance(0, 100);

	if (chance(generate) > C::Get<int>(Vars.iMiscBunnyHopChance))
		return;

	static bool bLastJumped = false, bShouldFake = false;

	if (!bLastJumped && bShouldFake)
	{
		bShouldFake = false;
		pCmd->iButtons |= IN_JUMP;
	}
	else if (pCmd->iButtons & IN_JUMP)
	{
		if (pLocal->GetFlags() & FL_ONGROUND || pLocal->GetFlags() & FL_PARTIALGROUND)
		{
			bLastJumped = true;
			bShouldFake = true;
		}
		else
		{
			pCmd->iButtons &= ~IN_JUMP;
			bLastJumped = false;
		}
	}
	else
	{
		bLastJumped = false;
		bShouldFake = false;
	}
}

void CMiscellaneous::AutoStrafe(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	if (pLocal->GetMoveType() == MOVETYPE_LADDER || pLocal->GetMoveType() == MOVETYPE_NOCLIP)
		return;

	if (pLocal->GetFlags() & FL_ONGROUND)
		return;

	static CConVar* cl_sidespeed = I::ConVar->FindVar(XorStr("cl_sidespeed"));

	if (cl_sidespeed == nullptr)
		return;

	pCmd->flSideMove = pCmd->sMouseDeltaX < 0 ? -cl_sidespeed->GetFloat() : cl_sidespeed->GetFloat();
}

void CMiscellaneous::PrePred(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	// gotta backup the flags before prediction for jumpbug (and edgejump if u want that)
	iFlagsBackup = pLocal->GetFlags();
	// gotta backup dat z vel
	flZVelBackup = pLocal->GetVelocity().z;

	if (bShouldEdgebug) // if we predicted an edgebug while crouching last tick then crouch again (mainly to reduce weird crouchspam shit)
		pCmd->iButtons = iEdgebugButtons;

	static CConVar* sv_gravity = I::ConVar->FindVar(XorStr("sv_gravity"));
	flBugSpeed = (sv_gravity->GetFloat() * 0.5f * I::Globals->flIntervalPerTick); // fun fact: edgebug falling speed is half a tick worth of gravity (i think lol)
	
	static float flZVelPrev = flZVelBackup;
	bShouldEdgebug = flZVelPrev < -flBugSpeed && round(pLocal->GetVelocity().z) == -round(flBugSpeed) && pLocal->GetMoveType() != MOVETYPE_LADDER;
	flZVelPrev = flZVelBackup;
	
	if (bShouldEdgebug && C::Get<bool>(Vars.bMiscEdgebug) && IPT::IsKeyDown(C::Get<int>(Vars.iMiscEdgebugKey)) && C::Get<bool>(Vars.bMiscEdgebugIndicator))
	{// indicator shit
		pLocal->HealthShotBoost() = I::Globals->flCurrentTime + 1;
		I::Surface->PlaySoundSurface(XorStr("buttons\\blip1.wav"));
	}

	// reset feet box indicators if we're on ground, nice to look at where your actual feet collider is when standing on stuff
	if (pLocal->GetFlags() & FL_ONGROUND)
		for (int i = 0; i < 4; i++)
			if (!greenfoot[i])
				wheredafeetat[i] = pLocal->GetOrigin();
}
