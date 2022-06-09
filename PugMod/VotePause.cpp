#include "precompiled.h"

CVotePause gVotePause;

void CVotePause::ClientDisconnected(edict_t* pEntity)
{
	auto EntityIndex = ENTINDEX(pEntity);

	memset(this->m_VotedPause[EntityIndex], false, sizeof(this->m_VotedPause[EntityIndex]));
}

bool CVotePause::Check(CBasePlayer* Player)
{
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		int State = gPugMod.GetState();

		if (State == PUG_STATE_FIRST_HALF || State == PUG_STATE_SECOND_HALF || State == PUG_STATE_OVERTIME)
		{
			return !gTask.Exists(PUG_TASK_EXEC);
		}

		gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));
	}

	return false;
}

void CVotePause::VotePause(CBasePlayer* Player)
{
	if (g_pGameRules)
	{
		if (this->Check(Player))
		{
			if (gPugMod.IsLive())
			{
				if (gCvars.GetVotePauseTime()->value)
				{
					if (this->m_PausedTeam == UNASSIGNED)
					{
						if (!CSGameRules()->IsFreezePeriod())
						{
							auto PlayerIndex = Player->entindex();

							if (!this->m_VotedPause[PlayerIndex][Player->m_iTeam])
							{
								this->m_VotedPause[PlayerIndex][Player->m_iTeam] = true;

								int VoteCount = 0;

								for (int i = 1; i <= gpGlobals->maxClients; ++i)
								{
									if (this->m_VotedPause[i][Player->m_iTeam])
									{
										VoteCount++;
									}
								}

								int VotesNeed = (int)(gPlayer.GetNum(Player->m_iTeam) * gCvars.GetVotePercentage()->value);
								int VotesLack = (VotesNeed - VoteCount);
								
								if (VotesLack)
								{
									gUtil.SayText(NULL, PlayerIndex, _T("\3%s\1 from voted for a timeout: \4%d\1 of \4%d\1 vote(s) to run timeout."), STRING(Player->edict()->v.netname), VoteCount, VotesNeed);
									gUtil.SayText(NULL, PlayerIndex, _T("Say \3.vote\1 for a timeout."));
								}
								else
								{
									this->m_PausedTeam = Player->m_iTeam;

									gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, _T("The \3%s\1 team paused the game."), PUG_MOD_TEAM_STR[this->m_PausedTeam]);

									gUtil.SayText(NULL, PlayerIndex, _T("Match will pause for \4%d\1 seconds on next round."), (int)gCvars.GetVotePauseTime()->value);
								}
							}
							else
							{
								gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("You already voted for a timeout."));
							}
						}
						else
						{
							gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to pause the match while is in freezetime period."));
						}
					}
					else
					{
						gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("The \3%s\1 team already paused the game."), PUG_MOD_TEAM_STR[this->m_PausedTeam]);
					}
				}
				else
				{
					gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to vote to pause the match."));
				}
			}
			else
			{
				gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));
			}
		}
	}
}

void CVotePause::RoundRestart()
{
	if (gPugMod.IsLive())
	{
		if (gCvars.GetVotePauseTime()->value > 0)
		{
			if (this->m_PausedTeam != UNASSIGNED)
			{
				gUtil.SetRoundTime((int)gCvars.GetVotePauseTime()->value, true);

				gTask.Create(PUG_TASK_PAUS, 0.5, true, (void*)this->VotePauseTimer);
			}
		}
	}
}

void CVotePause::RoundStart()
{
	if (this->m_PausedTeam != UNASSIGNED)
	{
		this->m_PausedTeam = UNASSIGNED;

		memset(this->m_VotedPause, false, sizeof(this->m_VotedPause));
	}
}

void CVotePause::VotePauseTimer()
{
	if (g_pGameRules)
	{
		time_t RemainTime = (time_t)(gCvars.GetVotePauseTime()->value - (gpGlobals->time - CSGameRules()->m_fRoundStartTime));

		if (RemainTime > 0)
		{
			struct tm* tm_info = localtime(&RemainTime);

			if (tm_info)
			{
				char Time[32] = { 0 };

				if (strftime(Time, sizeof(Time), _T("MATCH IS PAUSED\n%M:%S"), tm_info) > 0)
				{
					gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, -1.0, 0.2, 0, 0.6, 0.6), Time);
				}
			}
		}
		else
		{
			gTask.Remove(PUG_TASK_PAUS);

			gUtil.SetRoundTime((int)CVAR_GET_FLOAT("mp_freezetime"), true);
		}
	}
}