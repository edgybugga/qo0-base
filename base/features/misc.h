#pragma once
// used: winapi includes
#include "../common.h"

// used: usercmd
#include "../sdk/datatypes/usercmd.h"
// used: listener event function
#include "../sdk/interfaces/igameeventmanager.h"
// used: baseentity
#include "../sdk/entity.h"

class CMiscellaneous : public CSingleton<CMiscellaneous>
{
public:
	// Get
	void Run(CUserCmd* pCmd, CBaseEntity* pLocal, bool& bSendPacket);
	void Event(IGameEvent* pEvent, const FNV1A_t uNameHash);
	/* correct movement while anti-aiming */
	void MovementCorrection(CUserCmd* pCmd, const QAngle& angOldViewPoint) const;
	/* pasted movement fix */
	void PastedMovementCorrection(CUserCmd* cmd, QAngle old_angles, QAngle wish_angle);

	// Extra
	/* automatic shoot when pressed attack key */
	void AutoPistol(CUserCmd* pCmd, CBaseEntity* pLocal);
	/* dont send packets for a certain number of ticks */
	void FakeLag(CBaseEntity* pLocal, bool& bSendPacket);
	/* jump on da bug */
	void JumpBug(CBaseEntity* pLocal, CUserCmd* pCmd);
	/* bug on da edge */
	void EdgeBug(CBaseEntity* pLocal, CUserCmd* pCmd, QAngle& angView);

	/* bug on da edge or nuh? */
	bool bShouldEdgebug;
	/* where they at tho? */
	Vector wheredafeetat[4];
	bool greenfoot[4];
private:
	// Movement
	/* backup of player flags before prediction */
	int iFlagsBackup;
	/* backup of player z velocity before prediction */
	float flZVelBackup;
	/* da speeda da bug */
	float flBugSpeed;
	/* backup of our edgebug buttons */
	int iEdgebugButtons;
	/* automatic jump when steps on the ground */
	void BunnyHop(CUserCmd* pCmd, CBaseEntity* pLocal) const;
	/* strafes on optimal sides for maximum speed in air */
	void AutoStrafe(CUserCmd* pCmd, CBaseEntity* pLocal);
	/* we gotta do some stuff before the first enginepred in createmove */
	void PrePred(CUserCmd* pCmd, CBaseEntity* pLocal);
};
