// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#if !UE_BUILD_SHIPPING

class SUserWidgetExample
	: public SUserWidget
{
	public:

	SLATE_USER_ARGS(SUserWidgetExample)
		: _Title()
	{ }
		SLATE_ARGUMENT(FText, Title)
	SLATE_END_ARGS()

	virtual void Construct( const FArguments& InArgs ) = 0;

	virtual void DoStuff() = 0;
};

#endif // #if !UE_BUILD_SHIPPING
