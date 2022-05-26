#ifndef __PEFILE_CLR_TABLES_HPP__
#define __PEFILE_CLR_TABLES_HPP__

#include <stdint.h>

#include "clr_tables_helper.hpp"
#include "macros.h"


namespace pefile
{
	// ======================================================================
	using strings_index_t = detail::CLR_HEAP_INDEX<detail::CLR_HEAP_STRINGS>;
	using guid_index_t    = detail::CLR_HEAP_INDEX<detail::CLR_HEAP_GUIDS>;
	using blob_index_t    = detail::CLR_HEAP_INDEX<detail::CLR_HEAP_BLOBS>;
	// ======================================================================
	CLR_TABLES_SYSTEM_BEGIN(CLR)
		CLR_TABLE_DEF_BEGIN(Assembly, 0x20)
			CLR_ROW(uint32_t, HashAlgId)
			CLR_ROW(uint16_t, MajorVersion)
			CLR_ROW(uint16_t, MinorVersion)
			CLR_ROW(uint16_t, BuildNumber)
			CLR_ROW(uint16_t, RevisionNumber)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(blob_index_t, PublicKey)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(strings_index_t, Culture)
		CLR_TABLE_DEF_END(Assembly)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(AssemblyOS, 0x22)
			CLR_ROW(uint32_t, OSPlatformID)
			CLR_ROW(uint32_t, OSMajorVersion)
			CLR_ROW(uint32_t, OSMinorVersion)
		CLR_TABLE_DEF_END(AssemblyOS)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(AssemblyProcessor, 0x21)
			CLR_ROW(uint32_t, Processor)
		CLR_TABLE_DEF_END(AssemblyProcessor)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(AssemblyRef, 0x23)
			CLR_ROW(uint16_t, MajorVersion)
			CLR_ROW(uint16_t, MinorVersion)
			CLR_ROW(uint16_t, BuildNumber)
			CLR_ROW(uint16_t, RevisionNumber)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(blob_index_t, PublicKeyOrToken)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(strings_index_t, Culture)
			CLR_ROW(blob_index_t, HashValue)
		CLR_TABLE_DEF_END(AssemblyRef)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(AssemblyRefOS, 0x25)
			CLR_ROW(uint32_t, OSPlatformID)
			CLR_ROW(uint32_t, OSMajorVersion)
			CLR_ROW(uint32_t, OSMinorVersion)
			CLR_ROW(uint16_t, AssemblyRefIndex) // TODO
		CLR_TABLE_DEF_END(AssemblyRefOS)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(AssemblyRefProcessor, 0x24)
			CLR_ROW(uint32_t, Processor)
			CLR_ROW(uint16_t, AssemblyRefIndex) // TODO
		CLR_TABLE_DEF_END(AssemblyRefProcessor)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(ClassLayout, 0x0F)
			CLR_ROW(uint16_t, PackagingSize)
			CLR_ROW(uint32_t, ClassSize)
			CLR_ROW(uint16_t, Parent) // TODO
		CLR_TABLE_DEF_END(ClassLayout)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Constant, 0x0B)
			CLR_ROW(uint8_t, Type)
			CLR_ROW(uint8_t, Padding)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(blob_index_t, Value)
		CLR_TABLE_DEF_END(Constant)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(CustomAttribute, 0x0C)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(uint16_t, Type) // TODO
			CLR_ROW(blob_index_t, Value)
		CLR_TABLE_DEF_END(CustomAttribute)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(DeclSecurity, 0x0E)
			CLR_ROW(uint16_t, Action)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(blob_index_t, PermissionSet)
		CLR_TABLE_DEF_END(DeclSecurity)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(EventMap, 0x12)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(uint16_t, EventList) // TODO
		CLR_TABLE_DEF_END(EventMap)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(EventTable, 0x14)
			CLR_ROW(uint16_t, EventFlags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(uint16_t, EventType) // TODO
		CLR_TABLE_DEF_END(EventTable)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(ExportedType, 0x27)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(uint32_t, TypeDefId)
			CLR_ROW(strings_index_t, TypeName)
			CLR_ROW(strings_index_t, TypeNamespace)
			CLR_ROW(uint16_t, Implementation) // TODO
		CLR_TABLE_DEF_END(ExportedType)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Field, 0x04)
			CLR_ROW(uint16_t, Flags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(blob_index_t, Signature)
		CLR_TABLE_DEF_END(Field)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(FieldLayout, 0x10)
			CLR_ROW(uint32_t, Offset)
			CLR_ROW(uint16_t, Field) // TODO
		CLR_TABLE_DEF_END(FieldLayout)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(FieldMarshal, 0x0D)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(blob_index_t, NativeType)
		CLR_TABLE_DEF_END(FieldMarshal)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(FieldRVA, 0x1D)
			CLR_ROW(uint32_t, RVA) 
			CLR_ROW(uint16_t, Field) // TODO
		CLR_TABLE_DEF_END(FieldRVA)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(File, 0x26)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(blob_index_t, HashValue)
		CLR_TABLE_DEF_END(File)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(GenericParam, 0x2A)
			CLR_ROW(uint16_t, Number)
			CLR_ROW(uint16_t, Flags)
			CLR_ROW(strings_index_t, Name)
		CLR_TABLE_DEF_END(GenericParam)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(GenericParamConstraint, 0x2C)
			CLR_ROW(uint16_t, Owner) // TODO
			CLR_ROW(uint16_t, Constraint) // TODO
		CLR_TABLE_DEF_END(GenericParamConstraint)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(ImplMap, 0x1C)
			CLR_ROW(uint16_t, MappingFlags)
			CLR_ROW(uint16_t, MemberForwarded) // TODO
			CLR_ROW(strings_index_t, ImportName)
			CLR_ROW(uint16_t, ImportScope) // TODO
		CLR_TABLE_DEF_END(ImplMap)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(InterfaceImpl, 0x09)
			CLR_ROW(uint16_t, Class) // TODO
			CLR_ROW(uint16_t, Interface) // TODO
		CLR_TABLE_DEF_END(InterfaceImpl)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(ManifestResource, 0x28)
			CLR_ROW(uint32_t, Offset)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(uint16_t, Implementation)
		CLR_TABLE_DEF_END(ManifestResource)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(MemberRef, 0x0A)
			CLR_ROW(uint16_t, Class) // TODO
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(blob_index_t, Signature)
		CLR_TABLE_DEF_END(MemberRef)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(MethodDef, 0x06)
			CLR_ROW(uint32_t, RVA)
			CLR_ROW(uint16_t, ImplFlags)
			CLR_ROW(uint16_t, Flags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(blob_index_t, Signature)
			CLR_ROW(uint16_t, ParamList) // TODO
		CLR_TABLE_DEF_END(MethodDef)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(MethodImpl, 0x19)
			CLR_ROW(uint16_t, Class) // TODO
			CLR_ROW(uint16_t, MethodBody) // TODO
			CLR_ROW(uint16_t, MethodDeclaration) // TODO
		CLR_TABLE_DEF_END(MethodImpl)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(MethodSemantics, 0x18)
			CLR_ROW(uint16_t, Semantics)
			CLR_ROW(uint16_t, Method) // TODO
			CLR_ROW(uint16_t, Association) // TODO
		CLR_TABLE_DEF_END(MethodSemantics)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(MethodSpec, 0x2B)
			CLR_ROW(uint16_t, Method) // TODO
			CLR_ROW(blob_index_t, Instantiation)
		CLR_TABLE_DEF_END(MethodSpec)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Module, 0x00)
			CLR_ROW(uint16_t, Generation)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(guid_index_t, Mvid)
			CLR_ROW(guid_index_t, EncId)
			CLR_ROW(guid_index_t, EncBaseId)
		CLR_TABLE_DEF_END(Module)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(ModuleRef, 0x1A)
			CLR_ROW(strings_index_t, Name)
		CLR_TABLE_DEF_END(ModuleRef)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(NestedClass, 0x29)
			CLR_ROW(uint16_t, NestedKlass) // TODO
			CLR_ROW(uint16_t, EnclosingClass) // TODO
		CLR_TABLE_DEF_END(NestedClass)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Param, 0x08)
			CLR_ROW(uint16_t, Flags)
			CLR_ROW(uint16_t, Sequence)
			CLR_ROW(strings_index_t, Name)
		CLR_TABLE_DEF_END(Param)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Property, 0x17)
			CLR_ROW(uint16_t, Flags)
			CLR_ROW(strings_index_t, Name)
			CLR_ROW(blob_index_t, Type)
		CLR_TABLE_DEF_END(Property)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(PropertyMap, 0x15)
			CLR_ROW(uint16_t, Parent) // TODO
			CLR_ROW(uint16_t, PropertyList) // TODO
		CLR_TABLE_DEF_END(PropertyMap)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(StandaloneSig, 0x11)
			CLR_ROW(blob_index_t, Signature)
		CLR_TABLE_DEF_END(StandaloneSig)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Typedef, 0x02)
			CLR_ROW(uint32_t, Flags)
			CLR_ROW(strings_index_t, TypeName)
			CLR_ROW(strings_index_t, TypeNamespace)
			CLR_ROW(uint16_t, Extends) // TODO
			CLR_ROW(uint16_t, FieldList) // TODO
			CLR_ROW(uint16_t, MethodList) // TODO
		CLR_TABLE_DEF_END(Typedef)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Typeref, 0x01)
			CLR_ROW(uint16_t, ResolutionScope) // TODO
			CLR_ROW(strings_index_t, TypeName)
			CLR_ROW(strings_index_t, TypeNamespace)
		CLR_TABLE_DEF_END(Typeref)
		// ======================================================================
		CLR_TABLE_DEF_BEGIN(Typespec, 0x1B)
			CLR_ROW(blob_index_t, Signature)
		CLR_TABLE_DEF_END(Typespec)
	CLR_TABLES_SYSTEM_END;
	
} // ns pefile

#endif
