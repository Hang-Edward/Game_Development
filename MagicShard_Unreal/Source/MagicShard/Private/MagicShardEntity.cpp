#include "MagicShardEntity.h"

UMagicShardEntity::UMagicShardEntity()
    : EntityId(0),
      EntityName(TEXT("Entity")),
      bActive(true)
{
}

FString UMagicShardEntity::GetDisplayName() const
{
    return EntityName;
}

FString UMagicShardEntity::DescribeEntity() const
{
    return FString::Printf(TEXT("Entity[%d] %s Active=%s"), EntityId, *EntityName, bActive ? TEXT("true") : TEXT("false"));
}

void UMagicShardEntity::TickEntity(float DeltaSeconds)
{
    // 基类只保留扩展点，派生类按自己的规则更新。
    (void)DeltaSeconds;
}

int32 UMagicShardEntity::GetEntityId() const
{
    return EntityId;
}

void UMagicShardEntity::SetEntityId(int32 NewId)
{
    EntityId = NewId;
}

bool UMagicShardEntity::IsActive() const
{
    return bActive;
}

void UMagicShardEntity::SetActive(bool bNewActive)
{
    bActive = bNewActive;
}

UMagicShardLivingEntity::UMagicShardLivingEntity()
    : Health(100.0f),
      Mana(50.0f),
      MaxHealth(100.0f),
      MaxMana(50.0f)
{
    EntityName = TEXT("LivingEntity");
}

FString UMagicShardLivingEntity::DescribeEntity() const
{
    return FString::Printf(
        TEXT("%s HP=%.1f/%.1f MP=%.1f/%.1f"),
        *Super::DescribeEntity(),
        Health,
        MaxHealth,
        Mana,
        MaxMana);
}

void UMagicShardLivingEntity::TickEntity(float DeltaSeconds)
{
    Super::TickEntity(DeltaSeconds);

    if (!bActive)
    {
        return;
    }

    if (Mana < MaxMana)
    {
        Mana = FMath::Min(MaxMana, Mana + DeltaSeconds * 2.0f);
    }
}

void UMagicShardLivingEntity::ApplyDamage(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    Health = FMath::Clamp(Health - Amount, 0.0f, MaxHealth);
    if (Health <= 0.0f)
    {
        bActive = false;
    }
}

void UMagicShardLivingEntity::RestoreHealth(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
    if (Health > 0.0f)
    {
        bActive = true;
    }
}

bool UMagicShardLivingEntity::CanAct() const
{
    return bActive && Health > 0.0f;
}

float UMagicShardLivingEntity::GetHealth() const
{
    return Health;
}

float UMagicShardLivingEntity::GetMana() const
{
    return Mana;
}

void UMagicShardLivingEntity::SetHealth(float NewHealth)
{
    Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
    bActive = Health > 0.0f;
}

void UMagicShardLivingEntity::SetMana(float NewMana)
{
    Mana = FMath::Clamp(NewMana, 0.0f, MaxMana);
}

UMagicShardHeroEntity::UMagicShardHeroEntity()
    : CollectedShards(0),
      Level(1)
{
    EntityName = TEXT("Hero");
    MaxHealth = 120.0f;
    Health = MaxHealth;
    MaxMana = 80.0f;
    Mana = MaxMana;
}

FString UMagicShardHeroEntity::GetDisplayName() const
{
    return FString::Printf(TEXT("%s Lv.%d"), *EntityName, Level);
}

FString UMagicShardHeroEntity::DescribeEntity() const
{
    return FString::Printf(TEXT("%s Shards=%d"), *Super::DescribeEntity(), CollectedShards);
}

void UMagicShardHeroEntity::TickEntity(float DeltaSeconds)
{
    Super::TickEntity(DeltaSeconds);

    if (CollectedShards >= Level * 5)
    {
        CollectedShards -= Level * 5;
        ++Level;
        MaxHealth += 10.0f;
        MaxMana += 5.0f;
        Health = MaxHealth;
        Mana = MaxMana;
    }
}

void UMagicShardHeroEntity::AddShard(int32 Count)
{
    if (Count > 0)
    {
        CollectedShards += Count;
    }
}

bool UMagicShardHeroEntity::SpendMana(float Cost)
{
    if (Cost <= 0.0f)
    {
        return true;
    }

    if (Mana < Cost)
    {
        return false;
    }

    Mana -= Cost;
    return true;
}

int32 UMagicShardHeroEntity::GetShardCount() const
{
    return CollectedShards;
}

FMagicShardPlayerRecord UMagicShardHeroEntity::ToRecord(int32 SlotIndex, const FVector& Position) const
{
    FMagicShardPlayerRecord Record;
    Record.SlotIndex = SlotIndex;
    Record.Level = Level;
    Record.Health = Health;
    Record.Mana = Mana;
    Record.Position = Position;
    Record.PlayerName = EntityName;
    Record.CollectedShards = CollectedShards;
    return Record;
}

void UMagicShardHeroEntity::FromRecord(const FMagicShardPlayerRecord& Record)
{
    Level = FMath::Max(1, Record.Level);
    EntityName = Record.PlayerName.IsEmpty() ? TEXT("Hero") : Record.PlayerName;
    Health = FMath::Clamp(Record.Health, 0.0f, MaxHealth);
    Mana = FMath::Clamp(Record.Mana, 0.0f, MaxMana);
    CollectedShards = FMath::Max(0, Record.CollectedShards);
    bActive = Health > 0.0f;
}
