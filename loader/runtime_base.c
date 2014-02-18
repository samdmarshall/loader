//
//  runtime_base.c
//  loader
//
//  Created by Sam Marshall on 12/29/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_runtime_base_c
#define loader_runtime_base_c

#include "runtime_base.h"
#include <objc/runtime.h>
#include <string.h>

// SDM: SDMRuntimeBase.s calls
extern void SDMGenericGetSetInterceptor(void);

// SDM: this exists because we need the size of `Method`
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
typedef struct objc_method MethodStruct;
#pragma clang diagnostic pop

#define SDMGetSet (IMP)SDMGenericGetSetInterceptor

#define SDMCreateGetter(ReturnType, obj, selector) ((ReturnType (*)(id, SEL))SDMGenericGetSetInterceptor)(obj, selector)
#define SDMCallGetSetBlock(block) ((void (^)(id, SEL))block)

BOOL SDMCanRegisterForIvarInClass(char *ivarName, Class class);
BOOL SDMCanRegisterForPropertyInClass(char *propertyName, Class class);

extern IMP SDMFireGetterSetterNotificationsAndReturnIMP(id self, SEL _cmd);

#pragma mark -
#pragma mark Private Calls

BOOL SDMCanRegisterForIvarInClass(char *ivarName, Class class) {
	BOOL registerStatus = NO;
	Ivar ivar = class_getInstanceVariable(class, ivarName);
	if (ivar) {
		registerStatus = YES;
	}
	return registerStatus;
}

BOOL SDMCanRegisterForPropertyInClass(char *propertyName, Class class) {
	BOOL registerStatus = NO;
	objc_property_t property = class_getProperty(class, propertyName);
	if (property) {
		registerStatus = YES;
	}
	return registerStatus;
}

IMP observerResolveInstanceMethod(id self, SEL _cmd, ...) {
	IMP resolved = nil;
	printf("we are being called!");
	return resolved;
}

char* SDMGenerateGetterName(char *keyName) {
	unsigned long length = sizeof(char)*strlen(keyName);
	char *hasGet = calloc(0x1, length);
	memcpy(hasGet, keyName, length);
	return hasGet;
}

char* SDMGenerateObserver(char *selName) {
	char *observe = "Observer";
	unsigned long oLength = strlen(observe);
	unsigned long selLength = strlen(selName);
	unsigned long totalLength = sizeof(char)*(selLength+oLength+1);
	char *observerSelector = calloc(0x1, totalLength);
	memcpy(observerSelector, observe, oLength);
	memcpy(&(observerSelector[oLength]), selName, selLength);
	return observerSelector;
}

char* SDMGenerateSetterName(char *keyName) {
	unsigned long length = sizeof(char)*strlen(keyName);
	unsigned long totalLength = length+0x4;
	char *hasSet = calloc(0x1, totalLength);
	memcpy(hasSet, "set", 0x3);
	memcpy(&(hasSet[0x3]), keyName, length);
	hasSet[0x3] = toupper(hasSet[0x3]);
	memcpy(&(hasSet[totalLength-0x1]), ":", 0x1);
	return hasSet;
}

char* SDMGenerateMethodSignature(Method method) {
	unsigned int count = method_getNumberOfArguments(method);
	char *signature = calloc(0x1, sizeof(char)+0x2);
	method_getReturnType(method, signature, 0x2);
	
	for (unsigned int i = 0x0; i < count; i++) {
		unsigned int length = sizeof(char)*0x100;
		char *buffer = calloc(0x1, length);
		method_getArgumentType(method, i, buffer, length);
		unsigned long sigLength = sizeof(char)*(strlen(signature)+strlen(buffer));
		signature = realloc(signature, sigLength);
		strlcat(signature, buffer, sigLength);
		free(buffer);
	}
	return signature;
}

IMP SDMFireGetterSetterNotificationsAndReturnIMP(id self, SEL _cmd) {
	char *originalSelector = (char*)sel_getName(_cmd);
	char *resolveSelector;
	BOOL observedSelector = NO;
	BOOL isGetter = NO;
	BOOL isSetter = NO;
	IMP originalImplementation;
	struct MethodCalls *originalMethods = nil;
	struct ObserverArray *observers = (struct ObserverArray *)objc_getAssociatedObject(self, self);
	if (observers) {
		uint32_t observableCount = observers->count;
		uint32_t index;
		for (index = 0x0; index < observableCount; index++) {
			char *observableGetter = observers->array[index].getName;
			char *observableSetter = observers->array[index].setName;
			if (strncmp(originalSelector, observableGetter, strlen(originalSelector)) == 0x0) {
				observedSelector = isGetter = YES;
				break;
			}
			if (strncmp(originalSelector, observableSetter, strlen(originalSelector)) == 0x0) {
				observedSelector = isSetter = YES;
				break;
			}
		}
		if (isGetter) {
			originalMethods = (struct MethodCalls *)objc_getAssociatedObject(self, observers->array[index].getName);
			SDMCallGetSetBlock(originalMethods->block)(self, _cmd);
		}
		
		if (isSetter) {
			originalMethods = (struct MethodCalls *)objc_getAssociatedObject(self, observers->array[index].setName);
			SDMCallGetSetBlock(originalMethods->block)(self, _cmd);
		}
	}
	if (!observedSelector) {
		originalMethods = calloc(0x1, sizeof(struct MethodCalls));
		resolveSelector = SDMGenerateObserver(originalSelector);		
		originalMethods->originalCall = class_getInstanceMethod((Class)object_getClass(self), sel_registerName(resolveSelector));
		originalMethods->switchCall = class_getInstanceMethod((Class)object_getClass(self), _cmd);
	}
	originalImplementation = method_getImplementation(originalMethods->originalCall);
	return originalImplementation;
}

#pragma mark -
#pragma mark Public Calls

BOOL SDMRegisterCallbacksForKeyInInstance(BlockPointer getObserve, BlockPointer setObserve, char *keyName, id instance) {
	BOOL registerStatus = NO;
	BOOL registerGetStatus = NO;
	BOOL registerSetStatus = NO;
	if ((getObserve && setObserve) && instance) {
		__block char *getName = 0x0;
		__block char *setName = 0x0;
		
		Class class = objc_getClass(object_getClassName(instance));
		BOOL isProperty = SDMCanRegisterForPropertyInClass(keyName, class);
		if (isProperty) {
			objc_property_t property = class_getProperty(class, keyName);
			if (property) {
				
				char *hasGet = property_copyAttributeValue(property, "G");
				if (!hasGet) {
					hasGet = SDMGenerateGetterName(keyName);
				}
				getName = hasGet;
				
				char *isImmutable = property_copyAttributeValue(property, "R");
				if (!isImmutable) {
					char *hasSet = property_copyAttributeValue(property, "S");
					if (!hasSet) {
						hasSet = SDMGenerateSetterName(keyName);
					}
					setName = hasSet;
				}
			}
		} else {
			BOOL isIVar = SDMCanRegisterForIvarInClass(keyName, class);
			if (isIVar) {
				getName = SDMGenerateGetterName(keyName);
				setName = SDMGenerateSetterName(keyName);
			}
		}
		
		__block struct MethodNames *originalMethods;
		id associatedObject = objc_getAssociatedObject(instance, instance);
		if (associatedObject) {
			__block BOOL existingObserverForKey = NO;
			__block struct ObserverArray *existingObservers = PtrCast(associatedObject, struct ObserverArray*);
			__block uint32_t index;
			dispatch_sync(existingObservers->operationsQueue, ^{
				for (index = 0x0; index < existingObservers->count; index++) {
					char *key = existingObservers->array[index].keyName;
					if (strncmp(keyName, key, strlen(keyName)) == 0x0) {
						existingObserverForKey = YES;
						// SDM: we have an existing observer registered.
						break;
					}
				}
				if (!existingObserverForKey) {
					if (index == existingObservers->count) {
						originalMethods = calloc(0x1, sizeof(struct MethodNames));
						char *keyOperationsQueueName = calloc(0x1, 0x100);
						snprintf(keyOperationsQueueName,  0x100, "%s-%s-%p",class_getName(class),keyName,instance);
						originalMethods->keyQueue = dispatch_queue_create(keyOperationsQueueName, DISPATCH_QUEUE_SERIAL);
						originalMethods->keyName = calloc(0x1, strlen(keyName));
						memcpy(originalMethods->keyName, keyName, strlen(keyName));
						originalMethods->getName = calloc(0x1, strlen(getName));
						memcpy(originalMethods->getName, getName, strlen(getName));
						originalMethods->setName = calloc(0x1, strlen(setName));
						memcpy(originalMethods->setName, setName, strlen(setName));
						
						existingObservers->array = realloc(existingObservers->array, sizeof(struct MethodNames)*(existingObservers->count+0x1));
						existingObservers->count++;
						memcpy(&(existingObservers->array[existingObservers->count-0x1]), originalMethods, sizeof(struct MethodNames));
					}
				}
			});
			if (!existingObserverForKey) {
				if (index != existingObservers->count) {
					originalMethods = &(existingObservers->array[index]);
					if (originalMethods->isEnabled) {
						return registerStatus;
					}
				}
			} else {
				return registerStatus;
			}
		} else {
			originalMethods = calloc(0x1, sizeof(struct MethodNames));
			char *keyOperationsQueueName = calloc(0x1, 0x100);
			snprintf(keyOperationsQueueName,  0x100, "%s-%s-%p",class_getName(class),keyName,instance);
			originalMethods->keyQueue = dispatch_queue_create(keyOperationsQueueName, DISPATCH_QUEUE_SERIAL);
			originalMethods->keyName = calloc(0x1, strlen(keyName));
			memcpy(originalMethods->keyName, keyName, strlen(keyName));
			originalMethods->getName = calloc(0x1, strlen(getName));
			memcpy(originalMethods->getName, getName, strlen(getName));
			originalMethods->setName = calloc(0x1, strlen(setName));
			memcpy(originalMethods->setName, setName, strlen(setName));
			
			struct ObserverArray *observers = calloc(0x1, sizeof(struct ObserverArray));
			char *operationsQueueName = calloc(0x1, 0x100);
			snprintf(operationsQueueName,  0x100, "%s-observer-operations-queue",class_getName(class));
			observers->operationsQueue = dispatch_queue_create(operationsQueueName, DISPATCH_QUEUE_SERIAL);
			observers->array = originalMethods;
			observers->count = 0x1;
			objc_setAssociatedObject(instance, instance, PtrCast(observers, id), OBJC_ASSOCIATION_ASSIGN);
		}
		
		if (originalMethods->getName) {
			SEL realGetSelector = sel_registerName(originalMethods->getName);
			Method resolveGetter = class_getInstanceMethod(class, realGetSelector);
			SEL observerGetSelector;
			
			if (resolveGetter) {
				char *observerGetterName = SDMGenerateObserver(originalMethods->getName);
				observerGetSelector = sel_registerName(observerGetterName);
				char *getMethodSignature = SDMGenerateMethodSignature(resolveGetter);
				
				IMP getSelector = SDMGetSet;
				
				struct MethodCalls *methods = calloc(0x1, sizeof(struct MethodCalls));
				methods->block = getObserve;
				methods->switchCall = resolveGetter;
				BOOL addObserverGetter = class_addMethod(class, observerGetSelector, getSelector, getMethodSignature);
				if (addObserverGetter) {
					Method getter = class_getInstanceMethod(class, observerGetSelector);
					method_exchangeImplementations(resolveGetter, getter);
					Method observerGetter = calloc(0x1, sizeof(MethodStruct));
					memcpy(observerGetter, getter, sizeof(MethodStruct));
					methods->originalCall = observerGetter;
					objc_setAssociatedObject(instance, originalMethods->getName, PtrCast(methods,id), OBJC_ASSOCIATION_ASSIGN);
				} else {
					Method getTest = class_getInstanceMethod(class, observerGetSelector);
					if (getTest) {
						method_exchangeImplementations(resolveGetter, getTest);
						methods->originalCall = getTest;
						objc_setAssociatedObject(instance, originalMethods->getName, PtrCast(methods,id), OBJC_ASSOCIATION_ASSIGN);
						addObserverGetter = YES;
					}
				}
				registerGetStatus = addObserverGetter;
				free(observerGetterName);
			}
		}
		
		if (originalMethods->setName) {
			SEL realSetSelector = sel_registerName(originalMethods->setName);
			Method resolveSetter = class_getInstanceMethod(class, realSetSelector);
			
			if (resolveSetter) {
				char *observerSetterName = SDMGenerateObserver(originalMethods->setName);
				SEL observerSetSelector = sel_registerName(observerSetterName);
				char *setMethodSignature = SDMGenerateMethodSignature(resolveSetter);
				
				IMP setSelector = SDMGetSet;
				
				struct MethodCalls *methods = calloc(0x1, sizeof(struct MethodCalls));
				methods->block = setObserve;
				methods->switchCall = resolveSetter;
				BOOL addObserverSetter = class_addMethod(class, observerSetSelector, setSelector, setMethodSignature);
				if (addObserverSetter) {
					Method setter = class_getInstanceMethod(class, observerSetSelector);
					method_exchangeImplementations(resolveSetter, setter);
					Method observerSetter = calloc(0x1, sizeof(MethodStruct));
					memcpy(observerSetter, setter, sizeof(MethodStruct));
					methods->originalCall = observerSetter;
					objc_setAssociatedObject(instance, originalMethods->setName, PtrCast(methods,id), OBJC_ASSOCIATION_ASSIGN);
				} else {
					Method setTest = class_getInstanceMethod(class, observerSetSelector);
					if (setTest) {
						method_exchangeImplementations(resolveSetter, setTest);
						methods->originalCall = setTest;
						objc_setAssociatedObject(instance, originalMethods->setName, PtrCast(methods,id), OBJC_ASSOCIATION_ASSIGN);
						addObserverSetter = YES;
					}
				}
				registerSetStatus = addObserverSetter;
				free(observerSetterName);
			}
		}
		if (registerGetStatus) {
			registerStatus = YES;
			originalMethods->isEnabled = registerGetStatus;
		}
	}
	return registerStatus;
}

void SDMRemoveCallbackForKeyInInstance(char *keyName, id instance) {
	__block id associatedObject = objc_getAssociatedObject(instance, instance);
	if (associatedObject) {
		__block struct ObserverArray *observers = PtrCast(associatedObject, struct ObserverArray*);
		dispatch_sync(observers->operationsQueue, ^{
			for (uint32_t i = 0x0; i < observers->count; i++) {
				struct MethodNames *originalMethods = &(observers->array[i]);
				char *observerKey = originalMethods->keyName;
				if (strncmp(keyName, observerKey, strlen(keyName)) == 0x0) {
					
					id originalGetterValue = objc_getAssociatedObject(instance, originalMethods->getName);
					struct MethodCalls *observerGetterMethod = PtrCast(originalGetterValue,struct MethodCalls *);
					if (observerGetterMethod) {
						method_exchangeImplementations(observerGetterMethod->switchCall, observerGetterMethod->originalCall);
					}
					
					id originalSetterValue = objc_getAssociatedObject(instance, originalMethods->setName);
					struct MethodCalls *observerSetterMethod = PtrCast(originalSetterValue,struct MethodCalls *);
					if (observerSetterMethod) {
						method_exchangeImplementations(observerSetterMethod->switchCall, observerSetterMethod->originalCall);
					}
					
					originalMethods->isEnabled = NO;
				}
			}
		});
	}
}

#endif
