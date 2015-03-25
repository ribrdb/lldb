//
//  go-location.h
//  lldb
//
//  Created by Ryan Brown on 3/25/15.
//
//

#ifndef lldb_go_location_h
#define lldb_go_location_h

#include "lldb/Symbol/Declaration.h"

class Location
{
public:
    Location() : m_decl() {
    }
    
    explicit Location(lldb_private::Declaration decl) : m_decl(decl) {
    }
    const lldb_private::Declaration& GetDecl() { return m_decl; }

private:
    lldb_private::Declaration m_decl;
};

inline bool operator<(Location a, Location b) {
    return lldb_private::Declaration::Compare(a.GetDecl(), b.GetDecl()) < 0;
}

#define UNKNOWN_LOCATION 

#endif
