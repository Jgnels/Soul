#pragma once
#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"

// Presentation only: one logical canvas keeps text, art and click targets in sync.
// No gameplay state or asset ownership lives here.
struct FSoulHUDTheme
{
    AHUD& HUD;
    UCanvas& Canvas;
    float Scale, W, H;
    FVector2D Mouse = FVector2D(-1,-1);
    const FLinearColor Ink{.91f,.87f,.74f,1}, Muted{.62f,.66f,.65f,1};
    const FLinearColor Gold{.72f,.51f,.23f,1}, Bright{1.f,.81f,.40f,1};
    const FLinearColor Back{.027f,.034f,.039f,.97f}, Edge{.29f,.24f,.16f,1};
    FSoulHUDTheme(AHUD& InHUD, UCanvas& InCanvas) : HUD(InHUD),Canvas(InCanvas)
    {
        Scale=FMath::Max(.5f,FMath::Min(Canvas.ClipX/1280.f,Canvas.ClipY/720.f));
        W=Canvas.ClipX/Scale; H=Canvas.ClipY/Scale;
        if(auto* PC=HUD.GetOwningPlayerController()) { PC->GetMousePosition(Mouse.X,Mouse.Y); Mouse/=Scale; }
    }
    void Rect(float X,float Y,float Width,float Height,FLinearColor Color)
    { HUD.DrawRect(Color,X*Scale,Y*Scale,Width*Scale,Height*Scale); }
    void Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness=1)
    { HUD.DrawLine(X*Scale,Y*Scale,X2*Scale,Y2*Scale,Color,Thickness*Scale); }
    void Text(const FString& Value,float X,float Y,FLinearColor Color,float Size=1.15f)
    { HUD.DrawText(Value,Color,X*Scale,Y*Scale,GEngine->GetSmallFont(),Scale*Size,false); }
    void FitText(const FString& Value,float X,float Y,float Width,FLinearColor Color,float Size=1.15f)
    {
        float TW=0,TH=0; Canvas.StrLen(GEngine->GetSmallFont(),Value,TW,TH);
        // Ellipsize instead of shrinking important text below the readable body size.
        FString Label=Value;
        while(TW*Size>Width && Label.Len()>3) { Label.LeftChopInline(1); Canvas.StrLen(GEngine->GetSmallFont(),Label+TEXT("..."),TW,TH); }
        if(Label!=Value) Label+=TEXT("...");
        Text(Label,X,Y,Color,Size);
    }
    void Panel(float X,float Y,float Width,float Height,bool Selected=false)
    {
        Rect(X+2,Y+3,Width,Height,FLinearColor(0,0,0,.38f));
        Rect(X,Y,Width,Height,Back);
        Rect(X+2,Y+2,Width-4,Height-4,Selected?FLinearColor(.105f,.095f,.062f,.96f):FLinearColor(.041f,.049f,.050f,.98f));
        const FLinearColor Border=Selected?Gold:Edge;
        Line(X,Y,X+Width,Y,Border); Line(X,Y+Height,X+Width,Y+Height,Border);
        Line(X,Y,X,Y+Height,Border); Line(X+Width,Y,X+Width,Y+Height,Border);
        for(float CX:{X,X+Width}) for(float CY:{Y,Y+Height})
        { const float DX=CX==X?1.f:-1.f,DY=CY==Y?1.f:-1.f;
          Line(CX,CY,CX+DX*9,CY,Gold,2); Line(CX,CY,CX,CY+DY*9,Gold,2); }
    }
    bool Button(FName Id,const FString& Label,float X,float Y,float Width,float Height,bool Selected=false)
    {
        const bool Hover=Mouse.X>=X&&Mouse.X<=X+Width&&Mouse.Y>=Y&&Mouse.Y<=Y+Height;
        Panel(X,Y,Width,Height,Selected||Hover);
        FitText(Label,X+10,Y+(Height-13)*.5f,Width-20,Selected||Hover?Bright:Ink);
        HUD.AddHitBox(FVector2D(X,Y)*Scale,FVector2D(Width,Height)*Scale,Id,true,10);
        return Hover;
    }
    void Bar(float X,float Y,float Width,float Fraction,FLinearColor Color)
    { Rect(X,Y,Width,4,FLinearColor(.01f,.015f,.019f,1)); Rect(X,Y,Width*FMath::Clamp(Fraction,0.f,1.f),4,Color); }
    // Original line emblems: fire, lightning, frost, ward, wind, blade, standard.
    void Emblem(int32 Kind,float X,float Y,float R,FLinearColor Color)
    {
        auto L=[&](float A,float B,float C,float D){Line(X+A*R,Y+B*R,X+C*R,Y+D*R,Color,1.7f);};
        if(Kind==0){L(0,-1,-.6f,.15f);L(-.6f,.15f,-.35f,.8f);L(-.35f,.8f,.35f,.8f);L(.35f,.8f,.6f,.15f);L(.6f,.15f,0,-1);L(0,.7f,-.1f,0);L(-.1f,0,.3f,.3f);}
        else if(Kind==1){L(.3f,-1,-.6f,.1f);L(-.6f,.1f,.1f,.1f);L(.1f,.1f,-.3f,1);L(-.3f,1,.6f,-.15f);L(.6f,-.15f,0,-.15f);}
        else if(Kind==2){for(int I=0;I<6;++I){float A=I*PI/3;float C=FMath::Cos(A),S=FMath::Sin(A);L(0,0,C,S);L(C*.65f,S*.65f,C*.6f-S*.25f,S*.6f+C*.25f);}}
        else if(Kind==3){L(-.65f,-.8f,.65f,-.8f);L(.65f,-.8f,.55f,.35f);L(.55f,.35f,0,1);L(0,1,-.55f,.35f);L(-.55f,.35f,-.65f,-.8f);L(0,-.4f,0,.55f);L(-.3f,0,.3f,0);}
        else if(Kind==4){L(-.8f,.65f,.8f,-.8f);L(-.8f,.65f,-.15f,-.6f);L(-.15f,-.6f,.8f,-.8f);L(-.45f,.35f,.7f,-.25f);L(-.65f,.55f,.3f,.5f);}
        else if(Kind==5){L(-.6f,.85f,.65f,-.85f);L(.65f,-.85f,.55f,-.25f);L(.65f,-.85f,.1f,-.65f);L(-.65f,.2f,.05f,.7f);}
        else {L(-.5f,1,-.5f,-1);L(-.5f,-.85f,.7f,-.85f);L(.7f,-.85f,.4f,-.3f);L(.4f,-.3f,.7f,.2f);L(.7f,.2f,-.5f,.2f);}
    }
};
