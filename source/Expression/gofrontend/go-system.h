//
//  go-system.h
//  lldb
//
//  Created by Ryan Brown on 3/25/15.
//
//

#ifndef lldb_go_system_h
#define lldb_go_system_h

#include <stdio.h>
# include <unordered_map>
# include <unordered_set>
#include "lldb/Core/StreamString.h"

# define Unordered_map(KEYTYPE, VALTYPE) \
std::unordered_map<KEYTYPE, VALTYPE>

# define Unordered_map_hash(KEYTYPE, VALTYPE, HASHFN, EQFN) \
std::unordered_map<KEYTYPE, VALTYPE, HASHFN, EQFN>

# define Unordered_set(KEYTYPE) \
std::unordered_set<KEYTYPE>

# define Unordered_set_hash(KEYTYPE, HASHFN, EQFN) \
std::unordered_set<KEYTYPE, HASHFN, EQFN>

#define go_assert(x) assert (x)
#define go_unreachable() assert(0 && "unreachable")
#define error(...) fprintf(stderr, __VA_ARGS__)
#define inform(...) error_at(__VA_ARGS__)
#define error_at(loc, ...) do {lldb_private::StreamString __s; (loc).GetDecl().Dump(&__s, true); __s.Printf(": "); __s.Printf(__VA_ARGS__); fprintf(stderr, "%s", __s.GetData()); } while(false)
#define warning_at(...) error_at(__VA_ARGS__)
#define _(x) x
#define GO_EXTERN_C extern "C"
#endif
