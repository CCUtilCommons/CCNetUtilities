#pragma once

/**
 * @brief Force inline makes the performace if the functions shorts
 *
 */

// -------------- Session of CNETUTILS_FORCEINLINE --------------
#if defined(_MSC_VER)
// Microsoft Visual C++
#define CNETUTILS_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
// GNU C/C++ (GCC) and Clang/LLVM
#define CNETUTILS_FORCEINLINE __attribute__((always_inline)) inline
#else
#define CNETUTILS_FORCEINLINE inline // default
#endif

/**
 * @brief   OK these are for build in DDL or Sos
 *          using in controlling the symbols visibilities
 */

// -------------- Session of Symbol visibilities --------------
// is Windows?
#if defined(_WIN32) && (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER))

// is DDL?
#if defined(API_BUILD_DLL)
#define CNETUTILS_API __declspec(dllexport)
#else
#define CNETUTILS_API __declspec(dllimport) // OK Not
#endif

#elif defined(__GNUC__) || defined(__clang__)
#define CNETUTILS_API __attribute__((visibility("default")))
#else
#define CNETUTILS_API
#endif // CNETUTILS_API End
