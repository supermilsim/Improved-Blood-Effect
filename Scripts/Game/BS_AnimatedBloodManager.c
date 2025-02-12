class BS_AnimatedBloodManagerClass : GenericEntityClass
{
}

class BS_AnimatedBloodManager : GenericEntity
{
    // Singleton Instance
    protected static BS_AnimatedBloodManager instance;

    // Material Map (Stores Decal Materials)
    protected static ref map<EDecalType, ref array<ResourceName>> materialsMap;

    // Blood Trail & Decal Management
    protected ref array<Decal> allDecals;
    protected ref array<IEntity> deactiveTrails;

    // Decal Tracking
    protected int beforeNumber;

    // Colors & Lifetime
    protected int materialColor;
    protected int defaultLifeTime = -1;
    protected int defaultLifeTimeMilSec = 420000; // 7 minutes 420000 ms

    // Blood Management Limits
    protected const int maxDropled = 100;
    protected int currentDropled;
    protected const int maxWallsplatter = 25;
    protected int currentWallsplatter;
    protected const int maxTrail = 250;
    protected int currentTrail;
    protected const int maxPool = 20;
    protected int currentPool;

    // -------------------------------------------------------------------------------------------------
    // Singleton Management
	static BS_AnimatedBloodManager GetInstance()
	{
		if (!instance)
		{
			World gameWorld = GetGame().GetWorld();
			if (!gameWorld)
				return null;

			instance = BS_AnimatedBloodManager.Cast(gameWorld.FindEntityByName("BS_AnimatedBloodManager"));

			if (!instance)
			{
				instance = BS_AnimatedBloodManager.Cast(GetGame().SpawnEntity(BS_AnimatedBloodManager, gameWorld, null));
				if (instance)
					BS_AnimatedBloodManager.SetInstance(instance);
			}
		}

		return instance;
	}

    static void SetInstance(BS_AnimatedBloodManager newInstance)
    {
        instance = newInstance;
    }

    // -------------------------------------------------------------------------------------------------
    // Constructor
	void BS_AnimatedBloodManager(IEntitySource src, IEntity parent)
	{
		if (!instance)
			instance = this;

		SetEventMask(EntityEvent.INIT | EntityEvent.FRAME);
		SetFlags(EntityFlags.ACTIVE, true);

		if (!materialsMap)
			materialsMap = new map<EDecalType, ref array<ResourceName>>();

		allDecals = new array<Decal>();
		deactiveTrails = new array<IEntity>();

		materialColor = Color.FromRGBA(128, 0, 0, 255).PackToInt();
		LoadMaterialReferences();
	}

    // -------------------------------------------------------------------------------------------------
    // Load Material References (.emat File Paths)
    protected void LoadMaterialReferences()
    {
        materialsMap.Insert(EDecalType.BLOODPOOL, {
            "{4BEEAF470FDD2AED}Assets/Decals/Blood/Blood_Pool_Decal1.emat",
            "{15068604B050A4E2}Assets/Decals/Blood/Blood_Pool_Decal2.emat",
            "{E10EC19CBD8DCC96}Assets/Decals/Blood/Blood_Pool_Decal3.emat",
            "{A8D6D483CF4BB8FC}Assets/Decals/Blood/Blood_Pool_Decal4.emat"
        });

        materialsMap.Insert(EDecalType.DROPLETS, {
            "{F73C6C3193BCA499}Assets/Decals/Blood/Blood_Droplet_Decal0.emat",
            "{99715268A6DD4A11}Assets/Decals/Blood/Blood_Droplet_Decal1.emat",
            "{A18DC91C12B108C5}Assets/Decals/Blood/Blood_Droplet_Decal2.emat"
        });

        materialsMap.Insert(EDecalType.WALLSPLATTER, {
            "{BC4DA31342C146F4}Assets/Decals/Blood/Blood_Wallsplatters_Decal0.emat",
            "{4845E48B4F1C2E80}Assets/Decals/Blood/Blood_Wallsplatters_Decal1.emat",
            "{16ADCDC8F091A08F}Assets/Decals/Blood/Blood_Wallsplatters_Decal2.emat"
        });

        materialsMap.Insert(EDecalType.BLOODTRAIL, {
            "{F73C6C3193BCA499}Assets/Decals/Blood/Blood_Droplet_Decal0.emat",
            "{99715268A6DD4A11}Assets/Decals/Blood/Blood_Droplet_Decal1.emat",
            "{A18DC91C12B108C5}Assets/Decals/Blood/Blood_Droplet_Decal2.emat",
            "{C41FD463B6732744}Assets/Decals/Blood/Blood_Droplet_Decal3.emat",
            "{8DC7C17CC4B5532E}Assets/Decals/Blood/Blood_Droplet_Decal4.emat"
        });
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
		// Ensure deactiveTrails array is initialized
		if (!deactiveTrails)
			deactiveTrails = new array<IEntity>();

		int deadperson = deactiveTrails.Find(character);

		if (deadperson > -1)
		{
			if (erase)
				deactiveTrails.Remove(deadperson);

			return;
		}

		deactiveTrails.Insert(character);
	}

	void deathNoteWipe()
	{
		deactiveTrails.Clear();
	}

	void isBleedingX(IEntity character)
	{
		//Print("BLEEDING");
		// Ensure deactiveTrails array is initialized
		if (!deactiveTrails)
			deactiveTrails = new array<IEntity>();

		// Early return if character is null
		if (!character)
			return;

		// Check if the character is already in deactiveTrails
		if (deactiveTrails.Find(character) > -1)
			return;

		// Spawn blood trail if character is valid
		//Print("SPAWNING BLOODTRAIL");
		SpawnBloodTrail(character);
	}

	bool coinflip() // A randomizer with a funny name.
	{
		return Math.RandomInt(0, 2) == 1; // Ensures correct boolean return
	}

	void failSafe()
	{
		currentDropled = 0;
		deathNoteWipe();
	}

	void DecalArrayWipe()
	{
		GetGame().GetCallqueue().Remove(RemoveDecals);
		World tmpWorld = GetGame().GetWorld();

		if (!allDecals || allDecals.IsEmpty())
			return;

		for (int i = allDecals.Count() - 1; i >= 0; i--)
		{
			Decal decal = allDecals.Get(i);
			if (decal)
				tmpWorld.RemoveDecal(decal);

			allDecals.Remove(i);
		}
	
		//Print(allDecals.Count()); Array size check
		
	}

	void SpawnDecal(TraceParam traceParam, EDecalType type, vector origin, vector projection, float nearClip, float farClip, float angle, float size, float alphaTestValue = -1, float alphaMulValue = -1, int lifetime = defaultLifeTime)
	{
	
		if (!traceParam || !traceParam.TraceEnt)
			return;

		EntityFlags entityFlags = traceParam.TraceEnt.GetFlags();
		if (!(entityFlags & EntityFlags.STATIC) && traceParam.TraceEnt.Type() != RoadEntity && traceParam.TraceEnt.Type() != GenericTerrainEntity)
			return;

		if (!materialsMap)
			return;

		array<ResourceName> refResources = materialsMap.Get(type);
		if (!refResources || refResources.IsEmpty())
			return;

		if (lifetime != -1)
			lifetime = defaultLifeTime;

		int randomMaterialIndex = Math.RandomIntInclusive(0, refResources.Count() - 1);
		if (refResources.Count() > 1)
		{
			int attemptLimit = 5;
			int attempt = 0;
			while (randomMaterialIndex == beforeNumber && attempt < attemptLimit)
			{
				randomMaterialIndex = Math.RandomIntInclusive(0, refResources.Count() - 1);
				attempt++;
			}
		}

		beforeNumber = randomMaterialIndex;
		ResourceName materialResource = refResources[randomMaterialIndex];

		World tmpWorld = GetGame().GetWorld();
		if (!tmpWorld)
			return;

		Decal decal = tmpWorld.CreateDecal(traceParam.TraceEnt, origin, projection, nearClip, farClip, angle, size, 1, materialResource, lifetime, materialColor);
		if (!decal)
			return;

		allDecals.Insert(decal);

		// Store the timestamp of when the decal was created
		int decalIndex = allDecals.Find(decal);
		if (decalIndex != -1)
		{
			GetGame().GetCallqueue().CallLater(RemoveDecals, defaultLifeTimeMilSec, false, decalIndex);
		}
	
	}

	void RemoveDecals(int decalID = -1)
	{
		//Print("REMOVE DECALS"); Remove decal func check
		if (!allDecals || allDecals.IsEmpty())
			return;

		World tmpWorld = GetGame().GetWorld();
		if (!tmpWorld)
			return;
		
		// Ensure decalID is within bounds and check its lifetime
		if (decalID < 0 || decalID >= allDecals.Count())
			return;

		Decal decal = allDecals.Get(decalID);
		if (decal)
			tmpWorld.RemoveDecal(decal);

		allDecals.Set(decalID, null);
	}

	void SpawnBloodTrail(IEntity character)
	{
		// Ensure character is valid
		if (!character)
			return;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);

		// Ensure character is not in a vehicle
		if (player && player.IsInVehicle())
			return;

		// Get character components
		SCR_CharacterDamageManagerComponent charDamageManagerComponent = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		SCR_CharacterControllerComponent charControllerComponent = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));

		// Ensure valid components
		if (!charDamageManagerComponent || !charControllerComponent)
			return;

		// Get movement speed
		float speed = charControllerComponent.GetVelocity().Length();
		bool shouldBleed = charDamageManagerComponent.IsBleeding() && (speed > 0.1);

		// Schedule another blood check
		if (charDamageManagerComponent.IsBleeding())
		{
			GetGame().GetCallqueue().CallLater(isBleedingX, 350, false, character);
		}

		// Exit if not bleeding
		if (!shouldBleed)
			return;

		// Constants and variables
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float min = -0.22;
		const float max = 0.22;

		float x = Math.RandomFloatInclusive(min, max);
		float z = Math.RandomFloatInclusive(min, max);
		float size = Math.RandomFloatInclusive(0.3, 0.55);
		float angle = Math.RandomFloatInclusive(-360, 360);

		// Set origin and trace intersection
		vector origin = character.GetOrigin() + Vector(0, distance / 4, 0);
		vector intersectionPosition;
		TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), origin, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);

		// Ensure traceParam is valid
		if (!traceParam)
			return;

		// Offset the origin slightly
		origin = origin + Vector(x, 0, z);
		vector projection = -traceParam.TraceNorm;

		// Spawn blood decal
		SpawnDecal(traceParam, EDecalType.BLOODTRAIL, origin, projection, nearClip, farClip, angle, size, -1, -1);
	}

	void SpawnDroplets(IEntity character, vector hitPosition)
	{
		// Ensure character is valid
		if (!character)
			return;

		// Ensure deactiveTrails array is initialized
		if (!deactiveTrails)
			deactiveTrails = new array<IEntity>();

		// Prevent spawning for deactivated trails
		if (deactiveTrails.Find(character) > -1)
			return;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);

		// Ensure player is valid and not in a vehicle
		if (player && player.IsInVehicle())
			return;

		// Manage droplet count
		currentDropled += 2;
		if (currentDropled > maxDropled)
			return;

		// Constants
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float min = -0.95;
		const float max = 0.95;

		// Variables
		float x, z, size, angle;
		vector intersectionPosition, origin, projection;

		// Spawn multiple droplets
		for (int i = 0; i < 2; i++)
		{
			x = Math.RandomFloatInclusive(min, max);
			z = Math.RandomFloatInclusive(min, max);

			// Set origin and trace intersection
			origin = character.GetOrigin() + Vector(0, distance / 4, 0);
			TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), origin, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);

			// Ensure traceParam is valid
			if (!traceParam)
				continue;

			projection = -traceParam.TraceNorm;

			// Randomize size and angle
			size = Math.RandomFloatInclusive(0.35, 0.6);
			angle = Math.RandomFloatInclusive(-360, 360);

			// Randomly offset origin using coinflip()
			vector offset = Vector(x, 0, z);
			if (coinflip())
				origin = origin + offset;
			else
				origin = origin - offset;

			// Spawn blood droplet decal
			SpawnDecal(traceParam, EDecalType.DROPLETS, origin, projection, nearClip, farClip, angle, size, -1, -1);
		}
	}

	void SpawnWallSplatter(IEntity character, vector hitPosition, vector hitDirection)
	{
		// Ensure character is valid
		if (!character)
			return;

		// Ensure deactiveTrails array is initialized
		if (!deactiveTrails)
			deactiveTrails = new array<IEntity>();

		// Prevent spawning for deactivated trails
		if (deactiveTrails.Find(character) > -1)
			return;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);

		// Ensure player is valid and not in a vehicle
		if (player && player.IsInVehicle())
			return;

		// Constants
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float alphaMulValue = 1.3;
		const float alphaTestValue = 0;

		// Variables
		float size, angle;
		vector origin, projection, intersectionPosition;

		// Check intersection and setup vectors
		TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), hitPosition, hitDirection, distance, TraceFlags.ENTS, intersectionPosition);

		// Ensure traceParam is valid
		if (!traceParam || !traceParam.TraceEnt)
			return;

		// Adjust origin
		origin = intersectionPosition - (hitDirection * (distance / 4));

		// Calculate projection vector (NO ternary operators)
		float xProjection, zProjection;
		if (hitDirection[0] < 0)
			xProjection = -0.1;
		else
			xProjection = 0.1;

		float yProjection = 0.02;

		if (hitDirection[2] < 0)
			zProjection = -0.5;
		else
			zProjection = 0.5;

		projection = Vector(xProjection, yProjection, zProjection);

		// Randomize angle and size
		angle = Math.RandomFloatInclusive(-360, 360);
		size = Math.RandomFloatInclusive(0.8, 1.2);

		// Spawn the wall splatter decal
		SpawnDecal(traceParam, EDecalType.WALLSPLATTER, origin, projection, nearClip, farClip, angle, size, alphaTestValue, alphaMulValue);
	}

	void SpawnGroundBloodpool(IEntity character, vector hitPosition, vector hitDirection, int nodeId)
	{
		// Ensure character is valid
		if (!character)
			return;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);

		// Ensure player is valid and not in a vehicle
		if (player && player.IsInVehicle())
			return;

		// Constants
		const float distance = 2.0;
		const float nearClip = 0;
		const float farClip = 2.0;
		const float alphaMulValue = 1.2;
		const float alphaTestValue = 0;

		// Variables
		float size = 1.5;
		float angle;
		vector origin, projection, intersectionPosition;
		vector characterBoneMatrix[4];
		vector correctedHitPosition;

		// Check intersection with the surface
		TraceParam traceParam = GetSurfaceIntersection(character, GetGame().GetWorld(), hitPosition, Vector(0, -1, 0), distance, TraceFlags.WORLD | TraceFlags.ENTS, intersectionPosition);

		// Ensure traceParam is valid
		if (!traceParam)
			return;

		// Check if the character has a valid animation bone matrix
		if (character.GetAnimation() && character.GetAnimation().GetBoneMatrix(nodeId, characterBoneMatrix))
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

		// Randomize angle
		angle = Math.RandomFloatInclusive(-360, 360);

		// Spawn the blood pool decal
		SpawnDecal(traceParam, EDecalType.BLOODPOOL, origin, projection, nearClip, farClip, angle, size, alphaTestValue, alphaMulValue);
	}

	// Helpers
	static TraceParam GetSurfaceIntersection(IEntity owner, World m_world, vector origin, vector direction, float distance, int flags, out vector intersectionPosition)
	{
		// Ensure world is valid
		if (!m_world)
			return null;

		// Ensure direction is valid
		if (direction.Length() == 0)
			return null;

		// Create TraceParam object
		TraceParam param = new TraceParam();
		if (!param)
			return null;

		param.Start = origin;
		param.End = origin + (direction * distance);
		param.Flags = flags;

		// Exclude owner if valid
		if (owner)
			param.Exclude = owner;

		// Perform trace and calculate intersection distance
		float intersectionDistance = m_world.TraceMove(param, NULL) * distance;
		intersectionPosition = origin + (direction * intersectionDistance);

		return param;
	}
}

class DecalInformation
{
    protected ref DecalBaseInfo decalBaseInfo;
    protected ref DecalPositionInfo decalPositionInfo;
    protected ref MaterialInfo materialInfo;

    void DecalInformation(DecalBaseInfo baseInfo, DecalPositionInfo positionInfo, MaterialInfo matInfo)
    {
        if (!baseInfo || !positionInfo || !matInfo)
            return;

        decalBaseInfo = baseInfo;
        decalPositionInfo = positionInfo;
        materialInfo = matInfo;
    }

    // Getters for encapsulation
    DecalBaseInfo GetDecalBaseInfo()
    {
        return decalBaseInfo;
    }

    DecalPositionInfo GetDecalPositionInfo()
    {
        return decalPositionInfo;
    }

    MaterialInfo GetMaterialInfo()
    {
        return materialInfo;
    }
}

class DecalBaseInfo
{
    protected Decal decal;
    protected EDecalType type;

    protected int currentFrame;
    protected float size;
    protected float rotation;
    protected float currentAlphaMul;
    protected bool isTerrainOnly;

    void DecalBaseInfo(Decal p_decal, EDecalType p_type, int p_currentFrame, float p_size, float p_rotation, float p_currentAlphaMul)
    {
        // Validate decal object before assignment
        if (!p_decal)
            return;

        decal = p_decal;
        type = p_type;
        currentFrame = p_currentFrame;
        size = p_size;
        rotation = p_rotation;
        currentAlphaMul = p_currentAlphaMul;
    }

    // Getter methods for encapsulation
    Decal GetDecal()
    {
        return decal;
    }

    EDecalType GetType()
    {
        return type;
    }

    int GetCurrentFrame()
    {
        return currentFrame;
    }

    float GetSize()
    {
        return size;
    }

    float GetRotation()
    {
        return rotation;
    }

    float GetCurrentAlphaMul()
    {
        return currentAlphaMul;
    }

    void SetIsTerrainOnly(bool val)
    {
        isTerrainOnly = val;
    }

    bool GetIsTerrainOnly()
    {
        return isTerrainOnly;
    }
}

class DecalPositionInfo
{
    protected ref TraceParam traceParam;
    protected vector originPosition;
    protected vector projectionDirection;

    void DecalPositionInfo(TraceParam tf, vector originPos, vector projDir)
    {
        // Ensure traceParam is valid
        if (!tf)
            return;

        traceParam = tf;
        originPosition = originPos;
        projectionDirection = projDir;
    }

    // Getters for encapsulation
    TraceParam GetTraceParam()
    {
        return traceParam;
    }

    vector GetOriginPosition()
    {
        return originPosition;
    }

    vector GetProjectionDirection()
    {
        return projectionDirection;
    }
}

class MaterialInfo
{
    protected float alphaMul;
    protected float alphaTest;
    protected int indexAlpha; // Used only with alpha blended maps, not normal animated decals

    void MaterialInfo(float am = 1.0, float at = 1.0) // Default values for safety
    {
        alphaMul = am;
        alphaTest = at;
    }

    void SetIndexAlphaMap(int id)
    {
        indexAlpha = id;
    }

    int GetIndexAlphaMap()
    {
        return indexAlpha;
    }

    float GetAlphaMul()
    {
        return alphaMul;
    }

    float GetAlphaTest()
    {
        return alphaTest;
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