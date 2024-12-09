// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterNonPlayer.h"
#include "Engine/AssetManager.h"
#include "AI/ABAIController.h"
#include "CharacterStat/ABCharacterStatComponent.h"

AABCharacterNonPlayer::AABCharacterNonPlayer()
{
	// 처음에 액터를 배치하고 숨겨놓음
	GetMesh()->SetHiddenInGame(false);

	// AIControllerClass에 할당된 AABAIController를 자동으로 액터에 할당
	AIControllerClass = AABAIController::StaticClass();
	
	// 레벨에 배치된 액터 및 SpawnActor로 생성된 액터 모두 AABAIController를 자동으로 할당
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// AActor의 라이프사이쿨 중 하나로, 모든 컴포넌트가 초기화된 후 호출되며 추가적인 초기화 작업을 수행할 때 사용
// AActor 생성(SpawnActor, 월드 배치) -> 컴포넌트 초기화, CreateDefaultSubobject로 생성된 컴포넌트 등록 -> PostInitializeComponents()호출, 액터의 모든 컴포넌트 사용 가능 -> BeginPlay() 호출
void AABCharacterNonPlayer::PostInitializeComponents()
{ 
	// 부모 클래스의 메서드 호출, 상속된 부모 클래스의 초기화 로직 보존 및 실행
	Super::PostInitializeComponents();

	// 디버깅 시 조건이 참인지 확인하고, 거짓이면 경고
	ensure(NPCMeshes.Num() > 0);
	// NPCMeshes 배열 내 무작위 인덱스 생성
	int32 RandIndex = FMath::RandRange(0, NPCMeshes.Num() - 1);
	// 선택된 메쉬를 비동기적으로 로드, 로드 완료시 NPCMeshLoadCompleted함수 호출
	// FStreamableDelegate::CreateUObject를 통해 로드 완료시 호출할 델리게이트 지정
	NPCMeshHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(NPCMeshes[RandIndex], FStreamableDelegate::CreateUObject(this, &AABCharacterNonPlayer::NPCMeshLoadCompleted));
}

void AABCharacterNonPlayer::SetDead()
{
	Super::SetDead();

	AABAIController* ABAIController = Cast<AABAIController>(GetController());
	if (ABAIController)
	{
		ABAIController->StopAI();
	}

	FTimerHandle DeadTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda(
		// [&]는 현재 스코프의 모든 변수를 참조로 캡처
		// this는 현재 객체 포인터(this)만 캡처 외부 변수 X
		// 현재 객체(this)를 참조 캡처(by reference), 즉 현재 객체의 멤버 함수인 Destory()가 실행되므로 현재 객체 제거
		[&]()
		{
			// 액터 월드에서 제거
			Destroy();
		}
		// DeadEventDelayTime 후에 Destory() 샐행, 타이머 반복 X(false)
	), DeadEventDelayTime, false);
}

void AABCharacterNonPlayer::NPCMeshLoadCompleted()
{
	// 리소스가 제대로 로드되었다면,
	if(NPCMeshHandle.IsValid()){
		// NPCMeshHandle에 의해 비동기로 로드된 시소스 반환 및 타입 캐스팅
		USkeletalMesh* NPCMesh = Cast<USkeletalMesh>(NPCMeshHandle->GetLoadedAsset());
		if (NPCMesh)
		{
			// 로드된 메쉬 설정
			GetMesh()->SetSkeletalMesh(NPCMesh);
			// 숨긴 메쉬 다시 화면에 표시
			GetMesh()->SetHiddenInGame(false);
		}
	}

	// 사용이 끝난 로드 핸들을 해제하여 메모리 정리
	NPCMeshHandle->ReleaseHandle();
}

float AABCharacterNonPlayer::GetAIPatrolRadius()
{
	return 800.0f;
}

float AABCharacterNonPlayer::GetAIDetectRange()
{
	return 400.0f;
}

float AABCharacterNonPlayer::GetAIAttackRange()
{
	return Stat->GetTotalStat().AttackRange + Stat->GetAttackRadius() * 2;
}

float AABCharacterNonPlayer::GetAITurnSpeed()
{
	return 0.0f;
}

void AABCharacterNonPlayer::SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished)
{
	OnAttackFinished = InOnAttackFinished;
}

void AABCharacterNonPlayer::AttackByAI()
{
	ProcessComboCommand();
}

void AABCharacterNonPlayer::NotifyComboActionEnd()
{
	Super::NotifyComboActionEnd();
	OnAttackFinished.ExecuteIfBound();
}
