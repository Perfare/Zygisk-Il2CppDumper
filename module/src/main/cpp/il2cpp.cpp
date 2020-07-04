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
static uint64_t il2cpp_baseaddr = 0;

void init_il2cpp_api() {
#define DO_API(r, n, p) n = (r (*) p)dlsym(il2cpp_handle, #n)

#include IL2CPPAPI

#undef DO_API
}

//TODO 强化搜索，针对多个data字段
void process_maps(uint32_t typeDefinitionsCount, uint64_t *il2cpp_addr,
                  uint64_t *metadataregistration_addr) {
    char line[1024];

    bool flag = false;
    uint64_t start = 0;
    uint64_t end = 0;
    char flags[5];
    char path[PATH_MAX];

    uint64_t data_start = 0;
    uint64_t data_end = 0;

    uint64_t bss_start = 0;
    uint64_t bss_end = 0;

    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp != nullptr) {
        while (fgets(line, sizeof(line), fp)) {
            if (!flag && strstr(line, "libil2cpp.so")) {
                flag = true;
                char *pch = strtok(line, "-");
                *il2cpp_addr = strtoull(pch, nullptr, 16);
            }
            if (flag) {
                sscanf(line, "%" PRIx64"-%" PRIx64" %s %*" PRIx64" %*x:%*x %*u %s\n",
                       &start, &end, flags, path);
                if (strcmp(flags, "rw-p") == 0 && strstr(path, "libil2cpp.so")) {
                    LOGD("data start %" PRIx64"", start);
                    LOGD("data end %" PRIx64"", end);
                    data_start = start;
                    data_end = end;
                }
                if (strcmp(path, "[anon:.bss]") == 0) {
                    LOGD("bss start %" PRIx64"", start);
                    LOGD("bss end %" PRIx64"", end);
                    bss_start = start;
                    bss_end = end;
                    break;
                }
            }
        }
        fclose(fp);
    }

    auto search_addr = data_start;
    while (search_addr < data_end) {
#ifdef __LP64__
        search_addr += 8;
#else
        search_addr += 4;
#endif
        auto metadataRegistration = (Il2CppMetadataRegistration *) search_addr;
        if (metadataRegistration &&
            metadataRegistration->typeDefinitionsSizesCount == typeDefinitionsCount) {
            //LOGD("now: %" PRIx64"", search_addr - *il2cpp_addr);
            auto metadataUsages_addr = (uint64_t) metadataRegistration->metadataUsages;
            //LOGD("now2: %" PRIx64"", metadataUsages_addr);
            if (metadataUsages_addr >= data_start && metadataUsages_addr <= data_end) {
                flag = true;
                for (int i = 0; i < 5000; ++i) {
                    auto pointer_addr = (uint64_t) metadataRegistration->metadataUsages[i];
                    //LOGD("now3: %" PRIx64"", pointer_addr);
                    if ((pointer_addr < bss_start || pointer_addr > bss_end) &&
                        (pointer_addr < data_start || pointer_addr > data_end)) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    LOGD("metadataregistration_rva: %" PRIx64"", search_addr - *il2cpp_addr);
                    *metadataregistration_addr = search_addr;
                    break;
                }
            }
        }
    }
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
                outPut << std::hex << (uint64_t) method->methodPointer - il2cpp_baseaddr;
                outPut << " VA: 0x";
                outPut << std::hex << (uint64_t) method->methodPointer;
            } else {
                outPut << "\t// RVA: 0x VA: 0x0";
            }
            if (method->slot != 65535) {
                outPut << std::dec << " Slot: " << method->slot;
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
                //TODO DefaultValue
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
            //TODO DefaultValue
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
        void *iter = NULL;
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
    //TODO 2018.3.0f2(24.1)及以上版本可以使用il2cpp_image_get_class而不需要获取metadataregistration地址
    uint64_t metadataregistration_addr = 0;
    process_maps(typeDefinitionsCount, &il2cpp_baseaddr, &metadataregistration_addr);
    LOGI("il2cpp_addr: %" PRIx64"", il2cpp_baseaddr);
    LOGI("metadataregistration_addr: %" PRIx64"", metadataregistration_addr);
    if (metadataregistration_addr > 0) {
        auto *metadataRegistration = (Il2CppMetadataRegistration *) metadataregistration_addr;
        for (int i = 0; i < metadataRegistration->typesCount; ++i) {
            auto type = metadataRegistration->types[i];
            switch (type->type) {
                case IL2CPP_TYPE_VOID:
                case IL2CPP_TYPE_BOOLEAN:
                case IL2CPP_TYPE_CHAR:
                case IL2CPP_TYPE_I1:
                case IL2CPP_TYPE_U1:
                case IL2CPP_TYPE_I2:
                case IL2CPP_TYPE_U2:
                case IL2CPP_TYPE_I4:
                case IL2CPP_TYPE_U4:
                case IL2CPP_TYPE_I8:
                case IL2CPP_TYPE_U8:
                case IL2CPP_TYPE_R4:
                case IL2CPP_TYPE_R8:
                case IL2CPP_TYPE_STRING:
                case IL2CPP_TYPE_VALUETYPE:
                case IL2CPP_TYPE_CLASS:
                case IL2CPP_TYPE_TYPEDBYREF:
                case IL2CPP_TYPE_I:
                case IL2CPP_TYPE_U:
                case IL2CPP_TYPE_OBJECT:
                case IL2CPP_TYPE_ENUM: {
                    //LOGD("type name : %s", il2cpp_type_get_name(type));
                    auto klassIndex = type->data.klassIndex;
                    if (outPuts[klassIndex].empty()) {
                        outPuts[klassIndex] = dump_type(type);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        LOGI("write dump file");
        auto outPath = std::string(outDir).append("/files/dump.cs");
        std::ofstream outStream(outPath);
        outStream << imageOutput.str();
        for (int i = 0; i < typeDefinitionsCount; ++i) {
            if (!outPuts[i].empty()) {
                outStream << outPuts[i];
            } else {
                LOGW("miss typeDefinition: %d", i);
            }
        }
        outStream.close();
        LOGI("dump done!");
    }
}