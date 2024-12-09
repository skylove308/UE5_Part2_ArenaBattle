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
	// 컨트롤러 회전 비활성화
	// 컨트롤러 입력에 따라 직접 회전하지 않고, 이동 방향에 따라 자동으로 회전
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Capsule
	// 캡슐은 캐릭터의 충돌을 처리하며, 높이와 반지름을 설정 (반지름, 반쪽 높이)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	// 캡슐의 충돌 프로파일을 설정(프로젝트 설정 또는 DefaultEngine.ini)
	GetCapsuleComponent()->SetCollisionProfileName(CPROFILE_ABCAPSULE);

	// Movement
	// 캐릭터가 이동 방향으로 자동으로 회전하도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 회전속도 (상하속도/좌우속도/좌우기울기)
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	// 점프속도
	GetCharacterMovement()->JumpZVelocity = 700.f;
	// 공중에서 제어할 수 있는 정도 (0: 이동불가 1: 완전이동가능)
	GetCharacterMovement()->AirControl = 0.35f;
	// 이동시 최대 속도
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	// 이동시 최소 속도
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	// 걷는 중 멈출 때 감속하는 속도
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Mesh
	// 메쉬가 부모 컴포넌트에 적절히 배치되도록 설정
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.f), FRotator(0.0f, -90.0f, 0.0f));
	// 애니메이션 블루프린트를 통해 애니메이션 제어
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	// 메쉬의 충돌 프로파일을 NoCollision으로 설정(충돌 비활성화)
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// 경로 기반으로 리소스를 찾아 생성자에서 초기화
	// 정적인 객체나 초기화 시점에 리소스를 설정해야 할 때 유용
	// 런타임에 로드가 필요하면 UAsstManager 또는 FStreamableManager를 사용하는 것이 더 적합
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
	// Acotr의 컴포넌트를 초기화하고 생성. 즉 Stat 컴포넌트를 추가
	Stat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("Stat"));

	// Widget Component
	HpBar = CreateDefaultSubobject<UABWidgetComponent>(TEXT("Widget"));
	// HpBar를 캐릭터의 스켈레탈 메쉬에 부착
	HpBar->SetupAttachment(GetMesh());
	// 부모 컴포넌트에 대한 상대 위치 설정
	// 부모 컴포넌트 위쪽으로 180 단위 상승 -> 캐릭터 머리에 위치
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/ArenaBattle/UI/WBP_HpBar.WBP_HpBar_C"));
	if (HpBarWidgetRef.Class)
	{
		// HpBar 위젯 컴포넌트에 WBP_HpBar 클래스를 설정
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		// 위젯이 화면 공간에 표시
		// 화면 고정형 UI처럼 동작하지만 캐릭터 머리 위에 위치
		HpBar->SetWidgetSpace(EWidgetSpace::Screen);
		// 위젯의 크기 설정 (길이, 두께)
		HpBar->SetDrawSize(FVector2D(150.0f, 15.0f));
		// 위젯의 충돌 비활성화
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Item Actions
	// FTakeItemDelegateWrapper는 FOnTakeItemDelegate를 리스트에서 사용할 수 있도록 래핑
	// 단일 캐스팅 델리게이트를 다수의 동작으로 확장
	// 멀티캐스트 델리게이트 사용시 하나의 이벤트에 모든 동작이 묶이게 되어 동작 분리가 어려워짐
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
	// Stat 컴포넌트에서 OnHpZero.BroadCast()가 호출될때마다 SetDead함수가 실행
	Stat->OnHpZero.AddUObject(this, &AABCharacterBase::SetDead);
	// Stat 컴포넌트에서 OnStatChanged.BroadCast()가 호출될때마다 ApplyStat함수가 실행
	Stat->OnStatChanged.AddUObject(this, &AABCharacterBase::ApplyStat);
}

// CharacterControlData에 따라 캐릭터 컨트롤 설정
void AABCharacterBase::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	// Pawn
	// 캐릭터의 회전 방식을 결정하는 플래그
	// true인 경우 캐릭터는 컨트롤러의 방향을 따라 회전
	bUseControllerRotationYaw = CharacterControlData->bUseControllerRotationYaw;

	// CharacterMovement
	// 캐릭터의 회전 방향을 이동 방향에 맞출지를 결정
	GetCharacterMovement()->bOrientRotationToMovement = CharacterControlData->bOrientRotationToMovement;
	// 캐릭터가 컨트롤러의 원하는 회전 방향을 사용할지를 결정
	GetCharacterMovement()->bUseControllerDesiredRotation = CharacterControlData->bUseControllerDesiredRotation;
	// 캐릭터의 회전 속도를 정의
	GetCharacterMovement()->RotationRate = CharacterControlData->RotationRate;
}

void AABCharacterBase::ProcessComboCommand()
{
	if (CurrentCombo == 0)
	{
		ComboActionBegin();
		return;
	}

	// 타이머가 유효한지를 확인하여 콤보 타이머가 동작 중인지 확인
	if (!ComboTimerHandle.IsValid())
	{
		// 플레이어가 콤보 입력 안했음
		HasNextComboCommand = false;
	}
	else
	{
		// 콤보 시간 유효 
		HasNextComboCommand = true;
	}
}

void AABCharacterBase::ComboActionBegin()
{
	// Combo Status
	CurrentCombo = 1;

	// Movement Setting
	// 캐릭터의 이동을 비활성화하여 공격 중(콤보)에는 캐릭터가 이동할 수 없도록 제한.
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	// Animation Setting
	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(ComboActionMontage, AttackSpeedRate);

	FOnMontageEnded EndDelegate;
	// 델리게이트 호출시, ComboActionEnd함수 실행
	EndDelegate.BindUObject(this, &AABCharacterBase::ComboActionEnd);
	// ComboActionMontage가 종료될 때, EndDelegate 실행
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboActionMontage);

	// 콤보 초기화
	// 이전에 설정된 콤보 타이머 초기화
	ComboTimerHandle.Invalidate();
	// 다음 콤보 입력을 받기 위한 제한 시간 설정
	SetComboCheckTimer();
}

// 콤보 공격이 끝난 이후에는 이동 가능
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

// 콤보 입력 제한 시간 설정
void AABCharacterBase::SetComboCheckTimer()
{
	int32 ComboIndex = CurrentCombo - 1;
	// 현재 콤보 단계가 배열 내 유효한 인덱스인지 확인
	ensure(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	// (현재 콤보 애니메이션의 유효 프레임 수 / 애니메이션의 초당 프레임 수) / 캐릭터의 공격 속도
	float ComboEffectiveTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;
	if (ComboEffectiveTime > 0.0f)
	{
		// ComboEffective 시간 후에 ComboCheck 실행
		// 타이머 핸들, 객체, 타이머 만료시 실행 함수, 타이머가 만료되기까지의 시간 간격, 타이머 반복 실행 여부
		// 타이머 핸들이 새롭게 생성. 콤보 유효 시간 동안 재입력되면 HasNextComboCommand true 설정
		GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &AABCharacterBase::ComboCheck, ComboEffectiveTime, false);
	}
}

void AABCharacterBase::ComboCheck()
{
	// 타이머를 무효화하여 현재 타이머를 종료
	ComboTimerHandle.Invalidate();
	// 플레이어가 다음 콤보 입력을 했다면
	if (HasNextComboCommand)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		// CurrentCombo + 1 값을 1과 MaxComboCount 사이로 제한
		CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
		// 다음 콤보 애니메이션 섹션의 이름을 동적으로 생성
		FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
		// 다음 섹션으로 강제 이동
		AnimInstance->Montage_JumpToSection(NextSection, ComboActionMontage);
		SetComboCheckTimer();
		HasNextComboCommand = false;

	}
}

void AABCharacterBase::AttackHitCheck()
{
	// 충돌 감지 결과를 저장하는 구조체
	FHitResult OutHitResult;
	// 충돌 감지 시 사용할 추가 조건
	// SCENE_QUERY_STAT(Attack) : 디버깅용 태그, 충돌 쿼리의 이름은 Attack
	// false : 물리적인 충돌에 영향을 받지 않는 오브젝트(예: 가상 충돌체)도 포함
	// this: 충돌 감지에서 현재 액터를 제외함(자기 자신과는 충돌하지 않음)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

	// 공격 범위(길이)
	const float AttackRange = Stat->GetTotalStat().AttackRange;
	// 공격 범위의 반지름(구체 형태 충돌 감지용)
	const float AttackRadius = Stat->GetAttackRadius();
	const float AttackDamage = Stat->GetTotalStat().Attack;
	// 캐릭터의 현재 위치에서 전방으로 캡슐 반지름만큼 떨어진 지점을 시작 지점으로 설정
	const FVector Start = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
	// 공격 범위만큼 전방으로 확장한 끝 지점을 설정
	const FVector End = Start + GetActorForwardVector() + AttackRange;

	// 충돌한 액터의 정보는 OutHitResult에 저장
	// 회전을 사용하지 않고, CCHANNEL_ABACTION 충돌 채널을 지정
	// 감지 영역을 구체로 지정함
	bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);
	if(HitDetected){
		FDamageEvent DamageEvent;
		OutHitResult.GetActor()->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
	}

// 충돌 감지 디버깅 시각화
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
		// 위젯 초기화
		HpBarWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		HpBarWidget->UpdateHpBar(Stat->GetCurrentHp());
		// HP가 변경될 때 호출되는 이벤트 델리게이트
		// 이벤트가 발생하면 UpdateHpBar함수 호출하여 HP바 자동으로 업데이트
		Stat->OnHpChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateHpBar);
		Stat->OnStatChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateStat);
	}
}

// 조건에 따라 단일 캐스팅 델리게이트 리스트에서 적합한 델리게이트만 실행
void AABCharacterBase::TakeItem(UABItemData* InItemData)
{
	if (InItemData)
	{
		// InItemData->Type은 EItemType 타입이므로 배열의 인덱스로 직접 사용할 수 없음
		// (uint8) 캐스팅으로 열거형 값을 정수형으로 변환하여 배열 인덱스로 사용가능
		// ExecuteIfBound는 ItemDelegate가 유효한 경우 바인딩된 함수를 실행(권장 방식, Execute는 바인딩 여부(IsBound())확인 해야함)
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
	// 입력된 아이템 데이터를 무기 데이터로 캐스팅
	UABWeaponItemData* WeaponItemData = Cast<UABWeaponItemData>(InItemData);
	if (InItemData)
	{
		// 무기 메쉬가 아직 로드되지 않았는지 확인
		// true일 경우, 메쉬가 메모리에 로드되기 전임
		if (WeaponItemData->WeaponMesh.IsPending())
		{
			// 비동기로 로드될 수 있는 리소스를 즉시 동기적으로 로드
			// 즉, 무기 메쉬가 로드 되지 않았다면 동기적으로 로드
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


