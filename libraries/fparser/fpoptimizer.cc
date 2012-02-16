/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Function optimizer                                                      *|
|*-------------------------------------------------------------------------*|
|* Copyright: Joel Yliluoma                                                *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

/* NOTE:
 This file contains generated code (from the optimizer sources) and is
 not intended to be modified by hand. If you want to modify the optimizer,
 download the development version of the library.
*/

#include "fpconfig.hh"
#ifdef FP_SUPPORT_OPTIMIZER
#include "fparser.hh"
#include "fptypes.hh"
#define i43 nW1 a),
#define i33 info.yH
#define i23 ,cAtan2
#define i13 =false;
#define i03 cAbsIf
#define tZ3 "Found "
#define tY3 },{{2,
#define tX3 {if(apos
#define tW3 stackpos
#define tV3 "dup(%u) "
#define tU3 GetOpcode
#define tT3 eR{assert
#define tS3 "%d, cost "
#define tR3 "immed "<<
#define tQ3 mFuncParsers
#define tP3 "PUSH ";xE3(
#define tO3 stderr
#define tN3 sep2=" "
#define tM3 FPHASH_CONST
#define tL3 cache_needed[
#define tK3 fprintf
#define tJ3 ::cout<<"Applying "
#define tI3 ||tree.tU3
#define tH3 HANDLE_UNARY_CONST_FUNC
#define tG3 within,
#define tF3 y21){if
#define tE3 c_count
#define tD3 s_count
#define tC3 MaxOp
#define tB3 2)lS 2*
#define tA3 x11.cG3
#define t93 l7 0,2,
#define t83 <eS1(0)
#define t73 );else{
#define t63 (n6)nN3
#define t53 b.Value)
#define t43 b.Opcode
#define t33 .nB synth
#define t23 ].swap(
#define t13 =synth.
#define t03 codes[b
#define eZ3 Value){
#define eY3 whydump
#define eX3 i01 eF2
#define eW3 for(x53
#define eV3 ;for yS{
#define eU3 );synth
#define eT3 info.SaveMatchedParamIndex(
#define eS3 for(;a<
#define eR3 nparams
#define eQ3 first!=
#define eP3 i03,
#define eO3 l3 4,1,
#define eN3 cTan,xJ
#define eM3 cLog,xJ
#define eL3 l3 0,1,
#define eK3 cHypot,
#define eJ3 nR 0,
#define eI3 cAbs nR
#define eH3 std::cO
#define eG3 fp_pow(
#define eF3 ,cM2 lD
#define eE3 .second
#define eD3 ]eE3
#define eC3 ].first
#define eB3 Ne_Mask
#define eA3 Gt_Mask
#define e93 Lt_Mask
#define e83 {nL eS1(
#define e73 131,4,1,
#define e63 tC1 max
#define e53 tU cMul);
#define e43 131,8,1
#define e33 FindPos
#define e23 nN yL
#define e13 public:
#define e03 result xM
#define cZ3 result))lA2
#define cY3 result(
#define cX3 {data->
#define cW3 .eW1 n]
#define cV3 eG cK1);
#define cU3 eG yR1
#define cT3 if(eY1==
#define cS3 ),has_max(
#define cR3 (count
#define cQ3 .empty()
#define cP3 .iH1 a);
#define cO3 GetParamCount(nQ
#define cN3 GetParamCount();
#define cM3 ;}case
#define cL3 pclone
#define cK3 sim.x3
#define cJ3 fpdata
#define cI3 cCosh nR
#define cH3 cCosh,
#define cG3 Immeds
#define cF3 l6 1,
#define cE3 x8(tB2
#define cD3 newpow
#define cC3 change
#define cB3 133,2,
#define cA3 i01 x2
#define c93 Params
#define c83 Needs
#define c73 byteCode
#define c63 eT1&occ=
#define c53 child)
#define c43 AddFrom(
#define c33 lT1 nE==
#define c23 ;for xK2
#define c13 tC1 lY1
#define c03 factor_t
#define yZ3 tree.xJ1
#define yY3 yA tmp2)
#define yX3 value1
#define yW3 model.
#define yV3 true;n31
#define yU3 &&p0.max
#define yT3 p2 eT ifp2
#define yS3 yJ p2;p2
#define yR3 cAbsNot
#define yQ3 switch(tX
#define yP3 tZ case
#define yO3 IsNever
#define yN3 }switch
#define yM3 stackptr
#define yL3 nN[++IP]
#define yK3 const xH
#define yJ3 yE cPow)
#define yI3 default_function_handling
#define yH3 cLog);x9
#define yG3 ;sim.Push(
#define yF3 eS1(0.5)
#define yE3 iJ 1,
#define yD3 [funcno].
#define yC3 :start_at()
#define yB3 Rehash(tT yA
#define yA3 .size()
#define y93 IsLogicalValue nW1
#define y83 (tree))e5
#define y73 stack[stack yA3-
#define y63 stack yL
#define y53 tmp yC 0))
#define y43 ));tmp tU
#define y33 ,(long tG1
#define y23 );tmp2 yC
#define y13 cMul);xK
#define y03 1),eS1(1))
#define xZ3 constvalue
#define xY3 lD 0));
#define xX3 )nG1 xK
#define xW3 nN[IP]==
#define xV3 opcodes
#define xU3 did_muli
#define xT3 switch(lF3.first iY2
#define xS3 ;for iL1
#define xR3 Ge0Lt1
#define xQ3 &&p.max
#define xP3 :tree.
#define xO3 =y6 a));if(
#define xN3 used[b]
#define xM3 e8(),yG<
#define xL3 size_t n
#define xK3 sizeof(
#define xJ3 cNotNot,
#define xI3 cOr yF
#define xH3 359463,
#define xG3 2,7168,
#define xF3 param.
#define xE3 DumpTree
#define xD3 Gt0Le1
#define xC3 y21){eS1
#define xB3 base,eS1
#define xA3 ==cOr)l9
#define x93 cAdd iU1
#define x83 (cM2 lD
#define x73 lD 2));
#define x63 eT cond
#define x53 iX2 0;b<
#define x43 {}range(
#define x33 .n_int_sqrt
#define x23 ,cPow,xJ
#define x13 ,{1,218,
#define x03 lD 1))
#define nZ3 tmp yA tree
#define nY3 eS1(1)))
#define nX3 iterator
#define nW3 begin();
#define nV3 TreeSet
#define nU3 parent
#define nT3 insert(i
#define nS3 newrel
#define nR3 eS1(2)));
#define nQ3 {eS1 tmp
#define nP3 iU.hash1
#define nO3 (tree)!=
#define nN3 )n5 lC
#define nM3 break;}
#define nL3 b_needed
#define nK3 cachepos
#define nJ3 half=
#define nI3 src_pos
#define nH3 reserve(
#define nG3 treeptr
#define nF3 iC2 size()
#define nE3 .resize(
#define nD3 eU1 void
#define nC3 ImmedTag
#define nB3 a,const
#define nA3 RefCount
#define n93 Birth();
#define n83 mulgroup
#define n73 template
#define n63 cost_t
#define n53 n72 nZ
#define n43 middle
#define n33 ))break;eS1
#define n23 };enum
#define n13 if(op==
#define n03 (p1.xJ1
#define lZ3 cLog2by);
#define lY3 sqrt_cost
#define lX3 const int
#define lW3 mul_count
#define lV3 maxValue1
#define lU3 minValue1
#define lT3 maxValue0
#define lS3 minValue0
#define lR3 ValueType
#define lQ3 >(eS1(1),
#define lP3 abs_mul
#define lO3 pos_set
#define lN3 goto e3
#define lM3 nF1);}if(
#define lL3 yE cAdd);
#define lK3 subtree
#define lJ3 invtree
#define lI3 lV1=r.specs;if(r.found){
#define lH3 ,lV1,info
#define lG3 a;if(&t91
#define lF3 parampair
#define lE3 rulenumit
#define lD3 cIf,l6 3,
#define lC3 MakeEqual
#define lB3 nH1,l4::
#define lA3 nH1,{l4::
#define l93 newbase
#define l83 branch1op
#define l73 branch2op
#define l63 overlap
#define l53 truth_a
#define l43 ),Value(
#define l33 );nW l4::
#define l23 if nW1 0)
#define l13 found_dup
#define l03 &1)?(poly^(
#define iZ2 (xF3
#define iY2 ){case
#define iX2 size_t b=
#define iW2 i0 xQ3
#define iV2 has_min
#define iU2 Plan_Has(
#define iT2 ;if(half
#define iS2 ;}void
#define iR2 )iS2
#define iQ2 const nI2
#define iP2 yV1 class
#define iO2 namespace
#define iN2 rhs.hash2;}
#define iM2 rhs.hash1
#define iL2 ::res,b8<
#define iK2 inverted
#define iJ2 has_max=
#define iI2 has_max)
#define iH2 iftree
#define iG2 depcodes
#define iF2 explicit
#define iE2 VarBegin
#define iD2 cM3 lZ2
#define iC2 c93.
#define iB2 ].data);
#define iA2 y4)));nW
#define i92 .what eX
#define i82 ;if(fp_equal(
#define i72 x02 yJ
#define i62 begin(),
#define i52 cond_add
#define i42 cond_mul
#define i32 cond_and
#define i22 yG n91
#define i12 bool e31
#define i02 unsigned
#define tZ2 costree
#define tY2 sintree
#define tX2 leaf_count
#define tW2 =GetParam(
#define tV2 sub_params
#define tU2 printf(
#define tT2 cbrt_count
#define tS2 sqrt_count
#define tR2 Finite
#define tQ2 p1 eT ifp1
#define tP2 pcall_tree
#define tO2 after_powi
#define tN2 grammar
#define tM2 lE 0,1,
#define tL2 cCos,xJ
#define tK2 cEqual,
#define tJ2 ,eO3 507 tS
#define tI2 ,t71 l7 2,2,
#define tH2 cLog nR
#define tG2 l0 2,
#define tF2 cPow,nU
#define tE2 cAdd,xZ 2,
#define tD2 cInv,xZ 1,
#define tC2 cNeg,xZ 1,
#define tB2 ),0},{
#define tA2 x11.SubTrees
#define t92 x11.Others
#define t82 param=*i01
#define t72 std::move(
#define t62 nM3 switch(
#define t52 constraints=
#define t42 .constraints
#define t32 data;data.
#define t22 MakeNEqual
#define t12 for yS eC1
#define t02 yA mul)
#define eZ2 Dump(std::
#define eY2 isInteger(
#define eX2 (cond.cI cA2
#define eW2 yA3;++
#define eV2 nG1 r;r tU
#define eU2 Comparison
#define eT2 needs_flip
#define eS2 {data xB lM
#define eR2 ,eQ,synth);
#define eQ2 (half&63)-1;
#define eP2 value]
#define eO2 ));TriTruthValue
#define eN2 );range.nN2
#define eM2 ,2,1,4,1,2,
#define eL2 >StackMax)
#define eK2 ~size_t(0)
#define eJ2 );t4=!t4;}
#define eI2 ;}yV1 static
#define eH2 info.lQ[b].
#define eG2 const std::eO
#define eF2 Rule&rule,
#define eE2 .GetHash().
#define eD2 (list.first
#define eC2 ;iU.hash2+=
#define eB2 ,xA1);lC
#define eA2 ,const e1&
#define e92 struct
#define e82 cGreater,
#define e72 tree lD 0)
#define e62 const eS1&
#define e52 mul_item
#define e42 innersub
#define e32 cbrt_cost
#define e22 best_cost
#define e12 )))l81 lH
#define e02 result i0
#define cZ2 fp_mod(m.
#define cY2 Compare>
#define cX2 (*x6)[a].info
#define cW2 tree y21 yI
#define cV2 tree nE==
#define cU2 condition
#define cT2 per_item
#define cS2 item_type
#define cR2 first2
#define cQ2 ,l8 0,2,
#define cP2 tS 396676
#define cO2 Decision
#define cN2 not_tree
#define cM2 leaf1
#define cL2 =tree lD
#define cK2 group_by
#define cJ2 ->second
#define cI2 targetpos
#define cH2 eat_count
#define cG2 ParamSpec
#define cF2 Forget()
#define cE2 exponent
#define cD2 ,bool abs){
#define cC2 synth.Find(
#define cB2 params
#define cA2 &&cond eH))
#define c92 source_tree
#define c82 nE==cLog2&&
#define c72 =lT1;bool iQ
#define c62 <t8,n63>
#define c52 p1_evenness
#define c42 c32 i13 if(
#define c32 tC1 iV2
#define c22 isNegative(
#define c12 n73 i5
#define c02 neg_set
#define yZ2 StackTopIs(
#define yY2 cNop,cNop}}
#define yX2 synth.PushImmed(
#define yW2 FPoptimizer_ByteCode yU2
#define yV2 FPoptimizer_ByteCode::
#define yU2 ::ByteCodeSynth x8
#define yT2 cTanh,cNop,
#define yS2 matches
#define yR2 goto fail;}
#define yQ2 cSin,xJ
#define yP2 cTan nR
#define yO2 cCos nR
#define yN2 negated
#define yM2 i7,1,iX+1);
#define yL2 CodeTree
#define yK2 yL2 x8
#define yJ2 ifdata
#define yI2 best_score
#define yH2 mulvalue
#define yG2 pow_item
#define yF2 nE==cPow&&tJ
#define yE2 PowiResult
#define yD2 maxValue
#define yC2 minValue
#define yB2 ;pow tU cPow);pow
#define yA2 result cM3
#define y92 fp_min(xA,
#define y82 set_min_max(
#define y72 div_tree
#define y62 pow_tree
#define y52 preserve
#define y42 PullResult()
#define y32 dup_or_fetch
#define y22 nominator]
#define y12 Rehash(false
#define y02 test_order
#define xZ2 lF3,
#define xY2 .param_count
#define xX2 shift(index)
#define xW2 rulenumber
#define xV2 cTanh nR
#define xU2 (tree nE
#define xT2 GetDepth()
#define xS2 factor_immed
#define xR2 changes
#define xQ2 tU tree nE);
#define xP2 tU cond nE
#define xO2 Become nW1
#define xN2 },0,0x1},{{1,
#define xM2 ,lJ 0x7 tY3
#define xL2 ;n41 eT y9 lD
#define xK2 (size_t a=
#define xJ2 ;}static yN1
#define xI2 tree lD a)
#define xH2 for(typename
#define xG2 exp_diff
#define xF2 ExponentInfo
#define xE2 lower_bound(
#define xD2 factor
#define xC2 is_logical
#define xB2 newrel_and
#define xA2 Suboptimal
#define x92 cW[c tO
#define x82 IsAlways;if(
#define x72 n73 nA1
#define x62 res_stackpos
#define x52 half_pos
#define x42 ;}else{x6=new
#define x32 )lS 3*
#define x22 {e1 start_at;
#define x12 ,(long double)
#define x02 .Rehash()
#define nZ2 )return
#define nY2 );cK3 iJ 2,
#define nX2 return false;}
#define nW2 );nM3
#define nV2 xD1 tA+1);
#define nU2 ;i7.Remember(
#define nT2 .match_tree
#define nS2 l81 true;}
#define nR2 nD OPCODE
#define nQ2 yG x8&immed,
#define nP2 >>1)):(
#define nO2 CodeTreeData
#define nN2 multiply(
#define nM2 i0 i13
#define nL2 .n73
#define nK2 var_trees
#define nJ2 cOr,lP 2,
#define nI2 yL2&
#define nH2 ::Optimize(){
#define nG2 second eE3;
#define nF2 second.first;
#define nE2 log2_exponent
#define nD2 tT.swap(tmp);
#define nC2 Value(Value::
#define nB2 dup_fetch_pos
#define nA2 nN,size_t&l31
#define n92 *)&*start_at;
#define n82 yO3 yI lC
#define n72 ,tree))
#define n62 ContainsOtherCandidates(
#define n52 ,cPow l7 2,2,
#define n42 cSin nR
#define n32 lK 2},0,iR 1,
#define n22 lK 1},0,iR 1,
#define n12 Value_EvenInt
#define n02 Sign_Negative
#define lZ2 SubFunction:{
#define lY2 ParamHolder:{
#define lX2 MakeFalse,{l4
#define lW2 if(xW lD a)iA
#define lV2 ConditionType
#define lU2 SpecialOpcode
#define lT2 synth_it
#define lS2 fp_max(xA);
#define lR2 assimilated
#define lQ2 fraction
#define lP2 0x12},{{
#define lO2 DUP_BOTH();
#define lN2 -1-offset].
#define lM2 tU3()
#define lL2 parent_opcode)
#define lK2 TreeCounts
#define lJ2 bool t4 i13
#define lI2 SetOpcode(
#define lH2 found_log2
#define lG2 div_params
#define lF2 .CopyOnWrite()
#define lE2 immed_sum
#define lD2 OPCODE(opcode)
#define lC2 std::cout<<"POP "
#define lB2 (stack yA3-
#define lA2 break;result*=
#define l92 FactorStack x8
#define l82 IsAlways yI lC
#define l72 l7 2,2,lB1
#define l62 248024 tS
#define l52 cAnd,lP 2,
#define l42 cNot nR
#define l32 cMul,xZ 2,
#define l22 DumpHashesFrom
#define l12 replacing_slot
#define l02 RefParams
#define iZ1 if_always[
#define iY1 WhatDoWhenCase
#define iX1 exponent_immed
#define iW1 new_base_immed
#define iV1 base_immed
#define iU1 ||op1==
#define iT1 (size_t a xX
#define iS1 data[a eD3
#define iR1 AddCollection(
#define iQ1 if(newrel_or==
#define iP1 DUP_ONE(apos);
#define iO1 flipped
#define iN1 .UseGetNeeded(
#define iM1 e9 2,131,
#define iL1 (i02
#define iK1 OptimizedUsing
#define iJ1 Var_or_Funcno
#define iI1 iJ1;
#define iH1 DelParam(
#define iG1 typename nX1::nX3
#define iF1 )nZ2 true
#define iE1 GetParams(
#define iD1 crc32_t
#define iC1 fphash_value_t
#define iB1 signed_chain
#define iA1 IsDefined())
#define i91 MinusInf
#define i81 n_immeds
#define i71 FindClone(xK
#define i61 denominator]
#define i51 needs_rehash
#define i41 AnyWhere_Rec
#define i31 minimum_need
#define i21 ~i02(0)
#define i11 tC1 min
#define i01 (const
#define tZ1 ,i01 void*)&
#define tY1 41,42,43,44,
#define tX1 constraints&
#define tW1 tT.iH1
#define tV1 p1_logical_b
#define tU1 p0_logical_b
#define tT1 p1_logical_a
#define tS1 p0_logical_a
#define tR1 ,PowiCache&i7,
#define tQ1 synth.DoDup(
#define tP1 else if(
#define tO1 cache_needed
#define tN1 e9 2,1,e9 2,
#define tM1 treelist
#define tL1 IsDescendantOf(
#define tK1 has_bad_balance
#define tJ1 (tree,std::cout)
#define tI1 .SetParamsMove(
#define tH1 c03 xD2
#define tG1 double)cE2
#define tF1 {case IsAlways:
#define tE1 e02=false
#define tD1 ;cE2.Rehash(
#define tC1 result.
#define tB1 range x8 result
#define tA1 TopLevel)
#define t91 *start_at){x6=(
#define t81 (rule,tree,info
#define t71 cNEqual
#define t61 ,cEqual l7 2,2,
#define t51 cAdd,AnyParams,
#define t41 lP2 xG3
#define t31 ,cNotNot nR
#define t21 ,cLess l7 2,2,
#define t11 Oneness_NotOne|
#define t01 Value_IsInteger
#define eZ1 iK1(
#define eY1 reltype
#define eX1 SequenceOpcodes
#define eW1 sep_list[
#define eV1 );eG n83);
#define eU1 l02);
#define eT1 TreeCountItem
#define eS1 Value_t
#define eR1 divgroup
#define eQ1 ,eS1(-1)))xF
#define eP1 set_min(fp_floor
#define eO1 pihalf_limits
#define eN1 y41 p0.min>=0.0)
#define eM1 MaxChildDepth
#define eL1 situation_flags&
#define eK1 i02 opcode)
#define eJ1 =yM|i02(nN yA3
#define eI1 std::pair<It,It>
#define eH1 eO3 483 tS
#define eG1 ,l8 0,1,
#define eF1 tG2 7168,
#define eE1 Value_Logical
#define eD1 new_factor_immed
#define eC1 if(remaining[a])
#define eB1 occurance_pos
#define eA1 exponent_hash
#define e91 exponent_list
#define e81 CollectionSet x8
#define e71 CollectMulGroup(
#define e61 source_set
#define e51 cE2,nV3
#define e41 produce_count
#define e31 operator
#define e21 )yJ3;lC
#define e11 back().thenbranch
#define e01 ParamSpec_Extract
#define cZ1 retry_anyparams_3
#define cY1 retry_anyparams_2
#define cX1 needlist_cached_t
#define cW1 grammar_rules[*r]
#define cV1 tF2 0x1 tY3
#define cU1 CodeTreeImmed x8(
#define cT1 GetParamCount()==
#define cS1 by_float_exponent
#define cR1 fp_equal(cE2
#define cQ1 new_exp
#define cP1 end()&&i->first==
#define cO1 yB3 r);}
#define cN1 return BecomeZero;
#define cM1 return BecomeOne;
#define cL1 if(lQ yA3<=n2)
#define cK1 addgroup
#define cJ1 found_log2by
#define cI1 ())yE cMul);lC
#define cH1 >=eS1(0)
#define cG1 nE==yR3)
#define cF1 ParsePowiMuli(
#define cE1 branch1_backup
#define cD1 branch2_backup
#define cC1 exponent_map
#define cB1 plain_set
#define cA1 LightWeight(
#define c91 }nM3 case
#define c81 cN3++b)
#define c71 synth.x5 1
#define c61 ,i7 eR2
#define c51 if(value
#define c41 set_max(fp_ceil cY
#define c31 e62 v,n7
#define c21 {eS1 cE2=
#define c11 should_regenerate=true;
#define c01 should_regenerate,
#define yZ1 Collection
#define yY1 RelationshipResult
#define yX1 Subdivide_Combine(
#define yW1 long value
#define yV1 n73 lT
#define yU1 yV1 cA
#define yT1 yV1 e92
#define yS1 eP nE3 StackMax
#define yR1 subgroup
#define yQ1 best_sep_factor
#define yP1 tP1!result
#define yO1 needlist_cached
#define yN1 inline i02
#define yM1 221646 tS 24803
#define yL1 Constness_Const
#define yK1 opcode,bool pad
#define yJ1 n_occurrences
#define yI1 changed=true;
#define yH1 iH1 a);}
#define yG1 MakesInteger(
#define yF1 e62 value
#define yE1 best_sep_cost
#define yD1 MultiplicationRange
#define yC1 ;p1.yB3 p1
#define yB1 yV1 eS1
#define yA1 n_stacked
#define y91 AnyParams_Rec
#define y81 continue;
#define y71 Become(value lD 0))
#define y61 ,cGreater l7 2,2,
#define y51 yV1 inline TriTruthValue
#define y41 .iV2&&
#define y31 ));n41 y3 op1 tT.DelParams(
#define y21 .IsImmed()
#define y11 =comp.AddItem(atree
#define y01 needs_sincos
#define xZ1 Recheck_RefCount_Div
#define xY1 Recheck_RefCount_Mul
#define xX1 n83.
#define xW1 n83;n83 tU
#define xV1 MultiplyAndMakeLong(
#define xU1 cMul);y53;tmp
#define xT1 covers_plus1
#define xS1 lD2);
#define xR1 if(synth.FindAndDup(
#define xQ1 SynthesizeParam(
#define xP1 public e8,public yG<
#define xO1 grammar_func
#define xN1 221426 tS 237795
#define xM1 t93 165888 tS
#define xL1 Modulo_Radians},
#define xK1 tT.SetParam(
#define xJ1 GetImmed()
#define xI1 PositionType
#define xH1 CollectionResult
#define xG1 yV1 bool
#define xF1 const_offset
#define xE1 stacktop_desired
#define xD1 SetStackTop(
#define xC1 ,cLessOrEq l7 2,2,
#define xB1 yV1 void
#define xA1 cond_type
#define x91 Recheck_RefCount_RDiv
#define x81 static const range x8
#define x71 fPExponentIsTooLarge(
#define x61 CollectMulGroup_Item(
#define x51 pair<eS1,nV3>
#define x41 covers_full_cycle
#define x31 AssembleSequence(
#define x21 x8(rule.repl_param_list,
#define x11 NeedList
#define x01 )lF3 eE3
#define nZ1 <<std::dec<<")";}
#define nY1 &&IsLogicalValue(
#define nX1 TreeCountType x8
#define nW1 (tree lD
#define nV1 <yK2>
#define nU1 std::pair<T1,T2>&
#define nT1 n73<typename
#define nS1 has_good_balance_found
#define nR1 Rehash();tV2 yL
#define nQ1 found_log2_on_exponent
#define nP1 covers_minus1
#define nO1 needs_resynth
#define nN1 immed_product
#define nM1 }},{ProduceNewTree,2,1,
#define nL1 ,2,1)nS if(found[data.
#define nK1 t62 bitmask&
#define nJ1 Sign_Positive
#define nI1 {DataP slot_holder(xY[
#define nH1 ::MakeTrue
#define nG1 {yK2
#define nF1 tree.iH1 a
#define nE1 tree lD 1)y21&&
#define nD1 },{l4::MakeNotP0,l4::
#define nC1 SetParamMove(
#define nB1 CodeTreeImmed(eS1(
#define nA1 <i02 cY2 void
#define n91 <yL2>&
#define n81 yG<i02>&c73,
#define n71 void ByteCodeSynth x8::
#define n61 )const{return
#define n51 rhs n61 hash1
#define n41 changed_if
#define n31 min=eS1(0);
#define n21 opposite=
#define n11 7168 tS 279818,
#define n01 l81 xA2;
#define lZ1 MatchResultType
#define lY1 resulting_exponent
#define lX1 Unknown:default:;}
#define lW1 ,lE 2,1,
#define lV1 (*x6)[a].start_at
#define lU1 ,cAdd,SelectedParams,0},0,
#define lT1 GetParam(a)
#define lS1 inverse_nominator]
#define lR1 void FunctionParserBase
#define lQ1 ,nN,IP,limit,y1,stack);
#define lP1 ByteCodeSynth x8&synth)
#define lO1 xS3 a=0;a<xT;++a)
#define lN1 ;std::cout<<
#define lM1 const yK2
#define lL1 const yG nV1
#define lK1 synth.AddOperation(
#define lJ1 tQ1 found[data.
#define lI1 SetParams(iE1));
#define lH1 o<<"("<<std::hex<<data.
#define lG1 IfBalanceGood(
#define lF1 n_as_tan_param
#define lE1 changed_exponent
#define lD1 retry_positionalparams_2
#define lC1 i02 index
#define lB1 463 tS 273436,
#define lA1 l7 2,2,473304 tS
#define l91 {l4::MakeNotP1,l4::
#define l81 ;return
#define l71 yO3 l81 Unknown;}
#define l61 PlanNtimesCache(
#define l51 AddFunctionOpcode_Float(
#define l41 FPoptimizer_Grammar
#define l31 IP,size_t limit,size_t y1
#define l21 AddOperation(cInv,1,1)nS}
#define l11 e92 ImmedHashGenerator
#define l01 GetPositivityInfo nO3
#define iZ tB2 eS1(
#define iY CopyOnWrite();
#define iX recursioncount
#define iW ParamSpec_SubFunctionData
#define iV inverse_denominator]
#define iU NewHash
#define iT tree.GetParamCount()
#define iS PositionalParams_Rec
#define iR 0x4},{{
#define iQ needs_cow=GetRefCount()>1;
#define iP );nC1 0,cE2);iH1 1);
#define iO DumpTreeWithIndent(*this);
#define iN switch(type iY2 cond_or:
#define iM CalculateResultBoundaries(
#define iL e62 v,eS1(lN
#define iK AddFunctionOpcode_Integer(
#define iJ sim.Eat(
#define iI edited_powgroup
#define iH has_unknown_max
#define iG has_unknown_min
#define iF if(keep_powi
#define iE synthed_tree
#define iD 356668 tS 24852
#define iC +2]=yM|i02(Immed yA3);
#define iB matched_params
#define iA .IsIdenticalTo(
#define i9 by_exponent
#define i8 collections
#define i7 cache
#define i6 }inline
#define i5 lT void range x8::
#define i4 yA comp.cB1[a].value);
#define i3 AnyParams,2},0,0x0},{{
#define i2 cS1.data
#define i1 iF2 nO2(
#define i0 .has_max
#define tZ goto ReplaceTreeWithZero;
#define tY :goto ReplaceTreeWithOne;case
#define tX GetLogicalValue nW1
#define tW lN1 std::endl;DumpHashes(
#define tV ;p2.yB3 p2);tH iH2 nE);e5}
#define tU .lI2
#define tT );tree
#define tS ,{2,
#define tR lZ 0x0},{{
#define tQ yV1 nA
#define tP MakeFalse,l4::
#define tO ].relationship
#define tN <=fp_const_negativezero x8())
#define tM .hash1|=key;iC1 n9
#define tL [n2 eC3=true;lQ[n2 eD3
#define tK l41::Grammar*
#define tJ powgroup lD
#define tI ;pow tU cLog);tH cMul);
#define tH tree tU
#define tG eK2&&found[data.
#define tF },{l4::MakeNotNotP1,l4::
#define tE },{l4::MakeNotNotP0,l4::
#define tD cN3 a-->0;)if(
#define tC nB1(
#define tB has_mulgroups_remaining
#define tA StackTop
#define t9 MatchInfo x8&
#define t8 int_exponent_t
#define t7 RootPowerTable x8::RootPowers[
#define t6 MatchPositionSpec_AnyParams x8
#define t5 iO2 FPoptimizer_ByteCode
#define t4 is_signed
#define t3 iE1));xX1 Rehash();
#define t2 result_positivity
#define t1 biggest_minimum
#define t0 const iW
#define eZ ParamSpec_NumConstant x8
#define eY yK2&tree,std::ostream&o
#define eX !=Unchanged)if(TestCase(
#define eW cond_tree
#define eV else_tree
#define eU then_tree
#define eT .AddParam(
#define eS ;xE3(tree)lN1"\n";
#define eR yK2&tree)
#define eQ sequencing
#define eP StackState
#define eO string FP_GetOpcodeName(
#define eN if_stack
#define eM {return yK2(
#define eL n_as_sin_param
#define eK n_as_cos_param
#define eJ PowiResolver::
#define eI ];};extern"C"{
#define eH .BalanceGood
#define eG AddParamMove(
#define eF valueType
#define eE best_factor
#define eD back().endif_location
#define eC iC1(iJ1)
#define eB iC1 key
#define eA l8 2,1,
#define e9 130,1,
#define e8 MatchPositionSpecBase
#define e7 iF2 yL2(
#define e6 smallest_maximum
#define e5 goto redo;
#define e4 ++IP;y81}if(xW3 xV3.
#define e3 ReplaceTreeWithParam0;
#define e2 factor_needs_rehashing
#define e1 MatchPositionSpecBaseP
#define e0 nL eS1(-y03;
#define cZ 79,112,113,117,118,123,124,127,128,
#define cY )l81 m cM3
#define cX e01 x8(nM.param_list,
#define cW relationships
#define cV 28,29,30,31,32,33,34,35,36,
#define cU cIf,l0 3,
#define cT lK 2},0,0x0},{{
#define cS data.subfunc_opcode
#define cR }if eD2.xJ1==eS1(
#define cQ otherhalf
#define cP :{AdoptChildrenWithSameOpcode(tree);
#define cO map<fphash_t,std::set<std::string> >
#define cN const SequenceOpCode x8
#define cM =fp_cosh(m.min);m.max=fp_cosh(m.max);
#define cL MatchPositionSpec_PositionalParams x8
#define cK eS1(1.5)*fp_const_pi x8()
#define cJ !=Unchanged nZ2 iZ1
#define cI FoundChild
#define cH CalculatePowiFactorCost(
#define cG T1,typename T2>inline i12()(
#define cF yK2 tmp;tmp tU
#define cE has_nonlogical_values
#define cD from_logical_context)
#define cC for xK2 xW.cN3 a-->0;)
#define cB POWI_CACHE_SIZE
#define cA static inline yK2
#define c9 BalanceResultType
#define c8 cIf,eL3
#define c7 nA3(0),Opcode(
#define c6 const{return data->
#define c5 +=fp_const_twopi x8();
#define c4 .AddOperation(lM2,
#define c3 for xK2 0;a<cN3++a){if(
#define c2 static void MakeHash(nD fphash_t&iU,
#define c1 l2 2,
#define c0 MatchPositionSpec_AnyWhere
#define yZ if iZ2 data.match_type==
#define yY }PACKED_GRAMMAR_ATTRIBUTE;
#define yX ,cGreaterOrEq l7 2,2,
#define yW void OutFloatHex(std::ostream&o,
#define yV yV1 x72 range x8::
#define yU b;}};n73<>e92 Comp<
#define yT eT CodeTreeImmed(
#define yS xK2 0;a<iT;++a)
#define yR t93 115824 tS 122999,
#define yQ t93 129136 tS 128123,
#define yP t93 472176 tS 24699,
#define yO ,typename yK2::
#define yN AssembleSequence_Subdivide(
#define yM 0x80000000u
#define yL .push_back(
#define yK !=eK2){lJ1
#define yJ ;yK2
#define yI nZ2 false;
#define yH paramholder_matches
#define yG std::vector
#define yF ,AnyParams,0 l1 0,1,
#define yE ;iJ 2,
#define yD for(lE3 r=range.first;r!=range eE3;++r){
#define yC eT tree lD
#define yB ComparisonSetBase::
#define yA .eG
#define y9 branch2
#define y8 fp_const_twopi x8());if(
#define y7 n73 set_min_max_if<cGreater>(eS1(0),
#define y6 iM tree lD
#define y5 =y6 0));range x8
#define y4 tree lD 1).xJ1
#define y3 x02;tH
#define y2 lX&&tree lD 1)y21
#define y1 factor_stack_base
#define y0 cPow,l6 2,
#define xZ GroupFunction,0},n0{{
#define xY data->c93
#define xX =0;a<nU3.cN3++a)if(
#define xW branch1
#define xV {lK2.erase(i);y81}
#define xU xK2 iT;a-->0;)
#define xT nM xY2
#define xS i02 c;i02 char l[
#define xR 158,167,168,169,178,179,191,195,203,207,215,227,229,232,233,234,235,236,239,240,241,242,245,246,247,248,250,251}};}e92
#define xQ using iO2 FUNCTIONPARSERTYPES;
#define xP const eY=std::cout
#define xO IsIdenticalTo(leaf2 lD
#define xN FPOPT_autoptr
#define xM +=result l81 result;}yV1 inline eS1
#define xL int_exponent
#define xK newnode
#define xJ l3 2,1,
#define xI ParamSpec_SubFunction
#define xH ParamSpec_ParamHolder
#define xG has_highlevel_opcodes
#define xF {if(needs_cow){iY goto
#define xE ;if(fp_nequal(tmp,eS1(0)))lL eS1(1)/tmp)n5}lC
#define xD },{l4::Unchanged,l4::Never},{l4::Unchanged,l4::Never}}
#define xC best_selected_sep
#define xB ->Recalculate_Hash_NoRecursion();}
#define xA fp_sin(min),fp_sin(max))
#define x9 sim.AddConst(
#define x8 <eS1>
#define x7 :nG1 tmp,tmp2;tmp2 tU
#define x6 position
#define x5 GetStackTop()-
#define x4 for yS{range x8
#define x3 SwapLastTwoInStack();
#define x2 FPoptimizer_CodeTree::yK2&tree
#define x1 SetParam(0,iH2 lD 0))yJ p1;p1 tU
#define x0 TestImmedConstraints(param t42,tree)yI
#define nZ {tree.FixIncompleteHashes();}
#define nY {yE3 cInv nW2 x9-1 e21
#define nX paramholder_index
#define nW return true;case
#define nV occurance_counts
#define nU PositionalParams,0},0,
#define nT xQ xD1 tA-1);
#define nS ;synth.yZ2*this)l81;}
#define nR ,l0 1,
#define nQ );a-->0;){lM1&powgroup=lT1;if(powgroup
#define nP ;a<iT;++a)if(ApplyGrammar(tN2,xI2,
#define nO lK 1},0,0x0},{{
#define nN ByteCode
#define nM model_tree
#define nL return range x8(
#define nK yG nV1&l02
#define nJ ConstantFolding_LogicCommon(tree,yB
#define nI eS1>p xO3 p.
#define nH nT1 Ref>inline void xN<Ref>::
#define nG ;tmp2 yC 0 y43 cInv);tmp yY3 l81
#define nF ):data(new nO2 x8(
#define nE .lM2
#define nD FUNCTIONPARSERTYPES::
#define nC iJ1(),c93(),Hash(),Depth(1),eZ1 0){}
#define nB SynthesizeByteCode(synth);
#define nA nO2 x8::nO2(
#define n9 crc=(key>>10)|(key<<(64-10))eC2((~iC1(crc))*3)^1234567;}};
#define n8 GetIntegerInfo nW1 0))==IsAlways)lN3
#define n7 eS1(*const func)(eS1),range x8 model){
#define n6 e72.xJ1
#define n5 ;goto do_return;}
#define n4 while(ApplyGrammar(i01 Grammar&)
#define n3 DumpParams x8 iZ2 data.param_list,xF3 data xY2,o);
#define n2 restholder_index
#define n1 yK2 cE2;cE2 e53 cE2 eT
#define n0 yL1,0x0},
#define lZ t51 1},0,
#define lY ;tree yA n41)nS2
#define lX l23 y21
#define lW eG pow x03;pow.iH1 1);pow.Rehash(tT.nC1 0,pow);goto NowWeAreMulGroup;}
#define lV {cF cPow);y53;tmp yT eS1(
#define lU :if(ParamComparer x8()(c93[1],c93[0])){std::swap(c93[0],c93[1]);Opcode=
#define lT <typename eS1>
#define lS ,eS1(1)/eS1(
#define lR SetParam(0,e72 lD 0)xK1 1,CodeTreeImmed(
#define lQ restholder_matches
#define lP SelectedParams,0},0,0x0},{{
#define lO n41;n41 xQ2 n41 yA e72);n41 eT xW lD
#define lN *const func)(eS1),range x8 model=range x8());
#define lM yV1 yK2::yL2(
#define lL {tree.ReplaceWithImmed(
#define lK cMul,AnyParams,
#define lJ cMul,SelectedParams,0},0,
#define lI cPow,l0 2
#define lH iM tmp)cM3
#define lG :cC3=comp.AddRelationship(atree lD 0),atree lD 1),yB
#define lF typename eS1>i12()i01 eS1&a,e62 b){return a
#define lE t51 0 l1
#define lD .GetParam(
#define lC break;case
#define lB {range x8 m=y6 0));
#define lA xB1 yK2::
#define l9 ?0:1))yJ n41;n41 xQ2 n41 tI1 tree.iE1));n41 y3
#define l8 lK 0 l1
#define l7 ,PositionalParams,0 l1
#define l6 nU iR
#define l5 cAdd,lP 2,
#define l4 RangeComparisonData
#define l3 PositionalParams,0}},{ProduceNewTree,
#define l2 lJ 0x0},{{
#define l1 }},{ReplaceParams,
#define l0 nU 0x0},{{
#ifdef _MSC_VER
typedef
i02
int
iD1;
#else
#include <stdint.h>
typedef
uint_least32_t
iD1;
#endif
iO2
crc32{enum{startvalue=0xFFFFFFFFUL,poly=0xEDB88320UL}
;n73<iD1
crc>e92
b8{enum{b1=(crc
l03
crc
nP2
crc>>1),b2=(b1
l03
b1
nP2
b1>>1),b3=(b2
l03
b2
nP2
b2>>1),b4=(b3
l03
b3
nP2
b3>>1),b5=(b4
l03
b4
nP2
b4>>1),b6=(b5
l03
b5
nP2
b5>>1),b7=(b6
l03
b6
nP2
b6>>1),res=(b7
l03
b7
nP2
b7>>1)}
;}
;inline
iD1
update(iD1
crc,i02
b){
#define B4(n) b8<n>iL2 n+1>iL2 n+2>iL2 n+3>::res
#define R(n) B4(n),B4(n+4),B4(n+8),B4(n+12)
static
const
iD1
table[256]={R(0x00),R(0x10),R(0x20),R(0x30),R(0x40),R(0x50),R(0x60),R(0x70),R(0x80),R(0x90),R(0xA0),R(0xB0),R(0xC0),R(0xD0),R(0xE0),R(0xF0)}
;
#undef R
#undef B4
return((crc>>8))^table[(crc^b)&0xFF];i6
iD1
calc_upd(iD1
c,const
i02
char*buf,size_t
size){iD1
value=c;for(size_t
p=0;p<size;++p)value=update(value,buf[p])l81
value;i6
iD1
calc
i01
i02
char*buf,size_t
size){return
calc_upd(startvalue,buf,size);}
}
#ifndef FPOptimizerAutoPtrHH
#define FPOptimizerAutoPtrHH
nT1
Ref>class
xN{e13
xN():p(0){}
xN(Ref*b):p(b){n93}
xN
i01
xN&b):p(b.p){n93
i6
Ref&e31*(n61*p;i6
Ref*e31->(n61
p;}
xN&e31=(Ref*b){Set(b)l81*this;}
xN&e31=i01
xN&b){Set(b.p)l81*this;}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
xN(xN&&b):p(b.p){b.p=0;}
xN&e31=(xN&&b){if(p!=b.p){cF2;p=b.p;b.p=0;}
return*this;}
#endif
~xN(){cF2
iS2
UnsafeSetP(Ref*newp){p=newp
iS2
swap(xN<Ref>&b){Ref*tmp=p;p=b.p;b.p=tmp;}
private:inline
static
void
Have(Ref*p2);inline
void
cF2;inline
void
n93
inline
void
Set(Ref*p2);private:Ref*p;}
;nH
cF2{if(!p
nZ2;p->nA3-=1;if(!p->nA3)delete
p;}
nH
Have(Ref*p2){if(p2)++(p2->nA3);}
nH
Birth(){Have(p);}
nH
Set(Ref*p2){Have(p2);cF2;p=p2;}
#endif
#include <utility>
e92
Compare2ndRev{nT1
T>inline
i12()i01
T&nB3
T&b
n61
a
eE3>b
eE3;}
}
;e92
Compare1st{nT1
cG
const
nU1
nB3
nU1
b
n61
a.first<b.first;}
nT1
cG
const
nU1
a,T1
b
n61
a.first<b;}
nT1
cG
T1
nB3
nU1
b
n61
a<b.first;}
}
;
#ifndef FPoptimizerHashHH
#define FPoptimizerHashHH
#ifdef _MSC_VER
typedef
i02
long
long
iC1;
#define FPHASH_CONST(x) x##ULL
#else
#include <stdint.h>
typedef
uint_fast64_t
iC1;
#define FPHASH_CONST(x) x##ULL
#endif
iO2
FUNCTIONPARSERTYPES{e92
fphash_t{iC1
hash1,hash2;fphash_t():hash1(0),hash2(0){}
fphash_t
i01
iC1&nB3
iC1&b):hash1(a),hash2(b){}
i12==i01
fphash_t&n51==iM2&&hash2==iN2
i12!=i01
fphash_t&n51!=iM2||hash2!=iN2
i12<i01
fphash_t&n51!=iM2?hash1<iM2:hash2<iN2}
;}
#endif
#ifndef FPOptimizer_CodeTreeHH
#define FPOptimizer_CodeTreeHH
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
iO2
l41{e92
Grammar;}
t5{iP2
ByteCodeSynth;}
iO2
FPoptimizer_CodeTree{iP2
yL2;yT1
nO2;iP2
yL2{typedef
xN<nO2
x8>DataP;DataP
data;e13
yL2();~yL2();e92
OpcodeTag{}
;e7
nR2
o,OpcodeTag);e92
FuncOpcodeTag{}
;e7
nR2
o,i02
f,FuncOpcodeTag);e92
nC3{}
;e7
e62
v,nC3);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
e7
eS1&&v,nC3);
#endif
e92
VarTag{}
;e7
i02
varno,VarTag);e92
CloneTag{}
;e7
iQ2
b,CloneTag);void
GenerateFrom
i01
n81
const
nQ2
const
typename
FunctionParserBase
x8::Data&data,bool
keep_powi=false);void
GenerateFrom
i01
n81
const
nQ2
const
typename
FunctionParserBase
x8::Data&data,const
i22
nK2,bool
keep_powi=false);void
SynthesizeByteCode(n81
nQ2
size_t&stacktop_max);void
SynthesizeByteCode(yW2&synth,bool
MustPopTemps=true)const;size_t
SynthCommonSubExpressions(yV2
lP1
const;void
SetParams
i01
i22
nD3
SetParamsMove(i22
eU1
yL2
GetUniqueRef();
#ifdef __GXX_EXPERIMENTAL_CXX0X__
void
SetParams(yG<yL2>&&eU1
#endif
void
SetParam(size_t
which,iQ2
b);void
nC1
size_t
which,nI2
b);void
AddParam
i01
nI2
param);void
eG
nI2
param);void
AddParams
i01
i22
nD3
AddParamsMove(i22
nD3
AddParamsMove(i22
l02,size_t
l12);void
iH1
size_t
index);void
DelParams();void
Become
i01
nI2
b);inline
size_t
GetParamCount(n61
iE1)yA3;i6
nI2
GetParam(xL3){return
iE1)[n];i6
iQ2
GetParam(xL3
n61
iE1)[n];i6
void
lI2
nR2
o)cX3
Opcode=o;i6
nR2
lM2
c6
Opcode;i6
nD
fphash_t
GetHash()c6
Hash;i6
const
i22
iE1
n61
xY;i6
i22
iE1){return
xY;i6
size_t
xT2
c6
Depth;i6
e62
xJ1
c6
Value;i6
i02
GetVar()c6
iI1
i6
i02
GetFuncNo()c6
iI1
i6
bool
IsDefined(n61
lM2!=nD
cNop;i6
bool
IsImmed(n61
lM2==nD
cImmed;i6
bool
IsVar(n61
lM2==nD
iE2;i6
i02
GetRefCount()c6
nA3
iS2
ReplaceWithImmed
i01
eS1&i);void
Rehash(bool
constantfolding=true);void
Sort();inline
void
Mark_Incompletely_Hashed()cX3
Depth=0;i6
bool
Is_Incompletely_Hashed()c6
Depth==0;i6
const
tK
GetOptimizedUsing()c6
iK1;i6
void
SetOptimizedUsing
i01
tK
g)cX3
iK1=g;}
bool
RecreateInversionsAndNegations(bool
prefer_base2=false);void
FixIncompleteHashes();void
swap(nI2
b){data.swap(b.data);}
bool
IsIdenticalTo
i01
nI2
b)const;void
iY}
;yT1
nO2{int
nA3;nR2
Opcode;eS1
Value;i02
iI1
yG
nV1
c93;nD
fphash_t
Hash;size_t
Depth;const
tK
iK1;nO2();nO2
i01
nO2&b);i1
nR2
o);i1
nR2
o,i02
f);i1
e62
i);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
i1
eS1&&i);nO2(nO2&&b);
#endif
bool
IsIdenticalTo
i01
nO2&b)const;void
Sort();void
Recalculate_Hash_NoRecursion();private:void
e31=i01
nO2&b);}
;yU1
CodeTreeImmed
i01
eS1&i)eM
i
yO
nC3());}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
yU1
CodeTreeImmed(eS1&&i)eM
t72
i)yO
nC3());}
#endif
yU1
CodeTreeOp(nR2
opcode)eM
opcode
yO
OpcodeTag());}
yU1
CodeTreeFuncOp(nR2
opcode,i02
f)eM
opcode,f
yO
FuncOpcodeTag());}
yU1
CodeTreeVar
iL1
varno)eM
varno
yO
VarTag());}
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
xB1
DumpHashes(xP);xB1
xE3(xP);xB1
DumpTreeWithIndent(xP,const
std::string&indent="\\"
);
#endif
}
#endif
#endif
#ifndef FPOPT_NAN_CONST
#include <iostream>
#define FPOPT_NAN_CONST (-1712345.25)
iO2
FPoptimizer_CodeTree{iP2
yL2;}
iO2
l41{enum
ImmedConstraint_Value{ValueMask=0x07,Value_AnyNum=0x0,n12=0x1,Value_OddInt=0x2,t01=0x3,Value_NonInteger=0x4,eE1=0x5
n23
ImmedConstraint_Sign{SignMask=0x18,Sign_AnySign=0x00,nJ1=0x08,n02=0x10,Sign_NoIdea=0x18
n23
ImmedConstraint_Oneness{OnenessMask=0x60,Oneness_Any=0x00,Oneness_One=0x20,Oneness_NotOne=0x40
n23
ImmedConstraint_Constness{ConstnessMask=0x180,Constness_Any=0x00,yL1=0x80,Constness_NotConst=0x100
n23
Modulo_Mode{Modulo_None=0,Modulo_Radians=1
n23
Situation_Flags{LogicalContextOnly=0x01,NotForIntegers=0x02,OnlyForIntegers=0x04
n23
lU2{NumConstant,ParamHolder,SubFunction
n23
ParamMatchingType{PositionalParams,SelectedParams,AnyParams,GroupFunction
n23
RuleType{ProduceNewTree,ReplaceParams}
;
#ifdef __GNUC__
# define PACKED_GRAMMAR_ATTRIBUTE __attribute__((packed))
#else
# define PACKED_GRAMMAR_ATTRIBUTE
#endif
typedef
std::pair<lU2,const
void*>cG2;yV1
cG2
e01
iL1
paramlist,lC1);xG1
ParamSpec_Compare
i01
void*nB3
void*b,lU2
type);i02
ParamSpec_GetDepCode
i01
cG2&b);e92
xH{lC1:8;i02
constraints:9;i02
depcode:15;yY
yT1
ParamSpec_NumConstant{eS1
xZ3;i02
modulo;yY
e92
iW{i02
param_count:2;i02
param_list:30;nR2
subfunc_opcode:8;ParamMatchingType
match_type:3;i02
n2:5;yY
e92
xI{iW
data;i02
constraints:9;i02
depcode:7;yY
e92
Rule{RuleType
ruletype:2;i02
situation_flags:3;i02
repl_param_count:2+11;i02
repl_param_list:30;iW
match_tree;yY
e92
Grammar{i02
rule_count;i02
char
rule_list[999
eI
extern
const
Rule
grammar_rules[];}
xB1
DumpParam
i01
cG2&p,std::ostream&o=std::cout);xB1
DumpParams
iL1
paramlist,i02
count,std::ostream&o=std::cout);}
#endif
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#define CONSTANT_POS_INF HUGE_VAL
#define CONSTANT_NEG_INF (-HUGE_VAL)
iO2
FUNCTIONPARSERTYPES{yV1
inline
eS1
fp_const_pihalf(){return
fp_const_pi
x8()*yF3;}
yV1
inline
eS1
fp_const_twopi(){eS1
cY3
fp_const_pi
x8());e03
fp_const_twoe(){eS1
cY3
fp_const_e
x8());e03
fp_const_twoeinv(){eS1
cY3
fp_const_einv
x8());e03
fp_const_negativezero(){
#ifdef FP_EPSILON
return-fp_epsilon
x8();
#else
return
eS1(-1e-14);
#endif
}
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#include <iostream>
iO2
FPoptimizer_Optimize{using
iO2
l41;using
iO2
FPoptimizer_CodeTree;xQ
iP2
MatchInfo{e13
yG<std::pair<bool,yG
nV1> >lQ;yG
nV1
yH;yG<i02>iB;e13
MatchInfo():lQ(),yH(),iB(){}
e13
bool
SaveOrTestRestHolder
iL1
n2,lL1&tM1){cL1{lQ
nE3
n2+1);lQ
tL=tM1
nS2
if(lQ[n2
eC3==false){lQ
tL=tM1
nS2
lL1&found=lQ[n2
eD3;if(tM1
yA3!=found
yA3
yI
for
xK2
0;a<tM1
eW2
a)if(!tM1[a]iA
found[a])yI
return
true
iS2
SaveRestHolder
iL1
n2,yG
nV1&tM1){cL1
lQ
nE3
n2+1);lQ
tL.swap(tM1);}
bool
SaveOrTestParamHolder
iL1
nX,lM1&nG3){if(yH
yA3<=nX){yH.nH3
nX+1);yH
nE3
nX);yH
yL
nG3)nS2
if(!yH[nX].iA1{yH[nX]=nG3
nS2
return
nG3
iA
yH[nX]iR2
SaveMatchedParamIndex(lC1){iB
yL
index);}
lM1&GetParamHolderValueIfFound
iL1
nX)const{static
lM1
dummytree;if(yH
yA3<=nX
nZ2
dummytree
l81
yH[nX];}
lM1&GetParamHolderValue
iL1
nX
n61
yH[nX];}
bool
HasRestHolder
iL1
n2
n61
lQ
yA3>n2&&lQ[n2
eC3==true;}
lL1&GetRestHolderValues
iL1
n2)const{static
lL1
empty_result;cL1
return
empty_result
l81
lQ[n2
eD3;}
const
yG<i02>&GetMatchedParamIndexes(n61
iB
iS2
swap(t9
b){lQ.swap(b.lQ);yH.swap(b.yH);iB.swap(b.iB);}
t9
e31=i01
t9
b){lQ=b.lQ;yH=b.yH;iB=b.iB
l81*this;}
}
;class
e8;typedef
xN<e8>e1;class
e8{e13
int
nA3;e13
e8():nA3(0){}
virtual~e8(){}
}
;e92
lZ1{bool
found;e1
specs;lZ1(bool
f):found(f),specs(){}
lZ1(bool
f
eA2
s):found(f),specs(s){}
}
;xB1
SynthesizeRule
eX3
yK2&tree,t9
info);yV1
lZ1
TestParam
i01
cG2&xZ2
lM1&tree
eA2
start_at,t9
info);yV1
lZ1
TestParams(t0&nM,lM1&tree
eA2
start_at,t9
info,bool
tA1;xG1
ApplyGrammar
i01
Grammar&tN2,x2,bool
from_logical_context=false);xB1
ApplyGrammars(x2);xG1
IsLogisticallyPlausibleParamsMatch(t0&cB2,const
eR;}
iO2
l41{xB1
DumpMatch
eX3
const
x2,const
FPoptimizer_Optimize::t9
info,bool
DidMatch,std::ostream&o=std::cout);xB1
DumpMatch
eX3
const
x2,const
FPoptimizer_Optimize::t9
info,const
char*eY3,std::ostream&o=std::cout);}
#endif
#include <string>
eG2
l41::lU2
yK1=false);eG2
nR2
yK1=false);
#include <string>
#include <sstream>
#include <assert.h>
#include <iostream>
using
iO2
l41;xQ
eG2
l41::lU2
yK1){
#if 1
const
char*p=0;switch(opcode
iY2
NumConstant:p="NumConstant"
;lC
ParamHolder:p="ParamHolder"
;lC
SubFunction:p="SubFunction"
;nM3
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str()yA3<12)tmp<<' 'l81
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str()yA3<5)tmp<<' 'l81
tmp.str();
#endif
}
eG2
nR2
yK1){
#if 1
const
char*p=0;switch(opcode
iY2
cAbs:p="cAbs"
;lC
cAcos:p="cAcos"
;lC
cAcosh:p="cAcosh"
;lC
cAsin:p="cAsin"
;lC
cAsinh:p="cAsinh"
;lC
cAtan:p="cAtan"
;lC
cAtan2:p="cAtan2"
;lC
cAtanh:p="cAtanh"
;lC
cCbrt:p="cCbrt"
;lC
cCeil:p="cCeil"
;lC
cCos:p="cCos"
;lC
cCosh:p="cCosh"
;lC
cCot:p="cCot"
;lC
cCsc:p="cCsc"
;lC
cEval:p="cEval"
;lC
cExp:p="cExp"
;lC
cExp2:p="cExp2"
;lC
cFloor:p="cFloor"
;lC
cHypot:p="cHypot"
;lC
cIf:p="cIf"
;lC
cInt:p="cInt"
;lC
cLog:p="cLog"
;lC
cLog2:p="cLog2"
;lC
cLog10:p="cLog10"
;lC
cMax:p="cMax"
;lC
cMin:p="cMin"
;lC
cPow:p="cPow"
;lC
cSec:p="cSec"
;lC
cSin:p="cSin"
;lC
cSinh:p="cSinh"
;lC
cSqrt:p="cSqrt"
;lC
cTan:p="cTan"
;lC
cTanh:p="cTanh"
;lC
cTrunc:p="cTrunc"
;lC
cImmed:p="cImmed"
;lC
cJump:p="cJump"
;lC
cNeg:p="cNeg"
;lC
cAdd:p="cAdd"
;lC
cSub:p="cSub"
;lC
cMul:p="cMul"
;lC
cDiv:p="cDiv"
;lC
cMod:p="cMod"
;lC
cEqual:p="cEqual"
;lC
t71:p="cNEqual"
;lC
cLess:p="cLess"
;lC
cLessOrEq:p="cLessOrEq"
;lC
cGreater:p="cGreater"
;lC
cGreaterOrEq:p="cGreaterOrEq"
;lC
cNot:p="cNot"
;lC
cAnd:p="cAnd"
;lC
cOr:p="cOr"
;lC
cDeg:p="cDeg"
;lC
cRad:p="cRad"
;lC
cFCall:p="cFCall"
;lC
cPCall:p="cPCall"
;break;
#ifdef FP_SUPPORT_OPTIMIZER
case
cFetch:p="cFetch"
;lC
cPopNMov:p="cPopNMov"
;lC
cLog2by:p="cLog2by"
;lC
cNop:p="cNop"
;break;
#endif
case
cSinCos:p="cSinCos"
;lC
yR3:p="cAbsNot"
;lC
cAbsNotNot:p="cAbsNotNot"
;lC
cAbsAnd:p="cAbsAnd"
;lC
cAbsOr:p="cAbsOr"
;lC
i03:p="cAbsIf"
;lC
cDup:p="cDup"
;lC
cInv:p="cInv"
;lC
cSqr:p="cSqr"
;lC
cRDiv:p="cRDiv"
;lC
cRSub:p="cRSub"
;lC
cNotNot:p="cNotNot"
;lC
cRSqrt:p="cRSqrt"
;lC
iE2:p="VarBegin"
;nM3
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str()yA3<12)tmp<<' 'l81
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str()yA3<5)tmp<<' 'l81
tmp.str();
#endif
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#ifndef FP_GENERATING_POWI_TABLE
enum{MAX_POWI_BYTECODE_LENGTH=20}
;
#else
enum{MAX_POWI_BYTECODE_LENGTH=999}
;
#endif
enum{MAX_MULI_BYTECODE_LENGTH=3}
;t5{iP2
ByteCodeSynth{e13
ByteCodeSynth():nN(),Immed(),eP(),tA(0),StackMax(0){nN.nH3
64);Immed.nH3
8);eP.nH3
16
iR2
Pull(yG<i02>&bc,yG
x8&imm,size_t&StackTop_max){for
iL1
a=0;a<nN
eW2
a){nN[a]&=~yM;}
nN.swap(bc);Immed.swap(imm);StackTop_max=StackMax;}
size_t
GetByteCodeSize(n61
nN
yA3;}
size_t
GetStackTop(n61
tA
iS2
PushVar
iL1
varno){e23
varno);nV2}
void
PushImmed(eS1
immed){xQ
e23
cImmed);Immed
yL
immed);nV2}
void
StackTopIs
cA3,int
offset=0){if((int)tA>offset){eP[tA
lN2
first=true;eP[tA
lN2
second=tree;}
}
bool
IsStackTop
cA3,int
offset=0
n61(int)tA>offset&&eP[tA
lN2
first&&eP[tA
lN2
second
iA
tree);i6
void
EatNParams
iL1
cH2){tA-=cH2
iS2
ProducedNParams
iL1
e41){xD1
tA+e41
iR2
AddOperation
iL1
opcode,i02
cH2,i02
e41=1){EatNParams(cH2);xQ
AddFunctionOpcode(opcode);ProducedNParams(e41
iR2
DoPopNMov(size_t
cI2,size_t
srcpos){xQ
e23
cPopNMov);e23
yM|iL1)cI2);e23
yM|iL1)srcpos);xD1
srcpos+1);eP[cI2]=eP[srcpos];xD1
cI2+1
iR2
DoDup(size_t
nI3){xQ
if(nI3==tA-1){e23
cDup);}
else{e23
cFetch);e23
yM|iL1)nI3);}
nV2
eP[tA-1]=eP[nI3];}
size_t
e33
cA3)const{for
xK2
tA;a-->0;)if(eP[a
eC3&&eP[a
eD3
iA
tree)nZ2
a
l81
eK2;}
bool
Find
cA3
n61
e33
nO3
eK2;}
bool
FindAndDup
cA3){size_t
pos=e33(tree);if(pos!=eK2){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<tZ3"duplicate at ["
<<pos<<"]: "
;xE3(tree)lN1" -- issuing cDup or cFetch\n"
;
#endif
DoDup(pos)nS2
nX2
e92
IfData{size_t
ofs;}
;void
SynthIfStep1(IfData&yJ2,nR2
op){nT
yJ2.ofs=nN
yA3;e23
op);e23
yM);e23
yM
iR2
SynthIfStep2(IfData&yJ2){nT
nN[yJ2.ofs+1]eJ1+2);nN[yJ2.ofs
iC
yJ2.ofs=nN
yA3;e23
cJump);e23
yM);e23
yM
iR2
SynthIfStep3(IfData&yJ2){nT
nN.back()|=yM;nN[yJ2.ofs+1]eJ1-1);nN[yJ2.ofs
iC
nV2
for
xK2
0;a<yJ2.ofs;++a){if(nN[a]==cJump&&nN[a+1]==(yM|(yJ2.ofs-1))){nN[a+1]eJ1-1);nN[a
iC
yN3(nN[a]iY2
i03:case
cIf:case
cJump:case
cPopNMov:a+=2;lC
cFCall:case
cPCall:case
cFetch:a+=1;break;default:nM3}
}
protected:void
xD1
size_t
value){tA=value;if(tA
eL2{StackMax=tA;yS1);}
}
void
l51
eK1;void
iK
eK1;void
AddFunctionOpcode(eK1;private:yG<i02>nN;yG
x8
Immed;yG<std::pair<bool,FPoptimizer_CodeTree::yK2> >eP;size_t
tA;size_t
StackMax;}
;yT1
SequenceOpCode;yT1
eX1{static
cN
AddSequence;static
cN
MulSequence;}
;xB1
x31
long
count,cN&eQ,lP1;}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
xQ
t5{yT1
SequenceOpCode{eS1
basevalue;i02
op_flip;i02
op_normal,op_normal_flip;i02
op_inverse,op_inverse_flip;}
;yV1
cN
eX1
x8::AddSequence={eS1(0),cNeg,cAdd,cAdd,cSub,cRSub}
;yV1
cN
eX1
x8::MulSequence={eS1(1),cInv,cMul,cMul,cDiv,cRDiv}
;yV1
n71
l51
eK1{int
mStackPtr=0;
#define incStackPtr() do{if(tA+2 eL2 yS1=tA+2);}while(0)
#define findName(a,b,c) "var"
#define TryCompilePowi(o) false
#define mData this
#define mByteCode nN
#define mImmed Immed
# define FP_FLOAT_VERSION 1
# include "fp_opcode_add.inc"
# undef FP_FLOAT_VERSION
#undef mImmed
#undef mByteCode
#undef mData
#undef TryCompilePowi
#undef incStackPtr
}
yV1
n71
iK
eK1{int
mStackPtr=0;
#define incStackPtr() do{if(tA+2 eL2 yS1=tA+2);}while(0)
#define findName(a,b,c) "var"
#define TryCompilePowi(o) false
#define mData this
#define mByteCode nN
#define mImmed Immed
# define FP_FLOAT_VERSION 0
# include "fp_opcode_add.inc"
# undef FP_FLOAT_VERSION
#undef mImmed
#undef mByteCode
#undef mData
#undef TryCompilePowi
#undef incStackPtr
}
yV1
n71
AddFunctionOpcode(eK1{if(IsIntType
x8::result)iK
opcode);else
l51
opcode);}
}
using
t5;
#define POWI_TABLE_SIZE 256
#define POWI_WINDOW_SIZE 3
t5{
#ifndef FP_GENERATING_POWI_TABLE
extern
const
i02
char
powi_table[POWI_TABLE_SIZE];const
#endif
i02
char
powi_table[POWI_TABLE_SIZE]={0,1,1,1,2,1
eM2
1,4,1,2,e43
eM2
1,8,cB3
e73
15,1,16,1
eM2
e43,2,1,4,cB3
1,16,1,25,e73
27,5,8,3,2,1,30,1,31,3,32,1
eM2
1,8,1,2,e73
39,1,16,137,2,1,4,cB3
e43,45,135,4,31,2,5,32,1,2,131,50,1,51,1,8,3,2,1,54,1,55,3,16,1,57,133,4,137,2,135,60,1,61,3,62,133,63,1,tN1
131,tN1
139,iM1
e9
30,1,130,137,2,31,iM1
e9
e9
130,cB3
1,e9
e9
2,1,130,133,tN1
61,130,133,62,139,130,137,e9
iM1
e9
e9
tN1
131,e9
e9
130,131,2,133,iM1
130,141,e9
130,cB3
1,e9
5,135,e9
iM1
e9
iM1
130,133,130,141,130,131,e9
e9
2,131}
;}
static
lX3
cB=256;
#define FPO(x)
iO2{class
PowiCache{private:int
i7[cB];int
tO1[cB];e13
PowiCache():i7(),tO1(){i7[1]=1;}
bool
Plan_Add(yW1,int
count){c51>=cB
yI
tO1[eP2+=count
l81
i7[eP2!=0
iS2
iU2
yW1){c51<cB)i7[eP2=1
iS2
Start(size_t
value1_pos){for(int
n=2;n<cB;++n)i7[n]=-1;Remember(1,value1_pos);DumpContents();}
int
Find(yW1)const{c51<cB){if(i7[eP2>=0){FPO(tK3(tO3,"* I found %ld from cache (%u,%d)\n",value,(unsigned)cache[value],tL3 value]))l81
i7[eP2;}
}
return-1
iS2
Remember(yW1,size_t
tW3){c51>=cB
nZ2;FPO(tK3(tO3,"* Remembering that %ld can be found at %u (%d uses remain)\n",value,(unsigned)tW3,tL3 value]));i7[eP2=(int)tW3
iS2
DumpContents()const{FPO(for(int a=1;a<POWI_CACHE_SIZE;++a)if(cache[a]>=0||tL3 a]>0){tK3(tO3,"== cache: sp=%d, val=%d, needs=%d\n",cache[a],a,tL3 a]);})}
int
UseGetNeeded(yW1){c51>=0&&value<cB
nZ2--tO1[eP2
l81
0;}
}
;yV1
size_t
yN
long
count
tR1
cN&eQ,lP1;xB1
yX1
size_t
apos,long
aval,size_t
bpos,long
bval
tR1
i02
cumulation_opcode,i02
cimulation_opcode_flip,lP1;void
l61
yW1
tR1
int
need_count,int
iX=0){c51<1
nZ2;
#ifdef FP_GENERATING_POWI_TABLE
if(iX>32)throw
false;
#endif
if(i7.Plan_Add(value,need_count)nZ2;long
nJ3
1;c51<POWI_TABLE_SIZE){nJ3
powi_table[eP2
iT2&128){half&=127
iT2&64)nJ3-eQ2
FPO(tK3(tO3,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,value/half));l61
half,yM2
i7.iU2
half)l81;}
tP1
half&64){nJ3-eQ2}
}
else
c51&1)nJ3
value&((1<<POWI_WINDOW_SIZE)-1);else
nJ3
value/2;long
cQ=value-half
iT2>cQ||half<0)std::swap(half,cQ);FPO(tK3(tO3,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,otherhalf))iT2==cQ){l61
half,i7,2,iX+1);}
else{l61
half,yM2
l61
cQ>0?cQ:-cQ,yM2}
i7.iU2
value);}
yV1
size_t
yN
yW1
tR1
cN&eQ,lP1{int
nK3=i7.Find(value);if(nK3>=0){return
nK3;}
long
nJ3
1;c51<POWI_TABLE_SIZE){nJ3
powi_table[eP2
iT2&128){half&=127
iT2&64)nJ3-eQ2
FPO(tK3(tO3,"* I want %ld, my plan is %ld * %ld\n",value,half,value/half));size_t
x52=yN
half
c61
if(i7
iN1
half)>0||x52!=c71){tQ1
x52)nU2
half,c71);}
x31
value/half
eR2
size_t
tW3=c71
nU2
value,tW3);i7.DumpContents()l81
tW3;}
tP1
half&64){nJ3-eQ2}
}
else
c51&1)nJ3
value&((1<<POWI_WINDOW_SIZE)-1);else
nJ3
value/2;long
cQ=value-half
iT2>cQ||half<0)std::swap(half,cQ);FPO(tK3(tO3,"* I want %ld, my plan is %ld + %ld\n",value,half,value-half))iT2==cQ){size_t
x52=yN
half
c61
yX1
x52,half,x52,half,i7,eQ.op_normal,eQ.op_normal_flip,synth);}
else{long
part1=half;long
part2=cQ>0?cQ:-cQ;size_t
part1_pos=yN
part1
c61
size_t
part2_pos=yN
part2
c61
FPO(tK3(tO3,"Subdivide(%ld: %ld, %ld)\n",value,half,otherhalf));yX1
part1_pos,part1,part2_pos,part2,i7,cQ>0?eQ.op_normal:eQ.op_inverse,cQ>0?eQ.op_normal_flip:eQ.op_inverse_flip,synth);}
size_t
tW3=c71
nU2
value,tW3);i7.DumpContents()l81
tW3;}
xB1
yX1
size_t
apos,long
aval,size_t
bpos,long
bval
tR1
i02
cumulation_opcode,i02
cumulation_opcode_flip,lP1{int
a_needed=i7
iN1
aval);int
nL3=i7
iN1
bval);bool
iO1
i13
#define DUP_BOTH() do tX3<bpos){size_t tmp=apos;apos=bpos;bpos=tmp;iO1=!iO1;}FPO(tK3(tO3,"-> "tV3 tV3"op\n",(unsigned)apos,(unsigned)bpos));tQ1 apos);tQ1 apos==bpos?c71:bpos);}while(0)
#define DUP_ONE(p) do{FPO(tK3(tO3,"-> "tV3"op\n",(unsigned)p));tQ1 p);}while(0)
if(a_needed>0){if(nL3>0){lO2}
else{if(bpos!=c71)lO2
else{iP1
iO1=!iO1;}
}
}
tP1
nL3>0)tX3!=c71)lO2
else
DUP_ONE(bpos);}
else
tX3==bpos&&apos==c71)iP1
tP1
apos==c71&&bpos==synth.x5
2){FPO(tK3(tO3,"-> op\n"));iO1=!iO1;}
tP1
apos==synth.x5
2&&bpos==c71)FPO(tK3(tO3,"-> op\n"));tP1
apos==c71)DUP_ONE(bpos);tP1
bpos==c71){iP1
iO1=!iO1;}
else
lO2}
lK1
iO1?cumulation_opcode_flip:cumulation_opcode,2);}
xB1
cA1
long
count,cN&eQ,lP1{while
cR3<256){int
nJ3
yV2
powi_table[count]iT2&128){half&=127;cA1
half
eR2
count/=half;}
else
nM3
if
cR3==1
nZ2;if(!cR3&1)){lK1
cSqr,1);cA1
count/2
eR2}
else{tQ1
c71);cA1
count-1
eR2
lK1
cMul,2);}
}
}
t5{xB1
x31
long
count,cN&eQ,lP1{if
cR3==0)yX2
eQ.basevalue
t73
bool
eT2
i13
if
cR3<0){eT2=true;count=-count;}
if(false)cA1
count
eR2
tP1
count>1){PowiCache
i7;l61
count,i7,1);size_t
xE1
t13
GetStackTop();i7.Start(c71);FPO(tK3(tO3,"Calculating result for %ld...\n",count));size_t
x62=yN
count
c61
size_t
n_excess
t13
x5
xE1;if(n_excess>0||x62!=xE1-1){synth.DoPopNMov(xE1-1,x62);}
}
if(eT2)lK1
eQ.op_flip,1);}
}
}
#endif
#ifndef FPOptimizer_RangeEstimationHH
#define FPOptimizer_RangeEstimationHH
iO2
FPoptimizer_CodeTree{enum
TriTruthValue{IsAlways,yO3,Unknown}
;yT1
range{eS1
min,max;bool
iV2,has_max;range():min(),max(),iV2(false
cS3
false)x43
eS1
mi,eS1
ma):min(mi),max(ma),iV2(true
cS3
true)x43
bool,eS1
ma):min(),max(ma),iV2(false
cS3
true)x43
eS1
mi,bool):min(mi),max(),iV2(true
cS3
false){}
void
set_abs();void
set_neg();x72
set_min_max_if(iL
x72
set_min_if(iL
x72
set_max_if(iL
void
set_min(eS1(lN
void
set_max(eS1(lN
void
y82
eS1(lN}
;yV1
inline
bool
IsLogicalTrueValue
i01
range
x8&p
cD2
if(p
y41
p.min>=0.5
nZ2
true;if(!abs&&p
iW2<=-0.5
nZ2
true
l81
false;}
yV1
inline
bool
IsLogicalFalseValue
i01
range
x8&p
cD2
if(abs
nZ2
p
iW2<0.5;else
return
p
y41
p
i0&&p.min>-0.5
xQ3<0.5;}
yV1
range
x8
iM
const
eR;xG1
IsLogicalValue
i01
eR;yV1
TriTruthValue
GetIntegerInfo
i01
eR;y51
GetEvennessInfo
i01
eR{if(!tree
y21
nZ2
Unknown;yF1=yZ3;if(isEvenInteger(value)nZ2
x82
isOddInteger(value)nZ2
l71
y51
GetPositivityInfo
i01
eR{range
x8
p=iM
tree);if(p
y41
p.min
cH1
nZ2
x82
p
iW2
t83
nZ2
l71
y51
GetLogicalValue(lM1&tree
cD2
range
x8
p=iM
tree);if(IsLogicalTrueValue(p,abs)nZ2
x82
IsLogicalFalseValue(p,abs)nZ2
l71}
#endif
#ifndef FPOptimizer_ConstantFoldingHH
#define FPOptimizer_ConstantFoldingHH
iO2
FPoptimizer_CodeTree{xB1
ConstantFolding(eR;}
#endif
iO2{xQ
using
iO2
FPoptimizer_CodeTree;e92
ComparisonSetBase{enum{e93=0x1,Eq_Mask=0x2,Le_Mask=0x3,eA3=0x4,eB3=0x5,Ge_Mask=0x6}
;static
int
Swap_Mask(int
m){return(m&Eq_Mask)|((m&e93)?eA3:0)|((m&eA3)?e93:0);}
enum
yY1{Ok,BecomeZero,BecomeOne,xA2
n23
lV2{cond_or,i32,i42,i52}
;}
;yT1
ComparisonSet:public
ComparisonSetBase{e92
eU2
nG1
a
yJ
b;int
relationship;eU2():a(),b(),relationship(){}
}
;yG<eU2>cW;e92
Item
nG1
value;bool
yN2;Item():value(),yN2(false){}
}
;yG<Item>cB1;int
xF1;ComparisonSet():cW(),cB1(),xF1(0){}
yY1
AddItem(lM1&a,bool
yN2,lV2
type){for(size_t
c=0;c<cB1
eW2
c)if(cB1[c].value
iA
a)){if(yN2!=cB1[c].yN2){iN
cM1
case
i52:cB1.erase(cB1.begin()+c);xF1+=1
n01
case
i32:case
i42:cN1}
}
return
xA2;}
Item
pole;pole.value=a;pole.yN2=yN2;cB1
yL
pole)l81
Ok;}
yY1
AddRelationship(yK2
a,yK2
b,int
eY1,lV2
type){iN
cT3
7)cM1
lC
i52:cT3
7){xF1+=1
n01}
lC
i32:case
i42:cT3
0)cN1
nM3
if(!(a.GetHash()<b.GetHash())){a.swap(b);eY1=Swap_Mask(eY1);}
for(size_t
c=0;c<cW
eW2
c){if(cW[c].a
iA
a)&&cW[c].b
iA
b)){iN{int
nS3=x92|eY1;if(nS3==7)cM1
x92=nS3;nM3
case
i32:case
i42:{int
nS3=x92&eY1;if(nS3==0)cN1
x92=nS3;nM3
case
i52:{int
newrel_or=x92|eY1;int
xB2=x92&eY1;iQ1
5&&xB2==0){x92=eB3
n01}
iQ1
7&&xB2==0){xF1+=1;cW.erase(cW.begin()+c)n01}
iQ1
7&&xB2==Eq_Mask){x92=Eq_Mask;xF1+=1
n01}
y81}
}
return
xA2;}
}
eU2
comp;comp.a=a;comp.b=b;comp.relationship=eY1;cW
yL
comp)l81
Ok;}
}
;nT1
eS1,typename
CondType>bool
ConstantFolding_LogicCommon(yK2&tree,CondType
xA1,bool
xC2){bool
should_regenerate
i13
ComparisonSet
x8
comp
eV3
typename
yB
yY1
cC3=yB
Ok;lM1&atree=xI2;switch(atree
nE
iY2
cEqual
lG
Eq_Mask
eB2
t71
lG
eB3
eB2
cLess
lG
e93
eB2
cLessOrEq
lG
Le_Mask
eB2
cGreater
lG
eA3
eB2
cGreaterOrEq
lG
Ge_Mask
eB2
cNot:cC3
y11
lD
0),true
eB2
cNotNot:cC3
y11
lD
0),false,xA1);break;default:if(xC2||IsLogicalValue(atree))cC3
y11,false,xA1);yN3(cC3){ReplaceTreeWithZero
xP3
ReplaceWithImmed(0)l81
true;ReplaceTreeWithOne
xP3
ReplaceWithImmed(1);nW
yB
Ok:lC
yB
BecomeZero:yP3
yB
BecomeOne
tY
yB
xA2:c11
nM3}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_LogicCommon: "
eS
#endif
if(xC2){tree.DelParams();}
else{for
xU{lM1&atree=xI2;if(IsLogicalValue(atree))nF1);}
}
for
xK2
0;a<comp.cB1
eW2
a){if(comp.cB1[a].yN2)eV2
cNot);r
i4
r.cO1
tP1!xC2)eV2
cNotNot);r
i4
r.cO1
else
tree
i4}
for
xK2
0;a<comp.cW
eW2
a)eV2
cNop);switch(comp.cW[a
tO
iY2
yB
e93:r
tU
cLess);lC
yB
Eq_Mask:r
tU
cEqual);lC
yB
eA3:r
tU
cGreater);lC
yB
Le_Mask:r
tU
cLessOrEq);lC
yB
eB3:r
tU
t71);lC
yB
Ge_Mask:r
tU
cGreaterOrEq
nW2
r
yA
comp.cW[a].a);r
yA
comp.cW[a].b);r.cO1
if(comp.xF1!=0)tree
yT
eS1(comp.xF1)));
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_LogicCommon: "
eS
#endif
return
true;}
nX2
xG1
ConstantFolding_AndLogic(tT3(tree.tU3()==cAnd
tI3()==cAbsAnd)l81
nJ
i32,true);}
xG1
ConstantFolding_OrLogic(tT3(tree.tU3()==cOr
tI3()==cAbsOr)l81
nJ
cond_or,true);}
xG1
ConstantFolding_AddLogicItems(tT3(tree.tU3()==cAdd)l81
nJ
i52,false);}
xG1
ConstantFolding_MulLogicItems(tT3(tree.tU3()==cMul)l81
nJ
i42,false);}
}
#include <vector>
#include <map>
#include <algorithm>
iO2{xQ
using
iO2
FPoptimizer_CodeTree;e92
CollectionSetBase{enum
xH1{Ok,xA2}
;}
;yT1
CollectionSet:public
CollectionSetBase{e92
yZ1
nG1
value
yJ
xD2;bool
e2;yZ1():value(),xD2(),e2(false){}
yZ1(lM1&v,lM1&f):value(v),xD2(f),e2(false){}
}
;std::multimap<fphash_t,yZ1>i8;typedef
typename
std::multimap<fphash_t,yZ1>::nX3
xI1;CollectionSet():i8(){}
xI1
FindIdenticalValueTo(lM1&value){fphash_t
hash=value.GetHash();for(xI1
i=i8.xE2
hash);i!=i8.cP1
hash;++i){c51
iA
i
cJ2.value)nZ2
i;}
return
i8.end();}
bool
Found
i01
xI1&b){return
b!=i8.end();}
xH1
AddCollectionTo(lM1&xD2,const
xI1&into_which){yZ1&c=into_which
cJ2;if(c.e2)c.xD2
eT
xD2);else
nG1
add;add
tU
cAdd);add
yA
c.xD2);add
eT
xD2);c.xD2.swap(add);c.e2=true;}
return
xA2;}
xH1
iR1
lM1&value,lM1&xD2){const
fphash_t
hash=value.GetHash();xI1
i=i8.xE2
hash);for(;i!=i8.cP1
hash;++i){if(i
cJ2.value
iA
value)nZ2
AddCollectionTo(xD2,i);}
i8.nT3,std::make_pair(hash,yZ1(value,xD2)))l81
Ok;}
xH1
iR1
lM1&a){return
iR1
a,nB1
1)));}
}
;yT1
ConstantExponentCollection{typedef
yG
nV1
nV3;typedef
std::x51
xF2;yG<xF2>data;ConstantExponentCollection():data(){}
void
MoveToSet_Unique
i01
eS1&e51&e61){data
yL
std::x51(e51()));data.back()eE3.swap(e61
iR2
MoveToSet_NonUnique
i01
eS1&e51&e61){typename
yG<xF2>::nX3
i=std::xE2
data.i62
data.end(),cE2,Compare1st());if(i!=data.cP1
cE2){i
cJ2.nT3
cJ2.end(),e61.i62
e61.end());}
else{data.nT3,std::x51(cE2,e61));}
}
bool
Optimize(){bool
changed
i13
std::sort(data.i62
data.end(),Compare1st());redo:for
xK2
0;a<data
eW2
a){eS1
exp_a=data[a
eC3
i82
exp_a,nY3
y81
for(iX2
a+1;b<data
eW2
b){eS1
exp_b=data[b
eC3;eS1
xG2=exp_b-exp_a;if(xG2>=fp_abs(exp_a
n33
exp_diff_still_probable_integer=xG2*eS1(16);if(eY2
exp_diff_still_probable_integer)&&!(eY2
exp_b)&&!eY2
xG2))){nV3&a_set=iS1;nV3&b_set=data[b
eD3;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantExponentCollection iteration:\n"
;eZ2
cout);
#endif
if(isEvenInteger(exp_b)&&!isEvenInteger(xG2+exp_a))nG1
tmp2;tmp2
e53
tmp2
tI1
b_set);tmp2
i72
tmp;tmp
tU
cAbs);tmp
yY3;tmp
x02;b_set
nE3
1);b_set[0
t23
tmp);}
a_set.insert(a_set.end(),b_set.i62
b_set.end());nV3
b_copy=b_set;data.erase(data.begin()+b);MoveToSet_NonUnique(xG2,b_copy);yI1
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantExponentCollection iteration:\n"
;eZ2
cout);
#endif
e5}
}
}
return
changed;}
#ifdef DEBUG_SUBSTITUTIONS
void
eZ2
ostream&out){for
xK2
0;a<data
eW2
a){out.precision(12);out<<data[a
eC3<<": "
;eW3
iS1
eW2
b){if(b>0)out<<'*';xE3(iS1[b],out);}
out<<std::endl;}
}
#endif
}
;yV1
static
yK2
x61
yK2&value,bool&xG){switch(value
nE
iY2
cPow:nG1
cE2=value
lD
1);value.y71
l81
cE2
cM3
cRSqrt:value.y71;xG=true
l81
nB1-0.5));case
cInv:value.y71;xG=true
l81
nB1-1));default:nM3
return
nB1
1))eI2
void
e71
e81&mul,lM1&tree,lM1&xD2,bool&c01
bool&xG){for
yS
nG1
value
nW1
a))yJ
cE2(x61
value,xG));if(!xD2
y21||xD2.xJ1!=1.0)nG1
cQ1;cQ1
e53
cQ1
eT
cE2);cQ1
eT
xD2);cQ1
x02;cE2.swap(cQ1);}
#if 0 /* FIXME: This does not work */
c51
nE==cMul){if(1){bool
exponent_is_even=cE2
y21&&isEvenInteger(cE2.xJ1);eW3
value.c81{bool
tmp=false
yJ
val(value
lD
b))yJ
exp(x61
val,tmp));if(exponent_is_even||(exp
y21&&isEvenInteger(exp.xJ1)))nG1
cQ1;cQ1
e53
cQ1
eT
cE2);cQ1
yA
exp);cQ1.ConstantFolding();if(!cQ1
y21||!isEvenInteger(cQ1.xJ1)){goto
cannot_adopt_mul;}
}
}
}
e71
mul,value,cE2,c01
xG);}
else
cannot_adopt_mul:
#endif
{if(mul.iR1
value,cE2)==CollectionSetBase::xA2)c11}
}
}
xG1
ConstantFolding_MulGrouping(eR{bool
xG
i13
bool
should_regenerate
i13
e81
mul;e71
mul,tree,nB1
1)),c01
xG);typedef
std::pair<yK2,yG
nV1>e91;typedef
std::multimap<fphash_t,e91>cC1;cC1
i9;xH2
e81::xI1
j=mul.i8.nW3
j!=mul.i8.end();++j)nG1&value=j
cJ2.value
yJ&cE2=j
cJ2.xD2;if(j
cJ2.e2)cE2
x02;const
fphash_t
eA1=cE2.GetHash();typename
cC1::nX3
i=i9.xE2
eA1);for(;i!=i9.cP1
eA1;++i)if(i
cJ2.first
iA
cE2)){if(!cE2
y21||!cR1.xJ1,nY3
c11
i
cJ2
eE3
yL
value);goto
skip_b;}
i9.nT3,std::make_pair(eA1,std::make_pair(cE2,yG
nV1(size_t(1),value))));skip_b:;}
#ifdef FP_MUL_COMBINE_EXPONENTS
ConstantExponentCollection
x8
cS1;xH2
cC1::nX3
j,i=i9.nW3
i!=i9.end();i=j){j=i;++j;e91&list=i
cJ2;if
eD2
y21)c21
list.first.xJ1;if(!(cE2==eS1(0)))cS1.MoveToSet_Unique(cE2,list
eE3);i9.erase(i);}
}
if(cS1.Optimize())c11
#endif
if(should_regenerate)nG1
before=tree;before.iY
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_MulGrouping: "
;xE3(before)lN1"\n"
;
#endif
tree.DelParams();xH2
cC1::nX3
i=i9.nW3
i!=i9.end();++i){e91&list=i
cJ2;
#ifndef FP_MUL_COMBINE_EXPONENTS
if
eD2
y21)c21
list.first.xJ1;if(cE2==eS1(0))y81
if(cR1,nY3{tree.AddParamsMove(list
eE3);y81}
}
#endif
yK2
mul;mul
e53
mul
tI1
list
eE3);mul
x02;if(xG&&list.first
tF3
eD2.xJ1==eS1(1)/eS1(3))nG1
cbrt;cbrt
tU
cCbrt);cbrt
t02;cbrt.yB3
cbrt);y81
cR
0.5))nG1
sqrt;sqrt
tU
cSqrt);sqrt
t02;sqrt.yB3
sqrt);y81
cR-0.5))nG1
rsqrt;rsqrt
tU
cRSqrt);rsqrt
t02;rsqrt.yB3
rsqrt);y81
cR-1))nG1
inv;inv
tU
cInv);inv
t02;inv.yB3
inv);y81}
}
yK2
pow
yB2
t02;pow
yA
list.first);pow.yB3
pow);}
#ifdef FP_MUL_COMBINE_EXPONENTS
i9.clear()c23
0;a<i2
eW2
a)c21
i2[a
eC3;if(cR1,nY3{tree.AddParamsMove(i2[a
eD3);y81}
yK2
mul;mul
e53
mul
tI1
i2[a
eD3);mul
i72
pow
yB2
t02;pow
yT
cE2));pow.yB3
pow);}
#endif
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_MulGrouping: "
eS
#endif
return!tree
iA
before);}
nX2
xG1
ConstantFolding_AddGrouping(eR{bool
should_regenerate
i13
e81
add
eV3
if
nW1
a)nE==cMul)y81
if(add.AddCollection
nW1
a))==CollectionSetBase::xA2)c11}
yG<bool>remaining(iT);size_t
tB=0
eV3
lM1&n83=xI2;if(n83
nE==cMul){eW3
xX1
c81{if(n83
lD
b)y21)y81
typename
e81::xI1
c=add.FindIdenticalValueTo(n83
lD
b));if(add.Found(c))nG1
tmp(n83
yO
CloneTag());tmp.iH1
b);tmp
x02;add.AddCollectionTo(tmp,c);c11
goto
done_a;}
}
remaining[a]=true;tB+=1;done_a:;}
}
if(tB>0){if(tB>1){yG<std::pair<yK2,size_t> >nV;std::multimap<fphash_t,size_t>eB1;bool
l13
i13
t12{eW3
xI2.c81{lM1&p=xI2
lD
b);const
fphash_t
p_hash=p.GetHash();for(std::multimap<fphash_t,size_t>::const_iterator
i=eB1.xE2
p_hash);i!=eB1.cP1
p_hash;++i){if(nV[i
cJ2
eC3
iA
p)){nV[i
cJ2
eD3+=1;l13=true;goto
found_mulgroup_item_dup;}
}
nV
yL
std::make_pair(p,size_t(1)));eB1.insert(std::make_pair(p_hash,nV
yA3-1));found_mulgroup_item_dup:;}
}
if(l13)nG1
cK2;{size_t
max=0;for(size_t
p=0;p<nV
eW2
p)if(nV[p
eD3<=1)nV[p
eD3=0;else{nV[p
eD3*=nV[p
eC3.xT2;if(nV[p
eD3>max){cK2=nV[p
eC3;max=nV[p
eD3;}
}
}
yK2
group_add;group_add
tU
cAdd);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Duplicate across some trees: "
;xE3(cK2)lN1" in "
eS
#endif
t12
eW3
xI2.c81
if(cK2
iA
xI2
lD
b)))nG1
tmp
nW1
a)yO
CloneTag());tmp.iH1
b);tmp
x02;group_add
yA
tmp);remaining[a]i13
nM3
group_add
i72
group;group
e53
group
yA
cK2);group
yA
group_add);group
x02;add.iR1
group);c11}
}
t12{if(add.AddCollection
nW1
a))==CollectionSetBase::xA2)c11}
}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_AddGrouping: "
eS
#endif
tree.DelParams();xH2
e81::xI1
j=add.i8.nW3
j!=add.i8.end();++j)nG1&value=j
cJ2.value
yJ&coeff=j
cJ2.xD2;if(j
cJ2.e2)coeff
x02;if(coeff
tF3(fp_equal(coeff.xJ1,eS1(0)))y81
if(fp_equal(coeff.xJ1,nY3{tree
yA
value);y81}
}
yK2
mul;mul
e53
mul
yA
value);mul
yA
coeff);mul.Rehash(tT
t02;}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_AddGrouping: "
eS
#endif
return
true;}
nX2}
iO2{xQ
using
iO2
FPoptimizer_CodeTree;xG1
ConstantFolding_IfOperations(tT3(tree.tU3()==cIf
tI3()==i03);for(;;){l23
nE==cNot){tH
cIf
tT
lD
0).xO2
0)lD
0)tT
lD
1).swap
nW1
2));}
else
l23
cG1{tH
i03
tT
lD
0).xO2
0)lD
0)tT
lD
1).swap
nW1
2));}
else
t62
tX
0),cV2
i03))tF1
tree.xO2
1));nW
yO3
xP3
xO2
2));nW
lX1
l23
nE==cIf||e72
nE==i03)nG1
cond
cL2
0)yJ
l53;l53
xP2==cIf?cNotNot:cAbsNotNot);l53
x63
x03;ConstantFolding(l53)yJ
truth_b;truth_b
xP2==cIf?cNotNot:cAbsNotNot);truth_b
x63
x73
ConstantFolding(truth_b);if(l53
y21||truth_b
y21)nG1
eU;eU
xP2);eU
x63
x03;eU
yC
1));eU
yC
2));eU
i72
eV;eV
xP2);eV
x63
x73
eV
yC
1));eV
yC
2));eV
y3
cond
nE
xK1
0,cond
lD
0)tT.nC1
1,eU
tT.nC1
2,eV)nS2}
if
nW1
1)nE==tree
lD
2)nE&&nW1
1)nE==cIf||tree
lD
1)nE==i03))nG1&cM2
cL2
1)yJ&leaf2
cL2
2);if
x83
0).xO
0))&&x83
1).xO
1))||cM2
lD
2).xO
2))))nG1
eU;eU
xQ2
eU
yC
0));eU
eT
cM2
x03;eU
eT
leaf2
x03;eU
i72
eV;eV
xQ2
eV
yC
0));eV
eT
cM2
x73
eV
eT
leaf2
x73
eV
y3
cM2
nE
xK1
0
eF3
0)tT.nC1
1,eU
tT.nC1
2,eV)nS2
if
x83
1).xO
1))&&cM2
lD
2).xO
2)))nG1
eW;eW
xQ2
eW
yA
e72);eW
eT
cM2
xY3
eW
eT
leaf2
xY3
eW
y3
cM2
nE
tT.nC1
0,eW
xK1
2
eF3
2)xK1
1,cM2
x03
nS2
if
x83
1).xO
2))&&cM2
lD
2).xO
1)))nG1
cN2;cN2
tU
leaf2
nE==cIf?cNot:yR3);cN2
eT
leaf2
xY3
cN2
i72
eW;eW
xQ2
eW
yA
e72);eW
eT
cM2
xY3
eW
yA
cN2);eW
y3
cM2
nE
tT.nC1
0,eW
xK1
2
eF3
2)xK1
1,cM2
x03
nS2}
yK2&xW
cL2
1)yJ&y9
cL2
2);if(xW
iA
y9)){tree.xO2
1))nS2
const
OPCODE
op1=xW
nE;const
OPCODE
op2=y9
nE;if(op1==op2){if(xW.cT1
1)nG1
lO
0))xL2
0
y31)lY
if(xW.cT1
2&&y9.cT1
2){if(xW
lD
0)iA
y9
lD
0)))nG1
param0=xW
lD
0)yJ
lO
1))xL2
1
y31
tT
yA
param0)lY
if(xW
lD
1)iA
y9
x03)nG1
param1=xW
lD
1)yJ
lO
0))xL2
0
y31
tT
yA
n41
tT
yA
param1)nS2}
if(op1==x93
cMul
iU1
cAnd
iU1
cOr
iU1
cAbsAnd
iU1
cAbsOr
iU1
cMin
iU1
cMax){yG
nV1
l63;cC{for(iX2
y9.cN3
b-->0;){lW2
y9
lD
b))){if(l63
cQ3){xW.iY
y9.iY}
l63
yL
xW
lD
a));y9.iH1
b);xW.iH1
a
nW2}
}
if(!l63
cQ3){xW
x02;y9
i72
n41;n41
xQ2
n41
tI1
tree.iE1));n41
y3
op1
tT
tI1
l63)lY}
}
if(op1==x93
cMul||(op1==cAnd
nY1
y9))||(op1==cOr
nY1
y9))){cC
lW2
y9)){xW.iY
xW
cP3
xW
i72
cD1=y9;y9=tC
op1==x93
cOr)l9
op1
tT
yA
cD1)lY}
if((op1==cAnd
iU1
cOr)&&op2==cNotNot)nG1&l73=y9
lD
0);cC
lW2
l73)){xW.iY
xW
cP3
xW
i72
cD1=l73;y9=tC
op1
xA3
op1
tT
yA
cD1)lY}
if(op2==cAdd||op2==cMul||(op2==cAnd
nY1
xW))||(op2==cOr
nY1
xW))){for
xK2
y9.tD
y9
lD
a)iA
xW)){y9.iY
y9
cP3
y9
i72
cE1=xW;xW=tC
op2==cAdd||op2
xA3
op2
tT
yA
cE1)lY}
if((op2==cAnd||op2==cOr)&&op1==cNotNot)nG1&l83=xW
lD
0)c23
y9.tD
y9
lD
a)iA
l83)){y9.iY
y9
cP3
y9
i72
cE1=l83;xW=tC
op2
xA3
op2
tT
yA
cE1)lY}
nX2}
#include <limits>
iO2{xQ
using
iO2
FPoptimizer_CodeTree;yV1
int
maxFPExponent(){return
std::numeric_limits
x8::max_exponent;}
xG1
x71
eS1
xB3
cE2){if(base<eS1(0
iF1
i82
xB3(0))||fp_equal(xB3(1))yI
return
cE2>=eS1(maxFPExponent
x8())/fp_log2(base);}
xG1
ConstantFolding_PowOperations(tT3(tree.tU3()==cPow);y2){eS1
const_value=eG3
n6,y4
tT.ReplaceWithImmed(const_value)l81
false;}
if(nE1
fp_equal(y4,nY3{tree.xO2
0))nS2
lX&&fp_equal(n6,nY3
lL
1)l81
false;}
lX&&tree
lD
1)nE==cMul){bool
xR2
i13
eS1
iV1=n6
yJ
n83
cL2
1)c23
xX1
tD
n83
lD
a)xC3
imm=n83
lD
a).xJ1;{if(x71
iV1,imm
n33
iW1=eG3
iV1,imm)i82
iW1,eS1(0)))break;if(!xR2){xR2=true;xX1
iY}
iV1=iW1;xX1
iH1
a
nW2}
if(xR2){xX1
Rehash();
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before pow-mul change: "
eS
#endif
e72.Become(cU1
iV1)tT
lD
1).Become(n83);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After pow-mul change: "
eS
#endif
}
}
if(nE1
e72
nE==cMul){eS1
iX1=y4;eS1
xS2=1.0;bool
xR2=false
yJ&n83
cL2
0)c23
xX1
tD
n83
lD
a)xC3
imm=n83
lD
a).xJ1;{if(x71
imm,iX1
n33
eD1=eG3
imm,iX1)i82
eD1,eS1(0)))break;if(!xR2){xR2=true;xX1
iY}
xS2*=eD1;xX1
iH1
a
nW2}
if(xR2){xX1
Rehash()yJ
cD3;cD3
tU
cPow);cD3
tI1
tree.iE1));cD3.y12);tH
cMul
tT
yA
cD3
tT
eT
cU1
xS2))nS2}
l23
nE==cPow&&nE1
e72
lD
1)xC3
a
cL2
0)lD
1).xJ1;eS1
b=y4;eS1
c=a*b;if(isEvenInteger(a)&&!isEvenInteger(c))nG1
l93;l93
tU
cAbs);l93
yC
0)xY3
l93.Rehash(tT.nC1
0,l93);}
else
tree.SetParam(0,e72
lD
0)xK1
1,cU1
c));}
nX2}
iO2{xQ
using
iO2
FPoptimizer_CodeTree;e92
l4{enum
cO2{MakeFalse=0,MakeTrue=1,t22=2,lC3=3,MakeNotNotP0=4,MakeNotNotP1=5,MakeNotP0=6,MakeNotP1=7,Unchanged=8
n23
iY1{Never=0,Eq0=1,Eq1=2,xD3=3,xR3=4}
;cO2
if_identical;cO2
iZ1
4];e92{cO2
what:4;iY1
when:4;}
tS1,tT1,tU1,tV1;yV1
cO2
Analyze(lM1&a,lM1&b)const{if(a
iA
b)nZ2
if_identical;range
x8
p0=iM
a);range
x8
p1=iM
b);if(p0
i0&&p1.iV2){if(p0.max<p1.min&&iZ1
0]cJ
0];if(p0.max<=p1.min&&iZ1
1]cJ
1];}
if(p0
y41
p1
i0){if(p0.min>p1.max&&iZ1
2]cJ
2];if(p0.min>=p1.max&&iZ1
3]cJ
3];}
if(IsLogicalValue(a)){if(tS1
i92
tS1.when,p1)nZ2
tS1.what;if(tU1
i92
tU1.when,p1)nZ2
tU1.what;}
if(IsLogicalValue(b)){if(tT1
i92
tT1.when,p0)nZ2
tT1.what;if(tV1
i92
tV1.when,p0)nZ2
tV1.what;}
return
Unchanged
eI2
bool
TestCase(iY1
when,const
range
x8&p){if(!p.iV2||!p
i0
yI
switch(when
iY2
Eq0:return
p.min==0.0
xQ3==p.min;case
Eq1:return
p.min==1.0
xQ3==p.max;case
xD3:return
p.min>0
xQ3<=1;case
xR3:return
p.min>=0
xQ3<1;default:;}
nX2}
;iO2
RangeComparisonsData{static
const
l4
Data[6]={{l4
lA3
tP
Unchanged,l4::tP
Unchanged
tE
Eq1
tF
Eq1
nD1
Eq0}
,l91
Eq0}
}
,{l4::lX2
lB3
Unchanged,l4
lB3
Unchanged
tE
Eq0
tF
Eq0
nD1
Eq1}
,l91
Eq1}
}
,{l4::lX2
lB3
t22,l4::tP
MakeFalse
nD1
xD3
tF
xR3
xD,{l4
lA3
Unchanged,l4
lB3
tP
lC3
nD1
xR3
tF
xD3
xD,{l4::lX2::tP
tP
MakeTrue,l4::t22
tE
xR3}
,l91
xD3
xD,{l4
lA3
tP
lC3,l4::Unchanged,l4
nH1
tE
xD3}
,l91
xR3
xD}
;}
xG1
ConstantFolding_Comparison(eR{using
iO2
RangeComparisonsData;assert(tree.tU3()>=cEqual&&tree.tU3()<=cGreaterOrEq);switch(Data[tree
nE-cEqual].Analyze
nW1
0),tree
x03
iY2
l4::MakeFalse
xP3
ReplaceWithImmed(0);nW
l4
nH1
xP3
ReplaceWithImmed(1
l33
lC3:tH
cEqual
l33
t22:tH
t71
l33
MakeNotNotP0:tH
cNotNot
tW1
1
l33
MakeNotNotP1:tH
cNotNot
tW1
0
l33
MakeNotP0:tH
cNot
tW1
1
l33
MakeNotP1:tH
cNot
tW1
0
l33
Unchanged:;}
if
nW1
1)y21)switch
nW1
0)nE
iY2
cAsin
xP3
lR
fp_sin(iA2
cAcos
xP3
lR
fp_cos(y4)));tH
cV2
cLess?cGreater:cV2
cLessOrEq?cGreaterOrEq:cV2
cGreater?cLess:cV2
cGreaterOrEq?cLessOrEq:tree
nE);nW
cAtan
xP3
lR
fp_tan(iA2
cLog
xP3
lR
fp_exp(iA2
cSinh
xP3
lR
fp_asinh(iA2
cTanh:if(fp_less(fp_abs(y4),nY3{tree.lR
fp_atanh(y4)))nS2
break;default:nM3
nX2}
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
xQ
iO2{
#ifdef DEBUG_SUBSTITUTIONS
yW
double
d){union{double
d;uint_least64_t
h;}
t32
d=d;lH1
h
nZ1
#ifdef FP_SUPPORT_FLOAT_TYPE
yW
float
f){union{float
f;uint_least32_t
h;}
t32
f=f;lH1
h
nZ1
#endif
#ifdef FP_SUPPORT_LONG_DOUBLE_TYPE
yW
long
double
ld){union{long
double
ld;e92{uint_least64_t
a;i02
short
b;}
s;}
t32
ld=ld;lH1
s.b<<data.s.a
nZ1
#endif
#ifdef FP_SUPPORT_LONG_INT_TYPE
yW
long
ld){o<<"("
<<std::hex<<ld
nZ1
#endif
#endif
}
iO2
FPoptimizer_CodeTree{lM
nF)){}
lM
e62
i
yO
nC3
nF
i)){data
xB
#ifdef __GXX_EXPERIMENTAL_CXX0X__
lM
eS1&&i
yO
nC3
nF
t72
i))){data
xB
#endif
lM
i02
v
yO
VarTag
nF
iE2,v))eS2
nR2
o
yO
OpcodeTag
nF
o))eS2
nR2
o,i02
f
yO
FuncOpcodeTag
nF
o,f))eS2
lM1&b
yO
CloneTag
nF*b.data)){}
yV1
yK2::~yL2(){}
lA
ReplaceWithImmed
i01
eS1&i){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Replacing "
;xE3(*this);if(IsImmed())OutFloatHex(std::cout,xJ1)lN1" with const value "
<<i;OutFloatHex(std::cout,i)lN1"\n"
;
#endif
data=new
nO2
x8(i);}
yT1
ParamComparer{i12()(lM1&a,lM1&b)const{if(a.xT2!=b.xT2
nZ2
a.xT2<b.xT2
l81
a.GetHash()<b.GetHash();}
}
;xB1
nO2
x8::Sort(){switch(Opcode
iY2
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cAbsAnd:case
cOr:case
cAbsOr:case
cHypot:case
cEqual:case
t71:std::sort(iC2
i62
iC2
end(),ParamComparer
x8());lC
cLess
lU
cGreater;}
lC
cLessOrEq
lU
cGreaterOrEq;}
lC
cGreater
lU
cLess;}
lC
cGreaterOrEq
lU
cLessOrEq;}
break;default:nM3}
lA
AddParam(lM1&param){xY
yL
param);}
lA
eG
yK2&param){xY
yL
yK2());xY.back().swap(param);}
lA
SetParam(size_t
which,lM1&b)nI1
which
iB2
xY[which]=b;}
lA
nC1
size_t
which,yK2&b)nI1
which
iB2
xY[which
t23
b);}
lA
AddParams
i01
nK){xY.insert(xY.end(),l02.i62
l02.end());}
lA
AddParamsMove(nK){size_t
endpos=xY
yA3,added=l02
yA3;xY
nE3
endpos+added,yK2());for(size_t
p=0;p<added;++p)xY[endpos+p
t23
l02[p]);}
lA
AddParamsMove(nK,size_t
l12)nI1
l12
iB2
iH1
l12);AddParamsMove(eU1}
lA
SetParams
i01
nK){yG
nV1
tmp(eU1
xY.swap(tmp);}
lA
SetParamsMove(nK){xY.swap(eU1
l02.clear();}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
lA
SetParams(yG
nV1&&l02){SetParamsMove(eU1}
#endif
lA
iH1
size_t
index){yG
nV1&c93=xY;
#ifdef __GXX_EXPERIMENTAL_CXX0X__
iC2
erase(iC2
begin()+index);
#else
c93[index].data=0;for(size_t
p=index;p+1<c93
eW2
p)c93[p].data.UnsafeSetP(&*c93[p+1
iB2
c93[nF3-1].data.UnsafeSetP(0);iC2
resize(nF3-1);
#endif
}
lA
DelParams(){xY.clear();}
xG1
yK2::IsIdenticalTo(lM1&b)const{if(&*data==&*b.data
nZ2
true
l81
data->IsIdenticalTo(*b.data);}
xG1
nO2
x8::IsIdenticalTo
i01
nO2
x8&b)const{if(Hash!=b.Hash
yI
if(Opcode!=t43
yI
switch(Opcode
iY2
cImmed:return
fp_equal(Value,t53;case
iE2:return
iJ1==b.iI1
case
cFCall:case
cPCall:if(iJ1!=b.iJ1
yI
break;default:nM3
if(nF3!=b.nF3
yI
for
xK2
0;a<c93
eW2
a){if(!c93[a]iA
b.c93[a])yI}
return
true;}
lA
Become(lM1&b){if(&b!=this&&&*data!=&*b.data){DataP
tmp=b.data;iY
data.swap(tmp);}
}
lA
CopyOnWrite(){if(GetRefCount()>1)data=new
nO2
x8(*data);}
yV1
yK2
yK2::GetUniqueRef(){if(GetRefCount()>1
nZ2
yK2(*this,CloneTag())l81*this;}
tQ):c7
cNop
l43),nC
tQ
const
nO2&b):c7
t43
l43
t53,iJ1(b.iJ1),c93(b.c93),Hash(b.Hash),Depth(b.Depth),eZ1
b.iK1){}
tQ
e62
i):c7
cImmed
l43
i),nC
#ifdef __GXX_EXPERIMENTAL_CXX0X__
tQ
nO2
x8&&b):c7
t43
l43
t72
t53),iJ1(b.iJ1),c93(t72
b.c93)),Hash(b.Hash),Depth(b.Depth),eZ1
b.iK1){}
tQ
eS1&&i):c7
cImmed
l43
t72
i)),nC
#endif
tQ
nR2
o):c7
o
l43),nC
tQ
nR2
o,i02
f):c7
o
l43),iJ1(f),c93(),Hash(),Depth(1),eZ1
0){}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <iostream>
xQ
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
iO2{xB1
l22
cA3,eH3&done,std::ostream&o){for
yS
l22
i43
done,o);std::ostringstream
buf;xE3(tree,buf);done[tree.GetHash()].insert(buf.str());}
}
#endif
iO2
FPoptimizer_CodeTree{
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
xB1
DumpHashes
i01
eY){eH3
done;l22(tree,done,o);for(eH3::const_iterator
i=done.nW3
i!=done.end();++i){const
std::set<std::string>&flist=i
cJ2;if(flist
yA3!=1)o<<"ERROR - HASH COLLISION?\n"
;for(std::set<std::string>::const_iterator
j=flist.nW3
j!=flist.end();++j){o<<'['<<std::hex<<i->first.hash1<<','<<i->first.hash2<<']'<<std::dec;o<<": "
<<*j<<"\n"
;}
}
}
xB1
xE3
i01
eY){const
char*tN3;switch
xU2
iY2
cImmed:o<<yZ3
l81;case
iE2:o<<"Var"
<<(tree.GetVar()-iE2)l81;case
cAdd:tN3"+"
;lC
cMul:tN3"*"
;lC
cAnd:tN3"&"
;lC
cOr:tN3"|"
;lC
cPow:tN3"^"
;break;default:tN3;o<<FP_GetOpcodeName
xU2);if
xU2==cFCall||cV2
cPCall)o<<':'<<tree.GetFuncNo();}
o<<'(';if(iT<=1&&sep2[1])o<<(sep2+1)<<' 'eV3
if(a>0)o<<' ';xE3
i43
o);if(a+1<iT)o<<sep2;}
o<<')';}
xB1
DumpTreeWithIndent
i01
eY,const
std::string&indent){o<<'['<<std::hex<<(void*)(&tree.iE1))<<std::dec<<','<<tree.GetRefCount()<<']';o<<indent<<'_';switch
xU2
iY2
cImmed:o<<"cImmed "
<<yZ3;o<<'\n'l81;case
iE2:o<<"VarBegin "
<<(tree.GetVar()-iE2);o<<'\n'l81;default:o<<FP_GetOpcodeName
xU2);if
xU2==cFCall||cV2
cPCall)o<<':'<<tree.GetFuncNo();o<<'\n';}
for
yS{std::string
ind=indent;for(size_t
p=0;p<ind
yA3;p+=2)if(ind[p]=='\\')ind[p]=' ';ind+=(a+1<iT)?" |"
:" \\"
;DumpTreeWithIndent
i43
o,ind);}
o<<std::flush;}
#endif
}
#endif
using
iO2
l41;xQ
#include <cctype>
iO2
l41{xG1
ParamSpec_Compare
i01
void*aa,const
void*bb,lU2
type){switch(type
iY2
lY2
xH&a=*(xH*)aa;xH&b=*(xH*)bb
l81
a
t42==b
t42&&a.index==b.index&&a.depcode==b.depcode
cM3
NumConstant:{eZ&a=*(eZ*)aa;eZ&b=*(eZ*)bb
l81
fp_equal(a.xZ3,b.xZ3)&&a.modulo==b.modulo
iD2
xI&a=*(xI*)aa;xI&b=*(xI*)bb
l81
a
t42==b
t42&&a.cS==b.cS&&a.data.match_type==b.data.match_type&&a.data
xY2==b.data
xY2&&a.data.param_list==b.data.param_list&&a.data.n2==b.data.n2&&a.depcode==b.depcode;}
}
return
true;}
i02
ParamSpec_GetDepCode
i01
cG2&b){switch(b.first
iY2
lY2
yK3*s=i01
xH*)b
eE3
l81
s->depcode
iD2
const
xI*s=i01
xI*)b
eE3
l81
s->depcode;}
default:nM3
return
0;}
xB1
DumpParam
i01
cG2&xZ2
std::ostream&o){static
const
char
ParamHolderNames[][2]={"%"
,"&"
,"x"
,"y"
,"z"
,"a"
,"b"
,"c"
}
;i02
t52
0;xT3
NumConstant:{const
eZ&t82
eZ*x01;o.precision(12);o<<xF3
xZ3;nM3
case
lY2
yK3&t82
xH*x01;o<<ParamHolderNames[xF3
index];t52
param
t42;break
iD2
const
xI&t82
xI*x01;t52
param
t42;yZ
GroupFunction){if
iZ2
cS==cNeg){o<<"-"
;n3}
tP1
xF3
cS==cInv){o<<"/"
;n3}
else{std::string
opcode=FP_GetOpcodeName((nR2)xF3
cS).substr(1)c23
0;a<opcode
eW2
a)opcode[a]=(char)std::toupper(opcode[a]);o<<opcode<<"( "
;n3
o<<" )"
;}
}
else{o<<'('<<FP_GetOpcodeName((nR2)xF3
cS)<<' ';yZ
PositionalParams)o<<'[';yZ
SelectedParams)o<<'{';n3
if
iZ2
data.n2!=0)o<<" <"
<<xF3
data.n2<<'>';yZ
PositionalParams)o<<"]"
;yZ
SelectedParams)o<<"}"
;o<<')';}
nM3
yN3(ImmedConstraint_Value(tX1
ValueMask)iY2
ValueMask:lC
Value_AnyNum:lC
n12:o<<"@E"
;lC
Value_OddInt:o<<"@O"
;lC
t01:o<<"@I"
;lC
Value_NonInteger:o<<"@F"
;lC
eE1:o<<"@L"
;t62
ImmedConstraint_Sign(tX1
SignMask)iY2
SignMask:lC
Sign_AnySign:lC
nJ1:o<<"@P"
;lC
n02:o<<"@N"
;t62
ImmedConstraint_Oneness(tX1
OnenessMask)iY2
OnenessMask:lC
Oneness_Any:lC
Oneness_One:o<<"@1"
;lC
Oneness_NotOne:o<<"@M"
;t62
ImmedConstraint_Constness(tX1
ConstnessMask)iY2
ConstnessMask:lC
yL1:if(lF3.first==ParamHolder){yK3&t82
xH*x01;if
iZ2
index<2)nM3
o<<"@C"
;lC
Constness_NotConst:o<<"@V"
;lC
Oneness_Any:nM3}
xB1
DumpParams
iL1
paramlist,i02
count,std::ostream&o){for
iL1
a=0;a<count;++a){if(a>0)o<<' ';const
cG2&param=e01
x8(paramlist,a);DumpParam
x8(param,o);i02
depcode=ParamSpec_GetDepCode(param);if(depcode!=0)o<<"@D"
<<depcode;}
}
}
#include <algorithm>
using
iO2
l41;xQ
iO2{yK3
plist_p[36]={{2,0,0x0}
tS
0,0x4}
tS
nJ1,0x0}
tS
n02|Constness_NotConst,0x0}
tS
Sign_NoIdea,0x0}
tS
eE1,0x0}
,{3,Sign_NoIdea,0x0}
,{3,0,0x0}
,{3,eE1,0x0}
,{3,0,0x8}
,{3,Value_OddInt,0x0}
,{3,Value_NonInteger,0x0}
,{3,n12,0x0}
,{3,nJ1,0x0}
,{0,n02|n0{0,n0{0,nJ1|n0{0,n12|n0{0,yL1,0x1}
,{0,t01|nJ1|n0{0,t11
yL1,0x1}
,{0,t11
n0{0,Oneness_One|n0{0,eE1|n0{1,n0{1,n12|n0{1,t11
n0{1,t01|n0{1,nJ1|n0{6,0,0x0}
,{4,0,0x0}
,{4,t01,0x0}
,{4,n0{4,0,0x16}
,{5,0,0x0}
,{5,n0}
;yT1
plist_n_container{static
const
eZ
plist_n[19];}
;yV1
const
eZ
plist_n_container
x8::plist_n[19]={{eS1(-2
iZ-1
iZ-0.5
iZ
0
tB2
fp_const_deg_to_rad
cE3
fp_const_einv
cE3
fp_const_log10inv
x8(iZ
0.5
tB2
fp_const_log2
x8(iZ
1
tB2
fp_const_log2inv
x8(iZ
2
tB2
fp_const_log10
cE3
fp_const_e
cE3
fp_const_rad_to_deg
cE3-fp_const_pihalf
x8(),xL1{eS1(0),xL1{fp_const_pihalf
x8(),xL1{fp_const_pi
x8(),xL1}
;const
xI
plist_s[464]={{{1,15,tC2
15,cNeg,GroupFunction,0}
,yL1,0x1}
,{{1,422,tC2
423,tC2
15,tD2
24,tD2
410,tD2
411,cInv,xZ
2,301330,cAdd,tG2
271397
lU1
0x5
tY3
271397,l5
45
lU1
iR
2,58413,l5
140333,l5
194605
lU1
0x1
tY3
224301,l5
270373,l5
270381,l5
271405,l5
271405
lU1
0x5
tY3
140341,l5
223285,l5
286757,l5
286765,l5
322605,l5
232501,l5
7168,l5
30727,l5
141312,l5
179207,l5
58383,l5
59407,l5
285718
lU1
iR
2,59416,l5
29726,l5
34823,l5
18597,l5
46264,l5
15764,l5
57509,l5
293049,l5
292026,l5
161979,l5
161980,l5
172265,l5
173308,l5
243951,l5
247024,l5
38152,l5
46344,l5
293147,l5
1332,l5
24980,l5
183473,l5
183570,l5
418001,l5
420248,cAdd,lP
0,0,tR
0,0,cAdd,i3
1,43,tR
1,51,tR
1,52,tR
1,53,tR
1,54,tR
1,0,lZ
iR
1,0,t51
2}
,0,iR
1,0,tR
1,21,tR
1,15,tR
1,26,tR
1,24,cAdd,i3
2,56344,cAdd,i3
1,219,tR
1,230,cAdd,i3
1,245,lZ
0x16}
,{{1,329,lZ
0x16}
,{{1,399,lZ
iR
1,401,lZ
iR
0,0,t51
1}
,nJ1,0x0
tY3
46095,tE2
24591,tE2
32783,tE2
37,c1
7205,c1
114725,c1
288805,lJ
0x6
tY3
347173,lJ
0x6
tY3
331813,c1
350245,c1
372773,c1
377893,c1
378917,c1
383013,c1
388133,c1
439333,c1
442405,c1
447525,c1
450597,c1
459813,c1
468005,c1
305201,l2
3,61910016,c1
7168,c1
114688,c1
512000,l2
3,45508608,c1
15,c1
30727,c1
71695,c1
130063,c1
286735,lJ
0x1
tY3
29726,c1
34823,c1
115736,c1
114712,c1
299008,c1
288783
xM2
300032,c1
347151
xM2
357376,l2
3,65425438,c1
420864,c1
280611,c1
358407,c1
301088,c1
55,c1
38143,c1
38143,lJ
iR
2,38145,lJ
iR
2,38152,c1
38171,c1
15631,c1
15711,c1
56671,c1
38262,lJ
iR
2,60820,c1
38325,lJ
iR
3,15777143,c1
37303,c1
48505,l2
3,15777207,c1
347191
xM2
48571,c1
103714,c1
104739,c1
266538,c1
307547,c1
304475,c1
353627,c1
48490,c1
310338,c1
376173,lJ
lP2
3,39173485,lJ
lP2
2,436589,lJ
lP2
2,7578,c1
376232,lJ
lP2
2,436648,lJ
lP2
3,39233901,lJ
lP2
3,39233960,lJ
lP2
2,7651,c1
7675,l2
0,0,nO
0,0,cT
1,37,nO
1,37,cT
1,2,n22
2,n32
3,n22
3,n32
0,nO
1,0,n32
0,cT
1,14,nO
1,16,nO
1,16,lK
1
xN2
21,nO
1,15,nO
1,24,cT
2,24591,nO
1,55,nO
1,55,lK
2
xN2
275,n22
278,n32
284,cT
1,287,nO
1,288,nO
1,289,cT
1,462,cT
2,413757,lK
1
xN2
295,nO
1,329,cT
2,414025,lK
1}
,0,0x16}
,{{1,351,nO
1,404,nO
1,410,nO
2,60459,l32
44047,l32
24591,l32
32783,l32
44056,l32
41,lI,41,y0
49,lI,49,y0
365609,lI,222257,lI,365617,lI,366633,lI,366641,lI,48128,lI,15,lI,15,cV1
16,lI,10240,lI,11264,lI,7170,lI,7168,lI,7168,y0
7183,cV1
17408,lI,19456,lI,16384,lI,15360,lI,27648,lI,30720,lI,30722,lI,24,tF2
0x6
tY3
24,lI,7192,lI,68608,lI,83968,lI,86040,lI,87040,lI,88064,lI,90112,lI,432128,lI,433152,lI,37895,lI,14342,lI,25607,lI,7183,lI,56327,lI,114703,lI,114718,lI,257024,lI,419840,lI,260103,lI,37953,tF2
0x5
tY3
37956,y0
37961,tF2
0x5
tY3
38105,lI,38114,y0
38984,y0
44103,y0
44104,y0
38991,lI,44111,lI,44135,lI,44124,y0
44136,lI,48240,lI,60693,lI,38253,y0
38253,lI,38259,tF2
0x5
tY3
38260,cV1
38262,y0
38262,lI,48493,y0
48493,lI,15734,y0
137590,lI,38264,tF2
0x5
tY3
38292,lI,38294,lI,38300,lI,38301,lI,38312,y0
38325,y0
38332,y0
38332,lI,38341,lI,38341,y0
38343,lI,60,lI,60,tF2
0x6
tY3
48552,y0
48552,lI,257198,lI,260274,lI,24792,lI,7172,cPow,PositionalParams,0}
,nJ1,0x0
tY3
24591,cPow,xZ
2,60440,cPow,xZ
2,60451,cPow,xZ
2,61472,cPow,xZ
1,0,eI3
7,eI3
157,eI3
0,cAcos
eJ3
cAcosh
eJ3
cAsin
eJ3
cAsinh
nR
112,cAsinh
eJ3
cAtan,eF1
cAtan2,tG2
303104
i23
eJ3
cAtanh
eJ3
cCeil,cF3
216,cCeil
eJ3
yO2
0,cCos,cF3
7,yO2
81,yO2
83,yO2
112,yO2
180,yO2
234,yO2
0,cH3
cF3
0,cI3
176,cI3
180,cI3
409,cI3
0,cFloor,cF3
216,cFloor,tG2
308523,eK3
tG2
352555,eK3
tG2
352599,eK3
l0
3,31464448,cU
507534336,cU
508566528,cU
33579008,cU
30443520,lD3
31464448,lD3
7836672,cU
24612864,cU
93415424,cU
142744576,cU
174234624,cU
265547776,cU
435585024,cU
439783424,cU
519553024,cU
526900224,cU
58739160,cU
58739160,lD3
58739166,cU
58739166,cIf,cF3
112,cInt
eJ3
tH2
7,tH2
30,tH2
157,tH2
216,tH2
282,tH2
15,cLog,xZ
1,24,cLog,xZ
1,0,cLog10
eJ3
cLog2,eF1
cMax,tG2
29726,cMax,tG2
34823,cMax
eJ3
cMax,AnyParams,1}
,0,iR
xG3
cMin,tG2
29726,cMin,tG2
34823,cMin
eJ3
cMin,AnyParams,1}
,0,iR
2,46095,cMin,xZ
2,24591,cMin,xZ
1,0,n42
0,cSin,cF3
7,n42
81,n42
83,n42
112,n42
139,n42
161,cSin,nU
0x5}
,{{1,216,n42
227,n42
231,cSin,nU
0x1}
,{{1,234,n42
0,cSinh,cF3
0,cSinh
nR
161,cSinh,nU
0x5}
,{{1,176,cSinh
nR
216,cSinh
nR
227,cSinh
nR
234,cSinh
nR
409,cSinh
eJ3
yP2
0,cTan,cF3
75,cTan,cF3
76,yP2
161,yP2
216,yP2
231,yP2
227,yP2
234,yP2
0,xV2
0,cTanh,cF3
160,xV2
161,xV2
216,xV2
227,xV2
234,xV2
0,cTrunc,tG2
15384,cSub,xZ
2,15384,cDiv,xZ
2,420251,cDiv,xZ
xG3
tK2
nU
t41
tK2
tG2
30720,tK2
nU
0x20
tY3
30727,tK2
nU
0x24
tY3
30727,tK2
tG2
114743,tK2
tG2
114743,t71,tG2
39936,cLess,l6
2,39936,cLess,tG2
7,cLess,eF1
cLess,nU
t41
cLessOrEq,tG2
256216,cLessOrEq,tG2
39936,e82
l6
2,39936,e82
tG2
7,e82
eF1
e82
nU
t41
cGreaterOrEq,tG2
256216,cGreaterOrEq
eJ3
l42
7,l42
15,l42
30,l42
156,l42
494,l42
497,l42
498,l42
501,l42
504,l42
505,cNot,eF1
l52
29726,l52
34823,l52
394270,l52
398366,l52
7651,cAnd,lP
0,0,cAnd,AnyParams,1}
,0,0x0}
,{{xG3
nJ2
29726,nJ2
34823,nJ2
394270,nJ2
398366,nJ2
7651,cOr,lP
1,0
t31
81
t31
121
t31
156
t31
159
t31
216,cDeg
nR
216,cRad,eF1
cAbsAnd,lP
xG3
cAbsOr,lP
1,0,yR3
eJ3
cAbsNotNot,l0
3,31464448,eP3
nU
0x0}
,}
;}
iO2
l41{const
Rule
grammar_rules[253]={{ProduceNewTree,1,1,0,{1,0,cAbs,xJ
361,{1,172,cAtan,xJ
354
tS
1337
i23,xJ
356
tS
320513
i23
l7
2,2,222424
tS
226524
i23
l7
2,2,224474
tS
228574
i23,xJ
148
x13
cCeil,xJ
435,{1,80,tL2
429,{1,115,tL2
430,{1,117,tL2
146,{1,118,tL2
370,{1,116,tL2
0,{1,354,cCos
l7
2,1,0,{1,351,cCos
l7
2,1,216
x13
tL2
314,{1,357,cCosh
l7
2,1,0,{1,351,cCosh
l7
2,1,216
x13
cH3
xJ
144
x13
cFloor,xJ
403,{1,114,cFloor,eL3
214,{3,7379968,c8
518,{3,31464450,c8
499,{3,8428544,c8
501,{3,8434688,c8
215,{3,40901632,c8
494,{3,40902656,c8
507,{3,40940544,c8
506,{3,47194112,c8
483,{3,47225856,c8
414,{3,1058266,c8
418,{3,1058272,c8
418,{3,9438682,c8
414,{3,9438688,c8
460,{3,396733911,c8
460,{3,381020637,cIf
l7
0,3,31492569,{3,35682779,cIf
l7
0,3,31492575,{3,35682785,cIf,xJ
111,{1,228,eM3
110,{1,244,eM3
355,{1,106,eM3
204,{1,205,cLog
l7
0,1,395
cP2,cMax
yF
0
tS
431105,cMax
yF
396
cP2,cMin
yF
0
tS
427009,cMin,AnyParams,0}
}
,{ProduceNewTree,0,1,203
tS
24804,cPow,eL3
203
tS
25827,cPow,eL3
202
tS
126991,cPow
xM1
30988,cPow
xM1
30989,cPow
xM1
30990,cPow
t93
166239
tS
31066,cPow
xM1
32015,cPow
t93
7168
tS
12639,cPow
t93
7392
tS
12535
x23
380
tS
44095
x23
381
tS
44141
x23
382
tS
44140
x23
201
tS
109583
x23
200
tS
132129
x23
155
tS
133153
n52
419840
tS
413711
n52
254976
tS
253967
n52
221184
tS
251937
n52
221184
tS
248850
x23
150
x13
yQ2
372,{1,80,yQ2
146,{1,115,yQ2
370,{1,117,yQ2
149,{1,118,yQ2
429,{1,116,yQ2
0,{1,356,yQ2
152
x13
cSinh,xJ
312,{1,355,cSinh,xJ
153
x13
eN3
0,{1,359,eN3
170,{1,360,eN3
154
x13
cTanh
l7
0,1,392
tS
395279,tM2
391
cP2,tM2
199
tS
242924,tM2
198
tS
230636,tM2
164
tS
240869,tM2
163
tS
240660,t51
0
nM1
263,{1,311,t51
1
nM1
262,{1,310,t51
1
l1
2,1,261
tS
1333
lW1
259
tS
1331
lW1
407
tS
415124
lW1
45
tS
331093
lW1
324
tS
146477
lW1
342
tS
145453
lW1
427
tS
213202
lW1
428
tS
217298
lW1
368
tS
216270
lW1
145
tS
216271
lW1
369
tS
218318
lW1
197
tS
144665
lW1
194
tS
348441
lW1
193
tS
348300
lW1
195
tS
200875
lW1
192
tS
177323
lW1
257
tS
446837
lW1
181
tS
446653
lW1
151
tS
187765
lW1
147
tS
187828
lW1
255
tS
195957
lW1
374
tS
264383
lW1
437
tS
264381
lW1
437
tS
186741
lW1
374
tS
262580
lW1
151
tS
262333,tM2
98
tS
1155
eG1
97
tS
1156
eG1
305
tS
1330
eG1
99
tS
24704
eG1
100
tS
24698
eG1
394
tS
395279
eG1
393
cP2
eG1
353
tS
360799
eG1
96
tS
89360
eG1
105
tS
80155
eG1
95
tS
79131
cQ2
56671
tS
1424
cQ2
15711
tS
1426
cQ2
107535
tS
93467
cQ2
97295
tS
96539,l8
1,1,0,{1,351,l8
1,1,55,{1,14,lK
0
nM1
93
tS
70674,cMul,SelectedParams,0
nM1
512,{1,50,lK
1
nM1
513,{1,40,lK
1
l1
2,1,435
tS
443429,eA
442
tS
451621,eA
336
tS
382285,eA
437
tS
382406,eA
374
tS
446801,eA
365
tS
435534,eA
444
tS
325033,eA
318
tS
340413,eA
335
tS
375116,eA
424
tS
375229,eA
45
tS
456126,eA
450
tS
328114,eA
45
tS
460223,eA
452
tS
461861,eA
453
tS
329140,eA
322
tS
341446,eA
456
tS
335286,eA
448
tS
327087,eA
459
tS
469029,eA
455
tS
311360,eA
338
tS
309322,eA
412
tS
43412,eA
330
tS
49480,eA
413
tS
47508,eA
331
tS
45384,l8
2,2,334277
tS
333236
cQ2
39936
tS
xH3
cEqual
yP
cEqual
yQ
cEqual
yR
cEqual
lA1
24807
t61
iD
t61
l62
237799
t61
n11
cEqual
l72
tK2
eH1
39,tK2
eO3
0
tS
5165,cEqual
t93
39936
tS
xH3
t71
yP
t71
yQ
t71
yR
t71
lA1
24807
tI2
iD
tI2
l62
237799
tI2
n11
t71
l72
t71
tJ2
39,t71,eH1
5165,t71
yP
cLess
yQ
cLess
yR
cLess,xJ
516
tS
44032,cLess
lA1
24804
t21
yM1
t21
iD
t21
l62
237796
t21
xN1
t21
n11
cLess
l72
cLess
tJ2
xH3
cLess
yP
cLessOrEq
yQ
cLessOrEq
yR
cLessOrEq,xJ
510
tS
359439,cLessOrEq
lA1
24804
xC1
yM1
xC1
iD
xC1
l62
237796
xC1
xN1
xC1
n11
cLessOrEq
l72
cLessOrEq
tJ2
359469,cLessOrEq
yP
cGreater
yQ
cGreater
yR
e82
xJ
487
tS
359439,cGreater
lA1
24804
y61
yM1
y61
iD
y61
l62
237796
y61
xN1
y61
n11
cGreater
l72
e82
eH1
359469,cGreater
yP
cGreaterOrEq
yQ
cGreaterOrEq
yR
cGreaterOrEq,xJ
517
tS
44032
yX
473304
tS
24804
yX
yM1
yX
iD
yX
l62
237796
yX
xN1
yX
7168
tS
279818
yX
lB1
cGreaterOrEq,eH1
xH3
cGreaterOrEq,eL3
516,{1,2,cNot,eO3
469,{1,123,cNot,eL3
511,{1,5,cAnd,AnyParams,1
l1
0,1,514
tS
13314,cAnd
yF
397
cP2,cAnd
yF
491
tS
496099,cAnd
yF
492
tS
399846,cAnd
yF
493
tS
393702,cAnd,AnyParams,0
l1
0,2,479697,{3,489115088,cAnd
yF
515
tS
13314,xI3
508
tS
8197,xI3
398
cP2,xI3
488
tS
496099,xI3
489
tS
399846,xI3
490
tS
393702,xI3
509
tS
136197,cOr,AnyParams,0}
}
,{ProduceNewTree,0,1,517,{1,2,xJ3
l3
1,1,0,{1,0,xJ3
eO3
470,{1,123,xJ3
xJ
482,{1,228,cAbsNotNot,AnyParams,0
nM1
476,{1,227,cAbsNotNot,AnyParams,0}
}
,{ProduceNewTree,0,1,383,{3,31464955,eP3
eL3
517,{3,40940544,eP3
eL3
516,{3,47225856,i03
l7
0,3,31492569,{3,35682779,eP3
PositionalParams,0}
}
,}
;e92
grammar_optimize_abslogical_type{xS
9
eI
grammar_optimize_abslogical_type
grammar_optimize_abslogical={9,{21,183,219,228,231,237,244,249,252}
}
;}
e92
grammar_optimize_ignore_if_sideeffects_type{xS
59
eI
grammar_optimize_ignore_if_sideeffects_type
grammar_optimize_ignore_if_sideeffects={59,{0,20,22,23,24,25,26,27,cV
tY1
78,cZ
xR
grammar_optimize_nonshortcut_logical_evaluation_type{xS
56
eI
grammar_optimize_nonshortcut_logical_evaluation_type
grammar_optimize_nonshortcut_logical_evaluation={56,{0,26,cV
tY1
78,cZ
158,167,168,169,178,179,191,195,203,207,215,227,229,230,232,233,234,235,236,238,239,240,241,242,243,245,246,247,248,250,251}
}
;}
e92
grammar_optimize_round1_type{xS
118
eI
grammar_optimize_round1_type
grammar_optimize_round1={118,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,18,26,cV
37,38,tY1
45,46,47,48,49,50,51,52,53,54,58,59,60,61,62,63,64,65,66,67,68,69,70,71,78,79,80,81,82,83,88,89,90,91,92,93,94,95,96,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,132,xR
grammar_optimize_round2_type{xS
100
eI
grammar_optimize_round2_type
grammar_optimize_round2={100,{0,15,16,17,26,cV
39,40,tY1
45,46,47,48,49,50,51,52,53,54,59,60,72,73,78,79,84,85,86,87,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,117,118,119,120,121,122,123,124,125,126,127,128,133,157,xR
grammar_optimize_round3_type{xS
79
eI
grammar_optimize_round3_type
grammar_optimize_round3={79,{74,75,76,77,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,159,160,161,162,163,164,165,166,170,171,172,173,174,175,176,177,180,181,182,184,185,186,187,188,189,190,192,193,194,196,197,198,199,200,201,202,204,205,206,208,209,210,211,212,213,214,216,217,218,220,221,222,223,224,225,226}
}
;}
e92
grammar_optimize_round4_type{xS
10
eI
grammar_optimize_round4_type
grammar_optimize_round4={10,{19,55,56,57,130,131,153,154,155,156}
}
;}
e92
grammar_optimize_shortcut_logical_evaluation_type{xS
53
eI
grammar_optimize_shortcut_logical_evaluation_type
grammar_optimize_shortcut_logical_evaluation={53,{0,26,cV
tY1
78,cZ
158,167,168,169,178,179,191,195,203,207,215,227,229,232,233,234,235,236,239,240,241,242,245,246,247,248,250,251}
}
;}
}
iO2
l41{yV1
cG2
e01
iL1
paramlist,lC1){index=(paramlist>>(index*10))&1023;if(index>=55
nZ2
cG2(SubFunction
tZ1
plist_s[index-55]);if(index>=36
nZ2
cG2(NumConstant
tZ1
plist_n_container
x8::plist_n[index-36])l81
cG2(ParamHolder
tZ1
plist_p[index]);}
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <stdio.h>
#include <algorithm>
#include <map>
#include <sstream>
xQ
using
iO2
l41;using
iO2
FPoptimizer_CodeTree;using
iO2
FPoptimizer_Optimize;iO2{nT1
It,typename
T,typename
Comp>eI1
MyEqualRange(It
first,It
last,const
T&val,Comp
comp){size_t
len=last-first;while(len>0){size_t
nJ3
len/2;It
n43(first);n43+=half;if(comp(*n43,val)){first=n43;++first;len=len-half-1;}
tP1
comp(val,*n43)){len=half;}
else{It
left(first);{It&cR2=left;It
last2(n43);size_t
len2=last2-cR2;while(len2>0){size_t
half2=len2/2;It
middle2(cR2);middle2+=half2;if(comp(*middle2,val)){cR2=middle2;++cR2;len2=len2-half2-1;}
else
len2=half2;}
}
first+=len;It
right(++n43);{It&cR2=right;It&last2=first;size_t
len2=last2-cR2;while(len2>0){size_t
half2=len2/2;It
middle2(cR2);middle2+=half2;if(comp(val,*middle2))len2=half2;else{cR2=middle2;++cR2;len2=len2-half2-1;}
}
}
return
eI1(left,right);}
}
return
eI1(first,first);}
yT1
OpcodeRuleCompare{i12()(lM1&tree,i02
xW2)const{const
Rule&rule=grammar_rules[xW2]l81
tree
nE<rule
nT2.subfunc_opcode;}
i12()iL1
xW2,const
eR
const{const
Rule&rule=grammar_rules[xW2]l81
rule
nT2.subfunc_opcode<tree
nE;}
}
;xG1
TestRuleAndApplyIfMatch
eX3
yK2&tree,bool
cD{MatchInfo
x8
info;lZ1
found(false,e1());if((rule.eL1
LogicalContextOnly)&&!cD{yR2
if(nD
IsIntType
x8::result){if(rule.eL1
NotForIntegers)yR2
else{if(rule.eL1
OnlyForIntegers)yR2
for(;;){
#ifdef DEBUG_SUBSTITUTIONS
#endif
found=TestParams(rule
nT2,tree,found.specs,info,true);if(found.found)break;if(!&*found.specs){fail:;
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch
t81,false);
#endif
nX2}
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch
t81,true);
#endif
SynthesizeRule
t81)nS2}
iO2
FPoptimizer_Optimize{xG1
ApplyGrammar
i01
Grammar&tN2,yK2&tree,bool
cD{if(tree.GetOptimizedUsing()==&tN2){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Already optimized:  "
;xE3(tree)lN1"\n"
<<std::flush;
#endif
nX2
if(true){bool
changed
i13
switch
xU2
iY2
cNot:case
cNotNot:case
cAnd:case
cOr:for
xK2
0
nP
true))yI1
lC
cIf:case
i03:if(ApplyGrammar(tN2,e72,cV2
cIf))yI1
for
xK2
1
nP
cD)yI1
break;default:for
xK2
0
nP
false))yI1}
if(changed){tree.Mark_Incompletely_Hashed()nS2}
typedef
const
i02
char*lE3;std::pair<lE3,lE3>range=MyEqualRange(tN2.rule_list,tN2.rule_list+tN2.rule_count,tree,OpcodeRuleCompare
x8());if(range.eQ3
range
eE3){
#ifdef DEBUG_SUBSTITUTIONS
yG<i02
char>rules;rules.nH3
range
eE3-range.first);yD
if(IsLogisticallyPlausibleParamsMatch(cW1
nT2
n72
rules
yL*r);}
range.first=&rules[0];range
eE3=&rules[rules
yA3-1]+1;if(range.eQ3
range
eE3){std::cout<<"Input ("
<<FP_GetOpcodeName
xU2)<<")["
<<iT<<"]"
;if(cD
std::cout<<"(Logical)"
;i02
first=i21,prev=i21;const
char*sep=", rules "
;yD
if(first==i21)first=prev=*r;tP1*r==prev+1)prev=*r;else{std::cout<<sep<<first;sep=","
;if(prev!=first)std::cout<<'-'<<prev;first=prev=*r;}
}
if(eQ3
i21){std::cout<<sep<<first;if(prev!=first)std::cout<<'-'<<prev;}
std::cout<<": "
;xE3(tree)lN1"\n"
<<std::flush;}
#endif
bool
changed
i13
yD
#ifndef DEBUG_SUBSTITUTIONS
if(!IsLogisticallyPlausibleParamsMatch(cW1
nT2
n72
y81
#endif
if(TestRuleAndApplyIfMatch(cW1,tree,cD){yI1
nM3}
if(changed){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Changed."
<<std::endl
lN1"Output: "
;xE3(tree)lN1"\n"
<<std::flush;
#endif
tree.Mark_Incompletely_Hashed()nS2}
tree.SetOptimizedUsing(&tN2)l81
false;}
xB1
ApplyGrammars(x2){
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_round1\n"
;
#endif
n4
grammar_optimize_round1
n53
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_round2\n"
;
#endif
n4
grammar_optimize_round2
n53
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_round3\n"
;
#endif
n4
grammar_optimize_round3
n53
#ifndef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_nonshortcut_logical_evaluation\n"
;
#endif
n4
grammar_optimize_nonshortcut_logical_evaluation
n53
#endif
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_round4\n"
;
#endif
n4
grammar_optimize_round4
n53
#ifdef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_shortcut_logical_evaluation\n"
;
#endif
n4
grammar_optimize_shortcut_logical_evaluation
n53
#endif
#ifdef FP_ENABLE_IGNORE_IF_SIDEEFFECTS
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_ignore_if_sideeffects\n"
;
#endif
n4
grammar_optimize_ignore_if_sideeffects
n53
#endif
#ifdef DEBUG_SUBSTITUTIONS
std
tJ3"grammar_optimize_abslogical\n"
;
#endif
n4
grammar_optimize_abslogical
n53
#undef C
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <cmath>
#include <memory> /* for auto_ptr */
xQ
using
iO2
l41;using
iO2
FPoptimizer_CodeTree;using
iO2
FPoptimizer_Optimize;iO2{xG1
TestImmedConstraints
iL1
bitmask,const
eR{switch(bitmask&ValueMask
iY2
Value_AnyNum:case
ValueMask:lC
n12:if(GetEvennessInfo
nO3
l82
Value_OddInt:if(GetEvennessInfo
nO3
n82
t01:if(GetIntegerInfo
nO3
l82
Value_NonInteger:if(GetIntegerInfo
nO3
n82
eE1:if(!IsLogicalValue(tree)yI
nK1
SignMask
iY2
Sign_AnySign:lC
nJ1:if(l01
l82
n02:if(l01
n82
Sign_NoIdea:if(l01
Unknown
yI
nK1
OnenessMask
iY2
Oneness_Any:case
OnenessMask:lC
Oneness_One:if(!cW2
if(!fp_equal(fp_abs(yZ3),eS1(1))yI
lC
Oneness_NotOne:if(!cW2
if(fp_equal(fp_abs(yZ3),eS1(1))yI
nK1
ConstnessMask
iY2
Constness_Any:lC
yL1:if(!cW2
lC
Constness_NotConst:if(cW2
nM3
return
true;}
n73<i02
extent,i02
nbits,typename
cS2=i02
int>e92
nbitmap{private:static
const
i02
bits_in_char=8;static
const
i02
cT2=(xK3
cS2)*bits_in_char)/nbits;cS2
data[(extent+cT2-1)/cT2];e13
void
inc(lC1,int
by=1){data[pos(index)]+=by*cS2(1<<xX2);i6
void
dec(lC1){inc(index,-1);}
int
get(lC1
n61(data[pos(index)]>>xX2)&mask()xJ2
pos(lC1){return
index/cT2
xJ2
shift(lC1){return
nbits*(index%cT2)xJ2
mask(){return(1<<nbits)-1
xJ2
mask(lC1){return
mask()<<xX2;}
}
;e92
c83{int
SubTrees:8;int
Others:8;int
i31:8;int
cG3:8;nbitmap<iE2,2>SubTreesDetail;c83(){std::memset(this,0,xK3*this));}
c83
i01
c83&b){std::memcpy(this,&b,xK3
b));}
c83&e31=i01
c83&b){std::memcpy(this,&b,xK3
b))l81*this;}
}
;yV1
c83
CreateNeedList_uncached(t0&cB2){c83
x11
xS3
a=0;a<cB2
xY2;++a){const
cG2&lF3=e01
x8(cB2.param_list,a);xT3
lZ2
const
xI&t82
xI*x01;yZ
GroupFunction)++tA3;else{++tA2;assert(param.data.subfunc_opcode<VarBegin);x11.SubTreesDetail.inc
iZ2
cS);}
++x11.i31;nM3
case
NumConstant:case
ParamHolder:++t92;++x11.i31;nM3}
return
x11;}
yV1
c83&CreateNeedList(t0&cB2){typedef
std::map<t0*,c83>cX1;static
cX1
yO1;cX1::nX3
i=yO1.xE2&cB2);if(i!=yO1.cP1&cB2
nZ2
i
cJ2
l81
yO1.nT3,std::make_pair(&cB2,CreateNeedList_uncached
x8(cB2)))cJ2;}
yV1
yK2
CalculateGroupFunction
i01
cG2&xZ2
const
t9
info){xT3
NumConstant:{const
eZ&t82
eZ*x01
l81
CodeTreeImmed
iZ2
xZ3)cM3
lY2
yK3&t82
xH*x01
l81
info.GetParamHolderValueIfFound
iZ2
index)iD2
const
xI&t82
xI*x01
yJ
result;result
tU
xF3
cS);tC1
iE1).reserve
iZ2
data
xY2)xS3
a=0;a<xF3
data
xY2;++a)nG1
tmp(CalculateGroupFunction(e01
x8
iZ2
data.param_list,a),info));result
yA
tmp);}
tC1
Rehash()l81
result;}
}
return
yK2();}
}
iO2
FPoptimizer_Optimize{xG1
IsLogisticallyPlausibleParamsMatch(t0&cB2,const
eR{c83
x11(CreateNeedList
x8(cB2));size_t
eR3=iT;if(eR3<size_t(x11.i31)){nX2
for
xK2
0;a<eR3;++a){i02
opcode=xI2
nE;switch(opcode
iY2
cImmed:if(tA3>0)--tA3;else--t92;lC
iE2:case
cFCall:case
cPCall:--t92;break;default:assert(opcode<VarBegin);if(tA2>0&&x11.SubTreesDetail.get(opcode)>0){--tA2;x11.SubTreesDetail.dec(opcode);}
else--t92;}
}
if(tA3>0||tA2>0||t92>0){nX2
if(cB2.match_type!=AnyParams){if(0||tA2<0||t92<0){nX2}
return
true;}
yV1
lZ1
TestParam
i01
cG2&xZ2
lM1&tree
eA2
start_at,t9
info){xT3
NumConstant:{const
eZ&t82
eZ*x01;if(!cW2
eS1
imm=yZ3;switch
iZ2
modulo
iY2
Modulo_None:lC
Modulo_Radians:imm=fp_mod(imm,y8
imm<0)imm
c5
if(imm>fp_const_pi
x8())imm-=fp_const_twopi
x8(nW2
return
fp_equal(imm,xF3
xZ3)cM3
lY2
yK3&t82
xH*x01;if(!x0
return
info.SaveOrTestParamHolder
iZ2
index,tree)iD2
const
xI&t82
xI*x01;yZ
GroupFunction){if(!x0
yK2
xO1=CalculateGroupFunction(xZ2
info);
#ifdef DEBUG_SUBSTITUTIONS
DumpHashes(xO1)lN1*i01
void**)&xO1.xJ1
lN1"\n"
lN1*i01
void**)&yZ3
lN1"\n"
;DumpHashes(tree)lN1"Comparing "
;xE3(xO1)lN1" and "
;xE3(tree)lN1": "
lN1(xO1
iA
tree)?"true"
:"false"
)lN1"\n"
;
#endif
return
xO1
iA
tree);}
else{if(!&*start_at){if(!x0
if
xU2!=xF3
cS
yI}
return
TestParams
iZ2
data,tree,start_at,info,false);}
}
}
nX2
yT1
iS
x22
MatchInfo
x8
info;iS()yC3,info(){}
}
;iP2
MatchPositionSpec_PositionalParams:xP1
iS
x8>{e13
iF2
MatchPositionSpec_PositionalParams(xL3):xM3
iS
x8>(n){}
}
;e92
i41
x22
i41()yC3{}
}
;class
c0:xP1
i41>{e13
i02
trypos;iF2
c0(xL3):xM3
i41>(n),trypos(0){}
}
;yV1
lZ1
TestParam_AnyWhere
i01
cG2&xZ2
lM1&tree
eA2
start_at,t9
info,yG<bool>&used,bool
tA1{xN<c0>x6;i02
lG3
c0
n92
a=x6->trypos;goto
retry_anywhere_2
x42
c0(iT);a=0;}
eS3
iT;++a){if(used[a])y81
retry_anywhere:{lZ1
r=TestParam(xZ2
xI2
lH3);lI3
used[a]=true;if(tA1
eT3
a);x6->trypos=a
l81
lZ1(true,&*x6);}
}
retry_anywhere_2:if(&*lV1){goto
retry_anywhere;}
}
nX2
yT1
y91
x22
MatchInfo
x8
info;yG<bool>used;iF2
y91(size_t
eR3)yC3,info(),used(eR3){}
}
;iP2
MatchPositionSpec_AnyParams:xP1
y91
x8>{e13
iF2
MatchPositionSpec_AnyParams(xL3,size_t
m):xM3
y91
x8>(n,y91
x8(m)){}
}
;yV1
lZ1
TestParams(t0&nM,lM1&tree
eA2
start_at,t9
info,bool
tA1{if(nM.match_type!=AnyParams){if(xT!=iT
yI}
if(!IsLogisticallyPlausibleParamsMatch(nM
n72{nX2
switch(nM.match_type
iY2
PositionalParams:{xN<cL>x6;i02
lG3
cL
n92
a=xT-1;goto
lD1
x42
cL(xT);a=0;}
eS3
xT;++a){cX2=info;retry_positionalparams:{lZ1
r=TestParam(cX
a),xI2
lH3);lI3
y81}
}
lD1:if(&*lV1){info=cX2;goto
retry_positionalparams;}
if(a>0){--a;goto
lD1;}
info=(*x6)[0].info
l81
false;}
if(tA1
for
iL1
a=0;a<xT;++a)eT3
a)l81
lZ1(true,&*x6)cM3
SelectedParams:case
AnyParams:{xN<t6>x6;yG<bool>used(iT);yG<i02>iG2(xT);yG<i02>y02(xT)lO1{const
cG2
lF3=cX
a);iG2[a]=ParamSpec_GetDepCode(lF3);}
{i02
b=0
lO1
if(iG2[a]!=0)y02[b++]=a
lO1
if(iG2[a]==0)y02[b++]=a;}
i02
lG3
t6
n92
if(xT==0){a=0;goto
retry_anyparams_4;}
a=xT-1;goto
cY1
x42
t6(xT,iT);a=0;if(xT!=0){(*x6)[0].info=info;(*x6)[0].used=used;}
}
eS3
xT;++a){if(a>0){cX2=info;(*x6)[a].used=used;}
retry_anyparams:{lZ1
r=TestParam_AnyWhere
x8(cX
y02[a]),tree
lH3,used,tA1;lI3
y81}
}
cY1:if(&*lV1){info=cX2;used=(*x6)[a].used;goto
retry_anyparams;}
cZ1:if(a>0){--a;goto
cY1;}
info=(*x6)[0].info
l81
false;}
retry_anyparams_4:if(nM.n2!=0){if(!TopLevel||!info.HasRestHolder(nM.n2)){yG
nV1
yS2;yS2.nH3
iT)xS3
b=0;b<iT;++b){if(xN3)y81
yS2
yL
tree
lD
b));xN3=true;if(tA1
eT3
b);}
if(!info.SaveOrTestRestHolder(nM.n2,yS2)){goto
cZ1;}
}
else{lL1&yS2=info.GetRestHolderValues(nM.n2)c23
0;a<yS2
eW2
a){bool
found=false
xS3
b=0;b<iT;++b){if(xN3)y81
if(yS2[a]iA
tree
lD
b))){xN3=true;if(tA1
eT3
b);found=true;nM3}
if(!found){goto
cZ1;}
}
}
}
return
lZ1(true,xT?&*x6:0)cM3
GroupFunction:nM3
nX2}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <algorithm>
#include <assert.h>
using
iO2
FPoptimizer_CodeTree;using
iO2
FPoptimizer_Optimize;iO2{yV1
yK2
xQ1
const
cG2&xZ2
t9
info,bool
inner=true){xT3
NumConstant:{const
eZ&t82
eZ*x01
l81
CodeTreeImmed
iZ2
xZ3)cM3
lY2
yK3&t82
xH*x01
l81
info.GetParamHolderValue
iZ2
index)iD2
const
xI&t82
xI*x01
yJ
tree;tH
xF3
cS)xS3
a=0;a<xF3
data
xY2;++a)nG1
nparam=xQ1
e01
x8
iZ2
data.param_list,a),info,true
tT
yA
nparam);}
if
iZ2
data.n2!=0){yG
nV1
trees(info.GetRestHolderValues
iZ2
data.n2)tT.AddParamsMove(trees);if(iT==1){assert(tree.tU3()==cAdd tI3()==cMul tI3()==cMin tI3()==cMax tI3()==cAnd tI3()==cOr tI3()==cAbsAnd tI3()==cAbsOr);tree.xO2
0));}
tP1
iT==0){switch
xU2
iY2
cAdd:case
cOr:tree=nB1
0));lC
cMul:case
cAnd:tree=nB1
1));default:nM3}
}
if(inner)tree
x02
l81
tree;}
}
return
yK2();}
}
iO2
FPoptimizer_Optimize{xB1
SynthesizeRule
eX3
yK2&tree,t9
info){switch(rule.ruletype
iY2
ProduceNewTree:{tree.Become(xQ1
e01
x21
0),info,false)nW2
case
ReplaceParams:default:{yG<i02>list=info.GetMatchedParamIndexes();std::sort(list.i62
list.end())c23
list
yA3;a-->0;)tree.iH1
list[a])xS3
a=0;a<rule.repl_param_count;++a)nG1
nparam=xQ1
e01
x21
a),info,true
tT
yA
nparam);}
nM3}
}
}
#endif
#ifdef DEBUG_SUBSTITUTIONS
#include <sstream>
#include <cstring>
xQ
using
iO2
l41;using
iO2
FPoptimizer_CodeTree;using
iO2
FPoptimizer_Optimize;iO2
l41{xB1
DumpMatch
eX3
lM1&tree,const
t9
info,bool
DidMatch,std::ostream&o){DumpMatch
t81,DidMatch?tZ3"match"
:tZ3"mismatch"
,o);}
xB1
DumpMatch
eX3
lM1&tree,const
t9
info,const
char*eY3,std::ostream&o){static
const
char
ParamHolderNames[][2]={"%"
,"&"
,"x"
,"y"
,"z"
,"a"
,"b"
,"c"
}
;o<<eY3<<" (rule "
<<(&rule-grammar_rules)<<")"
<<":\n  Pattern    : "
;{cG2
tmp;tmp.first=SubFunction;xI
tmp2;tmp2.data=rule
nT2;tmp
eE3=i01
void*)&tmp2;DumpParam
x8(tmp,o);}
o<<"\n  Replacement: "
;DumpParams
x21
rule.repl_param_count,o);o<<"\n"
;o<<"  Tree       : "
;xE3(tree,o);o<<"\n"
;if(!std::strcmp(eY3,tZ3"match"
))DumpHashes(tree,o)c23
0;a<i33
eW2
a){if(!i33[a].iA1
y81
o<<"           "
<<ParamHolderNames[a]<<" = "
;xE3(i33[a],o);o<<"\n"
;}
eW3
info.lQ
eW2
b){if(!eH2
first)y81
for
xK2
0;a<eH2
second
eW2
a){o<<"         <"
<<b<<"> = "
;xE3(eH2
second[a],o);o<<std::endl;}
}
o<<std::flush;}
}
#endif
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
xQ
iO2{xG1
MarkIncompletes(x2){if(tree.Is_Incompletely_Hashed(iF1;bool
i51
i13
for
yS
i51|=MarkIncompletes
nW1
a));if(i51)tree.Mark_Incompletely_Hashed()l81
i51;}
xB1
FixIncompletes(x2){if(tree.Is_Incompletely_Hashed()){for
yS
FixIncompletes
nW1
a)tT
x02;}
}
}
iO2
FPoptimizer_CodeTree{lA
Sort()cX3
Sort();}
lA
Rehash(bool
constantfolding){if(constantfolding)ConstantFolding(*this);else
Sort();data
xB
yV1
l11{c2
e62
eZ3
nP3=0;
#if 0
long
double
value=Value;eB=crc32::calc(i01
i02
char*)&value,xK3
value));key^=(key<<24);
#elif 0
union{e92{i02
char
filler1[16];eS1
v;i02
char
filler2[16];}
buf2;e92{i02
char
filler3[xK3
eS1)+16-x
K3
iC1)];eB;}
buf1;}
data;memset(&data,0,xK3
data));data.buf2.v=Value;eB=data.buf1.key;
#else
int
cE2;eS1
lQ2=std::frexp(Value,&cE2);eB=iL1(cE2+0x8000)&0xFFFF);if(lQ2<0){lQ2=-lQ2;key=key^0xFFFF;}
else
key+=0x10000;lQ2-=yF3;key<<=39;key|=iC1((lQ2+lQ2)*eS1(1u<<31))<<8;
#endif
iU
tM
#ifdef FP_SUPPORT_LONG_INT_TYPE
n73<>l11<long>{c2
long
eZ3
eB=Value;iU
tM
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
n73<>l11<GmpInt>{c2
const
GmpInt&eZ3
eB=Value.toInt();iU
tM
#endif
xB1
nO2
x8::Recalculate_Hash_NoRecursion(){fphash_t
iU(iC1(Opcode)<<56,Opcode*tM3(0x1131462E270012B));Depth=1;switch(Opcode
iY2
cImmed:{ImmedHashGenerator
x8::MakeHash(iU,Value
nW2
case
iE2:{nP3|=eC<<48
eC2((eC)*11)^tM3(0x3A83A83A83A83A0);nM3
case
cFCall:case
cPCall:{nP3|=eC<<48
eC2((~eC)*7)^3456789;}
default:{size_t
eM1=0
c23
0;a<c93
eW2
a){if(c93[a].xT2>eM1)eM1=c93[a].xT2;nP3+=((c93[a]eE2
hash1*(a+1))>>12)eC2
c93[a]eE2
hash1
eC2(3)*tM3(0x9ABCD801357);iU.hash2*=tM3(0xECADB912345)eC2(~c93[a]eE2
hash2)^4567890;}
Depth+=eM1;}
}
if(Hash!=iU){Hash=iU;iK1=0;}
}
lA
FixIncompleteHashes(){MarkIncompletes(*this);FixIncompletes(*this);}
}
#endif
#include <cmath>
#include <list>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
xQ
iO2{using
iO2
FPoptimizer_CodeTree;xG1
x31
lM1&tree,long
count,const
yV2
SequenceOpCode
x8&eQ,yW2&synth,size_t
max_bytecode_grow_length);static
const
e92
SinCosTanDataType{OPCODE
whichopcode;OPCODE
inverse_opcode;enum{nominator,denominator,inverse_nominator,inverse_denominator}
;OPCODE
codes[4];}
SinCosTanData[12]={{cTan,cCot,{cSin,cCos,cCsc,cSec}
}
,{cCot,cCot,{cCos,cSin,cSec,cCsc}
}
,{cCos,cSec,{cSin,cTan,cCsc,cCot}
}
,{cSec,cCos,{cTan,cSin,cCot,cCsc}
}
,{cSin,cCsc,{cCos,cCot,cSec,cTan}
}
,{cCsc,cSin,{cCot,cCos,cTan,cSec}
}
,{yT2{cSinh,cH3
yY2,{cSinh,cNop,{yT2
cNop,cCosh}
}
,{cH3
cNop,{cSinh,yT2
cNop}
}
,{cNop,cTanh,{cH3
cSinh,yY2,{cNop,cSinh,{cNop,cTanh,cH3
cNop}
}
,{cNop,cH3{cTanh,cSinh,yY2}
;}
iO2
FPoptimizer_CodeTree{lA
SynthesizeByteCode(yG<i02>&nN,yG
x8&Immed,size_t&stacktop_max){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Making bytecode for:\n"
;iO
#endif
while(RecreateInversionsAndNegations()){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"One change issued, produced:\n"
;iO
#endif
FixIncompleteHashes();}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Actually synthesizing, after recreating inv/neg:\n"
;iO
#endif
yW2
synth;SynthesizeByteCode(synth,false
eU3.Pull(nN,Immed,stacktop_max);}
lA
SynthesizeByteCode(yW2&synth,bool
MustPopTemps)const{xR1*this)){return;}
for
xK2
0;a<12;++a){const
SinCosTanDataType&data=SinCosTanData[a];if(data.whichopcode!=cNop){if(lM2!=data.whichopcode)y81
yL2
lJ3;lJ3.lI1
lJ3
tU
data.inverse_opcode);lJ3.y12);xR1
lJ3)){synth.l21
else{if(lM2!=cInv)y81
if(GetParam(0)nE!=data.inverse_opcode)y81
xR1
GetParam(0))){synth.l21
size_t
found[4];eW3
4;++b){yL2
tree;if(data.t03]==cNop){tH
cInv);yL2
lK3;lK3.lI1
lK3
tU
data.t03^2]);lK3.y12
tT
yA
lK3);}
else{tree.lI1
tH
data.t03]);}
tree.y12);found[b]t13
e33(tree);}
if(found[data.y22!=tG
i61
yK
y22);lJ1
i61);lK1
cDiv
nL1
y22!=tG
iV
yK
y22);lJ1
iV);lK1
cMul
nL1
lS1!=tG
iV
yK
lS1);lJ1
iV);lK1
cRDiv
nL1
lS1!=tG
i61
yK
lS1);lJ1
i61);lK1
cMul,2,1
eU3.l21
size_t
n_subexpressions_synthesized=SynthCommonSubExpressions(synth);switch(lM2
iY2
iE2:synth.PushVar(GetVar());lC
cImmed:yX2
xJ1);lC
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:{if(lM2==cMul){bool
xU3
i13
c3
lT1
y21&&isLongInteger(lT1.xJ1)){yW1=makeLongInteger(lT1.xJ1);yL2
tmp(*this,typename
yL2::CloneTag());tmp
cP3
tmp
x02;if(x31
tmp,value,yV2
eX1
x8::AddSequence,synth,MAX_MULI_BYTECODE_LENGTH)){xU3=true;nM3}
}
if(xU3)nM3
int
yA1=0;yG<bool>done(GetParamCount(),false);yL2
iE;iE
tU
lM2);for(;;){bool
found
i13
c3
done[a])y81
if(synth.IsStackTop(lT1)){found=true;done[a]=true;lT1.nB
iE
eT
lT1);if(++yA1>1){synth
c4
2);iE.y12
eU3.yZ2
iE);yA1=yA1-2+1;}
}
}
if(!found)nM3
c3
done[a])y81
lT1.nB
iE
eT
lT1);if(++yA1>1){synth
c4
2);iE.y12
eU3.yZ2
iE);yA1=yA1-2+1;}
}
if(yA1==0){switch(lM2
iY2
cAdd:case
cOr:case
cAbsOr:yX2
0);lC
cMul:case
cAnd:case
cAbsAnd:yX2
1);lC
cMin:case
cMax:yX2
0);break;default:nM3++yA1;}
assert(n_stacked==1);nM3
case
cPow:{iQ2
p0
tW2
0);iQ2
p1
tW2
1);if(!p1
y21||!isLongInteger
n03)||!x31
p0,makeLongInteger
n03),yV2
eX1
x8::MulSequence,synth,MAX_POWI_BYTECODE_LENGTH)){p0.nB
p1
t33
c4
2);c91
cIf:case
i03:{typename
yW2::IfData
yJ2;GetParam(0)t33.SynthIfStep1(yJ2,lM2);GetParam(1)t33.SynthIfStep2(yJ2);GetParam(2)t33.SynthIfStep3(yJ2
nW2
case
cFCall:case
cPCall:{for
xK2
0;a<cN3++a)lT1
t33
c4
iL1)GetParamCount());lK1
yM|GetFuncNo(),0,0
nW2
default:{for
xK2
0;a<cN3++a)lT1
t33
c4
iL1)GetParamCount()nW2}
synth.yZ2*this);if(MustPopTemps&&n_subexpressions_synthesized>0){size_t
top
t13
GetStackTop(eU3.DoPopNMov(top-1-n_subexpressions_synthesized,top-1);}
}
}
iO2{xG1
x31
lM1&tree,long
count,const
yV2
SequenceOpCode
x8&eQ,yW2&synth,size_t
max_bytecode_grow_length){if
cR3!=0){yW2
backup=synth;tree.nB
size_t
bytecodesize_backup
t13
GetByteCodeSize();yV2
x31
count
eR2
size_t
bytecode_grow_amount
t13
GetByteCodeSize()-bytecodesize_backup;if(bytecode_grow_amount>max_bytecode_grow_length){synth=backup
l81
false;}
return
true;}
else{yV2
x31
count,eQ,synth)nS2}
}
#endif
#include <cmath>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
xQ
iO2{using
iO2
FPoptimizer_CodeTree;
#define FactorStack yG
const
e92
PowiMuliType{i02
opcode_square;i02
opcode_cumulate;i02
opcode_invert;i02
opcode_half;i02
opcode_invhalf;}
iseq_powi={cSqr,cMul,cInv,cSqrt,cRSqrt}
,iseq_muli={i21,cAdd,cNeg,i21,i21}
;yB1
cF1
const
PowiMuliType&xV3,const
yG<i02>&nA2,l92&stack){eS1
cY3
1);while(IP<limit){if(xW3
xV3.opcode_square){if(!eY2
cZ3
2;e4
opcode_invert){result=-result;e4
opcode_half){if(result>eS1(0)&&isEvenInteger(cZ3
yF3;e4
opcode_invhalf){if(result>eS1(0)&&isEvenInteger(cZ3
eS1(-0.5);++IP;y81}
size_t
nB2=IP;eS1
lhs(1);if(xW3
cFetch){lC1=yL3;if(index<y1||size_t(index-y1)>=stack
yA3){IP=nB2;nM3
lhs=stack[index-y1];goto
y32;}
if(xW3
cDup){lhs=result;goto
y32;y32:y63
result);++IP;eS1
subexponent=cF1
xV3
lQ1
if(IP>=limit||nN[IP]!=xV3.opcode_cumulate){IP=nB2;nM3++IP;stack.pop_back();result+=lhs*subexponent;y81}
nM3
return
result;}
yB1
ParsePowiSequence
i01
yG<i02>&nA2){l92
stack;y63
eS1(1))l81
cF1
iseq_powi
lQ1}
yB1
ParseMuliSequence
i01
yG<i02>&nA2){l92
stack;y63
eS1(1))l81
cF1
iseq_muli
lQ1}
iP2
CodeTreeParserData{e13
iF2
CodeTreeParserData(bool
k_powi):stack(),clones(),keep_powi(k_powi){}
void
Eat(size_t
eR3,OPCODE
opcode
xX3;xK
tU
opcode);yG
nV1
cB2=Pop(eR3);xK
tI1
cB2);if(!keep_powi)switch(opcode
iY2
cTanh:nG1
sinh,cosh;sinh
tU
cSinh);sinh
eT
xK
xY3
sinh
x02;cosh
tU
cCosh);cosh
yA
xK
xY3
cosh
i72
pow
yB2
yA
cosh);pow
yT
eS1(-1)));pow
x02;xK
tU
y13.nC1
0,sinh);xK
yA
pow
nW2
case
cTan:nG1
sin,cos;sin
tU
cSin);sin
eT
xK
xY3
sin
x02;cos
tU
cCos);cos
yA
xK
xY3
cos
i72
pow
yB2
yA
cos);pow
yT
eS1(-1)));pow
x02;xK
tU
y13.nC1
0,sin);xK
yA
pow
nW2
case
cPow:{lM1&p0=xK
lD
0);lM1&p1=xK
lD
1);if(p1
nE==cAdd){yG
nV1
n83(p1.GetParamCount())c23
0;a<p1.cN3++a)nG1
pow
yB2
eT
p0);pow
eT
p1
lD
a));pow
x02;n83[a
t23
pow);}
xK
tU
y13
tI1
n83);}
nM3
default:nM3
xK.Rehash(!keep_powi);i71,false);
#ifdef DEBUG_SUBSTITUTIONS
lC2<<eR3<<", "
<<FP_GetOpcodeName(opcode)<<"->"
<<FP_GetOpcodeName(xK
nE)<<": "
tP3
xK)tW
xK);
#endif
y63
xK
iR2
EatFunc(size_t
eR3,OPCODE
opcode,i02
funcno
xX3=CodeTreeFuncOp
x8(opcode,funcno);yG
nV1
cB2=Pop(eR3);xK
tI1
cB2);xK.y12);
#ifdef DEBUG_SUBSTITUTIONS
lC2<<eR3<<", "
tP3
xK)tW
xK);
#endif
i71);y63
xK
iR2
AddConst(yF1
xX3=CodeTreeImmed(value);i71);Push(xK
iR2
AddVar
iL1
varno
xX3=CodeTreeVar
x8(varno);i71);Push(xK
iR2
SwapLastTwoInStack(){y73
1
t23
y73
2]iR2
Dup(){Fetch
lB2
1
iR2
Fetch(size_t
which){Push(stack[which]);}
nT1
T>void
Push(T
tree){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<tP3
tree)tW
tree);
#endif
y63
tree
iR2
PopNMov(size_t
target,size_t
source){stack[target]=stack[source];stack
nE3
target+1);}
yK2
y42{clones.clear()yJ
cY3
stack.back());stack.resize
lB2
1)l81
result;}
yG
nV1
Pop(size_t
n_pop){yG
nV1
cY3
n_pop)xS3
n=0;n<n_pop;++n)result[n
t23
y73
n_pop+n]);
#ifdef DEBUG_SUBSTITUTIONS
for(xL3=n_pop;n-->0;){lC2;xE3(result[n])tW
result[n]);}
#endif
stack.resize
lB2
n_pop)l81
result;}
size_t
GetStackTop(n61
stack
yA3;}
private:void
FindClone(yK2&,bool=true){return;}
private:yG
nV1
stack;std::multimap<fphash_t,yK2>clones;bool
keep_powi;private:CodeTreeParserData
i01
CodeTreeParserData&);CodeTreeParserData&e31=i01
CodeTreeParserData&);}
;yT1
IfInfo
nG1
cU2
yJ
thenbranch;size_t
endif_location;IfInfo():cU2(),thenbranch(),endif_location(){}
}
;}
iO2
FPoptimizer_CodeTree{lA
GenerateFrom
i01
yG<i02>&nN,const
yG
x8&Immed,const
typename
FunctionParserBase
x8::Data&cJ3,bool
keep_powi){yG
nV1
nK2;nK2.nH3
cJ3.mVariablesAmount)xS3
n=0;n<cJ3.mVariablesAmount;++n){nK2
yL
CodeTreeVar
x8(n+iE2));}
GenerateFrom(nN,Immed,cJ3,nK2,keep_powi);}
lA
GenerateFrom
i01
yG<i02>&nN,const
yG
x8&Immed,const
typename
FunctionParserBase
x8::Data&cJ3,const
i22
nK2,bool
keep_powi){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"ENTERS GenerateFrom()\n"
;
#endif
CodeTreeParserData
x8
sim(keep_powi);yG<IfInfo
x8>eN;for(size_t
IP=0,DP=0;;++IP){tO2:while(!eN
cQ3&&(eN.eD==IP||(IP<nN
yA3&&xW3
cJump&&eN.e11.iA1)){yL2
elsebranch=sim.y42
yG3
eN.back().cU2)yG3
eN.e11)yG3
elsebranch);iJ
3,cIf);eN.pop_back();}
if(IP>=nN
yA3)break;i02
opcode=nN[IP];if((opcode==cSqr||opcode==cDup||(opcode==cInv&&!IsIntType
x8::result)||opcode==cNeg||opcode==cSqrt||opcode==cRSqrt||opcode==cFetch)){size_t
was_ip=IP;eS1
cE2=ParsePowiSequence
x8(nN,IP,eN
cQ3?nN
yA3:eN.eD,sim.x5
1);if(cE2!=1.0){x9
cE2)yJ3;goto
tO2;}
if(opcode==cDup||opcode==cFetch||opcode==cNeg){eS1
xD2=ParseMuliSequence
x8(nN,IP,eN
cQ3?nN
yA3:eN.eD,sim.x5
1);if(xD2!=1.0){x9
xD2)yE
cMul);goto
tO2;}
}
IP=was_ip;}
if(lD2>=iE2){sim.Push(nK2[opcode-iE2]);}
else{switch(lD2
iY2
cIf:case
i03:{eN
nE3
eN
yA3+1);yL2
res(sim.y42);eN.back().cU2.swap(res);eN.eD=nN
yA3;IP+=2;y81}
case
cJump:{yL2
res(sim.y42);eN.e11.swap(res);eN.eD=nN[IP+1]+1;IP+=2;y81}
case
cImmed:x9
Immed[DP++]);lC
cDup:sim.Dup();lC
cNop:lC
cFCall:{i02
funcno=yL3;assert(funcno<fpdata.mFuncPtrs.size());i02
cB2=cJ3.mFuncPtrs
yD3
mParams;sim.EatFunc(cB2,lD2,funcno
nW2
case
cPCall:{i02
funcno=yL3;assert(funcno<fpdata.tQ3.size());const
FunctionParserBase
x8&p=*cJ3.tQ3
yD3
mParserPtr;i02
cB2=cJ3.tQ3
yD3
mParams;yG<yL2>paramlist=sim.Pop(cB2);yL2
tP2;tP2.GenerateFrom(p.mData->mByteCode,p.mData->mImmed,*p.mData,paramlist)yG3
tP2
nW2
case
cInv:x9
1
nY2
cDiv);lC
cNeg:yE3
cNeg);break;x9
0
nY2
cSub);lC
cSqr:x9
2
e21
cSqrt:x9
yF3
e21
cRSqrt:x9
eS1(-0.5)e21
cCbrt:x9
eS1(1)/eS1(3)e21
cDeg:x9
fp_const_rad_to_deg
x8
cI1
cRad:x9
fp_const_deg_to_rad
x8
cI1
cExp:iF)goto
yI3;x9
fp_const_e
x8()nY2
cPow);lC
cExp2:iF)goto
yI3;x9
2.0
nY2
cPow);lC
cCot:yE3
cTan);iF)nY
cCsc:yE3
cSin);iF)nY
cSec:yE3
cCos);iF)nY
cInt:
#ifndef __x86_64
iF){yE3
cInt
nW2
#endif
x9
yF3)lL3
yE3
cFloor);lC
cLog10:yE3
yH3
fp_const_log10inv
x8
cI1
cLog2:yE3
yH3
fp_const_log2inv
x8
cI1
cLog2by:cK3
yE3
yH3
fp_const_log2inv
x8());iJ
3,cMul);lC
cHypot:x9
2)yJ3;cK3
x9
2)yJ3
lL3
x9
yF3
e21
cSinCos:sim.Dup();yE3
cSin);cK3
yE3
cCos);lC
cRSub:cK3
case
cSub:iF){iJ
2,cSub
nW2
x9-1)yE
cMul)lL3
lC
cRDiv:cK3
case
cDiv:iF||IsIntType
x8::result){iJ
2,cDiv
nW2
x9-1)yJ3
yE
cMul);lC
cAdd:case
cMul:case
cMod:case
cPow:case
cEqual:case
cLess:case
cGreater:case
t71:case
cLessOrEq:case
cGreaterOrEq:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:iJ
2,xS1
lC
cNot:case
cNotNot:case
yR3:case
cAbsNotNot:yE3
xS1
lC
cFetch:sim.Fetch(yL3);lC
cPopNMov:{i02
stackOffs_target=yL3;i02
stackOffs_source=yL3;sim.PopNMov(stackOffs_target,stackOffs_source
nW2
#ifndef FP_DISABLE_EVAL
case
cEval:{size_t
paramcount=cJ3.mVariablesAmount;iJ
paramcount,xS1
nM3
#endif
default:yI3:;i02
funcno=opcode-cAbs;assert(funcno<FUNC_AMOUNT);const
FuncDefinition&func=Functions[funcno];iJ
func.cB2,xS1
nM3}
}
Become(sim.y42);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Produced tree:\n"
;iO
#endif
}
}
#endif
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
#include <assert.h>
#define FP_MUL_COMBINE_EXPONENTS
iO2{xQ
using
iO2
FPoptimizer_CodeTree;yV1
static
void
AdoptChildrenWithSameOpcode(eR{
#ifdef DEBUG_SUBSTITUTIONS
bool
lR2
i13
#endif
for
xU
if
nW1
a)nE==tree
nE){
#ifdef DEBUG_SUBSTITUTIONS
if(!lR2){std::cout<<"Before assimilation: "
eS
lR2=true;}
#endif
tree.AddParamsMove
nW1
a).GetUniqueRef().iE1),a);}
#ifdef DEBUG_SUBSTITUTIONS
if(lR2){std::cout<<"After assimilation:   "
eS}
#endif
}
}
iO2
FPoptimizer_CodeTree{xB1
ConstantFolding(eR{tree.Sort();
#ifdef DEBUG_SUBSTITUTIONS
void*yM3=0
lN1"["
<<(&yM3)<<"]Runs ConstantFolding for: "
eS
DumpHashes(tree);
#endif
if(false){redo:;tree.Sort();
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"["
<<(&yM3)<<"]Re-runs ConstantFolding: "
eS
DumpHashes(tree);
#endif
}
if
xU2!=cImmed){range
x8
p=iM
tree);if(p
y41
p
i0&&p.min==p.max)lL
p.min)n5}
if(false){ReplaceTreeWithOne
xP3
ReplaceWithImmed(eS1(1));goto
do_return;ReplaceTreeWithZero
xP3
ReplaceWithImmed(eS1(0));goto
do_return;ReplaceTreeWithParam0:
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before replace: "
lN1
std::hex<<'['<<tree
eE2
hash1<<','<<tree
eE2
hash2<<']'<<std::dec
eS
#endif
tree.xO2
0));
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After replace: "
lN1
std::hex<<'['<<tree
eE2
hash1<<','<<tree
eE2
hash2<<']'<<std::dec
eS
#endif
e5
yN3
xU2
iY2
cImmed:lC
iE2:lC
cAnd:case
cAbsAnd
cP
bool
cE
i13
for
xU{if(!y93
a)))cE=true;yQ3
a),cV2
cAbsAnd)iY2
yO3:yP3
IsAlways:nF1);lC
lX1
yN3(iT
iY2
0
tY
1:tH
cV2
cAnd?cNotNot:cAbsNotNot);e5
default:if
xU2==cAnd||!cE)if(ConstantFolding_AndLogic
y83
c91
cOr:case
cAbsOr
cP
bool
cE
i13
for
xU{if(!y93
a)))cE=true;yQ3
a),cV2
cAbsOr)iY2
IsAlways
tY
yO3:nF1);lC
lX1
yN3(iT
iY2
0:yP3
1:tH
cV2
cOr?cNotNot:cAbsNotNot);e5
default:if
xU2==cOr||!cE)if(ConstantFolding_OrLogic
y83
c91
cNot:case
yR3:{i02
n21
0;switch
nW1
0)nE
iY2
cEqual:n21
t71;lC
t71:n21
cEqual;lC
cLess:n21
cGreaterOrEq;lC
cGreater:n21
cLessOrEq;lC
cLessOrEq:n21
cGreater;lC
cGreaterOrEq:n21
cLess;lC
cNotNot:n21
cNot;lC
cNot:n21
cNotNot;lC
yR3:n21
cAbsNotNot;lC
cAbsNotNot:n21
yR3;break;default:nM3
if(opposite){tH
OPCODE(opposite)tT.SetParamsMove
nW1
0).GetUniqueRef().iE1));e5
yN3(tX
0),tree
cG1)tF1
yP3
yO3
tY
lX1
if
xU2==cNot&&GetPositivityInfo
nW1
0))==IsAlways)tH
yR3);l23
nE==cIf||e72
nE==i03)nG1
iH2
cL2
0);lM1&ifp1=iH2
lD
1);lM1&ifp2=iH2
lD
2);if(ifp1
nE==cNot||ifp1
cG1{tree.x1
ifp1
nE==cNot?cNotNot:cAbsNotNot);tQ2
lD
0))yC1)yS3
xQ2
yT3)tV
if(ifp2
nE==cNot||ifp2
cG1{tree.x1
tree
nE);tQ2)yC1)yS3
tU
ifp2
nE==cNot?cNotNot:cAbsNotNot);yT3
lD
0))tV
c91
cNotNot:case
cAbsNotNot:{if(y93
0)))lN3
yQ3
0),cV2
cAbsNotNot)iY2
yO3:yP3
IsAlways
tY
lX1
if
xU2==cNotNot&&GetPositivityInfo
nW1
0))==IsAlways)tH
cAbsNotNot);l23
nE==cIf||e72
nE==i03)nG1
iH2
cL2
0);lM1&ifp1=iH2
lD
1);lM1&ifp2=iH2
lD
2);if(ifp1
nE==cNot||ifp1
cG1{tree.SetParam(0,iH2
lD
0)tT
eT
ifp1)yS3
xQ2
yT3)tV
if(ifp2
nE==cNot||ifp2
cG1{tree.x1
tree
nE);tQ2)yC1
tT
eT
ifp2);tH
iH2
nE);e5}
c91
cIf:case
i03:{if(ConstantFolding_IfOperations
y83
nM3
case
cMul:{NowWeAreMulGroup:;AdoptChildrenWithSameOpcode(tree);eS1
nN1=eS1(1);size_t
i81=0;bool
nO1=false
eV3
if(!xI2
y21)y81
eS1
immed=xI2.xJ1;if(immed==eS1(0))tZ
nN1*=immed;++i81;}
if(i81>1||(i81==1&&fp_equal(nN1,nY3)nO1=true;if(nO1){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"cMul: Will add new "
tR3
nN1<<"\n"
;
#endif
for
xU
if
nW1
a)y21){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<" - For that, deleting "
tR3
xI2.xJ1
lN1"\n"
;
#endif
lM3!fp_equal(nN1,nY3
tree
eT
cU1
nN1));yN3(iT
iY2
0
tY
1:lN3
default:if(ConstantFolding_MulGrouping
y83
if(ConstantFolding_MulLogicItems
y83
c91
cAdd
cP
eS1
lE2=0.0;size_t
i81=0;bool
nO1=false
eV3
if(!xI2
y21)y81
eS1
immed=xI2.xJ1;lE2+=immed;++i81;}
if(i81>1||(i81==1&&lE2==0.0))nO1=true;if(nO1){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"cAdd: Will add new "
tR3
lE2<<"\n"
lN1"In: "
eS
#endif
for
xU
if
nW1
a)y21){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<" - For that, deleting "
tR3
xI2.xJ1
lN1"\n"
;
#endif
lM3!(lE2==0.0))tree
eT
cU1
lE2));yN3(iT
iY2
0:yP3
1:lN3
default:if(ConstantFolding_AddGrouping
y83
if(ConstantFolding_AddLogicItems
y83
c91
cMin
cP
size_t
y52=0;range
x8
e6
eV3
while(a+1<iT&&xI2
iA
tree
lD
a+1)))nF1+1);range<nI
has_max&&(!e6
i0||(p.max)<e6.max)){e6.max=p.max;e6
i0=true;y52=a;}
}
if(e6
i0)for
xU{range<nI
iV2&&a!=y52&&p.min>=e6.max)lM3
iT==1){lN3
c91
cMax
cP
size_t
y52=0;range
x8
t1
eV3
while(a+1<iT&&xI2
iA
tree
lD
a+1)))nF1+1);range<nI
iV2&&(!t1.iV2||p.min>t1.min)){t1.min=p.min;t1.iV2=true;y52=a;}
}
if(t1.iV2){for
xU{range<nI
has_max&&a!=y52&&(p.max)<t1.min){nF1);}
}
}
if(iT==1){lN3
c91
cEqual:case
t71:case
cLess:case
cGreater:case
cLessOrEq:case
cGreaterOrEq:if(ConstantFolding_Comparison
y83
lC
cAbs:{range
x8
p0=y6
0));if(p0
eN1
lN3
if(p0
i0
yU3
tN{tH
cMul
tT
yT
nY3;goto
NowWeAreMulGroup;}
l23
nE==cMul){lM1&p
cL2
0);yG
nV1
lO3;yG
nV1
c02
c23
0;a<p.cN3++a){p0=iM
p
lD
a));if(p0
eN1{lO3
yL
p
lD
a));}
if(p0
i0
yU3
tN{c02
yL
p
lD
a));}
}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Abs: mul group has "
<<lO3
yA3<<" pos, "
<<c02
yA3<<"neg\n"
;
#endif
if(!lO3
cQ3||!c02
cQ3){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"AbsReplace-Before: "
;xE3(tree)lN1"\n"
<<std::flush;DumpHashes
tJ1;
#endif
yK2
cL3;cL3
tU
cMul)c23
0;a<p.cN3++a){p0=iM
p
lD
a));if((p0
eN1||(p0
i0
yU3
tN){}
else
cL3
eT
p
lD
a));}
cL3
i72
lP3;lP3
tU
cAbs);lP3
yA
cL3);lP3
i72
xW1
cMul);n83
yA
lP3);xX1
AddParamsMove(lO3);if(!c02
cQ3){if(c02
yA3%2)n83
yT
eS1(-1)));xX1
AddParamsMove(c02);}
tree.Become(n83);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"AbsReplace-After: "
;xE3
tJ1
lN1"\n"
<<std::flush;DumpHashes
tJ1;
#endif
goto
NowWeAreMulGroup;}
}
nM3
#define HANDLE_UNARY_CONST_FUNC(funcname) lX)lL funcname(n6))n5
case
cLog:tH3(fp_log);l23
nE==cPow)nG1
pow
cL2
0);if(GetPositivityInfo(pow
lD
0))==IsAlways){pow
lF2
tI
tree.lW
if(GetEvennessInfo(pow
x03==IsAlways){pow
lF2
yJ
abs;abs
tU
cAbs);abs
yA
pow
xY3
abs
x02
tI
pow.nC1
0,abs
tT.lW}
else
l23
nE==cAbs)nG1
pow
cL2
0)lD
0);if(pow
nE==cPow){pow
lF2
yJ
abs;abs
tU
cAbs);abs
yA
pow
xY3
abs
x02
tI
pow.nC1
0,abs
tT.lW}
lC
cAcosh:tH3(fp_acosh);lC
cAsinh:tH3(fp_asinh);lC
cAtanh:tH3(fp_atanh);lC
cAcos:tH3(fp_acos);lC
cAsin:tH3(fp_asin);lC
cAtan:tH3(fp_atan);lC
cCosh:tH3(fp_cosh);lC
cSinh:tH3(fp_sinh);lC
cTanh:tH3(fp_tanh);lC
cSin:tH3(fp_sin);lC
cCos:tH3(fp_cos);lC
cTan:tH3(fp_tan);lC
cCeil:if(n8
tH3(fp_ceil);lC
cTrunc:if(n8
tH3(fp_trunc);lC
cFloor:if(n8
tH3(fp_floor);lC
cInt:if(n8
tH3(fp_int);lC
cCbrt:tH3(fp_cbrt);lC
cSqrt:tH3(fp_sqrt);lC
cExp:tH3(fp_exp);lC
cLog2:tH3(fp_log2);lC
cLog10:tH3(fp_log10);lC
cLog2by:y2)lL
fp_log2(n6)*y4
nN3
cMod:y2)lL
fp_mod(n6,y4)nN3
cAtan2:{range
x8
p0
y5
p1=y6
1));lX&&fp_equal(n6,eS1(0))){if(p1
i0&&(p1.max)<0)lL
fp_const_pi
x8())n5
if(p1
y41
p1.min>=0.0)lL
eS1(0))n5}
if(nE1
fp_equal(y4,eS1(0))){if(p0
i0&&(p0.max)<0)lL-fp_const_pihalf
x8())n5
if(p0
y41
p0.min>0)lL
fp_const_pihalf
x8())n5}
y2)lL
fp_atan2(n6,y4))n5
if((p1
y41
p1.min>0.0)||(p1
i0&&(p1.max)<fp_const_negativezero
x8()))nG1
y62;y62
tU
cPow);y62
yA
tree
x03;y62
yT
eS1(-1)));y62
i72
y72;y72
e53
y72
yA
e72);y72
yA
y62);y72
y3
cAtan
tT.nC1
0,y72
tW1
1);c91
cPow:{if(ConstantFolding_PowOperations
y83
nM3
case
cDiv:y2&&y4!=0.0)lL
n6/y4
nN3
cInv:lX&&n6!=0.0)lL
eS1(1)/n6
nN3
cSub:y2)lL
n6-y4
nN3
cNeg:lX)lL-n6
nN3
cRad:lX)lL
RadiansToDegrees
t63
cDeg:lX)lL
DegreesToRadians
t63
cSqr:lX)lL
n6*n6
nN3
cExp2:tH3(fp_exp2);lC
cRSqrt:lX)lL
eS1(1)/fp_sqrt
t63
cCot:lX)nQ3=fp_tan(n6)xE
cSec:lX)nQ3=fp_cos(n6)xE
cCsc:lX)nQ3=fp_sin(n6)xE
cHypot:y2)lL
fp_hypot(n6,y4)nN3
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cNop:case
cJump:lC
cPCall:case
cFCall:case
cEval:nM3
do_return:;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"["
<<(&yM3)<<"]Done ConstantFolding, result: "
eS
DumpHashes(tree);
#endif
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
xQ
using
iO2
FPoptimizer_CodeTree;iO2{n73<i02
cY2
e92
Comp{}
;n73<>e92
Comp<cLess>{n73<lF<yU
cLessOrEq>{n73<lF<=yU
cGreater>{n73<lF>yU
cGreaterOrEq>{n73<lF>=yU
cEqual>{n73<lF==yU
t71>{n73<lF!=b;}
}
;}
iO2
FPoptimizer_CodeTree{c12
set_abs(){if(!iV2&&!iI2{iV2=yV3}
tP1!iV2&&max
t83){iV2=true;min=-max;iJ2
false;}
tP1!iV2){iV2=yV3
iJ2
false;}
tP1
min
cH1
nZ2;tP1!iI2{iV2=yV3}
tP1
max
t83)nQ3(-max);max=-min;min=tmp;}
tP1-min>=max){max=-min;n31}
else{n31}
}
c12
set_neg(){std::swap(iV2,iI2;std::swap(min,max);min=-min;max=-max;}
yV
set_min_if(c31
if(iV2&&Comp<cY2()(min,v))min=func(min
t73
iV2=yW3
iV2;min=yW3
min;}
}
yV
set_max_if(c31
if(has_max&&Comp<cY2()(max,v))max=func(max
t73
iJ2
model
i0;max=yW3
max;}
}
yV
set_min_max_if(c31
set_min_if<cY2(v,func,model);set_max_if<cY2(v,func,model);}
c12
set_min(n7
if(iV2)min=func(min
t73
iV2=yW3
iV2;min=yW3
min;}
}
c12
set_max(n7
if(iI2
max=func(max
t73
iJ2
model
i0;max=yW3
max;}
}
c12
y82
n7
set_min(func,model);set_max(func,model);}
yV1
range
x8
iM
const
eR
#ifdef DEBUG_SUBSTITUTIONS_extra_verbose
{range
x8
tmp=CalculateResultBoundaries_do(tree)lN1"Estimated boundaries: "
;if(tmp.iV2)std::cout<<tmp.min;else
std::cout<<"-inf"
lN1" .. "
;if(tmp
i0)std::cout<<tmp.max;else
std::cout<<"+inf"
lN1": "
;xE3(tree)lN1
std::endl
l81
tmp;}
yV1
range
x8
yK2::CalculateResultBoundaries_do
i01
eR
#endif
{x81
eO1(-fp_const_pihalf
x8(),fp_const_pihalf
x8());x81
pi_limits(-fp_const_pi
x8(),fp_const_pi
x8());x81
abs_pi_limits(eS1(0),fp_const_pi
x8());using
iO2
std;switch
xU2
iY2
cImmed:nL
yZ3,yZ3);case
cAnd:case
cAbsAnd:case
cOr:case
cAbsOr:case
cNot:case
yR3:case
cNotNot:case
cAbsNotNot:case
cEqual:case
t71:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:e83
0),eS1(1))cM3
cAbs:lB
m.set_abs(cY
cLog:lB
m.y7
fp_log
cY
cLog2:lB
m.y7
fp_log2
cY
cLog10:lB
m.y7
fp_log10
cY
cAcosh:lB
m
nL2
set_min_max_if<cGreaterOrEq
lQ3
fp_acosh
cY
cAsinh:lB
m.y82
fp_asinh
cY
cAtanh:lB
m
nL2
set_min_if<cGreater>(eS1(-1),fp_atanh);m
nL2
set_max_if<cLess
lQ3
fp_atanh
cY
cAcos:lB
nL(m
i0&&(m.max)<eS1(1))?fp_acos(m.max):eS1(0),(m
y41(m.min)>=eS1(-1))?fp_acos(m.min):fp_const_pi
x8())cM3
cAsin:lB
m
nL2
set_min_if<cGreater>(eS1(-1),fp_asin,eO1);m
nL2
set_max_if<cLess
lQ3
fp_asin,eO1
cY
cAtan:lB
m.y82
fp_atan,eO1
cY
cAtan2:{range
x8
p0
y5
p1=y6
1));lX&&fp_equal(n6,eS1(0))){return
abs_pi_limits;}
if(nE1
fp_equal(y4,eS1(0))){return
eO1;}
return
pi_limits
cM3
cSin:lB
bool
x41=!m.iV2||!m
i0||(m.max-m.min)>=(y8
x41)e0
eS1
min=cZ2
min,y8
min
t83)min
c5
eS1
max=cZ2
max,y8
max
t83)max
c5
if(max<min)max
c5
bool
xT1=(min<=fp_const_pihalf
x8()&&max>=fp_const_pihalf
x8());bool
nP1=(min<=cK&&max>=cK);if(xT1&&nP1)e0
if(nP1)nL
eS1(-1),lS2
if(xT1)nL
y92
eS1(1));nL
y92
lS2}
case
cCos:lB
if(m.iV2)m.min+=fp_const_pihalf
x8();if(m
i0)m.max+=fp_const_pihalf
x8();bool
x41=!m.iV2||!m
i0||(m.max-m.min)>=(y8
x41)e0
eS1
min=cZ2
min,y8
min
t83)min
c5
eS1
max=cZ2
max,y8
max
t83)max
c5
if(max<min)max
c5
bool
xT1=(min<=fp_const_pihalf
x8()&&max>=fp_const_pihalf
x8());bool
nP1=(min<=cK&&max>=cK);if(xT1&&nP1)e0
if(nP1)nL
eS1(-1),lS2
if(xT1)nL
y92
eS1(1));nL
y92
lS2}
case
cTan:{nL)cM3
cCeil:lB
m.c41
cFloor:lB
m.eP1
cY
cTrunc:lB
m.eP1);m.c41
cInt:lB
m.eP1);m.c41
cSinh:lB
m.y82
fp_sinh
cY
cTanh:lB
m.y82
fp_tanh,range
x8(eS1(-y03
cY
cCosh:lB
if(m.iV2){if(m
i0){if(m.min
cH1&&m.max
cH1){m.min
cM}
tP1(m.min)t83&&m.max
cH1)nQ3
cM
if(tmp>m.max)m.max=tmp;m.min=eS1(1);}
else{m.min
cM
std::swap(m.min,m.max);}
}
else{if(m.min
cH1){m
nM2
m.min=fp_cosh(m.min);}
else{m
nM2
m.min=eS1(1);}
}
}
else{m.iV2=true;m.min=eS1(1);if(m
i0){m.min=fp_cosh(m.max);m
nM2}
else
m
nM2}
return
m
cM3
cIf:case
i03:{range
x8
res1=y6
1));range
x8
res2=y6
2));if(!res2.iV2)res1.iV2
i13
tP1
res1
y41(res2.min)<res1.min)res1.min=res2.min;if(!res2
i0)res1
nM2
tP1
res1
i0&&(res2.max)>res1.max)res1.max=res2.max
l81
res1
cM3
cMin:{bool
iG
i13
bool
iH
i13
tB1;x4
m
xO3!m.iV2)iG=true;yP1.iV2||(m.min)<i11)i11=m.min;if(!m
i0)iH=true;yP1
i0||(m.max)<e63)e63=m.max;}
if(iG)c42
iH)tE1
l81
yA2
cMax:{bool
iG
i13
bool
iH
i13
tB1;x4
m
xO3!m.iV2)iG=true;yP1.iV2||m.min>i11)i11=m.min;if(!m
i0)iH=true;yP1
i0||m.max>e63)e63=m.max;}
if(iG)c42
iH)tE1
l81
yA2
cAdd:{tB1(eS1(0),eS1(0));x4
item
xO3
item.iV2)i11+=item.min;else
c42
item
i0)e63+=item.max;else
tE1;if(!result
y41!e02)nM3
if(result
y41
e02&&i11>e63)std::swap(i11,e63)l81
yA2
cMul:{e92
Value{enum
lR3{tR2,i91,PlusInf}
;lR3
eF;eS1
value;Value(lR3
t):eF(t),value(0){}
Value(eS1
v):eF(tR2),value(v){}
bool
c22
n61
eF==i91||(eF==tR2&&value
t83
iR2
e31*=i01
Value&rhs){if(eF==tR2&&rhs.eF==tR2)value*=rhs.value;else
eF=(c22)!=rhs.c22)?i91:PlusInf);}
i12<i01
Value&rhs
n61(eF==i91&&rhs.eF!=i91)||(eF==tR2&&(rhs.eF==PlusInf||(rhs.eF==tR2&&value<rhs.value)));}
}
;e92
yD1{Value
yC2,yD2;yD1():yC2(Value::PlusInf),yD2(Value::i91){}
void
nN2
Value
yX3,const
Value&value2){yX3*=value2;if(yX3<yC2)yC2=yX3;if(yD2<yX3)yD2=yX3;}
}
;tB1(eS1(y03;x4
item
xO3!item
y41!item
i0)nL);Value
lS3=c32?Value(i11):nC2
i91);Value
lT3=e02?Value(e63):nC2
PlusInf);Value
lU3=item.iV2?Value(item.min):nC2
i91);Value
lV3=item
i0?Value(item.max):nC2
PlusInf);yD1
range;range.nN2
lS3,lU3
eN2
lS3,lV3
eN2
lT3,lU3
eN2
lT3,lV3);if(range.yC2.eF==Value::tR2)i11=range.yC2.value;else
c42
range.yD2.eF==Value::tR2)e63=range.yD2.value;else
tE1;if(!result
y41!e02)nM3
if(result
y41
e02&&i11>e63)std::swap(i11,e63)l81
yA2
cMod:{range
x8
x
y5
y=y6
1));if(y
i0){if(y.max
cH1){if(!x.iV2||(x.min)<0)nL-y.max,y.max);else
nL
eS1(0),y.max);}
else{if(!x
i0||(x.max)>=0)nL
y.max,-y.max);else
nL
y.max,fp_const_negativezero
x8());}
}
else
nL)cM3
cPow:{if(nE1
y4==eS1(0))e83
y03;}
lX&&n6==eS1(0))e83
0),eS1(0));}
lX&&fp_equal(n6,nY3
e83
y03;}
if(nE1
y4>0&&GetEvennessInfo
nW1
1))==IsAlways)c21
y4;range
x8
tmp
y5
result;c32=true;i11=0;if(tmp
y41
tmp.min>=0)i11=eG3
tmp.min,cE2);tP1
tmp
i0&&tmp.max<=0)i11=eG3
tmp.max,cE2);tE1;if(tmp
y41
tmp
i0){e02=true;e63=std::max(fp_abs(tmp.min),fp_abs(tmp.max));e63=eG3
e63,cE2);}
return
result;}
range
x8
p0
y5
p1=y6
1
eO2
p0_positivity=(p0
y41(p0.min)cH1)?IsAlways:(p0
i0&&(p0.max)t83?yO3:Unknown);TriTruthValue
c52=GetEvennessInfo
nW1
1
eO2
t2=Unknown;switch(p0_positivity)tF1
t2=IsAlways;lC
yO3:{t2=c52;nM3
default:switch(c52)tF1
t2=IsAlways;lC
yO3:lC
Unknown:{if(nE1!eY2
y4)&&y4
cH1){t2=IsAlways;}
nM3}
yN3(t2)tF1{eS1
n31
if(p0
y41
p1.iV2){min=eG3
p0.min,p1.min);if(p0.min
t83&&(!p1
i0||p1.max
cH1)&&min
cH1)n31}
if(p0
y41
p0.min
cH1&&p0
i0&&p1
i0){eS1
max=eG3
p0.max,p1.max);if(min>max)std::swap(min,max);nL
min,max);}
nL
min,false)cM3
yO3:{nL
false,fp_const_negativezero
x8());}
default:{nM3
c91
cNeg:lB
m.set_neg(cY
cSub
x7
cNeg
y23
1
y43
cAdd);y53;tmp
yY3
l81
lH
cInv:lV-1
e12
cDiv
x7
cInv
y23
1
y43
xU1
yY3
l81
lH
cRad:{cF
xU1
yT
fp_const_rad_to_deg
x8(e12
cDeg:{cF
xU1
yT
fp_const_deg_to_rad
x8(e12
cSqr:lV
2
e12
cExp:{cF
cPow);tmp
yT
fp_const_e
x8()));y53
l81
lH
cExp2:{cF
cPow);tmp
yT
nR3
y53
l81
lH
cCbrt:lB
m.y82
fp_cbrt
cY
cSqrt:lB
if(m.iV2)m.min=(m.min)<0?0:fp_sqrt(m.min);if(m
i0)m.max=(m.max)<0?0:fp_sqrt(m.max
cY
cRSqrt:lV-0.5
e12
cHypot:nG1
xsqr,ysqr,add,sqrt;xsqr
yC
0));xsqr
yT
nR3
ysqr
yC
1));ysqr
yT
nR3
xsqr
tU
cPow);ysqr
tU
cPow);add
yA
xsqr);add
yA
ysqr);add
tU
cAdd);sqrt
yA
add);sqrt
tU
cSqrt)l81
iM
sqrt)cM3
cLog2by
x7
cLog2
y23
0
y43
cMul);tmp
yY3;tmp
yC
1))l81
lH
cCot
x7
cTan)nG
lH
cSec
x7
cCos)nG
lH
cCsc
x7
cSin)nG
iM
tmp);}
lC
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cNop:case
cJump:case
iE2:lC
cPCall:lC
cFCall:lC
cEval:nM3
nL);}
yV1
TriTruthValue
GetIntegerInfo
i01
eR{switch
xU2
iY2
cImmed:return
eY2
yZ3)?IsAlways:yO3;case
cFloor:case
cCeil:case
cTrunc:case
cInt:return
IsAlways;case
cAnd:case
cOr:case
cNot:case
cNotNot:case
cEqual:case
t71:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:return
IsAlways;case
cIf:{TriTruthValue
a=GetIntegerInfo
nW1
1
eO2
b=GetIntegerInfo
nW1
2));if(a==b
nZ2
a
l81
Unknown
cM3
cAdd:case
cMul:{for
xU
if(GetIntegerInfo
nW1
a))!=IsAlways
nZ2
Unknown
l81
IsAlways;}
default:nM3
return
Unknown;}
xG1
IsLogicalValue
i01
eR{switch
xU2
iY2
cImmed:return
fp_equal(yZ3,eS1(0))||fp_equal(yZ3,eS1(1));case
cAnd:case
cOr:case
cNot:case
cNotNot:case
cAbsAnd:case
cAbsOr:case
yR3:case
cAbsNotNot:case
cEqual:case
t71:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:nW
cMul:{for
xU
if(!y93
a))yI
return
true
cM3
cIf:case
i03:{return
y93
1))&&y93
2));}
default:nM3
nX2}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
xQ
#if defined(__x86_64) || !defined(FP_SUPPORT_CBRT)
# define CBRT_IS_SLOW
#endif
#if defined(DEBUG_POWI) || defined(DEBUG_SUBSTITUTIONS)
#include <cstdio>
#endif
t5{extern
const
i02
char
powi_table[256];}
iO2{using
iO2
FPoptimizer_CodeTree;xG1
IsOptimizableUsingPowi(long
immed,long
penalty=0){yW2
synth;synth.PushVar(iE2);size_t
bytecodesize_backup
t13
GetByteCodeSize();yV2
x31
immed,yV2
eX1
x8::MulSequence,synth);size_t
bytecode_grow_amount
t13
GetByteCodeSize()-bytecodesize_backup
l81
bytecode_grow_amount<size_t(MAX_POWI_BYTECODE_LENGTH-penalty);}
xB1
ChangeIntoRootChain(yK2&tree,bool
iK2,long
tS2,long
tT2){while(tT2>0){cF
cCbrt);nZ3);tmp.Rehash(nD2--tT2;}
while(tS2>0){cF
cSqrt);if(iK2){tmp
tU
cRSqrt);iK2
i13}
nZ3);tmp.Rehash(nD2--tS2;}
if(iK2){cF
cInv);nZ3
nD2}
}
yT1
RootPowerTable{static
const
eS1
RootPowers[(1+4)*(1+3)];}
;yV1
const
eS1
t7(1+4)*(1+3)]={eS1(1)lS
tB3
tB3
2*tB3
2*2*2)lS
3
x32
2
x32
2*2
x32
2*2*2
x32
2*2*2*2
x32
3
x32
3*2
x32
3*2*2
x32
3*2*2*2
x32
3*2*2*2*2
x32
3*3
x32
3*3*2
x32
3*3*2*2
x32
3*3*2*2*2
x32
3*3*2*2*2*2)}
;e92
PowiResolver{static
const
i02
MaxSep=4;static
lX3
tC3=5;typedef
int
c03;typedef
long
n63;typedef
long
t8;e92
yE2{yE2():n_int_sqrt(0),n_int_cbrt(0),sep_list(),lY1(0){}
int
n_int_sqrt;int
n_int_cbrt;int
eW1
MaxSep];t8
lY1;}
;yV1
static
yE2
CreatePowiResult(eS1
cE2){yE2
result;c03
eE=FindIntegerFactor(cE2);if(eE==0){
#ifdef DEBUG_POWI
tU2"no factor found for %Lg\n"
y33);
#endif
return
result;}
c13=xV1
cE2,eE);n63
e22=EvaluateFactorCost(eE,0,0,0)+cH
c13);int
tD3=0;int
tE3=0;int
lW3=0;
#ifdef DEBUG_POWI
tU2"orig = %Lg\n"
y33);tU2"plain factor = "
tS3"%ld\n"
,(int)eE,(long)e22);
#endif
for
iL1
n_s=0;n_s<MaxSep;++n_s){int
xC=0;n63
yE1=e22;c03
yQ1=eE;for(int
s=1;s<tC3*4;++s){
#ifdef CBRT_IS_SLOW
if(s>=tC3)break;
#endif
int
n_sqrt=s%tC3;int
n_cbrt=s/tC3;if(n_sqrt+n_cbrt>4)y81
eS1
lE1=cE2;lE1-=t7
s];tH1=FindIntegerFactor(lE1);if(xD2!=0){t8
xL=xV1
lE1,xD2);n63
cost=EvaluateFactorCost(xD2,tD3+n_sqrt,tE3+n_cbrt,lW3+1)+cH
xL);
#ifdef DEBUG_POWI
tU2"Candidate sep %u (%d*sqrt %d*cbrt)factor = "
tS3"%ld (for %Lg to %ld)\n"
,s,n_sqrt,n_cbrt,xD2,(long)cost
x12
lE1,(long)xL);
#endif
if(cost<yE1){xC=s;yQ1=xD2;yE1=cost;}
}
}
if(!xC)break;
#ifdef DEBUG_POWI
tU2"CHOSEN sep %u (%d*sqrt %d*cbrt)factor = "
tS3"%ld, exponent %Lg->%Lg\n"
,xC,xC%tC3,xC/tC3,yQ1,yE1
x12(cE2)x12(cE2-t7
xC]));
#endif
tC1
eW1
n_s]=xC;cE2-=t7
xC];tD3+=xC%tC3;tE3+=xC/tC3;e22=yE1;eE=yQ1;lW3+=1;}
c13=xV1
cE2,eE);
#ifdef DEBUG_POWI
tU2"resulting exponent is %ld (from exponent=%Lg, best_factor=%Lg)\n"
,c13
y33
x12
eE);
#endif
while(eE%2==0){++tC1
n_int_sqrt;eE/=2;}
while(eE%3==0){++tC1
n_int_cbrt;eE/=3;}
return
result;}
private:static
n63
cH
t8
xL){static
std::map
c62
i7;if(xL<0){n63
cost=22
l81
cost+cH-xL);}
std::map
c62::nX3
i=i7.xE2
xL);if(i!=i7.cP1
xL
nZ2
i
cJ2;std::pair
c62
cY3
xL,0.0);n63&cost=tC1
second;while(xL>1){int
xD2=0;if(xL<256){xD2=yV2
powi_table[xL];if(xD2&128)xD2&=127;else
xD2=0;if(xD2&64)xD2=-(xD2&63)-1;}
if(xD2){cost+=cH
xD2);xL/=xD2;y81}
if(!(xL&1)){xL/=2;cost+=6;}
else{cost+=7;xL-=1;}
}
i7.nT3,result)l81
cost
eI2
t8
xV1
yF1,tH1){return
makeLongInteger(value*eS1(xD2))eI2
bool
yG1
yF1,tH1){eS1
v=value*eS1(xD2)l81
isLongInteger(v)eI2
c03
FindIntegerFactor(yF1){tH1=(2*2*2*2);
#ifdef CBRT_IS_SLOW
#else
xD2*=(3*3*3);
#endif
c03
result=0;if(yG1
value,xD2)){result=xD2;while((xD2%2)==0&&yG1
value,xD2/2))result=xD2/=2;while((xD2%3)==0&&yG1
value,xD2/3))result=xD2/=3;}
#ifdef CBRT_IS_SLOW
if(result==0){if(yG1
value,3)nZ2
3;}
#endif
return
result;}
static
int
EvaluateFactorCost(int
xD2,int
s,int
c,int
nmuls){lX3
lY3=6;
#ifdef CBRT_IS_SLOW
lX3
e32=25;
#else
lX3
e32=8;
#endif
int
result=s*lY3+c*e32;while(xD2%2==0){xD2/=2;result+=lY3;}
while(xD2%3==0){xD2/=3;result+=e32;}
result+=nmuls
l81
result;}
}
;}
iO2
FPoptimizer_CodeTree{xG1
yK2::RecreateInversionsAndNegations(bool
prefer_base2){bool
changed=false
c23
0;a<cN3++a)if(lT1.RecreateInversionsAndNegations(prefer_base2))yI1
if(changed){exit_changed:Mark_Incompletely_Hashed()nS2
switch(lM2
iY2
cMul:{yG
nV1
lG2
yJ
lH2,cJ1;if(true){bool
nQ1
i13
eS1
nE2=0
c23
cO3
yF2
0)c82
tJ
1)y21){nQ1=true;nE2=tJ
1).xJ1;nM3}
if(nQ1){eS1
immeds=1.0
c23
cO3
y21){immeds*=powgroup.xJ1;yH1}
for
xK2
cN3
a-->0;)nG1&powgroup=lT1;if(powgroup
yF2
0)c82
tJ
1)y21)nG1&log2=tJ
0);log2.iY
log2
tU
lZ3
log2
yT
eG3
immeds,eS1(1)/nE2)));log2.Rehash(nW2}
}
}
for
xK2
cO3
yF2
1)y21){lM1&exp_param=tJ
1);eS1
cE2=exp_param.xJ1;if(cR1,eS1(-1))){iY
lG2
yL
lT1
xY3
yH1
tP1
cE2<0&&eY2
cE2))nG1
iI;iI
tU
cPow);iI
eT
tJ
0));iI
yT-cE2));iI
x02;lG2
yL
iI);iY
yH1}
tP1
powgroup
c82!lH2.iA1{lH2=tJ
0);iY
yH1
tP1
powgroup
nE==cLog2by&&!cJ1.iA1{cJ1=powgroup;iY
yH1}
if(!lG2
cQ3){changed=true
yJ
eR1;eR1
e53
eR1
tI1
lG2);eR1
i72
xW1
cMul);xX1
SetParamsMove(t3
if(xX1
IsImmed()&&fp_equal(xX1
xJ1,nY3{lI2
cInv);eG
eR1);}
else{if(xX1
xT2>=eR1.xT2){lI2
cDiv
eV1
eG
eR1);}
else{lI2
cRDiv);eG
eR1
eV1}
}
}
if(lH2.iA1
nG1
xW1
lM2);xX1
SetParamsMove(t3
while(xX1
RecreateInversionsAndNegations(prefer_base2))xX1
FixIncompleteHashes();lI2
lZ3
eG
lH2
eV1
yI1}
if(cJ1.iA1
nG1
xW1
cMul);n83
yA
cJ1
x03;xX1
AddParamsMove(t3
while(xX1
RecreateInversionsAndNegations(prefer_base2))xX1
FixIncompleteHashes();DelParams();lI2
lZ3
eG
cJ1
lD
0)eV1
yI1
c91
cAdd:{yG
nV1
tV2
c23
tD
c33
cMul){lJ2
xY1:yJ&n83
c72
for(iX2
xX1
cN3
b-->0;){if(n83
lD
b)xC3
xD2=n83
lD
b).xJ1
i82
xD2
eQ1
xY1;}
xX1
iY
xX1
iH1
b
eJ2
tP1
fp_equal(xD2,eS1(-2)))xF
xY1;}
xX1
iY
xX1
iH1
b);n83
yT
eS1(2))eJ2}
}
if(t4){xX1
nR1
n83);yH1}
tP1
c33
cDiv&&!IsIntType
x8::result){lJ2
xZ1:yJ&eR1
c72
if(eR1
lD
0)tF3(fp_equal(eR1
lD
0).xJ1
eQ1
xZ1;}
eR1.iY
eR1.iH1
0);eR1
tU
cInv
eJ2}
if(t4)xF
xZ1;}
eR1.nR1
eR1);yH1}
tP1
c33
cRDiv&&!IsIntType
x8::result){lJ2
x91:yJ&eR1
c72
if(eR1
lD
1)tF3(fp_equal(eR1
lD
1).xJ1
eQ1
x91;}
eR1.iY
eR1.iH1
1);eR1
tU
cInv
eJ2}
if(t4)xF
x91;}
eR1.nR1
eR1);yH1}
if(!tV2
cQ3){
#ifdef DEBUG_SUBSTITUTIONS
tU2"Will make a Sub conversion in:\n"
);fflush(stdout);iO
#endif
yK2
yR1;yR1
tU
cAdd);yR1
tI1
tV2);yR1
i72
cK1;cK1
tU
cAdd);cK1
tI1
iE1));cK1
x02;if(cK1
y21&&fp_equal(cK1.xJ1,eS1(0))){lI2
cNeg);cU3);}
else{if(cK1.xT2==1){lI2
cRSub);cU3);cV3}
tP1
yR1
nE==cAdd){lI2
cSub);cV3
cU3
xY3
for
xK2
1;a<yR1.cN3++a)nG1
e42;e42
tU
cSub);e42
tI1
iE1));e42.y12);eG
e42);cU3
lD
a));}
}
else{lI2
cSub);cV3
cU3);}
}
#ifdef DEBUG_SUBSTITUTIONS
tU2"After Sub conversion:\n"
);fflush(stdout);iO
#endif
c91
cPow:{lM1&p0
tW2
0);lM1&p1
tW2
1);if(p1
tF3
n03!=eS1(0)&&!eY2
p1.xJ1)){eJ
yE2
r=eJ
CreatePowiResult(fp_abs
n03));if(r.lY1!=0){bool
iB1
i13
if
n03<0&&r.eW1
0]==0&&r
x33>0){iB1=true;}
#ifdef DEBUG_POWI
tU2"Will resolve powi %Lg as powi(chain(%d,%d),%ld)"
x12
fp_abs
n03),r
x33,r.n_int_cbrt,r.lY1)xS3
n=0;n<eJ
MaxSep;++n){if(r
cW3==0)break;int
n_sqrt=r
cW3%eJ
tC3;int
n_cbrt=r
cW3/eJ
tC3;tU2"*chain(%d,%d)"
,n_sqrt,n_cbrt);}
tU2"\n"
);
#endif
yK2
c92
tW2
0)yJ
yG2=c92;yG2.iY
ChangeIntoRootChain(yG2,iB1,r
x33,r.n_int_cbrt);yG2
i72
pow;if(r.lY1!=1){pow
tU
cPow);pow
yA
yG2);pow
yT
eS1(r.lY1)));}
else
pow.swap(yG2)yJ
mul;mul
e53
mul
yA
pow)xS3
n=0;n<eJ
MaxSep;++n){if(r
cW3==0)break;int
n_sqrt=r
cW3%eJ
tC3;int
n_cbrt=r
cW3/eJ
tC3
yJ
e52=c92;e52.iY
ChangeIntoRootChain(e52,false,n_sqrt,n_cbrt);e52
x02;mul
yA
e52);}
if
n03<0&&!iB1){mul
x02;lI2
cInv);nC1
0,mul);iH1
1);}
else{lI2
cMul);SetParamsMove(mul.iE1));}
#ifdef DEBUG_POWI
iO
#endif
yI1
nM3}
}
if(lM2==cPow&&(!p1
y21||!isLongInteger
n03)||!IsOptimizableUsingPowi
x8(makeLongInteger
n03)))){if(p0
y21&&p0.xJ1>0.0){if(prefer_base2){eS1
yH2=fp_log2(p0.xJ1)i82
yH2,nY3{iH1
0);}
else{n1
cU1
yH2));cE2
eT
p1)tD1
iP}
lI2
cExp2);yI1}
else{eS1
yH2=fp_log(p0.xJ1)i82
yH2,nY3{iH1
0);}
else{n1
cU1
yH2));cE2
eT
p1)tD1
iP}
lI2
cExp);yI1}
}
tP1
GetPositivityInfo(p0)==IsAlways){if(prefer_base2)nG1
log;log
tU
cLog2);log
eT
p0);log
x02;n1
p1);cE2
yA
log)tD1);lI2
cExp2
iP
yI1}
else
nG1
log;log
tU
cLog);log
eT
p0);log
x02;n1
p1);cE2
yA
log)tD1);lI2
cExp
iP
yI1}
}
c91
cDiv:{if(GetParam(0)y21&&fp_equal(GetParam(0).xJ1,nY3{lI2
cInv);iH1
0);}
nM3
default:nM3
if(changed)goto
exit_changed
l81
changed;}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
xQ
iO2{using
iO2
FPoptimizer_CodeTree;class
eT1{size_t
yJ1;size_t
eK;size_t
eL;size_t
lF1;e13
eT1():yJ1(0),eK(0),eL(0),lF1(0){}
void
c43
OPCODE
op){yJ1+=1;n13
cCos)++eK;n13
cSin)++eL;n13
cSec)++eK;n13
cCsc)++eL;n13
cTan)++lF1;n13
cCot)++lF1;}
size_t
GetCSEscore()const{size_t
result=yJ1
l81
result;}
int
NeedsSinCos()const{bool
always_sincostan=(yJ1==(eK+eL+lF1));if((lF1&&(eL||eK))||(eL&&eK)){if(always_sincostan
nZ2
1
l81
2;}
return
0;}
size_t
MinimumDepth()const{size_t
n_sincos=std::min(eK,eL);if(n_sincos==0
nZ2
2
l81
1;}
}
;iP2
TreeCountType:public
std::multimap<fphash_t,std::pair<eT1,yK2> >{}
;xB1
FindTreeCounts(nX1&lK2,lM1&tree,OPCODE
lL2{iG1
i=lK2.xE2
tree.GetHash());bool
found
i13
for(;i!=lK2.cP1
tree.GetHash();++i){if(tree
iA
i
cJ2
eE3)){i
cJ2.first.c43
lL2;found=true;nM3}
if(!found){eT1
count;count.c43
lL2;lK2.nT3,std::make_pair(tree.GetHash(),std::make_pair
cR3
n72);}
for
yS
FindTreeCounts(lK2,xI2,tree
nE);}
e92
c9{bool
BalanceGood;bool
cI;}
;yV1
c9
lG1
lM1&root,lM1&c53{if(root
iA
c53){c9
result={true,true}
l81
result;}
c9
result={true,false}
;if(root
nE==cIf||root
nE==i03){c9
cond=lG1
root
lD
0),c53;c9
xW=lG1
root
lD
1),c53;c9
y9=lG1
root
lD
2),c53;if(cond.cI||xW.cI||y9.cI){tC1
cI=true;}
result
eH=((xW.cI==y9.cI)||eX2&&(cond
eH||(xW.cI&&y9.cI))&&(xW
eH||eX2&&(y9
eH||eX2;}
else{bool
tK1
i13
bool
nS1
i13
for(iX2
root.GetParamCount(),a=0;a<b;++a){c9
tmp=lG1
root
lD
a),c53;if(tmp.cI)tC1
cI=true;if(tmp
eH==false)tK1=true;tP1
tmp.cI)nS1=true;}
if(tK1&&!nS1)result
eH
i13}
return
result;}
xG1
n62
lM1&tG3
lM1&tree,const
yW2&synth,const
nX1&lK2){for(iX2
iT,a=0;a<b;++a){lM1&leaf=xI2;iG1
lT2;xH2
nX1::const_iterator
i=lK2.nW3
i!=lK2.end();++i){if(i->eQ3
leaf.GetHash())y81
const
c63
i->nF2
size_t
score=occ.GetCSEscore();lM1&candidate=i->nG2
if(cC2
candidate))y81
if(leaf.xT2<occ.MinimumDepth())y81
if(score<2)y81
if(lG1
tG3
leaf)eH==false)continue
nS2
if(n62
tG3
leaf,synth,lK2
iF1;}
nX2
xG1
tL1
lM1&nU3,lM1&expr){for
iT1
nU3
lD
a)iA
expr
iF1;for
iT1
tL1
nU3
lD
a),expr
iF1
l81
false;}
xG1
GoodMomentForCSE(lM1&nU3,lM1&expr){if(nU3
nE==cIf
nZ2
true;for
iT1
nU3
lD
a)iA
expr
iF1;size_t
tX2=0;for
iT1
tL1
nU3
lD
a),expr))++tX2
l81
tX2!=1;}
}
iO2
FPoptimizer_CodeTree{yV1
size_t
yK2::SynthCommonSubExpressions(yV2
lP1
const{size_t
stacktop_before
t13
GetStackTop();nX1
lK2;FindTreeCounts(lK2,*this,lM2);
#ifdef DEBUG_SUBSTITUTIONS_CSE
DumpHashes(*this);
#endif
for(;;){size_t
yI2=0;iG1
lT2;for(iG1
j,i=lK2.nW3
i!=lK2.end();i=j){j=i;++j;const
c63
i->nF2
size_t
score=occ.GetCSEscore();lM1&tree=i->nG2
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<"Score "
<<score<<":\n"
;DumpTreeWithIndent(tree);
#endif
if(cC2
tree))xV
if(tree.xT2<occ.MinimumDepth())xV
if(score<2)xV
if(lG1*this,tree)eH==false)xV
if(n62*this,tree,synth,lK2)){y81}
if(!GoodMomentForCSE(*this
n72
xV
score*=tree.xT2;if(score>yI2){yI2=score;lT2=i;}
}
if(yI2<=0)break;const
c63
lT2->nF2
lM1&tree=lT2->nG2
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<tZ3"Common Subexpression:"
;xE3
x8(tree)lN1"\n"
;
#endif
int
y01=occ.NeedsSinCos()yJ
tY2,tZ2;if(y01){tY2
eT
tree);tY2
tU
cSin);tY2
x02;tZ2
eT
tree);tZ2
tU
cCos);tZ2
x02;if(cC2
tY2)||cC2
tZ2)){if(y01==2){lK2.erase(lT2);y81}
y01=0;}
}
tree.SynthesizeByteCode(synth,false);lK2.erase(lT2);
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<"Done with Common Subexpression:"
;xE3
x8(tree)lN1"\n"
;
#endif
if(y01){if(y01==2){tQ1
c71);}
lK1
cSinCos,1,2
eU3.yZ2
tY2,1
eU3.yZ2
tZ2,0);}
}
return
synth.x5
stacktop_before;}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
yV1
lR1
x8
nH2
using
iO2
FPoptimizer_CodeTree;CopyOnWrite()yJ
tree;tree.GenerateFrom(mData->mByteCode,mData->mImmed,*mData);FPoptimizer_Optimize::ApplyGrammars(tree);yG<i02>c73;yG
x8
immed;size_t
stacktop_max=0;tree.SynthesizeByteCode(c73,immed,stacktop_max);if(mData->mStackSize!=stacktop_max){mData->mStackSize=i02(stacktop_max);
#if !defined(FP_USE_THREAD_SAFE_EVAL) && \
    !defined(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA)
mData->mStack
nE3
stacktop_max);
#endif
}
mData->mByteCode.swap(c73);mData->mImmed.swap(immed);}
#ifdef FP_SUPPORT_MPFR_FLOAT_TYPE
n73<>lR1<MpfrFloat>nH2}
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
n73<>lR1<GmpInt>nH2}
#endif
FUNCTIONPARSER_INSTANTIATE_TYPES
#endif

#endif
