/*
REWRITTEN FROM SCRATCH BY XINEF, IT OWNS NOW!
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"

enum Says
{
    SAY_AGGRO           = 0,
    SAY_GREET           = 1,
    SAY_SLAY            = 2,
};

enum Spells
{
	SPELL_IMPALE_10					= 28783,
	SPELL_IMPALE_25					= 56090,
	SPELL_LOCUST_SWARM_10			= 28785,
	SPELL_LOCUST_SWARM_25			= 54021,
	SPELL_SUMMON_CORPSE_SCRABS_5	= 29105,
	SPELL_SUMMON_CORPSE_SCRABS_10	= 28864,
	SPELL_BERSERK					= 26662,
};

enum Events
{
	EVENT_SPELL_IMPALE				= 1,
	EVENT_SPELL_LOCUST_SWARM		= 2,
	EVENT_SPELL_BERSERK				= 3,
};

enum Misc
{
	NPC_CORPSE_SCARAB				= 16698,
	NPC_CRYPT_GUARD					= 16573,

	ACHIEV_TIMED_START_EVENT		= 9891,
};

class boss_anubrekhan : public CreatureScript
{
public:
    boss_anubrekhan() : CreatureScript("boss_anubrekhan") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_anubrekhanAI (pCreature);
    }

	struct boss_anubrekhanAI : public ScriptedAI
	{
		boss_anubrekhanAI(Creature *c) : ScriptedAI(c), summons(me)
		{
			pInstance = c->GetInstanceScript();
			sayGreet = false;
		}

		InstanceScript* pInstance;
		EventMap events;
		SummonList summons;
		bool sayGreet;

		void SummonCryptGuards()
		{
			me->SummonCreature(NPC_CRYPT_GUARD, 3308.590f, -3466.29f, 287.16f, M_PI, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
			if (IsHeroic())
				me->SummonCreature(NPC_CRYPT_GUARD, 3308.590f, -3486.29f, 287.16f, M_PI, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
		}

		void Reset() 
		{
			events.Reset();
			summons.DespawnAll();
			SummonCryptGuards();

			if (pInstance)
			{
				pInstance->SetData(EVENT_ANUB, NOT_STARTED);
				if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_ANUB_GATE)))
					go->SetGoState(GO_STATE_ACTIVE);
			}
		}

		void JustSummoned(Creature* cr)
		{
			if (me->IsInCombat())
				cr->SetInCombatWithZone();
			if (cr->GetEntry() == NPC_CORPSE_SCARAB)
			{
				cr->SetReactState(REACT_PASSIVE);
				if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
					cr->AI()->AttackStart(target);
			}

			summons.Summon(cr);
		}

		void SummonedCreatureDies(Creature* cr, Unit*)
		{
			if (cr->GetEntry() == NPC_CRYPT_GUARD)
				cr->CastSpell(cr, SPELL_SUMMON_CORPSE_SCRABS_10, true, NULL, NULL, me->GetGUID());
		}

		void SummonedCreatureDespawn(Creature* cr) { summons.Despawn(cr); }

		void JustDied(Unit* Killer)
		{
			summons.DespawnAll();
			if (pInstance)
			{
				pInstance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
				pInstance->SetData(EVENT_ANUB, DONE);
			}
		}

		void KilledUnit(Unit* victim)
		{
			if (victim->GetTypeId() != TYPEID_PLAYER)
				return;

			if (!urand(0,3))
				Talk(SAY_SLAY);

			//Force the player to spawn corpse scarabs via spell
			victim->CastSpell(victim, SPELL_SUMMON_CORPSE_SCRABS_5, true, NULL, NULL, me->GetGUID());

			if (pInstance)
				pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
		}

		void EnterCombat(Unit *who)
		{
			me->CallForHelp(30.0f); // catch helpers
			Talk(SAY_AGGRO);
			if (pInstance)
			{
				pInstance->SetData(EVENT_ANUB, IN_PROGRESS);
				if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_ANUB_GATE)))
					go->SetGoState(GO_STATE_READY);
			}

			events.ScheduleEvent(EVENT_SPELL_IMPALE, 15000);
			events.ScheduleEvent(EVENT_SPELL_LOCUST_SWARM, 70000+urand(0,50000));
			events.ScheduleEvent(EVENT_SPELL_BERSERK, 600000);
			
			if (!summons.HasEntry(NPC_CRYPT_GUARD))
				SummonCryptGuards();
		}

		void MoveInLineOfSight(Unit *who)
		{
			if (!sayGreet && who->GetTypeId() == TYPEID_PLAYER)
			{
				Talk(SAY_GREET);
				sayGreet = true;
			}

			ScriptedAI::MoveInLineOfSight(who);
		}

		void UpdateAI(uint32 diff)
		{
			if (!UpdateVictim())
				return;

			events.Update(diff);
			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			switch (events.GetEvent())
			{
				case EVENT_SPELL_IMPALE:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
						me->CastSpell(target, RAID_MODE(SPELL_IMPALE_10, SPELL_IMPALE_25), false);
					events.RepeatEvent(20000);
					break;
				case EVENT_SPELL_LOCUST_SWARM:
				{
                    me->CastSpell(me, RAID_MODE(SPELL_LOCUST_SWARM_10, SPELL_LOCUST_SWARM_25), false);
                    Position pos = me->GetNearPosition(10.0f, rand_norm() * 2 * M_PI);
                    me->SummonCreature(NPC_CRYPT_GUARD, pos);
                    events.RepeatEvent(90000);
                    break;
				}
				case EVENT_SPELL_BERSERK:
					me->CastSpell(me, SPELL_BERSERK, true);
					events.PopEvent();
					break;
			}

			DoMeleeAttackIfReady();
		}
	};
};

void AddSC_boss_anubrekhan()
{
	new boss_anubrekhan();
}

