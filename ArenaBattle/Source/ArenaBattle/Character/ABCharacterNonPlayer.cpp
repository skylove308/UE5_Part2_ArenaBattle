// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterNonPlayer.h"
#include "Engine/AssetManager.h"
#include "AI/ABAIController.h"
#include "CharacterStat/ABCharacterStatComponent.h"

AABCharacterNonPlayer::AABCharacterNonPlayer()
{
	// ó���� ���͸� ��ġ�ϰ� ���ܳ���
	GetMesh()->SetHiddenInGame(false);

	// AIControllerClass�� �Ҵ�� AABAIController�� �ڵ����� ���Ϳ� �Ҵ�
	AIControllerClass = AABAIController::StaticClass();
	
	// ������ ��ġ�� ���� �� SpawnActor�� ������ ���� ��� AABAIController�� �ڵ����� �Ҵ�
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// AActor�� ������������ �� �ϳ���, ��� ������Ʈ�� �ʱ�ȭ�� �� ȣ��Ǹ� �߰����� �ʱ�ȭ �۾��� ������ �� ���
// AActor ����(SpawnActor, ���� ��ġ) -> ������Ʈ �ʱ�ȭ, CreateDefaultSubobject�� ������ ������Ʈ ��� -> PostInitializeComponents()ȣ��, ������ ��� ������Ʈ ��� ���� -> BeginPlay() ȣ��
void AABCharacterNonPlayer::PostInitializeComponents()
{ 
	// �θ� Ŭ������ �޼��� ȣ��, ��ӵ� �θ� Ŭ������ �ʱ�ȭ ���� ���� �� ����
	Super::PostInitializeComponents();

	// ����� �� ������ ������ Ȯ���ϰ�, �����̸� ���
	ensure(NPCMeshes.Num() > 0);
	// NPCMeshes �迭 �� ������ �ε��� ����
	int32 RandIndex = FMath::RandRange(0, NPCMeshes.Num() - 1);
	// ���õ� �޽��� �񵿱������� �ε�, �ε� �Ϸ�� NPCMeshLoadCompleted�Լ� ȣ��
	// FStreamableDelegate::CreateUObject�� ���� �ε� �Ϸ�� ȣ���� ��������Ʈ ����
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
		// [&]�� ���� �������� ��� ������ ������ ĸó
		// this�� ���� ��ü ������(this)�� ĸó �ܺ� ���� X
		// ���� ��ü(this)�� ���� ĸó(by reference), �� ���� ��ü�� ��� �Լ��� Destory()�� ����ǹǷ� ���� ��ü ����
		[&]()
		{
			// ���� ���忡�� ����
			Destroy();
		}
		// DeadEventDelayTime �Ŀ� Destory() ����, Ÿ�̸� �ݺ� X(false)
	), DeadEventDelayTime, false);
}

void AABCharacterNonPlayer::NPCMeshLoadCompleted()
{
	// ���ҽ��� ����� �ε�Ǿ��ٸ�,
	if(NPCMeshHandle.IsValid()){
		// NPCMeshHandle�� ���� �񵿱�� �ε�� �üҽ� ��ȯ �� Ÿ�� ĳ����
		USkeletalMesh* NPCMesh = Cast<USkeletalMesh>(NPCMeshHandle->GetLoadedAsset());
		if (NPCMesh)
		{
			// �ε�� �޽� ����
			GetMesh()->SetSkeletalMesh(NPCMesh);
			// ���� �޽� �ٽ� ȭ�鿡 ǥ��
			GetMesh()->SetHiddenInGame(false);
		}
	}

	// ����� ���� �ε� �ڵ��� �����Ͽ� �޸� ����
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
