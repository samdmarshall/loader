//
//  loader.h
//  loader
//
//  Created by Sam Marshall on 2/17/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_loader_h
#define loader_loader_h

#pragma mark -
#pragma mark #include
#include <mach/machine.h>
#include <mach/vm_prot.h>
#include "util.h"
#include "objc_runtime.h"

#pragma mark -
#pragma mark Private Types

struct loader_magic {
	uint32_t magic;
} ATR_PACK;

struct loader_arch {
	cpu_type_t cputype;
	cpu_subtype_t subtype;
} ATR_PACK;

struct loader_fat_header {
	struct loader_magic magic;
	uint32_t n_arch;
} ATR_PACK;

struct loader_generic_header {
	struct loader_magic magic;
	struct loader_arch arch;
	uint32_t filetype;
	uint32_t ncmds;
	uint32_t sizeofcmds;
	uint32_t flags;
} ATR_PACK;

struct loader_32_header {
	struct loader_generic_header info;
} ATR_PACK;

struct loader_64_header {
	struct loader_generic_header info;
	uint32_t reserved;
} ATR_PACK;

struct loader_loadcmd {
	uint32_t cmd;
	uint32_t cmdsize;
} ATR_PACK;

struct loader_64_position {
	uint64_t addr;
	uint64_t size;
} ATR_PACK;

struct loader_32_position {
	uint32_t addr;
	uint32_t size;
} ATR_PACK;

struct loader_segment {
	struct loader_loadcmd command;
	char segname[16];
} ATR_PACK;

struct loader_segment_data_64 {
	struct loader_64_position position;
	uint64_t fileoff;
} ATR_PACK;

struct loader_segment_data_32 {
	struct loader_32_position position;
	uint32_t fileoff;
} ATR_PACK;

struct loader_segment_info {
	vm_prot_t maxprot;
	vm_prot_t initprot;
	uint32_t nsects;
	uint32_t flags;
} ATR_PACK;

struct loader_segment_64 {
	struct loader_segment segment;
	struct loader_segment_data_64 data;
	struct loader_segment_info info;
} ATR_PACK;

struct loader_segment_32 {
	struct loader_segment segment;
	struct loader_segment_data_32 data;
	struct loader_segment_info info;
} ATR_PACK;

struct loader_section_name {
	char sectname[16];
	char segname[16];
} ATR_PACK;

struct loader_section_info {
	uint32_t offset;
	uint32_t align;
	uint32_t reloff;
	uint32_t nreloc;
	uint32_t flags;
	uint32_t reserved1;
	uint32_t reserved2;
} ATR_PACK;

struct loader_section_64 {
	struct loader_section_name name;
	struct loader_64_position position;
	struct loader_section_info info;
} ATR_PACK;

struct loader_section_32 {
	struct loader_section_name name;
	struct loader_32_position position;
	struct loader_section_info info;
} ATR_PACK;

struct loader_generic_nlist {
	union {
		uint32_t n_strx;
	} n_un;
	uint8_t n_type;
	uint8_t n_sect;
	uint16_t n_desc;
} ATR_PACK;

#pragma mark -
#pragma mark Public Types

struct loader_segment_map {
	struct loader_segment *text;
	struct loader_segment *link;
	struct loader_segment *objc;
} ATR_PACK;

struct loader_symbol {
	uint32_t symbol_number;
	uintptr_t offset;
	char *symbol_name;
	bool stub;
} ATR_PACK;

struct loader_symtab {
	uintptr_t *symtab;
	struct loader_symbol *symbol;
	uint64_t count;
} ATR_PACK;

struct loader_dependency_map {
	uintptr_t *dependency;
	uint32_t count;
} ATR_PACK;

struct loader_function_start {
	struct loader_loadcmd loadcmd;
	struct loader_32_position position;
} ATR_PACK;

struct loader_map {
	struct loader_segment_map *segment_map;
	struct loader_symtab *symbol_table;
	struct loader_dependency_map *dependency_map;
	struct loader_function_start *function_start;
} ATR_PACK;

struct loader_binary {
	uint32_t image_index;
	struct loader_generic_header *header;
	struct loader_map *map;
	struct SDMSTObjc *objc;
} ATR_PACK;

#pragma mark -
#pragma mark Functions

bool SDMBinaryIs64Bit(struct loader_generic_header *header);

struct loader_binary * SDMLoadBinaryWithPath(char *path);
void SDMReleaseBinary(struct loader_binary *binary);

#endif
