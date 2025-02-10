modded class SCR_BaseGameMode : BaseGameMode
{

	override void StartGameMode()
	{

		super.StartGameMode();
		
		// Setup animatedBloodManager
		BS_AnimatedBloodManager animatedBloodManager;
		animatedBloodManager = BS_AnimatedBloodManager.GetInstance();
		if (!animatedBloodManager)
			animatedBloodManager = BS_AnimatedBloodManager.Cast(GetGame().SpawnEntity(BS_AnimatedBloodManager, GetGame().GetWorld(), null));
		
		//Print("fail safe burada");
		GetGame().GetCallqueue().CallLater(animatedBloodManager.failSafe , 30000 ,true); // resets itself every 30 seconds
		GetGame().GetCallqueue().CallLater(animatedBloodManager.DecalArrayWipe , 960000 ,true); // resets itself every 16 min 960000
		
	}
};