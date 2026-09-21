// Copyright Lim Young.

#pragma once

#include "CoreMinimal.h"
#include "AITokenCondition.h"
#include "AITokenConditionPredicate.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, DefaultToInstanced, EditInlineNew, CollapseCategories)
class AITOKENCORE_API UAITokenConditionPredicate : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "AI Token")
	bool bReverse = false;

public:
	virtual bool Evaluate(const FAITokenConditionContext& Context) const;
};

UCLASS()
class AITOKENCORE_API UAITokenConditionPredicate_Single : public UAITokenConditionPredicate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = "AI Token")
	TObjectPtr<UAITokenCondition> Condition = nullptr;

protected:
	virtual bool Evaluate(const FAITokenConditionContext& Context) const override;
};

UCLASS()
class AITOKENCORE_API UAITokenConditionPredicate_And : public UAITokenConditionPredicate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = "AI Token")
	TArray<TObjectPtr<UAITokenConditionPredicate>> Predicates;

	virtual bool Evaluate(const FAITokenConditionContext& Context) const override;
};

UCLASS()
class AITOKENCORE_API UAITokenConditionPredicate_Or : public UAITokenConditionPredicate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = "AI Token")
	TArray<TObjectPtr<UAITokenConditionPredicate>> Predicates;

	virtual bool Evaluate(const FAITokenConditionContext& Context) const override;
};
