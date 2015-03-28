//
//  GoASTContext.h
//  lldb
//
//  Created by Ryan Brown on 3/26/15.
//
//

#ifndef __lldb__GoASTContext__
#define __lldb__GoASTContext__

#include <stdio.h>
namespace lldb_private {
    
class Declaration;

class GoASTContext
{
public:
    GoASTContext() { }
private:
    //------------------------------------------------------------------
    // For GoASTContext only
    //------------------------------------------------------------------
    GoASTContext(const GoASTContext&);
    const GoASTContext& operator=(const GoASTContext&);
};
}

#endif /* defined(__lldb__GoASTContext__) */
