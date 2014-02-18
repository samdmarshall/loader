//
//  runtime_base.h
//  loader
//
//  Created by Sam Marshall on 12/29/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_runtime_base_h
#define loader_runtime_base_h

#include "util.h"
#include "type_register.h"

#define ObserverGetSetBlock(ObserverName) void (^ObserverName)(id, SEL) = ^(id self, SEL _cmd)

BOOL SDMRegisterCallbacksForKeyInInstance(BlockPointer getObserve, BlockPointer setObserve, char *keyName, id instance);

void SDMRemoveCallbackForKeyInInstance(char *keyName, id instance);

#endif
