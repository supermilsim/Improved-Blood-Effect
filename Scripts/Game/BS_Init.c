modded class SCR_BaseGameMode : BaseGameMode
{
    override void StartGameMode()
    {
        super.StartGameMode();

        // Setup animatedBloodManager
        BS_AnimatedBloodManager animatedBloodManager = BS_AnimatedBloodManager.GetInstance();
        if (!animatedBloodManager)
        {
            animatedBloodManager = BS_AnimatedBloodManager.Cast(GetGame().SpawnEntity(BS_AnimatedBloodManager, GetGame().GetWorld(), null));

            // ✅ Use SetInstance() instead of direct access
            if (animatedBloodManager)
                BS_AnimatedBloodManager.SetInstance(animatedBloodManager);
        }

        // Ensure animatedBloodManager is valid before scheduling functions
        if (animatedBloodManager)
        {
            GetGame().GetCallqueue().CallLater(animatedBloodManager.failSafe, 30000, true); // Runs every 30 seconds
            GetGame().GetCallqueue().CallLater(animatedBloodManager.DecalArrayWipe, 960000, true); // Runs every 16 minutes
        }
    }
};
