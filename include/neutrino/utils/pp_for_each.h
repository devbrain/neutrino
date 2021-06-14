#ifndef NEUTRINO_UTILS_PP_FOR_EACH_HH
#define NEUTRINO_UTILS_PP_FOR_EACH_HH
//
// Created by igor on 21/06/2020.
//
// https://github.com/yuanzhubi/macro_for_each/blob/master/macro_for_each.h
// https://github.com/wangjieest/MACRO_FOR_EACH/blob/master/micro_for_each.h
// clang-format off

#define ARG_OP_1(pref, op, sep,  ignored_data, arg,...) pref op(arg)
#define ARG_OP_2(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_1(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_3(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_2(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_4(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_3(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_5(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_4(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_6(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_5(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_7(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_6(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_8(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_7(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_9(pref, op, sep,  ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_8(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_10(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_9(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_11(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_10(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_12(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_11(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_13(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_12(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_14(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_13(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_15(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_14(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_16(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_15(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_16(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_15(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_17(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_16(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_18(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_17(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_19(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_18(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_20(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_19(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_21(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_20(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_22(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_21(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_23(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_22(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_24(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_23(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_25(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_24(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_26(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_25(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_27(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_26(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_28(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_27(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_29(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_28(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_30(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_29(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_31(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_30(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_32(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_31(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_33(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_32(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_34(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_33(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_35(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_34(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_36(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_35(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_37(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_36(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_38(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_37(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_39(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_38(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_40(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_39(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_41(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_40(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_42(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_41(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_43(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_42(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_44(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_43(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_45(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_44(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_46(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_45(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_47(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_46(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_48(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_47(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_49(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_48(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_50(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_49(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_51(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_50(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_52(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_51(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_53(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_52(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_54(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_53(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_55(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_54(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_56(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_55(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_57(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_56(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_58(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_57(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_59(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_58(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_60(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_59(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_61(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_60(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_62(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_61(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_63(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_62(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_64(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_63(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_65(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_64(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_66(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_65(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_67(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_66(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_68(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_67(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_69(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_68(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_70(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_69(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_71(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_70(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_72(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_71(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_73(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_72(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_74(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_73(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_75(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_74(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_76(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_75(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_77(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_76(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_78(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_77(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_79(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_78(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_80(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_79(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_81(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_80(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_82(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_81(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_83(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_82(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_84(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_83(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_85(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_84(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_86(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_85(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_87(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_86(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_88(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_87(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_89(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_88(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_90(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_89(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_91(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_90(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_92(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_91(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_93(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_92(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_94(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_93(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_95(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_94(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_96(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_95(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_97(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_96(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_98(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_97(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))
#define ARG_OP_99(pref, op, sep, ignored_data, arg,...) pref op(arg) sep EXPAND_(ARG_OP_98(EXPAND_(pref), op, EXPAND_(sep), ignored_data, ##__VA_ARGS__))

#define ARG_SEQ() \
         99,98,97,96,95,94,93,92,91,90, \
         89,88,87,86,85,84,83,82,81,80, \
         79,78,77,76,75,74,73,72,71,70, \
         69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
          9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define ARG_N( \
         _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
        _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
        _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
        _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
        _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
        _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
        _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
        _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
        _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
        _91,_92,_93,_94,_95,_96,_97,_98,_99, N, ...) N

#define MACRO_CONCAT_(A, B) A##B
#define MACRO_CONCAT(A, B) MACRO_CONCAT_(A, B)
#define EXPAND_(...) __VA_ARGS__

#define ARG_COUNT_(...) EXPAND_(ARG_N(__VA_ARGS__))
#define ARG_COUNT(...) ARG_COUNT_(__VA_ARGS__, ARG_SEQ())
#define ARG_OP(...) MACRO_CONCAT(ARG_OP_, ARG_COUNT(__VA_ARGS__))

#define ARG_NULL
#define FOR_EACH(pref, op, sep, ...) EXPAND_(ARG_OP(EXPAND_(__VA_ARGS__))(pref, op, sep, IGNORED_DATA, __VA_ARGS__))

#define ARG_COMMA ,
#define FOR_EACH_COMMA(pref, op, ...) EXPAND_(ARG_OP(EXPAND_(__VA_ARGS__))(pref, op, ARG_COMMA, IGNORED_DATA, ##__VA_ARGS__))
// FOR_EACH_COMMA(MACRO, 1, 2, 3, 4)
// -->
// MICRO(1),MICRO(2),MICRO(3),MICRO(4)

#define WRAP_CALL(call, op, ...) call(FOR_EACH_COMMA(ARG_NULL, op, ##__VA_ARGS__))
// WRAP_CALL(call, MACRO, 1, 2, 3, 4)
// -->
// call(MACRO(1), MACRO(2), MACRO(3), MACRO(4))

// cannot use FOR_EACH on gcc and clang, why?
#define WRAP_CALL1(call, op, ...) ARG_OP(EXPAND_(__VA_ARGS__))(ARG_NULL, op, ARG_COMMA, IGNORED_DATA, ##__VA_ARGS__)


#define SEPERATOR_ARGS(sep, ...) FOR_EACH(ARG_NULL, EXPAND_, sep, ##__VA_ARGS__)
// std::cout <<  SEPERATOR_ARGS(<<, 1, 2, 3, std::endl)
//           -->
// std::cout <<  1 << 2 << 3 << std::endl

#define FOR_EACH_CALL(call, ...) FOR_EACH(ARG_NULL, call, ;, ##__VA_ARGS__)
// ARG_OP_CALL(MACRO, 1, 2, 3, 4)
// -->
// MACRO(1); MACRO(2); MACRO(3); MACRO(4)

#define CHAIN_CALL(call,...) FOR_EACH(ARG_NULL, call, ., ##__VA_ARGS__)
// CHAIN_CALL(MACRO, 1, 2, 3, 4)
// -->
// MACRO(1).MACRO(2).MACRO(3).MACRO(4)
// clang-format on


// https://stackoverflow.com/questions/17549906/c-json-serialization
#if defined(TEST)


#define EVENT_FIELD(TYPE, NAME) (TYPE, NAME)
#define D_EV_FIELDS_EXPAND(TYPE, NAME) TYPE NAME ;
#define D_EV_FIELDS(X) D_EV_FIELDS_EXPAND X

#define D_EV_PROPS_EXPAND(TYPE, NAME) NAME, #NAME
#define D_EV_PROPS(X) D_EV_PROPS_EXPAND X



#define DECLARE_EVENT(NAME, ...)                                                                    \
    struct NAME {                                                                                   \
        using tag = ::neutrino::detail::event_tag;                                                  \
        static constexpr const char* const event_name() {return #NAME; }                            \
        FOR_EACH(ARG_NULL, D_EV_FIELDS, ARG_NULL , ##__VA_ARGS__)                                   \
        constexpr static auto event_properties =                                                    \
            ::neutrino::detail::property_slice(std::make_tuple(                                     \
                FOR_EACH_COMMA (&NAME::, D_EV_PROPS,  ##__VA_ARGS__)                                \
            ));                                                                                     \
        friend std::ostream& operator << (std::ostream& os, const NAME& obj)                        \
        {                                                                                           \
            ::neutrino::events::to_string(obj, os);                                                 \
            return os;                                                                              \
        }                                                                                           \
    }

DECLARE_EVENT(ZOPA,
        EVENT_FIELD(int, a),
        EVENT_FIELD(double, b)
        );



#endif


#endif //SDLPP_PP_FOR_EACH_H
