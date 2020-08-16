//
// Created by Perfare on 2020/7/4.
//

#include "il2cpp.h"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "log.h"
#include "il2cpp-tabledefs.h"
#include IL2CPPCLASS

#define DO_API(r, n, p) r (*n) p

#include IL2CPPAPI

#undef DO_API

static void *il2cpp_handle = nullptr;
static uint64_t il2cpp_base = 0;

void init_il2cpp_api() {
#define DO_API(r, n, p) n = (r (*) p)dlsym(il2cpp_handle, #n)

#include IL2CPPAPI

#undef DO_API
}

uint64_t get_module_base(const char *module_name) {
    uint64_t addr = 0;
    char line[1024];
    uint64_t start = 0;
    uint64_t end = 0;
    char flags[5];
    char path[PATH_MAX];

    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp != nullptr) {
        while (fgets(line, sizeof(line), fp)) {
            strcpy(path, "");
            sscanf(line, "%" PRIx64"-%" PRIx64" %s %*" PRIx64" %*x:%*x %*u %s\n", &start, &end,
                   flags, path);
            if (strstr(path, module_name)) {
                addr = start;
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

std::string get_method_modifier(uint16_t flags) {
    std::stringstream outPut;
    auto access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access) {
        case METHOD_ATTRIBUTE_PRIVATE:
            outPut << "private ";
            break;
        case METHOD_ATTRIBUTE_PUBLIC:
            outPut << "public ";
            break;
        case METHOD_ATTRIBUTE_FAMILY:
            outPut << "protected ";
            break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM:
            outPut << "internal ";
            break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM:
            outPut << "protected internal ";
            break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC) {
        outPut << "static ";
    }
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) {
        outPut << "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT) {
            outPut << "override ";
        }
    } else if (flags & METHOD_ATTRIBUTE_FINAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT) {
            outPut << "sealed override ";
        }
    } else if (flags & METHOD_ATTRIBUTE_VIRTUAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT) {
            outPut << "virtual ";
        } else {
            outPut << "override ";
        }
    }
    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) {
        outPut << "extern ";
    }
    return outPut.str();
}

std::string dump_method(Il2CppClass *klass) {
    std::stringstream outPut;
    if (klass->method_count > 0) {
        outPut << "\n\t// Methods\n";
        void *iter = nullptr;
        while (auto method = il2cpp_class_get_methods(klass, &iter)) {
            //TODO attribute
            if (method->methodPointer) {
                outPut << "\t// RVA: 0x";
                outPut << std::hex << (uint64_t) method->methodPointer - il2cpp_base;
                outPut << " VA: 0x";
                outPut << std::hex << (uint64_t) method->methodPointer;
            } else {
                outPut << "\t// RVA: 0x VA: 0x0";
            }
            if (method->slot != 65535) {
                outPut << " Slot: " << std::dec << method->slot;
            }
            outPut << "\n\t";
            outPut << get_method_modifier(method->flags);
            //TODO genericContainerIndex
            auto return_type = method->return_type;
            if (return_type->byref) {
                outPut << "ref ";
            }
            auto return_class = il2cpp_class_from_type(return_type);
            outPut << return_class->name << " " << method->name << "(";
            for (int i = 0; i < method->parameters_count; ++i) {
                auto parameters = method->parameters[i];
                auto parameter_type = parameters.parameter_type;
                auto attrs = parameter_type->attrs;
                if (parameter_type->byref) {
                    if (attrs & PARAM_ATTRIBUTE_OUT && !(attrs & PARAM_ATTRIBUTE_IN)) {
                        outPut << "out ";
                    } else if (attrs & PARAM_ATTRIBUTE_IN && !(attrs & PARAM_ATTRIBUTE_OUT)) {
                        outPut << "in ";
                    } else {
                        outPut << "ref ";
                    }
                } else {
                    if (attrs & PARAM_ATTRIBUTE_IN) {
                        outPut << "[In] ";
                    }
                    if (attrs & PARAM_ATTRIBUTE_OUT) {
                        outPut << "[Out] ";
                    }
                }
                auto parameter_class = il2cpp_class_from_type(parameter_type);
                outPut << parameter_class->name << " " << parameters.name;
                outPut << ", ";
            }
            if (method->parameters_count > 0) {
                outPut.seekp(-2, outPut.cur);
            }
            outPut << ") { }\n";
            //TODO GenericInstMethod
        }
    }
    return outPut.str();
}

std::string dump_property(Il2CppClass *klass) {
    std::stringstream outPut;
    if (klass->property_count > 0) {
        outPut << "\n\t// Properties\n";
        void *iter = nullptr;
        while (auto prop = il2cpp_class_get_properties(klass, &iter)) {
            //TODO attribute
            outPut << "\t";
            Il2CppClass *prop_class = nullptr;
            if (prop->get) {
                outPut << get_method_modifier(prop->get->flags);
                prop_class = il2cpp_class_from_type(prop->get->return_type);
            } else if (prop->set) {
                outPut << get_method_modifier(prop->set->flags);
                prop_class = il2cpp_class_from_type(prop->set->parameters[0].parameter_type);
            }
            outPut << prop_class->name << " " << prop->name << " { ";
            if (prop->get) {
                outPut << "get; ";
            }
            if (prop->set) {
                outPut << "set; ";
            }
            outPut << "}\n";
        }
    }
    return outPut.str();
}

std::string dump_field(Il2CppClass *klass) {
    std::stringstream outPut;
    if (klass->field_count > 0) {
        outPut << "\n\t// Fields\n";
        void *iter = nullptr;
        while (auto field = il2cpp_class_get_fields(klass, &iter)) {
            //TODO attribute
            outPut << "\t";
            auto attrs = field->type->attrs;
            auto access = attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
            switch (access) {
                case FIELD_ATTRIBUTE_PRIVATE:
                    outPut << "private ";
                    break;
                case FIELD_ATTRIBUTE_PUBLIC:
                    outPut << "public ";
                    break;
                case FIELD_ATTRIBUTE_FAMILY:
                    outPut << "protected ";
                    break;
                case FIELD_ATTRIBUTE_ASSEMBLY:
                case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
                    outPut << "internal ";
                    break;
                case FIELD_ATTRIBUTE_FAM_OR_ASSEM:
                    outPut << "protected internal ";
                    break;
            }
            if (attrs & FIELD_ATTRIBUTE_LITERAL) {
                outPut << "const ";
            } else {
                if (attrs & FIELD_ATTRIBUTE_STATIC) {
                    outPut << "static ";
                }
                if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) {
                    outPut << "readonly ";
                }
            }
            auto field_class = il2cpp_class_from_type(field->type);
            outPut << field_class->name << " " << field->name;
            outPut << "; // 0x" << std::hex << field->offset << "\n";
        }
    }
    return outPut.str();
}

std::string dump_type(const Il2CppType *type) {
    std::stringstream outPut;
    auto *klass = il2cpp_class_from_type(type);
    outPut << "\n// Namespace: " << klass->namespaze << "\n";
    auto flags = klass->flags;
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) {
        outPut << "[Serializable]\n";
    }
    //TODO attribute
    auto visibility = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
    switch (visibility) {
        case TYPE_ATTRIBUTE_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_PUBLIC:
            outPut << "public ";
            break;
        case TYPE_ATTRIBUTE_NOT_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
            outPut << "internal ";
            break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE:
            outPut << "private ";
            break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY:
            outPut << "protected ";
            break;
        case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
            outPut << "protected internal ";
            break;
    }
    if (flags & TYPE_ATTRIBUTE_ABSTRACT && flags & TYPE_ATTRIBUTE_SEALED) {
        outPut << "static ";
    } else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && flags & TYPE_ATTRIBUTE_ABSTRACT) {
        outPut << "abstract ";
    } else if (!klass->valuetype && !klass->enumtype && flags & TYPE_ATTRIBUTE_SEALED) {
        outPut << "sealed ";
    }
    if (flags & TYPE_ATTRIBUTE_INTERFACE) {
        outPut << "interface ";
    } else if (klass->enumtype) {
        outPut << "enum ";
    } else if (klass->valuetype) {
        outPut << "struct ";
    } else {
        outPut << "class ";
    }
    outPut << klass->name; //TODO genericContainerIndex
    std::vector<std::string> extends;
    if (!klass->valuetype && !klass->enumtype && klass->parent) {
        auto parent_type = il2cpp_class_get_type(klass->parent);
        if (parent_type->type != IL2CPP_TYPE_OBJECT) {
            extends.emplace_back(klass->parent->name);
        }
    }
    if (klass->interfaces_count > 0) {
        void *iter = nullptr;
        while (auto itf = il2cpp_class_get_interfaces(klass, &iter)) {
            extends.emplace_back(itf->name);
        }
    }
    if (!extends.empty()) {
        outPut << " : " << extends[0];
        for (int i = 1; i < extends.size(); ++i) {
            outPut << ", " << extends[i];
        }
    }
    outPut << " // TypeDefIndex: " << type->data.klassIndex << "\n{";
    outPut << dump_field(klass);
    outPut << dump_property(klass);
    outPut << dump_method(klass);
    //TODO EventInfo
    outPut << "}\n";
    return outPut.str();
}

void il2cpp_dump(void *handle, char *outDir) {
    LOGI("il2cpp_handle: %p", handle);
    il2cpp_handle = handle;
    init_il2cpp_api();
    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    uint32_t typeDefinitionsCount = 0;
    std::stringstream imageOutput;
    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        typeDefinitionsCount += image->typeCount;
        imageOutput << "// Image " << i << ": " << image->name << " - " << image->typeStart << "\n";
    }
    std::vector<std::string> outPuts(typeDefinitionsCount);
    LOGI("typeDefinitionsCount: %i", typeDefinitionsCount);
    il2cpp_base = get_module_base("libil2cpp.so");
    LOGI("il2cpp_base: %" PRIx64"", il2cpp_base);
#ifdef VersionAboveV24
    //使用il2cpp_image_get_class
    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            auto klass = il2cpp_image_get_class(image, j);
            auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
            //LOGD("type name : %s", il2cpp_type_get_name(type));
            auto klassIndex = type->data.klassIndex;
            if (outPuts[klassIndex].empty()) {
                outPuts[klassIndex] = dump_type(type);
            }
        }
    }
#else
    //使用反射
    auto corlib = il2cpp_get_corlib();
    auto assemblyClass = il2cpp_class_from_name(corlib, "System.Reflection", "Assembly");
    auto assemblyLoad = il2cpp_class_get_method_from_name(assemblyClass, "Load", 1);
    auto assemblyGetTypes = il2cpp_class_get_method_from_name(assemblyClass, "GetTypes", 0);
    if (assemblyLoad && assemblyLoad->methodPointer) {
        LOGI("Assembly::Load: %p", assemblyLoad->methodPointer);
    } else {
        LOGI("miss Assembly::Load");
        return;
    }
    if (assemblyGetTypes && assemblyGetTypes->methodPointer) {
        LOGI("Assembly::GetTypes: %p", assemblyGetTypes->methodPointer);
    } else {
        LOGI("miss Assembly::GetTypes");
        return;
    }
#ifdef VersionAboveV24
    typedef void *(*Assembly_Load_ftn)(Il2CppString * , void * );
#else
    typedef void *(*Assembly_Load_ftn)(void *, Il2CppString *, void *);
#endif
    typedef Il2CppArray *(*Assembly_GetTypes_ftn)(void *, void *);
    LOGI("dumping...");
    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        //LOGD("image name : %s", image->name);
        auto imageName = std::string(image->name);
        auto pos = imageName.rfind('.');
        auto imageNameNoExt = imageName.substr(0, pos);
        auto assemblyFileName = il2cpp_string_new(imageNameNoExt.c_str());
#ifdef VersionAboveV24
        auto reflectionAssembly = ((Assembly_Load_ftn) assemblyLoad->methodPointer)(
                assemblyFileName, nullptr);
#else
        auto reflectionAssembly = ((Assembly_Load_ftn) assemblyLoad->methodPointer)(nullptr,
                                                                                    assemblyFileName,
                                                                                    nullptr);
#endif
        auto reflectionTypes = ((Assembly_GetTypes_ftn) assemblyGetTypes->methodPointer)(
                reflectionAssembly, nullptr);
        auto items = reflectionTypes->vector;
        for (int j = 0; j < reflectionTypes->max_length; ++j) {
            auto klass = il2cpp_class_from_system_type((Il2CppReflectionType *) items[j]);
            auto type = il2cpp_class_get_type(klass);
            //LOGD("type name : %s", il2cpp_type_get_name(type));
            auto klassIndex = type->data.klassIndex;
            if (outPuts[klassIndex].empty()) {
                outPuts[klassIndex] = dump_type(type);
            }
        }
    }
#endif
    LOGI("write dump file");
    auto outPath = std::string(outDir).append("/files/dump.cs");
    std::ofstream outStream(outPath);
    outStream << imageOutput.str();
    for (int i = 0; i < typeDefinitionsCount; ++i) {
        if (!outPuts[i].empty()) {
            outStream << outPuts[i];
        } else {
            // <Module> always missing
            //LOGW("miss typeDefinition: %d", i);
        }
    }
    outStream.close();
    LOGI("dump done!");
}