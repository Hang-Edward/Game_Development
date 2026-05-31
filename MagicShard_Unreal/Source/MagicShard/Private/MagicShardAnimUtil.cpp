#include "MagicShardAnimUtil.h"

#include "Animation/AnimSequence.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UMagicShardAnimUtil::ExportRunAnimLoopFixData()
{
    UAnimSequence* Seq = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Imported/Character/run_Anim.run_Anim"));
    if (Seq == nullptr || Seq->GetDataModel() == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AnimUtil] run_Anim is unavailable."));
        return false;
    }

    const int32 TotalBones = Seq->GetDataModel()->GetNumBoneTracks();
    FString Csv = TEXT("Bone;PosX;PosY;PosZ;RotX;RotY;RotZ;RotW;PosDelta;RotDeltaDeg\n");
    int32 ProblemBones = 0;

    for (int32 BoneIndex = 0; BoneIndex < TotalBones; ++BoneIndex)
    {
        const FBoneAnimationTrack& Track = Seq->GetDataModel()->GetBoneTrackByIndex(BoneIndex);
        const FRawAnimSequenceTrack& RawTrack = Track.InternalTrackData;

        if (RawTrack.PosKeys.Num() < 2 && RawTrack.RotKeys.Num() < 2)
        {
            continue;
        }

        const FVector FirstPos = RawTrack.PosKeys.Num() > 0 ? FVector(RawTrack.PosKeys[0]) : FVector::ZeroVector;
        const FVector LastPos = RawTrack.PosKeys.Num() > 0 ? FVector(RawTrack.PosKeys.Last()) : FirstPos;
        const FQuat FirstRot = RawTrack.RotKeys.Num() > 0 ? FQuat(RawTrack.RotKeys[0]) : FQuat::Identity;
        const FQuat LastRot = RawTrack.RotKeys.Num() > 0 ? FQuat(RawTrack.RotKeys.Last()) : FirstRot;

        const double LoopPos = (LastPos - FirstPos).Size();
        const double LoopRot = FMath::RadiansToDegrees(FirstRot.AngularDistance(LastRot));

        if (LoopPos > 0.01 || LoopRot > 0.1)
        {
            Csv += FString::Printf(TEXT("%s;%.6f;%.6f;%.6f;%.9f;%.9f;%.9f;%.9f;%.6f;%.6f\n"),
                *Track.Name.ToString(),
                FirstPos.X, FirstPos.Y, FirstPos.Z,
                FirstRot.X, FirstRot.Y, FirstRot.Z, FirstRot.W,
                LoopPos, LoopRot);
            ++ProblemBones;
        }
    }

    const FString SavePath = FPaths::ProjectSavedDir() / TEXT("run_bones_fix.csv");
    FFileHelper::SaveStringToFile(Csv, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8);
    UE_LOG(LogTemp, Display, TEXT("[AnimUtil] run_Anim loop check: bones=%d problemBones=%d csv=%s"),
        TotalBones,
        ProblemBones,
        *SavePath);

    return ProblemBones > 0;
}
