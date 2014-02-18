//
//  objc2_runtime.h
//  loader
//
//  Created by Sam Marshall on 11/2/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc2_runtime_h
#define loader_objc2_runtime_h

#define kObjc2SelRef "__objc_selrefs"
#define kObjc2MsgRefs "__objc_msgrefs"
#define kObjc2ClassRefs "__objc_classrefs"
#define kObjc2SuperRefs "__objc_superrefs"
#define kObjc2ClassList "__objc_classlist"
#define kObjc2NlClsList "__objc_nlclslist"
#define kObjc2CatList "__objc_catlist"
#define kObjc2NlCatList "__objc_nlcatlist"
#define kObjc2ProtoList "__objc_protolist"
#define kObjc2ProtoRefs "__objc_protorefs"

struct SDMSTObjc2ClassMethodInfo {
	uint32_t entrySize;
	uint32_t count;
} ATR_PACK SDMSTObjc2ClassMethodInfo;

struct SDMSTObjc2ClassProtcolInfo {
	uint64_t count;
} ATR_PACK SDMSTObjc2ClassProtcolInfo;

struct SDMSTObjc2ClassIVarInfo {
	uint32_t entrySize;
	uint32_t count;
} ATR_PACK SDMSTObjc2ClassIVarInfo;

struct SDMSTObjc2ClassPropertyInfo {
	uint32_t entrySize;
	uint32_t count;
} ATR_PACK SDMSTObjc2ClassPropertyInfo;

struct SDMSTObjc2ClassMethod {
	uint64_t name;
	uint64_t type;
	uint64_t imp;
} ATR_PACK SDMSTObjc2ClassMethod;

struct SDMSTObjc2ClassProtocol {
	uint64_t offset;
} ATR_PACK SDMSTObjc2ClassProtocol;

struct SDMSTObjc2ClassIVar {
	uint64_t offset;
	uint64_t name;
	uint64_t type;
	uint32_t align;
	uint32_t size;
} ATR_PACK SDMSTObjc2ClassIVar;

struct SDMSTObjc2ClassProperty {
	char *name;
	char *attributes;
} ATR_PACK SDMSTObjc2ClassProperty;

struct SDMSTObjc2ClassData {
	uint32_t flags;
	uint32_t instanceStart;
	uint32_t instanceSize;
	uint32_t reserved;
	uint64_t iVarLayout;
	uint64_t name; //char*
	uint64_t method; //struct SDMSTObjc2ClassMethodInfo*
	uint64_t protocol; //struct SDMSTObjc2ClassProtcolInfo*
	uint64_t ivar; //struct SDMSTObjc2ClassIVarInfo*
	uint64_t weakIVarLayout;
	uint64_t property; //struct SDMSTObjc2ClassProperty*
} ATR_PACK SDMSTObjc2ClassData;

struct SDMSTObjc2Class {
	struct SDMSTObjc2Class *isa;
	uint64_t superCls;
	uint64_t cache;
	uint64_t vTable;
	struct SDMSTObjc2ClassData *data;
} ATR_PACK SDMSTObjc2Class;

#endif
