modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
    protected BS_AnimatedBloodManager animatedBloodManager;
    protected SCR_CharacterDamageManagerComponent characterDamageManagerComponent;

    override void OnDeath(IEntity instigatorEntity, notnull Instigator instigator)
    {
        super.OnDeath(instigatorEntity, instigator);

        // Ensure animatedBloodManager is initialized
        if (!animatedBloodManager)
        {
            animatedBloodManager = BS_AnimatedBloodManager.GetInstance();
            if (!animatedBloodManager)
            {
                animatedBloodManager = BS_AnimatedBloodManager.Cast(GetGame().SpawnEntity(BS_AnimatedBloodManager, GetGame().GetWorld(), null));
            }
        }
    }
}
