#include "SoulHeartlandBridgeGeometry.h"
namespace SoulHeartlandBridge
{
const TArray<FVector>& River(){static const TArray<FVector> Points={  FVector(620.7324,3443.2262,117.7870),
  FVector(517.7912,3168.7164,112.5972),
  FVector(414.8500,2894.2066,106.4649),
  FVector(320.4873,2619.6968,98.4227),
  FVector(234.7030,2345.1870,88.4703),
  FVector(148.9186,2070.6772,78.5180),
  FVector(45.9775,1796.1674,64.2430),
  FVector(-56.9637,1521.6576,51.2874),
  FVector(-151.3265,1247.1478,39.2114),
  FVector(-237.1108,972.6380,27.5752),
  FVector(-322.8951,698.1282,15.9390),
  FVector(-425.8363,423.6184,8.3491),
  FVector(-528.7774,149.1086,0.7592),
  FVector(-658.8837,-102.5254,-5.7898),
  FVector(-816.1549,-331.2836,-11.2979),
  FVector(-973.4261,-560.0418,-16.8060),
  FVector(-1143.9735,-772.4601,-20.5205),
  FVector(-1327.7970,-968.5385,-21.4360),
  FVector(-1511.6206,-1164.6169,-22.3516),
  FVector(-1695.4441,-1360.6954,-23.2671),
  FVector(-1879.2676,-1556.7738,-23.2671),
  FVector(-2063.0912,-1752.8522,-23.2671),
  FVector(-2246.9147,-1948.9306,-23.2671),
  FVector(-2417.4621,-2161.3489,-23.2671),
  FVector(-2574.7333,-2390.1071,-23.2671),
  FVector(-2732.0046,-2618.8653,-23.2671),
  FVector(-2844.9539,-2870.4993,-23.2671),
  FVector(-2913.5814,-3145.0091,-23.2671),
  FVector(-2982.2088,-3419.5189,-23.2671)};return Points;}
FVector ToDeck(const FVector& P){return FRotator(0,15.74778799f,0).RotateVector(P);}
FVector FromDeck(const FVector& P){return FRotator(0,-15.74778799f,0).RotateVector(P);}
static float Bank(const FVector& P)
{
 float Best=MAX_flt,Side=1;
 for(int32 I=1;I<River().Num();++I)
 {
  FVector A=River()[I-1],B=River()[I];A.Z=B.Z=0;
  const FVector Q=FMath::ClosestPointOnSegment(FVector(P.X,P.Y,0),A,B);
  const float D=FVector::DistSquared2D(P,Q);
  if(D<Best){Best=D;const FVector T=(B-A).GetSafeNormal2D();Side=FVector::DotProduct(P-Q,FVector(-T.Y,T.X,0));}
 }
 return Side<0?-1.f:1.f;
}
bool IsWater(const FVector& P)
{
 const FVector D=ToDeck(P);
 if(FMath::Abs(D.X)<=1350&&FMath::Abs(D.Y)<=230)return false;
 for(int32 I=1;I<River().Num();++I)
 {
  FVector A=River()[I-1],B=River()[I];A.Z=B.Z=0;
  if(FVector::DistSquared2D(P,FMath::ClosestPointOnSegment(FVector(P.X,P.Y,0),A,B))<FMath::Square(200.f))return true;
 }
 return false;
}
bool CrossesWater(const FVector& A,const FVector& B)
{
 if(A.ContainsNaN()||B.ContainsNaN())return true;
 const int32 Steps=FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(A,B)/50.f),1,400);
 for(int32 I=0;I<=Steps;++I)if(IsWater(FMath::Lerp(A,B,float(I)/Steps)))return true;
 return false;
}
bool Approach(const FVector& From,const FVector& Target,FVector& Out)
{
 // Once parapets have physical collision, a same-bank destination can still
 // require leaving through the bridge mouth rather than steering through stone.
 const FVector A=ToDeck(From),B=ToDeck(Target);bool CrossesRail=false;
 if(!FMath::IsNearlyZero(B.Y-A.Y))for(float Side:{-1.f,1.f})
 {
  const float T=(Side*220.f-A.Y)/(B.Y-A.Y);
  if(T>=0&&T<=1&&FMath::Abs(FMath::Lerp(A.X,B.X,T))<1300.f)CrossesRail=true;
 }
 if(!CrossesWater(From,Target)&&!CrossesRail)return false;
 const float Side=Bank(From),Other=Bank(Target);
 const FVector Entry=FromDeck(FVector(Side*1500,0,0));
 const FVector D=ToDeck(From);
 // First converge at this bank's mouth. Then use the actual deck; no teleport,
 // terrain shortcut, or changed battle result. RB still moves every body.
 Out=FVector::Dist2D(From,Entry)<260 || (FMath::Abs(D.Y)<210&&FMath::Abs(D.X)<1500)
  ?FromDeck(FVector(Other*1500,0,0)):Entry;
 return true;
}
}
