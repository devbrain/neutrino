//
// Created by igor on 04/06/2020.
//

#ifndef NEUTRINO_UTILS_MACROS_HH
#define NEUTRINO_UTILS_MACROS_HH
/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define PPCAT_NX(A, B) A ## B

/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT(A, B) PPCAT_NX(A, B)

#define ANONYMOUS_VAR(X) PPCAT(X, __LINE__)

#define STRINGIZE_HELPER(x) #x
#define STRINGIZE(x) STRINGIZE_HELPER(x)

#endif
