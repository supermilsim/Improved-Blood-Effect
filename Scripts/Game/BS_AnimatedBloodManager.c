class BS_AnimatedBloodManagerClass : GenericEntityClass
{
}

class BS_AnimatedBloodManager : GenericEntity
{

	static ref map<EDecalType, ref array<ResourceName>> materialsMap;
	//ref map<int, ref DecalInformation> decalsSpawned;
	ref map<ref EntityID, ref BloodTrailInfo> bloodTrailsInfoMap;
	ref DecalInformation lastDecalSpawned;
	int beforeNumber;
	bool newDecalSpawned = false;
	ref array<IEntity> bleedingCharacters;
	private ref array<Decal> allDecals = {};
	ref static array<DecalWrapper> currentPlayerDecals;
	ref array<DecalWrapper> currentCharacterDecals;


	
	ref map<string, string> settings;
	
	
	int materialColor; // todo make this dynamic
	
	int defaultLifeTime = -1;
	int defaultLifeTimeMilSec = 420000; //360000; // 6 min // 420000 // 7 min
	float counter;
	
	const int maxDropled = 100;
	int currentDropled;
	
	const int maxWallsplatter = 25;
	int currentWallsplatter;
	
	
	const int maxTrail = 250;
	int currentTrail;
	ref array<IEntity> deactiveTrails;
	
	const int maxPool = 20;
	int currentPool;
	
	
	bool iambleeding = false;
	//int someoneStillBleeding = 0;
	//IEntity character, vector hitPosition, float damage
	
	
	static BS_AnimatedBloodManager instance;
	
	ref array<Material> materialsLoaded;
	// ref array<Material> wallsplattersLoaded;
	// ref array<Material> genericSplattersLoaded;

	// would be better if it's all a static class? No we can't allocate stuff maybe? I dont remember
	static BS_AnimatedBloodManager GetInstance()
	{
		return instance;
	}

	void BS_AnimatedBloodManager(IEntitySource src, IEntity parent)
	{

		SetEventMask(EntityEvent.INIT | EntityEvent.FRAME);
		SetFlags(EntityFlags.ACTIVE, true);

		Math.Randomize(-1); // todo this is not really random.

		
		//decalsSpawned = new map<int, ref DecalInformation>();
		materialsMap = new map<EDecalType, ref array<ResourceName>>();

		materialsMap.Insert(EDecalType.BLOODPOOL, {"{4BEEAF470FDD2AED}Assets/Decals/Blood/Blood_Pool_Decal1.emat","{15068604B050A4E2}Assets/Decals/Blood/Blood_Pool_Decal2.emat","{E10EC19CBD8DCC96}Assets/Decals/Blood/Blood_Pool_Decal3.emat","{A8D6D483CF4BB8FC}Assets/Decals/Blood/Blood_Pool_Decal4.emat"});
		materialsMap.Insert(EDecalType.DROPLETS ,{"{F73C6C3193BCA499}Assets/Decals/Blood/Blood_Droplet_Decal0.emat","{99715268A6DD4A11}Assets/Decals/Blood/Blood_Droplet_Decal1.emat","{A18DC91C12B108C5}Assets/Decals/Blood/Blood_Droplet_Decal2.emat"});
		materialsMap.Insert(EDecalType.WALLSPLATTER, {"{BC4DA31342C146F4}Assets/Decals/Blood/Blood_Wallsplatters_Decal0.emat","{4845E48B4F1C2E80}Assets/Decals/Blood/Blood_Wallsplatters_Decal1.emat","{16ADCDC8F091A08F}Assets/Decals/Blood/Blood_Wallsplatters_Decal2.emat"});
		materialsMap.Insert(EDecalType.BLOODTRAIL ,{"{F73C6C3193BCA499}Assets/Decals/Blood/Blood_Droplet_Decal0.emat","{99715268A6DD4A11}Assets/Decals/Blood/Blood_Droplet_Decal1.emat","{A18DC91C12B108C5}Assets/Decals/Blood/Blood_Droplet_Decal2.emat","{C41FD463B6732744}Assets/Decals/Blood/Blood_Droplet_Decal3.emat","{8DC7C17CC4B5532E}Assets/Decals/Blood/Blood_Droplet_Decal4.emat"});
		
		materialsLoaded = new array<Material>();

		//Print("hi");
		foreach (ResourceName rsBloodPool : materialsMap.Get(EDecalType.BLOODPOOL))
			materialsLoaded.Insert(Material.GetOrLoadMaterial(rsBloodPool, 0));
		foreach (ResourceName rsWallSplatter : materialsMap.Get(EDecalType.WALLSPLATTER))
			materialsLoaded.Insert(Material.GetOrLoadMaterial(rsWallSplatter, 0));
		foreach (ResourceName rsBloodTrail : materialsMap.Get(EDecalType.BLOODTRAIL))
			materialsLoaded.Insert(Material.GetOrLoadMaterial(rsBloodTrail, 0));
		foreach (ResourceName rsBloodDroplet : materialsMap.Get(EDecalType.DROPLETS))
			materialsLoaded.Insert(Material.GetOrLoadMaterial(rsBloodDroplet, 0));
		
		instance = this;

		if (!deactiveTrails)
			deactiveTrails = new array<IEntity>();
		
		
		instance = this;

		materialColor = Color.FromRGBA(128, 0, 0, 255).PackToInt();

		currentCharacterDecals = new array<DecalWrapper>();
		bleedingCharacters = new array<IEntity>();

		if (!currentPlayerDecals)
			currentPlayerDecals = new array<DecalWrapper>();

		// MCF_Debug.dbgShapes = new array<Shape>();

		SetEventMask(EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner) //! EntityEvent.INIT
	{
		if (System.IsConsoleApp())
			return;
		
		super.EOnInit(owner);
		
		// Allocate it whenever called. When called, let's start.
	}


	void deathNote(IEntity character, bool erase)
	{
		
		int deadperson = deactiveTrails.Find(character);
		
		if (deadperson > -1)
		{
			if (erase == true)
			{
				deactiveTrails.Remove(deadperson);
			}
			return;
		}
		else
		{
			deactiveTrails.Insert(character);
			//Print("someone Dead");
		}
		
	}
	
	void deathNoteWipe()
	{
		int i;
		if (deactiveTrails.Count() > 0)
		{
			for (i = 0; deactiveTrails.Count(); i++)
			{
				IEntity character = deactiveTrails.Get(0);
				if (character)
				{
					
					deactiveTrails.Remove(0);
				}
				else if (character == null){
					deactiveTrails.Remove(0);
				
				}
			}
		}
	}
	
	void isBleedingX(IEntity character, float damage)
	{
		
		
		//Print(deactiveTrails.Find(character));
		if (deactiveTrails.Find(character) > -1)
		{
			return;
		}
		
		
		if (character != null)
		{
			SpawnBloodTrail(character, damage);
		}
		
		
		
	}
	
	bool coinflip() // a randomizer. funny name.
	{
		return Math.RandomInt(0,2);	
	}
	
	void failSafe() //prevents game crashes
	{
		currentDropled = 0;
		//currentWallsplatter = 0;
		//currentTrail = 0;
		//currentPool = 0;
		deathNoteWipe();
		//RemoveDecals();
		//Print("failsafe calisti");
		//Print(allDecals.Count());
		
	}
	
	void DecalArrayWipe() //clears the decal array
	{
		//Print("decal wipe incoming"); 
		GetGame().GetCallqueue().Remove(RemoveDecals);
		World tmpWorld = GetGame().GetWorld();
		
		int i;
		if (allDecals.Count() > 0)
		{
			for (i = 0; allDecals.Count() - 1 ; i++)
			{
				
				Decal decal = allDecals.Get(0);
				if (decal)
				{
					tmpWorld.RemoveDecal(decal);
					allDecals.Remove(0);
				}
				else
				{
					allDecals.Remove(0);
				
				}
			}
		}
		
		
		
	}
	
	void SpawnDecal(TraceParam traceParam, EDecalType type, vector origin, vector projection, float nearClip, float farClip, float angle, float size, float alphaTestValue = -1, float alphaMulValue = -1, int lifetime = defaultLifeTime)
	{
	
		
		if (traceParam.TraceEnt == null)
		{
			return;
		}
		
		EntityFlags entityFlags = traceParam.TraceEnt.GetFlags();
		//Print(traceParam.TraceEnt.Type());
		
		
		if (entityFlags & EntityFlags.STATIC || traceParam.TraceEnt.Type() == RoadEntity || traceParam.TraceEnt.Type() == GenericTerrainEntity)
		{
			// check defaultLifeTime.
			lifetime = defaultLifeTime;
			array<ResourceName> refResources = materialsMap.Get(type);
			int randomMaterialIndex = Math.RandomIntInclusive(0, refResources.Count() - 1);
			if (randomMaterialIndex == beforeNumber)
			{
				randomMaterialIndex = Math.RandomIntInclusive(0, refResources.Count() - 1);
				
			}
			beforeNumber = randomMaterialIndex;
			ResourceName materialResource = refResources[randomMaterialIndex];
			Material material = Material.GetMaterial(materialResource);
			
			
			Decal decal;
	
			if (traceParam.TraceEnt)
			{
				
				World tmpWorld = GetGame().GetWorld();
				decal = tmpWorld.CreateDecal(traceParam.TraceEnt, origin, projection, nearClip, farClip, angle, size, 1, materialResource, lifetime, materialColor);
				
				if (!decal)
					return;
				
				allDecals.Insert(decal);
				GetGame().GetCallqueue().CallLater(RemoveDecals, defaultLifeTimeMilSec, false, allDecals.Find(decal)); // resets itself every 10 seconds
				DecalBaseInfo decalBaseInfo = new DecalBaseInfo(decal, type, 0, size, angle, 1);
				DecalPositionInfo decalPositionInfo = new DecalPositionInfo(traceParam, origin, projection);
				MaterialInfo materialInfo = new MaterialInfo(alphaMulValue, alphaTestValue); // todo need to see the og values.... set them over here, this is wrong for now
	
				//if (alphaTestValue > 0)
				//	material.SetParam("AlphaTest", alphaTestValue);
				//if (alphaMulValue > 0)
				//	material.SetParam("AlphaMul", alphaMulValue);
	
				materialInfo.SetIndexAlphaMap(randomMaterialIndex);
				DecalInformation decalInformation = new DecalInformation(decalBaseInfo, decalPositionInfo, materialInfo);
	
				
			}
		}else{
			return;
		}
	}
	
	
	void RemoveDecals(int decalID = 0)
	{
		if (decalID < 0)
			return;	
		
		World tmpWorld = GetGame().GetWorld();
		Decal decal = allDecals.Get(decalID);
		
			
		if (decal)
		{
			tmpWorld.RemoveDecal(decal);
			//allDecals.Remove(decalID);
			allDecals.Set(decalID,null)
		}
	}
	
	
	void SpawnBloodTrail(IEntity character, float damage)
	{
		//Print("trail");
		
		if (character == null)
		{
			// sonra bakicam.
			return;
		}
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);
		  
		if (player.IsInVehicle())
			return;
	
		//if (currentTrail > maxTrail)
		//	return;
		
		
		
		SCR_CharacterDamageManagerComponent charDamageManagerComponent = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		SCR_CharacterControllerComponent charControllerComponent = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		if (!charDamageManagerComponent)
			return;
		
		float speed = charControllerComponent.GetVelocity().Length();
		
		bool shouldBleed = charDamageManagerComponent.IsBleeding() && (speed > 0.1);
		
		
		if (charDamageManagerComponent.IsBleeding())
		{
			
			GetGame().GetCallqueue().CallLater(isBleedingX, 350, false, character, damage);
		}
		
		
		
		if (!shouldBleed)
			return;
			
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		float angle;
		float size;
		float min,max;
		min = -0.22;
		max = 0.22;
		float x,z;
		
		
		vector origin;
		vector projection;
		vector intersectionPosition;
		x = Math.RandomFloatInclusive(min, max);
		z = Math.RandomFloatInclusive(min, max);
		
		origin = character.GetOrigin() + Vector(0, distance / 4, 0);
		TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), origin, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);
		origin = origin + Vector(x,0,z);
		projection = -traceParam.TraceNorm;
		size = Math.RandomFloatInclusive(0.3, 0.55);
		angle = Math.RandomFloatInclusive(-360, 360);
		
		//currentTrail += 1;
		SpawnDecal(traceParam, EDecalType.BLOODTRAIL, origin, projection, nearClip, farClip, angle, size, -1, -1);
		
		
		
	}
	
	void SpawnDroplets(IEntity character, vector hitPosition)
	{
		//Print("droplet");
		if (character == null)
		{
			return;
		}
		
		if (deactiveTrails.Find(character) > -1)
		{
			return;
		}
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);
		  
		if (player.IsInVehicle())
			return;
		
		currentDropled += 2;
		if (currentDropled > maxDropled)
			return;
		
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		float angle;
		float size;

		vector intersectionPosition;
		vector origin;
		vector projection;
		float min,max;
		min = -0.95;
		max = 0.95;
		float x,z;
		
		int i;

		for (i = 0; i < 2; i++) 
		{
			//y = Math.RandomFloatInclusive(min, max);
			x = Math.RandomFloatInclusive(min, max);
			z = Math.RandomFloatInclusive(min, max);
			origin = character.GetOrigin() + Vector(0, distance / 4, 0);
			TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), origin, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);
			
			projection = -traceParam.TraceNorm;
			
			
			// projection should be a bit randomized
	
			size = Math.RandomFloatInclusive(0.35, 0.6);
			angle = Math.RandomFloatInclusive(-360, 360);
			
			
			
			
			if (coinflip())
			{
				origin = origin + Vector(x,0,z);
			} else {
				origin = origin - Vector(x,0,z);
			}
			
			SpawnDecal(traceParam, EDecalType.DROPLETS, origin, projection, nearClip, farClip, angle, size, -1, -1);
		}
		
		
		
		

	}

	void SpawnWallSplatter(IEntity character, vector hitPosition, vector hitDirection)
	{
		//Print("wall");
		if (character == null)
		{
			return;
		}
		
		if (deactiveTrails.Find(character) > -1)
		{
			return;
		}
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);
		if (player.IsInVehicle())
		{
			return;
		}
		
		//currentWallsplatter += 1;
		//if (currentWallsplatter > maxWallsplatter)
		//	return;
		
		
		array<string> refResources;
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float angle;
		float size;
		const float alphaMulValue = 1.3;
		const float alphaTestValue = 0; 
		

		TraceParam traceParam;
		vector origin;
		vector projection;
		vector intersectionPosition;
		vector correctedDirection;

		// Check intersection and setup vectors
		traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), hitPosition, hitDirection, distance, TraceFlags.ENTS, intersectionPosition);
		origin = intersectionPosition - hitDirection * (2.0 / 4);

		float xProjection;
		float yProjection;
		float zProjection;
		if (hitDirection[0] < 0)
			xProjection = -0.1;
		else
			xProjection = 0.1;

		yProjection = 0.02;

		if (hitDirection[2] < 0)
			zProjection = -0.5;
		else
			zProjection = 0.5;

		projection = {xProjection, yProjection, zProjection};

		if (traceParam.TraceEnt)
		{
			// angle = Math.RandomFloatInclusive(-360, 360);
			size = Math.RandomFloatInclusive(0.8, 1.2);
			
			SpawnDecal(traceParam, EDecalType.WALLSPLATTER, origin, projection, nearClip, farClip, angle, size, alphaTestValue, alphaMulValue);
		}
	}

	void SpawnGroundBloodpool(IEntity character, vector hitPosition, vector hitDirection, int nodeId)
	{
		
		//Print("pool");
		
		
		
		//Print(character);
		if (character == null)
		{
			return;
		}
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);
		if (player.IsInVehicle())
		{
			return;
		}
			
	
		
		//if (currentPool > maxPool)
		//	return;
		
		array<string> refResources;
		Material material;

		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float angle;
		float size;
		const float alphaMulValue = 1.2;
		const float alphaTestValue = 0;

		TraceParam traceParam;
		vector origin;
		vector projection;
		vector intersectionPosition;
		vector correctedDirection;

		vector characterBoneMatrix[4];
		vector correctedHitPosition;

	
		size = 1.5;

		
		traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), hitPosition, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);


		if (character.GetAnimation().GetBoneMatrix(nodeId,characterBoneMatrix))
		{

			correctedHitPosition = character.CoordToParent(characterBoneMatrix[3]);

			// Correct origin and projection
			origin = correctedHitPosition;
			projection = -traceParam.TraceNorm;
		}
		else
		{
			origin = character.GetOrigin() + Vector(0, distance / 4, 0);
			projection = vector.Lerp(-traceParam.TraceNorm, hitDirection, 0.5);
		}
		//currentPool += 1;
		SpawnDecal(traceParam, EDecalType.BLOODPOOL, origin, projection, nearClip, farClip, angle, size, alphaTestValue, alphaMulValue);
	}


	

	// Helpers
	static TraceParam GetSurfaceIntersection(IEntity owner, World m_world, vector origin, vector direction, float distance, int flags, out vector intersectionPosition)
	{
		auto param = new TraceParam();
		param.Start = origin;
		param.End = origin + direction * distance;
		param.Flags = flags;
		param.Exclude = owner;
		float intersectionDistance = m_world.TraceMove(param, NULL) * distance;
		intersectionPosition = origin + (direction * intersectionDistance);
		return param;
	}

}

class DecalInformation
{

	ref DecalBaseInfo decalBaseInfo;
	ref DecalPositionInfo decalPositionInfo;
	ref MaterialInfo materialInfo;

	void DecalInformation(DecalBaseInfo baseInfo, DecalPositionInfo positionInfo, MaterialInfo matInfo)
	{
		this.decalBaseInfo = baseInfo;
		this.decalPositionInfo = positionInfo;
		this.materialInfo = matInfo;
	}

}

class DecalBaseInfo
{
	Decal decal;
	EDecalType type;

	int currentFrame;
	float size;
	float rotation;
	float currentAlphaMul;

private bool isTerrainOnly;

	void DecalBaseInfo(Decal p_decal, EDecalType p_type, int p_currentFrame, float p_size, int p_rotation, int p_currentAlphaMul)
	{
		this.decal = p_decal;
		this.type = p_type;
		this.currentFrame = p_currentFrame;
		this.size = p_size;
		this.rotation = p_rotation;
		this.currentAlphaMul = p_currentAlphaMul;
	}

	void SetIsTerrainOnly(bool val)
	{
		this.isTerrainOnly = val;
	}

	bool GetIsTerrainOnly()
	{
		return isTerrainOnly;
	}

}

class DecalPositionInfo
{
	ref TraceParam traceParam;
	vector hitPosition;
	vector hitDirection;
	vector originPosition;
	vector projectionDirection;

	void DecalPositionInfo(TraceParam tf, vector originPos, vector projDir)
	{
		this.traceParam = tf;

		this.originPosition = originPos;
		this.projectionDirection = projDir;
	}

}

class MaterialInfo
{
	float alphaMul;
	float alphaTest;

	int indexAlpha; // to be used only with alpha blended maps not normal animated decals

	void MaterialInfo(float am, float at)
	{
		alphaMul = am;
		alphaTest = at;
	}

	void SetIndexAlphaMap(int id)
	{
		this.indexAlpha = id;
	}

	int GetIndexAlphaMap()
	{
		return indexAlpha;
	}

}

enum EDecalType {
	BLOODPOOL,
	BLOODTRAIL,
	WALLSPLATTER,
	SINGLE_FRAME_GENERIC_SPLATTER,
	DROPLETS
}

class BloodTrailInfo : TrackDecalInfo
{
};