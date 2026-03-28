// Copyright 2026, BlueprintsLab, All rights reserved
#include "MaterialGraphDescriber.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture.h"
#endif
FString FMaterialGraphDescriber::Describe(UMaterial* Material)
{
#if WITH_EDITOR
	if (!Material)
	{
		return TEXT("Invalid Material provided.");
	}
	Material->ConditionalPostLoad();
	TStringBuilder<4096> Report;
	Report.Appendf(TEXT("--- MATERIAL SUMMARY: %s ---\n\n"), *Material->GetName());
	TConstArrayView<TObjectPtr<UMaterialExpression>> Expressions = Material->GetExpressions();
	if (Expressions.Num() == 0)
	{
		return TEXT("This material has no expression nodes in its graph.");
	}
	Report.Append(TEXT("--- Nodes ---\n"));
	for (const UMaterialExpression* Expression : Expressions)
	{
		if (Expression)
		{
			TArray<FString> Captions;
			Expression->GetCaption(Captions);
			const FString NodeCaption = Captions.Num() > 0 ? Captions[0] : Expression->GetClass()->GetName();
			Report.Appendf(TEXT("- Node: %s (Class: %s)\n"), *NodeCaption, *Expression->GetClass()->GetName());
			if (const UMaterialExpressionTextureSample* TextureSampler = Cast<const UMaterialExpressionTextureSample>(Expression))
			{
				if (TextureSampler->Texture)
				{
					Report.Appendf(TEXT("    - Texture: %s\n"), *TextureSampler->Texture->GetName());
				}
				else
				{
					Report.Appendf(TEXT("    - Texture: None\n"));
				}
			}
		}
	}
	return FString(Report);
#else
	return TEXT("Material analysis is only available in an editor build.");
#endif
}
