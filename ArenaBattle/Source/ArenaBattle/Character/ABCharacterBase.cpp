// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework//CharacterMovementComponent.h"
#include "ABCharacterControlData.h"
#include "Animation/AnimMontage.h"
#include "ABComboActionData.h"
#include "Physics/ABCollision.h"
#include "Engine/DamageEvents.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "UI/ABWidgetComponent.h"
#include "UI/ABHpBarWidget.h"
#include "Item/ABItems.h"

DEFINE_LOG_CATEGORY(LogABCharacter);

// Sets default values
AABCharacterBase::AABCharacterBase()
{
	// Pawn
	// ��Ʈ�ѷ� ȸ�� ��Ȱ��ȭ
	// ��Ʈ�ѷ� �Է¿� ���� ���� ȸ������ �ʰ�, �̵� ���⿡ ���� �ڵ����� ȸ��
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Capsule
	// ĸ���� ĳ������ �浹�� ó���ϸ�, ���̿� �������� ���� (������, ���� ����)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	// ĸ���� �浹 ���������� ����(������Ʈ ���� �Ǵ� DefaultEngine.ini)
	GetCapsuleComponent()->SetCollisionProfileName(CPROFILE_ABCAPSULE);

	// Movement
	// ĳ���Ͱ� �̵� �������� �ڵ����� ȸ���ϵ��� ����
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// ȸ���ӵ� (���ϼӵ�/�¿�ӵ�/�¿����)
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	// �����ӵ�
	GetCharacterMovement()->JumpZVelocity = 700.f;
	// ���߿��� ������ �� �ִ� ���� (0: �̵��Ұ� 1: �����̵�����)
	GetCharacterMovement()->AirControl = 0.35f;
	// �̵��� �ִ� �ӵ�
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	// �̵��� �ּ� �ӵ�
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	// �ȴ� �� ���� �� �����ϴ� �ӵ�
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Mesh
	// �޽��� �θ� ������Ʈ�� ������ ��ġ�ǵ��� ����
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.f), FRotator(0.0f, -90.0f, 0.0f));
	// �ִϸ��̼� �������Ʈ�� ���� �ִϸ��̼� ����
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	// �޽��� �浹 ���������� NoCollision���� ����(�浹 ��Ȱ��ȭ)
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// ��� ������� ���ҽ��� ã�� �����ڿ��� �ʱ�ȭ
	// ������ ��ü�� �ʱ�ȭ ������ ���ҽ��� �����ؾ� �� �� ����
	// ��Ÿ�ӿ� �ε尡 �ʿ��ϸ� UAsstManager �Ǵ� FStreamableManager�� ����ϴ� ���� �� ����
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef((TEXT("/Script/Engine.SkeletalMesh'/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard'")));
	if (CharacterMeshRef.Object)
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClasRef((TEXT("/Game/ArenaBattle/Animation/ABP_ABCharacter.ABP_ABCharacter_C")));
	if (AnimInstanceClasRef.Class)
	{
		GetMesh()->SetAnimInstanceClass(AnimInstanceClasRef.Class);
	}

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> ShoulderDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Shoulder.ABC_Shoulder'"));
	if (ShoulderDataRef.Object)
	{
		CharacterControlManager.Add(ECharacterControlType::Shoulder, ShoulderDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> QuarterDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Quarter.ABC_Quarter'"));
	if (QuarterDataRef.Object)
	{
		CharacterControlManager.Add(ECharacterControlType::Quarter, QuarterDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboActionMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_ComboAttack.AM_ComboAttack'"));
	if (ComboActionMontageRef.Object)
	{
		ComboActionMontage = ComboActionMontageRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UABComboActionData> ComboActionDataRef(TEXT("/Script/ArenaBattle.ABComboActionData'/Game/ArenaBattle/CharacterAction/ABA_ComboAttack.ABA_ComboAttack'"));
	if (ComboActionDataRef.Object)
	{
		ComboActionData = ComboActionDataRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeadMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_Dead.AM_Dead'"));
	if (DeadMontageRef.Object)
	{
		DeadMontage = DeadMontageRef.Object;
	}

	// Stat Component
	// Acotr�� ������Ʈ�� �ʱ�ȭ�ϰ� ����. �� Stat ������Ʈ�� �߰�
	Stat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("Stat"));

	// Widget Component
	HpBar = CreateDefaultSubobject<UABWidgetComponent>(TEXT("Widget"));
	// HpBar�� ĳ������ ���̷�Ż �޽��� ����
	HpBar->SetupAttachment(GetMesh());
	// �θ� ������Ʈ�� ���� ��� ��ġ ����
	// �θ� ������Ʈ �������� 180 ���� ��� -> ĳ���� �Ӹ��� ��ġ
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/ArenaBattle/UI/WBP_HpBar.WBP_HpBar_C"));
	if (HpBarWidgetRef.Class)
	{
		// HpBar ���� ������Ʈ�� WBP_HpBar Ŭ������ ����
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		// ������ ȭ�� ������ ǥ��
		// ȭ�� ������ UIó�� ���������� ĳ���� �Ӹ� ���� ��ġ
		HpBar->SetWidgetSpace(EWidgetSpace::Screen);
		// ������ ũ�� ���� (����, �β�)
		HpBar->SetDrawSize(FVector2D(150.0f, 15.0f));
		// ������ �浹 ��Ȱ��ȭ
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Item Actions
	// FTakeItemDelegateWrapper�� FOnTakeItemDelegate�� ����Ʈ���� ����� �� �ֵ��� ����
	// ���� ĳ���� ��������Ʈ�� �ټ��� �������� Ȯ��
	// ��Ƽĳ��Ʈ ��������Ʈ ���� �ϳ��� �̺�Ʈ�� ��� ������ ���̰� �Ǿ� ���� �и��� �������
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::EquipWeapon)));
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::DrinkPotion)));
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::ReadScroll)));

	//Weapon Component
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));
}

void AABCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Stat ������Ʈ���� OnHpZero.BroadCast()�� ȣ��ɶ����� SetDead�Լ��� ����
	Stat->OnHpZero.AddUObject(this, &AABCharacterBase::SetDead);
	// Stat ������Ʈ���� OnStatChanged.BroadCast()�� ȣ��ɶ����� ApplyStat�Լ��� ����
	Stat->OnStatChanged.AddUObject(this, &AABCharacterBase::ApplyStat);
}

// CharacterControlData�� ���� ĳ���� ��Ʈ�� ����
void AABCharacterBase::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	// Pawn
	// ĳ������ ȸ�� ����� �����ϴ� �÷���
	// true�� ��� ĳ���ʹ� ��Ʈ�ѷ��� ������ ���� ȸ��
	bUseControllerRotationYaw = CharacterControlData->bUseControllerRotationYaw;

	// CharacterMovement
	// ĳ������ ȸ�� ������ �̵� ���⿡ �������� ����
	GetCharacterMovement()->bOrientRotationToMovement = CharacterControlData->bOrientRotationToMovement;
	// ĳ���Ͱ� ��Ʈ�ѷ��� ���ϴ� ȸ�� ������ ��������� ����
	GetCharacterMovement()->bUseControllerDesiredRotation = CharacterControlData->bUseControllerDesiredRotation;
	// ĳ������ ȸ�� �ӵ��� ����
	GetCharacterMovement()->RotationRate = CharacterControlData->RotationRate;
}

void AABCharacterBase::ProcessComboCommand()
{
	if (CurrentCombo == 0)
	{
		ComboActionBegin();
		return;
	}

	// Ÿ�̸Ӱ� ��ȿ������ Ȯ���Ͽ� �޺� Ÿ�̸Ӱ� ���� ������ Ȯ��
	if (!ComboTimerHandle.IsValid())
	{
		// �÷��̾ �޺� �Է� ������
		HasNextComboCommand = false;
	}
	else
	{
		// �޺� �ð� ��ȿ 
		HasNextComboCommand = true;
	}
}

void AABCharacterBase::ComboActionBegin()
{
	// Combo Status
	CurrentCombo = 1;

	// Movement Setting
	// ĳ������ �̵��� ��Ȱ��ȭ�Ͽ� ���� ��(�޺�)���� ĳ���Ͱ� �̵��� �� ������ ����.
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	// Animation Setting
	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(ComboActionMontage, AttackSpeedRate);

	FOnMontageEnded EndDelegate;
	// ��������Ʈ ȣ���, ComboActionEnd�Լ� ����
	EndDelegate.BindUObject(this, &AABCharacterBase::ComboActionEnd);
	// ComboActionMontage�� ����� ��, EndDelegate ����
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboActionMontage);

	// �޺� �ʱ�ȭ
	// ������ ������ �޺� Ÿ�̸� �ʱ�ȭ
	ComboTimerHandle.Invalidate();
	// ���� �޺� �Է��� �ޱ� ���� ���� �ð� ����
	SetComboCheckTimer();
}

// �޺� ������ ���� ���Ŀ��� �̵� ����
void AABCharacterBase::ComboActionEnd(UAnimMontage* TargetMontage, bool IsProperlyEnded)
{
	ensure(CurrentCombo != 0);
	CurrentCombo = 0;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	NotifyComboActionEnd();
}

void AABCharacterBase::NotifyComboActionEnd()
{
}

// �޺� �Է� ���� �ð� ����
void AABCharacterBase::SetComboCheckTimer()
{
	int32 ComboIndex = CurrentCombo - 1;
	// ���� �޺� �ܰ谡 �迭 �� ��ȿ�� �ε������� Ȯ��
	ensure(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	// (���� �޺� �ִϸ��̼��� ��ȿ ������ �� / �ִϸ��̼��� �ʴ� ������ ��) / ĳ������ ���� �ӵ�
	float ComboEffectiveTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;
	if (ComboEffectiveTime > 0.0f)
	{
		// ComboEffective �ð� �Ŀ� ComboCheck ����
		// Ÿ�̸� �ڵ�, ��ü, Ÿ�̸� ����� ���� �Լ�, Ÿ�̸Ӱ� ����Ǳ������ �ð� ����, Ÿ�̸� �ݺ� ���� ����
		// Ÿ�̸� �ڵ��� ���Ӱ� ����. �޺� ��ȿ �ð� ���� ���ԷµǸ� HasNextComboCommand true ����
		GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &AABCharacterBase::ComboCheck, ComboEffectiveTime, false);
	}
}

void AABCharacterBase::ComboCheck()
{
	// Ÿ�̸Ӹ� ��ȿȭ�Ͽ� ���� Ÿ�̸Ӹ� ����
	ComboTimerHandle.Invalidate();
	// �÷��̾ ���� �޺� �Է��� �ߴٸ�
	if (HasNextComboCommand)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		// CurrentCombo + 1 ���� 1�� MaxComboCount ���̷� ����
		CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
		// ���� �޺� �ִϸ��̼� ������ �̸��� �������� ����
		FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
		// ���� �������� ���� �̵�
		AnimInstance->Montage_JumpToSection(NextSection, ComboActionMontage);
		SetComboCheckTimer();
		HasNextComboCommand = false;

	}
}

void AABCharacterBase::AttackHitCheck()
{
	// �浹 ���� ����� �����ϴ� ����ü
	FHitResult OutHitResult;
	// �浹 ���� �� ����� �߰� ����
	// SCENE_QUERY_STAT(Attack) : ������ �±�, �浹 ������ �̸��� Attack
	// false : �������� �浹�� ������ ���� �ʴ� ������Ʈ(��: ���� �浹ü)�� ����
	// this: �浹 �������� ���� ���͸� ������(�ڱ� �ڽŰ��� �浹���� ����)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

	// ���� ����(����)
	const float AttackRange = Stat->GetTotalStat().AttackRange;
	// ���� ������ ������(��ü ���� �浹 ������)
	const float AttackRadius = Stat->GetAttackRadius();
	const float AttackDamage = Stat->GetTotalStat().Attack;
	// ĳ������ ���� ��ġ���� �������� ĸ�� ��������ŭ ������ ������ ���� �������� ����
	const FVector Start = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
	// ���� ������ŭ �������� Ȯ���� �� ������ ����
	const FVector End = Start + GetActorForwardVector() + AttackRange;

	// �浹�� ������ ������ OutHitResult�� ����
	// ȸ���� ������� �ʰ�, CCHANNEL_ABACTION �浹 ä���� ����
	// ���� ������ ��ü�� ������
	bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);
	if(HitDetected){
		FDamageEvent DamageEvent;
		OutHitResult.GetActor()->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
	}

// �浹 ���� ����� �ð�ȭ
#if ENABLE_DRAW_DEBUG

	FVector CapsuleOrigin = Start + (End - Start) * 0.5f;
	float CapsuleHalfHeight = AttackRange * 0.5f;
	FColor DrawColor = HitDetected ? FColor::Green : FColor::Red;
	DrawDebugCapsule(GetWorld(), CapsuleOrigin, CapsuleHalfHeight, AttackRadius, FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat(), DrawColor, false, 5.0f);

#endif
}

float AABCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Stat->ApplyDamage(DamageAmount);

	return DamageAmount;
}

void AABCharacterBase::SetDead()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	PlayDeadAnimation();
	SetActorEnableCollision(false);
	HpBar->SetHiddenInGame(true);
}

void AABCharacterBase::PlayDeadAnimation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->StopAllMontages(0.0f);
	AnimInstance->Montage_Play(DeadMontage, 1.0f);
}

void AABCharacterBase::SetupCharacterWidget(UABUserWidget* InUserWidget)
{
	UABHpBarWidget* HpBarWidget = Cast<UABHpBarWidget>(InUserWidget);
	if (HpBarWidget)
	{
		// ���� �ʱ�ȭ
		HpBarWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		HpBarWidget->UpdateHpBar(Stat->GetCurrentHp());
		// HP�� ����� �� ȣ��Ǵ� �̺�Ʈ ��������Ʈ
		// �̺�Ʈ�� �߻��ϸ� UpdateHpBar�Լ� ȣ���Ͽ� HP�� �ڵ����� ������Ʈ
		Stat->OnHpChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateHpBar);
		Stat->OnStatChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateStat);
	}
}

// ���ǿ� ���� ���� ĳ���� ��������Ʈ ����Ʈ���� ������ ��������Ʈ�� ����
void AABCharacterBase::TakeItem(UABItemData* InItemData)
{
	if (InItemData)
	{
		// InItemData->Type�� EItemType Ÿ���̹Ƿ� �迭�� �ε����� ���� ����� �� ����
		// (uint8) ĳ�������� ������ ���� ���������� ��ȯ�Ͽ� �迭 �ε����� ��밡��
		// ExecuteIfBound�� ItemDelegate�� ��ȿ�� ��� ���ε��� �Լ��� ����(���� ���, Execute�� ���ε� ����(IsBound())Ȯ�� �ؾ���)
		TakeItemActions[(uint8)InItemData->Type].ItemDelegate.ExecuteIfBound(InItemData);
	}
}

void AABCharacterBase::DrinkPotion(UABItemData* InItemData)
{
	UABPotionItemData* PotionItemData = Cast<UABPotionItemData>(InItemData);
	if (PotionItemData)
	{
		Stat->HealHp(PotionItemData->HealAmount);
	}
}

void AABCharacterBase::EquipWeapon(UABItemData* InItemData)
{
	// �Էµ� ������ �����͸� ���� �����ͷ� ĳ����
	UABWeaponItemData* WeaponItemData = Cast<UABWeaponItemData>(InItemData);
	if (InItemData)
	{
		// ���� �޽��� ���� �ε���� �ʾҴ��� Ȯ��
		// true�� ���, �޽��� �޸𸮿� �ε�Ǳ� ����
		if (WeaponItemData->WeaponMesh.IsPending())
		{
			// �񵿱�� �ε�� �� �ִ� ���ҽ��� ��� ���������� �ε�
			// ��, ���� �޽��� �ε� ���� �ʾҴٸ� ���������� �ε�
			WeaponItemData->WeaponMesh.LoadSynchronous();
		}
		Weapon->SetSkeletalMesh(WeaponItemData->WeaponMesh.Get()); 
		Stat->SetModifierStat(WeaponItemData->ModifierStat);
	}
}

void AABCharacterBase::ReadScroll(UABItemData* InItemData)
{
	UABScrollItemData* ScrollItemData = Cast<UABScrollItemData>(InItemData);
	if (ScrollItemData)
	{
		Stat->AddBaseStat(ScrollItemData->BaseStat);
	}
}

int32 AABCharacterBase::GetLevel()
{
	return Stat->GetCurrentLevel();
}

void AABCharacterBase::SetLevel(int32 InNewLevel)
{
	Stat->SetLevelStat(InNewLevel);
}

void AABCharacterBase::ApplyStat(const FABCharacterStat& BaseStat, const FABCharacterStat& ModifierStat)
{
	float MovementSpeed = (BaseStat + ModifierStat).MovementSpeed;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
}


