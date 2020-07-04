#ifndef WHALE_PUBLIC_H_
#define WHALE_PUBLIC_H_

#define NULLABLE

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void WInlineHookFunction(void *address, void *replace, void **backup);

void WImportHookFunction(const char *name, NULLABLE const char *libname, void *replace, void **backup);

void *WDynamicLibOpen(const char *name);

void *WDynamicLibOpenAlias(const char *name, const char *path);

void *WDynamicLibSymbol(void *handle, const char *name);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // WHALE_PUBLIC_H_
