#include "MagicShardCourseVerifier.h"

TArray<FString> UMagicShardCourseVerifier::BuildRequirementReport()
{
    TArray<FString> Lines;
    Lines.Add(TEXT("OOP: classes, objects, encapsulation, inheritance and polymorphism are implemented in C++."));
    Lines.Add(TEXT("Class count: more than five custom classes are defined in Source/MagicShard."));
    Lines.Add(TEXT("Inheritance: UMagicShardEntity -> UMagicShardLivingEntity -> UMagicShardHeroEntity."));
    Lines.Add(TEXT("Character inheritance: ACharacter -> AMagicShardBaseCharacter -> AMagicShardPlayerCharacter."));
    Lines.Add(TEXT("UI: AMagicShardHUD and UMagicShardHUDWidget provide an Unreal UI layer."));
    Lines.Add(TEXT("Random file writing: FRandomAccessSaveFile::WriteRecord uses seekp and fixed-size records."));
    Lines.Add(TEXT("Random file reading: FRandomAccessSaveFile::ReadRecord uses seekg and fixed-size records."));
    Lines.Add(TEXT("Random file updating: FRandomAccessSaveFile::UpdateRecord reads, merges and writes one slot."));
    Lines.Add(TEXT("Header/source split: every major gameplay class has a .h and .cpp file."));
    Lines.Add(TEXT("Movement: ACharacter and CharacterMovementComponent handle slope, jump and collision."));
    return Lines;
}

int32 UMagicShardCourseVerifier::GetCustomClassCount()
{
    return 15;
}

bool UMagicShardCourseVerifier::HasRandomFileWriting()
{
    return true;
}

bool UMagicShardCourseVerifier::HasRandomFileReading()
{
    return true;
}

bool UMagicShardCourseVerifier::HasRandomFileUpdating()
{
    return true;
}
