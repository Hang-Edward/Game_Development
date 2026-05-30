#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MagicShardTypes.h"
#include "MagicShardAnimInstance.generated.h"

/**
 * 角色动画实例。
 *
 * 在 NativeUpdateAnimation 中根据角色的 MoveSpeed/ActionState 计算 Speed，
 * 供 Animation Blueprint 中的 BlendSpace 使用，实现 Idle↔Walk↔Run 平滑过渡。
 *
 * 用法:
 *  - ABP_Character 的父类设为 UMagicShardAnimInstance
 *  - AnimGraph 中放入 BlendSpacePlayer(BS_Locomotion)，其 Speed 引脚绑定此变量
 */
UCLASS()
class MAGICSHARD_API UMagicShardAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UMagicShardAnimInstance();

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    /** BlendSpace 的 X 轴输入值: 0=Idle, 0.55=Walk, 1.0=Run */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float Speed;

    /** 角色当前动作状态 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    EMagicShardActionState ActionState;

    /** 是否在空中（跳跃/坠落） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsInAir;

private:
    /** 当前实际的混合值，用于平滑插值 */
    float CurrentBlendSpeed;
};
