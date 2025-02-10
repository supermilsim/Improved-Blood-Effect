modded class SCR_CharacterDamageManagerComponent : ScriptedDamageManagerComponent
{

	ref map<string, string> bsSettings;
	

	IEntity currentCharacter;
	bool alreadyDestroyed = false;
	float timerBetweenSplatters;
	World world;
	BS_AnimatedBloodManager animatedBloodManager;
	HitZone lastHitzone;
	protected vector m_lastHitPosition;
	protected vector m_lastHitDirection;
	protected int m_lastHitNodeId;

	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		currentCharacter = owner;
		world = owner.GetWorld();
		animatedBloodManager = BS_AnimatedBloodManager.GetInstance();
		if (!animatedBloodManager)
			animatedBloodManager = BS_AnimatedBloodManager.Cast(GetGame().SpawnEntity(BS_AnimatedBloodManager, GetGame().GetWorld(), null));
		
	}
	
	
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		
		super.OnLifeStateChanged(previousLifeState, newLifeState);
		
		if (newLifeState == ECharacterLifeState.DEAD)
		{
			RemoveBleedingParticleEffect(lastHitzone);
			GetGame().GetCallqueue().CallLater(animatedBloodManager.deathNote, 500, false, currentCharacter,false);
			GetGame().GetCallqueue().CallLater(animatedBloodManager.SpawnGroundBloodpool, 3500, false, currentCharacter, m_lastHitPosition, m_lastHitDirection, m_lastHitNodeId);
		}
		
		if (newLifeState == ECharacterLifeState.ALIVE)
		{
			GetGame().GetCallqueue().CallLater(animatedBloodManager.deathNote, 500, false, currentCharacter,true);
		}
		
		
	}


	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		
		// EDamageType type,
		// float damage,
		// HitZone pHitZone,
		// IEntity instigator,
		// inout vector hitTransform[3],
		// float speed,
		// int colliderID,
		// int nodeID
		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(currentCharacter.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damageMgr)
			return;
		
		
		int colliderID = damageContext.colliderID;
		HitZone pHitZone = damageContext.struckHitZone;
		vector hitTransform[3];
		hitTransform[0] = damageContext.hitPosition;
		hitTransform[1] = damageContext.hitDirection;
		hitTransform[2] = damageContext.hitNormal;

		float damage = damageContext.damageValue;

		animatedBloodManager = BS_AnimatedBloodManager.GetInstance();

		int correctNodeId;
		int colliderDescriptorIndex = pHitZone.GetColliderDescriptorIndex(colliderID);
		pHitZone.TryGetColliderDescription(currentCharacter, colliderDescriptorIndex, null, null, correctNodeId);

		if (hitTransform[0].Length() != 0)
		{

			EDamageState currentState = GetState();
			
			// Save current transform to var
			m_lastHitPosition = damageContext.hitPosition;
			m_lastHitDirection = damageContext.hitDirection;
			m_lastHitNodeId = correctNodeId;
			
			if (damage > 20.0)
			{
				GetGame().GetCallqueue().CallLater(animatedBloodManager.SpawnWallSplatter , 150 ,false,currentCharacter, hitTransform[0], hitTransform[1]);
			}
			if (damage > 10.0)
			{

				SCR_CharacterHitZone tempHitZone = SCR_CharacterHitZone.Cast(pHitZone);
				GetGame().GetCallqueue().CallLater(animatedBloodManager.SpawnDroplets, 250, false, currentCharacter, hitTransform[0]);
			}
			
			
		}
		if (damageMgr.IsBleeding())
		{
		
			GetGame().GetCallqueue().CallLater(animatedBloodManager.isBleedingX, 500, false, currentCharacter, damage);
			
		}
			
	}

	
	
	
	
	
	//------------------------------------------------------------------------------------------------
	override void RemoveBleedingParticleEffect(HitZone hitZone)
	{
		if (!m_mBleedingParticles)
			return;

		ParticleEffectEntity particleEmitter = m_mBleedingParticles.Get(hitZone);
		if (particleEmitter)
		{
			particleEmitter.StopEmission();
			m_mBleedingParticles.Remove(hitZone);
		}

		if (m_mBleedingParticles.IsEmpty())
			m_mBleedingParticles = null;
	}
	override void CreateBleedingParticleEffect(notnull HitZone hitZone, int colliderDescriptorIndex)
	{
		if (System.IsConsoleApp())
			return;
		
		// Play Bleeding particle
		if (m_sBleedingParticle.IsEmpty())
			return;

		RemoveBleedingParticleEffect(hitZone);
		RemoveBleedingParticleEffect(lastHitzone);
		// TODO: Blood traces on ground that should be left regardless of clothing, perhaps just delayed
		SCR_CharacterHitZone characterHitZone = SCR_CharacterHitZone.Cast(hitZone);
		//if (characterHitZone.IsCovered())
		//	return;
		
		array<HitZone> groupHitZones = {};
		GetHitZonesOfGroup(characterHitZone.GetHitZoneGroup(), groupHitZones);
		float bleedingRate;
		
		foreach (HitZone groupHitZone : groupHitZones)
		{
			SCR_RegeneratingHitZone regenHitZone = SCR_RegeneratingHitZone.Cast(groupHitZone);
			if (regenHitZone)
				bleedingRate +=	regenHitZone.GetHitZoneDamageOverTime(EDamageType.BLEEDING);
		}
		
		if (bleedingRate == 0 || m_fBleedingParticleRateScale == 0)
			return;

		// Get bone node
		vector transform[4];
		int boneIndex;
		int boneNode;
		if (!hitZone.TryGetColliderDescription(GetOwner(), colliderDescriptorIndex, transform, boneIndex, boneNode))
			return;

		// Create particle emitter
		ParticleEffectEntitySpawnParams spawnParams();
		spawnParams.Parent = GetOwner();
		spawnParams.PivotID = boneNode;
		ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_sBleedingParticle, spawnParams);
		if (System.IsConsoleApp())
			return;		
		
		if (!particleEmitter)
		{
			Print("Particle emitter: " + particleEmitter.ToString() + " There was a problem with creating the particle emitter: " + m_sBleedingParticle, LogLevel.WARNING);
			return;
		}

		// Track particle emitter in array
		if (!m_mBleedingParticles)
			m_mBleedingParticles = new map<HitZone, ParticleEffectEntity>;

		m_mBleedingParticles.Insert(hitZone, particleEmitter);
		lastHitzone = hitZone;

		// Play particles
		Particles particles = particleEmitter.GetParticles();
		if (particles)
			particles.MultParam(-1, EmitterParam.BIRTH_RATE, bleedingRate * m_fBleedingParticleRateScale);
		else
			Print("Particle: " + particles.ToString() + " Bleeding particle likely not created properly: " + m_sBleedingParticle, LogLevel.WARNING);
	
	}

	override void RemoveBleedingFromArray(notnull HitZone hitZone)
	{
		super.RemoveBleedingFromArray(hitZone);

		animatedBloodManager = BS_AnimatedBloodManager.GetInstance();
	}

}

class DecalWrapper
{
	Decal wrappedDecal;

	void DecalWrapper(Decal d)
	{
		wrappedDecal = d;
	}
}