//
//  objc_runtime.h
//  loader
//
//  Created by Sam Marshall on 11/2/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_runtime_h
#define loader_objc_runtime_h
#include "objc_lexer.h"
#include "objc1_runtime.h"
#include "objc2_runtime.h"

struct SDMSTObjcIVar {
	char *name;
	char *type;
	uintptr_t offset;
} ATR_PACK SDMSTObjcIVar;

struct SDMSTObjcMethod {
	char *name;
	char *type;
	uintptr_t offset;
} ATR_PACK SDMSTObjcMethod;

struct SDMSTObjcProtocol {
	uintptr_t offset;
} ATR_PACK SDMSTObjcProtocol;

struct SDMSTObjcClass {
	struct SDMSTObjcClass *superCls;
	char *className;
	struct SDMSTObjcIVar *ivar;
	uint32_t ivarCount;
	struct SDMSTObjcMethod *method;
	uint32_t methodCount;
	struct SDMSTObjcProtocol *protocol;
	uint32_t protocolCount;
} ATR_PACK SDMSTObjcClass;

struct SDMSTObjcModule {
	char *impName;
	struct SDMSTObjcClass *symbol;
} ATR_PACK SDMSTObjcModule;

struct SDMSTObjcModuleRaw {
	uint32_t version;
	uint32_t size;
	uint32_t name;
	uint32_t symtab;
} ATR_PACK SDMSTObjcModuleRaw;

struct SDMSTObjcModuleContainer {
	struct SDMSTObjcModule *module;
	uint32_t moduleCount;
} ATR_PACK SDMSTObjcModuleContainer;

struct SDMSTObjc {
	struct SDMSTObjcClass *cls;
	uint32_t clsCount;
	CoreRange classRange;
	CoreRange catRange;
	CoreRange protRange;
	CoreRange clsMRange;
	CoreRange instMRange;
} ATR_PACK SDMSTObjc;

#define CLS_CLASS               0x1
#define CLS_META                0x2
#define CLS_INITIALIZED         0x4
#define CLS_POSING              0x8
#define CLS_MAPPED              0x10
#define CLS_FLUSH_CACHE         0x20
#define CLS_GROW_CACHE          0x40
#define CLS_NEED_BIND           0x80
#define CLS_METHOD_ARRAY        0x100
#define CLS_JAVA_HYBRID         0x200
#define CLS_JAVA_CLASS          0x400
#define CLS_INITIALIZING        0x800
#define CLS_FROM_BUNDLE         0x1000
#define CLS_HAS_CXX_STRUCTORS   0x2000
#define CLS_NO_METHOD_ARRAY     0x4000
#define CLS_HAS_LOAD_METHOD     0x8000
#define CLS_CONSTRUCTING        0x10000
#define CLS_EXT                 0x20000

#define SDMSTObjc1ValidClassCheck(a) ((a | CLS_CLASS) == 0x0 || (a | CLS_META) == 0x0 || (a | CLS_INITIALIZED) == 0x0 || (a | CLS_POSING) == 0x0 || (a | CLS_MAPPED) == 0x0 || (a | CLS_FLUSH_CACHE) == 0x0 || (a | CLS_GROW_CACHE) == 0x0 || (a | CLS_NEED_BIND) == 0x0 || (a | CLS_METHOD_ARRAY) == 0x0 || (a | CLS_JAVA_HYBRID) == 0x0 || (a | CLS_JAVA_CLASS) == 0x0 || (a | CLS_INITIALIZING) == 0x0 || (a | CLS_FROM_BUNDLE) == 0x0 || (a | CLS_HAS_CXX_STRUCTORS) == 0x0 || (a | CLS_NO_METHOD_ARRAY) == 0x0 || (a | CLS_HAS_LOAD_METHOD) == 0x0 || (a | CLS_CONSTRUCTING) == 0x0 || (a | CLS_EXT) == 0x0)

struct SDMSTObjcClass* SDMSTObjc1CreateClassFromProtocol(struct SDMSTObjc *objcData, struct SDMSTObjc1Protocol *prot, uint64_t offset);
struct SDMSTObjcClass* SDMSTObjc1CreateClassFromCategory(struct SDMSTObjc *objcData, struct SDMSTObjc1Category *cat, uint64_t offset);
struct SDMSTObjcClass* SDMSTObjc1CreateClassFromClass(struct SDMSTObjc *objcData, struct SDMSTObjc1Class *cls, uint64_t offset);
void SDMSTObjc1CreateClassFromSymbol(struct SDMSTObjc *objcData, struct SDMSTObjc1Symtab *symtab);
struct SDMSTObjcClass* SDMSTObjc2ClassCreateFromClass(struct SDMSTObjc2Class *cls, struct SDMSTObjc2Class *parentClass, CoreRange dataRange, uint64_t offset);

#endif
