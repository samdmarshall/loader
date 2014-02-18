//
//  loader.c
//  loader
//
//  Created by Sam Marshall on 2/17/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_loader_c
#define loader_loader_c

#pragma mark -
#pragma mark #include
#include "loader.h"
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <string.h>

#pragma mark -
#pragma mark Private Types

#define kStubName "__sdmst_stub_"

#pragma mark -
#pragma mark Private Functions

bool SDMIsBinaryLoaded(char *path, struct loader_binary * binary) {
	bool isLoaded = false;
	uint32_t count = _dyld_image_count();
	for (uint32_t index = 0; index < count; index++) {
		const char *image_name = _dyld_get_image_name(index);
		if (strncmp(image_name, path, strlen(image_name)) == 0) {
			isLoaded = true;
			binary->image_index = index;
			binary->header = PtrCast(_dyld_get_image_header(index), struct loader_generic_header *);
			break;
		}
	}
	return isLoaded;
}

struct loader_map * SDMCreateBinaryMap(struct loader_generic_header * header) {
	struct loader_map *map = calloc(1, sizeof(struct loader_map));
	map->segment_map = calloc(1, sizeof(struct loader_segment_map));
	
	map->symbol_table = calloc(1, sizeof(struct loader_symtab));
	
	map->dependency_map = calloc(1, sizeof(struct loader_dependency_map));
	map->dependency_map->dependency = calloc(1, sizeof(uintptr_t));
	map->dependency_map->count = 0;
	
	bool is64Bit = SDMBinaryIs64Bit(header);
	struct loader_loadcmd *loadCmd = (struct loader_loadcmd *)PtrAdd(header, (is64Bit ? sizeof(struct loader_64_header) : sizeof(struct loader_32_header)));
	for (uint32_t command_index = 0; command_index < header->ncmds; command_index++) {
		switch (loadCmd->cmd) {
			case LC_SYMTAB: {
				map->symbol_table->symtab = PtrCast(loadCmd, uintptr_t*);
				break;
			};
			case LC_SEGMENT:
			case LC_SEGMENT_64: {
				struct loader_segment *segment = PtrCast(loadCmd, struct loader_segment *);
				if ((map->segment_map->text == NULL) && !strncmp(SEG_TEXT,segment->segname,sizeof(segment->segname))) {
					map->segment_map->text = segment;
				}
				if ((map->segment_map->link == NULL) && !strncmp(SEG_LINKEDIT,segment->segname,sizeof(segment->segname))) {
					map->segment_map->link = segment;
				}
				if ((map->segment_map->objc == NULL) && !strncmp((is64Bit ? SEG_DATA : SEG_OBJC), segment->segname, sizeof(segment->segname))) {
					map->segment_map->objc = segment;
				}
				break;
			};
			case LC_LOAD_DYLIB: {
				map->dependency_map->dependency = realloc(map->dependency_map->dependency, sizeof(uintptr_t)*(map->dependency_map->count+1));
				map->dependency_map->dependency[map->dependency_map->count] = *PtrCast(loadCmd, uintptr_t*);
				map->dependency_map->count++;
				break;
			};
			case LC_FUNCTION_STARTS: {
				map->function_start = PtrCast(loadCmd, struct loader_function_start *);
				break;
			};
			default: {
				break;
			};
		}
		loadCmd = (struct loader_loadcmd *)PtrAdd(loadCmd, loadCmd->cmdsize);
	}
	return map;
}

uint64_t SDMComputeFslide(struct loader_segment_map * segment_map, bool is64Bit) {
	uint64_t fslide = 0;
	if (is64Bit) {
		struct loader_segment_64 *text_segment = PtrCast(segment_map->text, struct loader_segment_64 *);
		struct loader_segment_64 *link_segment = PtrCast(segment_map->link, struct loader_segment_64 *);
		fslide = (uint64_t)(link_segment->data.position.addr - text_segment->data.position.addr) - link_segment->data.fileoff;
	} else {
		struct loader_segment_32 *text_segment = PtrCast(segment_map->text, struct loader_segment_32 *);
		struct loader_segment_32 *link_segment = PtrCast(segment_map->link, struct loader_segment_32 *);
		fslide = (uint64_t)(link_segment->data.position.addr - text_segment->data.position.addr) - link_segment->data.fileoff;
	}
	return fslide;
}

void SDMGenerateSymbols(struct loader_binary * binary) {
	uintptr_t symbol_address = 0;
	binary->map->symbol_table->symbol = calloc(1, sizeof(struct loader_symbol));
	binary->map->symbol_table->count = 0;
	struct symtab_command * symtab_cmd = PtrCast(binary->map->symbol_table->symtab, struct symtab_command *);
	bool is64Bit = SDMBinaryIs64Bit(binary->header);
	uint64_t fslide = SDMComputeFslide(binary->map->segment_map, is64Bit);
	struct loader_generic_nlist *entry = (struct loader_generic_nlist *)PtrAdd(binary->header, (symtab_cmd->symoff + fslide));
	for (uint32_t symbol_index = 0; symbol_index < symtab_cmd->nsyms; symbol_index++) {
		if (!(entry->n_type & N_STAB) && ((entry->n_type & N_TYPE) == N_SECT)) {
			char *strTable = PtrAdd(binary->header, (symtab_cmd->stroff + fslide));
			if (is64Bit) {
				uint64_t *n_value = (uint64_t*)PtrAdd(entry, sizeof(struct loader_generic_nlist));
				symbol_address = (uintptr_t)*n_value;
			} else {
				uint32_t *n_value = (uint32_t*)PtrAdd(entry, sizeof(struct loader_generic_nlist));
				symbol_address = (uintptr_t)*n_value;
			}
			binary->map->symbol_table->symbol = realloc(binary->map->symbol_table->symbol, sizeof(struct loader_symbol)*(binary->map->symbol_table->count+0x1));
			struct loader_symbol *symbol = (struct loader_symbol *)calloc(1, sizeof(struct loader_symbol));
			if (symbol) {
				symbol->symbol_number = symbol_index;
				symbol->offset = (uintptr_t)PtrAdd(symbol_address, _dyld_get_image_vmaddr_slide(binary->image_index));
				if (entry->n_un.n_strx && (entry->n_un.n_strx < symtab_cmd->strsize)) {
					symbol->symbol_name = PtrAdd(strTable, entry->n_un.n_strx);
					symbol->stub = false;
				} else {
					symbol->symbol_name = calloc(1 + strlen(kStubName) + ((binary->map->symbol_table->count==0) ? 1 : (uint32_t)log10(binary->map->symbol_table->count) + 0x1), sizeof(char));
					sprintf(symbol->symbol_name, "%s%llu", kStubName, binary->map->symbol_table->count);
					symbol->stub = true;
				}
				memcpy(&(binary->map->symbol_table->symbol[binary->map->symbol_table->count]), symbol, sizeof(struct loader_symbol));
				free(symbol);
				binary->map->symbol_table->count++;
			}
		}
		entry = (struct loader_generic_nlist *)PtrAdd(entry, (sizeof(struct loader_generic_nlist) + (is64Bit ? sizeof(uint64_t) : sizeof(uint32_t))));
	}
}

bool SDMMapObjcClasses32(struct loader_binary * binary) {
	bool result = (binary->map->segment_map->objc ? true : false);
	if (result) {
		struct SDMSTObjc *objc_data = calloc(1, sizeof(struct SDMSTObjc));
		struct loader_segment_32 *objc_segment = ((struct loader_segment_32 *)(binary->map->segment_map->objc));
		uint32_t module_count = 0;
		struct loader_section_32 *section = (struct loader_section_32 *)PtrAdd(objc_segment, sizeof(struct loader_segment_32));
		uint32_t section_count = objc_segment->info.nsects;
		struct SDMSTObjcModuleRaw *module = NULL;
		for (uint32_t index = 0; index < section_count; index++) {
			uint64_t mem_offset = _dyld_get_image_vmaddr_slide(binary->image_index);
			char *sectionName = Ptr(section->name.sectname);
			if (strncmp(sectionName, kObjc1ModuleInfo, sizeof(char[16])) == 0) {
				module = (struct SDMSTObjcModuleRaw *)PtrAdd(binary->header, section->info.offset);
				module_count = (section->position.size)/sizeof(struct SDMSTObjcModuleRaw);
			}
			if (strncmp(sectionName, kObjc1Class, sizeof(char[16])) == 0) {
				objc_data->classRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1Category, sizeof(char[16])) == 0) {
				objc_data->catRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1Protocol, sizeof(char[16])) == 0) {
				objc_data->protRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1ClsMeth, sizeof(char[16])) == 0) {
				objc_data->clsMRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1InstMeth, sizeof(char[16])) == 0) {
				objc_data->instMRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			section = (struct loader_section_32 *)PtrAdd(section, sizeof(struct loader_section_32));
		}
		if (module_count) {
			objc_data->cls = calloc(1, sizeof(struct SDMSTObjcClass));
			objc_data->clsCount = 0;
			uint64_t mem_offset = 0;
			for (uint32_t index = 0; index < module_count; index++) {
				struct SDMSTObjc1Symtab *symtab = (struct SDMSTObjc1Symtab *)PtrAdd(mem_offset, module[index].symtab);
				SDMSTObjc1CreateClassFromSymbol(objc_data, symtab);
			}
		}
		binary->objc = objc_data;
	}
	return result;
}

bool SDMMapObjcClasses64(struct loader_binary * binary) {
	bool result = (binary->map->segment_map->objc ? true : false);
	if (result) {
		struct SDMSTObjc *objc_data = calloc(1, sizeof(struct SDMSTObjc));
		struct loader_segment_64 *objc_segment = ((struct loader_segment_64 *)(binary->map->segment_map->objc));
		uint64_t mem_offset = _dyld_get_image_vmaddr_slide(binary->image_index) & k32BitMask;
		CoreRange data_range = CoreRangeCreate((uintptr_t)((uint64_t)(objc_segment->data.position.addr)+((uint64_t)mem_offset)),objc_segment->data.position.size);
		struct loader_section_64 *section = (struct loader_section_64 *)PtrAdd(objc_segment, sizeof(struct loader_section_64));
		uint32_t section_count = objc_segment->info.nsects;
		for (uint32_t index = 0; index < section_count; index++) {
			char *section_name = Ptr(section->name.sectname);
			if (strncmp(section_name, kObjc2ClassList, sizeof(char[16])) == 0) {
				objc_data->clsCount = (uint32_t)((section->position.size)/sizeof(uint64_t));
				break;
			}
			section = (struct loader_section_64 *)PtrAdd(section, sizeof(struct loader_section_64));
		}
		if (objc_data->clsCount) {
			objc_data->cls = calloc(objc_data->clsCount, sizeof(struct SDMSTObjcClass));
			for (uint32_t index = 0; index < objc_data->clsCount; index++) {
				uint64_t *classes = (uint64_t*)PtrAdd(section->position.addr, mem_offset);
				struct SDMSTObjc2Class *cls = (struct SDMSTObjc2Class *)PtrAdd(mem_offset, classes[index]);
				struct SDMSTObjcClass *objclass = SDMSTObjc2ClassCreateFromClass(cls, 0, data_range, mem_offset);
				memcpy(&(objc_data->cls[index]), objclass, sizeof(struct SDMSTObjcClass));
				free(objclass);
			}
		}
		binary->objc = calloc(1, sizeof(struct SDMSTObjc));
		memcpy(binary->objc, objc_segment, sizeof(SDMSTObjc));
		free(objc_segment);
	}
	return result;
}


#pragma mark -
#pragma mark Functions

bool SDMBinaryIs64Bit(struct loader_generic_header *header) {
	bool isCPU64Bit = ((header->arch->cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64);
	bool isMagic64Bit = (header->magic->magic == MH_MAGIC_64 || header->magic->magic == MH_CIGAM_64);
	return (isCPU64Bit && isMagic64Bit);
}

struct loader_binary * SDMLoadBinaryWithPath(char *path) {
	struct loader_binary *binary = calloc(1, sizeof(struct loader_binary));
	bool inMemory = SDMIsBinaryLoaded(path, binary);
	if (inMemory) {
		binary->map = SDMCreateBinaryMap(binary->header);
		SDMGenerateSymbols(binary);
		bool loadedObjc = false;
		bool is64Bit = SDMBinaryIs64Bit(binary->header);
		if (is64Bit) {
			loadedObjc = SDMMapObjcClasses64(binary);
		} else {
			loadedObjc = SDMMapObjcClasses32(binary);
		}
	} else {
		SDMReleaseBinary(binary);
		binary = NULL;
	}
	return binary;
}

void SDMReleaseBinary(struct loader_binary *binary) {
	free(binary);
}

#endif