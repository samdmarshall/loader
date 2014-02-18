//
//  objc_runtime.c
//  loader
//
//  Created by Sam Marshall on 11/10/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_runtime_c
#define loader_objc_runtime_c

#include "objc_runtime.h"
#include <string.h>

struct SDMSTObjcClass* SDMSTObjc1CreateClassFromProtocol(struct SDMSTObjc *objcData, struct SDMSTObjc1Protocol *prot, uint64_t offset) {
	struct SDMSTObjcClass *newClass = calloc(0x1, sizeof(struct SDMSTObjcClass));
	if (prot) {
		newClass->className = Ptr(PtrAdd(offset, prot->name));
	}
	return newClass;
}

struct SDMSTObjcClass* SDMSTObjc1CreateClassFromCategory(struct SDMSTObjc *objcData, struct SDMSTObjc1Category *cat, uint64_t offset) {
	struct SDMSTObjcClass *newClass = calloc(0x1, sizeof(struct SDMSTObjcClass));
	if (cat) {
		newClass->className = Ptr(PtrAdd(offset, cat->name));
	}
	return newClass;
}

struct SDMSTObjcClass* SDMSTObjc1CreateClassFromClass(struct SDMSTObjc *objcData, struct SDMSTObjc1Class *cls, uint64_t offset) {
	struct SDMSTObjcClass *newClass = calloc(0x1, sizeof(struct SDMSTObjcClass));
	if (cls) {
		if (cls->superClass != 0x0) {
			bool isValidClass = SDMSTObjc1ValidClassCheck(((uint32_t)(cls->info)));
			if (cls->superClass != cls->isa && isValidClass) {
				struct SDMSTObjc1Class *objc1class = (struct SDMSTObjc1Class *)PtrAdd(offset, cls->superClass);
				newClass->superCls = SDMSTObjc1CreateClassFromClass(objcData, objc1class, offset);
			} else if (isValidClass) {
				newClass->superCls = (struct SDMSTObjcClass *)PtrAdd(offset, cls->superClass);
			} else {
				newClass->superCls = 0x0;
			}
			newClass->className = Ptr(PtrAdd(offset, cls->name));
			
			struct SDMSTObjc1ClassIVarInfo *ivarInfo = (struct SDMSTObjc1ClassIVarInfo *)PtrAdd(offset, cls->ivars);
			if (ivarInfo) {
				newClass->ivarCount = ivarInfo->count;
				newClass->ivar = calloc(newClass->ivarCount, sizeof(struct SDMSTObjcIVar));
				struct SDMSTObjc1ClassIVar *ivarOffset = (struct SDMSTObjc1ClassIVar *)PtrAdd(ivarInfo, sizeof(struct SDMSTObjc1ClassIVarInfo));
				for (uint32_t i = 0x0; i < newClass->ivarCount; i++) {
					newClass->ivar[i].name = Ptr(PtrAdd(offset, ivarOffset[i].name));
					newClass->ivar[i].type = Ptr(PtrAdd(offset, ivarOffset[i].type));
					newClass->ivar[i].offset = (uintptr_t)(ivarOffset[i].offset);
				}
				
			}
						
			struct SDMSTObjc1ClassMethodInfo *methodInfo = (struct SDMSTObjc1ClassMethodInfo *)PtrAdd(offset, cls->methods);
			if (methodInfo && (((uint64_t)methodInfo >= (uint64_t)PtrAdd(PtrHighPointer(offset), objcData->classRange.offset) && (uint64_t)methodInfo < (uint64_t)PtrAdd(PtrHighPointer(offset), (objcData->clsMRange.offset + (uint64_t)objcData->clsMRange.length))) || ((uint64_t)methodInfo >= (uint64_t)PtrAdd(PtrHighPointer(offset), objcData->instMRange.offset) && (uint64_t)methodInfo < (uint64_t)PtrAdd(PtrHighPointer(offset), (objcData->instMRange.offset + objcData->instMRange.length))))) {
				newClass->methodCount = methodInfo->count;
				newClass->method = calloc(newClass->methodCount, sizeof(struct SDMSTObjcMethod));
				struct SDMSTObjc1ClassMethod *methodOffset = (struct SDMSTObjc1ClassMethod *)PtrAdd(methodInfo, sizeof(struct SDMSTObjc1ClassMethodInfo));
				for (uint32_t i = 0x0; i < newClass->methodCount; i++) {
					newClass->method[i].name = Ptr(PtrAdd(offset, methodOffset[i].name));
					newClass->method[i].type = Ptr(PtrAdd(offset, methodOffset[i].type));
					newClass->method[i].offset = (uintptr_t)(methodOffset[i].imp);
				}
			}
			
			struct SDMSTObjc1Protocol *protocolInfo = (struct SDMSTObjc1Protocol *)PtrAdd(offset, cls->protocols);
			if (protocolInfo) {
				
			}
		}
	}
	return newClass;
}

void SDMSTObjc1CreateClassFromSymbol(struct SDMSTObjc *objcData, struct SDMSTObjc1Symtab *symtab) {
	if (symtab) {
		uint32_t counter = symtab->catCount + symtab->classCount;
		struct SDMSTObjc1SymtabDefinition *symbol = (struct SDMSTObjc1SymtabDefinition *)PtrAdd(symtab, sizeof(struct SDMSTObjc1Symtab));
		for (uint32_t i = 0x0; i < counter; i++) {
			uint64_t memOffset;
			if (((PtrAdd(memOffset, symbol[i].defintion) >= (PtrAdd(PtrHighPointer(memOffset), objcData->classRange.offset))) && (PtrAdd(memOffset, symbol[i].defintion) <= (PtrAdd(PtrHighPointer(memOffset), ((uint64_t)(objcData->classRange.offset) + (uint64_t)objcData->classRange.length)))))) {
				struct SDMSTObjc1Class *objc1class = (struct SDMSTObjc1Class *)PtrAdd(memOffset, symbol[i].defintion);
				struct SDMSTObjcClass *newClass = SDMSTObjc1CreateClassFromClass(objcData, objc1class, memOffset);
				memcpy(&(objcData->cls[objcData->clsCount]), newClass, sizeof(struct SDMSTObjcClass));
				free(newClass);
				objcData->clsCount++;
				objcData->cls = realloc(objcData->cls, sizeof(struct SDMSTObjcClass)*(objcData->clsCount+0x1));
			}
			if ((PtrAdd(memOffset, symbol[i].defintion) >= PtrAdd(PtrHighPointer(memOffset), objcData->catRange.offset)) && (PtrAdd(memOffset, symbol[i].defintion) <= (PtrAdd(PtrHighPointer(memOffset), (objcData->catRange.offset + objcData->catRange.length))))) {
				struct SDMSTObjc1Category *objc1cat = (struct SDMSTObjc1Category *)PtrAdd(memOffset,symbol[i].defintion);
				struct SDMSTObjcClass *newClass = SDMSTObjc1CreateClassFromCategory(objcData, objc1cat, memOffset);
				memcpy(&(objcData->cls[objcData->clsCount]), newClass, sizeof(struct SDMSTObjcClass));
				free(newClass);
				objcData->clsCount++;
				objcData->cls = realloc(objcData->cls, sizeof(struct SDMSTObjcClass)*(objcData->clsCount+0x1));
			}
			symbol = (struct SDMSTObjc1SymtabDefinition *)PtrAdd(symbol, sizeof(struct SDMSTObjc1SymtabDefinition));
		}
	}
}

struct SDMSTObjcClass* SDMSTObjc2ClassCreateFromClass(struct SDMSTObjc2Class *cls, struct SDMSTObjc2Class *parentClass, CoreRange dataRange, uint64_t offset) {
	struct SDMSTObjcClass *newClass = calloc(0x1, sizeof(struct SDMSTObjcClass));
	if (cls != parentClass) {
		if ((PtrAdd(offset, cls->isa >= Ptr(dataRange.offset)) && (PtrAdd(offset, cls->isa) < (PtrAdd(offset, (dataRange.offset + dataRange.length)))))) {
			newClass->superCls = SDMSTObjc2ClassCreateFromClass((cls->isa),cls, dataRange, offset);
			struct SDMSTObjc2ClassData *data = (struct SDMSTObjc2ClassData *)PtrAdd(cls->data, offset);
			newClass->className = Ptr(PtrAdd(data->name, offset));
			
			struct SDMSTObjc2ClassIVarInfo *ivarInfo = ((struct SDMSTObjc2ClassIVarInfo*)PtrAdd(data->ivar, offset));
			if (ivarInfo && (uint64_t)ivarInfo != offset) {
				newClass->ivarCount = ivarInfo->count;
				newClass->ivar = calloc(newClass->ivarCount, sizeof(struct SDMSTObjcIVar));
				struct SDMSTObjc2ClassIVar *ivarOffset = (struct SDMSTObjc2ClassIVar *)PtrAdd(ivarInfo, sizeof(struct SDMSTObjc2ClassIVarInfo));
				for (uint32_t i = 0x0; i < newClass->ivarCount; i++) {
					newClass->ivar[i].name = Ptr(PtrAdd(offset, ivarOffset[i].name));
					newClass->ivar[i].type = Ptr(PtrAdd(offset, ivarOffset[i].type));
					newClass->ivar[i].offset = (uintptr_t)(ivarOffset[i].offset);
				}
			}
			
			struct SDMSTObjc2ClassMethodInfo *methodInfo = ((struct SDMSTObjc2ClassMethodInfo*)PtrAdd(data->method, offset));
			if (methodInfo && (uint64_t)methodInfo != offset) {
				newClass->methodCount = methodInfo->count;
				newClass->method = calloc(newClass->methodCount, sizeof(struct SDMSTObjcMethod));
				struct SDMSTObjc2ClassMethod *methodOffset = (struct SDMSTObjc2ClassMethod *)PtrAdd(methodInfo, sizeof(struct SDMSTObjc2ClassMethodInfo));
				for (uint32_t i = 0x0; i < newClass->methodCount; i++) {
					newClass->method[i].name = Ptr(PtrAdd(offset, methodOffset[i].name));
					newClass->method[i].type = Ptr(PtrAdd(offset, methodOffset[i].type));
					newClass->method[i].offset = (uintptr_t)(methodOffset[i].imp);
				}
			}
			
			struct SDMSTObjc2ClassProtcolInfo *protocolInfo = ((struct SDMSTObjc2ClassProtcolInfo*)PtrAdd(data->protocol, offset));
			if (protocolInfo && (uint64_t)protocolInfo != offset) {
				newClass->protocolCount = (uint32_t)(protocolInfo->count);
				newClass->protocol = calloc(newClass->protocolCount, sizeof(struct SDMSTObjcProtocol));
				struct SDMSTObjc2ClassProtocol *protocolOffset = (struct SDMSTObjc2ClassProtocol *)PtrAdd(protocolInfo, sizeof(struct SDMSTObjc2ClassProtcolInfo));
				for (uint32_t i = 0x0; i < newClass->protocolCount; i++) {
					newClass->protocol[i].offset = (uintptr_t)(protocolOffset[i].offset);
				}
			}
		}
	}
	return newClass;
}


#endif